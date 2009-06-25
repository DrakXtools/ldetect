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
char* proc_pci_path = NULL;

static void __attribute__((noreturn)) error_and_die(char *msg, ...)
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
             pci_set_param(pacc, "proc.path", proc_pci_path);
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
		e->pci_domain = dev->domain;
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

		close(devf);
	}
	r.entries = realloc(r.entries,  sizeof(struct pciusb_entry) * r.nb);

	pci_cleanup(pacc);

	if (pciusb_find_modules(&r, "pcitable", DO_NOT_LOAD, 1))
		return r;

	/* should not happen */
	exit(1);
}
