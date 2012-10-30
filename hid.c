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
const char *sysfs_hid_path = "/sys/bus/"HID_BUS_NAME"/devices";
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
	char *modalias;
	char *end;

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

static void add_entry(dmi_hid_entries_t *entry_list, char *name, char *module)
{
    
	dmi_hid_entry_t *new_entries;

	new_entries = realloc(entry_list->entries, (entry_list->nb+1)*sizeof(*(entry_list->entries)));
	if (new_entries != NULL) {
		new_entries[entry_list->nb].module = module;
		new_entries[entry_list->nb].text = name;
		entry_list->entries = new_entries;
		entry_list->nb++;
	}
}

static void parse_device(struct kmod_ctx *ctx, dmi_hid_entries_t *entries, const char *dev)
{
	char keyfile[SYSFS_PATH_MAX];
	char *modalias;
	char *modname;
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

	modname = modalias_resolve_module(ctx, modalias);
	free(modalias);
	DEBUG("%s: module name is [%s]\n", HID_BUS_NAME, modname);
	if (modname != NULL) 
		add_entry(entries, device_name, modname);
}


dmi_hid_entries_t hid_probe(void)
{
	DIR *dir;
	dmi_hid_entries_t entry_list;
	struct kmod_ctx *ctx = modalias_init();

	memset(&entry_list, 0, sizeof(entry_list));

	dir = opendir(sysfs_hid_path);
	if (dir == NULL)
		goto end_probe;

	for (struct dirent *dent = readdir(dir); dent != NULL; dent = readdir(dir)) {
		if (dent->d_name[0] == '.')
			continue;
		DEBUG("%s: device found %s\n", HID_BUS_NAME, dent->d_name);
		parse_device(ctx, &entry_list, dent->d_name);
	}

end_probe:
	modalias_cleanup(ctx);
	if (dir)
		closedir(dir);

	return entry_list;
}

