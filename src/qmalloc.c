#include "qmalloc_in.h"

__thread struct QmallocBlockClass	*g_thread_mempool_blocks_class_array = NULL ;
__thread size_t				g_thread_mempool_blocks_count = 0 ;
__thread size_t				g_thread_mempool_blocks_total_size = 0 ;

struct QmallocBlockClass		*g_process_mempool_blocks_class_array = NULL ;
size_t					g_process_mempool_blocks_count = 0 ;
size_t					g_process_mempool_blocks_total_size = 0 ;

int	g_spanlock_status = SPINLOCK_UNLOCK ;

static inline int _qceil_size( size_t size , size_t *p_block_class_index , size_t *p_ceil_size )
{
	register size_t		block_class_index ;
	register size_t		ceil_size ;
	
	if( size == 0 )
	{
		(*p_block_class_index) = 0 ;
		(*p_ceil_size) = 0 ;
		return 0;
	}
	else if( size > MAX_PROCESS_MEMPOOL_BLOCK_SIZE )
	{
		return -1;
	}
	
	for( block_class_index=0 , ceil_size=1 ; ceil_size <= MAX_PROCESS_MEMPOOL_BLOCK_SIZE ; block_class_index++ )
	{
		if( size <= ceil_size )
		{
			(*p_block_class_index) = block_class_index ;
			(*p_ceil_size) = ceil_size ;
			return 0;
		}
		
		ceil_size <<= 1 ;
	}
	
	return -2;
}

static inline int _qinit()
{
	size_t				block_class_index ;
	size_t				block_size ;
	struct QmallocBlockClass	*p_block_class ;
	
	if( g_thread_mempool_blocks_class_array == NULL )
	{
		g_thread_mempool_blocks_class_array = (struct QmallocBlockClass *)malloc( sizeof(struct QmallocBlockClass) * (MAX_THREAD_MEMPOOL_BLOCK_SIZE_CLASS+1) ) ;
		if( g_thread_mempool_blocks_class_array == NULL )
		{
			return -1;
		}
		
		for( block_class_index=0 , block_size=1 ; block_class_index<=MAX_THREAD_MEMPOOL_BLOCK_SIZE_CLASS ; block_class_index++ , block_size<<=1 )
		{
			p_block_class = g_thread_mempool_blocks_class_array + block_class_index ;
			
			p_block_class->block_size = block_size ;
			INIT_LIST_HEAD( & (p_block_class->used_block_list) );
			INIT_LIST_HEAD( & (p_block_class->unused_block_list) );
		}
	}
	
	if( g_process_mempool_blocks_class_array == NULL )
	{
		LOCK_SPINLOCK
		
		if( g_process_mempool_blocks_class_array == NULL )
		{
			g_process_mempool_blocks_class_array = (struct QmallocBlockClass *)malloc( sizeof(struct QmallocBlockClass) * (DIFF_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS+1) ) ;
			if( g_process_mempool_blocks_class_array == NULL )
			{
				UNLOCK_SPINLOCK
				return -2;
			}
			
			for( block_class_index=0 , block_size=1<<(MAX_THREAD_MEMPOOL_BLOCK_SIZE_CLASS+1) ; block_class_index<=DIFF_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS ; block_class_index++ , block_size<<=1 )
			{
				p_block_class = g_thread_mempool_blocks_class_array + block_class_index ;
				
				p_block_class->block_size = block_size ;
				INIT_LIST_HEAD( & (p_block_class->used_block_list) );
				INIT_LIST_HEAD( & (p_block_class->unused_block_list) );
			}
		}
		
		UNLOCK_SPINLOCK
	}
	
	return 0;
}

