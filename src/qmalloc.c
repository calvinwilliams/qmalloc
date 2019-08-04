#include "qmalloc_in.h"

#define _DEBUG_QMALLOC			0
#define _DEBUG_QFREE			0
#define _DEBUG_QTRAVEL			0

__thread struct QmallocBlockClass	g_small_mempool_blocks_class_array[ MAX_MEDIUM_MEMPOOL_BLOCK_SIZE_CLASS+1] ; /* 线程全局池小型内存块类数组 */
__thread size_t				g_small_mempool_blocks_used_count = 0 ;
__thread size_t				g_small_mempool_blocks_used_total_size = 0 ;
__thread size_t				g_small_mempool_blocks_unused_count = 0 ;
__thread size_t				g_small_mempool_blocks_unused_total_size = 0 ;

struct QmallocBlockClass		g_medium_mempool_blocks_class_array[ MAX_MEDIUM_MEMPOOL_BLOCK_SIZE_CLASS+1 ] ; /* 进程全局池中型内存块类数组 */
size_t					g_medium_mempool_blocks_used_count = 0 ;
size_t					g_medium_mempool_blocks_used_total_size = 0 ;
size_t					g_medium_mempool_blocks_unused_count = 0 ;
size_t					g_medium_mempool_blocks_unused_total_size = 0 ;

struct list_head			g_large_mempool_block_list ; /* 大型内存块链表 */
size_t					g_large_mempool_blocks_used_count = 0 ;
size_t					g_large_mempool_blocks_used_total_size = 0 ;

int	g_spanlock_status = SPINLOCK_UNLOCK ;

static inline int _qinit()
{
	size_t				block_class_index ;
	size_t				block_size ;
	struct QmallocBlockClass	*p_block_class ;
	
	if( g_small_mempool_blocks_class_array[0].block_size == 0 )
	{
#if _DEBUG_QMALLOC
		printf( "_qinit : g_small_mempool_blocks_class_array is not initial\n" );
#endif
		
		for( block_class_index=0 , block_size=1 ; block_class_index<=MAX_SMALL_MEMPOOL_BLOCK_SIZE_CLASS ; block_class_index++ , block_size<<=1 )
		{
			p_block_class = g_small_mempool_blocks_class_array + block_class_index ;
			
			p_block_class->block_size = block_size ;
			INIT_LIST_HEAD( & (p_block_class->used_block_list) );
			INIT_LIST_HEAD( & (p_block_class->unused_block_list) );
#if _DEBUG_QMALLOC
			printf( "_qinit : init block_class_index[%zu] p_block_class[%p] block_size[%zu] in small mempool\n" , block_class_index , p_block_class , block_size );
#endif
		}
	}
	
	if( g_medium_mempool_blocks_class_array[0].block_size == 0 )
	{
#if _DEBUG_QMALLOC
		printf( "_qinit : g_medium_mempool_blocks_class_array is not initial\n" );
#endif
		
		LOCK_SPINLOCK
		{
			if( g_medium_mempool_blocks_class_array[0].block_size == 0 )
			{
#if _DEBUG_QMALLOC
				printf( "_qinit : g_medium_mempool_blocks_class_array is not initial too\n" );
#endif
			
				for( block_class_index=0 , block_size=1<<(MAX_SMALL_MEMPOOL_BLOCK_SIZE_CLASS+1) ; block_class_index<=DIFF_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS ; block_class_index++ , block_size<<=1 )
				{
					p_block_class = g_medium_mempool_blocks_class_array + block_class_index ;
					
					p_block_class->block_size = block_size ;
					INIT_LIST_HEAD( & (p_block_class->used_block_list) );
					INIT_LIST_HEAD( & (p_block_class->unused_block_list) );
#if _DEBUG_QMALLOC
					printf( "_qinit : init block_class_index[%zu] p_block_class[%p] block_size[%zu] in medium mempool\n" , block_class_index , p_block_class , block_size );
#endif
				}
			}
			
			INIT_LIST_HEAD( & g_large_mempool_block_list );
#if _DEBUG_QMALLOC
			printf( "_qinit : init block_list in large mempool\n" );
#endif
		}
		UNLOCK_SPINLOCK
	}
	
	return 0;
}

static inline int _qget_size_class( size_t size )
{
	if( size <= MAX_SMALL_MEMPOOL_BLOCK_SIZE )
		return SMALL_MEMBLOCK;
	else if( size <= MAX_MEDIUM_MEMPOOL_BLOCK_SIZE )
		return MEDIUM_MEMBLOCK;
	else
		return LARGE_MEMBLOCK;
}

