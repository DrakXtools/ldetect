#include <stdio.h>
#include <iostream>
#include <cassert>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "libldetect.h"
#ifdef DRAKX_ONE_BINARY
#include "lspcidrake.h"
#endif

namespace ldetect {

static int verboze = 0;

static void printit(std::vector<pciusb_entry> *entries, void print_class(unsigned long )) {
	for (unsigned int i = 0; i < entries->size(); i++) {
		pciusb_entry &e = (*entries)[i];
		printf("%-16s: ", e.module.empty() ? "unknown": e.module.c_str());
		if (!e.text.empty())
			printf("%s", e.text.c_str());
		else	printf("unknown (%04x/%04x/%04x/%04x)", e.vendor, e.device, e.subvendor, e.subdevice);
		if (e.class_id) {
			print_class(e.class_id);
		}
		if (verboze && !e.text.empty()) {
			printf(" (vendor:%04x device:%04x", e.vendor, e.device);
			if (e.subvendor != 0xffff || e.subdevice != 0xffff)
				printf(" subv:%04x subd:%04x", e.subvendor, e.subdevice);
			printf(")");
		}
		if (e.pci_revision)
                     printf(" (rev: %02x)", e.pci_revision);
		printf("\n");
	}
	delete entries;
}

static void print_pci_class(unsigned long class_id) {
    const std::string &s = pci_class2text(class_id);
    if (s != "NOT_DEFINED")
	std::cout << " [" << s << "]" << std::endl;
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

static void print_dmi_hid_entries(std::vector<entry> *entries) {
	for (std::vector<entry>::const_iterator entry = entries->begin();
			entry < entries->end();
			++entry)
		printf("%-16s: %s\n", entry->name.c_str(), entry->val.c_str());
	delete entries;
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

#ifdef DRAKX_ONE_BINARY
int lspcidrake_main(int argc, char **argv) {
#else
}

using namespace ldetect;
int main(int argc, char **argv) {
#endif

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
	
	if ((!fake && geteuid() == 0) || dmidecode_file) {
		std::vector<entry> *dmi_entries = dmi_probe();
		if(dmi_entries)
			print_dmi_hid_entries(dmi_entries);
	}

	if (!fake || sysfs_hid_path) {
		std::vector<entry> *hid_entries = hid_probe();
		if (hid_entries)
			print_dmi_hid_entries(hid_entries);
	}

	return 0;
}
#ifdef DRAKX_ONE_BINARY
}
#endif

