#ifndef _H_QMALLOC_
#define _H_QMALLOC_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define qmalloc(_size_)			_qmalloc(_size_,__FILE__,__LINE__)
#define qfree(_ptr_)			_qfree(_ptr_,__FILE__,__LINE__)
#define qcalloc(_nmemb_,_size_)		_qmalloc((_nmemb_*_size_),__FILE__,__LINE__)
#define qrealloc(_ptr_,_size_)		_qrealloc(_ptr_,_size_,__FILE__,__LINE__)
#define qstrdup(_str_)			_qstrdup(_str_,__FILE__,__LINE__)
#define qstrndup(_str_,_n_)		_qstrndup(_str_,_n_,__FILE__,__LINE__)

void *_qmalloc( size_t size , char *FILE , int LINE );
void _qfree( void *ptr , char *FILE , int LINE );
void *_qrealloc( void *ptr , size_t size , char *FILE , int LINE );
void *_qstrdup( const char *s , char *FILE , int LINE );
void *_qstrndup( const char *s , size_t n , char *FILE , int LINE );

void *_qtravel_used_by_size( void *ptr );
void *_qtravel_unused_by_size( void *ptr );
void *_qtravel_used_by_addr( void *ptr );
void *_qtravel_unused_by_addr( void *ptr );

size_t _qget_size( void *ptr );
char *_qget_alloc_source_file( void *ptr );
int _qget_alloc_source_line( void *ptr );
char *_qget_free_source_file( void *ptr );
int _qget_free_source_line( void *ptr );

void _qfree_all();

int _qget_block_header_size();
int _qget_normal_block_max_size();
int _qget_blocks_page_size();

#ifdef __cplusplus
extern }
#endif

#endif
