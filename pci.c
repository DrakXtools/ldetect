#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pci/pci.h>
#include "libldetect.h"
#include "libldetect-private.h"
#include "common.h"

char *proc_pci_path_default = "/proc/bus/pci/devices";
char *proc_pci_path = NULL;

extern struct pciusb_entries pci_probe(int probe_type) {
	FILE *f, *devf;
	char buf[BUF_SIZE];
	unsigned short *bufi = (unsigned short *) &buf;
	unsigned short devbusfn;
	unsigned int id;
	struct pciusb_entries r;
	char file[25];

	r.nb = 0;
	if (!(f = fopen(proc_pci_path ? proc_pci_path : proc_pci_path_default, "r"))) {
	        if (proc_pci_path) {
		    char *err_msg;
		    asprintf(&err_msg, "unable to open \"%s\"\n"
			     "You may have passed a wrong argument to the \"-p\" option.\n"
			     "fopen() sets errno to", proc_pci_path);
		    perror(err_msg);
		    free(err_msg);
		}
		r.entries = NULL;
		return r;
	}
	r.entries = malloc(sizeof(struct pciusb_entry) * MAX_DEVICES);

	for (; fgets(buf, sizeof(buf) - 1, f) && r.nb < MAX_DEVICES; r.nb++) {
		struct pciusb_entry *e = &r.entries[r.nb];
		unsigned char class_prog = 0;
		pciusb_initialize(e);

		sscanf(buf, "%hx %x", &devbusfn, &id);
		e->vendor = id >> 16;
		e->device = id & 0xffff;
		e->pci_bus = devbusfn >> 8;
		e->pci_device = (devbusfn & 0xff) >> 3;
		e->pci_function = (devbusfn & 0xff) & 0x07;
		if (probe_type != 1)
			continue;
		snprintf(file, sizeof(file), "/proc/bus/pci/%02x/%02x.%d", e->pci_bus, e->pci_device, e->pci_function);
		if ((devf = fopen(file, "r")) == NULL)
			continue;
		fread(&buf, 0x30, 1, devf); /* these files're 256 bytes but we only need first 48 bytes*/
		e->class_ = bufi[0x5];
		e->subvendor = bufi[0x16];
		e->subdevice = bufi[0x17];
				
		if ((e->subvendor == 0 && e->subdevice == 0) ||
		    (e->subvendor == e->vendor && e->subdevice == e->device)) {
			e->subvendor = 0xffff;
			e->subdevice = 0xffff;
		}
		class_prog = buf[0x9];
		if (e->class_ == PCI_CLASS_SERIAL_USB) /* taken from kudzu's pci.c */
			e->module = strdup(class_prog == 0 ? "usb-uhci" : "usb-ohci");
		fclose(devf);
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
