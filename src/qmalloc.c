#include "qmalloc.h"
#include <stdint.h>
#include <inttypes.h>

#include "rbtree_tpl.h"

#define _DEBUG			0

#define BLOCKS_PAGE_SIZE	4*1024*1024
#define NORMAL_BLOCK_MAX_SIZE	(BLOCKS_PAGE_SIZE-sizeof(struct QmallocBlock))

struct QmallocBlock
{
	unsigned char	dmz ; /* 防止上一内存块写数据越界破坏当前块头信息 */
	
	void		*block_addr ;
	struct rb_node	block_tree_node_order_by_addr ;
	
	size_t		data_size ;
	struct rb_node	block_tree_node_order_by_size_allowduplicate ;
	
	unsigned char	alloc_page_flag ;
	char		*alloc_source_file ;
	size_t		alloc_source_line ;
	char		*free_source_file ;
	size_t		free_source_line ;
	
	char		data_addr[0] ;
} ;

struct QmallocRootBlock
{
	struct rb_root	block_tree_order_by_addr ;
	struct rb_root	block_tree_order_by_size_allowduplicate ;
	size_t		blocks_count ;
	size_t		blocks_total_size ;
} ;

__thread struct QmallocRootBlock	g_used_root_qmalloc_block = { RB_ROOT,RB_ROOT,0,0 } ;
__thread struct QmallocRootBlock	g_unused_root_qmalloc_block = { RB_ROOT,RB_ROOT,0,0 } ;

__thread size_t				g_cache_blocks_max_size = 100*1024*1024 ;

LINK_RBTREENODE_INT_ALLOWDUPLICATE( LinkQmallocBlockToTreeByBlockSizeAllowduplicated , struct QmallocRootBlock , block_tree_order_by_size_allowduplicate , struct QmallocBlock , block_tree_node_order_by_size_allowduplicate , data_size )
UNLINK_RBTREENODE( UnlinkQmallocBlockFromTreeByBlockSize , struct QmallocRootBlock , block_tree_order_by_size_allowduplicate , struct QmallocBlock , block_tree_node_order_by_size_allowduplicate )
QUERY_RBTREENODE_INT( QueryQmallocBlockTreeByBlockSize , struct QmallocRootBlock , block_tree_order_by_size_allowduplicate , struct QmallocBlock , block_tree_node_order_by_size_allowduplicate , data_size )
QUERY_RBTREENODE_INT_NOLESSTHAN( QueryQmallocBlockTreeByBlockSizeNolessthan , struct QmallocRootBlock , block_tree_order_by_size_allowduplicate , struct QmallocBlock , block_tree_node_order_by_size_allowduplicate , data_size )
TRAVEL_RBTREENODE( TravelQmallocBlockTreeByBlockSize , struct QmallocRootBlock , block_tree_order_by_size_allowduplicate , struct QmallocBlock , block_tree_node_order_by_size_allowduplicate )

int LinkQmallocBlockToTreeByBlockSizeAllowduplicated( struct QmallocRootBlock *block_tree , struct QmallocBlock *p_block );
void UnlinkQmallocBlockFromTreeByBlockSize( struct QmallocRootBlock *block_tree , struct QmallocBlock *p_block );
struct QmallocBlock *QueryQmallocBlockTreeByBlockSize( struct QmallocRootBlock *block_tree , struct QmallocBlock *block );
struct QmallocBlock *QueryQmallocBlockTreeByBlockSizeNolessthan( struct QmallocRootBlock *block_tree , struct QmallocBlock *block );
struct QmallocBlock *TravelQmallocBlockTreeByBlockSize( struct QmallocRootBlock *block_tree , struct QmallocBlock *p_block );

LINK_RBTREENODE_POINTER( LinkQmallocBlockToTreeByBlockAddr , struct QmallocRootBlock , block_tree_order_by_addr , struct QmallocBlock , block_tree_node_order_by_addr , block_addr )
UNLINK_RBTREENODE( UnlinkQmallocBlockFromTreeByBlockAddr , struct QmallocRootBlock , block_tree_order_by_addr , struct QmallocBlock , block_tree_node_order_by_addr )
QUERY_RBTREENODE_POINTER( QueryQmallocBlockTreeByBlockAddr , struct QmallocRootBlock , block_tree_order_by_addr , struct QmallocBlock , block_tree_node_order_by_addr , block_addr )
TRAVEL_RBTREENODE( TravelQmallocBlockTreeByBlockAddr , struct QmallocRootBlock , block_tree_order_by_addr , struct QmallocBlock , block_tree_node_order_by_addr )

