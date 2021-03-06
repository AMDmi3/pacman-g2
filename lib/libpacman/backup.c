/*
 *  backup.c
 *
 *  Copyright (c) 2005 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2005 by Christian Hamar <krics@linuxforum.hu>
 *  Copyright (c) 2006 by Miklos Vajna <vmiklos@frugalware.org>
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

#include "config.h"
#include <stdlib.h>
#include <string.h>
/* pacman-g2 */
#include "backup.h"

/* Look for a filename in a pmpkg_t.backup list.  If we find it,
 * then we return the md5 or sha1 hash (parsed from the same line)
 */
char *_pacman_needbackup(char *file, pmlist_t *backup)
{
	pmlist_t *lp;

	if(file == NULL || backup == NULL) {
		return(NULL);
	}

	/* run through the backup list and parse out the md5 or sha1 hash for our file */
	for(lp = backup; lp; lp = lp->next) {
		char *str = strdup(lp->data);
		char *ptr;

		/* tab delimiter */
		ptr = strchr(str, '\t');
		if(ptr == NULL) {
			free(str);
			continue;
		}
		*ptr = '\0';
		ptr++;
		/* now str points to the filename and ptr points to the md5 or sha1 hash */
		if(!strcmp(file, str)) {
			char *md5 = strdup(ptr);
			free(str);
			return(md5);
		}
		free(str);
	}

	return(NULL);
}

/* vim: set ts=2 sw=2 noet: */
