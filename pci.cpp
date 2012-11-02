#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <cstring>
#include <cstdio>
#include <stdarg.h>
extern "C" {
#include <pci/pci.h>
}
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <pci/header.h>
#include "common.h"

/* /proc files're 256 bytes but we only need first 64 bytes*/
#define CONFIG_SPACE_SIZE 64

static const char proc_pci_path_default[] = "/proc/bus/pci/devices";
const char* proc_pci_path = NULL;

static void __attribute__((noreturn)) error_and_die(char *msg, ...)
{
  va_list args;

  va_start(args, msg);
  fprintf(stderr, "%s: ", "lspcidrake");
  vfprintf(stderr, msg, args);
  fputc('\n', stderr);
  exit(1);
}

static struct pci_access *pacc;
const std::string& get_pci_description(uint16_t vendor_id, uint16_t device_id) {
        char vendorbuf[128], devbuf[128];

        if (!pacc) {
           pacc = pci_alloc();
           pci_init(pacc);               /* Initialize the PCI library */
           pacc->numeric_ids = 0;
           pci_init(pacc);
	}

	pci_lookup_name(pacc, vendorbuf, sizeof(vendorbuf), PCI_LOOKUP_VENDOR | PCI_LOOKUP_NO_NUMBERS, vendor_id, device_id);
	pci_lookup_name(pacc, devbuf,    sizeof(devbuf),    PCI_LOOKUP_DEVICE, vendor_id, device_id);

        //pci_cleanup(pacc); // only used by stage1's merge-pcitable.pl that doesn't care about leaking _one_ struct
	return std::string(vendorbuf).append("|").append(devbuf);
}

std::vector<pciusb_entry>* pci_probe(void) {
	u8 buf[CONFIG_SPACE_SIZE];
	std::vector<pciusb_entry> *r = NULL;

	static struct pci_access *pacc;
	char classbuf[128] = {0}, vendorbuf[128] {0}, devbuf[128] = {0};

	if ((access(proc_pci_path_default, R_OK) != 0)
		|| (proc_pci_path && (access(proc_pci_path_default, R_OK) != 0)))
		return r;

	pacc = pci_alloc();

	if (proc_pci_path)
             pci_set_param(pacc, const_cast<char*>("proc.path"), const_cast<char*>(proc_pci_path));


	pci_init(pacc);
	pacc->error = error_and_die;
	pci_scan_bus(pacc);

	r = new std::vector<pciusb_entry>;

	for (struct pci_dev *dev = pacc->devices; dev && r->size() < MAX_DEVICES; dev = dev->next) {
		r->push_back(pciusb_entry());
		pciusb_entry &e = r->back();
		memset(buf, 0, CONFIG_SPACE_SIZE); // make sure not to retrieve values from previous devices
		pci_setup_cache(dev, (u8*)buf, CONFIG_SPACE_SIZE);
		pci_read_block(dev, 0, buf, CONFIG_SPACE_SIZE);
		pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_CLASS | PCI_FILL_CAPS);

		pci_lookup_name(pacc, vendorbuf, sizeof(vendorbuf), PCI_LOOKUP_VENDOR, dev->vendor_id, dev->device_id);
		pci_lookup_name(pacc, devbuf,    sizeof(devbuf),    PCI_LOOKUP_DEVICE, dev->vendor_id, dev->device_id);
		e.text.append(vendorbuf).append("|").append(devbuf);
		e.class_type += classbuf;
		e.vendor =     dev->vendor_id;
		e.device =     dev->device_id;
		e.pci_domain = dev->domain;
		e.pci_bus =    dev->bus;
		e.pci_device = dev->dev;
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

	pci_cleanup(pacc);

	if (pciusb_find_modules(*r, "pcitable", DO_NOT_LOAD, 1))
		return r;

	/* should not happen */
	exit(1);
}
