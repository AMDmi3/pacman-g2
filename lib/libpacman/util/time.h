/*
 *  time.h
 *
 *  Copyright (c) 2013 by Michel Hermier <hermier@frugalware.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *  USA.
 */
#ifndef _PACMAN_TIME_H
#define _PACMAN_TIME_H

#include <time.h>

#define PM_TIME_INVALID ((time_t) -1)

static inline
double f_difftimeval(struct timeval timeval1, struct timeval timeval2)
{
	return difftime(timeval1.tv_sec, timeval2.tv_sec) +
		((double)(timeval1.tv_usec - timeval2.tv_usec) / 1000000);
}

/* Return the localtime for timep. If timep is NULL, return the conversion for time(NULL) (libc returns NULL instead).
 */
struct tm *f_localtime(const time_t *timep);

/* _lc means localised C */
size_t f_strftime_lc(char *s, size_t max, const char *format, const struct tm *tm);
char *f_strptime_lc(const char *s, const char *format, struct tm *tm);

#define F_RFC1123_FORMAT "%a, %d %b %Y %H:%M:%S %z"
#define f_strftime_rfc1123_lc(s, max, tm) f_strftime_lc((s), (max), F_RFC1123_FORMAT, (tm))
#define f_strptime_rfc1123_lc(s, tm) f_strptime_lc((s), F_RFC1123_FORMAT, (tm))

#endif /* _PACMAN_TIME_H */

/* vim: set ts=2 sw=2 noet: */
