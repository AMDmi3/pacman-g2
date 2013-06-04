/*
 *  list.h
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
#ifndef _PM_LIST_H
#define _PM_LIST_H

#include <pacman.h>

#include "flist.h"

/* Chained list struct */
typedef FList list_t;

#define FREELIST(p) do { if(p) { f_list_delete (p, (FVisitorFunc)free, NULL); p = NULL; } } while(0)
#define FREELISTPTR(p) do { if(p) { f_list_delete (p, NULL, NULL); p = NULL; } } while (0)

#define list_add _f_list_append
void list_display(const char *title, list_t *list);

#endif /* _PM_LIST_H */

/* vim: set ts=2 sw=2 noet: */
