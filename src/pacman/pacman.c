/*
 *  pacman.c
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
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#ifndef CYGWIN
#include <mcheck.h> /* debug */
#else
#include <libgen.h> /* basename */
#endif

#include <alpm.h>
/* pacman */
#include "list.h"
#include "util.h"
#include "log.h"
#include "download.h"
#include "conf.h"
#include "package.h"
#include "add.h"
#include "remove.h"
#include "upgrade.h"
#include "query.h"
#include "sync.h"
#include "pacman.h"

config_t *config = NULL;

PM_DB *db_local;
/* list of (sync_t *) structs for sync locations */
list_t *pmc_syncs = NULL;
/* list of targets specified on command line */
list_t *pm_targets  = NULL;

int maxcols = 80;

int main(int argc, char *argv[])
{
	int ret = 0;
	char *cenv = NULL;
	uid_t myuid;

#ifndef CYGWIN
	/* debug */
	mtrace();
#endif

	cenv = getenv("COLUMNS");
	if(cenv != NULL) {
		maxcols = atoi(cenv);
	}

	if(argc < 2) {
		usage(PM_OP_MAIN, basename(argv[0]));
		return(0);
	}

	/* set signal handlers */
	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);

	/* init config data */
	config = config_new();
	if(config == NULL) {
		ERR(NL, "could not allocate memory for pacman config data.\n");
		return(1);
	}
  config->op    = PM_OP_MAIN;
  config->debug |= PM_LOG_WARNING | PM_LOG_ERROR;
  config->verbose = 1;

	/* initialize pm library */
	if(alpm_initialize(config->root) == -1) {
		ERR(NL, "failed to initilize alpm library (%s)\n", alpm_strerror(pm_errno));
		cleanup(1);
	}

	/* parse the command line */
	ret = parseargs(argc, argv);
	if(ret != 0) {
		config_free(config);
		exit(ret);
	}

	/* see if we're root or not */
	myuid = geteuid();
	if(!myuid && getenv("FAKEROOTKEY")) {
		/* fakeroot doesn't count, we're non-root */
		myuid = 99;
	}

	/* check if we have sufficient permission for the requested operation */
	if(myuid > 0) {
		if(config->op != PM_OP_MAIN && config->op != PM_OP_QUERY && config->op != PM_OP_DEPTEST) {
			if((config->op == PM_OP_SYNC && !config->op_s_sync &&
					(config->op_s_search || config->op_s_printuris || config->group || config->op_q_list ||
					 config->op_q_info)) || (config->op == PM_OP_DEPTEST && !config->op_d_resolve)) {
				/* special case:  PM_OP_SYNC can be used w/ config->op_s_search by any user */
			} else {
				ERR(NL, "you cannot perform this operation unless you are root.\n");
				exit(1);
			}
		}
	}

	if(config->root == NULL) {
		config->root = strdup(PM_ROOT);
	}

	/* add a trailing '/' if there isn't one */
	if(config->root[strlen(config->root)-1] != '/') {
		char *ptr;
		MALLOC(ptr, strlen(config->root)+2);
		strcpy(ptr, config->root);
		strcat(ptr, "/");
		FREE(config->root);
		config->root = ptr;
	}

	if(config->configfile == NULL) {
		config->configfile = strdup(PACCONF);
	}
	if(parseconfig(config->configfile, config) == -1) {
		cleanup(1);
	}
	if(config->dbpath == NULL) {
		config->dbpath = strdup(PM_DBPATH);
	}
	if(config->cachedir == NULL) {
		config->cachedir = strdup(PM_CACHEDIR);
	}

	/* set library parameters */
	if(alpm_set_option(PM_OPT_LOGMASK, (long)config->debug) == -1) {
		ERR(NL, "failed to set option LOGMASK (%s)\n", alpm_strerror(pm_errno));
		cleanup(1);
	}
	if(alpm_set_option(PM_OPT_LOGCB, (long)cb_log) == -1) {
		ERR(NL, "failed to set option LOGCB (%s)\n", alpm_strerror(pm_errno));
		cleanup(1);
	}
	if(alpm_set_option(PM_OPT_DBPATH, (long)config->dbpath) == -1) {
		ERR(NL, "failed to set option DBPATH (%s)\n", alpm_strerror(pm_errno));
		cleanup(1);
	}
	if(alpm_set_option(PM_OPT_CACHEDIR, (long)config->cachedir) == -1) {
		ERR(NL, "failed to set option CACHEDIR (%s)\n", alpm_strerror(pm_errno));
		cleanup(1);
	}
	
	if(config->verbose > 1) {
		printf("Root  : %s\n", config->root);
		printf("DBPath: %s\n", config->dbpath);
		list_display("Targets:", pm_targets);
	}

	/* Opening local database */
	db_local = alpm_db_register("local");
	if(db_local == NULL) {
		ERR(NL, "could not register 'local' database (%s)\n", alpm_strerror(pm_errno));
		cleanup(1);
	}

	/* start the requested operation */
	switch(config->op) {
		case PM_OP_ADD:     ret = pacman_add(pm_targets);     break;
		case PM_OP_REMOVE:  ret = pacman_remove(pm_targets);  break;
		case PM_OP_UPGRADE: ret = pacman_upgrade(pm_targets); break;
		case PM_OP_QUERY:   ret = pacman_query(pm_targets);   break;
		case PM_OP_SYNC:    ret = pacman_sync(pm_targets);    break;
		case PM_OP_DEPTEST: ret = pacman_deptest(pm_targets); break;
		case PM_OP_MAIN:    ret = 0; break;
		default:
			ERR(NL, "no operation specified (use -h for help)\n");
			ret = 1;
	}
	if(ret != 0 && config->op_d_vertest == 0) {
		MSG(NL, "\n");
	}

	cleanup(ret);
	/* not reached */
	return(0);
}

