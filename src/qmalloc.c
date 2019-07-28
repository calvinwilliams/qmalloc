#include "qmalloc_in.h"

#define _DEBUG			0

__thread struct QmallocBlockClassTree	g_used_qmalloc_block_class_tree = { RB_ROOT,RB_ROOT,0,0 } ;
__thread struct QmallocBlockClassTree	g_unused_qmalloc_block_class_tree = { RB_ROOT,RB_ROOT,0,0 } ;

__thread size_t				g_cache_blocks_max_size = 100*1024*1024 ;

LINK_RBTREENODE_INT( LinkQmallocBlockClassToTreeByDataSize , struct QmallocBlockClassTree , block_class_tree_order_by_size , struct QmallocBlock , block_class_tree_node_order_by_size , data_size )
UNLINK_RBTREENODE( UnlinkQmallocBlockClassFromTreeByDataSize , struct QmallocBlockClassTree , block_class_tree_order_by_size , struct QmallocBlock , block_class_tree_node_order_by_size )
QUERY_RBTREENODE_INT( QueryQmallocBlockClassTreeByDataSize , struct QmallocBlockClassTree , block_class_tree_order_by_size , struct QmallocBlock , block_class_tree_node_order_by_size , data_size )
QUERY_RBTREENODE_INT_NOLESSTHAN( QueryQmallocBlockClassTreeByDataSizeNolessthan , struct QmallocBlockClassTree , block_class_tree_order_by_size , struct QmallocBlock , block_class_tree_node_order_by_size , data_size )
TRAVEL_RBTREENODE( TravelQmallocBlockClassTreeByDataSize , struct QmallocBlockClassTree , block_class_tree_order_by_size , struct QmallocBlock , block_class_tree_node_order_by_size )

LINK_RBTREENODE_POINTER( LinkQmallocBlockToTreeByBlockAddr , struct QmallocBlockClassTree , block_tree_order_by_addr , struct QmallocBlock , block_tree_node_order_by_addr , block_addr )
UNLINK_RBTREENODE( UnlinkQmallocBlockFromTreeByAddr , struct QmallocBlockClassTree , block_tree_order_by_addr , struct QmallocBlock , block_tree_node_order_by_addr )
QUERY_RBTREENODE_POINTER( QueryQmallocBlockTreeByAddr , struct QmallocBlockClassTree , block_tree_order_by_addr , struct QmallocBlock , block_tree_node_order_by_addr , block_addr )
TRAVEL_RBTREENODE( TravelQmallocBlockTreeByAddr , struct QmallocBlockClassTree , block_tree_order_by_addr , struct QmallocBlock , block_tree_node_order_by_addr )

static inline struct QmallocBlockClass *AllocAndLinkBlockClass( size_t size , struct QmallocBlockClassTree *p_block_class_tree )
{
	struct QmallocBlockClass	*p_block_class ;
	
	p_block_class = (struct QmallocBlockClass *)malloc( sizeof(struct QmallocBlockClass) ) ;
	if( p_block_class == NULL )
	{
#if _DEBUG
printf( "_qmalloc : call malloc QmallocBlockClass failed , errno[%d]\n" , errno );
printf( "_qmalloc : return[%p]\n" , NULL );
#endif
		return NULL;
	}
	p_block_class->data_size = size ;
	LIST_HEAD_INIT( p_block_class->block_list )
	LinkQmallocBlockClassToTreeByDataSize( p_block_class_tree , p_block_class );
	
	p_block_class_tree->blocks_total_size += sizeof(struct QmallocBlockClass) ;
	
	return p_block_class;
}

