#include "qmalloc.h"

void show_parameters()
{
	printf( "         qget_block_header_size[%zu]\n" , qget_block_header_size() );
	printf( " qget_thread_mempool_block_size[%zu]\n" , qget_thread_mempool_block_size() );
	printf( "qget_process_mempool_block_size[%zu]\n" , qget_process_mempool_block_size() );
	
	return;
}

void travel_blocks()
{
	char		*data = NULL ;
	
	printf( "travel_blocks : USED - blocks_count[%zu] blocks_total_size[%zu]\n" , qstat_used_blocks_count() , qstat_used_blocks_total_size() );
	
	data = NULL ;
	while(1)
	{
		data = qtravel_used_blocks(data) ;
		if( data == NULL )
			break;
		printf( "      USED - size[%zu] data[%p] alloc_source_file[%s] alloc_source_line[%d]\n" , qget_size(data) , data , qget_alloc_source_file(data) , qget_alloc_source_line(data) );
	}
	
	printf( "travel_blocks : UNUSED - blocks_count[%zu] blocks_total_size[%zu]\n" , qstat_unused_blocks_count() , qstat_unused_blocks_total_size() );
	
	data = NULL ;
	while(1)
	{
		data = qtravel_unused_blocks(data) ;
		if( data == NULL )
			break;
		printf( "    UNUSED - size[%zu] data[%p] alloc_source_file[%s] alloc_source_line[%d]\n" , qget_size(data) , data , qget_alloc_source_file(data) , qget_alloc_source_line(data) );
	}
	
	return;
}

