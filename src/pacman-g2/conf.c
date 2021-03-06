/*
 *  conf.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <libintl.h>

#include <pacman.h>
/* pacman-g2 */
#include "util.h"
#include "log.h"
#include "list.h"
#include "sync.h"
#include "download.h"
#include "conf.h"

extern list_t *pmc_syncs;

config_t *config_new()
{
	config_t *config;

	MALLOC(config, sizeof(config_t));

	memset(config, 0, sizeof(config_t));

	return(config);
}

int config_free(config_t *config)
{
	if(config == NULL) {
		return(-1);
	}

	FREE(config->root);
	FREE(config->configfile);
	FREELIST(config->op_s_ignore);
	free(config);

	return(0);
}

void cb_db_register(const char *section, PM_DB *db)
{
	pmc_syncs = list_add(pmc_syncs, db);
}

/* vim: set ts=2 sw=2 noet: */