void *_qmalloc( size_t size , char *FILE , int LINE )
{
	size_t		block_class_index ;
	size_t		ceil_size ;
	
	int		nret = 0 ;
	
	nret = _qinit() ;
	if( nret )
		return NULL;
	
	nret = _qceil_size( size , & block_class_index , & ceil_size ) ;
	if( nret == -1 )
	{
		struct QmallocBlockHeader	*p_block_header ;
		
		p_block_header = (struct QmallocBlockHeader *)malloc( sizeof(struct QmallocBlockHeader) + size ) ;
		if( p_block_header == NULL )
		{
			return NULL;
		}
		
		p_block_header->dmz = 0 ;
		p_block_header->p_block_class = NULL ;
		p_block_header->alloc_source_file = FILE ;
		p_block_header->alloc_source_line = LINE ;
		
		return p_block_header->data_base;
	}
	else if( nret == -2 )
		return NULL;
	
	if( block_class_index <= MAX_THREAD_MEMPOOL_BLOCK_SIZE_CLASS )
	{
		struct QmallocBlockClass	*p_block_class = NULL ;
		struct QmallocBlockHeader	*p_block_header ;
		
		p_block_class = g_thread_mempool_blocks_class_array + block_class_index ;
		
		if( ! list_empty( & (p_block_class->unused_block_list) ) )
		{
			p_block_header = list_first_entry( & (p_block_class->unused_block_list) , struct QmallocBlockHeader , block_list_node ) ;
			list_del( & (p_block_header->block_list_node) );
			list_add_tail( & (p_block_header->block_list_node) , & (p_block_class->used_block_list) );
			
			p_block_header->alloc_source_file = FILE ;
			p_block_header->alloc_source_line = LINE ;
			
			return p_block_header->data_base;
		}
		
		p_block_header = (struct QmallocBlockHeader *)malloc( sizeof(struct QmallocBlockHeader) + ceil_size ) ;
		if( p_block_header == NULL )
		{
			return NULL;
		}
		
		p_block_header->dmz = 0 ;
		p_block_header->p_block_class = p_block_class ;
		p_block_header->alloc_source_file = FILE ;
		p_block_header->alloc_source_line = LINE ;
		list_add_tail( & (p_block_header->block_list_node) , & (p_block_class->used_block_list) );
		
		return p_block_header->data_base;
	}
	else if( block_class_index <= MAX_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS )
	{
		struct QmallocBlockClass	*p_block_class = NULL ;
		struct QmallocBlockHeader	*p_block_header ;
		
		LOCK_SPINLOCK
		
		p_block_class = g_process_mempool_blocks_class_array + (block_class_index-MAX_THREAD_MEMPOOL_BLOCK_SIZE_CLASS) ;
		
		if( ! list_empty( & (p_block_class->unused_block_list) ) )
		{
			p_block_header = list_first_entry( & (p_block_class->unused_block_list) , struct QmallocBlockHeader , block_list_node ) ;
			list_del( & (p_block_header->block_list_node) );
			list_add_tail( & (p_block_header->block_list_node) , & (p_block_class->used_block_list) );
			
			p_block_header->alloc_source_file = FILE ;
			p_block_header->alloc_source_line = LINE ;
			
			UNLOCK_SPINLOCK
			
			return p_block_header->data_base;
		}
		
		p_block_header = (struct QmallocBlockHeader *)malloc( sizeof(struct QmallocBlockHeader) + ceil_size ) ;
		if( p_block_header == NULL )
		{
			UNLOCK_SPINLOCK
			return NULL;
		}
		
		p_block_header->dmz = 0 ;
		p_block_header->p_block_class = p_block_class ;
		p_block_header->alloc_source_file = FILE ;
		p_block_header->alloc_source_line = LINE ;
		list_add_tail( & (p_block_header->block_list_node) , & (p_block_class->used_block_list) );
		
		UNLOCK_SPINLOCK
		
		return p_block_header->data_base;
	}
	else
	{
		return NULL;
	}
}

void _qfree( void *ptr )
{
	struct QmallocBlockHeader	*p_block_header ;
	struct QmallocBlockClass	*p_block_class ;
	
	p_block_header = (struct QmallocBlockHeader *)((char*)ptr-sizeof(struct QmallocBlockHeader)) ;
	if( p_block_header->p_block_class == NULL )
	{
		free(p_block_header);
		return;
	}
	
	p_block_class = p_block_header->p_block_class ;
	
	p_block_header->alloc_source_file = NULL ;
	p_block_header->alloc_source_line = 0 ;
	list_del( & (p_block_header->block_list_node) );
	list_add_tail( & (p_block_header->block_list_node) , & (p_block_class->unused_block_list) );
	
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
	return g_thread_mempool_blocks_count;
}

size_t qstat_used_blocks_total_size()
{
	return g_thread_mempool_blocks_total_size;
}

size_t qstat_unused_blocks_count()
{
	return g_process_mempool_blocks_count;
}

size_t qstat_unused_blocks_total_size()
{
	return g_process_mempool_blocks_total_size;
}

#define member_of(_base_,_type_,_offset_of_struct_)	(_type_*)((_base_)+(_offset_of_struct_))