void cleanup(int signum)
{
	list_t *lp;

	/* free alpm library resources */
	if(alpm_release() == -1) {
		ERR(NL, "%s\n", alpm_strerror(pm_errno));
	}

	/* free memory */
	for(lp = pmc_syncs; lp; lp = lp->next) {
		sync_t *sync = lp->data;
		list_t *i;
		for(i = sync->servers; i; i = i->next) {
			server_t *server = i->data;
			FREE(server->protocol);
			FREE(server->server);
			FREE(server->path);
		}
		FREELIST(sync->servers);
		FREE(sync->treename);
	}
	FREELIST(pmc_syncs);
	FREELIST(pm_targets);
	FREECONF(config);

#ifndef CYGWIN
	/* debug */
	muntrace();
#endif

	fflush(stdout);

	exit(signum);
}

int pacman_deptest(list_t *targets)
{
	PM_LIST *data;
	list_t *i;
	char *str;

	if(targets == NULL) {
		return(0);
	}

	if(config->op_d_vertest) {
		if(targets->data && targets->next && targets->next->data) {
			int ret = alpm_pkg_vercmp(targets->data, targets->next->data);
			printf("%d\n", ret);
			return(ret);
		}
		return(0);
	}

	/* we create a transaction to hold a dummy package to be able to use
	 * deps checkings from alpm_trans_prepare() */
	if(alpm_trans_init(PM_TRANS_TYPE_ADD, 0, NULL, NULL, NULL) == -1) {
		ERR(NL, "%s", alpm_strerror(pm_errno));
		return(1);
	}

	/* We use a hidden facility from alpm_trans_addtarget() to add a dummy
	 * target to the transaction (see the library code for details).
	 * It allows us to use alpm_trans_prepare() to check dependencies of the
	 * given target.
	 */
	str = (char *)malloc(strlen("name=dummy|version=1.0-1")+1);
	strcpy(str, "name=dummy|version=1.0-1");
	for(i = targets; i; i = i->next) {
		str = (char *)realloc(str, strlen(str)+8+strlen(i->data)+1);
		strcat(str, "|depend=");
		strcat(str, i->data);
	}
	if(alpm_trans_addtarget(str) == -1) {
		FREE(str);
		ERR(NL, "%s\n", alpm_strerror(pm_errno));
		alpm_trans_release();
		return(1);
	}
	FREE(str);

	if(alpm_trans_prepare(&data) == -1) {
		PM_LIST *lp;
		int ret = 126;
		list_t *synctargs = NULL;

		switch(pm_errno) {
			case PM_ERR_UNSATISFIED_DEPS:
				for(lp = alpm_list_first(data); lp; lp = alpm_list_next(lp)) {
					PM_DEPMISS *miss = alpm_list_getdata(lp);
					if(!config->op_d_resolve) {
						MSG(NL, "requires: %s", alpm_dep_getinfo(miss, PM_DEP_NAME));
						switch((int)alpm_dep_getinfo(miss, PM_DEP_MOD)) {
							case PM_DEP_MOD_EQ: MSG(CL, "=%s", alpm_dep_getinfo(miss, PM_DEP_VERSION));  break;
							case PM_DEP_MOD_GE: MSG(CL, ">=%s", alpm_dep_getinfo(miss, PM_DEP_VERSION)); break;
							case PM_DEP_MOD_LE: MSG(CL, "<=%s", alpm_dep_getinfo(miss, PM_DEP_VERSION)); break;
						}
						MSG(CL, "\n");
					}
					synctargs = list_add(synctargs, strdup(alpm_dep_getinfo(miss, PM_DEP_NAME)));
				}
				alpm_list_free(data);
			break;
			case PM_ERR_CONFLICTING_DEPS:
				/* we can't auto-resolve conflicts */
				for(lp = alpm_list_first(data); lp; lp = alpm_list_next(lp)) {
					PM_DEPMISS *miss = alpm_list_getdata(lp);
					MSG(NL, "conflict: %s", alpm_dep_getinfo(miss, PM_DEP_NAME));
				}
				ret = 127;
				alpm_list_free(data);
			break;
			default:
				ret = 127;
			break;
		}

		if(alpm_trans_release() == -1) {
			ERR(NL, "%s", alpm_strerror(pm_errno));
			return(1);
		}

		/* attempt to resolve missing dependencies */
		/* TODO: handle version comparators (eg, glibc>=2.2.5) */
		if(ret == 126 && synctargs != NULL) {
			if(!config->op_d_resolve || pacman_sync(synctargs) != 0) {
				/* error (or -D not used) */
				ret = 127;
			}
		}
		FREELIST(synctargs);
		return(ret);
	}

	if(alpm_trans_release() == -1) {
		ERR(NL, "%s", alpm_strerror(pm_errno));
		return(1);
	}

	return(0);
}