static inline struct QmallocBlock *AllocAndLinkBlock( unsigned char alloc_page_flag , struct QmallocBlockClass *p_block_class , char *FILE , int LINE , struct QmallocBlockClassTree *p_block_class_tree )
{
	struct QmallocBlock		*p_block ;
	size_t				total_block_size ;
	
	total_block_size = sizeof(struct QmallocBlock) + size ;
	p_block = (struct QmallocBlock *)malloc( total_block_size ) ;
	if( p_block == NULL )
	{
#if _DEBUG
printf( "_qmalloc : call malloc QmallocBlock failed , errno[%d]\n" , errno );
printf( "_qmalloc : return[%p]\n" , NULL );
#endif
		return NULL;
	}
	p_block->dmz[0] = '\0' ;
	p_block->alloc_page_flag = alloc_page_flag ;
	p_block->p_block_class = p_block_class ;
	p_block->block_addr = p_block ;
	p_block->alloc_source_file = FILE ;
	p_block->alloc_source_line = LINE ;
	LinkQmallocBlockToTreeByBlockAddr( p_block_class_tree , p_block );
	
	list_add_tail( &(p_block->block_list_node) , &(p_block_class->block_list) )
	
	p_block_class_tree->blocks_count++;
	p_block_class_tree->blocks_total_size += total_block_size ;
	
	return 
}

