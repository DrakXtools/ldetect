#include <iostream>
#include <fstream>
#include <cstdio>
#include <map>
#include <libkmod.h>
#include <sysfs/libsysfs.h>

#include "gzstream.h"
#include "libldetect.h"
#include "common.h"
#include "dmi.h"

namespace ldetect {

void dmi::probe(void)
{
    char buf[BUF_SIZE];

    struct dmiTable {
	    std::string table;
	    std::string name;
	    std::string subtable;
	    std::string attr;
	    std::string item;
	    std::string value;
    };

    instream fp = fh_open("dmitable");
    std::vector<dmiTable> dmitable;

    std::string subtableFirst, subtableSecond;
    std::string tableFirst, tableSecond;
    while (!fp->eof()) {
	fp->getline(buf, sizeof(buf));

	if (*buf == '#') continue; // skip comments
	char *sep = strchr(buf, ':');
	if (!sep)
	    continue;
	if (isalpha(*buf))
	    tableFirst.assign(buf, sep-buf), tableSecond = sep+2;
	else if (buf[0] == ' ' && buf[1] == ' ') {
	    if (isalpha(buf[2]))
		subtableFirst.assign(buf+2, sep-buf-2), subtableSecond = sep+2;
	    else if (buf[2] == '=' && buf[3] == '>' && buf[4] == ' ' && isalpha(buf[5])) {
		dmitable.push_back({tableFirst, tableSecond, subtableFirst, subtableSecond, std::string(buf+5, sep-buf-5), std::string(sep+2)});
	    }
	}
    }

    struct kmod_ctx *ctx = modalias_init();
    struct sysfs_class *sfc = sysfs_open_class("dmi");
    struct dlist *classlist = sysfs_get_class_devices(sfc);
    struct sysfs_class_device *class_device = nullptr;

    dlist_for_each_data(classlist, class_device, struct sysfs_class_device) {
	size_t pos;
	struct sysfs_attribute *attr1 = nullptr, *attr2 = nullptr;
	std::string deviceName;

	for(std::vector<dmiTable>::const_iterator it = dmitable.begin(); it != dmitable.end(); ++it) {
	    if (((attr1 = sysfs_get_classdev_attr(class_device, it->table.c_str())) &&
			(((pos = it->name.find_last_of(".*")) != std::string::npos &&
			  !it->name.compare(0, pos-1, attr1->value, pos-1)) ||
			 it->name == attr1->value)) &&
		    ((attr2 = sysfs_get_classdev_attr(class_device, it->subtable.c_str())) &&
		     (((pos = it->attr.find_last_of(".*")) != std::string::npos &&
		       !it->attr.compare(0, pos-1, attr2->value, pos-1)) ||
		      it->attr == attr2->value) &&
		     it->item == "Module"))
		_entries.push_back(dmiEntry(it->value, std::string(attr1->value, strlen(attr1->value)-1)));
	}

	if ((attr1 = sysfs_get_classdev_attr(class_device, "sys_vendor")) != nullptr) {
	    deviceName.append(attr1->value, strlen(attr1->value)-1);
	}
	if ((attr1 = sysfs_get_classdev_attr(class_device, "product_name")) != nullptr) {
	    if (!deviceName.empty())
		deviceName += "|";
	    deviceName.append(attr1->value, strlen(attr1->value)-1);
	}

	struct sysfs_attribute *sysfs_attr = sysfs_get_classdev_attr(class_device, "modalias");

	if (!sysfs_attr || sysfs_read_attribute(sysfs_attr) != 0 || !sysfs_attr->value)
	    continue;

	std::string modalias(sysfs_attr->value);

	std::string modname = modalias_resolve_module(ctx, modalias.c_str());
	if (!modname.empty()) 
	    _entries.push_back(dmiEntry(modname, deviceName));

    }

    kmod_unref(ctx);
    sysfs_close_class(sfc);
}

std::ostream& operator<<(std::ostream& os, const dmiEntry& e) {
    return os << std::setw(16) << std::left << e.module << ": " << e.text;
}

}
