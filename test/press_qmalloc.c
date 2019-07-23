#include "qmalloc.h"

#include <sys/time.h>

#define PRESS_ROUND	10000
#define PRESS_COUNT	16

int press_qmalloc()
{
	int		j ;
	int		size ;
	void		*ptrs[PRESS_COUNT] = { NULL } ;
	
	for( j = 0 , size = 1 ; j < PRESS_COUNT ; j++ , size *= 2 )
	{
		ptrs[j] = qmalloc( size ) ;
		if( ptrs[j] == NULL )
		{
			printf( "*** ERROR : qmalloc failed , errno[%d]\n" , errno );
		}
	}
	
	for( j = 0 ; j < PRESS_COUNT ; j++ )
	{
		qfree( ptrs[j] );
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
	int		round ;
	struct timeval	begin_tv ;
	struct timeval	end_tv ;
	struct timeval	diff_tv ;
	
	int		nret = 0 ;
	
	gettimeofday( & begin_tv , NULL );
	for( round = 1 ; round <= PRESS_ROUND ; round++ )
	{
		nret = press_qmalloc() ;
		if( nret )
			return 1;
	}
	gettimeofday( & end_tv , NULL );
	DIFF_TV( begin_tv , end_tv , diff_tv )
	if( nret )
		printf( "do press_qmalloc failed[%d]\n" , nret );
	else
		printf( "do press_qmalloc ok , elapse[%ld.%06ld]\n" , diff_tv.tv_sec , diff_tv.tv_usec );
	
	return 0;
}
