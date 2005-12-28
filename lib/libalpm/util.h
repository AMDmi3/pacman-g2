/*
 *  util.h
 * 
 *  Copyright (c) 2002-2005 by Judd Vinet <jvinet@zeroflux.org>
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
#ifndef _ALPM_UTIL_H
#define _ALPM_UTIL_H

#include <stdio.h>
#include <archive.h>
#include <archive_entry.h>

#define MALLOC(p, b) { \
	if((b) > 0) { \
		p = malloc(b); \
		if (!(p)) { \
			fprintf(stderr, "malloc failure: could not allocate %d bytes\n", (b)); \
			exit(1); \
		} \
	} else { \
		p = NULL; \
	} \
}
#define FREE(p) do { if (p) { free(p); p = NULL; } } while(0)

#define ASSERT(cond, action) do { if(!(cond)) { action; } } while(0)

#define STRNCPY(s1, s2, len) do { \
	strncpy(s1, s2, (len)-1); \
	s1[(len)-1] = 0; \
} while(0)

#define ARCHIVE_EXTRACT_FLAGS ARCHIVE_EXTRACT_OWNER | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_TIME

int _alpm_archive_read_entry_data_into_fd (struct archive *archive, int file);
int _alpm_makepath(char *path);
int _alpm_copyfile(char *src, char *dest);
char *_alpm_strtoupper(char *str);
char *_alpm_strtrim(char *str);
int _alpm_grep(const char *fn, const char *needle);
int _alpm_lckmk(char *file);
int _alpm_lckrm(char *file);
int _alpm_unpack(char *archive, const char *prefix, const char *fn);
int _alpm_rmrf(char *path);
int _alpm_log_action(unsigned char usesyslog, FILE *f, char *fmt, ...);
int _alpm_ldconfig(char *root);
int _alpm_runscriptlet(char *util, char *installfn, char *script, char *ver, char *oldver);
char *_alpm_strdep(int mod);

#endif /* _ALPM_UTIL_H */

/* vim: set ts=2 sw=2 noet: */
