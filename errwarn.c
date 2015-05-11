/*
 * *BSD inspired error & warning reporting micro-framework.
 *
 * Copyright (c) 2006-2015 Loic Tortay <tortay@cc.in2p3.fr>.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errwarn.h"

extern const char progname[];

static void verror(const int, const char*, va_list);


void
verror(const int errnum, const char *fmt, va_list ap)
{
	char *errmsg = NULL;

	if (errnum != -1)
		errmsg = strerror(errnum);

	fprintf(stderr, "%s: ", progname);
	if (fmt != NULL)
		vfprintf(stderr, fmt, ap);

	if (errnum != -1)
		fprintf(stderr, ": %.100s (%d)\n", errmsg, errnum);
	else
		fputc('\n', stderr);
}


void
error(const int excode, const int errnum, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	verror(errnum, fmt, ap);
	va_end(ap);

	exit(excode);
}


void
warning(const int errnum, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	verror(errnum, fmt, ap);
	va_end(ap);
}