void *_qmalloc( size_t size , char *FILE , int LINE )
{
	struct QmallocBlockClass	block_class ;
	struct QmallocBlockClass	*p_block_class ;
	struct QmallocBlock		block ;
	struct QmallocBlock		*p_block ;
	
#if _DEBUG
printf( "_qmalloc : size[%zu] FILE[%s] LINE[%d]\n" , size , FILE , LINE );
#endif
	if( size == 0 )
	{
#if _DEBUG
printf( "_qmalloc : return[%p]\n" , NULL );
#endif
		return NULL;
	}
	
	size = MEMADDR_ALIGN(sizeof(struct QmallocBlock)+size) - sizeof(struct QmallocBlock) ; /* 内存地址对齐，与rb_node.rb_parent_color配合 */
	
	if( size > NORMAL_BLOCK_MAX_SIZE )
	{
		size_t		total_block_size ;
		
		block_class.data_size = size ;
		p_block_class = QueryQmallocBlockClassTreeByDataSize( & g_unused_qmalloc_block_class_tree , & block_class ) ;
		if( p_block_class == NULL )
		{
			p_block_class = AllocAndLinkBlockClass( size , & g_used_qmalloc_block_class_tree ) ;
			if( p_block_class == NULL )
				return NULL;
		}
		
		p_block = AllocAndLinkBlock( 1 , p_block_class , FILE , LINE , & g_used_qmalloc_block_class_tree ) ;
		if( p_block == NULL )
			return NULL;
		
		return p_block->data_addr;
	}
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
#if 0
	{
		if( p_block_class )
		{
			UnlinkQmallocBlockClassFromTreeByDataSize( & g_unused_qmalloc_block_class_tree , p_block );
			UnlinkQmallocBlockFromTreeByAddr( & g_unused_qmalloc_block_class_tree , p_block );
			g_unused_qmalloc_block_class_tree.blocks_count--;
			g_unused_qmalloc_block_class_tree.blocks_total_size -= sizeof(struct QmallocBlock)+p_block->data_size ;
			
			p_block->alloc_source_file = FILE ;
			p_block->alloc_source_line = LINE ;
			
			LinkQmallocBlockClassToTreeByBlockSizeAllowduplicated( & g_used_qmalloc_block_class_tree , p_block );
			LinkQmallocBlockToTreeByBlockAddr( & g_used_qmalloc_block_class_tree , p_block );
			g_used_qmalloc_block_class_tree.blocks_count++;
			g_used_qmalloc_block_class_tree.blocks_total_size += sizeof(struct QmallocBlock)+p_block->data_size ;
			
#if _DEBUG
printf( "_qmalloc : return[%p]\n" , p_block->data_addr );
#endif
			return p_block->data_addr;
		}
		
		p_block = (struct QmallocBlock *)malloc( block.data_size ) ;
		if( p_block == NULL )
		{
#if _DEBUG
printf( "_qmalloc : call malloc failed , errno[%d]\n" , errno );
printf( "_qmalloc : return[%p]\n" , NULL );
#endif
			return NULL;
		}
		
#if _DEBUG
printf( "_qmalloc : call malloc ok , addr[%p] size[%zu][%zu]bytes data[%p]\n" , p_block , sizeof(struct QmallocBlock) , size , p_block->data_addr );
#endif
		p_block->block_addr = p_block ;
		p_block->data_size = size ;
		p_block->alloc_page_flag = 1 ;
		p_block->alloc_source_file = FILE ;
		p_block->alloc_source_line = LINE ;
		
		LinkQmallocBlockClassToTreeByBlockSizeAllowduplicated( & g_used_qmalloc_block_class_tree , p_block );
		LinkQmallocBlockToTreeByBlockAddr( & g_used_qmalloc_block_class_tree , p_block );
		g_used_qmalloc_block_class_tree.blocks_count++;
		g_used_qmalloc_block_class_tree.blocks_total_size += sizeof(struct QmallocBlock)+p_block->data_size ;
		
#if _DEBUG
printf( "_qmalloc : return[%p]\n" , p_block->data_addr );
#endif
		return p_block->data_addr;
	}
	
	block_class.data_size = sizeof(struct QmallocBlock) + size ;
	p_block_class = QueryQmallocBlockClassTreeByDataSizeNolessthan( & g_unused_root_qmalloc_blockblock_class , & block_class ) ;
#if _DEBUG
if( p_block == NULL )
printf( "_qmalloc : QueryQmallocBlockClassTreeByDataSizeNolessthan [%zu][%zu]bytes return NULL\n" , sizeof(struct QmallocBlock) , size );
else
printf( "_qmalloc : QueryQmallocBlockClassTreeByDataSizeNolessthan [%zu][%zu]bytes ok , addr[%p] [%zu]bytes\n" , sizeof(struct QmallocBlock) , size , p_block_class , p_block_class->data_size );
#endif
	if( p_block_class && p_block_class->data_size == size )
	{
		UnlinkQmallocBlockClassFromTreeByDataSize( & g_unused_qmalloc_block_class_tree , p_block );
		UnlinkQmallocBlockFromTreeByAddr( & g_unused_qmalloc_block_class_tree , p_block );
		p_block->alloc_source_file = FILE ;
		p_block->alloc_source_line = LINE ;
		LinkQmallocBlockClassToTreeByBlockSizeAllowduplicated( & g_used_qmalloc_block_class_tree , p_block );
		LinkQmallocBlockToTreeByBlockAddr( & g_used_qmalloc_block_class_tree , p_block );
#if _DEBUG
printf( "_qmalloc : return[%p]\n" , p_block->data_addr );
#endif
		return p_block->data_addr;
	}
	
	if( p_block == NULL || p_block->data_size > NORMAL_BLOCK_MAX_SIZE )
	{
#if _DEBUG
if( p_block == NULL )
printf( "_qmalloc : no unused block\n" );
else
printf( "_qmalloc : no unused block less than\n" );
#endif
		p_block = (struct QmallocBlock *)malloc( BLOCKS_PAGE_SIZE ) ;
		if( p_block == NULL )
		{
#if _DEBUG
printf( "_qmalloc : call malloc failed , errno[%d]\n" , errno );
printf( "_qmalloc : return[%p]\n" , NULL );
#endif
			return NULL;
		}
		
#if _DEBUG
printf( "_qmalloc : call malloc ok , addr[%p] size[%d]bytes data_size[%zu] data_addr[%p]\n" , p_block , BLOCKS_PAGE_SIZE , BLOCKS_PAGE_SIZE-sizeof(struct QmallocBlock) , p_block->data_addr );
#endif
		if( BLOCKS_PAGE_SIZE - (sizeof(struct QmallocBlock)+size) < sizeof(struct QmallocBlock) )
		{
#if _DEBUG
printf( "_qmalloc : entire BLOCKS_PAGE_SIZE[%d]bytes to used\n" , BLOCKS_PAGE_SIZE );
#endif
			p_block->block_addr = p_block ;
			p_block->data_size = NORMAL_BLOCK_MAX_SIZE ;
			p_block->alloc_page_flag = 1 ;
			p_block->alloc_source_file = FILE ;
			p_block->alloc_source_line = LINE ;
			
			LinkQmallocBlockClassToTreeByBlockSizeAllowduplicated( & g_used_qmalloc_block_class_tree , p_block );
			LinkQmallocBlockToTreeByBlockAddr( & g_used_qmalloc_block_class_tree , p_block );
			g_used_qmalloc_block_class_tree.blocks_count++;
			g_used_qmalloc_block_class_tree.blocks_total_size += sizeof(struct QmallocBlock)+p_block->data_size ;
			
#if _DEBUG
printf( "_qmalloc : return[%p]\n" , p_block->data_addr );
#endif
			return p_block->data_addr;
		}
		else
		{
			struct QmallocBlock	*p_next_block ;
			
#if _DEBUG
printf( "_qmalloc : BLOCKS_PAGE_SIZE[%d]bytes divide [%zu][%zu]bytes to used and [%zu][%zu]bytes to unused\n" , BLOCKS_PAGE_SIZE , sizeof(struct QmallocBlock) , size , sizeof(struct QmallocBlock) , BLOCKS_PAGE_SIZE-( sizeof(struct QmallocBlock)+size+sizeof(struct QmallocBlock) ) );
#endif
			p_block->block_addr = p_block ;
			p_block->data_size = size ;
			p_block->alloc_page_flag = 1 ;
			p_block->alloc_source_file = FILE ;
			p_block->alloc_source_line = LINE ;
			
			LinkQmallocBlockClassToTreeByBlockSizeAllowduplicated( & g_used_qmalloc_block_class_tree , p_block );
			LinkQmallocBlockToTreeByBlockAddr( & g_used_qmalloc_block_class_tree , p_block );
			g_used_qmalloc_block_class_tree.blocks_count++;
			g_used_qmalloc_block_class_tree.blocks_total_size += sizeof(struct QmallocBlock)+p_block->data_size ;
			
			p_next_block = (struct QmallocBlock *)(p_block->data_addr+size) ;
			p_next_block->block_addr = p_next_block ;
			p_next_block->data_size = (BLOCKS_PAGE_SIZE-sizeof(struct QmallocBlock)) - (sizeof(struct QmallocBlock)+p_block->data_size) ;
			p_block->alloc_page_flag = 0 ;
			p_next_block->alloc_source_file = NULL ;
			p_next_block->alloc_source_line = 0 ;
			
			LinkQmallocBlockClassToTreeByBlockSizeAllowduplicated( & g_unused_qmalloc_block_class_tree , p_next_block );
			LinkQmallocBlockToTreeByBlockAddr( & g_unused_qmalloc_block_class_tree , p_next_block );
			g_unused_qmalloc_block_class_tree.blocks_count++;
			g_unused_qmalloc_block_class_tree.blocks_total_size += sizeof(struct QmallocBlock)+p_next_block->data_size ;
			
#if _DEBUG
printf( "_qmalloc : return[%p]\n" , p_block->data_addr );
#endif
			return p_block->data_addr;
		}
	}
	else
	{
		if( p_block->data_size - (sizeof(struct QmallocBlock)+size) < sizeof(struct QmallocBlock) )
		{
#if _DEBUG
printf( "_qmalloc : entire block size[%zu]bytes to used\n" , p_block->data_size );
#endif
			UnlinkQmallocBlockClassFromTreeByDataSize( & g_unused_qmalloc_block_class_tree , p_block );
			g_unused_qmalloc_block_class_tree.blocks_count--;
			g_unused_qmalloc_block_class_tree.blocks_total_size -= p_block->data_size ;
			
			p_block->alloc_source_file = FILE ;
			p_block->alloc_source_line = LINE ;
			
			LinkQmallocBlockClassToTreeByBlockSizeAllowduplicated( & g_used_qmalloc_block_class_tree , p_block );
			g_used_qmalloc_block_class_tree.blocks_count++;
			g_used_qmalloc_block_class_tree.blocks_total_size += sizeof(struct QmallocBlock)+p_block->data_size ;
			
#if _DEBUG
printf( "_qmalloc : return[%p]\n" , p_block->data_addr );
#endif
			return p_block->data_addr;
		}
		else
		{
			struct QmallocBlock	*p_next_block ;
			size_t			total_block_size ;
			
			total_block_size = p_block->data_size ;
#if _DEBUG
printf( "_qmalloc : block size[%zu]bytes divide [%zu][%zu]bytes to used and [%zu][%zu]bytes to unused\n" , total_block_size , sizeof(struct QmallocBlock) , size , sizeof(struct QmallocBlock) , total_block_size-(sizeof(struct QmallocBlock)+size+sizeof(struct QmallocBlock)) );
#endif
			
			UnlinkQmallocBlockClassFromTreeByDataSize( & g_unused_qmalloc_block_class_tree , p_block );
			UnlinkQmallocBlockFromTreeByAddr( & g_unused_qmalloc_block_class_tree , p_block );
			g_unused_qmalloc_block_class_tree.blocks_count--;
			g_unused_qmalloc_block_class_tree.blocks_total_size -= sizeof(struct QmallocBlock)+p_block->data_size ;
			
			p_block->data_size = size ;
			p_block->alloc_source_file = FILE ;
			p_block->alloc_source_line = LINE ;
			LinkQmallocBlockClassToTreeByBlockSizeAllowduplicated( & g_used_qmalloc_block_class_tree , p_block );
			LinkQmallocBlockToTreeByBlockAddr( & g_used_qmalloc_block_class_tree , p_block );
			g_used_qmalloc_block_class_tree.blocks_count++;
			g_used_qmalloc_block_class_tree.blocks_total_size += sizeof(struct QmallocBlock)+p_block->data_size ;
			
			p_next_block = (struct QmallocBlock *)(p_block->data_addr+size) ;
			p_next_block->block_addr = p_next_block ;
			p_next_block->data_size = total_block_size - (sizeof(struct QmallocBlock)+size) ;
			p_next_block->alloc_page_flag = 0 ;
			p_next_block->alloc_source_file = NULL ;
			p_next_block->alloc_source_line = 0 ;
			LinkQmallocBlockClassToTreeByBlockSizeAllowduplicated( & g_unused_qmalloc_block_class_tree , p_next_block );
			LinkQmallocBlockToTreeByBlockAddr( & g_unused_qmalloc_block_class_tree , p_next_block );
			g_unused_qmalloc_block_class_tree.blocks_count++;
			g_unused_qmalloc_block_class_tree.blocks_total_size += sizeof(struct QmallocBlock)+p_next_block->data_size ;
			
#if _DEBUG
printf( "_qmalloc : return[%p]\n" , p_block->data_addr );
#endif
			return p_block->data_addr;
		}
	}
#endif
}

