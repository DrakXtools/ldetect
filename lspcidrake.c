#include <stdio.h>
#include <string.h>
#include "libldetect.h"

static int verboze = 0;


typedef const char *(f_t)(unsigned long );

static void printit(struct pciusb_entries *entries, f_t find_class) {
	unsigned int i;
	for (i = 0; i < entries->nb; i++) {
		struct pciusb_entry *e = &entries->entries[i];
		printf("%-16s: ", e->module ? e->module : "unknown");
		if (e->text)
			printf(e->text);
		else	printf("unknown (%04x/%04x/%04x/%04x)", e->vendor, e->device, e->subvendor, e->subdevice);
		if (e->class_) {
			const char *class_ = find_class(e->class_);
			if (strcmp(class_, "NOT_DEFINED") != 0) printf(" [%s]", class_);
		}
		if (verboze && e->text) {
			printf(" (vendor:%04x device:%04x", e->vendor, e->device);
			if (e->subvendor != 0xffff || e->subdevice != 0xffff)
				printf(" subv:%04x subd:%04x", e->subvendor, e->subdevice);
			printf(")");
		}
		printf("\n");
	}
}


int main(int argc, char **argv) {
	char ** ptr = argv;
	int full_probe = 0;

	while (ptr && *ptr) {
		if (!strcmp(*ptr, "-h") || !strcmp(*ptr, "--help")) {
			printf("usage: lspcidrake [-v] [-f]\n");	
			return 0;	
		}
		if (!strcmp(*ptr, "-v")) {
			verboze = 1;
			full_probe = 1;
		}
		if (!strcmp(*ptr, "-f"))
			full_probe = 1;
		ptr++;
	}

	struct pciusb_entries entries = pci_probe(full_probe);
	printit(&entries, pci_class2text);
	pciusb_free(&entries);

	entries = usb_probe();
	printit(&entries, usb_class2text);
	pciusb_free(&entries);
	return 0;
}
