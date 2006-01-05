#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pci/pci.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include "common.h"

static char *proc_pci_path_default = "/proc/bus/pci/devices";
char *proc_pci_path = NULL;

#define MAX_PCI_DOMAINS 256
static int pci_domains[MAX_PCI_DOMAINS] = { 0, };

static int probe_domains(void)
{
    static int n_pci_domains = -1;
    if (n_pci_domains == -1) {
	int i;
	DIR *dir;
	for (i = 0; i < MAX_PCI_DOMAINS; i++)
	    pci_domains[i] = 0;
	if ((dir = opendir("/proc/bus/pci")) != NULL) {
	    struct dirent *dent;
	    while ((dent = readdir(dir)) != NULL) {
		int domain, bus, ret;
		ret = sscanf(dent->d_name, "%x:%x", &domain, &bus);
		if (ret == 2) {
		    if (domain < MAX_PCI_DOMAINS)
			pci_domains[domain] = 1;
		}
	    }
	    closedir(dir);
	    n_pci_domains = 0;
	    for (i = 0; i < MAX_PCI_DOMAINS; i++) {
		if (pci_domains[i])
		    pci_domains[n_pci_domains++] = i;
	    }
	}
    }
    return n_pci_domains;
}

extern struct pciusb_entries pci_probe(void) {
	FILE *f; int devf;
	char buf[BUF_SIZE];
	unsigned short *bufi = (unsigned short *) &buf;
	unsigned short devbusfn;
	unsigned int id;
	struct pciusb_entries r;
	char file[32];
	int n_pci_domains;

	n_pci_domains = probe_domains();

	r.nb = 0;
	if (!(f = fopen(proc_pci_path ? proc_pci_path : proc_pci_path_default, "r"))) {
	        if (proc_pci_path) {
		    char *err_msg;
		    asprintf(&err_msg, "unable to open \"%s\"\n"
			     "You may have passed a wrong argument to the \"-p\" option.\n"
			     "fopen() sets errno to", proc_pci_path);
		    perror(err_msg);
		    free(err_msg);
		}
		r.nb = 0;
		r.entries = NULL; // is that needed?
		return r;
	}
	r.entries = malloc(sizeof(struct pciusb_entry) * MAX_DEVICES);

	for (; fgets(buf, sizeof(buf) - 1, f) && r.nb < MAX_DEVICES; r.nb++) {
		struct pciusb_entry *e = &r.entries[r.nb];
		unsigned char class_prog = 0;
		pciusb_initialize(e);

		sscanf(buf, "%hx %x", &devbusfn, &id);
		e->vendor = id >> 16;
		e->device = id & 0xffff;
		e->pci_bus = devbusfn >> 8;
		e->pci_device = (devbusfn & 0xff) >> 3;
		e->pci_function = (devbusfn & 0xff) & 0x07;
		snprintf(file, sizeof(file), "/proc/bus/pci/%02x/%02x.%d", e->pci_bus, e->pci_device, e->pci_function);
		if ((devf = open(file, O_RDONLY)) == -1) {
		    /* try with pci domains */
		    int found = 0;
		    if (n_pci_domains) {
			int i;
			for (i = 0; !found && i < n_pci_domains; i++) {
			    snprintf(file, sizeof(file), "/proc/bus/pci/%04x:%02x/%02x.%d", pci_domains[i], e->pci_bus, e->pci_device, e->pci_function);
			    if ((devf = open(file, O_RDONLY)) >= 0)
				found = 1;
			}
		    }
		    if (!found)
			continue;
		}
		read(devf, &buf, 0x30); /* these files're 256 bytes but we only need first 48 bytes*/
		e->class_ = bufi[0x5];
		e->subvendor = bufi[0x16];
		e->subdevice = bufi[0x17];
				
		if ((e->subvendor == 0 && e->subdevice == 0) ||
		    (e->subvendor == e->vendor && e->subdevice == e->device)) {
			e->subvendor = 0xffff;
			e->subdevice = 0xffff;
		}
		class_prog = buf[0x9];

		/* special rules below must be in sync with gi/mdk-stage1/probing.c */

		if (e->class_ == PCI_CLASS_SERIAL_USB) {
		  /* taken from kudzu's pci.c */
		  char *module = 
		    class_prog == 0 ? "usb-uhci" : 
		    class_prog == 0x10 ? "usb-ohci" :
		    class_prog == 0x20 ? "ehci-hcd" : NULL;
		  if (module) e->module = strdup(module);

		} else if (e->class_ == PCI_CLASS_SERIAL_FIREWIRE) {    
		  /* taken from kudzu's pci.c */
		  if (class_prog == 0x10) e->module = strdup("ohci1394");

		} else if (e->class_ == PCI_CLASS_BRIDGE_CARDBUS) {
			e->module = strdup("yenta_socket");
		} else if (e->class_ == (PCI_CLASS_BRIDGE_HOST << 8)) { /* AGP */
               if (e->vendor == 0x10b9)
                    e->module = strdup("ali-agp");
               else if (e->vendor == 0x1002)
                    e->module = strdup("ati-agp");
               else if (e->vendor == 0x1039)
                    e->module = strdup("sis-agp");
               else if (e->vendor == 0x1166)
                    e->module = strdup("sworks-agp");
               else if (e->vendor == 0x1279)
                    e->module = strdup("efficeon-agp");
		} else if (e->device == 0x8139) {
               if (e->subvendor == 0x8139 && e->subdevice == 0x10ec
                   || e->subvendor == 0x1186 && e->subdevice == 0x1300
                   || e->subvendor == 0x13d1 && e->subdevice == 0xab06)
                    e->module = strdup("8139too");
		} else if (e->vendor == 0x10de && e->class_ == PCI_CLASS_STORAGE_IDE) {
			e->module = strdup("sata_nv");
		} else if (e->vendor == 0x10b5 && (e->device == 0x9030 || e->device == 0x9050) && e->subvendor == 0x1369) {
			e->module = strdup("snd-vx222");
		} else if (e->vendor == 0x1119) {   /* Vortex only makes RAID controllers. */
			e->module = strdup("gdth");
		}

		close(devf);
	}
	fclose(f);
	realloc(r.entries,  sizeof(struct pciusb_entry) * r.nb);

	if (pciusb_find_modules(&r, "pcitable"))
		return r;

	/* should not happen */
	exit(1);
}