static inline int _qceil_size( size_t size , size_t *p_block_class_index , size_t *p_ceil_size )
{
	register size_t		block_class_index ;
	register size_t		ceil_size ;
	
	if( size == 0 )
	{
		(*p_block_class_index) = 0 ;
		(*p_ceil_size) = 1 ;
#if _DEBUG_QMALLOC
		printf( "_qceil_size : size[%zu] return block_class_index[%zu] ceil_size[%zu]\n" , size , (*p_block_class_index) , (*p_ceil_size) );
#endif
		return SMALL_MEMBLOCK;
	}
	else if( size > MAX_MEDIUM_MEMPOOL_BLOCK_SIZE )
	{
#if _DEBUG_QMALLOC
		printf( "_qceil_size : size[%zu] return[BIG_MEMBLOCK]\n" , size );
#endif
		return LARGE_MEMBLOCK;
	}
	
	for( block_class_index=0 , ceil_size=1 ; ceil_size <= MAX_MEDIUM_MEMPOOL_BLOCK_SIZE ; block_class_index++ )
	{
		if( size <= ceil_size )
		{
			(*p_block_class_index) = block_class_index ;
			(*p_ceil_size) = ceil_size ;
#if _DEBUG_QMALLOC
			printf( "_qceil_size : size[%zu] return block_class_index[%zu] ceil_size[%zu]\n" , size , (*p_block_class_index) , (*p_ceil_size) );
#endif
			if( block_class_index <= MAX_SMALL_MEMPOOL_BLOCK_SIZE_CLASS )
				return SMALL_MEMBLOCK;
			else if( block_class_index <= MAX_MEDIUM_MEMPOOL_BLOCK_SIZE_CLASS )
				return MEDIUM_MEMBLOCK;
			else
				return INTERNAL_ERROR;
		}
		
		ceil_size <<= 1 ;
	}
	
#if _DEBUG_QMALLOC
	printf( "_qceil_size : size[%zu] return[INTERNAL_ERROR]\n" , size );
#endif
	return INTERNAL_ERROR;
}

