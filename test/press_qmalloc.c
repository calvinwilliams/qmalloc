#include "qmalloc.h"

#include <sys/time.h>

/*
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -o press_qmalloc press_qmalloc.o -L. -L/home/calvin/lib ../src/qmalloc.o ../src/rbtree.o  -pg
./press_qmalloc
gprof ./press_qmalloc gmon.out -p
gprof ./press_qmalloc gmon.out -q
gprof ./press_qmalloc gmon.out -A
*/

#define PRESS_ROUND	10000
#define PRESS_COUNT	1024

void travel_blocks()
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

int press_qmalloc()
{
	int		round ;
	int		j ;
	int		size ;
	void		*ptrs[PRESS_COUNT] = { NULL } ;
	
	for( round = 1 ; round <= PRESS_ROUND ; round++ )
	{
		for( j = 0 , size = 1 ; j < PRESS_COUNT ; j++ , size++ )
		{
			ptrs[j] = qmalloc( size ) ;
			if( ptrs[j] == NULL )
			{
				printf( "*** ERROR : qmalloc failed , errno[%d]\n" , errno );
			}
		}
		
		/*
		travel_blocks();
		*/
		
		for( j = 0 ; j < PRESS_COUNT ; j++ )
		{
			qfree( ptrs[j] );
		}
		
		/*
		travel_blocks();
		*/
	}
	
	return 0;
}

#define DIFF_TV(_begin_tv_,_end_tv_,_diff_tv_) \
	diff_tv.tv_sec = end_tv.tv_sec - begin_tv.tv_sec ; \
	diff_tv.tv_usec = end_tv.tv_usec - begin_tv.tv_usec ; \
	while( diff_tv.tv_usec < 0 ) \
	{ \
		diff_tv.tv_usec += 1000000 ; \
		diff_tv.tv_sec--; \
	} \

int main()
{
	struct timeval	begin_tv ;
	struct timeval	end_tv ;
	struct timeval	diff_tv ;
	
	int		nret = 0 ;
	
	gettimeofday( & begin_tv , NULL );
	nret = press_qmalloc() ;
	if( nret )
	{
		printf( "press_qmalloc failed\n" );
		return 1;
	}
	gettimeofday( & end_tv , NULL );
	DIFF_TV( begin_tv , end_tv , diff_tv )
	printf( "press_qmalloc ok , elapse[%ld.%06ld]\n" , diff_tv.tv_sec , diff_tv.tv_usec );
	
	qfree_all_unused();
	
	return 0;
}