void _qfree( void *ptr )
{
	struct QmallocBlock	*p_block ;
	struct QmallocBlock	*p_prev_block_order_by_addr ;
	struct QmallocBlock	*p_next_block_order_by_addr ;
	struct rb_node		*p ;
	
#if _DEBUG
printf( "_qfree : ptr[%p]\n" , ptr );
#endif
	if( ptr == NULL )
		return;
	
	p_block = (struct QmallocBlock *)( (char*)ptr - sizeof(struct QmallocBlock) ) ;
	
	UnlinkQmallocBlockClassFromTreeByDataSize( & g_used_qmalloc_block_class_tree , p_block );
	UnlinkQmallocBlockFromTreeByAddr( & g_used_qmalloc_block_class_tree , p_block );
	g_used_qmalloc_block_class_tree.blocks_count--;
	g_used_qmalloc_block_class_tree.blocks_total_size -= sizeof(struct QmallocBlock)+p_block->data_size ;
	
	LinkQmallocBlockClassToTreeByBlockSizeAllowduplicated( & g_unused_qmalloc_block_class_tree , p_block );
	LinkQmallocBlockToTreeByBlockAddr( & g_unused_qmalloc_block_class_tree , p_block );
	g_unused_qmalloc_block_class_tree.blocks_count++;
	g_unused_qmalloc_block_class_tree.blocks_total_size += sizeof(struct QmallocBlock)+p_block->data_size ;
	
	p = rb_prev(&(p_block->block_tree_node_order_by_addr)) ;
	if( p )
	{
		p_prev_block_order_by_addr = container_of( p , struct QmallocBlock , block_tree_node_order_by_addr ) ;
#if _DEBUG
printf( "_qfree : prev_block[%p] prev_block_size[%zu] , block[%p] data_size[%zu]\n" , p_prev_block_order_by_addr->block_addr , p_prev_block_order_by_addr->data_size , p_block->block_addr , p_block->data_size );
#endif
		if( p_prev_block_order_by_addr->data_addr + p_prev_block_order_by_addr->data_size == p_block->block_addr )
		{
			p_prev_block_order_by_addr->data_size += sizeof(struct QmallocBlock)+p_block->data_size ;
			
#if _DEBUG
printf( "_qfree : unlink block[%p] data_size[%zu]\n" , p_block->block_addr , p_block->data_size );
#endif
			UnlinkQmallocBlockClassFromTreeByDataSize( & g_unused_qmalloc_block_class_tree , p_block );
			UnlinkQmallocBlockFromTreeByAddr( & g_unused_qmalloc_block_class_tree , p_block );
			g_unused_qmalloc_block_class_tree.blocks_count--;
			
			p_block = p_prev_block_order_by_addr ;
		}
	}
	
	p = rb_next(&(p_block->block_tree_node_order_by_addr)) ;
	if( p )
	{
		p_next_block_order_by_addr = container_of( p , struct QmallocBlock , block_tree_node_order_by_addr ) ;
#if _DEBUG
printf( "_qfree : block[%p] data_size[%zu] , next_block[%p] next_block_size[%zu]\n" , p_block->block_addr , p_block->data_size , p_next_block_order_by_addr->block_addr , p_next_block_order_by_addr->data_size );
#endif
		if( p_block->data_addr + p_block->data_size == p_next_block_order_by_addr->block_addr )
		{
			p_block->data_size += sizeof(struct QmallocBlock)+p_next_block_order_by_addr->data_size ;
			
#if _DEBUG
printf( "_qfree : unlink next_block[%p] next_block_size[%zu]\n" , p_next_block_order_by_addr->block_addr , p_next_block_order_by_addr->data_size );
#endif
			UnlinkQmallocBlockClassFromTreeByDataSize( & g_unused_qmalloc_block_class_tree , p_next_block_order_by_addr );
			UnlinkQmallocBlockFromTreeByAddr( & g_unused_qmalloc_block_class_tree , p_next_block_order_by_addr );
			g_unused_qmalloc_block_class_tree.blocks_count--;
		}
	}
	
	if( g_unused_qmalloc_block_class_tree.blocks_total_size > g_cache_blocks_max_size && p_block->alloc_page_flag == 1 )
	{
#if _DEBUG
printf( "_qfree : call free[%p]\n" , p_block );
#endif
		UnlinkQmallocBlockClassFromTreeByDataSize( & g_unused_qmalloc_block_class_tree , p_block );
		UnlinkQmallocBlockFromTreeByAddr( & g_unused_qmalloc_block_class_tree , p_block );
		g_unused_qmalloc_block_class_tree.blocks_count--;
		g_unused_qmalloc_block_class_tree.blocks_total_size -= sizeof(struct QmallocBlock)+p_block->data_size ;
		free( p_block );
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
	return g_used_qmalloc_block_class_tree.blocks_count;
}

size_t qstat_used_blocks_total_size()
{
	return g_used_qmalloc_block_class_tree.blocks_total_size;
}

size_t qstat_unused_blocks_count()
{
	return g_unused_qmalloc_block_class_tree.blocks_count;
}

size_t qstat_unused_blocks_total_size()
{
	return g_unused_qmalloc_block_class_tree.blocks_total_size;
}

void *qtravel_used_by_size( void *ptr )
{
	struct QmallocBlock	*p_block ;
	struct rb_node		*p ;
	
	if( ptr == NULL )
	{
		p = rb_first(&(g_used_qmalloc_block_class_tree.block_class_tree_order_by_size)) ;
		if( p == NULL )
			return NULL;
		p_block = container_of( p , struct QmallocBlock , block_class_tree_node_order_by_size ) ;
		return p_block->data_addr;
	}
	else
	{
		p_block = container_of( ptr , struct QmallocBlock , data_addr ) ;
		p = rb_next(&(p_block->block_class_tree_node_order_by_size)) ;
		if( p == NULL )
			return NULL;
		p_block = container_of( p , struct QmallocBlock , block_class_tree_node_order_by_size ) ;
		return p_block->data_addr;
	}
}

void *qtravel_unused_by_size( void *ptr )
{
	struct QmallocBlock	*p_block ;
	struct rb_node		*p ;
	
	if( ptr == NULL )
	{
		p = rb_first(&(g_unused_qmalloc_block_class_tree.block_class_tree_order_by_size)) ;
		if( p == NULL )
			return NULL;
		p_block = container_of( p , struct QmallocBlock , block_class_tree_node_order_by_size ) ;
		return p_block->data_addr;
	}
	else
	{
		p_block = container_of( ptr , struct QmallocBlock , data_addr ) ;
		p = rb_next(&(p_block->block_class_tree_node_order_by_size)) ;
		if( p == NULL )
			return NULL;
		p_block = container_of( p , struct QmallocBlock , block_class_tree_node_order_by_size ) ;
		return p_block->data_addr;
	}
}

void *qtravel_used_by_addr( void *ptr )
{
	struct QmallocBlock	*p_block ;
	struct rb_node		*p ;
	
	if( ptr == NULL )
	{
		p = rb_first(&(g_used_qmalloc_block_class_tree.block_tree_order_by_addr)) ;
		if( p == NULL )
			return NULL;
		p_block = container_of( p , struct QmallocBlock , block_tree_node_order_by_addr ) ;
		return p_block->data_addr;
	}
	else
	{
		p_block = container_of( ptr , struct QmallocBlock , data_addr ) ;
		p = rb_next(&(p_block->block_tree_node_order_by_addr)) ;
		if( p == NULL )
			return NULL;
		p_block = container_of( p , struct QmallocBlock , block_tree_node_order_by_addr ) ;
		return p_block->data_addr;
	}
}

void *qtravel_unused_by_addr( void *ptr )
{
	struct QmallocBlock	*p_block ;
	struct rb_node		*p ;
	
	if( ptr == NULL )
	{
		p = rb_first(&(g_unused_qmalloc_block_class_tree.block_tree_order_by_addr)) ;
		if( p == NULL )
			return NULL;
		p_block = container_of( p , struct QmallocBlock , block_tree_node_order_by_addr ) ;
		return p_block->data_addr;
	}
	else
	{
		p_block = container_of( ptr , struct QmallocBlock , data_addr ) ;
		p = rb_next(&(p_block->block_tree_node_order_by_addr)) ;
		if( p == NULL )
			return NULL;
		p_block = container_of( p , struct QmallocBlock , block_tree_node_order_by_addr ) ;
		return p_block->data_addr;
	}
}

size_t qget_size( void *ptr )
{
	struct QmallocBlock	*p_block = container_of( ptr , struct QmallocBlock , data_addr ) ;
	return p_block->data_size;
}

char *qget_alloc_source_file( void *ptr )
{
	struct QmallocBlock	*p_block = container_of( ptr , struct QmallocBlock , data_addr ) ;
	return p_block->alloc_source_file;
}

int qget_alloc_source_line( void *ptr )
{
	struct QmallocBlock	*p_block = container_of( ptr , struct QmallocBlock , data_addr ) ;
	return p_block->alloc_source_line;
}

void qset_cache_blocks_max_size( size_t max_size )
{
	g_cache_blocks_max_size = max_size ;
	return;
}

void qfree_all_unused()
{
	struct rb_node		*p = NULL ;
	struct rb_node		*p_next = NULL ;
	struct QmallocBlock	*p_block = NULL ;
	
	p = rb_first( & (g_unused_qmalloc_block_class_tree.block_tree_order_by_addr) ) ;
	while( p )
	{
		p_next = rb_next( p ) ;
		
		p_block = container_of( p , struct QmallocBlock , block_tree_node_order_by_addr ) ;
		UnlinkQmallocBlockClassFromTreeByDataSize( & g_unused_qmalloc_block_class_tree , p_block );
		UnlinkQmallocBlockFromTreeByAddr( & g_unused_qmalloc_block_class_tree , p_block );
		g_unused_qmalloc_block_class_tree.blocks_count--;
		g_unused_qmalloc_block_class_tree.blocks_total_size -= sizeof(struct QmallocBlock)+p_block->data_size ;
#if _DEBUG
printf( "qfree_all_unused : call free[%p]\n" , p_block );
#endif
		free( p_block );
		
		p = p_next ;
	}
	
	return;
}

size_t qget_block_header_size()
{
	return sizeof(struct QmallocBlock);
}

size_t qget_normal_block_max_size()
{
	return NORMAL_BLOCK_MAX_SIZE;
}

size_t qget_blocks_page_size()
{
	return BLOCKS_PAGE_SIZE;
}

size_t qget_cache_blocks_max_size()
{
	return g_cache_blocks_max_size;
}

