#include <unistd.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libldetect.h"
#include "libldetect-private.h"
#include "common.h"

char *proc_usb_path = "/proc/bus/usb/devices";


extern struct pciusb_entries usb_probe(void) {
	FILE *f;
	char buf[BUF_SIZE];
	int line;
	struct pciusb_entry t[MAX_DEVICES];
	struct pciusb_entries r;
	struct pciusb_entry *e = NULL;

	if (access(proc_pci_path, R_OK) != 0) {
		r.nb = 0; r.entries = NULL;
		return r;
	}

	if (!(f = fopen(proc_usb_path, "r"))) {
		char *err_msg;
		if (proc_usb_path==NULL || strcmp(proc_usb_path, "/proc/bus/usb/devices")) {
			asprintf(&err_msg, "unable to open \"%s\"\n"
				    "You may have passed a wrong argument to the \"-u\" option.\n"
				    "fopen() sets errno to", proc_usb_path);
			perror(err_msg);
			free(err_msg);
		} /*else {
			asprintf(&err_msg, "unable to open \"%s\"\n"
				    "You should enable the usb service (as root, type 'service usb start'.\n"
				    "fopen() sets errno to", proc_usb_path);
			perror(err_msg);
			free(err_msg);
		} */
		r.nb = 0; r.entries = NULL;
		return r;
	}
  
	for(r.nb = 0, line = 1; fgets(buf, sizeof(buf) - 1, f) && r.nb < psizeof(t); line++) {
		if (buf[0] == 'T') {
		        unsigned short pci_bus, pci_device;
			e = &t[r.nb++];
			pciusb_initialize(e);

			if (sscanf(buf, "T:  Bus=%02hd Lev=%*02d Prnt=%*02d Port=%*02d Cnt=%*02d Dev#=%3hd Spd=%*3s MxCh=%*2d", &pci_bus, &pci_device) == 2) {
				e->pci_bus = pci_bus;
				e->pci_device = pci_device;
			} else {
				fprintf(stderr, "%s %d: unknown ``T'' line\n", proc_usb_path, line);
			}

		} else if (buf[0] == 'P') {
		        unsigned short vendor, device;
			if (sscanf(buf, "P:  Vendor=%hx ProdID=%hx", &vendor, &device) == 2) {
				e->vendor = vendor;
				e->device = device;
			} else {
				fprintf(stderr, "%s %d: unknown ``P'' line\n", proc_usb_path, line);
			}
		} else if (e && buf[0] == 'I' && e->class_ == 0) {
			int class_, sub, prot = 0;
			if (sscanf(buf, "I:  If#=%*2d Alt=%*2d #EPs=%*2d Cls=%02x(%*5c) Sub=%02x Prot=%02x", &class_, &sub, &prot) == 3) {
				e->class_ = (class_ * 0x100 + sub) * 0x100 + prot;
				if (e->class_ == (0x1*0x100+ 0x01)) /* USB_AUDIO_CLASS*0x100 + USB_SUBCLASS_AUDIO_CONTROL*/
					e->module = "snd-usb-audio";

			} else {
				fprintf(stderr, "%s %d: unknown ``I'' line\n", proc_usb_path, line);
			}
		} else if (e && buf[0] == 'S') {
			int offset;
			char dummy;
			size_t length = strlen(buf) -1;
			if (sscanf(buf, "S:  Manufacturer=%n%c", &offset, &dummy) == 1) {
				buf[length] = '|'; /* replacing '\n' by '|' */
				e->text = strdup(buf + offset);
			} else if (sscanf(buf, "S:  Product=%n%c", &offset, &dummy) == 1) {
				if (!e->text) 
					e->text = strdup("Unknown|");
				buf[length - 1] = 0; /* removing '\n' */
				e->text = realloc(e->text, strlen(e->text) + length-offset + 2);
				strcat(e->text, buf + offset);
			}
		}
	}
	fclose(f);
	r.entries = memdup(t, sizeof(struct pciusb_entry) * r.nb);

	pciusb_find_modules(&r, "usbtable", 1 /* no_subid */);
	return r;
}