void *_qmalloc( size_t size , char *FILE , size_t LINE )
{
	size_t				block_class_index = 0 ;
	size_t				ceil_size = 0 ;
	
	struct QmallocBlockClass	*p_block_class ;
	struct QmallocBlockHeader	*p_block_header ;
	size_t				block_total_size ;
	
	int				nret = 0 ;
	
#if _DEBUG_QMALLOC
	printf( "_qmalloc : size[%zu] FILE[%s] LINE[%zu]\n" , size , FILE , LINE );
#endif
	
	nret = _qinit() ;
	if( nret )
	{
#if _DEBUG_QMALLOC
		printf( "_qmalloc : return NULL\n" );
#endif
		return NULL;
	}
	
	nret = _qceil_size( size , & block_class_index , & ceil_size ) ;
	if( nret == SMALL_MEMBLOCK )
	{
#if _DEBUG_QMALLOC
		printf( "_qmalloc : _qceil_size return SMALL_MEMBLOCK , block_class_index[%zu] ceil_size[%zu]\n" , block_class_index , ceil_size );
#endif
		
		block_total_size = sizeof(struct QmallocBlockHeader)+ceil_size ;
		
		p_block_class = g_small_mempool_blocks_class_array+block_class_index ;
		
#if _DEBUG_QMALLOC
		printf( "_qmalloc : query block class in thread mempool , p_block_class[%p]\n" , p_block_class );
#endif
		
		if( ! list_empty( & (p_block_class->unused_block_list) ) )
		{
			p_block_header = list_first_entry( & (p_block_class->unused_block_list) , struct QmallocBlockHeader , block_list_node ) ;
#if _DEBUG_QMALLOC
			printf( "_qmalloc : move unused block to used , p_block_header[%p] data_base[%p]\n" , p_block_header , p_block_header->data_base );
#endif
			list_del( & (p_block_header->block_list_node) );
			list_add_tail( & (p_block_header->block_list_node) , & (p_block_class->used_block_list) );
			
			g_small_mempool_blocks_unused_count--;
			g_small_mempool_blocks_unused_total_size -= block_total_size ;
			g_small_mempool_blocks_used_count++;
			g_small_mempool_blocks_used_total_size += block_total_size ;
			
			p_block_header->alloc_source_file = FILE ;
			p_block_header->alloc_source_line = LINE ;
			p_block_header->alloc_size = size ;
			
#if _DEBUG_QMALLOC
			printf( "_qmalloc : return p_block_header[%p] data_base[%p]\n" , p_block_header , p_block_header->data_base );
#endif
			return p_block_header->data_base;
		}
		
		p_block_header = (struct QmallocBlockHeader *)malloc( block_total_size ) ;
		if( p_block_header == NULL )
		{
#if _DEBUG_QMALLOC
			printf( "_qmalloc : malloc SMALL_MEMBLOCK [%zu]+[%zu]bytes ok , errno[%d]\n" , sizeof(struct QmallocBlockHeader),ceil_size , errno );
			printf( "_qmalloc : return NULL\n" );
#endif
			return NULL;
		}
#if _DEBUG_QMALLOC
		printf( "_qmalloc : malloc SMALL_MEMBLOCK [%zu]+[%zu]bytes ok , p_block_header[%p] data_base[%p]\n" , sizeof(struct QmallocBlockHeader),ceil_size , p_block_header , p_block_header->data_base );
#endif
		
		p_block_header->dmz = 0 ;
		p_block_header->p_block_class = p_block_class ;
		p_block_header->alloc_source_file = FILE ;
		p_block_header->alloc_source_line = LINE ;
		p_block_header->alloc_size = size ;
		list_add_tail( & (p_block_header->block_list_node) , & (p_block_class->used_block_list) );
		
		g_small_mempool_blocks_used_count++;
		g_small_mempool_blocks_used_total_size += block_total_size ;
		
#if _DEBUG_QMALLOC
		printf( "_qmalloc : return p_block_header[%p] data_base[%p]\n" , p_block_header , p_block_header->data_base );
#endif
		return p_block_header->data_base;
	}
	else if( nret == MEDIUM_MEMBLOCK )
	{
#if _DEBUG_QMALLOC
		printf( "_qmalloc : _qceil_size return MEDIUM_MEMBLOCK , block_class_index[%zu] ceil_size[%zu]\n" , block_class_index , ceil_size );
#endif
		
		block_total_size = sizeof(struct QmallocBlockHeader)+ceil_size ;
		
		p_block_class = g_medium_mempool_blocks_class_array+(block_class_index-MAX_SMALL_MEMPOOL_BLOCK_SIZE_CLASS-1) ;
		
#if _DEBUG_QMALLOC
		printf( "_qmalloc : query block class in process mempool , p_block_class[%p]\n" , p_block_class );
#endif
		
		LOCK_SPINLOCK
		{
			if( ! list_empty( & (p_block_class->unused_block_list) ) )
			{
				p_block_header = list_first_entry( & (p_block_class->unused_block_list) , struct QmallocBlockHeader , block_list_node ) ;
#if _DEBUG_QMALLOC
				printf( "_qmalloc : move unused block to used , p_block_header[%p] data_base[%p]\n" , p_block_header , p_block_header->data_base );
#endif
				list_del( & (p_block_header->block_list_node) );
				list_add_tail( & (p_block_header->block_list_node) , & (p_block_class->used_block_list) );
				
				g_small_mempool_blocks_unused_count--;
				g_small_mempool_blocks_unused_total_size -= block_total_size ;
				g_small_mempool_blocks_used_count++;
				g_small_mempool_blocks_used_total_size += block_total_size ;
				
				p_block_header->alloc_source_file = FILE ;
				p_block_header->alloc_source_line = LINE ;
				p_block_header->alloc_size = size ;
				
#if _DEBUG_QMALLOC
				printf( "_qmalloc : return p_block_header[%p] data_base[%p]\n" , p_block_header , p_block_header->data_base );
#endif
				UNLOCK_SPINLOCK
				return p_block_header->data_base;
			}
		}
		UNLOCK_SPINLOCK
		
		p_block_header = (struct QmallocBlockHeader *)malloc( block_total_size ) ;
		if( p_block_header == NULL )
		{
#if _DEBUG_QMALLOC
			printf( "_qmalloc : malloc MEDIUM_MEMBLOCK [%zu]+[%zu]bytes ok , errno[%d]\n" , sizeof(struct QmallocBlockHeader),ceil_size , errno );
			printf( "_qmalloc : return NULL\n" );
#endif
			return NULL;
		}
#if _DEBUG_QMALLOC
		printf( "_qmalloc : malloc MEDIUM_MEMBLOCK [%zu]+[%zu]bytes ok , p_block_header[%p] data_base[%p]\n" , sizeof(struct QmallocBlockHeader),ceil_size , p_block_header , p_block_header->data_base );
#endif
		
		p_block_header->dmz = 0 ;
		p_block_header->p_block_class = p_block_class ;
		p_block_header->alloc_source_file = FILE ;
		p_block_header->alloc_source_line = LINE ;
		p_block_header->alloc_size = size ;
		
		LOCK_SPINLOCK
		{
			list_add_tail( & (p_block_header->block_list_node) , & (p_block_class->used_block_list) );
			
			g_small_mempool_blocks_used_count++;
			g_small_mempool_blocks_used_total_size += block_total_size ;
		}
		UNLOCK_SPINLOCK
		
#if _DEBUG_QMALLOC
		printf( "_qmalloc : return p_block_header[%p] data_base[%p]\n" , p_block_header , p_block_header->data_base );
#endif
		
		return p_block_header->data_base;
	}
	else if( nret == LARGE_MEMBLOCK )
	{
#if _DEBUG_QMALLOC
		printf( "_qmalloc : _qceil_size return LARGE_MEMBLOCK , block_class_index[%zu] ceil_size[%zu]\n" , block_class_index , ceil_size );
#endif
		
		block_total_size = sizeof(struct QmallocBlockHeader)+size ;
		
		p_block_header = (struct QmallocBlockHeader *)malloc( block_total_size ) ;
		if( p_block_header == NULL )
		{
#if _DEBUG_QMALLOC
			printf( "_qmalloc : malloc LARGE_MEMBLOCK [%zu]bytes failed , errno[%d]\n" , sizeof(struct QmallocBlockHeader)+size , errno );
#endif
			return NULL;
		}
#if _DEBUG_QMALLOC
		printf( "_qmalloc : malloc LARGE_MEMBLOCK [%zu]+[%zu]bytes ok\n" , sizeof(struct QmallocBlockHeader),size );
#endif
		
		p_block_header->dmz = 0 ;
		p_block_header->p_block_class = NULL ;
		p_block_header->alloc_source_file = FILE ;
		p_block_header->alloc_source_line = LINE ;
		p_block_header->alloc_size = size ;
		
		LOCK_SPINLOCK
		{
			list_add_tail( & (p_block_header->block_list_node) , & g_large_mempool_block_list );
			
			g_large_mempool_blocks_used_count++;
			g_large_mempool_blocks_used_total_size += block_total_size ;
		}
		UNLOCK_SPINLOCK
		
#if _DEBUG_QMALLOC
		printf( "_qmalloc : return p_block_header[%p] data_base[%p]\n" , p_block_header , p_block_header->data_base );
#endif
		return p_block_header->data_base;
	}
	else if( nret == INTERNAL_ERROR )
	{
#if _DEBUG_QMALLOC
		printf( "_qmalloc : _qceil_size return INTERNAL_ERROR\n" );
#endif
		
#if _DEBUG_QMALLOC
		printf( "_qmalloc : return NULL\n" );
#endif
		return NULL;
	}
	else
	{
#if _DEBUG_QMALLOC
		printf( "_qmalloc : _qceil_size return UNKNOW\n" );
#endif
		
#if _DEBUG_QMALLOC
		printf( "_qmalloc : return NULL\n" );
#endif
		return NULL;
	}
}

