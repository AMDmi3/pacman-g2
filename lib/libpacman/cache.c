/*
 *  cache.c
 * 
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2007 by Miklos Vajna <vmiklos@frugalware.org>
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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <libintl.h>
/* pacman-g2 */
#include "log.h"
#include "pacman.h"
#include "list.h"
#include "util.h"
#include "package.h"
#include "group.h"
#include "db.h"
#include "cache.h"

/* Returns a new package cache from db.
 * It frees the cache if it already exists.
 */
int _pacman_db_load_pkgcache(pmdb_t *db)
{
	pmpkg_t *info;

	if(db == NULL) {
		return(-1);
	}

	_pacman_db_free_pkgcache(db);

	_pacman_log(PM_LOG_DEBUG, _("loading package cache (infolevel=%#x) for repository '%s'"),
	                        0, db->treename);

	_pacman_db_rewind(db);
	while((info = _pacman_db_scan(db, NULL, 0)) != NULL) {
		info->origin = PKG_FROM_CACHE;
		info->data = db;
		/* add to the collective */
		db->pkgcache = _pacman_list_add_sorted(db->pkgcache, info, _pacman_pkg_cmp);
	}

	return(0);
}

void _pacman_db_free_pkgcache(pmdb_t *db)
{
	if(db == NULL || db->pkgcache == NULL) {
		return;
	}

	_pacman_log(PM_LOG_DEBUG, _("freeing package cache for repository '%s'"),
	                        db->treename);

	FREELISTPKGS(db->pkgcache);

	if(db->grpcache) {
		_pacman_db_free_grpcache(db);
	}
}

pmlist_t *_pacman_db_get_pkgcache(pmdb_t *db)
{
	if(db == NULL) {
		return(NULL);
	}

	if(db->pkgcache == NULL) {
		_pacman_db_load_pkgcache(db);
	}

	return(db->pkgcache);
}

int _pacman_db_add_pkgincache(pmdb_t *db, pmpkg_t *pkg)
{
	pmpkg_t *newpkg;

	if(db == NULL || pkg == NULL) {
		return(-1);
	}

	newpkg = _pacman_pkg_dup(pkg);
	if(newpkg == NULL) {
		return(-1);
	}
	_pacman_log(PM_LOG_DEBUG, _("adding entry '%s' in '%s' cache"), newpkg->name, db->treename);
	db->pkgcache = _pacman_list_add_sorted(db->pkgcache, newpkg, _pacman_pkg_cmp);

	_pacman_db_free_grpcache(db);

	return(0);
}

int _pacman_db_remove_pkgfromcache(pmdb_t *db, pmpkg_t *pkg)
{
	pmpkg_t *data;

	if(db == NULL || pkg == NULL) {
		return(-1);
	}

	db->pkgcache = _pacman_list_remove(db->pkgcache, pkg, _pacman_pkg_cmp, (void **)&data);
	if(data == NULL) {
		/* package not found */
		return(-1);
	}

	_pacman_log(PM_LOG_DEBUG, _("removing entry '%s' from '%s' cache"), pkg->name, db->treename);
	FREEPKG(data);

	_pacman_db_free_grpcache(db);

	return(0);
}

pmpkg_t *_pacman_db_get_pkgfromcache(pmdb_t *db, char *target)
{
	if(db == NULL) {
		return(NULL);
	}

	return(_pacman_pkg_isin(target, _pacman_db_get_pkgcache(db)));
}

/* Returns a new group cache from db.
 */
int _pacman_db_load_grpcache(pmdb_t *db)
{
	pmlist_t *lp;

	if(db == NULL) {
		return(-1);
	}

	if(db->pkgcache == NULL) {
		_pacman_db_load_pkgcache(db);
	}

	_pacman_log(PM_LOG_DEBUG, _("loading group cache for repository '%s'"), db->treename);

	for(lp = db->pkgcache; lp; lp = lp->next) {
		pmlist_t *i;
		pmpkg_t *pkg = lp->data;

		if(!(pkg->infolevel & INFRQ_DESC)) {
			_pacman_db_read(pkg->data, INFRQ_DESC, pkg);
		}

		for(i = pkg->groups; i; i = i->next) {
			if(!_pacman_list_is_strin(i->data, db->grpcache)) {
				pmgrp_t *grp = _pacman_grp_new();

				STRNCPY(grp->name, (char *)i->data, GRP_NAME_LEN);
				grp->packages = _pacman_list_add_sorted(grp->packages, pkg->name, _pacman_grp_cmp);
				db->grpcache = _pacman_list_add_sorted(db->grpcache, grp, _pacman_grp_cmp);
			} else {
				pmlist_t *j;

				for(j = db->grpcache; j; j = j->next) {
					pmgrp_t *grp = j->data;

					if(strcmp(grp->name, i->data) == 0) {
						if(!_pacman_list_is_strin(pkg->name, grp->packages)) {
							grp->packages = _pacman_list_add_sorted(grp->packages, (char *)pkg->name, _pacman_grp_cmp);
						}
					}
				}
			}
		}
	}

	return(0);
}

void _pacman_db_free_grpcache(pmdb_t *db)
{
	pmlist_t *lg;

	if(db == NULL || db->grpcache == NULL) {
		return;
	}

	for(lg = db->grpcache; lg; lg = lg->next) {
		pmgrp_t *grp = lg->data;

		FREELISTPTR(grp->packages);
		FREEGRP(lg->data);
	}
	FREELIST(db->grpcache);
}

pmlist_t *_pacman_db_get_grpcache(pmdb_t *db)
{
	if(db == NULL) {
		return(NULL);
	}

	if(db->grpcache == NULL) {
		_pacman_db_load_grpcache(db);
	}

	return(db->grpcache);
}

pmgrp_t *_pacman_db_get_grpfromcache(pmdb_t *db, char *target)
{
	pmlist_t *i;

	if(db == NULL || target == NULL || strlen(target) == 0) {
		return(NULL);
	}

	for(i = _pacman_db_get_grpcache(db); i; i = i->next) {
		pmgrp_t *info = i->data;

		if(strcmp(info->name, target) == 0) {
			return(info);
		}
	}

	return(NULL);
}

/* vim: set ts=2 sw=2 noet: */