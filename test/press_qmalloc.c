#include "qmalloc.h"
#include "util.h"

#include <sys/time.h>

/*
gcc -g -fPIC -O2 -Wall -Werror -fno-strict-aliasing -o press_qmalloc press_qmalloc.o -L. -L/home/calvin/lib ../src/qmalloc.o ../src/rbtree.o  -pg
./press_qmalloc
gprof ./press_qmalloc gmon.out -p
gprof ./press_qmalloc gmon.out -q
gprof ./press_qmalloc gmon.out -A
*/

#define PRESS_ROUND	10000
#define PRESS_COUNT	4096

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
	
	/*
	system("ps aux | grep -v grep | grep -v vi | grep -w press_qmalloc");
	
	qfree_all_unused();
	printf( "       qstat_used_blocks_count[%zu]\n" , qstat_used_blocks_count() );
	printf( "  qstat_used_blocks_total_size[%zu]\n" , qstat_used_blocks_total_size() );
	printf( "     qstat_unused_blocks_count[%zu]\n" , qstat_unused_blocks_count() );
	printf( "qstat_unused_blocks_total_size[%zu]\n" , qstat_unused_blocks_total_size() );
	
	system("ps aux | grep -v grep | grep -v vi | grep -w press_qmalloc");
	*/
	
	return 0;
}
