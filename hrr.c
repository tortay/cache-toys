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
 * hrr.c: Reads data randomly from a file, giving (POSIX) hints or
 * instructions (readahead()) to the pagecache.
 */

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/vfs.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "errwarn.h"

const char	progname[] = "hrr";
const off_t	default_alignment = 512;
const size_t	default_minbsize = 512;
const size_t	default_maxbsize = 32768;

void usage(FILE *);
int give_posix_hints(int, off_t, size_t);
int prefetch(int, off_t, size_t);
#ifndef GLIBC_IS_NO_LONGER_BRAINDEAD
ssize_t readahead(int fd, off_t *offset, size_t count);
#endif /* GLIBC_IS_NO_LONGER_BRAINDEAD */

void usage(FILE *fp)
{
	fprintf(fp,
"\nReads data randomly from a file.\n"
"\nUsage:\n"
"%s [-b minbsize] [-B maxbsize] [-H] [-L length] [-O startoffset] [-P] [-R] "
"[-S size] [-Z alignment] filename\n"
"\nWhere:\n"
" -b minbsize: set the minimum default read block size to minbsize.\n"
"    Default is: %lu bytes.\n"
" -B maxbsize: set the maximum default read block size to maxbsize.\n"
"    Default is: %lu bytes.\n"
" -H gives a hint to the filesystem (with posix_fadvise)\n"
" -L length: max offset to read from, relative to startoffset, default is\n"
"    (file size - offset).\n"
" -O startoffset: do I/Os in the file from that offset on, default is 0\n"
"    start of file.\n"
" -P Use pread instead of lseek & read.\n"
" -R instructs the pagecache to prefetch the file target zone before reading.\n"
" -S size: amount of data to read, default is 1/8 of the file size.\n"
" -Z alignment: align read block boundaries on alignment (bytes).\n"
"    For pure random reads use 1, 512 for sector alignment, 4096 for generic\n"
"    FS block alignment, etc.  Default is sector alignment.\n"
"\nFor cache hints and instructions, the prefetched/hinted part of the file\n"
"is the zone between 'startoffset' & 'startoffset+length' (these values can\n"
"be specified with -O & -L)\n",
	    progname, (unsigned long) default_minbsize,
	    (unsigned long) default_maxbsize);

	exit(1);
}