int LinkQmallocBlockToTreeByBlockAddr( struct QmallocRootBlock *block_tree , struct QmallocBlock *p_block );
void UnlinkQmallocBlockFromTreeByBlockAddr( struct QmallocRootBlock *block_tree , struct QmallocBlock *p_block );
struct QmallocBlock *QueryQmallocBlockTreeByBlockAddr( struct QmallocRootBlock *block_tree , struct QmallocBlock *block );
struct QmallocBlock *TravelQmallocBlockTreeByBlockAddr( struct QmallocRootBlock *block_tree , struct QmallocBlock *p_block );

void *_qmalloc( size_t size , char *FILE , int LINE )
{
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
	
	size = MEMADDR_ALIGN( size ) ; /* 内存地址对齐，与rb_node.rb_parent_color配合 */
	
	if( size <= NORMAL_BLOCK_MAX_SIZE )
	{
		struct QmallocBlock	block ;
		struct QmallocBlock	*p_block ;
		
		block.data_size = sizeof(struct QmallocBlock) + size ;
		p_block = QueryQmallocBlockTreeByBlockSizeNolessthan( & g_unused_root_qmalloc_block , & block ) ;
#if _DEBUG
if( p_block == NULL )
	printf( "_qmalloc : QueryQmallocBlockTreeByBlockSizeNolessthan [%zu][%zu]bytes return NULL\n" , sizeof(struct QmallocBlock) , size );
else
	printf( "_qmalloc : QueryQmallocBlockTreeByBlockSizeNolessthan [%zu][%zu]bytes ok , addr[%p] [%zu]bytes\n" , sizeof(struct QmallocBlock) , size , p_block , p_block->data_size );
#endif
		if( p_block && p_block->data_size == size )
		{
			UnlinkQmallocBlockFromTreeByBlockSize( & g_unused_root_qmalloc_block , p_block );
			UnlinkQmallocBlockFromTreeByBlockAddr( & g_unused_root_qmalloc_block , p_block );
			p_block->alloc_source_file = FILE ;
			p_block->alloc_source_line = LINE ;
			p_block->free_source_file = NULL ;
			p_block->free_source_line = 0 ;
			LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_used_root_qmalloc_block , p_block );
			LinkQmallocBlockToTreeByBlockAddr( & g_used_root_qmalloc_block , p_block );
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
				p_block->free_source_file = NULL ;
				p_block->free_source_line = 0 ;
				
				LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_used_root_qmalloc_block , p_block );
				LinkQmallocBlockToTreeByBlockAddr( & g_used_root_qmalloc_block , p_block );
				g_used_root_qmalloc_block.blocks_count++;
				g_used_root_qmalloc_block.blocks_total_size += sizeof(struct QmallocBlock)+p_block->data_size ;
				
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
				p_block->free_source_file = NULL ;
				p_block->free_source_line = 0 ;
				LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_used_root_qmalloc_block , p_block );
				LinkQmallocBlockToTreeByBlockAddr( & g_used_root_qmalloc_block , p_block );
				g_used_root_qmalloc_block.blocks_count++;
				g_used_root_qmalloc_block.blocks_total_size += sizeof(struct QmallocBlock)+p_block->data_size ;
				
				p_next_block = (struct QmallocBlock *)(p_block->data_addr+size) ;
				p_next_block->block_addr = p_next_block ;
				p_next_block->data_size = (BLOCKS_PAGE_SIZE-sizeof(struct QmallocBlock)) - (sizeof(struct QmallocBlock)+p_block->data_size) ;
				p_block->alloc_page_flag = 0 ;
				p_next_block->alloc_source_file = NULL ;
				p_next_block->alloc_source_line = 0 ;
				p_next_block->free_source_file = NULL ;
				p_next_block->free_source_line = 0 ;
				LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_unused_root_qmalloc_block , p_next_block );
				LinkQmallocBlockToTreeByBlockAddr( & g_unused_root_qmalloc_block , p_next_block );
				g_unused_root_qmalloc_block.blocks_count++;
				g_unused_root_qmalloc_block.blocks_total_size += sizeof(struct QmallocBlock)+p_next_block->data_size ;
				
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
				UnlinkQmallocBlockFromTreeByBlockSize( & g_unused_root_qmalloc_block , p_block );
				g_unused_root_qmalloc_block.blocks_count--;
				g_unused_root_qmalloc_block.blocks_total_size -= p_block->data_size ;
				
				p_block->alloc_source_file = FILE ;
				p_block->alloc_source_line = LINE ;
				p_block->free_source_file = NULL ;
				p_block->free_source_line = 0 ;
				
				LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_used_root_qmalloc_block , p_block );
				g_used_root_qmalloc_block.blocks_count++;
				g_used_root_qmalloc_block.blocks_total_size += sizeof(struct QmallocBlock)+p_block->data_size ;
				
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
				
				UnlinkQmallocBlockFromTreeByBlockSize( & g_unused_root_qmalloc_block , p_block );
				UnlinkQmallocBlockFromTreeByBlockAddr( & g_unused_root_qmalloc_block , p_block );
				g_unused_root_qmalloc_block.blocks_count--;
				g_unused_root_qmalloc_block.blocks_total_size -= sizeof(struct QmallocBlock)+p_block->data_size ;
				
				p_block->data_size = size ;
				p_block->alloc_source_file = FILE ;
				p_block->alloc_source_line = LINE ;
				p_block->free_source_file = NULL ;
				p_block->free_source_line = 0 ;
				LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_used_root_qmalloc_block , p_block );
				LinkQmallocBlockToTreeByBlockAddr( & g_used_root_qmalloc_block , p_block );
				g_used_root_qmalloc_block.blocks_count++;
				g_used_root_qmalloc_block.blocks_total_size += sizeof(struct QmallocBlock)+p_block->data_size ;
				
				p_next_block = (struct QmallocBlock *)(p_block->data_addr+size) ;
				p_next_block->block_addr = p_next_block ;
				p_next_block->data_size = total_block_size - (sizeof(struct QmallocBlock)+size) ;
				p_next_block->alloc_page_flag = 0 ;
				p_next_block->alloc_source_file = NULL ;
				p_next_block->alloc_source_line = 0 ;
				p_next_block->free_source_file = NULL ;
				p_next_block->free_source_line = 0 ;
				LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_unused_root_qmalloc_block , p_next_block );
				LinkQmallocBlockToTreeByBlockAddr( & g_unused_root_qmalloc_block , p_next_block );
				g_unused_root_qmalloc_block.blocks_count++;
				g_unused_root_qmalloc_block.blocks_total_size += sizeof(struct QmallocBlock)+p_next_block->data_size ;
				
