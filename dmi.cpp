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
    std::ifstream f("/usr/share/ldetect-lst/dmitable", std::ios_base::in | std::ios_base::binary);

    igzstream gzsb;
    std::istream *fp;
    if(!f.is_open()) {
    	gzsb.open("/usr/share/ldetect-lst/dmitable.gz");
	fp = &gzsb;
    } else
	fp = &f;

    typedef std::map<std::string, std::string> Item;
    typedef std::map<std::string, Item> Attr;
    typedef std::map<std::string, Attr> Subtable;
    typedef std::map<std::string, Subtable> Name;
    typedef std::map<std::string, Name> Table;
    Table dmitable;

    std::string attrFirst, attrSecond;
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
		attrFirst.assign(buf+5, sep-buf-5), attrSecond = sep+2;
		dmitable[tableFirst][tableSecond][subtableFirst][subtableSecond][attrFirst] = attrSecond;
	    }
	}
    }
    if (f.is_open())
    	f.close();
    else
	gzsb.close();

    struct kmod_ctx *ctx = modalias_init();
    struct sysfs_class *sfc = sysfs_open_class("dmi");
    struct dlist *classlist = sysfs_get_class_devices(sfc);
    struct sysfs_class_device *class_device = nullptr;

    dlist_for_each_data(classlist, class_device, struct sysfs_class_device) {
	size_t pos;
	struct sysfs_attribute *attr = nullptr;
	for (Table::const_iterator tit = dmitable.begin();
		tit != dmitable.end(); ++tit) {
	    for (Name::const_iterator nit = tit->second.begin();
		    nit != tit->second.end(); ++nit) {
		if ((attr = sysfs_get_classdev_attr(class_device, tit->first.c_str()))) {
		    if (((pos = nit->first.find_last_of(".*")) != std::string::npos &&
				!nit->first.compare(0, pos-1, attr->value, pos-1)) ||
			    nit->first == attr->value) {

			std::string deviceName(attr->value, strlen(attr->value)-1);
			for (Subtable::const_iterator sit = nit->second.begin();
				sit != nit->second.end(); ++sit) {
			    for (Attr::const_iterator ait = sit->second.begin();
				    ait != sit->second.end(); ++ait) {
				if ((attr = sysfs_get_classdev_attr(class_device, sit->first.c_str()))) {
				    if (((pos = ait->first.find_last_of(".*")) != std::string::npos &&
						!ait->first.compare(0, pos-1, attr->value, pos-1)) ||
					    ait->first == attr->value) {

					deviceName.append("|").append(attr->value, strlen(attr->value)-1);
					for (Item::const_iterator iit = ait->second.begin();
						iit != ait->second.end(); ++iit) {
					    if (iit->first == "Module")
						_entries.push_back(dmiEntry(iit->second, deviceName));
					}
				    }
				}
			    }
			}
		    }
		}
	    }
	}

	std::string deviceName;
	if ((attr = sysfs_get_classdev_attr(class_device, "sys_vendor")) != nullptr) {
	    deviceName.append(attr->value, strlen(attr->value)-1);
	}
	if ((attr = sysfs_get_classdev_attr(class_device, "product_name")) != nullptr) {
	    if (!deviceName.empty())
		deviceName += "|";
	    deviceName.append(attr->value, strlen(attr->value)-1);
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
