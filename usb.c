#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "libldetect.h"
#include "common.h"

static char *proc_usb_path_default = "/proc/bus/usb/devices";
char *proc_usb_path = NULL;


extern struct pciusb_entries usb_probe(void) {
	FILE *f;
	char buf[BUF_SIZE];
	int line;
	struct pciusb_entries r;
	struct pciusb_entry *e = NULL;
	r.nb = 0;

	if (!(f = fopen(proc_usb_path ? proc_usb_path : proc_usb_path_default, "r"))) {
		if (proc_usb_path) {
		        char *err_msg;
			asprintf(&err_msg, "unable to open \"%s\"\n"
				    "You may have passed a wrong argument to the \"-u\" option.\n"
				    "fopen() sets errno to", proc_usb_path);
			perror(err_msg);
			free(err_msg);
		}
		r.entries = NULL;
		return r;
	}

	r.entries = malloc(sizeof(struct pciusb_entry) * MAX_DEVICES);
	/* for further information on the format parsed by this state machine,
	 * read /usr/share/doc/kernel-doc-X.Y.Z/usb/proc_usb_info.txt */  
	for(line = 1; fgets(buf, sizeof(buf) - 1, f) && r.nb < MAX_DEVICES; line++) {

		switch (buf[0]) {
		case 'T': {
			unsigned short pci_bus, pci_device;
			e = &r.entries[r.nb++];
			pciusb_initialize(e);

			if (sscanf(buf, "T:  Bus=%02hd Lev=%*02d Prnt=%*02d Port=%*02d Cnt=%*02d Dev#=%3hd Spd=%*3s MxCh=%*2d", &pci_bus, &pci_device) == 2) {
				e->pci_bus = pci_bus;
				e->pci_device = pci_device;
			} else fprintf(stderr, "%s %d: unknown ``T'' line\n", proc_usb_path, line);
			break;
		}
		case 'P': {
			unsigned short vendor, device;
			if (sscanf(buf, "P:  Vendor=%hx ProdID=%hx", &vendor, &device) == 2) {
				e->vendor = vendor;
				e->device = device;
			} else fprintf(stderr, "%s %d: unknown ``P'' line\n", proc_usb_path, line);
			break;
		}
		case 'I': if (e->class_ == 0) {
			char driver[50];
			int class_, sub, prot = 0;
			if (sscanf(buf, "I:  If#=%*2d Alt=%*2d #EPs=%*2d Cls=%02x(%*5c) Sub=%02x Prot=%02x Driver=%s", &class_, &sub, &prot, driver) == 4) {
				e->class_ = (class_ * 0x100 + sub) * 0x100 + prot;
				if (strncmp(driver, "(none)", 6))
					e->module = strdup(driver);
				/* see linux/sound/usb/usbaudio.c::usb_audio_ids */
				if (e->class_ == (0x1*0x100+ 0x01)) /* USB_AUDIO_CLASS*0x100 + USB_SUBCLASS_AUDIO_CONTROL*/
					e->module = "snd-usb-audio";

			} else fprintf(stderr, "%s %d: unknown ``I'' line\n", proc_usb_path, line);
			break;
		}
		case 'S': {
			int offset;
			char dummy;
			size_t length = strlen(buf) -1;
			if (sscanf(buf, "S:  Manufacturer=%n%c", &offset, &dummy) == 1) {
				buf[length] = '|'; /* replacing '\n' by '|' */
				e->text = strdup(buf + offset);
			} else if (sscanf(buf, "S:  Product=%n%c", &offset, &dummy) == 1) {
				if (!e->text) 
					e->text = strdup("Unknown|");
				buf[length] = 0; /* removing '\n' */
				e->text = realloc(e->text, strlen(e->text) + length-offset + 2);
				strcat(e->text, buf + offset);
			}
		}
		}
	}
	fclose(f);
	realloc(r.entries,  sizeof(struct pciusb_entry) * r.nb);

	pciusb_find_modules(&r, "usbtable");
	return r;
}

