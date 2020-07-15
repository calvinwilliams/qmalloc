#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include <gperftools/tcmalloc.h>

#define PRESS_ROUND	10000
#define PRESS_COUNT	4096

int press_malloc()
{
	int		round ;
	int		j ;
	int		size ;
	char		*ptrs[PRESS_COUNT] = { NULL } ;
	
	for( round = 1 ; round <= PRESS_ROUND ; round++ )
	{
		for( j = 0 , size = 1 ; j < PRESS_COUNT ; j++ , size++ )
		{
			ptrs[j] = tc_malloc( size ) ;
			if( ptrs[j] == NULL )
			{
				printf( "*** ERROR : tc_malloc failed , errno[%d]\n" , errno );
			}
			ptrs[j][0] = 'X' ;
			ptrs[j][size-1] = 'Y' ;
		}
		
		for( j = 0 ; j < PRESS_COUNT ; j++ )
		{
			tc_free( ptrs[j] );
		}
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
	nret = press_malloc() ;
	if( nret )
	{
		printf( "press_malloc failed\n" );
		return 1;
	}
	gettimeofday( & end_tv , NULL );
	DIFF_TV( begin_tv , end_tv , diff_tv )
	printf( "press_malloc ok , elapse[%ld.%06ld]\n" , diff_tv.tv_sec , diff_tv.tv_usec );
	
	/*
	system("ps aux | grep -v grep | grep -v vi | grep -w press_tcmalloc");
	*/
	
	return 0;
}
