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
#include <libkmod.h>
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


char*dirname;

static void set_default_alias_file(void) {
	struct utsname rel_buf;
	if (!aliasdefault) {
		char *dirname;
		char *fallback_aliases = table_name_to_file("fallback-modules.alias");
		char *aliasfilename;
		struct stat st_alias, st_fallback;

		uname(&rel_buf);
		asprintf(&dirname, "%s/%s", "/lib/modules", rel_buf.release);
		asprintf(&aliasfilename, "%s/modules.alias", dirname);
		free(dirname);

		/* fallback on ldetect-lst's modules.alias and prefer it if more recent */
		if (stat(aliasfilename, &st_alias) ||
		    (!stat(fallback_aliases, &st_fallback) && st_fallback.st_mtime > st_alias.st_mtime)) {
			free(aliasfilename);
			aliasdefault = fallback_aliases;
		} else {
			aliasdefault = aliasfilename;
			free(fallback_aliases);
		}
	}
}

char *modalias_resolve_module(const char *modalias) {
        struct kmod_ctx *ctx;
	struct kmod_list *l, *list = NULL;
	int err;
	char* dkms_file,  *str = NULL;

	if (!aliasdefault)
		set_default_alias_file();

	get_version();

	/* We only use canned aliases as last resort. */
	dkms_file = table_name_to_file("dkms-modules.alias");
	const char *alias_filelist[] = {
		"/run/modprobe.d",
		"/etc/modprobe.d",
		"/lib/modprobe.d",
		"/lib/module-init-tools/ldetect-lst-modules.alias",
		aliasdefault,
		dkms_file,
		NULL,
	};

	/* Init libkmod */
	ctx = kmod_new(dirname, alias_filelist);
	if (!ctx) {
		fputs("Error: kmod_new() failed!\n", stderr);
		goto exit;
	}
	kmod_load_resources(ctx);

	err = kmod_module_new_from_lookup(ctx, modalias, &list);
	if (err < 0)
		goto exit;

	// No module found...
	if (list == NULL)
		goto exit;

	// filter through blacklist
	struct kmod_list *filtered = NULL;
	err = kmod_module_get_filtered_blacklist(ctx, list, &filtered);
	kmod_module_unref_list(list);
	if (err <0)
		goto exit;
	list = filtered;

	kmod_list_foreach(l, list) {
		struct kmod_module *mod = kmod_module_get_module(l);
		//if (str) // keep last one
		//	free(str);
		if (!str) // keep first one
		str = strdup(kmod_module_get_name(mod));
		kmod_module_unref(mod);
		if (err < 0)
			break;
	}

	kmod_module_unref_list(list);

exit:
	free(dkms_file);
	kmod_unref(ctx);
	return str;
}

void modalias_cleanup(void) {
    ifree(aliasdefault);
    ifree(version);
}
