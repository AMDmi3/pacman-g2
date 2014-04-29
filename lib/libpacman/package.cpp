/*
 *  package.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2005, 2006 by Christian Hamar <krics@linuxforum.hu>
 *  Copyright (c) 2005, 2006. 2007 by Miklos Vajna <vmiklos@frugalware.org>
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
#include "package.h"

#include "util.h"
#include "error.h"
#include "db.h"
#include "handle.h"
#include "cache.h"
#include "pacman.h"

#include "io/archive.h"
#include "util/list.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "fstdlib.h"
#include "fstring.h"

#include <sys/utsname.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>

using namespace libpacman;

Package::Package(Database *_database)
	: database(_database), reason(PM_PKG_REASON_EXPLICIT)
{
}

Package::Package(const char *name, const char *version)
	: reason(PM_PKG_REASON_EXPLICIT)
{
	if(!_pacman_strempty(name)) {
		STRNCPY(m_name, name, PKG_NAME_LEN);
	}
	if(!_pacman_strempty(version)) {
		STRNCPY(m_version, version, PKG_VERSION_LEN);
	}
}

Package::Package(const libpacman::Package &other)
{
	database       = other.database;
	STRNCPY(m_name,      other.m_name,      PKG_NAME_LEN);
	STRNCPY(m_version,   other.m_version,   PKG_VERSION_LEN);
	STRNCPY(desc,        other.desc,        PKG_DESC_LEN);
	STRNCPY(url,         other.url,         PKG_URL_LEN);
	STRNCPY(builddate,   other.builddate,   PKG_DATE_LEN);
	STRNCPY(buildtype,   other.buildtype,   PKG_DATE_LEN);
	STRNCPY(installdate, other.installdate, PKG_DATE_LEN);
	STRNCPY(packager,    other.packager,    PKG_PACKAGER_LEN);
	STRNCPY(md5sum,      other.md5sum,      PKG_MD5SUM_LEN);
	STRNCPY(sha1sum,     other.sha1sum,     PKG_SHA1SUM_LEN);
	STRNCPY(arch,        other.arch,        PKG_ARCH_LEN);
	size           = other.size;
	usize          = other.usize;
	force          = other.force;
	stick          = other.stick;
	scriptlet      = other.scriptlet;
	reason         = other.reason;
	license        = _pacman_list_strdup(other.license);
	desc_localized = _pacman_list_strdup(other.desc_localized);
	requiredby     = _pacman_list_strdup(other.requiredby);
	conflicts      = _pacman_list_strdup(other.conflicts);
	files          = _pacman_list_strdup(other.files);
	backup         = _pacman_list_strdup(other.backup);
	depends        = _pacman_list_strdup(other.depends);
	removes        = _pacman_list_strdup(other.removes);
	groups         = _pacman_list_strdup(other.groups);
	m_provides     = _pacman_list_strdup(other.m_provides);
	replaces       = _pacman_list_strdup(other.replaces);
	triggers       = _pacman_list_strdup(other.triggers);

	/* internal */
	origin         = other.origin;
	data           = (origin == PKG_FROM_FILE) ? strdup(other.data) : other.data;
	infolevel      = other.infolevel;
}

Package::~Package()
{
	FREELIST(license);
	FREELIST(desc_localized);
	FREELIST(files);
	FREELIST(backup);
	FREELIST(depends);
	FREELIST(removes);
	FREELIST(conflicts);
	FREELIST(requiredby);
	FREELIST(groups);
	FREELIST(m_provides);
	FREELIST(replaces);
	FREELIST(triggers);
	if(origin == PKG_FROM_FILE) {
		FREE(data);
	}
}

libpacman::Package *Package::dup() const
{
	return new Package(*this);
}

