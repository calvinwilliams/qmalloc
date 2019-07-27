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

static void travel_by_size()
{
	char		*data = NULL ;
	
	printf( "--- travel_by_size --- used ---- blocks_count[%zu] blocks_total_size[%zu]\n" , _qstat_used_blocks_count() , _qstat_used_blocks_total_size() );
	
	data = NULL ;
	while(1)
	{
		data = _qtravel_used_by_size(data) ;
		if( data == NULL )
			break;
		printf( "  USED - size[%zu] data[%p] alloc_source_file[%s] alloc_source_line[%d] free_source_file[%s] free_source_line[%d]\n" , _qget_size(data) , data , _qget_alloc_source_file(data) , _qget_alloc_source_line(data) , _qget_free_source_file(data) , _qget_free_source_line(data) );
	}
	
	printf( "--- travel_by_size --- unused --- blocks_count[%zu] blocks_total_size[%zu]\n" , _qstat_unused_blocks_count() , _qstat_unused_blocks_total_size() );
	
	data = NULL ;
	while(1)
	{
		data = _qtravel_unused_by_size(data) ;
		if( data == NULL )
			break;
		printf( "UNUSED - size[%zu] data[%p] alloc_source_file[%s] alloc_source_line[%d] free_source_file[%s] free_source_line[%d]\n" , _qget_size(data) , data , _qget_alloc_source_file(data) , _qget_alloc_source_line(data) , _qget_free_source_file(data) , _qget_free_source_line(data) );
	}
	
	return;
}

static void travel_by_addr()
{
	char		*data = NULL ;
	
	printf( "--- travel_by_addr --- used --- blocks_count[%zu] blocks_total_size[%zu]\n" , _qstat_used_blocks_count() , _qstat_used_blocks_total_size() );
	
	data = NULL ;
	while(1)
	{
		data = _qtravel_used_by_addr(data) ;
		if( data == NULL )
			break;
		printf( "  USED - size[%zu] data[%p] alloc_source_file[%s] alloc_source_line[%d] free_source_file[%s] free_source_line[%d]\n" , _qget_size(data) , data , _qget_alloc_source_file(data) , _qget_alloc_source_line(data) , _qget_free_source_file(data) , _qget_free_source_line(data) );
	}
	
	printf( "--- travel_by_addr --- unused --- blocks_count[%zu] blocks_total_size[%zu]\n" , _qstat_unused_blocks_count() , _qstat_unused_blocks_total_size() );
	
	data = NULL ;
	while(1)
	{
		data = _qtravel_unused_by_addr(data) ;
		if( data == NULL )
			break;
		printf( "UNUSED - size[%zu] data[%p] alloc_source_file[%s] alloc_source_line[%d] free_source_file[%s] free_source_line[%d]\n" , _qget_size(data) , data , _qget_alloc_source_file(data) , _qget_alloc_source_line(data) , _qget_free_source_file(data) , _qget_free_source_line(data) );
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
	
	travel_by_size();
	travel_by_addr();
	
	size = 2 ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_by_size();
	travel_by_addr();
	
	size = 4 ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_by_size();
	travel_by_addr();
	
	size = 8 ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_by_size();
	travel_by_addr();
	
	size = 64 ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_by_size();
	travel_by_addr();
	
	size = 256 ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_by_size();
	travel_by_addr();
	
	size = _qget_normal_block_max_size()-1 ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_by_size();
	travel_by_addr();
	
	size = _qget_normal_block_max_size() ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_by_size();
	travel_by_addr();
	
	size = _qget_normal_block_max_size()+1 ;
	data[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel_by_size();
	travel_by_addr();
	
	count = i ;
	for( i = 0 ; i < count ; i++ )
	{
		printf( "test_basic : qfree[%p] ...\n" , data[i] );
		qfree( data[i] ) ;
		printf( "test_basic : qfree[%p] ok\n" , data[i] );
		
		travel_by_size();
		travel_by_addr();
	}
	
	return 0;
}

int main()
{
	_qset_cache_blocks_max_size( _qget_normal_block_max_size() * 3 );
	
	printf( "    get_block_header_size[%zu]\n" , _qget_block_header_size() );
	printf( "get_normal_block_max_size[%zu]\n" , _qget_normal_block_max_size() );
	printf( "     get_blocks_page_size[%zu]\n" , _qget_blocks_page_size() );
	printf( "get_cache_blocks_max_size[%zu]\n" , _qget_cache_blocks_max_size() );
	
	test(1);
	
	test(2);
	
	printf( "test_basic : _qfree_all_unused ...\n" );
	_qfree_all_unused();
	printf( "test_basic : _qfree_all_unused ok\n" );
	
	travel_by_size();
	travel_by_addr();
	
	return 0;
}

