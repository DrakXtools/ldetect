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
#include <modprobe.h>
#include <dirent.h>
#include "common.h"

static char *aliasdefault = NULL;

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
	struct module_command *commands = NULL;
	struct module_options *modoptions = NULL;
	struct module_alias *aliases = NULL;
	struct module_blacklist *blacklist = NULL;
	const char *config = NULL;

	if (!aliasdefault)
		set_default_alias_file();

	/* Returns the resolved alias, options */
	read_toplevel_config(config, modalias, 0,
			     0, &modoptions, &commands, &aliases, &blacklist);

	if (!aliases) {
		/* We only use canned aliases as last resort. */
		char *dkms_file = table_name_to_file("dkms-modules.alias");
		char *alias_filelist[] = {
			"/lib/module-init-tools/ldetect-lst-modules.alias",
			aliasdefault,
			dkms_file,
			NULL,
		};
		char **alias_file = alias_filelist;
		while (!aliases && *alias_file) {
			read_config(*alias_file, modalias, 0,
				    0, &modoptions, &commands,
				    &aliases, &blacklist);
			aliases = apply_blacklist(aliases, blacklist);
			alias_file++;
		}
		free(dkms_file);
	}
	if (aliases) {
		// take the last one because we find eg: generic/ata_generic/sata_sil
		while (aliases->next)
			aliases = aliases->next;

		return strdup(aliases->module);
	}

	return NULL;
}

void modalias_cleanup(void) {
    ifree(aliasdefault);
}
