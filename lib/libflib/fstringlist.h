/*
 *  stringlist.h
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
#ifndef F_STRINGLIST_H
#define F_STRINGLIST_H

#include "flist.h"

typedef struct FStringListItem FStringListItem;

typedef struct FStringList FStringList;

void f_stringlist_delete (FPtrList *stringlist);

FPtrList *f_stringlist_add (FPtrList *list, const char *str);
FPtrList *f_stringlist_add_sorted (FPtrList *list, const char *str);
FPtrList *f_stringlist_append (FPtrList *list, const char *str);
FPtrList *f_stringlist_deep_copy (FPtrList *list);
void   f_stringlist_detach (FPtrList *list);
FPtrList *f_stringlist_find (FPtrList *list, const char *str);
char  *f_stringlist_join (FPtrList *list, const char *sep);
FPtrList *f_stringlist_remove_all (FPtrList *list, const char *str);
FPtrList *f_stringlist_uniques (FPtrList *list);

#endif /* F_STRINGLIST_H */

/* vim: set ts=2 sw=2 noet: */
