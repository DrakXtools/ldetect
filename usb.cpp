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

usb::usb(std::string proc_usb_path) : pciusb("usb"), _proc_usb_path(proc_usb_path) {
    names_init("/usr/share/usb.ids");
}

usb::~usb() {
    names_exit();
}

void usb::probe(void) {
	if (_sysfs_bus != nullptr) {
	    struct dlist *devs = sysfs_get_bus_devices(_sysfs_bus);
	    struct sysfs_device *device = nullptr;

	    std::stringstream buf;
	    dlist_for_each_data(devs, device, struct sysfs_device) {
		if (device != nullptr) {
		    struct sysfs_attribute *attr = sysfs_get_device_attr(device, "idVendor");
		    if(attr == nullptr)
			continue;

		    _entries.push_back(usbEntry());
		    usbEntry &e = _entries.back();

		    buf.clear();
		    buf << attr->value;
		    buf >> std::hex >> e.vendor;

		    attr = sysfs_get_device_attr(device, "idProduct");
		    if(attr == nullptr)
			continue;
		    buf.clear();
		    buf << attr->value;
		    buf >> std::hex >> e.device;

		    attr = sysfs_get_device_attr(device, "busnum");
		    if(attr == nullptr)
			continue;
		    e.bus = atoi(attr->value);

		    uint32_t class_id, sub, prot = 0;

		    // FIXME: busted...
		    attr = sysfs_get_device_attr(device, "bDeviceClass");
		    if(attr == nullptr)
			continue;
		    e.class_id = atoi(attr->value) * 0x100;
		    buf.clear();
		    buf << attr->value;
		    buf >> std::hex >> class_id;

		    attr = sysfs_get_device_attr(device, "bDeviceSubClass");
		    if(attr == nullptr)
			continue;
		    e.class_id += atoi(attr->value);
		    buf.clear();
		    buf << attr->value;
		    buf >> std::hex >> sub;


		    attr = sysfs_get_device_attr(device, "bDeviceProtocol");
		    if(attr == nullptr)
			continue;
		    e.class_id *= 0x100;
		    e.class_id += atoi(attr->value);
		    buf.clear();
		    buf << attr->value;
		    buf >> std::hex >> prot;

		    e.class_id = (class_id * 0x100 + sub) * 0x100 + prot;

		    attr = sysfs_get_device_attr(device, "devnum");
		    if(attr == nullptr)
			continue;
		    e.pciusb_device = atoi(attr->value);

		    e.text = names_vendor(e.vendor);
		    if (e.text.empty()) {
			attr = sysfs_get_device_attr(device, "product");
			if(attr == nullptr)
			    continue;
			e.text = attr->value;
		    }

		    e.text += "|";
		    const char *productName = names_product(e.vendor, e.device);
		    if (productName == nullptr) {
			attr = sysfs_get_device_attr(device, "product");
			if(attr == nullptr)
			    continue;
			e.text += attr->value;
		    } else
			e.text += productName;

		    attr = sysfs_get_device_attr(device, "devpath");
		    if(attr == nullptr)
			continue;
		    buf.clear();
		    buf << attr->value;
		    buf >> e.devpath;

		    attr = sysfs_get_device_attr(device, "bConfigurationValue");
		    if(attr == nullptr)
			continue;
		    buf.clear();
		    buf << attr->value;
		    buf >> e.usb_port;

		    attr = sysfs_get_device_attr(device, "bNumInterfaces");
		    if(attr == nullptr)
			continue;
		    e.interfaces = atoi(attr->value);
		}
	    }
	}

	findModules("usbtable", false);

}

void usb::find_modules_through_aliases(struct kmod_ctx *ctx, usbEntry &e) {
    char buf[16];
    for (int i = 0; i < e.interfaces && e.module.empty(); i++) {
	snprintf(buf, sizeof(buf), "%d-%s:%d.%d", e.bus, e.devpath.c_str(), e.usb_port, i);
	struct sysfs_device *device = sysfs_get_bus_device(_sysfs_bus, buf);
	if (device != nullptr) {
	    struct sysfs_attribute *attr = sysfs_get_device_attr(device, "modalias");
	    if (attr != nullptr)
		e.module =  modalias_resolve_module(ctx, attr->value);
	}
    }
}

}
