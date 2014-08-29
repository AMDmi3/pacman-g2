/*
 *  fdispatchlogger.h
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
#ifndef F_DISPATCHLOGGER_H
#define F_DISPATCHLOGGER_H

#include <stdio.h>

#include "util/flogger.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct FDispatchLogger FDispatchLogger;

FDispatchLogger *f_dispatchlogger_new(unsigned char mask);
void f_dispatchlogger_delete(FDispatchLogger *dispatchlogger);

#ifdef __cplusplus
}
#endif
#endif /* F_DISPATCHLOGGER_H */

/* vim: set ts=2 sw=2 noet: */
