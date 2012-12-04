#include <sstream>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <vector>
#include <libkmod.h>
#include <dirent.h>

#include "common.h"
#include "names.h"

#include "usb.h"
#include "libldetect.h"

namespace ldetect {

// FIXME
std::ostream& operator<<(std::ostream& os, const usbEntry& e) {
    os << static_cast<const pciusbEntry&>(e);
    struct usb_class_text s = usb_class2text(e.class_id);
    if (!s.class_text.empty()) {
	os << " [" << s.class_text;
	if (!s.sub_text.empty()) os << "|" << s.sub_text;
	if (!s.prot_text.empty()) os << "|" << s.prot_text;
	os << "]";
    }
    return os;
}

usb::usb() {
    names_init("/usr/share/usb.ids");
}

usb::~usb() {
    names_exit();
}

static const std::string usbDevs("/sys/bus/usb/devices/");

void usb::probe(void) {
    DIR *dp;
    struct dirent *dirp;
    if((dp = opendir(usbDevs.c_str())) == nullptr)
	return;

    std::ifstream f;
    std::string usbPath, f1val, f2val;
    while ((dirp = readdir(dp)) != nullptr) {
	if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
	    continue;
	size_t pos;
	usbPath.assign(usbDevs).append(dirp->d_name).append("/");
	f.open(usbPath + "idVendor");
	if (f.is_open()) {
	    _entries.push_back(usbEntry());
	    usbEntry &e = _entries.back();
	    f >> std::hex >> e.vendor;

	    f.close();
	    f.open(usbPath + "idProduct");
	    if (f.is_open()) {
		f >> std::hex >> e.device;
		f.close();
	    }

	    f.open(usbPath + "busnum");
	    if (f.is_open()) {
		// FIXME
		uint16_t bus;
		f >> std::hex >> bus;
		e.bus = bus;
		f.close();
	    }

	    f.open(usbPath + "devnum");
	    if (f.is_open()) {
		// FIXME
		uint16_t pciusb_device;
		f >> std::hex >> pciusb_device;
		e.pciusb_device = pciusb_device;
		f.close();
	    }

	    e.text = names_vendor(e.vendor);
	    if (e.text.empty()) {
		f.open(usbPath + "manufacturer");
		if (f.is_open()) {
		    getline(f, e.text);
		    f.close();
		}
	    }

	    e.text += "|";
	    const char *productName = names_product(e.vendor, e.device);
	    if (productName == nullptr) {
		std::string product;
		getline(f, product);
		e.text += product;
	    } else
		e.text += productName;

	    f.open(usbPath + "devpath");
	    if (f.is_open()) {
		getline(f, e.devpath);
		f.close();
	    }

	    f.open(usbPath + "bConfigurationValue");
	    if (f.is_open()) {
		f >> e.usb_port;
		f.close();
	    }

	    f.open(usbPath + "bNumInterfaces");
	    if (f.is_open()) {
		f >> e.interfaces;
		f.close();
	    }

	    uint32_t class_id, sub, prot = 0;

	    DIR *dp2;
	    uint16_t i;
	    if((dp2 = opendir(usbPath.c_str())) == nullptr)
		continue;

	    for (i = 0, dirp = readdir(dp2);
		    i < e.interfaces && dirp != nullptr;
		    dirp = readdir(dp2)) {
		if (dirp->d_type != DT_DIR || !strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
		    continue;
		std::string usbPath2 = std::string(usbPath).append(dirp->d_name).append("/");
    		// FIXME: partially busted...
		f.open(usbPath2 + "bInterfaceClass");
		if (f.is_open()) {
		    f >> std::hex >> class_id;
		    f.close();
		    i++;
		} else
		    continue;

		f.open(usbPath2 + "bInterfaceSubClass");
		if (f.is_open()) {
		    f >> std::hex >> sub;
		    f.close();
		}

		f.open(usbPath2 + "bInterfaceProtocol");
		if (f.is_open()) {
		    f >> std::hex >> prot;
		    f.close();
		}

		e.class_id = (class_id * 0x100 + sub) * 0x100 + prot;
		if (e.class_id)
		    break;
	    }
	    closedir(dp2);
	}
    }

    findModules("usbtable", false);

}

void usb::find_modules_through_aliases(struct kmod_ctx *ctx, usbEntry &e) {
    char buf[16];
    std::ifstream f;
    for (uint16_t i = 0; i < e.interfaces && e.module.empty(); i++) {
	snprintf(buf, sizeof(buf), "%d-%s:%d.%d", e.bus, e.devpath.c_str(), e.usb_port, i);
	f.open(std::string(usbDevs).append(buf).append("/modalias"));
	std::string modalias;
	getline(f, modalias);
	e.module = modalias_resolve_module(ctx, modalias.c_str());
    }
}

}