void _qfree( void *ptr )
{
	struct QmallocBlockHeader	*p_block_header ;
	struct QmallocBlockClass	*p_block_class ;
	size_t				block_total_size ;
	
	int				nret ;
#if _DEBUG_QFREE
	printf( "_qfree : ptr[%p]\n" , ptr );
#endif
	
	p_block_header = (struct QmallocBlockHeader *)((char*)ptr-sizeof(struct QmallocBlockHeader)) ;
	p_block_class = p_block_header->p_block_class ;
#if _DEBUG_QFREE
	printf( "_qfree : p_block_header[%p] data_base[%p] alloc_source_file[%s] alloc_source_line[%zu] alloc_size[%zu] , p_block_class[%p] block_size[%zu]\n" , p_block_header , p_block_header->data_base , p_block_header->alloc_source_file , p_block_header->alloc_source_line , p_block_header->alloc_size , p_block_class , (p_block_class?p_block_class->block_size:0) );
#endif
	
	if( p_block_class )
		nret = _qget_size_class( p_block_class->block_size ) ;
	else
		nret = _qget_size_class( p_block_header->alloc_size ) ;
	if( nret == SMALL_MEMBLOCK )
	{
#if _DEBUG_QFREE
		printf( "_qfree : move SMALL_MEMBLOCK [%zu]+[%zu]bytes used block to unused , p_block_header[%p] data_base[%p]\n" , sizeof(struct QmallocBlockHeader),p_block_header->alloc_size , p_block_header , p_block_header->data_base );
#endif
		
		block_total_size = sizeof(struct QmallocBlockHeader)+p_block_class->block_size ;
		
		p_block_header->alloc_source_file = NULL ;
		p_block_header->alloc_source_line = 0 ;
		p_block_header->alloc_size = 0 ;
		
		list_del( & (p_block_header->block_list_node) );
		list_add_tail( & (p_block_header->block_list_node) , & (p_block_class->unused_block_list) );
		
		g_small_mempool_blocks_used_count--;
		g_small_mempool_blocks_used_total_size -= block_total_size ;
		g_small_mempool_blocks_unused_count++;
		g_small_mempool_blocks_unused_total_size += block_total_size ;
	}
	else if( nret == MEDIUM_MEMBLOCK )
	{
#if _DEBUG_QFREE
		printf( "_qfree : move MEDIUM_MEMBLOCK [%zu]+[%zu]bytes used block to unused , p_block_header[%p] data_base[%p]\n" , sizeof(struct QmallocBlockHeader),p_block_header->alloc_size , p_block_header , p_block_header->data_base );
#endif
		
		block_total_size = sizeof(struct QmallocBlockHeader)+p_block_class->block_size ;
		
		p_block_header->alloc_source_file = NULL ;
		p_block_header->alloc_source_line = 0 ;
		p_block_header->alloc_size = 0 ;
		
		LOCK_SPINLOCK
		{
			list_del( & (p_block_header->block_list_node) );
			list_add_tail( & (p_block_header->block_list_node) , & (p_block_class->unused_block_list) );
			
			g_medium_mempool_blocks_used_count--;
			g_medium_mempool_blocks_used_total_size -= block_total_size ;
			g_medium_mempool_blocks_unused_count++;
			g_medium_mempool_blocks_unused_total_size += block_total_size ;
		}
		UNLOCK_SPINLOCK
	}
	else if( nret == LARGE_MEMBLOCK )
	{
#if _DEBUG_QFREE
		printf( "_qfree : free BIG_MEMBLOCK [%zu]+[%zu]bytes , p_block_header[%p]\n" , sizeof(struct QmallocBlockHeader),p_block_header->alloc_size , p_block_header );
#endif
		
		block_total_size = sizeof(struct QmallocBlockHeader)+p_block_header->alloc_size ;
		
		LOCK_SPINLOCK
		{
			list_del( & (p_block_header->block_list_node) );
			free(p_block_header);
			
			g_large_mempool_blocks_used_count--;
			g_large_mempool_blocks_used_total_size -= block_total_size ;
		}
		UNLOCK_SPINLOCK
	}
	else
	{
#if _DEBUG_QFREE
		printf( "_qfree : return INTERNAL_ERROR\n" );
#endif
		return;
	}
	
	return;
}

