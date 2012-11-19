#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
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

namespace ldetect {


static std::string aliasdefault;
static std::string dirname("/lib/modules/");

static void set_default_alias_file(void) {
    std::string fallback_aliases(table_name_dir + "fallback-modules.alias");
    struct stat st_alias, st_fallback;
    struct utsname buf;

    uname(&buf);
    dirname += buf.release;

    std::string aliasfilename(dirname+"/modules.alias");

    /* fallback on ldetect-lst's modules.alias and prefer it if more recent */
    if (stat(aliasfilename.c_str(), &st_alias) ||
	    (!stat(fallback_aliases.c_str(), &st_fallback) && st_fallback.st_mtime > st_alias.st_mtime))
	aliasdefault = fallback_aliases;
    else
	aliasdefault = aliasfilename;
}

struct kmod_ctx* modalias_init(void) {
	if (aliasdefault.empty())
		set_default_alias_file();

	std::string dkms_file(table_name_dir + "dkms-modules.alias");

	/* We only use canned aliases as last resort. */
	const char *alias_filelist[] = {
		"/run/modprobe.d",
		"/etc/modprobe.d",
		"/lib/modprobe.d",
		"/lib/module-init-tools/ldetect-lst-modules.alias",
		aliasdefault.c_str(),
		dkms_file.c_str(),
		nullptr,
	};

	/* Init libkmod */
	struct kmod_ctx *ctx = kmod_new(dirname.c_str(), alias_filelist);
	if (!ctx) {
		fputs("Error: kmod_new() failed!\n", stderr);
		kmod_unref(ctx);
		ctx = nullptr;
	}
	kmod_load_resources(ctx);
	return ctx;
}

std::string modalias_resolve_module(struct kmod_ctx *ctx, const char *modalias) {
	struct kmod_list *l = nullptr, *list = nullptr, *filtered = nullptr;
	std::string str;
	int err = kmod_module_new_from_lookup(ctx, modalias, &list);
	if (err < 0)
		goto exit;

	// No module found...
	if (list == nullptr)
		goto exit;

	// filter through blacklist
	err =  kmod_module_apply_filter(ctx, KMOD_FILTER_BLACKLIST, list, &filtered);
	kmod_module_unref_list(list);
	if (err <0)
		goto exit;
	list = filtered;

	kmod_list_foreach(l, list) {
		struct kmod_module *mod = kmod_module_get_module(l);
		//if (str) // keep last one
		//	free(str);
		if (str.empty()) // keep first one
			str = kmod_module_get_name(mod);
		kmod_module_unref(mod);
		if (err < 0)
			break;
	}

	kmod_module_unref_list(list);

exit:
	return str;
}

}
