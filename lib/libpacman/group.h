/*
 *  group.h
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
#ifndef _PACMAN_GROUP_H
#define _PACMAN_GROUP_H

#include "list.h"

#define GRP_NAME_LEN 256

/* Groups structure */
typedef struct __pmgrp_t {
	char name[GRP_NAME_LEN];
	pmlist_t *packages; /* List of strings */
} pmgrp_t;

#define FREEGRP(p) do { if(p) { _pacman_grp_free(p); p = NULL; } } while(0)

#define FREELISTGRPS(p) _FREELIST(p, _pacman_grp_free)

pmgrp_t *_pacman_grp_new(void);
void _pacman_grp_free(void *data);
int _pacman_grp_cmp(const void *g1, const void *g2);

#endif /* _PACMAN_GROUP_H */

/* vim: set ts=2 sw=2 noet: */
