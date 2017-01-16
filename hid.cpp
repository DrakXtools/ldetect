#include <fstream>
#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <dirent.h>
#include <libkmod.h>

#include "libldetect.h"
#include "common.h"
#include "hid.h"

namespace ldetect {

hid::hid() {
}

hid::~hid() {
}

void hid::probe(void)
{
    const std::string hidDevs("/sys/bus/hid/devices/");
    DIR *dir = opendir(hidDevs.c_str());
    if (dir == nullptr)
	return;
    struct kmod_ctx *ctx = modalias_init();

    std::ifstream f;
    for (struct dirent *dent = readdir(dir); dent != nullptr; dent = readdir(dir)) {
	if ((dent->d_type != DT_DIR && dent->d_type != DT_LNK) || !strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
	    continue;
	std::string hidDev(std::string(hidDevs).append(dent->d_name));

	f.open((hidDev + "/modalias").c_str());
	std::string modname;
	if (f.is_open()) {
	    std::string modalias;
	    getline(f, modalias);
	    std::vector<std::string> kmodules = modalias_resolve_modules(ctx, modalias);
	    if (!kmodules.empty())
		modname = kmodules.front();
	    f.close();
	}

	f.open((hidDev + "/uevent").c_str());
	std::string deviceName;
	if (f.is_open()) {
	    std::string line;
	    while (!f.eof()) {
		getline(f, line);
		if (!line.compare(0,sizeof("HID_NAME")-1, "HID_NAME")) {
		    deviceName.assign(line, sizeof("HID_NAME"), line.size()-sizeof("HID_NAME"));
		    break;
		}
	    }
	    f.close();
	} else
	    deviceName = "HID Device";

	if (!modname.empty() && !(!_entries.empty() && _entries.back().module == modname))
	    _entries.push_back(entry(modname, deviceName));
    }

    kmod_unref(ctx);
    closedir(dir);
}

}
