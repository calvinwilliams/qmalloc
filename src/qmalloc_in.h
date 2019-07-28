#ifndef _H_QMALLOC_IN_
#define _H_QMALLOC_IN_

#ifdef __cplusplus
extern "C" {
#endif

#include "qmalloc.h"
#include <stdint.h>
#include <inttypes.h>

#include "rbtree_tpl.h"
#include "list.h"

#define BLOCKS_PAGE_SIZE	4*1024*1024
#define NORMAL_BLOCK_MAX_SIZE	(BLOCKS_PAGE_SIZE-sizeof(struct QmallocBlock))

struct QmallocBlock
{
	unsigned char			dmz[7] ; /* 防止上一内存块写数据越界破坏当前块头信息 */
	
	unsigned char			alloc_page_flag ;
	
	struct QmallocBlockClass	*p_block_class ;
	
	struct list_head		block_list_node ;
	
	void				*block_addr ;
	struct rb_node			block_tree_node_order_by_addr ;
	
	char				*alloc_source_file ;
	size_t				alloc_source_line ;
	
	char				data_addr[0] ;
} ;

struct QmallocBlockClass
{
	size_t			data_size ;
	struct rb_node		block_class_tree_node_order_by_size ;
	
	struct list_head	block_list ;
} ;

struct QmallocBlockClassTree
{
	struct rb_root		block_tree_order_by_addr ;
	struct rb_root		block_class_tree_order_by_size ;
	size_t			blocks_count ;
	size_t			blocks_total_size ;
} ;

int LinkQmallocBlockClassToTreeByDataSize( struct QmallocBlockClassTree *block_tree , struct QmallocBlock *p_block );
void UnlinkQmallocBlockClassFromTreeByDataSize( struct QmallocBlockClassTree *block_tree , struct QmallocBlock *p_block );
struct QmallocBlock *QueryQmallocBlockClassTreeByDataSize( struct QmallocBlockClassTree *block_tree , struct QmallocBlock *block );
struct QmallocBlock *QueryQmallocBlockClassTreeByDataSizeNolessthan( struct QmallocBlockClassTree *block_tree , struct QmallocBlock *block );
struct QmallocBlock *TravelQmallocBlockClassTreeByDataSize( struct QmallocBlockClassTree *block_tree , struct QmallocBlock *p_block );

int LinkQmallocBlockToTreeByBlockAddr( struct QmallocBlockClassTree *block_tree , struct QmallocBlock *p_block );
void UnlinkQmallocBlockFromTreeByAddr( struct QmallocBlockClassTree *block_tree , struct QmallocBlock *p_block );
struct QmallocBlock *QueryQmallocBlockTreeByAddr( struct QmallocBlockClassTree *block_tree , struct QmallocBlock *block );
struct QmallocBlock *TravelQmallocBlockTreeByAddr( struct QmallocBlockClassTree *block_tree , struct QmallocBlock *p_block );

#ifdef __cplusplus
extern }
#endif

#endif
