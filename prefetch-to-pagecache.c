/*
 * prefetch-to-pagecache: give hints to the pagecache to prefetch the content
 * of the files which names are given on the command line.
 * All these files must be readable by the user.
 * This is a hint given to the pagecache which is free to ignore it.
 *
 * Copyright (c) 2012-2015 Loic Tortay <tortay@cc.in2p3.fr>.
 */

#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "errwarn.h"

const char progname[] = "prefetch-to-pagecache";

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
			if (posix_fadvise(fd, 0, st.st_size,
			    POSIX_FADV_WILLNEED) != 0) {
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

