/*
 *  db.c
 * 
 *  Copyright (c) 2002-2005 by Judd Vinet <jvinet@zeroflux.org>
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
/* pacman */
#include "util.h"
#include "group.h"
#include "cache.h"
#include "db.h"

/* Open a database and return a pmdb_t handle */
pmdb_t *db_open(char *root, char *dbpath, char *treename)
{
	pmdb_t *db;

	if(root == NULL || dbpath == NULL || treename == NULL) {
		return(NULL);
	}

	MALLOC(db, sizeof(pmdb_t));

	MALLOC(db->path, strlen(root)+strlen(dbpath)+strlen(treename)+2);
	sprintf(db->path, "%s%s/%s", root, dbpath, treename);

	db->dir = opendir(db->path);
	if(db->dir == NULL) {
		FREE(db->path);
		FREE(db);
		return(NULL);
	}

	strncpy(db->treename, treename, sizeof(db->treename)-1);

	db->pkgcache = NULL;
	db->grpcache = NULL;

	return(db);
}

void db_close(pmdb_t *db)
{
	if(db == NULL) {
		return;
	}

	if(db->dir) {
		closedir(db->dir);
		db->dir = NULL;
	}
	FREE(db->path);

	db_free_pkgcache(db);
	db_free_grpcache(db);

	free(db);

	return;
}

int db_create(char *root, char *dbpath, char *treename)
{
	char path[PATH_MAX];

	if(root == NULL || dbpath == NULL || treename == NULL) {
		return(-1);
	}

	snprintf(path, PATH_MAX, "%s%s/local", root, dbpath);
	if(_alpm_makepath(path) != 0) {
		return(-1);
	}

	return(0);
}

int db_update(char *root, char *dbpath, char *treename, char *archive)
{
	char ldir[PATH_MAX];

	snprintf(ldir, PATH_MAX, "%s%s/%s", root, dbpath, treename);
	/* remove the old dir */
	/* ORE - do we want to include alpm.h and use the log mechanism from db.c?
	_alpm_log(PM_LOG_FLOW2, "removing %s (if it exists)\n", ldir);*/
	/* ORE 
	We should only rmrf the database content, and not the top directory, in case
	a (DIR *) structure is associated with it (i.e a call to db_open). */
	_alpm_rmrf(ldir);

	/* make the new dir */
	if(db_create(root, dbpath, treename) != 0) {
		return(-1);
	}

	/* uncompress the sync database */
	/* ORE
	_alpm_log(PM_LOG_FLOW2, "Unpacking %s...\n", archive);*/
	if(_alpm_unpack(archive, ldir, NULL)) {
		return(-1);
	}

	/* ORE
	Should we let the the library manage updates only if needed?
	Create a .lastupdate file in ldir? Ask for a timestamp as db_update argument? */

	return(0);
}

void db_rewind(pmdb_t *db)
{
	if(db == NULL || db->dir == NULL) {
		return;
	}

	rewinddir(db->dir);
}

pmpkg_t *db_scan(pmdb_t *db, char *target, unsigned int inforeq)
{
	struct dirent *ent = NULL;
	char name[256];
	char *ptr = NULL;
	int ret, found = 0;
	pmpkg_t *pkg;

	if(db == NULL) {
		return(NULL);
	}

	if(target != NULL) {
		/* search for a specific package (by name only) */
		rewinddir(db->dir);
		while(!found && (ent = readdir(db->dir)) != NULL) {
			if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
				continue;
			}
			strncpy(name, ent->d_name, 255);
			/* truncate the string at the second-to-last hyphen, */
			/* which will give us the package name */
			if((ptr = rindex(name, '-'))) {
				*ptr = '\0';
			}
			if((ptr = rindex(name, '-'))) {
				*ptr = '\0';
			}
			if(!strcmp(name, target)) {
				found = 1;
			}
		}
		if(!found) {
			return(NULL);
		}
	} else {
		/* normal iteration */
		ent = readdir(db->dir);
		if(ent == NULL) {
			return(NULL);
		}
		if(!strcmp(ent->d_name, ".")) {
			ent = readdir(db->dir);
			if(ent == NULL) {
				return(NULL);
			}
		}
		if(!strcmp(ent->d_name, "..")) {
			ent = readdir(db->dir);
			if(ent == NULL) {
				return(NULL);
			}
		}
	}

	pkg = pkg_new();
	if(pkg == NULL) {
		return(NULL);
	}
	ret = db_read(db, ent->d_name, inforeq, pkg);
	if(ret == -1) {
		FREEPKG(pkg);
	}

	return(ret == 0 ? pkg : NULL);
}

