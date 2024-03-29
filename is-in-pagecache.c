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
 * is-in-pagecache: try to infer if the content of files is actually in
 * pagecache.
 * All these files must be readable by the user.
 */

#include <sys/mman.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "errwarn.h"

const char progname[] = "is-in-pagecache";

int main(int argc, char *argv[])
{
	struct stat	 st;
	void		*filemap = NULL;
#ifdef __linux__
	unsigned char	*vec = NULL;
#else
	char		*vec = NULL;
#endif
	size_t		 vlen;
	unsigned long	 pim, k;
	long int	 pagesize;
	int		 i, fd;

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
				filemap = (unsigned char*) mmap(NULL, st.st_size,
				    PROT_READ, MAP_SHARED, fd, 0);
				if (filemap == MAP_FAILED) {
					warning(errno, "Unable to map '%s'",
					    argv[i]);
					continue;
				}

				vlen = (st.st_size + pagesize - 1) / pagesize;
				vec = malloc(vlen);
				if (vec == NULL)
					error(2, errno, "Unable to allocate "
					    "vec[%lu]", (unsigned long) vlen);

				if (mincore(filemap, st.st_size, vec) == -1) {
					warning(errno, "Unable to get core info"
					    " for '%s'", argv[i]);
					continue;
				}
				pim = 0;
				for (k = 0; k < vlen; k++) {
					if (vec[k] & 1)
						pim++;
				}
				printf("'%s': %lu pages out of %lu appear to be"
				    " in pagecache\n", argv[i], pim,
				    (unsigned long) vlen);

				free(vec);
				vec = NULL;

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

