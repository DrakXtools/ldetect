#include <unistd.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pci/pci.h>
#include "libldetect.h"
#include "libldetect-private.h"
#include "common.h"

char *proc_pci_path = "/proc/bus/pci/devices";

extern struct pciusb_entries pci_probe(int probe_type) {
	FILE *f;
	char buf[BUF_SIZE];
	unsigned short devbusfn;
	unsigned int id;
	struct pciusb_entries r;
	r.entries = malloc(sizeof(struct pciusb_entry) * MAX_DEVICES);

	if (!(f = fopen(proc_pci_path, "r"))) {
		char *err_msg;
		asprintf(&err_msg, "unable to open \"%s\"\n"
			    "You may have passed a wrong argument to the \"-p\" option.\n"
			    "fopen() sets errno to", proc_pci_path);
		perror(err_msg);
		exit(1);
	}

	for (r.nb = 0; fgets(buf, sizeof(buf) - 1, f) && r.nb < MAX_DEVICES; r.nb++) {
		struct pciusb_entry *e = &r.entries[r.nb];
		pciusb_initialize(e);

		sscanf(buf, "%hx %x", &devbusfn, &id);
		e->vendor = id >> 16;
		e->device = id & 0xffff;
		e->pci_bus = devbusfn >> 8;
		e->pci_device = (devbusfn & 0xff) >> 3;
		e->pci_function = (devbusfn & 0xff) & 0x07;
		if (probe_type == 1) {
			char file[25];
			FILE *f;
			snprintf(file, sizeof(file), "/proc/bus/pci/%02x/%02x.%d", e->pci_bus, e->pci_device, e->pci_function);
			if ((f = fopen(file, "r"))) {
				if (fseek(f, 10, SEEK_SET) == 0) 
					fread(&e->class_, 2, 1, f);
				if (fseek(f, 0x2c, SEEK_SET) == 0) 
					fread(&e->subvendor, 2, 1, f), fread(&e->subdevice, 2, 1, f);

				if ((e->subvendor == 0 && e->subdevice == 0) ||
				    (e->subvendor == e->vendor && e->subdevice == e->device)) {
					e->subvendor = 0xffff;
					e->subdevice = 0xffff;
				}

				if (fseek(f, 9, SEEK_SET) == 0) {
					unsigned char class_prog = 0;
					fread(&class_prog, 1, 1, f);
					if (e->class_ == PCI_CLASS_SERIAL_USB) {
						/* taken from kudzu's pci.c */
						e->module = strdup(class_prog == 0 ? "usb-uhci" : "usb-ohci");
					}
				}
				fclose(f);
			}
		}
	}
	fclose(f);
	realloc(r.entries,  sizeof(struct pciusb_entry) * r.nb);

	if (pciusb_find_modules(&r, "pcitable", probe_type))
		return r;

	/* ok, let's try again with subids */
	if (probe_type == 0) 
		return pci_probe(1);

	/* should not happen */
	exit(1);
}

