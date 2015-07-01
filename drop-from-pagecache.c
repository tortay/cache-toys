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
 * drop-from-pagecache: give hints to the pagecache to make room by removing
 * the no-longer needed data for the files which names are given on the command
 * line.
 * All these files must be readable by the user.  Files which are otherwise
 * accessed by a process (of the current user or another) are likely to be only
 * partially removed from the pagecache.
 * This is a hint given to the pagecache which is free to ignore it.
 */

#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "errwarn.h"

const char progname[] = "drop-from-pagecache";

int main(int argc, char *argv[])
{
	struct stat st;
	int i, fd;

	for (i = 1; i < argc; i++) {
		fd = open(argv[i], O_RDONLY);
		if (fd != -1) {
			if (fstat(fd, &st) == -1) {
				warning(errno, "Unable to stat '%s'", argv[i]);
				continue;
			}
			if (st.st_size > 0 && posix_fadvise(fd, 0, st.st_size,
			    POSIX_FADV_DONTNEED) != 0) {
				warning(errno, "Unable to give cache hint for "
				    "'%s'", argv[i]);
				continue;
			}
			if (close(fd) == -1)
				warning(errno, "Problem closing '%s'", argv[i]);
		} else
			warning(errno, "Unable to open '%s'", argv[i]);
	}
	return (0);
}