void *_qrealloc( void *ptr , size_t size , char *FILE , int LINE )
{
	void		*ptr2 ;
	
	if( ptr == NULL )
		return NULL;
	
	ptr2 = _qmalloc( size , FILE , LINE ) ;
	if( ptr2 == NULL )
		return NULL;
	
	_qfree( ptr );
	return ptr2;
}

void *_qstrdup( const char *s , char *FILE , int LINE )
{
	char	*p ;
	size_t	len = strlen(s) ;
	
	p = _qmalloc( len+1 , FILE , LINE ) ;
	if( p == NULL )
		return NULL;
	memcpy( p , s , len );
	p[len] = '\0' ;
	
	return p;
}

void *_qstrndup( const char *s , size_t n , char *FILE , int LINE )
{
	char	*p ;
	
	p = _qmalloc( n+1 , FILE , LINE ) ;
	if( p == NULL )
		return NULL;
	memcpy( p , s , n );
	p[n] = '\0' ;
	
	return p;
}

size_t qstat_used_blocks_count()
{
	size_t		blocks_count ;
	
	LOCK_SPINLOCK
	{
		blocks_count = g_small_mempool_blocks_used_count + g_medium_mempool_blocks_used_count + g_large_mempool_blocks_used_count ;
	}
	UNLOCK_SPINLOCK
	
	return blocks_count;
}

