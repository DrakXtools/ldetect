#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "libldetect.h"
#include "common.h"
#include "names.h"

static const char proc_usb_path_default[] = "/sys/kernel/debug/usb/devices";
const char *proc_usb_path = NULL;

static void build_text(struct pciusb_entry *e, char *vendor_text, char *product_text) {
	if(e) {
		if(!vendor_text) {
			const char *vendorname = names_vendor(e->vendor);
			if(vendorname) {
				vendor_text = (char*)malloc(strlen(vendorname)+2);
				sprintf(vendor_text, "%s|", vendorname);
			} else 
				vendor_text = strdup("Unknown|");
		}
		if(!product_text) {
			const char *productname = names_product(e->vendor, e->device);
			if(productname)
				product_text = productname ? strdup(productname) : strdup("Unknown");
		}
		vendor_text = (char*)realloc(vendor_text, strlen(vendor_text)+strlen(product_text)+1);
		e->text = vendor_text;
		strcat(e->text, product_text);
		free(product_text);
	}
}


struct pciusb_entries usb_probe(void) {
	FILE *f;
	char buf[BUF_SIZE];
	int line;
	struct pciusb_entries r;
	struct pciusb_entry *e = NULL;
	char *vendor_text = NULL, *product_text = NULL;
	r.nb = 0;

	names_init("/usr/share/usb.ids");
	if (!(f = fopen(proc_usb_path ? proc_usb_path : proc_usb_path_default, "r"))) {
		if (proc_usb_path) {
			fprintf(stderr, "unable to open \"%s\"\n: %s"
				    "You may have passed a wrong argument to the \"-u\" option.\n",
				    strerror(errno), proc_usb_path);
		}
		r.entries = NULL;
		goto exit;
	}

	r.entries = (struct pciusb_entry*)malloc(sizeof(struct pciusb_entry) * MAX_DEVICES);
	/* for further information on the format parsed by this state machine,
	 * read /usr/share/doc/kernel-doc-X.Y.Z/usb/proc_usb_info.txt */  
	for(line = 1; fgets(buf, sizeof(buf) - 1, f) && r.nb < MAX_DEVICES; line++) {

		switch (buf[0]) {
		case 'T': {
			build_text(e, vendor_text, product_text);
			vendor_text = NULL;
			product_text = NULL;
			e = &r.entries[r.nb++];
			pciusb_initialize(e);

			if (!sscanf(buf, "T:  Bus=%02hhu Lev=%*02d Prnt=%*04d Port=%02hu Cnt=%*02d Dev#=%3hhu Spd=%*3s MxCh=%*2d", &e->pci_bus, &e->usb_port, &e->pci_device) == 3)
				fprintf(stderr, "%s %d: unknown ``T'' line\n", proc_usb_path, line);
			break;
		}
		case 'P': {
			if (!sscanf(buf, "P:  Vendor=%hx ProdID=%hx", &e->vendor, &e->device) == 2)
				fprintf(stderr, "%s %d: unknown ``P'' line\n", proc_usb_path, line);
			break;
		}
		case 'I': if (e->class_id == 0 || e->module == NULL) {
			char driver[50];
			unsigned int class_id, sub, prot = 0;
			if (sscanf(buf, "I:* If#=%*2d Alt=%*2d #EPs=%*2d Cls=%02x(%*5c) Sub=%02x Prot=%02x Driver=%s", &class_id, &sub, &prot, driver) >= 3) {
				unsigned long cid = (class_id * 0x100 + sub) * 0x100 + prot;
				if (e->class_id == 0)
					e->class_id = cid;
				if (strncmp(driver, "(none)", 6)) {
					char *p = strdup(driver);
					/* Get current class if we are on the first one having used by a driver */
					e->class_id = cid;
					e->module = p;
					/* replace '-' characters with '_' to be compliant with modnames from modaliases */
					while (p && *p) {
						if (*p == '-') *p = '_';
						p++;
					}
				}
				/* see linux/sound/usb/usbaudio.c::usb_audio_ids */
				if (e->class_id == (0x1*0x100+ 0x01)) /* USB_AUDIO_CLASS*0x100 + USB_SUBCLASS_AUDIO_CONTROL*/
					e->module = strdup("snd_usb_audio");

			} else if (sscanf(buf, "I:  If#=%*2d Alt=%*2d #EPs=%*2d Cls=%02x(%*5c) Sub=%02x Prot=%02x Driver=%s", &class_id, &sub, &prot, driver) >= 3) {
				/* Ignore interfaces not active */
			} else fprintf(stderr, "%s %d: unknown ``I'' line\n", proc_usb_path, line);
			break;
		}
		case 'S': {
			int offset;
			char dummy;
			size_t length = strlen(buf) - 1;
			if (sscanf(buf, "S:  Manufacturer=%n%c", &offset, &dummy) == 1) {
				buf[length] = '|'; /* replacing '\n' by '|' */
				vendor_text = strdup(buf + offset);
			} else if (sscanf(buf, "S:  Product=%n%c", &offset, &dummy) == 1) {
				buf[length] = 0; /* removing '\n' */
				product_text = strdup(buf + offset);
			}
		}
		}
	}

	build_text(e, vendor_text, product_text);

	fclose(f);

	/* shrink to real size */
	r.entries = (struct pciusb_entry*)realloc(r.entries,  sizeof(struct pciusb_entry) * r.nb);

	pciusb_find_modules(&r, "usbtable", DO_NOT_LOAD, 0);

exit:
	names_exit();
	return r;
}

