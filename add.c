#include "postgres.h"
#include "hstore.h"
#include "fmgr.h"
#include <string.h>
#include <utils/array.h>
#include <stdlib.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

typedef struct {
    char ** keys;
    char ** vstr;
    int  *  vals;
    int  *  sizes;
    size_t  used;
    size_t  size;
    bool *  found;
} Array;

HStore * hstorePairs( Pairs *pairs, int4 pcount, int4 buflen )
{
    HStore     *out;
    HEntry     *entry;
    char       *ptr;
    char       *buf;
    int4        len;
    int4        i;

    len = CALCDATASIZE( pcount, buflen );
    out = palloc( len );
    SET_VARSIZE( out, len );
    HS_SETCOUNT( out, pcount );

    if ( pcount == 0 )
        return out;

    entry = ARRPTR( out );
    buf = ptr = STRPTR( out );

    for( i = 0; i < pcount; i++ )
    {
        HS_ADDITEM( entry, buf, ptr, pairs[i] );
    }

    HS_FINALIZE( out, pcount, buf, ptr );

    return out;
}

void init_array( Array *a, size_t initial_size )
{
    int i = 0;
    a->keys  = ( char ** )palloc( initial_size * sizeof( char * ) );
    a->vstr  = ( char ** )palloc( initial_size * sizeof( char * ) );
    a->vals  = ( int  *  )palloc( initial_size * sizeof( int  * ) );
    a->sizes = ( int  *  )palloc( initial_size * sizeof( int  * ) );
    a->found = ( bool *  )palloc( initial_size * sizeof( bool * ) );
    a->used = 0;
    a->size = initial_size;
    for( ; i < initial_size; ++i )
    {
        a->vals[i]  = 0;
        a->sizes[i] = 0;
        a->found[i] = false;
    }
}

void insert_array( Array *a, char * key, int val, int elem_size )
{
    if( a->used == a->size )
    {
        int i = a->size;
        a->size *= 2;

        char ** keys_swap = a->keys;
        a->keys = ( char ** )palloc( a->size * sizeof( char * ) );
        memcpy( a->keys, keys_swap, sizeof( char * ) * i );
        pfree( keys_swap );

        char ** vstr_swap = a->vstr;
        a->vstr = ( char ** )palloc( a->size * sizeof( char * ) );
        memcpy( a->vstr, vstr_swap, sizeof( char * ) * i );
        pfree( vstr_swap );

        int * vals_swap = a->vals;
        a->vals = ( int * )palloc( a->size * sizeof( int ) );
        memcpy( a->vals, vals_swap, sizeof( int ) * i );
        pfree( vals_swap );

        int * sizes_swap = a->sizes;
        a->sizes = ( int * )palloc( a->size * sizeof( int ) );
        memcpy( a->sizes, sizes_swap, sizeof( int ) * i );
        pfree( sizes_swap );

        bool * found_swap = a->found;
        a->found = ( bool * )palloc( a->size * sizeof( bool ) );
        memcpy( a->found, found_swap, sizeof( bool ) * i );
        pfree( found_swap );

        for( ; i < a->size; ++i )
        {
            a->vals[i]  = 0;
            a->sizes[i] = 0;
            a->found[i] = false;
        }
    }
    a->keys[a->used]   = key;
    a->sizes[a->used]  = elem_size;
    a->vals[a->used++] = val;
}

void free_array( Array *a )
{
    int i;
    for( i = 0; i < a->used; ++i )
    {
        pfree( a->keys[i] );
    }
    for( i = 0; i < a->used; ++i )
    {
        pfree( a->vstr[i] );
    }
    pfree( a->keys );
    pfree( a->vstr );
    pfree( a->vals );
    pfree( a->sizes );
    pfree( a->found );
    a->keys = NULL;
    a->vstr = NULL;
    a->vals = NULL;
    a->found = NULL;
    a->used = a->size = 0;
}

HStore * hstoreUpgrade(Datum orig)
{
	HStore	   *hs = (HStore *) PG_DETOAST_DATUM(orig);
    return hs;
}

int get_digit_num( int number )
{
    if( number == 0 )
        return 1;
    size_t count = 0;
    while( number != 0 )
    {
        number /= 10;
        ++count;
    }
    return count;
}

void read_pair( HEntry * entries, char * base, int index, char ** key, int * vali, size_t * keylen )
{
    size_t vallen = HS_VALLEN( entries, index );
    char * val = palloc( ( vallen + 1 ) * sizeof( char ) );
    memset( val, '\0', vallen + 1 );
    memcpy(val, HS_VAL( entries, base, index ), vallen );

    *keylen = HS_KEYLEN( entries, index );
    *key = palloc( ( *keylen + 1 ) * sizeof( char ) );
    memset( *key, '\0', *keylen + 1 );
    memcpy(*key, HS_KEY( entries, base, index ), *keylen );
    *vali = atoi( val );

    pfree( val );
}

int min( int a, int b )
{
    return ( a < b ) ? a : b;
}

int compare( char * key1, int keylen1, char * key2, int keylen2 )
{
    if( keylen1 < keylen2 )
        return -1;
    if( keylen1 > keylen2 )
        return 1;

    int len = min( keylen1, keylen2 );
    int cmp = strncmp( key1, key2, len );

    return cmp;
}

PG_FUNCTION_INFO_V1( welle_add );