int db_read(pmdb_t *db, char *name, unsigned int inforeq, pmpkg_t *info)
{
	FILE *fp = NULL;
	struct stat buf;
	char path[PATH_MAX];
	char line[512];

	if(db == NULL || name == NULL || info == NULL) {
		return(-1);
	}

	snprintf(path, PATH_MAX, "%s/%s", db->path, name);
	if(stat(path, &buf)) {
		/* directory doesn't exist or can't be opened */
		return(-1);
	}

	/* DESC */
	if(inforeq & INFRQ_DESC) {
		snprintf(path, PATH_MAX, "%s/%s/desc", db->path, name);
		fp = fopen(path, "r");
		if(fp == NULL) {
			fprintf(stderr, "error: %s: %s\n", path, strerror(errno));
			return(-1);
		}
		while(!feof(fp)) {
			if(fgets(line, 256, fp) == NULL) {
				break;
			}
			_alpm_strtrim(line);
			if(!strcmp(line, "%NAME%")) {
				if(fgets(info->name, sizeof(info->name), fp) == NULL) {
					return(-1);
				}
				_alpm_strtrim(info->name);
			} else if(!strcmp(line, "%VERSION%")) {
				if(fgets(info->version, sizeof(info->version), fp) == NULL) {
					return(-1);
				}
				_alpm_strtrim(info->version);
			} else if(!strcmp(line, "%DESC%")) {
				if(fgets(info->desc, sizeof(info->desc), fp) == NULL) {
					return(-1);
				}
				_alpm_strtrim(info->desc);
			} else if(!strcmp(line, "%GROUPS%")) {
				while(fgets(line, 512, fp) && strlen(_alpm_strtrim(line))) {
					char *s = strdup(line);
					info->groups = pm_list_add(info->groups, s);
				}
			} else if(!strcmp(line, "%URL%")) {
				if(fgets(info->url, sizeof(info->url), fp) == NULL) {
					return(-1);
				}
				_alpm_strtrim(info->url);
			} else if(!strcmp(line, "%LICENSE%")) {
				if(fgets(info->license, sizeof(info->license), fp) == NULL) {
					return(-1);
				}
				_alpm_strtrim(info->license);
			} else if(!strcmp(line, "%ARCH%")) {
				if(fgets(info->arch, sizeof(info->arch), fp) == NULL) {
					return(-1);
				}
				_alpm_strtrim(info->arch);
			} else if(!strcmp(line, "%BUILDDATE%")) {
				if(fgets(info->builddate, sizeof(info->builddate), fp) == NULL) {
					return(-1);
				}
				_alpm_strtrim(info->builddate);
			} else if(!strcmp(line, "%INSTALLDATE%")) {
				if(fgets(info->installdate, sizeof(info->installdate), fp) == NULL) {
					return(-1);
				}
				_alpm_strtrim(info->installdate);
			} else if(!strcmp(line, "%PACKAGER%")) {
				if(fgets(info->packager, sizeof(info->packager), fp) == NULL) {
					return(-1);
				}
				_alpm_strtrim(info->packager);
			} else if(!strcmp(line, "%REASON%")) {
				char tmp[32];
				if(fgets(tmp, sizeof(tmp), fp) == NULL) {
					return(-1);
				}
				_alpm_strtrim(tmp);
				info->reason = atol(tmp);
			} else if(!strcmp(line, "%SIZE%")) {
				char tmp[32];
				if(fgets(tmp, sizeof(tmp), fp) == NULL) {
					return(-1);
				}
				_alpm_strtrim(tmp);
				info->size = atol(tmp);
			} else if(!strcmp(line, "%CSIZE%")) {
				/* NOTE: the CSIZE and SIZE fields both share the "size" field
				 *       in the pkginfo_t struct.  This can be done b/c CSIZE
				 *       is currently only used in sync databases, and SIZE is
				 *       only used in local databases.
				 */
				char tmp[32];
				if(fgets(tmp, sizeof(tmp), fp) == NULL) {
					return(-1);
				}
				_alpm_strtrim(tmp);
				info->size = atol(tmp);
			} else if(!strcmp(line, "%REPLACES%")) {
				/* the REPLACES tag is special -- it only appears in sync repositories,
				 * not the local one. */
				while(fgets(line, 512, fp) && strlen(_alpm_strtrim(line))) {
					info->replaces = pm_list_add(info->replaces, strdup(line));
				}
			} else if(!strcmp(line, "%MD5SUM%")) {
				/* MD5SUM tag only appears in sync repositories,
				 * not the local one. */
				if(fgets(info->md5sum, sizeof(info->md5sum), fp) == NULL) {
					return(-1);
				}
			} else if(!strcmp(line, "%FORCE%")) {
				/* FORCE tag only appears in sync repositories,
				 * not the local one. */
				info->force = 1;
			}
		}
		fclose(fp);
	}

	/* FILES */
	if(inforeq & INFRQ_FILES) {
		snprintf(path, PATH_MAX, "%s/%s/files", db->path, name);
		fp = fopen(path, "r");
		if(fp == NULL) {
			fprintf(stderr, "error: %s: %s\n", path, strerror(errno));
			return(-1);
		}
		while(fgets(line, 256, fp)) {
			_alpm_strtrim(line);
			if(!strcmp(line, "%FILES%")) {
				while(fgets(line, 512, fp) && strlen(_alpm_strtrim(line))) {
					info->files = pm_list_add(info->files, strdup(line));
				}
			} else if(!strcmp(line, "%BACKUP%")) {
				while(fgets(line, 512, fp) && strlen(_alpm_strtrim(line))) {
					info->backup = pm_list_add(info->backup, strdup(line));
				}
			}
		}
		fclose(fp);
	}

	/* DEPENDS */
	if(inforeq & INFRQ_DEPENDS) {
		snprintf(path, PATH_MAX, "%s/%s/depends", db->path, name);
		fp = fopen(path, "r");
		if(fp == NULL) {
			fprintf(stderr, "error: %s: %s\n", path, strerror(errno));
			return(-1);
		}
		while(!feof(fp)) {
			fgets(line, 255, fp);
			_alpm_strtrim(line);
			if(!strcmp(line, "%DEPENDS%")) {
				while(fgets(line, 512, fp) && strlen(_alpm_strtrim(line))) {
					info->depends = pm_list_add(info->depends, strdup(line));
				}
			} else if(!strcmp(line, "%REQUIREDBY%")) {
				while(fgets(line, 512, fp) && strlen(_alpm_strtrim(line))) {
					info->requiredby = pm_list_add(info->requiredby, strdup(line));
				}
			} else if(!strcmp(line, "%CONFLICTS%")) {
				while(fgets(line, 512, fp) && strlen(_alpm_strtrim(line))) {
					info->conflicts = pm_list_add(info->conflicts, strdup(line));
				}
			} else if(!strcmp(line, "%PROVIDES%")) {
				while(fgets(line, 512, fp) && strlen(_alpm_strtrim(line))) {
					info->provides = pm_list_add(info->provides, strdup(line));
				}
			}
		}
		fclose(fp);
	}

	/* INSTALL */
	if(inforeq & INFRQ_SCRIPLET) {
		snprintf(path, PATH_MAX, "%s/%s/install", db->path, name);
		if(!stat(path, &buf)) {
			info->scriptlet = 1;
		}
	}

	/* internal */
	info->infolevel |= inforeq;

	return(0);
}

