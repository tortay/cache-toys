/*
 * Copyright (c) 2006-2015, Loic Tortay <tortay@cc.in2p3.fr>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * slices-in-pagecache: try to infer which parts of files are actually in
 * pagecache.
 * All these files must be readable by the user.
 */

#ifndef GLIBC_IS_NO_LONGER_BRAINDEAD
/* _BSD_SOURCE required for mincore() with Glibc */
#define _BSD_SOURCE
#endif /* GLIBC_IS_NO_LONGER_BRAINDEAD */

#include <sys/mman.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "errwarn.h"

const char progname[] = "slices-in-pagecache";

void print_slice(int, long int, long int, long int);

void
print_slice(int sdx, long int pgsz, long int slstart, long int slend)
{
	/* sim: pages in memory in "this slice", "+ 1" since slstart is
	 * 0-based
	 */
	unsigned long sim = (slend + 1 - slstart) / pgsz;
	printf("\tSlice[%d]: %lu:%lu (%lu pages)\n", sdx, slstart, slend, sim);
}

int main(int argc, char *argv[])
{
	struct stat	 st;
	void		*filemap = NULL;
#ifdef __linux__
	unsigned char	*pages = NULL;
#else
	char		*pages = NULL;
#endif
	size_t		 lip; /* lip: length in pages */
	unsigned long	 pim, /* pim: pages in memory counter */ k;
	long int	 pagesize, slice_start, slice_end;
	int		 i, fd, in_a_slice, sindex;

	pagesize = sysconf(_SC_PAGESIZE);
	if (pagesize == -1)
		error(1, errno, "Unable to get pagesize");
	else
		printf("Pagesize is: %ld bytes.\n", pagesize);

	for (i = 1; i < argc; i++) {
		fd = open(argv[i], O_RDONLY);
		if (fd != -1) {
			if (fstat(fd, &st) == -1) {
				warning(errno, "Unable to stat '%s'", argv[i]);
				continue;
			}
			if (st.st_size > 0) {
				filemap = mmap(NULL, st.st_size, PROT_READ,
				    MAP_SHARED, fd, 0);
				if (filemap == MAP_FAILED) {
					warning(errno, "Unable to map '%s'",
					    argv[i]);
					continue;
				}
				lip = (st.st_size + pagesize - 1) / pagesize;
				pages = malloc(lip);
				if (pages == NULL)
					error(2, errno, "Unable to allocate "
					    "pages[%lu]", (unsigned long) lip);

				if (mincore(filemap, st.st_size, pages) == -1) {
					warning(errno, "Unable to get core info"
					    " for '%s'", argv[i]);
					continue;
				}
				pim = 0;
				sindex = slice_start = slice_end = in_a_slice = 0;
				printf("'%s':\n", argv[i]);
				for (k = 0; k < lip; k++) {
					if (pages[k] & 1) {
						if (!in_a_slice) {
							in_a_slice = 1;
							slice_start = pagesize
							    * k;
							slice_end = slice_start
							    + pagesize - 1;
						} else {
							slice_end += pagesize;
						}
						pim++;
					} else if (in_a_slice) {
						in_a_slice = 0;
						print_slice(sindex, pagesize,
						    slice_start, slice_end);
						sindex++;
					}
				}
				if (in_a_slice) {
				/* last page of the file in pagecache ? */
					print_slice(sindex, pagesize,
					    slice_start, slice_end);
				}
				printf("\t%lu pages out of %lu appear to be in "
				    "pagecache\n", pim, (unsigned long) lip);

				free(pages);
				pages = NULL;

				if (munmap(filemap, st.st_size) == -1) {
					warning(errno, "Unable to unmap '%s'",
					    argv[i]);
					continue;
				}
			}
			if (close(fd) == -1)
				warning(errno, "Problem closing '%s'", argv[i]);
		} else {
			warning(errno, "Unable to open '%s'", argv[i]);
		}
	}
	return (0);
}