size_t qstat_used_blocks_total_size()
{
	size_t		blocks_total_size ;
	
	LOCK_SPINLOCK
	{
		blocks_total_size = g_small_mempool_blocks_used_total_size + g_medium_mempool_blocks_used_total_size + g_large_mempool_blocks_used_total_size ;
	}
	UNLOCK_SPINLOCK
	
	return blocks_total_size;
}

size_t qstat_unused_blocks_count()
{
	size_t		blocks_count ;
	
	LOCK_SPINLOCK
	{
		blocks_count = g_small_mempool_blocks_unused_count + g_medium_mempool_blocks_unused_count ;
	}
	UNLOCK_SPINLOCK
	
	return blocks_count;
}

size_t qstat_unused_blocks_total_size()
{
	size_t		blocks_total_size ;
	
	LOCK_SPINLOCK
	{
		blocks_total_size = g_small_mempool_blocks_unused_total_size + g_medium_mempool_blocks_unused_total_size ;
	}
	UNLOCK_SPINLOCK
	
	return blocks_total_size;
}

#define member_of(_base_,_type_,_offset_of_struct_)	(_type_*)((char*)(_base_)+(_offset_of_struct_))

static void *_qtravel_block( void *ptr , size_t block_list_offset_of_struct )
{
	size_t				block_class_index ;
	struct QmallocBlockClass	*p_block_class ;
	struct QmallocBlockHeader	*p_block_header ;
	struct list_head		*p_next_node ;
	struct QmallocBlockHeader	*p_next_block_header ;
	
	int				nret = 0 ;
	
	nret = _qinit() ;
	if( nret )
	{
		return NULL;
	}
	
	if( ptr == NULL )
	{
		LOCK_SPINLOCK
		{
			for( block_class_index=0 ; block_class_index<=MAX_SMALL_MEMPOOL_BLOCK_SIZE_CLASS ; block_class_index++ )
			{
				p_block_class = g_small_mempool_blocks_class_array+block_class_index ;
				
				if( ! list_empty( member_of(p_block_class,struct list_head,block_list_offset_of_struct) ) )
				{
					p_block_header = list_first_entry( member_of(p_block_class,struct list_head,block_list_offset_of_struct) , struct QmallocBlockHeader , block_list_node ) ;
					UNLOCK_SPINLOCK
					return p_block_header->data_base;
				}
			}
			
			for( block_class_index=0 ; block_class_index<=DIFF_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS ; block_class_index++ )
			{
				p_block_class = g_medium_mempool_blocks_class_array+block_class_index ;
				
				if( ! list_empty( member_of(p_block_class,struct list_head,block_list_offset_of_struct) ) )
				{
					p_block_header = list_first_entry( member_of(p_block_class,struct list_head,block_list_offset_of_struct) , struct QmallocBlockHeader , block_list_node ) ;
					UNLOCK_SPINLOCK
					return p_block_header->data_base;
				}
			}
			
			if( block_list_offset_of_struct == offsetof(struct QmallocBlockClass,used_block_list) )
			{
				if( ! list_empty( & g_large_mempool_block_list ) )
				{
					p_block_header = list_first_entry( & g_large_mempool_block_list , struct QmallocBlockHeader , block_list_node ) ;
					UNLOCK_SPINLOCK
					return p_block_header->data_base;
				}
			}
		}
		UNLOCK_SPINLOCK
		
		return NULL;
	}
	else
	{
		LOCK_SPINLOCK
		{
			p_block_header = (struct QmallocBlockHeader *)((char*)ptr-sizeof(struct QmallocBlockHeader)) ;
			p_block_class = p_block_header->p_block_class ;
			
			p_next_node = list_next( p_block_header , block_list_node ) ;
			if( p_block_class )
			{
				if( p_next_node != member_of(p_block_class,struct list_head,block_list_offset_of_struct) )
				{
					p_next_block_header = list_entry( p_next_node , struct QmallocBlockHeader , block_list_node ) ;
					UNLOCK_SPINLOCK
					return p_next_block_header->data_base;
				}
			}
			else if( block_list_offset_of_struct == offsetof(struct QmallocBlockClass,used_block_list) )
			{
				if( p_next_node != & g_large_mempool_block_list )
				{
					p_next_block_header = list_entry( p_next_node , struct QmallocBlockHeader , block_list_node ) ;
					UNLOCK_SPINLOCK
					return p_next_block_header->data_base;
				}
				else
				{
					UNLOCK_SPINLOCK
#if _DEBUG_QTRAVEL
					printf( "_qtravel_block : return NULL\n" );
#endif
					return NULL;
				}
			}
			
#if _DEBUG_QTRAVEL
			printf( "_qtravel_block : p_block_class[%p]\n" , p_block_class );
#endif
			if( p_block_class == g_small_mempool_blocks_class_array+MAX_SMALL_MEMPOOL_BLOCK_SIZE_CLASS )
				p_block_class = g_medium_mempool_blocks_class_array ;
			else if( p_block_class == g_medium_mempool_blocks_class_array+DIFF_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS )
				p_block_class = NULL ;
			else if( p_block_class )
				p_block_class++;
			
			if( g_small_mempool_blocks_class_array <= p_block_class && p_block_class <= g_small_mempool_blocks_class_array+MAX_SMALL_MEMPOOL_BLOCK_SIZE_CLASS )
			{
#if _DEBUG_QTRAVEL
				printf( "_qtravel_block : in small mempool\n" );
#endif
				while(1)
				{
#if _DEBUG_QTRAVEL
					printf( "_qtravel_block : p_block_class[%p]\n" , p_block_class );
#endif
					if( p_block_class > g_small_mempool_blocks_class_array+MAX_SMALL_MEMPOOL_BLOCK_SIZE_CLASS )
					{
						p_block_class = g_medium_mempool_blocks_class_array ;
#if _DEBUG_QTRAVEL
						printf( "_qtravel_block : break\n" );
#endif
						break;
					}
					
					if( ! list_empty( member_of(p_block_class,struct list_head,block_list_offset_of_struct) ) )
					{
						p_block_header = list_first_entry( member_of(p_block_class,struct list_head,block_list_offset_of_struct) , struct QmallocBlockHeader , block_list_node ) ;
						UNLOCK_SPINLOCK
#if _DEBUG_QTRAVEL
						printf( "_qtravel_block : return p_block_header[%p] data_base[%p]\n" , p_block_header , p_block_header->data_base );
#endif
						return p_block_header->data_base;
					}
					
					p_block_class++;
				}
			}
			
			if( g_medium_mempool_blocks_class_array <= p_block_class && p_block_class <= g_medium_mempool_blocks_class_array+DIFF_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS )
			{
#if _DEBUG_QTRAVEL
				printf( "_qtravel_block : in medium mempool\n" );
#endif
				while(1)
				{
#if _DEBUG_QTRAVEL
					printf( "_qtravel_block : p_block_class[%p]\n" , p_block_class );
#endif
					if( p_block_class > g_medium_mempool_blocks_class_array+DIFF_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS )
					{
						p_block_class = NULL ;
						break;
					}
					
					if( ! list_empty( member_of(p_block_class,struct list_head,block_list_offset_of_struct) ) )
					{
						p_block_header = list_first_entry( member_of(p_block_class,struct list_head,block_list_offset_of_struct) , struct QmallocBlockHeader , block_list_node ) ;
						UNLOCK_SPINLOCK
#if _DEBUG_QTRAVEL
						printf( "_qtravel_block : return p_block_header[%p] data_base[%p]\n" , p_block_header , p_block_header->data_base );
#endif
						return p_block_header->data_base;
					}
					
					p_block_class++;
				}
			}
			
			if( p_block_class == NULL )
			{
				if( block_list_offset_of_struct == offsetof(struct QmallocBlockClass,used_block_list) )
				{
#if _DEBUG_QTRAVEL
					printf( "_qtravel_block : in large mempool\n" );
#endif
					if( ! list_empty( & g_large_mempool_block_list ) )
					{
						p_block_header = list_first_entry( & g_large_mempool_block_list , struct QmallocBlockHeader , block_list_node ) ;
						UNLOCK_SPINLOCK
#if _DEBUG_QTRAVEL
						printf( "_qtravel_block : return p_block_header[%p] data_base[%p]\n" , p_block_header , p_block_header->data_base );
#endif
						return p_block_header->data_base;
					}
					else
					{
						UNLOCK_SPINLOCK
#if _DEBUG_QTRAVEL
						printf( "_qtravel_block : return NULL\n" );
#endif
						return NULL;
					}
				}
			}
		}
		UNLOCK_SPINLOCK
#if _DEBUG_QTRAVEL
		printf( "_qtravel_block : return NULL\n" );
#endif
		return NULL;
	}
}

