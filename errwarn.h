/*
 */

#ifndef __ERRWARN_H__
#define __ERRWARN_H__

#include <errno.h>
#include <stdarg.h>

extern void error(const int, const int, const char *, ...);
extern void warning(const int, const char *, ...);

#endif /* __ERRWARN_H__ */

