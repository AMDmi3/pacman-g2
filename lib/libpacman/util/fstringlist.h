/*
 *  fstringlist.h
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
#ifndef F_STRINGLIST_H
#define F_STRINGLIST_H

#include "util/fptrlist.h"
#include "fstring.h"

#ifndef __cplusplus
typedef struct FStringList FStringList;
typedef FPtrListIterator FStringListIterator;

#else
typedef class FStringList FStringList;
typedef FPtrListIterator FStringListIterator;

extern "C" {
#endif

int _pacman_list_is_strin(const char *needle, FStringList *haystack);
FStringList *_pacman_list_remove_dupes(FStringList *list);

const char *f_stringlistitem_to_str(const FStringListIterator *self);

int f_stringlist_delete(FStringList *self);

FStringList *f_stringlist_add(FStringList *list, const char *s);
FStringList *f_stringlist_add_stringlist(FStringList *dest, const FStringList *src);
int f_stringlist_clear(FStringList *self);

#ifdef __cplusplus
}

class FStringList
	: public FPtrList
{
public:
	FStringList();
	FStringList(const FStringList &o);
	FStringList(FStringList &&o);

	FStringList &operator = (const FStringList &o);
	FStringList &operator = (FStringList &&o);

	bool add(const char *s);
};

#endif
#endif /* F_STRINGLIST_H */

/* vim: set ts=2 sw=2 noet: */