int main(int argc, char *argv[])
{
	struct timeval	 tv;
	struct stat	 st;
	unsigned char	*buffer = NULL;
	char		*filename = NULL;
	off_t		 offset = 0, start_offset;
	off_t		 alignment = default_alignment;
	size_t		 toread = 0, length = 0, bsize = 0;
	size_t		 minbsize = default_minbsize;
	size_t		 maxbsize = default_maxbsize;;
	ssize_t		 nr = 0;
	unsigned int	 seed = 0;
	int		 fd = 1;
	int		 ch = -1;
	long long int	 opt_size = 0, opt_maxbsize = 0, opt_minbsize = 0;
	long long int	 opt_offset = 0, opt_length = 0, opt_alignment = 0;
	int		 opt_pread = 0, opt_give_hints = 0, opt_prefetch = 0;

	while ((ch = getopt(argc, argv, ":b:B:HL:O:PRS:Z:")) != -1) {
		switch (ch) {
		case 'b':
			opt_minbsize = atoll(optarg);
			break;
		case 'B':
			opt_maxbsize = atoll(optarg);
			break;
		case 'H':
			opt_give_hints = 1;
			break;
		case 'L':
			opt_length = atoll(optarg);
			break;
		case 'O':
			opt_offset = atoll(optarg);
			break;
		case 'P':
			opt_pread = 1;
			break;
		case 'R':
			opt_prefetch = 1;
			break;
		case 'S':
			opt_size = atoll(optarg);
			break;
		case 'Z':
			opt_alignment = atoll(optarg);
			break;
		default:
			usage(stderr);
			break;
		}
	}
	if (optind != (argc - 1)) {
		warning(-1, "Filename required");
		usage(stderr);
	}
	filename = argv[optind];

	memset(&st, 0, sizeof(st));
	if (lstat(filename, &st) == -1)
		error(1, errno, "Unable to stat '%s'", argv[1]);

	if (!S_ISREG(st.st_mode))
		error(1, -1, "'%s' is not a regular file", argv[1]);

	if (opt_size) {
		if (opt_size > 0)
			toread = (size_t) opt_size;
		else {
			close(fd);
			error(1, -1, "Invalid block size: %d", opt_size);
		}
	} else
		toread = (size_t) st.st_size / 8;

	if (opt_maxbsize) {
		if (opt_maxbsize > 0)
			maxbsize = (size_t) opt_maxbsize;
		else {
			close(fd);
			error(1, -1, "Invalid max block size: %d", opt_maxbsize);
		}
	}

	if (opt_minbsize) {
		if (opt_minbsize > 0)
			minbsize = (size_t) opt_minbsize;
		else {
			close(fd);
			error(1, -1, "Invalid min block size: %d", opt_minbsize);
		}
	}

	if (opt_offset) {
		if (opt_offset > 0)
			start_offset = (off_t) opt_offset;
		else {
			close(fd);
			error(1, -1, "Invalid start offset: %d", opt_offset);
		}
		if (start_offset > st.st_size) {
			close(fd);
			error(1, -1, "Start offset beyond '%s' size: %lu",
			    (unsigned long) start_offset);
		}
	} else
		start_offset = (off_t) 0U;

	if (opt_length) {
		if (opt_length > 0)
			length = (off_t) opt_length;
		else {
			close(fd);
			error(1, -1, "Invalid initial length: %d", opt_length);
		}
		if ((start_offset + (off_t) length) > st.st_size) {
			close(fd);
			error(1, -1, "Target zone beyond '%s' size: %lu",
			    (unsigned long) start_offset + length);
		}
	} else
		length = (size_t) st.st_size - (size_t) start_offset;

	if (opt_alignment) {
		if (opt_alignment > 0)
			alignment = (off_t) opt_alignment;
		else {
			close(fd);
			error(1, -1, "Invalid alignmen: %d", opt_alignment);
		}
	} else
		alignment = default_alignment;

	fd = open(filename, O_RDONLY);
	if (fd == -1)
		error(1, errno, "Unable to open '%s'", filename);

	if (opt_give_hints) {
		int hrc = give_posix_hints(fd, start_offset, length);

		if (hrc == -1)
			warning(errno, "Unable to give cache hints for '%s'",
			    filename);

		if (opt_prefetch)
			warning(-1, "Prefetch & FS hints are mutually "
			    "exclusive");
	} else if (opt_prefetch)
		prefetch(fd, start_offset, length);

	memset(&tv, 0, sizeof(tv));
	if (gettimeofday(&tv, NULL) == -1)
		error(1, errno, "Unable to get time of day");

	seed = (unsigned) tv.tv_usec;
	srand(seed);

	buffer = malloc(maxbsize);
	if (buffer == NULL)
		error(1, errno, "Unable to allocate memory for buffer (%lu "
		    "bytes)", (unsigned long) maxbsize);

	offset = start_offset;
	printf("Will read %lu bytes from '%s', window: [%lu:%lu], minb: "
	    "%lu, maxb: %lu, alignment: %lu\n",
	    (unsigned long) toread, filename, (unsigned long) offset,
	    (unsigned long) (offset + length), (unsigned long) minbsize,
	    (unsigned long) maxbsize, (unsigned long) alignment);

	if (opt_give_hints)
		printf("Hints given to the FS, read from window [%lu:%lu] "
		    "from '%s'\n", (unsigned long) offset,
		    (unsigned long) (offset + length), filename);
	else if (opt_prefetch)
		printf("Instructed the pagecache to prefetch window [%lu:%lu] "
		    "from '%s'\n", (unsigned long) offset,
		    (unsigned long) (offset + length), filename);

	if (maxbsize < minbsize) {
		printf("minbsize (%lu) > maxbsize (%lu), min <=> max\n",
		    (unsigned long) minbsize, (unsigned long) maxbsize);
		size_t m = minbsize; minbsize = maxbsize; maxbsize = m;
		if (realloc(buffer, maxbsize) == NULL)
			error(1, errno, "Failed to expand buffer to %lu bytes",
			    maxbsize);
	}

	while (toread > 0) {
		if (toread < maxbsize)
			maxbsize = toread;

		if (maxbsize < minbsize)
			minbsize = maxbsize;

		bsize = minbsize + (size_t) ((double) (maxbsize - minbsize)
			* (double) rand() / (double) RAND_MAX);
		offset = start_offset + (off_t) ((double) length
			* (double) rand() / (double) RAND_MAX);

		if (offset > st.st_size)
			offset = st.st_size;

		offset -= offset % alignment;

		if (offset >= (off_t) bsize)
			offset -= (off_t) bsize;

		if (opt_pread)
			nr = pread(fd, buffer, bsize, offset);
		else {
			if (lseek(fd, offset, SEEK_SET) != offset)
				warning(errno, "Unable to seek to %lu in '%s'",
				    (unsigned long) offset, filename);

			nr = read(fd, buffer, bsize);
		}
		if (nr == -1)
			error(1, errno, "Error while reading '%s', offset: %lu",
			    filename, (unsigned long) offset);

		toread -= (size_t) nr;
	}
	if (close(fd) == -1)
		warning(errno, "Problem closing '%s'", filename);

	free(buffer);

	return (0);
}



int give_posix_hints(int fd, off_t start, size_t len)
{
	return posix_fadvise(fd, start, len, POSIX_FADV_WILLNEED);
}


int prefetch(int fd, off_t start, size_t len)
{
	return readahead(fd, &start, len);
}