int db_write(pmdb_t *db, pmpkg_t *info, unsigned int inforeq)
{
	char topdir[PATH_MAX];
	FILE *fp = NULL;
	char path[PATH_MAX];
	mode_t oldmask;
	PMList *lp = NULL;

	if(db == NULL || info == NULL) {
		return(-1);
	}

	snprintf(topdir, PATH_MAX, "%s/%s-%s", db->path,
		info->name, info->version);
	oldmask = umask(0000);
	mkdir(topdir, 0755);
	/* make sure we have a sane umask */
	umask(0022);

	/* DESC */
	if(inforeq & INFRQ_DESC) {
		snprintf(path, PATH_MAX, "%s/desc", topdir);
		if((fp = fopen(path, "w")) == NULL) {
			perror("db_write");
			umask(oldmask);
			return(-1);
		}
		fputs("%NAME%\n", fp);
		fprintf(fp, "%s\n\n", info->name);
		fputs("%VERSION%\n", fp);
		fprintf(fp, "%s\n\n", info->version);
		fputs("%DESC%\n", fp);
		fprintf(fp, "%s\n\n", info->desc);
		fputs("%GROUPS%\n", fp);
		for(lp = info->groups; lp; lp = lp->next) {
			fprintf(fp, "%s\n", (char*)lp->data);
		}
		fprintf(fp, "\n");
		fputs("%URL%\n", fp);
		fprintf(fp, "%s\n\n", info->url);
		fputs("%LICENSE%\n", fp);
		fprintf(fp, "%s\n\n", info->license);
		fputs("%ARCH%\n", fp);
		fprintf(fp, "%s\n\n", info->arch);
		fputs("%BUILDDATE%\n", fp);
		fprintf(fp, "%s\n\n", info->builddate);
		fputs("%INSTALLDATE%\n", fp);
		fprintf(fp, "%s\n\n", info->installdate);
		fputs("%PACKAGER%\n", fp);
		fprintf(fp, "%s\n\n", info->packager);
		fputs("%SIZE%\n", fp);
		fprintf(fp, "%ld\n\n", info->size);
		fputs("%REASON%\n", fp);
		fprintf(fp, "%ld\n\n", info->size);
		fclose(fp);
	}

	/* FILES */
	if(inforeq & INFRQ_FILES) {
		snprintf(path, PATH_MAX, "%s/files", topdir);
		if((fp = fopen(path, "w")) == NULL) {
			perror("db_write");
			umask(oldmask);
			return(-1);
		}
		fputs("%FILES%\n", fp);
		for(lp = info->files; lp; lp = lp->next) {
			fprintf(fp, "%s\n", (char*)lp->data);
		}
		fprintf(fp, "\n");
		fputs("%BACKUP%\n", fp);
		for(lp = info->backup; lp; lp = lp->next) {
			fprintf(fp, "%s\n", (char*)lp->data);
		}
		fprintf(fp, "\n");
		fclose(fp);
	}

	/* DEPENDS */
	if(inforeq & INFRQ_DEPENDS) {
		snprintf(path, PATH_MAX, "%s/depends", topdir);
		if((fp = fopen(path, "w")) == NULL) {
			perror("db_write");
			umask(oldmask);
			return(-1);
		}
		fputs("%DEPENDS%\n", fp);
		for(lp = info->depends; lp; lp = lp->next) {
			fprintf(fp, "%s\n", (char*)lp->data);
		}
		fprintf(fp, "\n");
		fputs("%REQUIREDBY%\n", fp);
		for(lp = info->requiredby; lp; lp = lp->next) {
			fprintf(fp, "%s\n", (char*)lp->data);
		}
		fprintf(fp, "\n");
		fputs("%CONFLICTS%\n", fp);
		for(lp = info->conflicts; lp; lp = lp->next) {
			fprintf(fp, "%s\n", (char*)lp->data);
		}
		fprintf(fp, "\n");
		fputs("%PROVIDES%\n", fp);
		for(lp = info->provides; lp; lp = lp->next) {
			fprintf(fp, "%s\n", (char*)lp->data);
		}
		fprintf(fp, "\n");
		fclose(fp);
	}

	/* INSTALL */
	/* nothing needed here (script is automatically extracted) */

	umask(oldmask);

	return(0);
}