void *qtravel_used_blocks( void *ptr )
{
	return _qtravel_block( ptr , offsetof(struct QmallocBlockClass,used_block_list) );
}

void *qtravel_unused_blocks( void *ptr )
{
	return _qtravel_block( ptr , offsetof(struct QmallocBlockClass,unused_block_list) );
}

char *qget_alloc_source_file( void *ptr )
{
	struct QmallocBlockHeader	*p_block_header ;
	
	p_block_header = (struct QmallocBlockHeader *)((char*)ptr-sizeof(struct QmallocBlockHeader)) ;
	
	return p_block_header->alloc_source_file;
}

int qget_alloc_source_line( void *ptr )
{
	struct QmallocBlockHeader	*p_block_header ;
	
	p_block_header = (struct QmallocBlockHeader *)((char*)ptr-sizeof(struct QmallocBlockHeader)) ;
	
	return p_block_header->alloc_source_line;
}

size_t qget_alloc_size( void *ptr )
{
	struct QmallocBlockHeader	*p_block_header ;
	
	p_block_header = (struct QmallocBlockHeader *)((char*)ptr-sizeof(struct QmallocBlockHeader)) ;
	
	return p_block_header->alloc_size;
}

size_t qget_block_size( void *ptr )
{
	struct QmallocBlockHeader	*p_block_header ;
	struct QmallocBlockClass	*p_block_class ;
	
	p_block_header = (struct QmallocBlockHeader *)((char*)ptr-sizeof(struct QmallocBlockHeader)) ;
	p_block_class = p_block_header->p_block_class ;
	
	if( p_block_class )
		return p_block_header->p_block_class->block_size;
	else
		return p_block_header->alloc_size;
}

