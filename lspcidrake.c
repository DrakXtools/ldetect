#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "libldetect.h"

static int verboze = 0;

static void printit(struct pciusb_entries entries, void print_class(unsigned long )) {
	unsigned int i;
	for (i = 0; i < entries.nb; i++) {
		struct pciusb_entry *e = &entries.entries[i];
		printf("%-16s: ", e->module ? e->module : "unknown");
		if (e->text)
			printf("%s", e->text);
		else	printf("unknown (%04x/%04x/%04x/%04x)", e->vendor, e->device, e->subvendor, e->subdevice);
		if (e->class_id) {
			print_class(e->class_id);
		}
		if (verboze && e->text) {
			printf(" (vendor:%04x device:%04x", e->vendor, e->device);
			if (e->subvendor != 0xffff || e->subdevice != 0xffff)
				printf(" subv:%04x subd:%04x", e->subvendor, e->subdevice);
			printf(")");
		}
		if (e->pci_revision)    
                     printf(" (rev: %02x)", e->pci_revision);
		printf("\n");
	}
	pciusb_free(&entries);
}

static void print_pci_class(unsigned long class_id) {
  const char *s = pci_class2text(class_id);
  if (strcmp(s, "NOT_DEFINED") != 0) 
     printf(" [%s]", s);
}

static void print_usb_class(unsigned long class_id) {
  struct usb_class_text s = usb_class2text(class_id);
  if (s.usb_class_text) {
    printf(" [");
    printf("%s", s.usb_class_text);
    if (s.usb_sub_text) printf("|%s", s.usb_sub_text);
    if (s.usb_prot_text) printf("|%s", s.usb_prot_text);
    printf("]");
  }
}

static void print_dmi_entries(struct dmi_entries entries) {
	unsigned int i;
	for (i = 0; i < entries.nb; i++)
		printf("%-16s: %s\n", entries.entries[i].module, entries.entries[i].constraints);
}

static void print_hid_entries(struct hid_entries entries) {
	unsigned int i;
	for (i = 0; i < entries.nb; i++)
		printf("%-16s: %s\n", entries.entries[i].module,
		       entries.entries[i].text);	 
}

static void usage(void)
{
	printf(
	"usage: lspcidrake [options]\n"
	"\t-p, --pci-file <file>\tPCI devices source [/proc/bus/pci/devices by default]\n"
	"\t-u, --usb-file <file>\tUSB devices source  [/proc/bus/usb/devices by default]\n"
	"\t-v, --verbose\t\tVerbose mode [print ids and sub-ids], implies full probe\n"
	"\t-d, --dmidecode <file>\tTo use this dmidecode output instead of calling demicode\n");
}

int main(int argc, char **argv) {
	int opt, fake = 0;
	struct option options[] = { { "verbose", 0, NULL, 'v' },
				    { "pci-file", 1, NULL, 'p' },
				    { "usb-file", 1, NULL, 'u' },
				    { "dmidecode", 0, NULL, 'd' },
				    { NULL, 0, NULL, 0 } };

	while ((opt = getopt_long(argc, argv, "vp:u:d", options, NULL)) != -1) {
		switch (opt) {
			case 'v':
				verboze = 1;
				break;
			case 'p':
				proc_pci_path = optarg;
				fake = 1;
				break;
			case 'u':
				proc_usb_path = optarg;
				fake = 1;
				break;
			case 'd':
				dmidecode_file = optarg;
				fake = 1;
				break;
			default:
				usage();
				return 1;
		}
	}

	if (!fake || proc_pci_path) printit(pci_probe(), print_pci_class);
	if (!fake || proc_usb_path) printit(usb_probe(), print_usb_class);
	
	if (!fake && geteuid() == 0 || dmidecode_file) {
	    struct dmi_entries dmi_entries = dmi_probe();
	    print_dmi_entries(dmi_entries);
	    dmi_entries_free(dmi_entries);
	}

	if (!fake || sysfs_hid_path) {
	    struct hid_entries hid_entries = hid_probe();
	    print_hid_entries(hid_entries);
	    hid_entries_free(&hid_entries);
	}

	return 0;
}
