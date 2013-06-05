/*
 *  flistaccumulator.c
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

#include "config.h"

#include <assert.h>

#include "flistaccumulator.h"

#include "flist.h"

void f_ptrlistaccumulator_init (FPtrListAccumulator *listaccumulator, FPtrList *list) {
	listaccumulator->head = list;
	listaccumulator->last = f_ptrlist_last (list);
}

FPtrList *f_ptrlistaccumulator_fini (FPtrListAccumulator *listaccumulator) {
	FPtrList *ret = listaccumulator->head;

	listaccumulator->head = listaccumulator->last = NULL;
	return ret;
}

void f_ptrlistaccumulator_accumulate (FPtrListAccumulator *listaccumulator, void *data) {
	f_ptrlistaccumulate (data, listaccumulator);
}

void f_ptrlistaccumulator_reverse_accumulate (FPtrListAccumulator *listaccumulator, void *data) {
	f_ptrlistreverseaccumulate (data, listaccumulator);
}

void f_ptrlistaccumulate (void *data, FPtrListAccumulator *listaccumulator) {
	FPtrList *item = f_ptrlistitem_new (data);

	if (listaccumulator->head != NULL) {
		f_ptrlist_insert_after (listaccumulator->last, item);
		listaccumulator->last = listaccumulator->last->next;
	} else {
		listaccumulator->head = listaccumulator->last = item;
	}
}

void f_ptrlistreverseaccumulate (void *data, FPtrListAccumulator *listaccumulator) {
	FPtrList *item = f_ptrlistitem_new (data);

	if (listaccumulator->head != NULL) {
		f_ptrlist_insert_before (listaccumulator->head, item);
		listaccumulator->head = listaccumulator->head->prev;
	} else {
		listaccumulator->head = listaccumulator->last = item;
	}
}

/* vim: set ts=2 sw=2 noet: */
