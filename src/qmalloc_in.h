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
#define MAX_THREAD_MEMPOOL_BLOCK_SIZE		(1<<MAX_THREAD_MEMPOOL_BLOCK_SIZE_CLASS) /* ���ڵ��ڴ˴�С���ڴ�����߳�ȫ�ֳ��й��� */

#define MAX_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS	26
#define MAX_PROCESS_MEMPOOL_BLOCK_SIZE		(1<<MAX_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS) /* ���ڵ��ڴ˴�С���ڴ���ڽ���ȫ�ֳ��й��� */

#define DIFF_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS	(MAX_PROCESS_MEMPOOL_BLOCK_SIZE_CLASS-MAX_THREAD_MEMPOOL_BLOCK_SIZE_CLASS)

struct QmallocBlockHeader
{
	unsigned char			dmz ; /* ��ֹ��һ�ڴ��д����Խ���ƻ���ǰ��ͷ��Ϣ */
	
	struct QmallocBlockClass	*p_block_class ; /* �ڴ���С�� */
	
	char				*alloc_source_file ; /* �����ڴ�ʱ��Դ�����ļ��� */
	size_t				alloc_source_line ; /* �����ڴ�ʱ��Դ�����ļ��к� */
	
	struct list_head		block_list_node ; /* �ڴ������ڵ� */
	
	char				data_base[0] ; /* �ڴ�����ݻ���ַ */
} ;

struct QmallocBlockClass
{
	size_t			block_size ; /* �ڴ���С */
	
	struct list_head	used_block_list ; /* ��ʹ���ڴ������ڵ� */
	struct list_head	unused_block_list ; /* δʹ���ڴ������ڵ� */
} ;

#define SPINLOCK_UNLOCK		0
#define SPINLOCK_LOCK		1

#define LOCK_SPINLOCK		while( __sync_val_compare_and_swap( & g_spanlock_status , SPINLOCK_UNLOCK , SPINLOCK_LOCK ) == SPINLOCK_LOCK ) ;
#define UNLOCK_SPINLOCK		g_spanlock_status = SPINLOCK_UNLOCK ;

#ifdef __cplusplus
extern }
#endif

#endif