#if _DEBUG
printf( "_qmalloc : return[%p]\n" , p_block->data_addr );
#endif
				return p_block->data_addr;
			}
		}
	}
	else
	{
		struct QmallocBlock	block ;
		struct QmallocBlock	*p_block ;
		
		block.data_size = sizeof(struct QmallocBlock) + size ;
		p_block = QueryQmallocBlockTreeByBlockSize( & g_unused_root_qmalloc_block , & block ) ;
		if( p_block )
		{
			UnlinkQmallocBlockFromTreeByBlockSize( & g_unused_root_qmalloc_block , p_block );
			UnlinkQmallocBlockFromTreeByBlockAddr( & g_unused_root_qmalloc_block , p_block );
			g_unused_root_qmalloc_block.blocks_count--;
			g_unused_root_qmalloc_block.blocks_total_size -= sizeof(struct QmallocBlock)+p_block->data_size ;
			
			p_block->alloc_source_file = FILE ;
			p_block->alloc_source_line = LINE ;
			p_block->free_source_file = NULL ;
			p_block->free_source_line = 0 ;
			
			LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_used_root_qmalloc_block , p_block );
			LinkQmallocBlockToTreeByBlockAddr( & g_used_root_qmalloc_block , p_block );
			g_used_root_qmalloc_block.blocks_count++;
			g_used_root_qmalloc_block.blocks_total_size += sizeof(struct QmallocBlock)+p_block->data_size ;
			
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
		p_block->free_source_file = NULL ;
		p_block->free_source_line = 0 ;
		
		LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_used_root_qmalloc_block , p_block );
		LinkQmallocBlockToTreeByBlockAddr( & g_used_root_qmalloc_block , p_block );
		g_used_root_qmalloc_block.blocks_count++;
		g_used_root_qmalloc_block.blocks_total_size += sizeof(struct QmallocBlock)+p_block->data_size ;
		
