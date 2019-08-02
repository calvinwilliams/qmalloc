# 此文件由makeobj.sh自动生成
############################################################
# 项目名 : 
# 模块名 : 
# 备  注 : 
############################################################

###### 源文件配置区
#@ c_FILE
c_FILE		=	\
			util.c \
			test_min.c \
			test_full.c \
			press_qmalloc.c \
			press_malloc.c \

###### 目标文件、安装目录配置区
include makeinstall
BIN		=	\
			test_min	\
			test_full	\
			press_qmalloc	\
			press_malloc	\

###### 编译选项
CFLAGS		=	$(_CFLAGS) \
			-I$(HOME)/include/qmalloc \

###### 链接选项
LFLAGS		=	$(_LFLAGS) \
			-L$(HOME)/lib \
			-lqmalloc \

###### 额外宏定义区
CLEAN_ADDITION	=

PREMAKE		=

###### 加载mktpl模板库
#@ make_all
#@ make_clean_test
#@ make_clean
#@ make_install
#@ make_install_BININST
#@ make_install_DFTHDERINST
#@ make_uninstall
#@ make_uninstall_BININST
#@ make_uninstall_DFTHDERINST
include $(MKTPL2_HOME)/makeobj_$(MKTPL2_OS).inc

###### 目标文件依赖关系
test_min		:	test_min.o util.o
	$(CC) -o $@ test_min.o util.o $(LFLAGS)

test_full		:	test_full.o util.o
	$(CC) -o $@ test_full.o util.o $(LFLAGS)

press_qmalloc		:	press_qmalloc.o util.o
	$(CC) -o $@ press_qmalloc.o util.o $(LFLAGS)

press_malloc		:	press_malloc.o
	$(CC) -o $@ press_malloc.o $(_LFLAGS)

