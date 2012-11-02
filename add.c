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

PG_FUNCTION_INFO_V1( roa_add );

Datum roa_add( PG_FUNCTION_ARGS )
{
    if( PG_ARGISNULL( 0 ) )
    {
        PG_RETURN_NULL();
    }

    HStore * hs_one = PG_GETARG_HS( 0 );
    HStore * hs_two = PG_GETARG_HS( 1 );

    HEntry * entries_one = ARRPTR( hs_one );
    char   * base_one    = STRPTR( hs_one );
    int      count_one   = HS_COUNT( hs_one );

    HEntry * entries_two = ARRPTR( hs_two );
    char   * base_two    = STRPTR( hs_two );
    int      count_two   = HS_COUNT( hs_two );
    int i,j;

    Array a;
    init_array( &a, 10 );

    for( i = 0; i < count_one; ++i )
    {
        size_t key_len = HS_KEYLEN( entries_one, i );
        size_t val_len = HS_VALLEN( entries_one, i );
        char * current_key = palloc( (key_len + 1) * sizeof( char ) );
        memset( current_key, '\0', key_len + 1 );
        memcpy(current_key, HS_KEY( entries_one, base_one, i ), key_len );
        char * current_val = palloc( val_len * sizeof( char ) );
        memcpy(current_val, HS_VAL( entries_one, base_one, i ), val_len );
        int current_val_int = atoi( current_val );
        pfree( current_val );
        insert_array( &a, current_key, current_val_int, ( int )key_len );
    }

    for( i = 0; i < count_two; ++i )
    {
        size_t key_len = HS_KEYLEN( entries_two, i );
        size_t val_len = HS_VALLEN( entries_two, i );
        char * current_key = palloc( key_len * sizeof( char ) );
        memcpy(current_key, HS_KEY( entries_two, base_two, i ), key_len );
        char * current_val = palloc( val_len * sizeof( char ) );
        memcpy(current_val, HS_VAL( entries_two, base_two, i ), val_len );
        int current_val_int = atoi( current_val );
        pfree( current_val );

        for( j = 0; j < a.used; ++j )
        {
            if( bcmp( current_key, a.keys[j], key_len ) == 0 )
            {
                a.vals[j] += current_val_int;
                a.found[j] = true;
                break;
            }
        }
        if( j == a.used && ! a.found[j] )
        {
            insert_array( &a, current_key, current_val_int, ( int )key_len );
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

