#include "qmalloc.h"

int main()
{
	int		i ;
	int		size ;
	void		*ptrs[10] ;
	void		*ptr ;
	
	for( i = 0 ; i < 10 ; i++ )
	{
		size = i * 1024 + 1024 ;
		ptrs[i] = qmalloc( size ) ;
		if( ptrs[i] == NULL )
		{
			printf( "*** ERROR : qmalloc failed , errno[%d]\n" , errno );
		}
	}
	
	printf( "all qmalloc ok\n" );
	
	ptr = NULL ;
	while(1)
	{
		ptr = _qtravel_used_by_size( ptr) ;
		if( ptr == NULL )
			break;
		printf( "_qget_size[%zu] _qtravel_used_by_size[%p] _qget_alloc_source_file[%s] _qget_alloc_source_line[%d]\n" , _qget_size(ptr) , ptr , _qget_alloc_source_file(ptr) , _qget_alloc_source_line(ptr) );
	}
	
	ptr = NULL ;
	while(1)
	{
		ptr = _qtravel_unused_by_size( ptr) ;
		if( ptr == NULL )
			break;
		printf( "_qget_size[%zu] _qtravel_unused_by_size[%p] _qget_free_source_file[%s] _qget_free_source_line[%d]\n" , _qget_size(ptr) , ptr , _qget_free_source_file(ptr) , _qget_free_source_line(ptr) );
	}
	
	for( i = 0 ; i < 5 ; i++ )
	{
		qfree( ptrs[i] );
	}
	
	printf( "half qfree ok\n" );
	
	ptr = NULL ;
	while(1)
	{
		ptr = _qtravel_used_by_size( ptr) ;
		if( ptr == NULL )
			break;
		printf( "_qget_size[%zu] _qtravel_used_by_size[%p] _qget_alloc_source_file[%s] _qget_alloc_source_line[%d]\n" , _qget_size(ptr) , ptr , _qget_alloc_source_file(ptr) , _qget_alloc_source_line(ptr) );
	}
	
	ptr = NULL ;
	while(1)
	{
		ptr = _qtravel_unused_by_size( ptr) ;
		if( ptr == NULL )
			break;
		printf( "_qget_size[%zu] _qtravel_unused_by_size[%p] _qget_free_source_file[%s] _qget_free_source_line[%d]\n" , _qget_size(ptr) , ptr , _qget_free_source_file(ptr) , _qget_free_source_line(ptr) );
	}
	
	_qfree_all();
	
	printf( "_qfree_all ok\n" );
	
	ptr = NULL ;
	while(1)
	{
		ptr = _qtravel_used_by_size( ptr) ;
		if( ptr == NULL )
			break;
		printf( "_qget_size[%zu] _qtravel_used_by_size[%p] _qget_alloc_source_file[%s] _qget_alloc_source_line[%d]\n" , _qget_size(ptr) , ptr , _qget_alloc_source_file(ptr) , _qget_alloc_source_line(ptr) );
	}
	
	ptr = NULL ;
	while(1)
	{
		ptr = _qtravel_unused_by_size( ptr) ;
		if( ptr == NULL )
			break;
		printf( "_qget_size[%zu] _qtravel_unused_by_size[%p] _qget_free_source_file[%s] _qget_free_source_line[%d]\n" , _qget_size(ptr) , ptr , _qget_free_source_file(ptr) , _qget_free_source_line(ptr) );
	}
	
	for( ; i < 10 ; i++ )
	{
		qfree( ptrs[i] );
	}
	
	printf( "all qfree ok\n" );
	
	ptr = NULL ;
	while(1)
	{
		ptr = _qtravel_used_by_size( ptr) ;
		if( ptr == NULL )
			break;
		printf( "_qget_size[%zu] _qtravel_used_by_size[%p] _qget_alloc_source_file[%s] _qget_alloc_source_line[%d]\n" , _qget_size(ptr) , ptr , _qget_alloc_source_file(ptr) , _qget_alloc_source_line(ptr) );
	}
	
	ptr = NULL ;
	while(1)
	{
		ptr = _qtravel_unused_by_size( ptr) ;
		if( ptr == NULL )
			break;
		printf( "_qget_size[%zu] _qtravel_unused_by_size[%p] _qget_free_source_file[%s] _qget_free_source_line[%d]\n" , _qget_size(ptr) , ptr , _qget_free_source_file(ptr) , _qget_free_source_line(ptr) );
	}
	
	_qfree_all();
	
	printf( "_qfree_all ok\n" );
	
	ptr = NULL ;
	while(1)
	{
		ptr = _qtravel_used_by_size( ptr) ;
		if( ptr == NULL )
			break;
		printf( "_qget_size[%zu] _qtravel_used_by_size[%p] _qget_alloc_source_file[%s] _qget_alloc_source_line[%d]\n" , _qget_size(ptr) , ptr , _qget_alloc_source_file(ptr) , _qget_alloc_source_line(ptr) );
	}
	
	ptr = NULL ;
	while(1)
	{
		ptr = _qtravel_unused_by_size( ptr) ;
		if( ptr == NULL )
			break;
		printf( "_qget_size[%zu] _qtravel_unused_by_size[%p] _qget_free_source_file[%s] _qget_free_source_line[%d]\n" , _qget_size(ptr) , ptr , _qget_free_source_file(ptr) , _qget_free_source_line(ptr) );
	}
	
	return 0;
}
