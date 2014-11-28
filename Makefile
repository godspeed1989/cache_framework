CC = gcc
CFLAGS = -Wall -Werror -Wno-unused-function -g
RM = rm -f

INCLUDE = -I./include
_OBJS += cache/CacheCommon.o cache/Storage.o
_OBJS += bpt/bpt_insert.o bpt/bpt_delete.o \
         bpt/bpt_find.o bpt/bpt_util.o bpt/bpt_print.o
_OBJS += replace/LRU.o replace/LRFU.o

# in cmd of windows
ifeq ($(SHELL),sh.exe)
    RM := del /f/q
    OBJS = $(subst /,\,$(_OBJS))
else
    OBJS = $(_OBJS)
endif

all: Test

Test: Test.c $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) $+ -o $@

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	$(RM) $(OBJS) *.o Test *.dot *.exe