static void *_qtravel_block( void *ptr , size_t block_list_offset_of_struct )
{
	size_t				block_class_index ;
	struct QmallocBlockClass	*p_block_class ;
	struct QmallocBlockHeader	*p_block_header ;
	struct list_head		*p_next_node ;
	struct QmallocBlockHeader	*p_next_block_header ;
	
	if( ptr == NULL )
	{
		for( block_class_index=0 ; block_class_index<=MAX_THREAD_MEMPOOL_BLOCK_SIZE_CLASS ; block_class_index++ )
		{
			p_block_class = g_thread_mempool_blocks_class_array + block_class_index ;
			
			if( ! list_empty( member_of(p_block_class,struct list_head,block_list_offset_of_struct) ) )
			{
				p_block_header = list_first_entry( & (p_block_class->unused_block_list) , struct QmallocBlockHeader , block_list_node ) ;
				return p_block_header->data_base;
			}
		}
		
		LOCK_SPINLOCK
		
		for( block_class_index=0 ; block_class_index<=DIFF_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS ; block_class_index++ )
		{
			p_block_class = g_process_mempool_blocks_class_array + block_class_index ;
			
			if( ! list_empty( member_of(p_block_class,struct list_head,block_list_offset_of_struct) ) )
			{
				p_block_header = list_first_entry( member_of(p_block_class,struct list_head,block_list_offset_of_struct) , struct QmallocBlockHeader , block_list_node ) ;
				return p_block_header->data_base;
			}
		}
		
		UNLOCK_SPINLOCK
		
		return NULL;
	}
	else
	{
		p_block_header = (struct QmallocBlockHeader *)((char*)ptr-sizeof(struct QmallocBlockHeader)) ;
		p_block_class = p_block_header->p_block_class ;
		
		while(1)
		{
			p_next_node = list_next( p_block_header , block_list_node ) ;
			if( p_next_node != member_of(p_block_class,struct list_head,block_list_offset_of_struct) )
			{
				p_next_block_header =  list_entry( p_next_node , struct QmallocBlockHeader , block_list_node ) ;
				return p_next_block_header->data_base;
			}
			
			LOCK_SPINLOCK
			
			if( g_thread_mempool_blocks_class_array <= p_block_class && p_block_class <= g_thread_mempool_blocks_class_array+MAX_THREAD_MEMPOOL_BLOCK_SIZE_CLASS )
			{
				while(1)
				{
					p_block_class++;
					if( p_block_class > g_thread_mempool_blocks_class_array+MAX_THREAD_MEMPOOL_BLOCK_SIZE_CLASS )
					{
						UNLOCK_SPINLOCK
						return NULL;
					}
					
					if( ! list_empty( member_of(p_block_class,struct list_head,block_list_offset_of_struct) ) )
					{
						p_block_header = list_first_entry( member_of(p_block_class,struct list_head,block_list_offset_of_struct) , struct QmallocBlockHeader , block_list_node ) ;
						return p_block_header->data_base;
					}
				}
			}
			else if( g_process_mempool_blocks_class_array <= p_block_class && p_block_class <= g_process_mempool_blocks_class_array+DIFF_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS )
			{
				while(1)
				{
					p_block_class++;
					if( p_block_class > g_process_mempool_blocks_class_array+MAX_THREAD_MEMPOOL_BLOCK_SIZE_CLASS )
					{
						UNLOCK_SPINLOCK
						return NULL;
					}
					
					if( ! list_empty( member_of(p_block_class,struct list_head,block_list_offset_of_struct) ) )
					{
						p_block_header = list_first_entry( member_of(p_block_class,struct list_head,block_list_offset_of_struct) , struct QmallocBlockHeader , block_list_node ) ;
						return p_block_header->data_base;
					}
				}
			}
			else
			{
				UNLOCK_SPINLOCK
				return NULL;
			}
			
			UNLOCK_SPINLOCK
		}
		
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

size_t qget_size( void *ptr )
{
	struct QmallocBlockHeader	*p_block_header ;
	struct QmallocBlockClass	*p_block_class ;
	
	p_block_header = (struct QmallocBlockHeader *)((char*)ptr-sizeof(struct QmallocBlockHeader)) ;
	p_block_class = p_block_header->p_block_class ;
	
	return p_block_class->block_size;
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

void qfree_all_unused()
{
	size_t				block_class_index ;
	struct QmallocBlockClass	*p_block_class ;
	struct QmallocBlockHeader	*p_block_header ;
	struct QmallocBlockHeader	*p_next_block_header ;
	
	int				nret = 0 ;
	
	nret = _qinit() ;
	if( nret )
		return;
	
	for( block_class_index=0 ; block_class_index<=MAX_THREAD_MEMPOOL_BLOCK_SIZE_CLASS ; block_class_index++ )
	{
		p_block_class = g_thread_mempool_blocks_class_array + block_class_index ;
		
		list_for_each_entry_safe( p_block_header , p_next_block_header , & (p_block_class->unused_block_list) , struct QmallocBlockHeader , block_list_node )
		{
			list_del( & (p_block_header->block_list_node) );
			
			free( p_block_header );
		}
	}
	
	LOCK_SPINLOCK
	
	for( block_class_index=0 ; block_class_index<=DIFF_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS ; block_class_index++ )
	{
		p_block_class = g_process_mempool_blocks_class_array + block_class_index ;
		
		list_for_each_entry_safe( p_block_header , p_next_block_header , & (p_block_class->unused_block_list) , struct QmallocBlockHeader , block_list_node )
		{
			list_del( & (p_block_header->block_list_node) );
			
			free( p_block_header );
		}
	}
	
	UNLOCK_SPINLOCK
	
	return;
}

size_t qget_block_header_size()
{
	return sizeof(struct QmallocBlockHeader);
}

size_t qget_thread_mempool_block_size()
{
	return MAX_THREAD_MEMPOOL_BLOCK_SIZE;
}

size_t qget_process_mempool_block_size()
{
	return MAX_PROCESS_MEMPOOL_BLOCK_SIZE;
}

