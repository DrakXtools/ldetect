#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list.h>
#include <logging.h>
#include <modprobe.h>
#include <dirent.h>
#include "common.h"

static char *aliasdefault = NULL;
static char * version = NULL;

static void get_version(void) {
	if (version != NULL)
		return;
	struct utsname buf;
	uname(&buf);
	version = strdup(buf.release);
}

static void free_aliases(struct module_alias *aliases) {
	if (aliases == NULL) 
		return;
	while (aliases->next) {
		struct module_alias *next;
		next = aliases->next;
		free(aliases->module);
		free(aliases);
		aliases = next;
	}
	free(aliases);
}

static void free_options(struct module_options *modoptions) {
	if (modoptions == NULL) 
		return;
	while (modoptions->next) {
		struct module_options *next;
		next = modoptions->next;
		free(modoptions->modulename);
		free(modoptions->options);
		free(modoptions);
		modoptions = next;
	}
	free(modoptions);
}

static void free_commands(struct module_command *commands) {
	if (commands == NULL) 
		return;
	while (commands->next) {
		struct module_command *next;
		next = commands->next;
		free(commands->modulename);
		free(commands->command);
		free(commands);
		commands = next;
	}
	free(commands);
}

static void free_blacklist(struct module_blacklist *blacklist) {
	if (blacklist == NULL) 
		return;
	while (blacklist->next) {
		struct module_blacklist *next;
		next = blacklist->next;
		free(blacklist->modulename);
		free(blacklist);
		blacklist = next;
	}
	free(blacklist);
}

char*dirname;

static void set_default_alias_file(void) {
	struct utsname rel_buf;
	if (!aliasdefault) {
		char *dirname;
		char *fallback_aliases = table_name_to_file("fallback-modules.alias");
		char *aliasfilename;
		struct stat st_alias, st_fallback;

		uname(&rel_buf);
		asprintf(&dirname, "%s/%s", MODULE_DIR, rel_buf.release);
		asprintf(&aliasfilename, "%s/modules.alias", dirname);
		free(dirname);

		/* fallback on ldetect-lst's modules.alias and prefer it if more recent */
		if (stat(aliasfilename, &st_alias) ||
		    (!stat(fallback_aliases, &st_fallback) && st_fallback.st_mtime > st_alias.st_mtime)) {
			free(aliasfilename);
			aliasdefault = fallback_aliases;
		} else {
			aliasdefault = aliasfilename;
		}
	}
}

char *modalias_resolve_module(const char *modalias) {
	struct module_alias *aliases = NULL;
	struct module_blacklist *blacklist = NULL;
       struct modprobe_conf conf = {};
	LIST_HEAD(list);

	if (!aliasdefault)
		set_default_alias_file();

	get_version();
	/* Returns the resolved alias, options */
	parse_toplevel_config(NULL, &conf, 0, 0);

	errfn_t error;
        aliases = find_matching_aliases(modalias, "", &conf, dirname, error, mit_dry_run, &list);

	/* only load blacklisted modules with specific request (no alias) */
	apply_blacklist(&aliases, conf.blacklist);

	if (!aliases) {
		/* We only use canned aliases as last resort. */
		char *dkms_file = table_name_to_file("dkms-modules.alias");
		const char *alias_filelist[] = {
			"/lib/module-init-tools/ldetect-lst-modules.alias",
			aliasdefault,
			dkms_file,
			NULL,
		};
		const char **alias_file = alias_filelist;
		while (!aliases && *alias_file) {
			parse_config_file(*alias_file, &conf, 0, 0);
			aliases = find_matching_aliases(modalias, "", &conf, dirname, error, mit_dry_run, &list);
			apply_blacklist(&aliases, blacklist);
			alias_file++;
		}
		free(dkms_file);
	}
	free_blacklist(blacklist);
	if (aliases) {
		char *result;
		// take the last one because we find eg: generic/ata_generic/sata_sil
		struct module_alias *it = aliases;
		while (it->next)
			it = it->next;

		result = strdup(it->module);
		free_aliases(aliases);
		return result;
	}

	return NULL;
}

void modalias_cleanup(void) {
    ifree(aliasdefault);
}
