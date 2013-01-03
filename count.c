#include "postgres.h"
#include "hstore.h"
#include "avltree.h"
#include "fmgr.h"
#include <string.h>
#include <utils/array.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

size_t hstoreCheckKeyLen( size_t len )
{
    if( len > HSTORE_MAX_KEY_LEN )
        ereport( ERROR,
                ( errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION ),
                 errmsg( "string too long for hstore key" ) ) );
    return len;
}

typedef struct {
    char ** array;
    char ** counts_str;
    size_t  used;
    size_t  size;
    int *   counts;
    int *   sizes;
} Array;

void init_array( Array *a, size_t initial_size )
{
    int i = 0;
    a->array      = ( char ** )palloc( initial_size * sizeof( char* ) );
    a->counts_str = ( char ** )palloc( initial_size * sizeof( char* ) );
    a->used       = 0;
    a->size       = initial_size;
    a->counts     = ( int * )palloc( initial_size * sizeof( int ) );
    a->sizes      = ( int * )palloc( initial_size * sizeof( int ) );
    for( i = 0; i < a->size; ++i )
    {
        a->counts[i] = 0;
    }
}

void insert_array(Array *a, char* elem, size_t elem_size )
{
    if( a->used == a->size )
    {
        int i = a->size;
        a->size *= 2;
        char ** array_swap = a->array;
        a->array = ( char ** )palloc( a->size * sizeof( char* ) );
        memcpy( a->array, array_swap, sizeof( char* ) * i );
        pfree( array_swap );
        char ** counts_str_swap = a->counts_str;
        a->counts_str = ( char ** )palloc( a->size * sizeof( char* ) );
        memcpy( a->counts_str, counts_str_swap, sizeof( char* ) * i );
        pfree( counts_str_swap );
        int * count_swap = a->counts;
        a->counts = ( int * )palloc( a->size * sizeof( int ) );
        memcpy( a->counts, count_swap, sizeof( int ) * i );
        pfree( count_swap );
        int * sizes_swap = a->sizes;
        a->sizes = ( int * )palloc( a->size * sizeof( int ) );
        memcpy( a->sizes, sizes_swap, sizeof( int ) * i );
        pfree( sizes_swap );
        for( ; i < a->size; ++i )
        {
            a->counts[i] = 0;
            a->sizes[i]  = 0;
        }
    }
    a->sizes[a->used] = ( int ) elem_size;
    a->array[a->used++] = elem;
}

void free_array( Array *a )
{
    int i;
    for( i = 0; i < a->used; ++i )
    {
        pfree( a->array[i] );
        pfree( a->counts_str[i] );
    }
    pfree( a->array );
    pfree( a->counts_str );
    pfree( a->counts );
    pfree( a->sizes );
    a->array = NULL;
    a->used = a->size = 0;
}

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

PG_FUNCTION_INFO_V1( welle_count );

Datum welle_count( PG_FUNCTION_ARGS )
{

    if( PG_ARGISNULL( 0 ) )
    {
        PG_RETURN_NULL();
    }

    ArrayType * input;
    input = PG_GETARG_ARRAYTYPE_P( 0 );

    Datum * i_data;

    Oid i_eltype;
    i_eltype = ARR_ELEMTYPE( input );

    int16 i_typlen;
    bool  i_typbyval;
    char  i_typalign;

    get_typlenbyvalalign(
            i_eltype,
            &i_typlen,
            &i_typbyval,
            &i_typalign
    );

    bool * nulls;
    int n;

    deconstruct_array(
            input,
            i_eltype,
            i_typlen,
            i_typbyval,
            i_typalign,
            &i_data,
            &nulls,
            &n
    );

    int i, j;

    Array a;
    init_array( &a, 10 );
    AvlTree tree = make_empty( NULL );

    for( i = 0; i < n; ++i )
    {
        if( ! nulls[i] )
        {
            bool found = false;
            size_t datum_len = VARSIZE( i_data[i] ) - VARHDRSZ;
            char * current_datum = ( char * ) palloc ( datum_len );
            memcpy( current_datum, VARDATA( i_data[i] ), datum_len );

            Position position = find( current_datum, datum_len, tree );
            if( position == NULL )
            {
                j = a.used;
                tree = insert( current_datum, datum_len, j, tree );
                insert_array( &a, current_datum, datum_len );
            }
            else
            {
                j = value( position );
            }
    
            a.counts[j] += 1;
        }
    }

    // save sort permutation to create pairs in order of ascending keys
    // we assume that postgres stores the pairs in that order
    int * perm = ( int * ) palloc ( a.used * sizeof( int ) );
    sort_perm( tree, perm );

    make_empty( tree );

    Pairs * pairs = palloc( a.used * sizeof( Pairs ) );
    int4 buflen = 0;
    for( i = 0; i < a.used; ++i )
    {
        j = perm[i];
        if( a.array[j] != NULL )
        {
            size_t datum_len = a.sizes[j];
            int digit_num = get_digit_num( a.counts[j] );
            char * dig_str = palloc(digit_num);
            sprintf( dig_str, "%d", a.counts[j] );
            a.counts_str[j] = dig_str;
            pairs[i].key = a.array[j];
            pairs[i].keylen =  datum_len;
            pairs[i].val = dig_str;
            pairs[i].vallen =  digit_num;
            pairs[i].isnull = false;
            pairs[i].needfree = false;
            buflen += pairs[i].keylen;
            buflen += pairs[i].vallen;
        }
    }
    HStore * out;
    out = hstorePairs( pairs, a.used, buflen );
    free_array( &a );
    PG_RETURN_POINTER( out );
    pfree( perm );
}
