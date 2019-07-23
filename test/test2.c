#include "qmalloc.h"

#include <sys/time.h>

#define PRESS_ROUND	1024
#define PRESS_COUNT	1024

int press_qmalloc()
{
	int		i , j ;
	int		size ;
	void		*ptrs[PRESS_COUNT] = { NULL } ;
	
	for( i = 0 ; i < PRESS_ROUND ; i++ )
	{
		for( j = 0 ; j < PRESS_COUNT ; j++ )
		{
			size = j * PRESS_COUNT + 1024 ;
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
	}
	
	return 0;
}

int press_malloc()
{
	int		i , j ;
	int		size ;
	void		*ptrs[PRESS_COUNT] = { NULL } ;
	
	for( i = 0 ; i < PRESS_ROUND ; i++ )
	{
		for( j = 0 ; j < PRESS_COUNT ; j++ )
		{
			size = j * PRESS_COUNT + 1024 ;
			ptrs[j] = malloc( size ) ;
			if( ptrs[j] == NULL )
			{
				printf( "*** ERROR : malloc failed , errno[%d]\n" , errno );
			}
		}
		
		for( j = 0 ; j < PRESS_COUNT ; j++ )
		{
			free( ptrs[j] );
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
	int		round ;
	struct timeval	begin_tv ;
	struct timeval	end_tv ;
	struct timeval	diff_tv ;
	
	int		nret = 0 ;
	
	for( round = 1 ; round <= 2 ; round++ )
	{
		printf( "================= ROUND %d\n" , round );
		
		gettimeofday( & begin_tv , NULL );
		nret = press_qmalloc() ;
		gettimeofday( & end_tv , NULL );
		DIFF_TV( begin_tv , end_tv , diff_tv )
		if( nret )
			printf( "do press_qmalloc failed[%d]\n" , nret );
		else
			printf( "do press_qmalloc ok , elapse[%ld.%06ld]\n" , diff_tv.tv_sec , diff_tv.tv_usec );
		
		gettimeofday( & begin_tv , NULL );
		nret = press_malloc() ;
		gettimeofday( & end_tv , NULL );
		DIFF_TV( begin_tv , end_tv , diff_tv )
		if( nret )
			printf( "do press_malloc failed[%d]\n" , nret );
		else
			printf( "do press_malloc ok , elapse[%ld.%06ld]\n" , diff_tv.tv_sec , diff_tv.tv_usec );
		
	}
	
	return 0;
}
