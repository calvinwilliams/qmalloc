#include "qmalloc.h"
#include <stdint.h>
#include <inttypes.h>

#include "rbtree_tpl.h"

#define _DEBUG			1

#define BLOCKS_PAGE_SIZE	64*1024
#define NORMAL_BLOCK_MAX_SIZE	(BLOCKS_PAGE_SIZE-sizeof(struct QmallocBlock))

struct QmallocBlock
{
	void		*block_addr ;
	struct rb_node	block_tree_node_order_by_addr ;
	
	size_t		block_size ;
	struct rb_node	block_tree_node_order_by_size_allowduplicate ;
	
	char		*alloc_source_file ;
	size_t		alloc_source_line ;
	char		*free_source_file ;
	size_t		free_source_line ;
	
	char		block_base[0] ;
} ;

struct QmallocRootBlock
{
	struct rb_root	block_tree_order_by_addr ;
	struct rb_root	block_tree_order_by_size_allowduplicate ;
	size_t		blocks_count ;
	size_t		blocks_total_size ;
} ;

__thread struct QmallocRootBlock	g_used_root_qmalloc_block = { {NULL} } ;
__thread struct QmallocRootBlock	g_unused_root_qmalloc_block = { {NULL} } ;

__thread size_t				g_cache_blocks_max_size = 100*1024*1024 ;

LINK_RBTREENODE_INT_ALLOWDUPLICATE( LinkQmallocBlockToTreeByBlockSizeAllowduplicated , struct QmallocRootBlock , block_tree_order_by_size_allowduplicate , struct QmallocBlock , block_tree_node_order_by_size_allowduplicate , block_size )
UNLINK_RBTREENODE( UnlinkQmallocBlockFromTreeByBlockSize , struct QmallocRootBlock , block_tree_order_by_size_allowduplicate , struct QmallocBlock , block_tree_node_order_by_size_allowduplicate )
QUERY_RBTREENODE_INT( QueryQmallocBlockTreeByBlockSize , struct QmallocRootBlock , block_tree_order_by_size_allowduplicate , struct QmallocBlock , block_tree_node_order_by_size_allowduplicate , block_size )
QUERY_RBTREENODE_INT_NOLESSTHAN( QueryQmallocBlockTreeByBlockSizeNolessthan , struct QmallocRootBlock , block_tree_order_by_size_allowduplicate , struct QmallocBlock , block_tree_node_order_by_size_allowduplicate , block_size )
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
	if( size == 0 )
		return NULL;
	
	if( size <= NORMAL_BLOCK_MAX_SIZE )
	{
		struct QmallocBlock	block ;
		struct QmallocBlock	*p_block ;
		
		block.block_size = sizeof(struct QmallocBlock) + size ;
		p_block = QueryQmallocBlockTreeByBlockSizeNolessthan( & g_unused_root_qmalloc_block , & block ) ;
		if( p_block && p_block->block_size == size )
		{
			UnlinkQmallocBlockFromTreeByBlockSize( & g_unused_root_qmalloc_block , p_block );
			UnlinkQmallocBlockFromTreeByBlockAddr( & g_unused_root_qmalloc_block , p_block );
			p_block->alloc_source_file = FILE ;
			p_block->alloc_source_line = LINE ;
			p_block->free_source_file = NULL ;
			p_block->free_source_line = 0 ;
			LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_used_root_qmalloc_block , p_block );
			LinkQmallocBlockToTreeByBlockAddr( & g_used_root_qmalloc_block , p_block );
			return p_block->block_base;
		}
		
#if _DEBUG
if( p_block == NULL )
	printf( "DEBUG : QueryQmallocBlockTreeByBlockSizeNolessthan return NULL\n" );
else
	printf( "DEBUG : QueryQmallocBlockTreeByBlockSizeNolessthan ok , addr[%"PRIu64"] [%zu]bytes\n" , (uint64_t)p_block , p_block->block_size );