/* Parse command-line arguments for each operation
 *     argc: argc
 *     argv: argv
 *     
 * Returns: 0 on success, 1 on error
 */
int parseargs(int argc, char *argv[])
{
	int opt;
	int option_index = 0;
	static struct option opts[] =
	{
		{"add",        no_argument,       0, 'A'},
		{"resolve",    no_argument,       0, 'D'}, /* used by 'makepkg -s' */
		{"freshen",    no_argument,       0, 'F'},
		{"query",      no_argument,       0, 'Q'},
		{"remove",     no_argument,       0, 'R'},
		{"sync",       no_argument,       0, 'S'},
		{"deptest",    no_argument,       0, 'T'}, /* used by makepkg */
		{"upgrade",    no_argument,       0, 'U'},
		{"version",    no_argument,       0, 'V'},
		{"vertest",    no_argument,       0, 'Y'}, /* does the same as the 'vercmp' binary */
		{"dbpath",     required_argument, 0, 'b'},
		{"cascade",    no_argument,       0, 'c'},
		{"changelog",  no_argument,       0, 'c'},
		{"clean",      no_argument,       0, 'c'},
		{"nodeps",     no_argument,       0, 'd'},
		{"dependsonly",no_argument,       0, 'e'},
		{"orphans",    no_argument,       0, 'e'},
		{"force",      no_argument,       0, 'f'},
		{"groups",     no_argument,       0, 'g'},
		{"help",       no_argument,       0, 'h'},
		{"info",       no_argument,       0, 'i'},
		{"dbonly",     no_argument,       0, 'k'},
		{"list",       no_argument,       0, 'l'},
		{"nosave",     no_argument,       0, 'n'},
		{"foreign",    no_argument,       0, 'm'},
		{"owns",       no_argument,       0, 'o'},
		{"file",       no_argument,       0, 'p'},
		{"print-uris", no_argument,       0, 'p'},
		{"root",       required_argument, 0, 'r'},
		{"recursive",  no_argument,       0, 's'},
		{"search",     no_argument,       0, 's'},
		{"sysupgrade", no_argument,       0, 'u'},
		{"verbose",    no_argument,       0, 'v'},
		{"downloadonly", no_argument,     0, 'w'},
		{"refresh",    no_argument,       0, 'y'},
		{"noconfirm",  no_argument,       0, 1000},
		{"config",     required_argument, 0, 1001},
		{"ignore",     required_argument, 0, 1002},
		{"debug",      required_argument, 0, 1003},
		{0, 0, 0, 0}
	};
	char root[PATH_MAX];

	while((opt = getopt_long(argc, argv, "ARUFQSTDYr:b:vkhscVfmnoldepiuwyg", opts, &option_index))) {
		if(opt < 0) {
			break;
		}
		switch(opt) {
			case 0:   break;
			case 1000: config->noconfirm = 1; break;
			case 1001:
				if(config->configfile) {
					free(config->configfile);
				}
				config->configfile = strndup(optarg, PATH_MAX);
				break;
			case 1002:
				if(alpm_set_option(PM_OPT_IGNOREPKG, (long)optarg) == -1) {
					ERR(NL, "failed to set option IGNOREPKG (%s)\n", alpm_strerror(pm_errno));
					return(1);
				}
				break;
			case 1003:
				config->debug = atoi(optarg);
				break;
			case 'A': config->op = (config->op != PM_OP_MAIN ? 0 : PM_OP_ADD);     break;
			case 'D': config->op = (config->op != PM_OP_MAIN ? 0 : PM_OP_DEPTEST); config->op_d_resolve = 1; break;
			case 'F': config->op = (config->op != PM_OP_MAIN ? 0 : PM_OP_UPGRADE); config->flags |= PM_TRANS_FLAG_FRESHEN; break;
			case 'Q': config->op = (config->op != PM_OP_MAIN ? 0 : PM_OP_QUERY);   break;
			case 'R': config->op = (config->op != PM_OP_MAIN ? 0 : PM_OP_REMOVE);  break;
			case 'S': config->op = (config->op != PM_OP_MAIN ? 0 : PM_OP_SYNC);    break;
			case 'T': config->op = (config->op != PM_OP_MAIN ? 0 : PM_OP_DEPTEST); break;
			case 'U': config->op = (config->op != PM_OP_MAIN ? 0 : PM_OP_UPGRADE); break;
			case 'V': config->version = 1; break;
			case 'Y': config->op = (config->op != PM_OP_MAIN ? 0 : PM_OP_DEPTEST); config->op_d_vertest = 1; break;
			case 'b':
				if(config->dbpath) {
					free(config->dbpath);
				}
				config->dbpath = strdup(optarg);
			break;
			case 'c': config->op_s_clean++; config->flags |= PM_TRANS_FLAG_CASCADE; config->op_q_changelog = 1; break;
			case 'd': config->flags |= PM_TRANS_FLAG_NODEPS; break;
			case 'e': config->op_q_orphans = 1; config->flags |= PM_TRANS_FLAG_DEPENDSONLY; break;
			case 'f': config->flags |= PM_TRANS_FLAG_FORCE; break;
			case 'g': config->group = 1; break;
			case 'h': config->help = 1; break;
			case 'i': config->op_q_info++; config->op_s_info++; break;
			case 'k': config->flags |= PM_TRANS_FLAG_DBONLY; break;
			case 'l': config->op_q_list = 1; break;
			case 'm': config->op_q_foreign = 1; break;
			case 'n': config->flags |= PM_TRANS_FLAG_NOSAVE; break;
			case 'o': config->op_q_owns = 1; break;
			case 'p': config->op_q_isfile = 1; config->op_s_printuris = 1; break;
			case 'r':
				if(realpath(optarg, root) == NULL) {
					perror("bad root path");
					return(1);
				}
				if(config->root) {
					free(config->root);
				}
				config->root = strdup(root);
			break;
			case 's': config->op_s_search = 1; config->op_q_search = 1; config->flags |= PM_TRANS_FLAG_RECURSE; break;
			case 'u': config->op_s_upgrade = 1; break;
			case 'v': config->verbose++; break;
			case 'w': config->op_s_downloadonly = 1; break;
			case 'y': config->op_s_sync = 1; break;
			case '?': return(1);
			default:  return(1);
		}
	}

	if(config->op == 0) {
		ERR(NL, "only one operation may be used at a time\n");
		return(1);
	}

	if(config->help) {
		usage(config->op, basename(argv[0]));
		return(2);
	}
	if(config->version) {
		version();
		return(2);
	}

	while(optind < argc) {
		/* add the target to our target array */
		pm_targets = list_add(pm_targets, strdup(argv[optind]));
		optind++;
	}

	return(0);
}

