#include "qmalloc.h"
#include "inttypes.h"

#define IF_P_EQ_NULL_EXIT \
	if( data[i] == NULL ) \
	{ \
		printf( "*** ERROR : qmalloc failed , errno[%d] , size[%d]\n" , errno , size ); \
		return 1; \
	} \
	else \
	{ \
		printf( "test_basic : qmalloc[%d] ok , data[%p]\n" , size , data[i] ); \
		i++; \
	} \

static void travel_blocks()
{
	char		*data = NULL ;
	
	printf( "--- travel_by --- used ---- blocks_count[%zu] blocks_total_size[%zu]\n" , qstat_used_blocks_count() , qstat_used_blocks_total_size() );
	
	data = NULL ;
	while(1)
	{
		data = qtravel_used_blocks(data) ;
		if( data == NULL )
			break;
		printf( "  USED - size[%zu] data[%p] alloc_source_file[%s] alloc_source_line[%d]\n" , qget_size(data) , data , qget_alloc_source_file(data) , qget_alloc_source_line(data) );
	}
	
	printf( "--- travel --- unused --- blocks_count[%zu] blocks_total_size[%zu]\n" , qstat_unused_blocks_count() , qstat_unused_blocks_total_size() );
	
	data = NULL ;
	while(1)
	{
		data = qtravel_unused_blocks(data) ;
		if( data == NULL )
			break;
		printf( "UNUSED - size[%zu] data[%p] alloc_source_file[%s] alloc_source_line[%d]\n" , qget_size(data) , data , qget_alloc_source_file(data) , qget_alloc_source_line(data) );
	}
	
	return;
}

int test( int round )
{
	int		size ;
	char		*data[ 16 ] = { NULL } ;
	int		i , count ;
	
	printf( "=================================== ROUND %d\n" , round );
	
	i = 0 ;
	
	size = 1 ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_blocks();
	
	size = 2 ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_blocks();
	
	size = 4 ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_blocks();
	
	size = 8 ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_blocks();
	
	size = 64 ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_blocks();
	
	size = 256 ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_blocks();
	
	size = qget_thread_mempool_block_size()-1 ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_blocks();
	
	size = qget_thread_mempool_block_size() ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_blocks();
	
	size = qget_thread_mempool_block_size()+1 ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_blocks();
	
	size = qget_process_mempool_block_size()-1 ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_blocks();
	
	size = qget_process_mempool_block_size() ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_blocks();
	
	size = qget_process_mempool_block_size()+1 ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_blocks();
	
	count = i ;
	for( i = 0 ; i < count ; i++ )
	{
		printf( "test_basic : qfree[%p] ...\n" , data[i] );
		qfree( data[i] ) ;
		printf( "test_basic : qfree[%p] ok\n" , data[i] );
		
		travel_blocks();
	}
	
	return 0;
}

int main()
{
	printf( "         qget_block_header_size[%zu]\n" , qget_block_header_size() );
	printf( " qget_thread_mempool_block_size[%zu]\n" , qget_thread_mempool_block_size() );
	printf( "qget_process_mempool_block_size[%zu]\n" , qget_process_mempool_block_size() );
	
	test(1);
	
	test(2);
	
	printf( "test_basic : qfree_all_unused ...\n" );
	qfree_all_unused();
	printf( "test_basic : qfree_all_unused ok\n" );
	
	travel_blocks();
	
	return 0;
}