void qfree_all_unused()
{
	size_t				block_class_index ;
	struct QmallocBlockClass	*p_block_class ;
	struct QmallocBlockHeader	*p_block_header ;
	struct QmallocBlockHeader	*p_next_block_header ;
	size_t				block_total_size ;
	
	int				nret = 0 ;
	
	nret = _qinit() ;
	if( nret )
	{
#if _DEBUG_QMALLOC
		printf( "qfree_all_unused\n" );
#endif
		return;
	}
	
	for( block_class_index=0 ; block_class_index<=MAX_SMALL_MEMPOOL_BLOCK_SIZE_CLASS ; block_class_index++ )
	{
		p_block_class = g_small_mempool_blocks_class_array + block_class_index ;
		block_total_size = sizeof(struct QmallocBlockHeader)+p_block_class->block_size ;
		list_for_each_entry_safe( p_block_header , p_next_block_header , & (p_block_class->unused_block_list) , struct QmallocBlockHeader , block_list_node )
		{
#if _DEBUG_QMALLOC
			printf( "qfree_all_unused : unlink and free p_block_header[%p]\n" , p_block_header );
#endif
			list_del( & (p_block_header->block_list_node) );
			free( p_block_header );
			g_small_mempool_blocks_unused_count--;
			g_small_mempool_blocks_unused_total_size -= block_total_size ;
		}
	}
	
	LOCK_SPINLOCK
	{
		for( block_class_index=0 ; block_class_index<=DIFF_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS ; block_class_index++ )
		{
			p_block_class = g_medium_mempool_blocks_class_array + block_class_index ;
			block_total_size = sizeof(struct QmallocBlockHeader)+p_block_class->block_size ;
			list_for_each_entry_safe( p_block_header , p_next_block_header , & (p_block_class->unused_block_list) , struct QmallocBlockHeader , block_list_node )
			{
#if _DEBUG_QMALLOC
				printf( "qfree_all_unused : unlink and free p_block_header[%p]\n" , p_block_header );
#endif
				list_del( & (p_block_header->block_list_node) );
				free( p_block_header );
				g_medium_mempool_blocks_unused_count--;
				g_medium_mempool_blocks_unused_total_size -= block_total_size ;
			}
		}
	}
	UNLOCK_SPINLOCK
	
#if _DEBUG_QMALLOC
	printf( "qfree_all_unused : return\n" );
#endif
	return;
}

size_t qget_block_header_size()
{
	return sizeof(struct QmallocBlockHeader);
}

size_t qget_thread_mempool_block_size()
{
	return MAX_SMALL_MEMPOOL_BLOCK_SIZE;
}

size_t qget_process_mempool_block_size()
{
	return MAX_MEDIUM_MEMPOOL_BLOCK_SIZE;
}