#endif
		if( p_block == NULL || p_block->block_size > NORMAL_BLOCK_MAX_SIZE )
		{
#if _DEBUG
if( p_block == NULL )
	printf( "DEBUG : no block\n" );
else
	printf( "DEBUG : no block less than\n" );
#endif
			p_block = (struct QmallocBlock *)malloc( BLOCKS_PAGE_SIZE ) ;
			if( p_block == NULL )
				return NULL;
#if _DEBUG
printf( "DEBUG : malloc ok , addr[%"PRIu64"] [%d]bytes\n" , (uint64_t)p_block , BLOCKS_PAGE_SIZE );
#endif
			if( BLOCKS_PAGE_SIZE - (sizeof(struct QmallocBlock)+size) < sizeof(struct QmallocBlock) )
			{
#if _DEBUG
printf( "DEBUG : entire BLOCKS_PAGE_SIZE to used\n" );
#endif
				p_block->block_addr = p_block ;
				p_block->block_size = NORMAL_BLOCK_MAX_SIZE ;
				p_block->alloc_source_file = FILE ;
				p_block->alloc_source_line = LINE ;
				p_block->free_source_file = NULL ;
				p_block->free_source_line = 0 ;
				
				LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_used_root_qmalloc_block , p_block );
				LinkQmallocBlockToTreeByBlockAddr( & g_used_root_qmalloc_block , p_block );
				g_used_root_qmalloc_block.blocks_count++;
				g_used_root_qmalloc_block.blocks_total_size += p_block->block_size ;
				
				return p_block->block_base;
			}
			else
			{
				struct QmallocBlock	*p_remain_block ;
				
#if _DEBUG
printf( "DEBUG : BLOCKS_PAGE_SIZE divide [%zu]bytes to used and [%zu]bytes to unused\n" , size , BLOCKS_PAGE_SIZE - ( sizeof(struct QmallocBlock)+p_block->block_size + sizeof(struct QmallocBlock) ) );
#endif
				p_block->block_addr = p_block ;
				p_block->block_size = size ;
				p_block->alloc_source_file = FILE ;
				p_block->alloc_source_line = LINE ;
				p_block->free_source_file = NULL ;
				p_block->free_source_line = 0 ;
				LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_used_root_qmalloc_block , p_block );
				LinkQmallocBlockToTreeByBlockAddr( & g_used_root_qmalloc_block , p_block );
				g_used_root_qmalloc_block.blocks_count++;
				g_used_root_qmalloc_block.blocks_total_size += p_block->block_size ;
				
				p_remain_block = (struct QmallocBlock *)(p_block->block_base+size) ;
				p_remain_block->block_addr = p_remain_block ;
				p_remain_block->block_size = BLOCKS_PAGE_SIZE - ( sizeof(struct QmallocBlock)+p_block->block_size + sizeof(struct QmallocBlock) ) ;
				p_remain_block->alloc_source_file = NULL ;
				p_remain_block->alloc_source_line = 0 ;
				p_remain_block->free_source_file = NULL ;
				p_remain_block->free_source_line = 0 ;
				LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_unused_root_qmalloc_block , p_remain_block );
				LinkQmallocBlockToTreeByBlockAddr( & g_unused_root_qmalloc_block , p_remain_block );
				g_unused_root_qmalloc_block.blocks_count++;
				g_unused_root_qmalloc_block.blocks_total_size += p_remain_block->block_size ;
				
				return p_block->block_base;
			}
		}
		else
		{
			if( p_block->block_size - (sizeof(struct QmallocBlock)+size) < sizeof(struct QmallocBlock) )
			{
#if _DEBUG
printf( "DEBUG : entire block size[%zu]bytes to used\n" , p_block->block_size );
#endif
				UnlinkQmallocBlockFromTreeByBlockSize( & g_unused_root_qmalloc_block , p_block );
				g_unused_root_qmalloc_block.blocks_count--;
				g_unused_root_qmalloc_block.blocks_total_size -= p_block->block_size ;
				
				p_block->alloc_source_file = FILE ;
				p_block->alloc_source_line = LINE ;
				p_block->free_source_file = NULL ;
				p_block->free_source_line = 0 ;
				
				LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_used_root_qmalloc_block , p_block );
				g_used_root_qmalloc_block.blocks_count++;
				g_used_root_qmalloc_block.blocks_total_size += p_block->block_size ;
				
				return p_block->block_base;
			}
			else
			{
				struct QmallocBlock	*p_remain_block ;
				size_t			total_block_size ;
				
				total_block_size = p_block->block_size ;
#if _DEBUG
printf( "DEBUG : block size[%zu]bytes divide [%zu]bytes to used and [%zu]bytes to unused\n" , total_block_size , size , total_block_size - (sizeof(struct QmallocBlock)+size) );
#endif
				
				UnlinkQmallocBlockFromTreeByBlockSize( & g_unused_root_qmalloc_block , p_block );
				UnlinkQmallocBlockFromTreeByBlockAddr( & g_unused_root_qmalloc_block , p_block );
				g_unused_root_qmalloc_block.blocks_count--;
				g_unused_root_qmalloc_block.blocks_total_size -= p_block->block_size ;
				
				p_block->block_size = size ;
				p_block->alloc_source_file = FILE ;
				p_block->alloc_source_line = LINE ;
				p_block->free_source_file = NULL ;
				p_block->free_source_line = 0 ;
				LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_used_root_qmalloc_block , p_block );
				LinkQmallocBlockToTreeByBlockAddr( & g_used_root_qmalloc_block , p_block );
				g_used_root_qmalloc_block.blocks_count++;
				g_used_root_qmalloc_block.blocks_total_size += p_block->block_size ;
				
				p_remain_block = (struct QmallocBlock *)(p_block->block_base+size) ;
				p_remain_block->block_addr = p_remain_block ;
				p_remain_block->block_size = total_block_size - (sizeof(struct QmallocBlock)+size) ;
				p_remain_block->alloc_source_file = NULL ;
				p_remain_block->alloc_source_line = 0 ;
				p_remain_block->free_source_file = NULL ;
				p_remain_block->free_source_line = 0 ;
				LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_unused_root_qmalloc_block , p_remain_block );
				LinkQmallocBlockToTreeByBlockAddr( & g_unused_root_qmalloc_block , p_remain_block );
				g_unused_root_qmalloc_block.blocks_count++;
				g_unused_root_qmalloc_block.blocks_total_size += p_remain_block->block_size ;
				
				return p_block->block_base;
			}
		}
	}
	else
	{
		struct QmallocBlock	block ;
		struct QmallocBlock	*p_block ;
		
		block.block_size = sizeof(struct QmallocBlock) + size ;
		p_block = QueryQmallocBlockTreeByBlockSize( & g_unused_root_qmalloc_block , & block ) ;
		if( p_block )
		{
			UnlinkQmallocBlockFromTreeByBlockSize( & g_unused_root_qmalloc_block , p_block );
			UnlinkQmallocBlockFromTreeByBlockAddr( & g_unused_root_qmalloc_block , p_block );
			g_unused_root_qmalloc_block.blocks_count--;
			g_unused_root_qmalloc_block.blocks_total_size -= p_block->block_size ;
			
			p_block->alloc_source_file = FILE ;
			p_block->alloc_source_line = LINE ;
			p_block->free_source_file = NULL ;
			p_block->free_source_line = 0 ;
			
			LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_used_root_qmalloc_block , p_block );
			LinkQmallocBlockToTreeByBlockAddr( & g_used_root_qmalloc_block , p_block );
			g_used_root_qmalloc_block.blocks_count++;
			g_used_root_qmalloc_block.blocks_total_size += p_block->block_size ;
			
			return p_block->block_base;
		}
		
		p_block = (struct QmallocBlock *)malloc( block.block_size ) ;
		if( p_block == NULL )
			return NULL;
