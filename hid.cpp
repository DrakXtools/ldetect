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

/* currently untested, lacking any hid devices to test with...
 */


void hid::probe(void)
{
    const std::string hidDevs("/sys/bus/hid/devices/");
    DIR *dir = opendir(hidDevs.c_str());
    if (dir == nullptr)
	return;
    struct kmod_ctx *ctx = modalias_init();

    std::ifstream f;
    for (struct dirent *dent = readdir(dir); dent != nullptr; dent = readdir(dir)) {
	if (dent->d_type != DT_DIR || !strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
	    continue;
	std::string hidDev(std::string(hidDevs).append("/").append(dent->d_name));

	f.open((hidDev + "/modalias").c_str());
	std::string modname;
	if (f.is_open()) {
	    std::string modalias;
	    getline(f, modalias);
	    modname = modalias_resolve_module(ctx, modalias);
	    f.close();
	}

	f.open((hidDev + "/uevent").c_str());
	std::string deviceName;
	if (f.is_open()) {
	    std::string line;
	    while (!f.eof()) {
		getline(f, line);
		if (!line.compare(0,sizeof("HID_DEVICE")-1, "HID_DEVICE")) {
		    deviceName.assign(line, sizeof("HID_DEVICE"), line.size()-sizeof("HID_DEVICE"));
		    break;
		}
	    }
	    f.close();
	} else
	    deviceName = "HID Device";

	if (!modname.empty()) 
	    _entries.push_back(entry(modname, deviceName));
    }

    kmod_unref(ctx);
    closedir(dir);
}

}
