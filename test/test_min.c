#include "qmalloc.h"
#include "util.h"

#include <inttypes.h>

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

int test( int round )
{
	int		size ;
	char		*data[ 16 ] = { NULL } ;
	int		i , count ;
	
	i = 0 ;
	
	travel_blocks();
	
	size = 64 ;
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
	show_parameters();
	
	test(1);
	
	printf( "test_basic : qfree_all_unused ...\n" );
	qfree_all_unused();
	printf( "test_basic : qfree_all_unused ok\n" );
	
	travel_blocks();
	
	return 0;
}

