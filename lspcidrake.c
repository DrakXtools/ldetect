#include <stdio.h>
#include <string.h>
#include "libldetect.h"
#include "libldetect-private.h"

static int verboze = 0;

static void printit(struct pciusb_entries entries, const char *(find_class)(unsigned long )) {
	unsigned int i;
	for (i = 0; i < entries.nb; i++) {
		struct pciusb_entry *e = &entries.entries[i];
		printf("%-16s: ", e->module ? e->module : "unknown");
		if (e->text)
			printf(e->text);
		else	printf("unknown (%04x/%04x/%04x/%04x)", e->vendor, e->device, e->subvendor, e->subdevice);
		if (e->class_) {
			const char *class_ = find_class(e->class_);
			if (strcmp(class_, "NOT_DEFINED") != 0) 
				printf(" [%s]", class_);
		}
		if (verboze && e->text) {
			printf(" (vendor:%04x device:%04x", e->vendor, e->device);
			if (e->subvendor != 0xffff || e->subdevice != 0xffff)
				printf(" subv:%04x subd:%04x", e->subvendor, e->subdevice);
			printf(")");
		}
		printf("\n");
	}
	pciusb_free(&entries);
}


int main(int argc, char **argv) {
	char ** ptr = argv;
	int full_probe = 0;

	while (ptr && *ptr) {
		if (!strcmp(*ptr, "-h") || !strcmp(*ptr, "--help")) {
			printf("usage: lspcidrake [-v|-f|-u]\n"
				"-f : full probe [aka look for pci subids & class]\n"
				"-p : pci devices source [/proc/bus/pci/devices by default\n"
				"-u : usb devices source [/proc/bus/usb/devices by default\n"
				"-v : verbose mode [print ids and sub-ids], implies full probe\n"
				);	
			return 0;	
		}
		if (!strcmp(*ptr, "-v")) {
			verboze = 1;
			full_probe = 1;
		}
		if (!strcmp(*ptr, "-f"))
			full_probe = 1;
		if (!strcmp(*ptr, "-u")) {
			proc_usb_path = *++ptr;
			continue;
		}
		if (!strcmp(*ptr, "-p"))
			proc_pci_path = *++ptr;
		ptr++;
	}

	printit(pci_probe(full_probe), pci_class2text);
	printit(usb_probe(), usb_class2text);
	return 0;
}
