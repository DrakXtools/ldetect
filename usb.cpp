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

usbEntry::usbEntry(struct sysfs_device *sfsDevice) : usb_port(0xffff) {
    std::stringstream buf;
    struct sysfs_attribute *attr = sysfs_get_device_attr(sfsDevice, "idVendor");

    buf << attr->value;
    buf >> std::hex >> vendor;

    attr = sysfs_get_device_attr(sfsDevice, "idProduct");
    buf.clear();
    buf << attr->value;
    buf >> std::hex >> device;

    attr = sysfs_get_device_attr(sfsDevice, "busnum");
    bus = atoi(attr->value);

    uint32_t class_id, sub, prot = 0;

    // FIXME: busted...
    attr = sysfs_get_device_attr(sfsDevice, "bDeviceClass");
    class_id = atoi(attr->value) * 0x100;
    buf.clear();
    buf << attr->value;
    buf >> std::hex >> class_id;

    attr = sysfs_get_device_attr(sfsDevice, "bDeviceSubClass");
    class_id += atoi(attr->value);
    buf.clear();
    buf << attr->value;
    buf >> std::hex >> sub;


    attr = sysfs_get_device_attr(sfsDevice, "bDeviceProtocol");
    class_id *= 0x100;
    class_id += atoi(attr->value);
    buf.clear();
    buf << attr->value;
    buf >> std::hex >> prot;

    class_id = (class_id * 0x100 + sub) * 0x100 + prot;

    attr = sysfs_get_device_attr(sfsDevice, "devnum");
    pciusb_device = atoi(attr->value);

    text = names_vendor(vendor);
    if (text.empty()) {
	attr = sysfs_get_device_attr(sfsDevice, "product");
	text = attr->value;
    }

    text += "|";
    const char *productName = names_product(vendor, device);
    if (productName == nullptr) {
	attr = sysfs_get_device_attr(sfsDevice, "product");
	text += attr->value;
    } else
	text += productName;

    attr = sysfs_get_device_attr(sfsDevice, "devpath");
    buf.clear();
    buf << attr->value;
    buf >> devpath;

    attr = sysfs_get_device_attr(sfsDevice, "bConfigurationValue");
    buf.clear();
    buf << attr->value;
    buf >> usb_port;

    attr = sysfs_get_device_attr(sfsDevice, "bNumInterfaces");
    interfaces = atoi(attr->value);
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
		_entries.push_back(device);
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
