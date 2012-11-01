#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libkmod.h>

#include "libsysfs.h"
#include "libldetect.h"
#include "common.h"


#define HID_BUS_NAME "hid"
const char *sysfs_hid_path = "/sys/bus/hid/devices";
#if 0
#define DEBUG(args...) printf(args)
#else 
#define DEBUG(args...)
#endif

#if 0
struct module_alias {
    const char      *modalias;
    const char      *module;
};

static struct module_alias aliasdb[] = { {NULL, NULL} };
static const char *resolve_modalias(const struct module_alias *aliasdb,
				    const char *alias_name)
{
    const struct module_alias *alias = aliasdb;

    while (alias->modalias != NULL) {
	if (fnmatch(alias->modalias, alias_name, 0) == 0) 
	    return alias->module;

	alias++;
    }
    return NULL;
}
#endif

static char *get_field_value(const char *fields, const char *field_name)
{
	const char *modalias;
	const char *end;

	modalias = strstr(fields, field_name);
	if (modalias == NULL)
		return NULL;
	end = strchr(modalias, '\n');
	if (end == NULL)
		end = modalias + strlen(modalias);

	return strndup(modalias+strlen(field_name), end - (modalias+strlen(field_name)));
}

static char *parse_modalias(char *fields)
{
	return get_field_value(fields, "MODALIAS=");
}

static char *parse_name(char *fields)
{
	return get_field_value(fields, "HID_NAME=");
}



static void parse_device(struct kmod_ctx *ctx, std::vector<entry> &entries, const char *dev)
{
	char keyfile[SYSFS_PATH_MAX];
	char *modalias;
	char *device_name;
	struct sysfs_attribute *sysfs_attr;

	snprintf(keyfile, sizeof(keyfile)-1, "%s/%s/uevent",
			sysfs_hid_path, dev);
	sysfs_attr = sysfs_open_attribute(keyfile);
	if (!sysfs_attr)
		return;
	if (sysfs_read_attribute(sysfs_attr) != 0 || !sysfs_attr->value) {
		sysfs_close_attribute(sysfs_attr);
		return;
	}

	DEBUG("%s: read %s\n", HID_BUS_NAME, sysfs_attr->value);

	modalias = parse_modalias(sysfs_attr->value);
	if (modalias == NULL)
		return;
	DEBUG("%s: modalias is [%s]\n", HID_BUS_NAME, modalias);

	device_name = parse_name(sysfs_attr->value);
	sysfs_close_attribute(sysfs_attr);
	if (device_name != NULL) 
		DEBUG("%s: device name is [%s]\n", HID_BUS_NAME, device_name);
	else 
		device_name = strdup("HID Device");

	std::string modname = modalias_resolve_module(ctx, modalias);
	free(modalias);
	DEBUG("%s: module name is [%s]\n", HID_BUS_NAME, modname.c_str());
	if (!modname.empty()) 
		entries.push_back(entry(modname,device_name));
}


std::vector<entry>* hid_probe(void)
{
	std::vector<entry> *entry_list = NULL;
	struct kmod_ctx *ctx = modalias_init();
	DIR *dir = opendir(sysfs_hid_path);
	if (dir == NULL) {
		fprintf(stderr, "Unable to open \"%s\": %s\n"
				    "Won't scan for hid devices\n",
				    sysfs_hid_path, strerror(errno));
		goto end_probe;
	}

	entry_list = new std::vector<entry>(0);

	for (struct dirent *dent = readdir(dir); dent != NULL; dent = readdir(dir)) {
		if (dent->d_name[0] == '.')
			continue;
		DEBUG("%s: device found %s\n", HID_BUS_NAME, dent->d_name);
		parse_device(ctx, *entry_list, dent->d_name);
	}

end_probe:
	modalias_cleanup(ctx);
	if (dir)
		closedir(dir);

	return entry_list;
}

