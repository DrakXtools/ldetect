extern "C" {
#include <pci/pci.h>
}
#include <sys/types.h>
#include <dirent.h>
#include <pci/header.h>
#include <libkmod.h>

#include "common.h"
#include "pci.h"

/* /proc files're 256 bytes but we only need first 64 bytes*/
#define CONFIG_SPACE_SIZE 64

namespace ldetect {

pciEntry::pciEntry(pci_dev *dev, pci_access *pacc) : pciusbEntry(),
			//vendor(0xffff), device(0xffff),
			pci_domain(0), /*pci_bus(0xff), pci_device(0xff),*/
			pci_function(0xff), pci_revision(0), is_pciexpress(false) {
    uint8_t buf[CONFIG_SPACE_SIZE] = {0};
    char classbuf[128] = {0}, vendorbuf[128] {0}, devbuf[128] = {0};


    pci_setup_cache(dev, (u8*)buf, CONFIG_SPACE_SIZE);
    pci_read_block(dev, 0, buf, CONFIG_SPACE_SIZE);
    pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_CLASS | PCI_FILL_CAPS);

    pci_lookup_name(pacc, vendorbuf, sizeof(vendorbuf), PCI_LOOKUP_VENDOR, dev->vendor_id, dev->device_id);
    pci_lookup_name(pacc, devbuf,    sizeof(devbuf),    PCI_LOOKUP_DEVICE, dev->vendor_id, dev->device_id);
    text.append(vendorbuf).append("|").append(devbuf);
    class_type += classbuf;
    vendor =     dev->vendor_id;
    device =     dev->device_id;
    pci_domain = dev->domain;
    bus =        dev->bus;
    pciusb_device = dev->dev;
    pci_function = dev->func;

    class_id = dev->device_class;
    subvendor = pci_read_word(dev, PCI_SUBSYSTEM_VENDOR_ID);
    subdevice = pci_read_word(dev, PCI_SUBSYSTEM_ID);
    pci_revision = pci_read_byte(dev, PCI_REVISION_ID);

    if ((subvendor == 0 && subdevice == 0) ||
	    (subvendor == vendor && subdevice == device)) {
	subvendor = 0xffff;
	subdevice = 0xffff;
    }

    if (pci_find_cap(dev,PCI_CAP_ID_EXP, PCI_CAP_NORMAL))
	is_pciexpress = true;

    /* special case for realtek 8139 that has two drivers */
    if (vendor == 0x10ec && device == 0x8139) {
	if (pci_revision < 0x20)
	    module += "8139too";
	else
	    module += "8139cp";
    }

}

std::string pciEntry::verbose() const {
    std::ostringstream oss(std::ostringstream::out);
    oss << " (vendor:" << hexFmt(vendor) << " device:" << hexFmt(device);
    if (subvendor != 0xffff || subdevice != 0xffff)
	oss << " subv:" << hexFmt(subvendor) << " subd:" << hexFmt(subdevice);
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

static void __attribute__((noreturn)) error_and_die(char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    fprintf(stderr, "%s: ", "lspcidrake");
    vfprintf(stderr, msg, args);
    fputc('\n', stderr);
    exit(1);
}

void pci::probe(void) {
	_pacc->error = error_and_die;
	pci_scan_bus(_pacc);

	for (struct pci_dev *dev = _pacc->devices; dev && _entries.size() < MAX_DEVICES; dev = dev->next)
	    _entries.push_back(pciEntry(dev, _pacc));

	findModules("pcitable", false);
}

void pci::find_modules_through_aliases(struct kmod_ctx *ctx, pciEntry &e) {
    std::ostringstream modalias_path(std::ostringstream::out);
    modalias_path << "/sys/bus/pci/devices/" << hexFmt(e.pci_domain, 4, false) << ":" <<  hexFmt(e.bus, 2, false) <<
	":" << hexFmt(e.pciusb_device, 2, false) << "." << hexFmt(e.pci_function, 0, false) << "/modalias";
    set_modules_from_modalias_file(ctx, e, modalias_path.str());
}

}
