/*
 * All functions in this file use the adeven_add_.. namespace to avoid name collisions
 */

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
    long *  vals;
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
    out = palloc0( len );
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

void adeven_add_init_array( Array *a, size_t initial_size )
{
    int i = 0;
    a->keys  = ( char ** )palloc0( initial_size * sizeof( char * ) );
    a->vstr  = ( char ** )palloc0( initial_size * sizeof( char * ) );
    a->vals  = ( long *  )palloc0( initial_size * sizeof( long * ) );
    a->sizes = ( int  *  )palloc0( initial_size * sizeof( int  * ) );
    a->found = ( bool *  )palloc0( initial_size * sizeof( bool * ) );
    a->used = 0;
    a->size = initial_size;
    for( ; i < initial_size; ++i )
    {
        a->vals[i]  = 0;
        a->sizes[i] = 0;
        a->found[i] = false;
    }
}

void adeven_add_insert_array( Array *a, char * key, long val, int elem_size )
{
    if( a->used == a->size )
    {
        int i = a->size;
        a->size *= 2;

        char ** keys_swap = a->keys;
        a->keys = ( char ** )palloc0( a->size * sizeof( char * ) );
        memcpy( a->keys, keys_swap, sizeof( char * ) * i );
        pfree( keys_swap );

        char ** vstr_swap = a->vstr;
        a->vstr = ( char ** )palloc0( a->size * sizeof( char * ) );
        memcpy( a->vstr, vstr_swap, sizeof( char * ) * i );
        pfree( vstr_swap );

        long * vals_swap = a->vals;
        a->vals = ( long * )palloc0( a->size * sizeof( long ) );
        memcpy( a->vals, vals_swap, sizeof( long ) * i );
        pfree( vals_swap );

        int * sizes_swap = a->sizes;
        a->sizes = ( int * )palloc0( a->size * sizeof( int ) );
        memcpy( a->sizes, sizes_swap, sizeof( int ) * i );
        pfree( sizes_swap );

        bool * found_swap = a->found;
        a->found = ( bool * )palloc0( a->size * sizeof( bool ) );
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

void adeven_add_free_array( Array *a )
{
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

int adeven_add_get_digit_num( long number )
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

void adeven_add_read_pair( HEntry * entries, char * base, int index, char ** key, long * vali, size_t * keylen )
{
    size_t vallen = HS_VALLEN( entries, index );
    char * val = palloc0( ( vallen + 1 ) * sizeof( char ) );
    memset( val, '\0', vallen + 1 );
    memcpy(val, HS_VAL( entries, base, index ), vallen );

    *keylen = HS_KEYLEN( entries, index );
    *key = palloc0( ( *keylen + 1 ) * sizeof( char ) );
    memset( *key, '\0', *keylen + 1 );
    memcpy(*key, HS_KEY( entries, base, index ), *keylen );
    *vali = atol( val );

    pfree( val );
}

int adeven_add_min( int a, int b )
{
    return ( a < b ) ? a : b;
}

int adeven_add_compare( char * key1, int keylen1, char * key2, int keylen2 )
{
    if( keylen1 < keylen2 )
        return -1;
    if( keylen1 > keylen2 )
        return 1;

    int len = adeven_add_min( keylen1, keylen2 );
    int cmp = strncmp( key1, key2, len );

    return cmp;
}

PG_FUNCTION_INFO_V1( welle_add );

// works on sorted hstores only, returns sorted result
// further idea for improvement: work on array of hstores and use heap to
// select minimum key (avoids serialization of many hstores)
Datum welle_add( PG_FUNCTION_ARGS )
{
    if( PG_ARGISNULL( 0 ) && ! PG_ARGISNULL( 1 ) )
    {
        PG_RETURN_POINTER( PG_GETARG_HS( 1 ) );
    }
    if( ! PG_ARGISNULL( 0 ) && PG_ARGISNULL( 1 ) )
    {
        PG_RETURN_POINTER( PG_GETARG_HS( 0 ) );
    }
    if( PG_ARGISNULL( 0 ) && PG_ARGISNULL( 1 ) )
    {
        HStore * out;
        out = hstorePairs( 0, 0, 0 );
        PG_RETURN_POINTER( out );
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
    adeven_add_init_array( &a, 200 );

    int index1 = 0, index2 = 0;
    char * key1, * key2;
    long val1, val2;
    size_t keylen1, keylen2;

    // merge both lists by appending the smaller key
    // or the sum of the values if the keys equal
    while( index1 < count1 && index2 < count2 )
    {
        adeven_add_read_pair( entries1, base1, index1, &key1, &val1, &keylen1 );
        adeven_add_read_pair( entries2, base2, index2, &key2, &val2, &keylen2 );

        int cmp = adeven_add_compare( key1, keylen1, key2, keylen2 );
        if( cmp < 0 )
        {
            adeven_add_insert_array( &a, key1, val1, ( int )keylen1 );
            index1 += 1;
        }
        else if( cmp > 0 )
        {
            adeven_add_insert_array( &a, key2, val2, ( int )keylen2 );
            index2 += 1;
        }
        else
        {
            adeven_add_insert_array( &a, key1, val1 + val2, ( int )keylen1 );
            index1 += 1;
            index2 += 1;
        }
    }

    // finish by appending the longer list
    while( index1 < count1 )
    {
        adeven_add_read_pair( entries1, base1, index1, &key1, &val1, &keylen1);
        adeven_add_insert_array( &a, key1, val1, ( int )keylen1 );
        index1 += 1;
    }
    while( index2 < count2 )
    {
        adeven_add_read_pair( entries2, base2, index2, &key2, &val2, &keylen2);
        adeven_add_insert_array( &a, key2, val2, ( int )keylen2 );
        index2 += 1;
    }

    Pairs * pairs = palloc0( a.used * sizeof( Pairs ) );
    int4 buflen = 0;
    for( i = 0; i < a.used; ++i )
    {
        size_t datum_len = a.sizes[i];
        int digit_num = adeven_add_get_digit_num( a.vals[i] );
        char * dig_str = palloc0( digit_num );
        sprintf( dig_str, "%ld", a.vals[i] );
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
    //adeven_add_free_array( &a );

    PG_RETURN_POINTER( out );
}

