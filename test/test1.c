#include "qmalloc.h"
#include "inttypes.h"

#define IF_P_EQ_NULL_EXIT \
	if( p[i] == NULL ) \
	{ \
		printf( "*** ERROR : qmalloc failed , errno[%d] , size[%d]\n" , errno , size ); \
		return 1; \
	} \
	else \
	{ \
		printf( "qmalloc[%d] ok , addr[%"PRIu64"]\n" , size , (uint64_t)(p[i]) ); \
		i++; \
	} \

static void travel()
{
	char		*p = NULL ;
	
	p = NULL ;
	while(1)
	{
		p = _qtravel_used_by_size( p) ;
		if( p == NULL )
			break;
		printf( "  USED - size[%zu] p[%"PRIu64"] alloc_source_file[%s] alloc_source_line[%d] free_source_file[%s] free_source_line[%d]\n" , _qget_size(p) , (uint64_t)p , _qget_alloc_source_file(p) , _qget_alloc_source_line(p) , _qget_free_source_file(p) , _qget_free_source_line(p) );
	}
	
	p = NULL ;
	while(1)
	{
		p = _qtravel_unused_by_size( p) ;
		if( p == NULL )
			break;
		printf( "UNUSED - size[%zu] p[%"PRIu64"] alloc_source_file[%s] alloc_source_line[%d] free_source_file[%s] free_source_line[%d]\n" , _qget_size(p) , (uint64_t)p , _qget_alloc_source_file(p) , _qget_alloc_source_line(p) , _qget_free_source_file(p) , _qget_free_source_line(p) );
	}
	
	return;
}

int main()
{
	int		size ;
	char		*p[ 16 ] = { NULL } ;
	int		i ;
	
	printf( "    _qget_block_header_size[%d]\n" , _qget_block_header_size() );
	printf( "_qget_normal_block_max_size[%d]\n" , _qget_normal_block_max_size() );
	printf( "     _qget_blocks_page_size[%d]\n" , _qget_blocks_page_size() );
	
	i = 0 ;
	
	size = 8 ;
	p[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel();
	
	size = 64 ;
	p[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel();
	
	size = 256 ;
	p[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel();
	
	/*
	size = 64824 ;
	p[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel();
	*/
	
	size = _qget_normal_block_max_size()-1 ;
	p[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel();
	
	size = _qget_normal_block_max_size() ;
	p[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel();
	
	size = _qget_normal_block_max_size()+1 ;
	p[i] = qmalloc( size ) ;
	IF_P_EQ_NULL_EXIT
	
	travel();
	
	for( ; i >= 0 ; i-- )
	{
		qfree( p[i] ) ;
		printf( "qfree[%p] ok\n" , p[i] );
	}
	
	travel();
	
	_qfree_all();
	printf( "_qfree_all ok\n" );
	
	travel();
	
	return 0;
}