/* Display usage/syntax for the specified operation.
 *     op:     the operation code requested
 *     myname: basename(argv[0])
 */
void usage(int op, char *myname)
{
	if(op == PM_OP_MAIN) {
		printf("usage:  %s {-h --help}\n", myname);
		printf("        %s {-V --version}\n", myname);
		printf("        %s {-A --add}     [options] <file>\n", myname);
		printf("        %s {-R --remove}  [options] <package>\n", myname);
		printf("        %s {-U --upgrade} [options] <file>\n", myname);
		printf("        %s {-F --freshen} [options] <file>\n", myname);
		printf("        %s {-Q --query}   [options] [package]\n", myname);
		printf("        %s {-S --sync}    [options] [package]\n", myname);
		printf("\nuse '%s --help' with other options for more syntax\n", myname);
	} else {
		if(op == PM_OP_ADD) {
			printf("usage:  %s {-A --add} [options] <file>\n", myname);
			printf("options:\n");
			printf("  -d, --nodeps        skip dependency checks\n");
			printf("  -f, --force         force install, overwrite conflicting files\n");
		} else if(op == PM_OP_REMOVE) {
			printf("usage:  %s {-R --remove} [options] <package>\n", myname);
			printf("options:\n");
			printf("  -c, --cascade       remove packages and all packages that depend on them\n");
			printf("  -d, --nodeps        skip dependency checks\n");
			printf("  -k, --dbonly        only remove database entry, do not remove files\n");
			printf("  -n, --nosave        remove configuration files as well\n");
			printf("  -s, --recursive     remove dependencies also (that won't break packages)\n");
		} else if(op == PM_OP_UPGRADE) {
			if(config->flags & PM_TRANS_FLAG_FRESHEN) {
				printf("usage:  %s {-F --freshen} [options] <file>\n", myname);
			} else {
				printf("usage:  %s {-U --upgrade} [options] <file>\n", myname);
			}
			printf("options:\n");
			printf("  -d, --nodeps        skip dependency checks\n");
			printf("  -f, --force         force install, overwrite conflicting files\n");
		} else if(op == PM_OP_QUERY) {
			printf("usage:  %s {-Q --query} [options] [package]\n", myname);
			printf("options:\n");
			printf("  -c, --changelog     view the changelog of a package\n");
			printf("  -e, --orphans       list all packages that were installed as a dependency\n");
			printf("                      and are not required by any other packages\n");
			printf("  -g, --groups        view all members of a package group\n");
			printf("  -i, --info          view package information\n");
			printf("  -l, --list          list the contents of the queried package\n");
			printf("  -m, --foreign       list all packages that were not found in the sync db(s)\n");
			printf("  -o, --owns <file>   query the package that owns <file>\n");
			printf("  -p, --file          pacman will query the package file [package] instead of\n");
			printf("                      looking in the database\n");
			printf("  -s, --search        search locally-installed packages for matching strings\n");
		} else if(op == PM_OP_SYNC) {
			printf("usage:  %s {-S --sync} [options] [package]\n", myname);
			printf("options:\n");
			printf("  -c, --clean         remove old packages from cache directory (use -cc for all)\n");
			printf("  -d, --nodeps        skip dependency checks\n");
			printf("  -e, --dependsonly   install dependencies only\n");
			printf("  -f, --force         force install, overwrite conflicting files\n");
			printf("  -g, --groups        view all members of a package group\n");
			printf("  -p, --print-uris    print out URIs for given packages and their dependencies\n");
			printf("  -s, --search        search remote repositories for matching strings\n");
			printf("  -u, --sysupgrade    upgrade all packages that are out of date\n");
			printf("  -w, --downloadonly  download packages but do not install/upgrade anything\n");
			printf("  -y, --refresh       download fresh package databases from the server\n");
			printf("      --ignore <pkg>  ignore a package upgrade (can be used more than once)\n");
		}
		printf("      --config <path> set an alternate configuration file\n");
		printf("      --noconfirm     do not ask for anything confirmation\n");
		printf("  -v, --verbose       be verbose\n");
		printf("  -r, --root <path>   set an alternate installation root\n");
		printf("  -b, --dbpath <path> set an alternate database location\n");
	}
}

