#include <stdio.h>
#include <iostream>
#include <cassert>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "libldetect.h"
#include "hid.h"
#include "pci.h"
#include "usb.h"
#include "dmi.h"
#ifdef DRAKX_ONE_BINARY
#include "lspcidrake.h"
#endif

namespace ldetect {

static int verboze = 0;

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
int lspcidrake_main(int argc, char *argv[]) {
#else
}

using namespace ldetect;
int main(int argc, char *argv[]) {
#endif

	int opt, fake = 0;
	const char *proc_pci_path = nullptr,
	      *proc_usb_path = nullptr;
	struct option options[] = { { "verbose", 0, nullptr, 'v' },
				    { "pci-file", 1, nullptr, 'p' },
				    { "usb-file", 1, nullptr, 'u' },
				    { "dmidecode", 0, nullptr, 'd' },
				    { nullptr, 0, nullptr, 0 } };

	while ((opt = getopt_long(argc, argv, "vp:u:d", options, nullptr)) != -1) {
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
				//dmidecode_file = optarg;
				fake = 1;
				break;
			default:
				usage();
				return 1;
		}
	}

	ldetect::pci p = proc_pci_path ? ldetect::pci(proc_pci_path) : ldetect::pci();
	p.probe();
	if (!fake) {
	    for (unsigned int i = 0; i < p.size(); i++) {
		const pciEntry &e = p[i];
		std::cout << e;
		if (verboze)
		    std::cout << e.verbose();
		std::cout << e.rev() << std::endl;
	    }
	}

	ldetect::usb u = proc_usb_path ? ldetect::usb(proc_usb_path) : ldetect::usb();
	u.probe();
	if (!fake)
	    for (unsigned int i = 0; i < u.size(); i++)
		std::cout << u[i] << std::endl;

	ldetect::dmi d;
	d.probe();
	if (!fake)
	    for (auto i = 0; i < d.size(); i++)
		std::cout << d[i] << std::endl;

	ldetect::hid h;
	h.probe();
	if (!fake)
	    for (unsigned int i = 0; i < h.size(); i++)
		std::cout << h[i] << std::endl;

	return 0;
}
#ifdef DRAKX_ONE_BINARY
}
#endif

