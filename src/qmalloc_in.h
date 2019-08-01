#ifndef _H_QMALLOC_IN_
#define _H_QMALLOC_IN_

#ifdef __cplusplus
extern "C" {
#endif

#include "qmalloc.h"
#include <stdint.h>
#include <inttypes.h>

#include "list.h"

/*
00	1
01	2
02	4
03	8
04	16
05	32
06	64
07	128
08	256
09	512
10	1024
11	2048
12	4096
13	8192
14	16384
15	32768
16	65536		64KB

17	131072		128KB
18	262144
19	524288
20	1048576
21	2097152
22	4194304
23	8388608
24	16777216
25	33554432
26	67108864	64MB

27	134217728	128MB
28	268435456
29	536870912
30	1073741824
31	2147483648
32	4294967296
*/

#define MAX_THREAD_MEMPOOL_BLOCK_SIZE_CLASS	16
#define MAX_THREAD_MEMPOOL_BLOCK_SIZE		(1<<MAX_THREAD_MEMPOOL_BLOCK_SIZE_CLASS) /* 低于等于此大小的内存块在线程全局池中管理 */

#define MAX_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS	26
#define MAX_PROCESS_MEMPOOL_BLOCK_SIZE		(1<<MAX_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS) /* 低于等于此大小的内存块在进程全局池中管理 */

#define DIFF_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS	(MAX_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS-MAX_THREAD_MEMPOOL_BLOCK_SIZE_CLASS)

struct QmallocBlockHeader
{
	unsigned char			dmz ; /* 防止上一内存块写数据越界破坏当前块头信息 */
	
	struct QmallocBlockClass	*p_block_class ; /* 内存块大小类 */
	
	char				*alloc_source_file ; /* 申请内存时的源代码文件名 */
	size_t				alloc_source_line ; /* 申请内存时的源代码文件行号 */
	
	struct list_head		block_list_node ; /* 内存块链表节点 */
	
	char				data_base[0] ; /* 内存块数据基地址 */
} ;

struct QmallocBlockClass
{
	size_t			block_size ; /* 内存块大小 */
	
	struct list_head	used_block_list ; /* 已使用内存块链表节点 */
	struct list_head	unused_block_list ; /* 未使用内存块链表节点 */
} ;

#define SPINLOCK_UNLOCK		0
#define SPINLOCK_LOCK		1

#define LOCK_SPINLOCK		while( __sync_val_compare_and_swap( & g_spanlock_status , SPINLOCK_UNLOCK , SPINLOCK_LOCK ) == SPINLOCK_LOCK ) ;
#define UNLOCK_SPINLOCK		g_spanlock_status = SPINLOCK_UNLOCK ;

#ifdef __cplusplus
extern }
#endif

#endif
