#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libldetect.h"
#include "libldetect-private.h"
#include "common.h"


extern struct pciusb_entries usb_probe(void) {
	FILE *f;
	char buf[512];
	int line;
	const char *file = "/proc/bus/usb/devices";
	struct pciusb_entry t[100];
	struct pciusb_entries r;
	struct pciusb_entry *e = NULL;

	if (!(f = fopen(file, "r")))
		exit(1);
  
	for(r.nb = line = 0; fgets(buf, sizeof(buf) - 1, f) && r.nb < psizeof(t); line++) {
		if (buf[0] == 'P') {
			e = &t[r.nb++];
			pciusb_initialize(e);

			if (sscanf(buf, "P:  Vendor=%hx ProdID=%hx", &e->vendor, &e->device) != 2) {
				fprintf(stderr, "%s %d: unknown ``P'' line\n", file, line);
				pciusb_initialize(e);
			}
		} else if (e && buf[0] == 'I' && e->class_ == 0) {
			int class_, sub, prot = 0;
			if (sscanf(buf, "I:  If#=%*2d Alt=%*2d #EPs=%*2d Cls=%02x(%*5c) Sub=%02x Prot=%02x", &class_, &sub, &prot) == 3) {
				e->class_ = (class_ * 0x100 + sub) * 0x100 + prot;
			} else {
				fprintf(stderr, "%s %d: unknown ``I'' line\n", file, line);
			}
		} else if (e && buf[0] == 'S') {
			int offset;
			char dummy;
			if (sscanf(buf, "S:  Manufacturer=%n%c", &offset, &dummy) == 1) {
				buf[strlen(buf) - 1] = '|'; /* replacing '\n' by '|' */
				e->text = strdup(buf + offset);
			} else if (sscanf(buf, "S:  Product=%n%c", &offset, &dummy) == 1) {
				if (!e->text) e->text = strdup("Unknown|");
				buf[strlen(buf) - 1] = 0; /* removing '\n' */
				e->text = realloc(e->text, strlen(e->text) + strlen(buf + offset) + 1);
				strcat(e->text, buf + offset);
			}
		}
	}
	fclose(f);
	r.entries = memdup(t, sizeof(struct pciusb_entry) * r.nb);

	pciusb_find_modules(&r, "usbtable", 1 /* no_subid */);
	return r;
}

