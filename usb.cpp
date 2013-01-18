#include <sstream>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <vector>
#include <libkmod.h>
#include <dirent.h>

#include "common.h"

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

usb::usb() : _names("/usr/share/usb.ids") {
}

usb::~usb() {
}

static const std::string usbDevs("/sys/bus/usb/devices/");

void usb::probe(void) {
    DIR *dp;
    struct dirent *dirp;
    if((dp = opendir(usbDevs.c_str())) == nullptr)
	return;

    std::ifstream f;
    std::string usbPath;
    while ((dirp = readdir(dp)) != nullptr) {
	if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
	    continue;
	usbPath.assign(usbDevs).append(dirp->d_name).append("/");
	f.open((usbPath + "idVendor").c_str());
	if (f.is_open()) {
	    _entries.push_back(usbEntry());
	    usbEntry &e = _entries.back();
	    f >> std::hex >> e.vendor;

	    f.close();
	    f.open((usbPath + "idProduct").c_str());
	    if (f.is_open()) {
	    f >> std::hex >> e.device;
		f.close();
	    }

	    f.open((usbPath + "busnum").c_str());
	    if (f.is_open()) {
		// FIXME
		uint16_t bus;
		f >> std::hex >> bus;
		e.bus = bus;
		f.close();
	    }

	    f.open((usbPath + "devnum").c_str());
	    if (f.is_open()) {
		// FIXME
		uint16_t pciusb_device;
		f >> std::hex >> pciusb_device;
		e.pciusb_device = pciusb_device;
		f.close();
	    }

	    e.text = _names.getVendor(e.vendor);
	    if (e.text.empty()) {
		f.open((usbPath + "manufacturer").c_str());
		if (f.is_open()) {
		    getline(f, e.text);
		    f.close();
		}
	    }

	    e.text += "|";
#ifdef __UCLIBCXX_MAJOR__
	    const char *productName = _names.getProduct(e.vendor, e.device);
	    if (productName == nullptr) {
#else
	    const std::string &productName = _names.getProduct(e.vendor, e.device);
	    if (productName.empty()) {
#endif
		std::string product;
		getline(f, product);
		e.text += product;
	    } else
		e.text += productName;

	    f.open((usbPath + "devpath").c_str());
	    if (f.is_open()) {
		getline(f, e.devpath);
		f.close();
	    }

	    f.open((usbPath + "bConfigurationValue").c_str());
	    if (f.is_open()) {
		f >> e.usb_port;
		f.close();
	    }

	    f.open((usbPath + "bNumInterfaces").c_str());
	    if (f.is_open()) {
		f >> e.interfaces;
		f.close();
	    }
	}
    }
    free(dp);

    findModules("usbtable", false);

}

void usb::findModules(std::string &&fpciusbtable, bool descr_lookup) {
    ldetect::findModules(fpciusbtable, descr_lookup, _entries);
    ::kmod_ctx *ctx = modalias_init();

    for (uint16_t i = 0; i < _entries.size(); i++) {
	usbEntry &e = _entries[i];

	// No special case found in pcitable ? Then lookup modalias for PCI devices
	if (!e.module.empty() && e.module != "unknown")
	    continue;
	{
	    std::ostringstream devname(std::ostringstream::out);
	    devname << static_cast<uint16_t>(e.bus) << "-" << e.devpath << ":" << e.usb_port << ".";

	    std::ifstream f;
	    std::string path(usbDevs + devname.str());
	    for (uint16_t i = 0; i < e.interfaces && e.module.empty(); i++) {
		std::ostringstream numStr(std::ostringstream::out);
		numStr << i;
		std::string devPath(path + numStr.str());
		f.open((devPath + "/modalias").c_str());
		if (f.is_open()) {
		    std::string modalias;
		    getline(f, modalias);
		    e.module = modalias_resolve_module(ctx, modalias);
		}
		f.close();
		if (!e.class_id) {
		    f.open((devPath + "/bInterfaceClass").c_str());
		    if (f.is_open()) {
			uint32_t cid, sub, prot = 0;

			f >> std::hex >> cid;
			f.close();

			f.open((devPath + "/bInterfaceSubClass").c_str());
			if (f.is_open()) {
			    f >> std::hex >> sub;
			    f.close();
			}

			f.open((devPath + "/bInterfaceProtocol").c_str());
			if (f.is_open()) {
			    f >> std::hex >> prot;
			    f.close();
			}
			e.class_id = (cid * 0x100 + sub) * 0x100 + prot;
		    }
		    f.close();
		}
	    }
	}
    }

    kmod_unref(ctx);
}

}