// works on sorted hstores only, returns sorted result
// further idea for improvement: work on array of hstores and use heap to
// select minimum key (avoids serialization of many hstores)
Datum welle_add( PG_FUNCTION_ARGS )
{
    if( PG_ARGISNULL( 0 ) || PG_ARGISNULL( 1 ) )
    {
        PG_RETURN_NULL();
    }

    HStore * hstore1 = PG_GETARG_HS( 0 );
    HStore * hstore2 = PG_GETARG_HS( 1 );
    HEntry * entries1 = ARRPTR( hstore1 );
    HEntry * entries2 = ARRPTR( hstore2 );
    char * base1 = STRPTR( hstore1 );
    char * base2 = STRPTR( hstore2 );
    int count1 = HS_COUNT( hstore1 );
    int count2 = HS_COUNT( hstore2 );
    int i,j;

    Array a;
    init_array( &a, 10 );

    int index1 = 0, index2 = 0;
    char * key1, * key2;
    int val1, val2;
    size_t keylen1, keylen2;

    // merge both lists by appending the smaller key
    // or the sum of the values if the keys equal
    while( index1 < count1 && index2 < count2 )
    {
        read_pair( entries1, base1, index1, &key1, &val1, &keylen1 );
        read_pair( entries2, base2, index2, &key2, &val2, &keylen2 );

        int cmp = compare( key1, keylen1, key2, keylen2 );
        if( cmp < 0 )
        {
            insert_array( &a, key1, val1, ( int )keylen1 );
            index1 += 1;
        }
        else if( cmp > 0 )
        {
            insert_array( &a, key2, val2, ( int )keylen2 );
            index2 += 1;
        }
        else
        {
            insert_array( &a, key1, val1 + val2, ( int )keylen1 );
            index1 += 1;
            index2 += 1;
        }
    }

    // finish by appending the longer list
    while( index1 < count1 )
    {
        read_pair( entries1, base1, index1, &key1, &val1, &keylen1);
        insert_array( &a, key1, val1, ( int )keylen1 );
        index1 += 1;
    }
    while( index2 < count2 )
    {
        read_pair( entries2, base2, index2, &key2, &val2, &keylen2);
        insert_array( &a, key2, val2, ( int )keylen2 );
        index2 += 1;
    }

    Pairs * pairs = palloc( a.used * sizeof( Pairs ) );
    int4 buflen = 0;
    for( i = 0; i < a.used; ++i )
    {
        size_t datum_len = a.sizes[i];
        int digit_num = get_digit_num( a.vals[i] );
        char * dig_str = palloc( digit_num );
        sprintf( dig_str, "%d", a.vals[i] );
        a.vstr[i] = dig_str;
        pairs[i].key = a.keys[i];
        pairs[i].keylen =  datum_len;
        pairs[i].val = dig_str;
        pairs[i].vallen =  digit_num;
        pairs[i].isnull = false;
        pairs[i].needfree = false;
        buflen += pairs[i].keylen;
        buflen += pairs[i].vallen;
    }

    HStore * out;
    out = hstorePairs( pairs, a.used, buflen );
    free_array( &a );

    PG_RETURN_POINTER( out );
}

PG_FUNCTION_INFO_V1( roa_add );

// works on all hstores (no need for sorted keys)
Datum roa_add( PG_FUNCTION_ARGS )
{
    if( PG_ARGISNULL( 0 ) || PG_ARGISNULL( 1 ) )
    {
        PG_RETURN_NULL();
    }

    HStore * hstore1 = PG_GETARG_HS( 0 );
    HStore * hstore2 = PG_GETARG_HS( 1 );
    HEntry * entries1 = ARRPTR( hstore1 );
    HEntry * entries2 = ARRPTR( hstore2 );
    char * base1 = STRPTR( hstore1 );
    char * base2 = STRPTR( hstore2 );
    int count1 = HS_COUNT( hstore1 );
    int count2 = HS_COUNT( hstore2 );
    int i,j;

    Array a;
    init_array( &a, 10 );

    int index1 = 0, index2 = 0;
    char * key1, * key2;
    int val1, val2;
    size_t keylen1, keylen2;

    for( index1 = 0; index1 < count1; ++index1 )
    {
        read_pair( entries1, base1, index1, &key1, &val1, &keylen1 );
        insert_array( &a, key1, val1, ( int )keylen1 );
    }

    for( index2 = 0; index2 < count2; ++index2 )
    {
        read_pair( entries2, base2, index2, &key2, &val2, &keylen2 );

        for( j = 0; j < a.used; ++j )
        {
            if( bcmp( key2, a.keys[j], keylen2 ) == 0 )
            {
                a.vals[j] += val2;
                a.found[j] = true;
                break;
            }
        }
        if( j == a.used && ! a.found[j] )
        {
            insert_array( &a, key2, val2, ( int )keylen2 );
        }
    }

    Pairs * pairs = palloc( a.used * sizeof( Pairs ) );
    int4 buflen = 0;
    for( i = 0; i < a.used; ++i )
    {
        size_t datum_len = a.sizes[i];
        int digit_num = get_digit_num( a.vals[i] );
        char * dig_str = palloc( digit_num );
        sprintf( dig_str, "%d", a.vals[i] );
        a.vstr[i] = dig_str;
        pairs[i].key = a.keys[i];
        pairs[i].keylen =  datum_len;
        pairs[i].val = dig_str;
        pairs[i].vallen =  digit_num;
        pairs[i].isnull = false;
        pairs[i].needfree = false;
        buflen += pairs[i].keylen;
        buflen += pairs[i].vallen;
    }

    HStore * out;
    out = hstorePairs( pairs, a.used, buflen );
    free_array( &a );

    PG_RETURN_POINTER( out );
}