/*
static
unsigned int _pacman_pkg_parm_to_flag(unsigned char parm)
{
	switch(parm) {
	case PM_PKG_NAME:
		return PM_PACKAGE_FLAG_NAME;
	case PM_PKG_VERSION:
		return PM_PACKAGE_FLAG_VERSION;
	case PM_PKG_DESC:
		return PM_PACKAGE_FLAG_DESCRIPTION;
	case PM_PKG_URL:
		return PM_PACKAGE_FLAG_URL;
	case PM_PKG_BUILDDATE:
		return PM_PACKAGE_FLAG_BUILDDATE;
	case PM_PKG_BUILDTYPE:
		return PM_PACKAGE_FLAG_BUILDTYPE;
	case PM_PKG_INSTALLDATE:
		return PM_PACKAGE_FLAG_INSTALLDATE;
//	case PM_PKG_MD5SUM:
//	case PM_PKG_SHA1SUM:
//		return PM_PACKAGE_FLAG_HASH;
	case PM_PKG_ARCH:
		return PM_PACKAGE_FLAG_ARCH;
//	case PM_PKG_DESC:
//		return PM_PACKAGE_FLAG_LOCALISED_DESCRIPTION;
	case PM_PKG_LICENSE:
		return PM_PACKAGE_FLAG_LICENSE;
	case PM_PKG_REPLACES:
		return PM_PACKAGE_FLAG_REPLACES;
	case PM_PKG_GROUPS:
		return PM_PACKAGE_FLAG_GROUPS;
	case PM_PKG_FILES:
		return PM_PACKAGE_FLAG_FILES;
	case PM_PKG_BACKUP:
		return PM_PACKAGE_FLAG_BACKUP;
	case PM_PKG_DEPENDS:
		return PM_PACKAGE_FLAG_DEPENDS;
	case PM_PKG_REMOVES:
		return PM_PACKAGE_FLAG_REMOVES;
	case PM_PKG_REQUIREDBY:
		return PM_PACKAGE_FLAG_REQUIREDBY;
	case PM_PKG_CONFLICTS:
		return PM_PACKAGE_FLAG_CONFLITS;
	case PM_PKG_PROVIDES:
		return PM_PACKAGE_FLAG_PROVIDES;
	case PM_PKG_TRIGGERS:
		return PM_PACKAGE_FLAG_TRIGGERS;
	};
	return 0;
}
*/

bool Package::set_filename(const char *filename, int witharch)
{
	return Package::splitname(filename, m_name, m_version, witharch);
}

int _pacman_pkg_delete(Package *self)
{
	delete self;
	return 0;
}

int _pacman_pkg_fini(Package *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	FREELIST(self->license);
	FREELIST(self->desc_localized);
	FREELIST(self->files);
	FREELIST(self->backup);
	FREELIST(self->depends);
	FREELIST(self->removes);
	FREELIST(self->conflicts);
	FREELIST(self->requiredby);
	FREELIST(self->groups);
	FREELIST(self->m_provides);
	FREELIST(self->replaces);
	FREELIST(self->triggers);
	if(self->origin == PKG_FROM_FILE) {
		FREE(self->data);
	}
	return 0;
}

/* Helper function for comparing packages
 */
int _pacman_pkg_cmp(const void *p1, const void *p2)
{
	return(strcmp(((Package *)p1)->name(), ((Package *)p2)->name()));
}

int _pacman_pkg_is_valid(const Package *pkg, const pmtrans_t *trans, const char *pkgfile)
{
	struct utsname name;

	if(_pacman_strempty(pkg->m_name)) {
		_pacman_log(PM_LOG_ERROR, _("missing package name in %s"), pkgfile);
		goto pkg_error;
	}
	if(_pacman_strempty(pkg->m_version)) {
		_pacman_log(PM_LOG_ERROR, _("missing package version in %s"), pkgfile);
		goto pkg_error;
	}
	if(strchr(pkg->m_version, '-') != strrchr(pkg->m_version, '-')) {
		_pacman_log(PM_LOG_ERROR, _("version contains additional hyphens in %s"), pkgfile);
		goto invalid_name_error;
	}
	if (trans != NULL && !(trans->flags & PM_TRANS_FLAG_NOARCH)) {
		if(_pacman_strempty(pkg->arch)) {
			_pacman_log(PM_LOG_ERROR, _("missing package architecture in %s"), pkgfile);
			goto pkg_error;
		}

		uname (&name);
		if(strncmp(name.machine, pkg->arch, strlen(pkg->arch))) {
			_pacman_log(PM_LOG_ERROR, _("wrong package architecture in %s"), pkgfile);
			goto arch_error;
		}
	}
	return 0;

invalid_name_error:
	pm_errno = PM_ERR_PKG_INVALID_NAME;
	return -1;

arch_error:
	pm_errno = PM_ERR_WRONG_ARCH;
	return -1;

pkg_error:
	pm_errno = PM_ERR_PKG_INVALID;
	return -1;
}

/* Test for existence of a package in a pmlist_t*
 * of Package*
 */
Package *_pacman_pkg_isin(const char *needle, pmlist_t *haystack)
{
	pmlist_t *lp;

	if(needle == NULL || haystack == NULL) {
		return(NULL);
	}

	for(lp = haystack; lp; lp = lp->next) {
		Package *info = lp->data;

		if(info && !strcmp(info->name(), needle)) {
			return(lp->data);
		}
	}
	return(NULL);
}

