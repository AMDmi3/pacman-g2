/*
 *  download.h
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
#ifndef _PM_DOWNLOAD_H
#define _PM_DOWNLOAD_H

#define DLFNM_PROGRESS_LEN 22

extern char sync_fnm[PM_DLFNM_LEN+1];
extern int offset;
extern struct timeval t0, t;
extern float rate;
extern int xfered1;
extern unsigned int eta_h, eta_m, eta_s, remain, howmany;

int log_progress(PM_NETBUF *ctl, int xfered, void *arg);

#endif /* _PM_DOWNLOAD_H */

/* vim: set ts=2 sw=2 noet: */
