#
# $Id$
#
CC	= cc
CFLAGS	= -g -m64 -D_XOPEN_SOURCE=600 -W -Wall -Werror -Wstrict-prototypes \
	-Wpointer-arith -Wmissing-prototypes -Wsign-compare -std=c99 \
	-pedantic -pipe
LDFLAGS	=
#
RM	= /bin/rm
#

.SUFFIXES:	.c .o
.c.o:
	$(CC) $(CFLAGS) -c $<

all: drop-from-pagecache is-in-pagecache prefetch-to-pagecache slices-in-pagecache


drop-from-pagecache: drop-from-pagecache.o errwarn.o
	@$(RM) -f $@
	$(CC) $^ $(LDFLAGS) -o $@

is-in-pagecache: is-in-pagecache.o errwarn.o
	@$(RM) -f $@
	$(CC) $^ $(LDFLAGS) -o $@

hrr: hrr.o errwarn.o
	@$(RM) -f $@
	$(CC) $^ $(LDFLAGS) -o $@

prefetch-to-pagecache: prefetch-to-pagecache.o errwarn.o
	@$(RM) -f $@
	$(CC) $^ $(LDFLAGS) -o $@

slices-in-pagecache: slices-in-pagecache.o errwarn.o
	@$(RM) -f $@
	$(CC) $^ $(LDFLAGS) -o $@

clean:
	@$(RM) -f *.o drop-from-pagecache hrr is-in-pagecache prefetch-to-pagecache prefetch-to-pagecache slices-in-pagecache

