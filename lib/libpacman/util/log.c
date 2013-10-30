/*
 *  log.c
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
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <time.h>

/* pacman-g2 */
#include "log.h"

#include "pacman.h"

/* Internal library log mechanism */
pacman_cb_log pm_logcb     = NULL;
unsigned char pm_logmask = 0;

void _pacman_log(unsigned char flag, const char *fmt, ...)
{
	if(pm_logcb == NULL) {
		return;
	}

	if(flag & pm_logmask) {
		char str[LOG_STR_LEN];
		va_list args;

		va_start(args, fmt);
		vsnprintf(str, LOG_STR_LEN, fmt, args);
		va_end(args);

		pm_logcb(flag, str);
		pacman_logaction(str);
	}
}

int _pacman_logaction(unsigned char usesyslog, FILE *f, char *fmt, ...)
{
	char msg[1024];
	int smsg = sizeof(msg)-1;
	va_list args;

	va_start(args, fmt);
	vsnprintf(msg, smsg, fmt, args);
	va_end(args);

	if(usesyslog) {
		syslog(LOG_WARNING, "%s", msg);
	}

	if(f) {
		time_t t;
		struct tm *tm;

		t = time(NULL);
		tm = localtime(&t);

		fprintf(f, "[%02d/%02d/%02d %02d:%02d] %s\n",
		        tm->tm_mon+1, tm->tm_mday, tm->tm_year-100,
		        tm->tm_hour, tm->tm_min,
		        _pacman_strtrim(msg));
		fflush(f);
	}

	return(0);
}


/* vim: set ts=2 sw=2 noet: */
