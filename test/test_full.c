#include "qmalloc.h"
#include "util.h"

#include <inttypes.h>

#define DATA_PTR_MAX_COUNT	64

#define BEFORE_CALL_QMALLOC(_size_) \
	size = (_size_) ; \
	if( i >= DATA_PTR_MAX_COUNT ) \
	{ \
		printf( "*** ERROR : data ptr array overflow , resize data array and test again\n" ); \
		return 1; \
	} \
	else \
	{ \
		printf( "-----------------------------------\n" ); \
		printf( "test_basic : qmalloc [%zu]bytes ...\n" , size ); \
	} \

#define CALL_QMALLOC \
	data[i] = qmalloc( size ) ; \

#define AFTER_CALL_QMALLOC \
	if( data[i] == NULL ) \
	{ \
		printf( "*** ERROR : qmalloc [%zu]bytes failed , errno[%d]\n" , size , errno ); \
		return 1; \
	} \
	else \
	{ \
		printf( "test_basic : qmalloc [%zu]bytes ok , data[%p]\n" , size , data[i] ); \
		i++; \
	} \

#define CALL_QFREE(_index_) \
	qfree( data[(_index_)] ) ; \

int test( int round )
{
	size_t		size ;
	char		*data[ 64 ] = { NULL } ;
	int		i , count ;
	
	printf( "=================================== ROUND %d\n" , round );
	
	i = 0 ;
	
	BEFORE_CALL_QMALLOC(0)
	CALL_QMALLOC
	AFTER_CALL_QMALLOC
	
	travel_blocks();
	
	BEFORE_CALL_QMALLOC(1)
	CALL_QMALLOC
	AFTER_CALL_QMALLOC
	
	travel_blocks();
	
	BEFORE_CALL_QMALLOC(2)
	CALL_QMALLOC
	AFTER_CALL_QMALLOC
	
	travel_blocks();
	
	BEFORE_CALL_QMALLOC(4)
	CALL_QMALLOC
	AFTER_CALL_QMALLOC
	
	travel_blocks();
	
	BEFORE_CALL_QMALLOC(8)
	CALL_QMALLOC
	AFTER_CALL_QMALLOC
	
	travel_blocks();
	
	BEFORE_CALL_QMALLOC(64)
	CALL_QMALLOC
	AFTER_CALL_QMALLOC
	
	travel_blocks();
	
	BEFORE_CALL_QMALLOC(256)
	CALL_QMALLOC
	AFTER_CALL_QMALLOC
	
	travel_blocks();
	
	BEFORE_CALL_QMALLOC(qget_thread_mempool_block_size()-1)
	CALL_QMALLOC
	AFTER_CALL_QMALLOC
	
	travel_blocks();
	
	BEFORE_CALL_QMALLOC(qget_thread_mempool_block_size())
	CALL_QMALLOC
	AFTER_CALL_QMALLOC
	
	travel_blocks();
	
	BEFORE_CALL_QMALLOC(qget_thread_mempool_block_size()+1)
	CALL_QMALLOC
	AFTER_CALL_QMALLOC
	
	travel_blocks();
	
	BEFORE_CALL_QMALLOC(qget_process_mempool_block_size()-1)
	CALL_QMALLOC
	AFTER_CALL_QMALLOC
	
	travel_blocks();
	
	BEFORE_CALL_QMALLOC(qget_process_mempool_block_size())
	CALL_QMALLOC
	AFTER_CALL_QMALLOC
	
	travel_blocks();
	
	BEFORE_CALL_QMALLOC(qget_process_mempool_block_size()+1)
	CALL_QMALLOC
	AFTER_CALL_QMALLOC
	
	travel_blocks();
	
	count = i ;
	for( i = 0 ; i < count ; i++ )
	{
		printf( "-----------------------------------\n" ); \
		printf( "test_basic : qfree[%p] ...\n" , data[i] );
		CALL_QFREE(i)
		printf( "test_basic : qfree[%p] ok\n" , data[i] );
		
		travel_blocks();
	}
	
	return 0;
}

int main()
{
	show_parameters();
	
	test(1);
	
	test(2);
	
	printf( "test_basic : qfree_all_unused ...\n" );
	qfree_all_unused();
	printf( "test_basic : qfree_all_unused ok\n" );
	
	travel_blocks();
	
	return 0;
}

