#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

extern int pciusb_find_modules(struct pciusb_entries *entries, const char *fpciusbtable, const descr_lookup descr_lookup) {
	fh f;
	char buf[2048];
	int line;

	f = fh_open(fpciusbtable);

	for (line = 1; fgets(buf, sizeof(buf) - 1, f.f); line++) {
		unsigned short vendor, device, subvendor, subdevice;
		char *p = NULL, *q = NULL;
		int offset; unsigned int i;
		int nb;
		if (buf[0]=='#')
			continue; // skip comments

		nb = sscanf(buf, "0x%hx\t0x%hx\t0x%hx\t0x%hx\t%n", &vendor, &device, &subvendor, &subdevice, &offset);
		if (nb != 4) {
			nb = sscanf(buf, "0x%hx\t0x%hx\t%n", &vendor, &device, &offset);
			if (nb != 2) {
				fprintf(stderr, "%s %d: bad line\n", fpciusbtable, line);
				continue; // skip bad line
			}
		}
		for (i = 0; i < entries->nb; i++) {
			struct pciusb_entry *e = &entries->entries[i];
			if (e->already_found)
				continue;	// skip since already found with sub ids
			if (vendor != e->vendor ||  device != e->device)
				continue; // main ids differ

			if (nb == 4 && !(subvendor == e->subvendor && subdevice == e->subdevice))
				continue; // subids differ

			if (!p) { // only calc text & module if not already done
				p = buf + offset + 1;
				q = strchr(p, '\t');
			}
			if (strncmp(p, "unknown", q-p-1)) {
				ifree(e->module);
				e->module = strndup(p,q-p-1);
			}
			/* special case for buggy 0x0 usb entry */
			if (descr_lookup == LOAD && 2 < strlen(q+2) && vendor != 0 && device != 0 && e->class_id != 0x90000d) { /* Hub class */
				ifree(e->text); /* usb.c set it so that we display something when usbtable doesn't refer that hw*/
				e->text = strndup(q+2, strlen(q)-4);
			}
			/* if subids read on pcitable line, we know that subids matches :
			   (see "subids differ" test above) */
			if (nb == 4)
				e->already_found = 1;
		}
	}
	fh_close(&f);
	return 1;
}

extern void pciusb_initialize(struct pciusb_entry *e) {
	e->vendor = 0xffff;
	e->device = 0xffff;
	e->subvendor = 0xffff;
	e->subdevice = 0xffff;
	e->class_id = 0;
	e->pci_bus = 0xffff;
	e->pci_device = 0xffff;
	e->pci_function = 0xffff;
	e->module = NULL;
	e->text   = NULL;
	e->already_found = 0;
}

extern void pciusb_free(struct pciusb_entries *entries) {
	unsigned int i;
	for (i = 0; i < entries->nb; i++) {
		struct pciusb_entry *e = &entries->entries[i];
		ifree(e->module);
		ifree(e->text);
	}
	if (entries->nb) ifree(entries->entries);
}
