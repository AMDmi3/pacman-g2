/*
 *  stringlist.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
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
#include <stdio.h>
#include <assert.h>

/* pacman-g2 */
#include "stringlist.h"

#include "util.h"

/* Test for existence of a string in a pmlist_t
 */
int _pacman_list_is_strin(const char *needle, pmlist_t *haystack)
{
	pmlist_t *lp;

	for(lp = haystack; lp; lp = lp->next) {
		if(lp->data && !strcmp(lp->data, needle)) {
			return(1);
		}
	}
	return(0);
}

/* Filter out any duplicate strings in a list.
 *
 * Not the most efficient way, but simple to implement -- we assemble
 * a new list, using is_in() to check for dupes at each iteration.
 *
 */
pmlist_t *_pacman_list_remove_dupes(pmlist_t *list)
{
	pmlist_t *i, *newlist = NULL;

	for(i = list; i; i = i->next) {
		if(!_pacman_list_is_strin(i->data, newlist)) {
			newlist = _pacman_list_add(newlist, strdup(i->data));
		}
	}
	return newlist;
}

pmlist_t *_pacman_list_strdup(pmlist_t *list)
{
	pmlist_t *newlist = NULL;
	pmlist_t *lp;

	for(lp = list; lp; lp = lp->next) {
		newlist = _pacman_list_add(newlist, strdup(lp->data));
	}

	return(newlist);
}

/* vim: set ts=2 sw=2 noet: */