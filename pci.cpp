extern "C" {
#include <pci/pci.h>
}
#include <sys/types.h>
#include <dirent.h>
#include <pci/header.h>
#include <libkmod.h>
#include <sysfs/libsysfs.h>

#include "common.h"
#include "pci.h"

/* /proc files're 256 bytes but we only need first 64 bytes*/
#define CONFIG_SPACE_SIZE 64

namespace ldetect {

std::string pciEntry::verbose() const {
    std::ostringstream oss(std::ostringstream::out);
    oss << " (vendor:" << hexFmt(vendor, 4, false) << " device:" << hexFmt(device, 4, false);
    if (subvendor != 0xffff || subdevice != 0xffff)
	oss << " subv:" << hexFmt(subvendor, 4, false) << " subd:" << hexFmt(subdevice, 4, false);
    oss << ")";

    return oss.str();
}

std::string pciEntry::rev() const {
    std::ostringstream oss(std::ostringstream::out);
    if (pci_revision)
	oss << " (rev: " << hexFmt(pci_revision, 2, false) << ")";

    return oss.str();
}

std::ostream& operator<<(std::ostream& os, const pciEntry& e) {
    os << static_cast<const pciusbEntry&>(e);
    if (e.class_id) {
	const std::string &s = pci_class2text(e.class_id);
	if (s != "NOT_DEFINED")
	    os << " [" << s << "]";
    }

    return os;
}

pci::pci(std::string proc_pci_path) : _pacc(pci_alloc()) {
    pci_init(_pacc);
    _pacc->numeric_ids = 0;
    pci_set_param(_pacc, const_cast<char*>("proc.path"), const_cast<char*>(proc_pci_path.c_str()));
}

pci::pci(const pci &p) : _pacc(nullptr) {
    *this = p;
    pci_init(_pacc);
    _pacc->numeric_ids = 0;
    pci_set_param(_pacc, const_cast<char*>("proc.path"), pci_get_param(p._pacc, const_cast<char*>("proc.path")));
}

pci& pci::operator=(const pci &p) {
    *this = p;
    pci_init(_pacc);
    _pacc->numeric_ids = 0;
    pci_set_param(_pacc, const_cast<char*>("proc.path"), pci_get_param(p._pacc, const_cast<char*>("proc.path")));
    return *this;
}

pci::~pci() {
    pci_cleanup(_pacc);
}

const std::string& pci::getDescription(uint16_t vendor_id, uint16_t device_id) {
        char vendorbuf[128], devbuf[128];

	pci_lookup_name(_pacc, vendorbuf, sizeof(vendorbuf), PCI_LOOKUP_VENDOR | PCI_LOOKUP_NO_NUMBERS, vendor_id, device_id);
	pci_lookup_name(_pacc, devbuf,    sizeof(devbuf),    PCI_LOOKUP_DEVICE, vendor_id, device_id);

	return std::string(vendorbuf).append("|").append(devbuf);
}

void pci::probe(void) {
    pci_scan_bus(_pacc);

    uint8_t buf[CONFIG_SPACE_SIZE] = {0};
    char classbuf[128] = {0}, vendorbuf[128] {0}, devbuf[128] = {0};

    for (struct pci_dev *dev = _pacc->devices; dev && _entries.size() < MAX_DEVICES; dev = dev->next) {
	_entries.push_back(pciEntry());
	pciEntry &e = _entries.back();
	memset(buf, 0, sizeof(buf));
	memset(classbuf, 0, sizeof(classbuf));
	memset(vendorbuf, 0, sizeof(vendorbuf));
	memset(devbuf, 0, sizeof(devbuf));

	pci_setup_cache(dev, buf, CONFIG_SPACE_SIZE);
	pci_read_block(dev, 0, buf, CONFIG_SPACE_SIZE);
	pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_CLASS | PCI_FILL_CAPS);

	pci_lookup_name(_pacc, vendorbuf, sizeof(vendorbuf), PCI_LOOKUP_VENDOR, dev->vendor_id, dev->device_id);
	pci_lookup_name(_pacc, devbuf,    sizeof(devbuf),    PCI_LOOKUP_DEVICE, dev->vendor_id, dev->device_id);
	e.text.append(vendorbuf).append("|").append(devbuf);
	e.class_type += classbuf;
	e.vendor =     dev->vendor_id;
	e.device =     dev->device_id;
	e.pci_domain = dev->domain;
	e.bus =        dev->bus;
	e.pciusb_device = dev->dev;
	e.pci_function = dev->func;

	e.class_id = dev->device_class;
	e.subvendor = pci_read_word(dev, PCI_SUBSYSTEM_VENDOR_ID);
	e.subdevice = pci_read_word(dev, PCI_SUBSYSTEM_ID);
	e.pci_revision = pci_read_byte(dev, PCI_REVISION_ID);

	if ((e.subvendor == 0 && e.subdevice == 0) ||
		(e.subvendor == e.vendor && e.subdevice == e.device)) {
	    e.subvendor = 0xffff;
	    e.subdevice = 0xffff;
	}

	if (pci_find_cap(dev,PCI_CAP_ID_EXP, PCI_CAP_NORMAL))
	    e.is_pciexpress = true;

	/* special case for realtek 8139 that has two drivers */
	if (e.vendor == 0x10ec && e.device == 0x8139) {
	    if (e.pci_revision < 0x20)
		e.module = "8139too";
	    else
		e.module = "8139cp";
	}
    }

    findModules("pcitable", false);
}

void pci::findModules(std::string &&fpciusbtable, bool descr_lookup) {
    ldetect::findModules(fpciusbtable, descr_lookup, _entries);
    ::kmod_ctx *ctx = modalias_init();

    for (std::vector<pciEntry>::iterator it = _entries.begin();
	    it != _entries.end(); ++it) {
	pciEntry &e = *it;

	// No special case found in pcitable ? Then lookup modalias for PCI devices
	if (!e.module.empty() && e.module != "unknown")
	    continue;
	{
	    std::ostringstream devname(std::ostringstream::out);
	    devname << hexFmt(e.pci_domain, 4, false) << ":" <<  hexFmt(e.bus, 2, false) <<
		":" << hexFmt(e.pciusb_device, 2, false) << "." << hexFmt(e.pci_function, 0, false);

	    std::ifstream f(std::string("/sys/bus/pci/devices/").append(devname.str()).append("/modalias").c_str());
	    if (f.is_open()) {
		std::string modalias;
		getline(f, modalias);
		e.module = modalias_resolve_module(ctx, modalias);
	    }

	}
    }

    kmod_unref(ctx);

}

}