#if _DEBUG
printf( "DEBUG : malloc ok , addr[%"PRIu64"] [%zu]bytes\n" , (uint64_t)p_block , block.block_size );
#endif
		p_block->block_addr = p_block ;
		p_block->block_size = size ;
		p_block->alloc_source_file = FILE ;
		p_block->alloc_source_line = LINE ;
		p_block->free_source_file = NULL ;
		p_block->free_source_line = 0 ;
		
		LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_used_root_qmalloc_block , p_block );
		LinkQmallocBlockToTreeByBlockAddr( & g_used_root_qmalloc_block , p_block );
		g_used_root_qmalloc_block.blocks_count++;
		g_used_root_qmalloc_block.blocks_total_size += p_block->block_size ;
		
		return p_block->block_base;
	}
}

void _qfree( void *ptr , char *FILE , int LINE )
{
	struct QmallocBlock	*p_block ;
	struct QmallocBlock	*p_prev_block_order_by_addr ;
	struct QmallocBlock	*p_next_block_order_by_addr ;
	struct rb_node		*p ;
	
	if( ptr == NULL )
		return;
	
	p_block = (struct QmallocBlock *)( (char*)ptr - sizeof(struct QmallocBlock) ) ;
	
	UnlinkQmallocBlockFromTreeByBlockSize( & g_used_root_qmalloc_block , p_block );
	UnlinkQmallocBlockFromTreeByBlockAddr( & g_used_root_qmalloc_block , p_block );
	g_used_root_qmalloc_block.blocks_count--;
	g_used_root_qmalloc_block.blocks_total_size -= p_block->block_size ;
	
	p_block->free_source_file = FILE ;
	p_block->free_source_line = LINE ;
	
	LinkQmallocBlockToTreeByBlockSizeAllowduplicated( & g_unused_root_qmalloc_block , p_block );
	LinkQmallocBlockToTreeByBlockAddr( & g_unused_root_qmalloc_block , p_block );
	g_unused_root_qmalloc_block.blocks_count++;
	g_unused_root_qmalloc_block.blocks_total_size += p_block->block_size ;
	
	p = rb_prev(&(p_block->block_tree_node_order_by_addr)) ;
	if( p )
	{
		p_prev_block_order_by_addr = container_of( p , struct QmallocBlock , block_tree_node_order_by_addr ) ;
#if _DEBUG
printf( "DEBUG : on qfree , prev_block[%"PRIu64"] prev_block_size[%zu] , block[%"PRIu64"] block_size[%zu]\n" , (uint64_t)(p_prev_block_order_by_addr->block_addr) , p_prev_block_order_by_addr->block_size , (uint64_t)(p_block->block_addr) , p_block->block_size );
#endif
		if( p_prev_block_order_by_addr->block_base + p_prev_block_order_by_addr->block_size == p_block->block_addr )
		{
			p_prev_block_order_by_addr->block_size += sizeof(struct QmallocBlock) + p_block->block_size ;
			
#if _DEBUG
printf( "DEBUG : on qfree , unlink block[%"PRIu64"] block_size[%zu]\n" , (uint64_t)(p_block->block_addr) , p_block->block_size );
#endif
			UnlinkQmallocBlockFromTreeByBlockSize( & g_unused_root_qmalloc_block , p_block );
			UnlinkQmallocBlockFromTreeByBlockAddr( & g_unused_root_qmalloc_block , p_block );
			g_unused_root_qmalloc_block.blocks_count--;
			g_unused_root_qmalloc_block.blocks_total_size += sizeof(struct QmallocBlock) ;
			
			p_block = p_prev_block_order_by_addr ;
		}
	}
	
	p = rb_next(&(p_block->block_tree_node_order_by_addr)) ;
	if( p )
	{
		p_next_block_order_by_addr = container_of( p , struct QmallocBlock , block_tree_node_order_by_addr ) ;
#if _DEBUG
printf( "DEBUG : on qfree , block[%"PRIu64"] block_size[%zu] , next_block[%"PRIu64"] next_block_size[%zu]\n" , (uint64_t)(p_block->block_addr) , p_block->block_size , (uint64_t)(p_next_block_order_by_addr->block_addr) , p_next_block_order_by_addr->block_size );
#endif
		if( p_block->block_base + p_block->block_size == p_next_block_order_by_addr->block_addr )
		{
			p_block->block_size += sizeof(struct QmallocBlock) + p_next_block_order_by_addr->block_size ;
			
#if _DEBUG
printf( "DEBUG : on qfree , unlink next_block[%"PRIu64"] next_block_size[%zu]\n" , (uint64_t)(p_next_block_order_by_addr->block_addr) , p_next_block_order_by_addr->block_size );
#endif
			UnlinkQmallocBlockFromTreeByBlockSize( & g_unused_root_qmalloc_block , p_next_block_order_by_addr );
			UnlinkQmallocBlockFromTreeByBlockAddr( & g_unused_root_qmalloc_block , p_next_block_order_by_addr );
			g_unused_root_qmalloc_block.blocks_count--;
			g_unused_root_qmalloc_block.blocks_total_size += sizeof(struct QmallocBlock) ;
		}
	}
	
	if( g_unused_root_qmalloc_block.blocks_total_size > g_cache_blocks_max_size )
	{
#if _DEBUG
printf( "DEBUG : on qfree , free[%"PRIu64"]\n" , (uint64_t)(p_block) );
#endif
		UnlinkQmallocBlockFromTreeByBlockSize( & g_unused_root_qmalloc_block , p_block );
		UnlinkQmallocBlockFromTreeByBlockAddr( & g_unused_root_qmalloc_block , p_block );
		g_unused_root_qmalloc_block.blocks_count--;
		g_unused_root_qmalloc_block.blocks_total_size -= p_block->block_size ;
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
		return p_block->block_base;
	}
	else
	{
		p_block = container_of( ptr , struct QmallocBlock , block_base ) ;
		p = rb_next(&(p_block->block_tree_node_order_by_size_allowduplicate)) ;
		if( p == NULL )
			return NULL;
		p_block = container_of( p , struct QmallocBlock , block_tree_node_order_by_size_allowduplicate ) ;
		return p_block->block_base;
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
		return p_block->block_base;
	}
	else
	{
		p_block = container_of( ptr , struct QmallocBlock , block_base ) ;
		p = rb_next(&(p_block->block_tree_node_order_by_size_allowduplicate)) ;
		if( p == NULL )
			return NULL;
		p_block = container_of( p , struct QmallocBlock , block_tree_node_order_by_size_allowduplicate ) ;
		return p_block->block_base;
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
		return p_block->block_base;
	}
	else
	{
		p_block = container_of( ptr , struct QmallocBlock , block_base ) ;
		p = rb_next(&(p_block->block_tree_node_order_by_addr)) ;
		if( p == NULL )
			return NULL;
		p_block = container_of( p , struct QmallocBlock , block_tree_node_order_by_addr ) ;
		return p_block->block_base;
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
		return p_block->block_base;
	}
	else
	{
		p_block = container_of( ptr , struct QmallocBlock , block_base ) ;
		p = rb_next(&(p_block->block_tree_node_order_by_addr)) ;
		if( p == NULL )
			return NULL;
		p_block = container_of( p , struct QmallocBlock , block_tree_node_order_by_addr ) ;
		return p_block->block_base;
	}
}

size_t _qget_size( void *ptr )
{
	struct QmallocBlock	*p_block = container_of( ptr , struct QmallocBlock , block_base ) ;
	return p_block->block_size;
}

char *_qget_alloc_source_file( void *ptr )
{
	struct QmallocBlock	*p_block = container_of( ptr , struct QmallocBlock , block_base ) ;
	return p_block->alloc_source_file;
}

int _qget_alloc_source_line( void *ptr )
{
	struct QmallocBlock	*p_block = container_of( ptr , struct QmallocBlock , block_base ) ;
	return p_block->alloc_source_line;
}

char *_qget_free_source_file( void *ptr )
{
	struct QmallocBlock	*p_block = container_of( ptr , struct QmallocBlock , block_base ) ;
	return p_block->free_source_file;
}

int _qget_free_source_line( void *ptr )
{
	struct QmallocBlock	*p_block = container_of( ptr , struct QmallocBlock , block_base ) ;
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
		g_unused_root_qmalloc_block.blocks_total_size -= p_block->block_size ;
#if _DEBUG
printf( "DEBUG : on _qfree_all_unused , free[%"PRIu64"]\n" , (uint64_t)(p_block) );
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