#if _DEBUG
printf( "_qmalloc : return[%p]\n" , p_block->data_addr );
#endif
		return p_block->data_addr;
	}
}

void _qfree( void *ptr , char *FILE , int LINE )
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
	
	UnlinkQmallocBlockFromTreeByBlockSize( & g_used_root_qmalloc_block , p_block );
	UnlinkQmallocBlockFromTreeByBlockAddr( & g_used_root_qmalloc_block , p_block );
	g_used_root_qmalloc_block.blocks_count--;
	g_used_root_qmalloc_block.blocks_total_size -= sizeof(struct QmallocBlock)+p_block->data_size ;
	
	p_block->free_source_file = FILE ;
	p_block->free_source_line = LINE ;
	
	LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_unused_root_qmalloc_block , p_block );
	LinkQmallocBlockToTreeByBlockAddr( & g_unused_root_qmalloc_block , p_block );
	g_unused_root_qmalloc_block.blocks_count++;
	g_unused_root_qmalloc_block.blocks_total_size += sizeof(struct QmallocBlock)+p_block->data_size ;
	
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
			UnlinkQmallocBlockFromTreeByBlockSize( & g_unused_root_qmalloc_block , p_block );
			UnlinkQmallocBlockFromTreeByBlockAddr( & g_unused_root_qmalloc_block , p_block );
			g_unused_root_qmalloc_block.blocks_count--;
			
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
			UnlinkQmallocBlockFromTreeByBlockSize( & g_unused_root_qmalloc_block , p_next_block_order_by_addr );
			UnlinkQmallocBlockFromTreeByBlockAddr( & g_unused_root_qmalloc_block , p_next_block_order_by_addr );
			g_unused_root_qmalloc_block.blocks_count--;
		}
	}
	
	if( g_unused_root_qmalloc_block.blocks_total_size > g_cache_blocks_max_size && p_block->alloc_page_flag == 1 )
	{
#if _DEBUG
printf( "_qfree : call free[%p]\n" , p_block );
#endif
		UnlinkQmallocBlockFromTreeByBlockSize( & g_unused_root_qmalloc_block , p_block );
		UnlinkQmallocBlockFromTreeByBlockAddr( & g_unused_root_qmalloc_block , p_block );
		g_unused_root_qmalloc_block.blocks_count--;
		g_unused_root_qmalloc_block.blocks_total_size -= sizeof(struct QmallocBlock)+p_block->data_size ;
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
	
	_qfree( ptr , FILE , LINE );
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

size_t _qstat_used_blocks_count()
{
	return g_used_root_qmalloc_block.blocks_count;
}

size_t _qstat_used_blocks_total_size()
{
	return g_used_root_qmalloc_block.blocks_total_size;
}

size_t _qstat_unused_blocks_count()
{
	return g_unused_root_qmalloc_block.blocks_count;
}

size_t _qstat_unused_blocks_total_size()
{
	return g_unused_root_qmalloc_block.blocks_total_size;
}

void *_qtravel_used_by_size( void *ptr )
{
	struct QmallocBlock	*p_block ;
	struct rb_node		*p ;
	
	if( ptr == NULL )
	{
		p = rb_first(&(g_used_root_qmalloc_block.block_tree_order_by_size_allowduplicate)) ;
		if( p == NULL )
			return NULL;
		p_block = container_of( p , struct QmallocBlock , block_tree_node_order_by_size_allowduplicate ) ;
		return p_block->data_addr;
	}
	else
	{
		p_block = container_of( ptr , struct QmallocBlock , data_addr ) ;
		p = rb_next(&(p_block->block_tree_node_order_by_size_allowduplicate)) ;
		if( p == NULL )
			return NULL;
		p_block = container_of( p , struct QmallocBlock , block_tree_node_order_by_size_allowduplicate ) ;
		return p_block->data_addr;
	}
}

void *_qtravel_unused_by_size( void *ptr )
{
	struct QmallocBlock	*p_block ;
	struct rb_node		*p ;
	
	if( ptr == NULL )
	{
		p = rb_first(&(g_unused_root_qmalloc_block.block_tree_order_by_size_allowduplicate)) ;
		if( p == NULL )
			return NULL;
		p_block = container_of( p , struct QmallocBlock , block_tree_node_order_by_size_allowduplicate ) ;
		return p_block->data_addr;
	}
	else
	{
		p_block = container_of( ptr , struct QmallocBlock , data_addr ) ;
		p = rb_next(&(p_block->block_tree_node_order_by_size_allowduplicate)) ;
		if( p == NULL )
			return NULL;
		p_block = container_of( p , struct QmallocBlock , block_tree_node_order_by_size_allowduplicate ) ;
		return p_block->data_addr;
	}
}

void *_qtravel_used_by_addr( void *ptr )
{
	struct QmallocBlock	*p_block ;
	struct rb_node		*p ;
	
	if( ptr == NULL )
	{
		p = rb_first(&(g_used_root_qmalloc_block.block_tree_order_by_addr)) ;
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

void *_qtravel_unused_by_addr( void *ptr )
{
	struct QmallocBlock	*p_block ;
	struct rb_node		*p ;
	
	if( ptr == NULL )
	{
		p = rb_first(&(g_unused_root_qmalloc_block.block_tree_order_by_addr)) ;
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

size_t _qget_size( void *ptr )
{
	struct QmallocBlock	*p_block = container_of( ptr , struct QmallocBlock , data_addr ) ;
	return p_block->data_size;
}

char *_qget_alloc_source_file( void *ptr )
{
	struct QmallocBlock	*p_block = container_of( ptr , struct QmallocBlock , data_addr ) ;
	return p_block->alloc_source_file;
}

int _qget_alloc_source_line( void *ptr )
{
	struct QmallocBlock	*p_block = container_of( ptr , struct QmallocBlock , data_addr ) ;
	return p_block->alloc_source_line;
}

char *_qget_free_source_file( void *ptr )
{
	struct QmallocBlock	*p_block = container_of( ptr , struct QmallocBlock , data_addr ) ;
	return p_block->free_source_file;
}

int _qget_free_source_line( void *ptr )
{
	struct QmallocBlock	*p_block = container_of( ptr , struct QmallocBlock , data_addr ) ;
	return p_block->free_source_line;
}

void _qset_cache_blocks_max_size( size_t max_size )
{
	g_cache_blocks_max_size = max_size ;
	return;
}

void _qfree_all_unused()
{
	struct rb_node		*p = NULL ;
	struct rb_node		*p_next = NULL ;
	struct QmallocBlock	*p_block = NULL ;
	
	p = rb_first( & (g_unused_root_qmalloc_block.block_tree_order_by_addr) ) ;
	while( p )
	{
		p_next = rb_next( p ) ;
		
		p_block = container_of( p , struct QmallocBlock , block_tree_node_order_by_addr ) ;
		UnlinkQmallocBlockFromTreeByBlockSize( & g_unused_root_qmalloc_block , p_block );
		UnlinkQmallocBlockFromTreeByBlockAddr( & g_unused_root_qmalloc_block , p_block );
		g_unused_root_qmalloc_block.blocks_count--;
		g_unused_root_qmalloc_block.blocks_total_size -= sizeof(struct QmallocBlock)+p_block->data_size ;
#if _DEBUG
printf( "_qfree_all_unused : call free[%p]\n" , p_block );
#endif
		free( p_block );
		
		p = p_next ;
	}
	
	return;
}

size_t _qget_block_header_size()
{
	return sizeof(struct QmallocBlock);
}

size_t _qget_normal_block_max_size()
{
	return NORMAL_BLOCK_MAX_SIZE;
}

size_t _qget_blocks_page_size()
{
	return BLOCKS_PAGE_SIZE;
}

size_t _qget_cache_blocks_max_size()
{
	return g_cache_blocks_max_size;
}

