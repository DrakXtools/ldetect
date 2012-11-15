#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cerrno>
#include <libkmod.h>
#include <sysfs/libsysfs.h>

#include "libldetect.h"
#include "common.h"
#include "hid.h"

namespace ldetect {

/* need to rewrite these functions in c++, but lack any hid devices to get
 * any input for testing from
 */
static char *get_field_value(const char *fields, const char *field_name)
{
	const char *modalias;
	const char *end;

	modalias = strstr(fields, field_name);
	if (modalias == nullptr)
		return nullptr;
	end = strchr(modalias, '\n');
	if (end == nullptr)
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


void hid::probe(void)
{
    DIR *dir = opendir(_sysfs_hid_path.c_str());
    if (dir == nullptr) {
	std::cerr << "Unable to open \"" << _sysfs_hid_path << "\": " << strerror(errno) << std::endl <<
		"Won't scan for hid devices" << std::endl;
	return;
    }
    struct kmod_ctx *ctx = modalias_init();

    for (struct dirent *dent = readdir(dir); dent != nullptr; dent = readdir(dir))
	if (dent->d_name[0] != '.')
	    parse_device(ctx, dent->d_name);

    modalias_cleanup(ctx);
    closedir(dir);
}

void hid::parse_device(struct kmod_ctx *ctx, const char *dev)
{
	char *modalias;
	struct sysfs_attribute *sysfs_attr;

	std::string keyfile(_sysfs_hid_path + "/");
	keyfile.append(dev).append("/").append("uevent");
	sysfs_attr = sysfs_open_attribute(keyfile.c_str());
	if (!sysfs_attr)
		return;
	if (sysfs_read_attribute(sysfs_attr) != 0 || !sysfs_attr->value) {
		sysfs_close_attribute(sysfs_attr);
		return;
	}

	modalias = parse_modalias(sysfs_attr->value);
	if (modalias == nullptr)
		return;

	char *device_name = parse_name(sysfs_attr->value);
	sysfs_close_attribute(sysfs_attr);

	std::string modname = modalias_resolve_module(ctx, modalias);
	free(modalias);
	if (!modname.empty()) 
		_entries.push_back(hidEntry(modname, device_name ? device_name : "HID Device"));
	if (device_name)
	    free(device_name);
}

std::ostream& operator<<(std::ostream& os, const hidEntry& e) {
    return os << std::setw(16) << std::left << e.module << ":" << e.text;
}

}