bool Package::splitname(const char *target, char *name, char *version, int witharch)
{
	char *tmp;
	char *p, *q;

	if ((tmp = _pacman_basename(target)) == NULL) {
		return false;
	}
	/* trim file extension (if any) */
	if((p = strstr(tmp, PM_EXT_PKG))) {
		*p = 0;
	}
	if(witharch) {
		/* trim architecture */
		if((p = strrchr(tmp, '-'))) {
			*p = 0;
		}
	}

	p = tmp + strlen(tmp);

	for(q = --p; *q && *q != '-'; q--);
	if(*q != '-' || q == tmp) {
		return false;
	}
	for(p = --q; *p && *p != '-'; p--);
	if(*p != '-' || p == tmp) {
		return false;
	}
	if(version) {
		STRNCPY(version, p+1, PKG_VERSION_LEN);
	}
	*p = 0;

	if(name) {
		STRNCPY(name, tmp, PKG_NAME_LEN);
	}

	return true;
}

int Package::read(unsigned int flags)
{
	ASSERT(database != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	if(~this->flags & flags) {
		return database->read(this, flags);
	}
	return 0;
}

int Package::write(unsigned int flags)
{
	int ret;

	ASSERT(database != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	if((ret = database->write(this, flags)) != 0) {
		_pacman_log(PM_LOG_ERROR, _("could not update requiredby for database entry %s-%s"),
			name(), version());
	}
	return ret;
}

int Package::remove()
{
	ASSERT(database != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	return database->remove(this);
}

void *Package::getinfo(unsigned char parm)
{
	void *data = NULL;

#if 1
	/* Update the cache package entry if needed */
	if(origin == PKG_FROM_CACHE) {
		switch(parm) {
			/* Desc entry */
			case PM_PKG_DESC:
			case PM_PKG_GROUPS:
			case PM_PKG_URL:
			case PM_PKG_LICENSE:
			case PM_PKG_ARCH:
			case PM_PKG_BUILDDATE:
			case PM_PKG_INSTALLDATE:
			case PM_PKG_PACKAGER:
			case PM_PKG_SIZE:
			case PM_PKG_USIZE:
			case PM_PKG_REASON:
			case PM_PKG_MD5SUM:
			case PM_PKG_SHA1SUM:
			case PM_PKG_REPLACES:
			case PM_PKG_FORCE:
				if(!(infolevel & INFRQ_DESC)) {
					_pacman_log(PM_LOG_DEBUG, _("loading DESC info for '%s'"), name());
					database->read(this, INFRQ_DESC);
				}
			break;
			/* Depends entry */
			case PM_PKG_DEPENDS:
			case PM_PKG_REQUIREDBY:
			case PM_PKG_CONFLICTS:
			case PM_PKG_PROVIDES:
				if(!(infolevel & INFRQ_DEPENDS)) {
					_pacman_log(PM_LOG_DEBUG, "loading DEPENDS info for '%s'", name());
					database->read(this, INFRQ_DEPENDS);
				}
			break;
			/* Files entry */
			case PM_PKG_FILES:
			case PM_PKG_BACKUP:
				if(this->data == handle->db_local && !(infolevel & INFRQ_FILES)) {
					_pacman_log(PM_LOG_DEBUG, _("loading FILES info for '%s'"), name());
					database->read(this, INFRQ_FILES);
				}
			break;
			/* Scriptlet */
			case PM_PKG_SCRIPLET:
				if(this->data == handle->db_local && !(infolevel & INFRQ_SCRIPLET)) {
					_pacman_log(PM_LOG_DEBUG, _("loading SCRIPLET info for '%s'"), name());
					database->read(this, INFRQ_SCRIPLET);
				}
			break;
		}
	}
#else
	read(_pacman_pkg_parm_to_flag(parm));
#endif

	switch(parm) {
		case PM_PKG_NAME:        data = m_name; break;
		case PM_PKG_VERSION:     data = m_version; break;
		case PM_PKG_DESC:        data = desc; break;
		case PM_PKG_GROUPS:      data = groups; break;
		case PM_PKG_URL:         data = url; break;
		case PM_PKG_ARCH:        data = arch; break;
		case PM_PKG_BUILDDATE:   data = builddate; break;
		case PM_PKG_BUILDTYPE:   data = buildtype; break;
		case PM_PKG_INSTALLDATE: data = installdate; break;
		case PM_PKG_PACKAGER:    data = packager; break;
		case PM_PKG_SIZE:        data = (void *)(long)size; break;
		case PM_PKG_USIZE:       data = (void *)(long)usize; break;
		case PM_PKG_REASON:      data = (void *)(long)reason; break;
		case PM_PKG_LICENSE:     data = license; break;
		case PM_PKG_REPLACES:    data = replaces; break;
		case PM_PKG_FORCE:       data = (void *)(long)force; break;
		case PM_PKG_STICK:       data = (void *)(long)stick; break;
		case PM_PKG_MD5SUM:      data = md5sum; break;
		case PM_PKG_SHA1SUM:     data = sha1sum; break;
		case PM_PKG_DEPENDS:     data = depends; break;
		case PM_PKG_REMOVES:     data = removes; break;
		case PM_PKG_REQUIREDBY:  data = requiredby; break;
		case PM_PKG_PROVIDES:    data = m_provides; break;
		case PM_PKG_CONFLICTS:   data = conflicts; break;
		case PM_PKG_FILES:       data = files; break;
		case PM_PKG_BACKUP:      data = backup; break;
		case PM_PKG_SCRIPLET:    data = (void *)(long)scriptlet; break;
		case PM_PKG_DATA:        data = this->data; break;
		case PM_PKG_TRIGGERS:    data = triggers; break;
		default:
			data = NULL;
		break;
	}

	return(data);
}

pmlist_t *_pacman_pkg_getowners(const char *filename)
{
	struct stat buf;
	int gotcha = 0;
	char rpath[PATH_MAX];
	pmlist_t *lp, *ret = NULL;

	if(stat(filename, &buf) == -1 || realpath(filename, rpath) == NULL) {
		RET_ERR(PM_ERR_PKG_OPEN, NULL);
	}

	if(S_ISDIR(buf.st_mode)) {
		/* this is a directory and the db has a / suffix for dirs - add it here so
		 * that we'll find dirs, too */
		rpath[strlen(rpath)+1] = '\0';
		rpath[strlen(rpath)] = '/';
	}

	for(lp = _pacman_db_get_pkgcache(handle->db_local); lp; lp = lp->next) {
		Package *info;
		pmlist_t *i;

		info = lp->data;

		for(i = info->getinfo(PM_PKG_FILES); i; i = i->next) {
			char path[PATH_MAX];

			snprintf(path, PATH_MAX, "%s%s", handle->root, (char *)i->data);
			if(!strcmp(path, rpath)) {
				ret = _pacman_list_add(ret, info);
				if(rpath[strlen(rpath)-1] != '/') {
					/* we are searching for a file and multiple packages won't contain
					 * the same file */
					return(ret);
				}
				gotcha = 1;
			}
		}
	}
	if(!gotcha) {
		RET_ERR(PM_ERR_NO_OWNER, NULL);
	}

	return(ret);
}

int Package::filename(char *str, size_t size) const
{
	return snprintf(str, size, "%s-%s-%s%s",
			m_name, m_version, arch, PM_EXT_PKG);
}

/* Look for a filename in a Package.backup list.  If we find it,
 * then we return the md5 or sha1 hash (parsed from the same line)
 */
char *Package::fileneedbackup(const char *file) const
{
	const pmlist_t *lp;

	ASSERT(!_pacman_strempty(file), RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	/* run through the backup list and parse out the md5 or sha1 hash for our file */
	for(lp = backup; lp; lp = lp->next) {
		char *str = strdup(lp->data);
		char *ptr;

		/* tab delimiter */
		ptr = strchr(str, '\t');
		if(ptr == NULL) {
			free(str);
			continue;
		}
		*ptr = '\0';
		ptr++;
		/* now str points to the filename and ptr points to the md5 or sha1 hash */
		if(!strcmp(file, str)) {
			char *hash = strdup(ptr);
			free(str);
			return hash;
		}
		free(str);
	}

	return(NULL);
}

const char *Package::name()
{
	return getinfo(PM_PKG_NAME);
}

const char *Package::version()
{
	return getinfo(PM_PKG_VERSION);
}

FStringList *Package::provides() const
{
	return m_provides;
}

bool Package::provides(const char *pkgname)
{
	return _pacman_list_is_strin(pkgname, getinfo(PM_PKG_PROVIDES));
}

typedef struct FPackageStrMatcher FPackageStrMatcher;

struct FPackageStrMatcher
{
	int flags;
	const FStrMatcher *strmatcher;
};

static
int _pacman_packagestrmatcher_match(const void *ptr, const void *matcher_data) {
	const Package *pkg = ptr;
	const FPackageStrMatcher *data = matcher_data;
	const int flags = data->flags;
	const FStrMatcher *strmatcher = data->strmatcher;

#if 0
	/* Update the cache package entry if needed */
	if(pkg->origin == PKG_FROM_CACHE) {
		if(!(pkg->infolevel & INFRQ_DESC)) {
			_pacman_log(PM_LOG_DEBUG, _("loading DESC info for '%s'"), pkg->name);
			_pacman_db_read(pkg->data, (Package *)pkg, INFRQ_DESC);
		}
		if(!(pkg->infolevel & INFRQ_DEPENDS)) {
			_pacman_log(PM_LOG_DEBUG, "loading DEPENDS info for '%s'", pkg->name);
			_pacman_db_read(pkg->data, (Package *)pkg, INFRQ_DEPENDS);
		}
		if(pkg->data == handle->db_local && !(pkg->infolevel & INFRQ_FILES)) {
			_pacman_log(PM_LOG_DEBUG, _("loading FILES info for '%s'"), pkg->name);
			_pacman_db_read(pkg->data, (Package *)pkg, INFRQ_FILES);
		}
		if(pkg->data == handle->db_local && !(pkg->infolevel & INFRQ_SCRIPLET)) {
			_pacman_log(PM_LOG_DEBUG, _("loading SCRIPLET info for '%s'"), pkg->name);
			_pacman_db_read(pkg->data, (Package *)pkg, INFRQ_SCRIPLET);
		}
	}
#endif

	if(((flags & PM_PACKAGE_FLAG_NAME) && f_str_match(pkg->m_name, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_VERSION) && f_str_match(pkg->m_version, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_DESCRIPTION) && f_str_match(pkg->desc, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_BUILDDATE) && f_str_match(pkg->builddate, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_BUILDTYPE) && f_str_match(pkg->buildtype, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_INSTALLDATE) && f_str_match(pkg->installdate, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_PACKAGER) && f_str_match(pkg->packager, strmatcher)) ||
//			((flags & PM_PACKAGE_FLAG_HASH) && ) ||
			((flags & PM_PACKAGE_FLAG_ARCH) && f_str_match(pkg->arch, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_LOCALISED_DESCRIPTION) && f_stringlist_any_match(pkg->desc_localized, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_LICENSE) && f_stringlist_any_match(pkg->license, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_REPLACES) && f_stringlist_any_match(pkg->replaces, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_GROUPS) && f_stringlist_any_match(pkg->groups, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_FILES) && f_stringlist_any_match(pkg->files, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_BACKUP) && f_stringlist_any_match(pkg->backup, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_DEPENDS) && f_stringlist_any_match(pkg->depends, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_REMOVES) && f_stringlist_any_match(pkg->removes, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_REQUIREDBY) && f_stringlist_any_match(pkg->requiredby, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_CONFLITS) && f_stringlist_any_match(pkg->conflicts, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_PROVIDES) && f_stringlist_any_match(pkg->m_provides, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_TRIGGERS) && f_stringlist_any_match(pkg->triggers, strmatcher))) {
		return 1;
	}
	return 0;
}

int _pacman_packagestrmatcher_init(FMatcher *matcher, const FStrMatcher *strmatcher, int flags)
{
	FPackageStrMatcher *data = NULL;

	ASSERT(matcher != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(strmatcher != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT((data = f_zalloc(sizeof(*data))) != NULL, return -1);

	matcher->fn = _pacman_packagestrmatcher_match;
	matcher->data = data;
	data->strmatcher = strmatcher;
	return _pacman_packagestrmatcher_set_flags(matcher, flags);
}

int _pacman_packagestrmatcher_fini(FMatcher *matcher)
{
	FPackageStrMatcher *data = NULL;

	ASSERT(matcher != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(matcher->fn == _pacman_packagestrmatcher_match, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT((data = (FPackageStrMatcher *)matcher->data) != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	free(data);
	return 0;
}

int _pacman_packagestrmatcher_set_flags(FMatcher *matcher, int flags)
{
	FPackageStrMatcher *data = NULL;

	ASSERT(matcher != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(matcher->fn == _pacman_packagestrmatcher_match, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT((data = (FPackageStrMatcher *)matcher->data) != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	data->flags = flags;
	return 0;
}

/* vim: set ts=2 sw=2 noet: */
