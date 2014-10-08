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

#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* pacman-g2 */
#include "cache.h"

#include "db/localdb_files.h"
#include "util/log.h"
#include "fstring.h"
#include "pacman.h"
#include "util.h"
#include "package.h"
#include "group.h"
#include "db.h"
#include "handle.h"
#include "error.h"

using namespace libpacman;

static
int _pacman_db_clear_grpcache(Database *db);

static
int _pacman_db_load_grpcache(Database *db);

/* Returns a new package cache from db.
 * It frees the cache if it already exists.
 */
static
int _pacman_db_load_pkgcache(Database *db)
{
	Package *info;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	_pacman_db_free_pkgcache(db);

	unsigned int inforeq = 0;
	if (db != db->m_handle->db_local)
		inforeq = INFRQ_DESC | INFRQ_DEPENDS;
	_pacman_log(PM_LOG_DEBUG, _("loading package cache (infolevel=%#x) for repository '%s'"),
	                        inforeq, db->treename());

	db->rewind();
	while((info = db->readpkg(inforeq)) != NULL) {
		/* add to the collective */
		db->pkgcache.add(info);
	}

	return(0);
}

void _pacman_db_free_pkgcache(Database *db)
{
	ASSERT(db != NULL, pm_errno = PM_ERR_DB_NULL; return);
	if(db->pkgcache.empty()) {
		return;
	}

	_pacman_log(PM_LOG_DEBUG, _("freeing package cache for repository '%s'"),
	                        db->treename());

	db->pkgcache.clear(/*_pacman_pkg_delete*/);

	_pacman_db_clear_grpcache(db);
}

libpacman::package_set &Database::get_packages()
{
	if(pkgcache.empty()) {
		_pacman_db_load_pkgcache(this);
	}
	return pkgcache;
}

int _pacman_db_add_pkgincache(Database *db, Package *pkg)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	if(pkg == NULL) {
		return(-1);
	}

	pkg->acquire(); // FIXME: Should not be necessary, but required during migration to refcounted object
	_pacman_log(PM_LOG_DEBUG, _("adding entry '%s' in '%s' cache"), pkg->name(), db->treename());
	db->pkgcache.add(pkg);

	_pacman_db_clear_grpcache(db);

	return(0);
}

int _pacman_db_remove_pkgfromcache(Database *db, Package *pkg)
{
	Package *data;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	if(pkg == NULL) {
		return(-1);
	}

	if(!db->pkgcache.remove(pkg, f_ptrcmp, &data)) {
		/* package not found */
		return(-1);
	}

	_pacman_log(PM_LOG_DEBUG, _("removing entry '%s' from '%s' cache"), pkg->name(), db->treename());
	data->release(); // FIXME: Should not be necessary, but required during migration to refcounted object

	_pacman_db_clear_grpcache(db);

	return(0);
}

static
Group *_pacman_db_get_grpfromlist(const FList<Group *> &list, const char *target)
{
	if(_pacman_strempty(target)) {
		return(NULL);
	}

	for(auto i = list.begin(), end = list.end(); i != end; ++i) {
		Group *info = *i;

		if(strcmp(info->name, target) == 0) {
			return(info);
		}
	}

	return(NULL);
}

/* Returns a new group cache from db.
 */
int _pacman_db_load_grpcache(Database *db)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	auto &cache = db->get_packages();

	_pacman_log(PM_LOG_DEBUG, _("loading group cache for repository '%s'"), db->treename());

	for(auto it = cache.begin(), end = cache.end(); it != end; ++it) {
		Package *pkg = *it;

		if(!(pkg->flags & INFRQ_DESC)) {
			pkg->read(INFRQ_DESC);
		}

		auto &groups = pkg->groups();
		for(auto git = groups.begin(), git_end = groups.end(); git != git_end; ++git) {
			const char *grp_name = *git;

			Group *grp = _pacman_db_get_grpfromlist(db->grpcache, grp_name);

			if(grp == NULL) {
				grp = new Group(grp_name);
				db->grpcache.add(grp);
			}
			if(!grp->packages.contains(pkg->name())) {
				grp->packages.add(pkg->name());
			}
		}
	}

	return(0);
}

int _pacman_db_clear_grpcache(Database *db)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	db->grpcache.clear();
	return 0;
}

libpacman::group_set &Database::get_groups()
{
	if(grpcache.empty()) {
		_pacman_db_load_grpcache(this);
	}
	return grpcache;
}

Group *_pacman_db_get_grpfromcache(Database *db, const char *target)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));

	return _pacman_db_get_grpfromlist(db->get_groups(), target);
}

/* vim: set ts=2 sw=2 noet: */