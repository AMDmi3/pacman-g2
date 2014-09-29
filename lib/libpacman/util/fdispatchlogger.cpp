/*
 *  fdispatchlogger.c
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

/* pacman-g2 */
#include "util/fdispatchlogger.h"

#include "util/fptrlist.h"

static
void f_dispatchlogger_log(unsigned char flag, const char *message, void *data)
{
	FPtrList *list = (FPtrList *)data;

	for (auto it = f_ptrlist_first(list), end = f_ptrlist_end(list); it != end; it = it->next()) {
		((FAbstractLogger *)f_ptrlistitem_data(it))->logs(flag, message);
	}
}

FDispatchLogger::FDispatchLogger(unsigned char mask)
	: FAbstractLogger(mask, f_dispatchlogger_log, f_ptrlist_new())
{ }

FDispatchLogger::~FDispatchLogger()
{ }

/* vim: set ts=2 sw=2 noet: */