int db_remove(pmdb_t *db, pmpkg_t *info)
{
	char topdir[PATH_MAX];
	char file[PATH_MAX];

	if(db == NULL || info == NULL) {
		return(-1);
	}

	snprintf(topdir, PATH_MAX, "%s/%s-%s", db->path, info->name, info->version);

	/* DESC */
	snprintf(file, PATH_MAX, "%s/desc", topdir);
	unlink(file);
	/* FILES */
	snprintf(file, PATH_MAX, "%s/files", topdir);
	unlink(file);
	/* DEPENDS */
	snprintf(file, PATH_MAX, "%s/depends", topdir);
	unlink(file);
	/* INSTALL */
	snprintf(file, PATH_MAX, "%s/install", topdir);
	unlink(file);
	/* Package directory */
	if(rmdir(topdir) == -1) {
		return(-1);
	}

	return(0);
}

PMList *db_find_conflicts(pmdb_t *db, PMList *targets, char *root)
{
	PMList *i, *j, *k;
	char *filestr = NULL;
	char path[PATH_MAX+1];
	char *str = NULL;
	struct stat buf, buf2;
	PMList *conflicts = NULL;

	if(db == NULL || targets == NULL || root == NULL) {
		return(NULL);
	}

	/* CHECK 1: check every db package against every target package */
	/* XXX: I've disabled the database-against-targets check for now, as the
	 *      many many strcmp() calls slow it down heavily and most of the
	 *      checking is redundant to the targets-against-filesystem check.
	 *      This will be re-enabled if I can improve performance significantly.
	 *
	pmpkg_t *info = NULL;
	char *dbstr   = NULL;
	rewinddir(db->dir);
	while((info = db_scan(db, NULL, INFRQ_DESC | INFRQ_FILES)) != NULL) {
		for(i = info->files; i; i = i->next) {
			if(i->data == NULL) continue;
			dbstr = (char*)i->data;
			for(j = targets; j; j = j->next) {
				pmpkg_t *targ = (pmpkg_t*)j->data;
				if(strcmp(info->name, targ->name)) {
					for(k = targ->files; k; k = k->next) {
						filestr = (char*)k->data;
						if(!strcmp(dbstr, filestr)) {
							if(rindex(k->data, '/') == filestr+strlen(filestr)-1) {
								continue;
							}
							MALLOC(str, 512);
							snprintf(str, 512, "%s: exists in \"%s\" (target) and \"%s\" (installed)", dbstr,
								targ->name, info->name);
							conflicts = pm_list_add(conflicts, str);
						}
					}
				}
			}
		}
	}*/

	/* CHECK 2: check every target against every target */
	for(i = targets; i; i = i->next) {
		pmpkg_t *p1 = (pmpkg_t*)i->data;
		for(j = i; j; j = j->next) {
			pmpkg_t *p2 = (pmpkg_t*)j->data;
			if(strcmp(p1->name, p2->name)) {
				for(k = p1->files; k; k = k->next) {
					filestr = k->data;
					if(!strcmp(filestr, "._install") || !strcmp(filestr, ".INSTALL")) {
						continue;
					}
					if(rindex(filestr, '/') == filestr+strlen(filestr)-1) {
						/* this filename has a trailing '/', so it's a directory -- skip it. */
						continue;
					}
					if(pm_list_is_strin(filestr, p2->files)) {
						MALLOC(str, 512);
						snprintf(str, 512, "%s: exists in \"%s\" (target) and \"%s\" (target)",
							filestr, p1->name, p2->name);
						conflicts = pm_list_add(conflicts, str);
					}
				}
			}
		}
	}

	/* CHECK 3: check every target against the filesystem */
	for(i = targets; i; i = i->next) {
		pmpkg_t *p = (pmpkg_t*)i->data;
		pmpkg_t *dbpkg = NULL;
		for(j = p->files; j; j = j->next) {
			filestr = (char*)j->data;
			snprintf(path, PATH_MAX, "%s%s", root, filestr);
			if(!stat(path, &buf) && !S_ISDIR(buf.st_mode)) {
				int ok = 0;
				if(dbpkg == NULL) {
					dbpkg = db_scan(db, p->name, INFRQ_DESC | INFRQ_FILES);
				}
				if(dbpkg && pm_list_is_strin(j->data, dbpkg->files)) {
					ok = 1;
				}
				/* Make sure that the supposedly-conflicting file is not actually just
				 * a symlink that points to a path that used to exist in the package.
				 */
				/* Check if any part of the conflicting file's path is a symlink */
				if(dbpkg && !ok) {
					char str[PATH_MAX];
					for(k = dbpkg->files; k; k = k->next) {
						snprintf(str, PATH_MAX, "%s%s", root, (char*)k->data);
						stat(str, &buf2);
						if(buf.st_ino == buf2.st_ino) {
							ok = 1;
						}
					}
				}
				/* Check if the conflicting file has been moved to another package/target */
				if(!ok) {
					/* Look at all the targets */
					for(k = targets; k && !ok; k = k->next) {
						pmpkg_t *p1 = (pmpkg_t *)k->data;
						/* As long as they're not the current package */
						if(strcmp(p1->name, p->name)) {
							pmpkg_t *dbpkg2 = NULL;
							dbpkg2 = db_scan(db, p1->name, INFRQ_DESC | INFRQ_FILES);
							/* If it used to exist in there, but doesn't anymore */
							if(dbpkg2 && !pm_list_is_strin(filestr, p1->files) && pm_list_is_strin(filestr, dbpkg2->files)) {
								ok = 1;
							}
							FREEPKG(dbpkg2);
						}
					}
				}
				if(!ok) {
					MALLOC(str, 512);
					snprintf(str, 512, "%s: exists in filesystem", path);
					conflicts = pm_list_add(conflicts, str);
				}
			}
		}
		FREEPKG(dbpkg);
	}

	return(conflicts);
}

/* vim: set ts=2 sw=2 noet: */