/* Version
 */
void version()
{
	printf("\n");
	printf(" .--.                  Pacman v%s - libalpm v%s\n", PACMAN_VERSION, PM_VERSION);
	printf("/ _.-' .-.  .-.  .-.   Copyright (C) 2002-2005 Judd Vinet <jvinet@zeroflux.org>\n");
	printf("\\  '-. '-'  '-'  '-'   & Frugalware developers <frugalware-devel@frugalware.org>\n");
	printf(" '--'                  \n");
	printf("                       This program may be freely redistributed under\n");
	printf("                       the terms of the GNU General Public License\n");
	printf("\n");
}

/*
 * Misc functions
 */

/* Condense a list of strings into one long (space-delimited) string
 */
char *buildstring(list_t *strlist)
{
	char *str;
	int size = 1;
	list_t *lp;

	for(lp = strlist; lp; lp = lp->next) {
		size += strlen(lp->data) + 1;
	}
	str = (char *)malloc(size);
	if(str == NULL) {
		ERR(NL, "failed to allocated %d bytes\n", size);
	}
	str[0] = '\0';
	for(lp = strlist; lp; lp = lp->next) {
		strcat(str, lp->data);
		strcat(str, " ");
	}
	/* shave off the last space */
	str[strlen(str)-1] = '\0';

	return(str);
}

/* vim: set ts=2 sw=2 noet: */
