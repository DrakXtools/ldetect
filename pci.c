#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <pci/pci.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <pci/pci.h>
#include "common.h"

static char *proc_pci_path_default = "/proc/bus/pci/devices";
char *proc_pci_path = NULL;

void __attribute__((noreturn)) error_and_die(char *msg, ...)
{
  va_list args;

  va_start(args, msg);
  fprintf(stderr, "%s: ", "lspcidrake");
  vfprintf(stderr, msg, args);
  fputc('\n', stderr);
  exit(1);
}

extern struct pciusb_entries pci_probe(void) {
	int devf;
	char buf[BUF_SIZE];
	unsigned short *bufi = (unsigned short *) &buf;
	struct pciusb_entries r;
	char file[32];

	static struct pci_access *pacc;
	struct pci_dev *dev;
	char classbuf[128], vendorbuf[128], devbuf[128];

	pacc = pci_alloc();

	if (proc_pci_path) {
          pacc->method_params[PCI_ACCESS_PROC_BUS_PCI] = proc_pci_path;
          pacc->method = PCI_ACCESS_PROC_BUS_PCI;
	}


	pci_init(pacc);
	pacc->error = error_and_die;
	pci_scan_bus(pacc);

	r.nb = 0;
	r.entries = malloc(sizeof(struct pciusb_entry) * MAX_DEVICES);

	for (dev = pacc->devices; dev; dev = dev->next, r.nb++) {

		struct pciusb_entry *e = &r.entries[r.nb];
		unsigned char class_prog = 0;
		pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_CLASS);

		pciusb_initialize(e);

          asprintf(&e->text, "%s|%s",
                   pci_lookup_name(pacc, vendorbuf, sizeof(vendorbuf), PCI_LOOKUP_VENDOR, dev->vendor_id, dev->device_id),
                   pci_lookup_name(pacc, devbuf,    sizeof(devbuf),    PCI_LOOKUP_DEVICE, dev->vendor_id, dev->device_id)
               );
          pci_lookup_name(pacc, classbuf, sizeof(classbuf), PCI_LOOKUP_CLASS, dev->device_class),
          e->class=strdup(classbuf);    

		e->vendor =     dev->vendor_id;
		e->device =     dev->device_id;
		e->pci_bus =    dev->bus;
		e->pci_device = dev->dev;
		e->pci_function = dev->func;
		snprintf(file, sizeof(file), "/proc/bus/pci/%02x/%02x.%d", e->pci_bus, e->pci_device, e->pci_function);
		if ((devf = open(file, O_RDONLY)) == -1) {
		    /* try with pci domains */
		    int found = 0;
              snprintf(file, sizeof(file), "/proc/bus/pci/%04x:%02x/%02x.%d", dev->domain, e->pci_bus, e->pci_device, e->pci_function);
			    if ((devf = open(file, O_RDONLY)) >= 0)
				found = 1;
		    if (!found)
			continue;
		}
		read(devf, &buf, 0x30); /* these files're 256 bytes but we only need first 48 bytes*/
		e->class_id = dev->device_class;
		/* we divide by 2 because we address the array as a word array since we read a word */
		e->subvendor = bufi[PCI_SUBSYSTEM_VENDOR_ID/2]; // == (u16)!(buf[PCI_SUBSYSTEM_VENDOR_ID] | (buf[PCI_SUBSYSTEM_VENDOR_ID+1] << 8))
		e->subdevice = bufi[PCI_SUBSYSTEM_ID/2];
				
		if ((e->subvendor == 0 && e->subdevice == 0) ||
		    (e->subvendor == e->vendor && e->subdevice == e->device)) {
			e->subvendor = 0xffff;
			e->subdevice = 0xffff;
		}
		class_prog = buf[PCI_CLASS_PROG];

		/* special rules below must be in sync with gi/mdk-stage1/probing.c */

		if (e->class_id == PCI_CLASS_SERIAL_USB) {
		  /* taken from kudzu's pci.c */
		  char *module = 
		    class_prog == 0 ? "usb-uhci" : 
		    class_prog == 0x10 ? "usb-ohci" :
		    class_prog == 0x20 ? "ehci-hcd" : NULL;
		  if (module) e->module = strdup(module);

		} else if (e->class_id == PCI_CLASS_SERIAL_FIREWIRE) {    
		  /* taken from kudzu's pci.c */
		  if (class_prog == 0x10) e->module = strdup("ohci1394");

		} else if (e->class_id == PCI_CLASS_BRIDGE_CARDBUS) {
			e->module = strdup("yenta_socket");
		} else if (e->class_id == (PCI_CLASS_BRIDGE_HOST << 8)) { /* AGP */
               if (e->vendor == 0x10b9)
                    e->module = strdup("ali-agp");
               else if (e->vendor == 0x1002)
                    e->module = strdup("ati-agp");
               else if (e->vendor == 0x1039)
                    e->module = strdup("sis-agp");
               else if (e->vendor == 0x1166)
                    e->module = strdup("sworks-agp");
               else if (e->vendor == 0x1279)
                    e->module = strdup("efficeon-agp");
		} else if (e->device == 0x8139) {
               if (e->subvendor == 0x8139 && e->subdevice == 0x10ec
                   || e->subvendor == 0x1186 && e->subdevice == 0x1300
                   || e->subvendor == 0x13d1 && e->subdevice == 0xab06)
                    e->module = strdup("8139too");
		} else if (e->vendor == 0x10de && e->class_id == PCI_CLASS_STORAGE_IDE) {
			e->module = strdup("sata_nv");
		} else if (e->vendor == 0x10b5 && (e->device == 0x9030 || e->device == 0x9050) && e->subvendor == 0x1369) {
			e->module = strdup("snd-vx222");
		} else if (e->vendor == 0x1119) {   /* Vortex only makes RAID controllers. */
			e->module = strdup("gdth");
		}

		close(devf);
	}
	realloc(r.entries,  sizeof(struct pciusb_entry) * r.nb);
	pci_cleanup(pacc);

	if (pciusb_find_modules(&r, "pcitable", DO_NOT_LOAD))
		return r;

	/* should not happen */
	exit(1);
}
