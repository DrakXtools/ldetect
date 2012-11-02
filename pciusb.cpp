#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <libkmod.h>
#include "common.h"

static void set_modules_from_modalias_file(struct kmod_ctx *ctx, pciusb_entry &e, char *modalias_path) {
	FILE *file;
	file = fopen(modalias_path, "r");
	if (file) {
		char *modalias = NULL;
		size_t n, size;
		if (-1 == getline(&modalias, &n, file)) {
			fprintf(stderr, "Unable to read modalias from %s\n", modalias_path);
			fclose(file);
			return;
		}
		fclose(file);
		size = strlen(modalias);
		if (size)
			modalias[size-1] = 0;

		e.module = modalias_resolve_module(ctx, modalias);
		free(modalias);
	} else {
		fprintf(stderr, "Unable to read modalias from %s\n", modalias_path);
		return;
	}
}

static void find_pci_modules_through_aliases(struct kmod_ctx *ctx, pciusb_entry &e) {
	char *modalias_path;
	asprintf(&modalias_path,
		 "/sys/bus/pci/devices/%04x:%02x:%02x.%x/modalias",
		 e.pci_domain, e.pci_bus, e.pci_device, e.pci_function);
	set_modules_from_modalias_file(ctx, e, modalias_path);
	free(modalias_path);
}

static void find_usb_modules_through_aliases(struct kmod_ctx *ctx, pciusb_entry &e) {
	char *usb_prefix, *sysfs_path;
	DIR *dir;
	struct dirent *dent;

	asprintf(&usb_prefix, "%d-", e.pci_bus);
	/* USB port is indexed from 0 in procfs, from 1 in sysfs */
	asprintf(&sysfs_path, "/sys/bus/usb/devices/%d-%d", e.pci_bus, e.usb_port + 1);

	dir = opendir(sysfs_path);
	if (!dir) {
		goto end;
	}
	while ((dent = readdir(dir)) != NULL) {
		if ((dent->d_type == DT_DIR) &&
		    !strncmp(usb_prefix, dent->d_name, strlen(usb_prefix))) {
			char *modalias_path;
			asprintf(&modalias_path, "%s/%s/modalias", sysfs_path, dent->d_name);
			set_modules_from_modalias_file(ctx, e, modalias_path);
			free(modalias_path);
			/* maybe we would need a "other_modules" field in pciusb_entry 
			   to list modules from all USB interfaces */
			if (!e.module.empty())
				break;
		}
	}
	closedir(dir);
end:
	free(sysfs_path);
	free(usb_prefix);
}
 
static void find_modules_through_aliases_one(struct kmod_ctx *ctx, const char *bus, pciusb_entry &e) {
	if (!strcmp("pci", bus)) {
		find_pci_modules_through_aliases(ctx, e);
	} else if (!strcmp("usb", bus)) {
		find_usb_modules_through_aliases(ctx, e);
	}
}

static void find_modules_through_aliases(const char *bus, std::vector<pciusb_entry> &entries) {
	struct kmod_ctx *ctx = modalias_init();

	for (unsigned int i = 0; i < entries.size(); i++) {
		pciusb_entry &e = entries[i];

		// No special case found in pcitable ? Then lookup modalias for PCI devices
		if (!e.module.empty() && e.module != "unknown")
			continue;
		find_modules_through_aliases_one(ctx, bus, e);
	}

	modalias_cleanup(ctx);
}

int pciusb_find_modules(std::vector<pciusb_entry> &entries, const char *fpciusbtable, const descr_lookup descr_lookup, int is_pci) {
	fh f;
	char buf[2048];

	f = fh_open(fpciusbtable);

	for (int line = 1; fh_gets(buf, sizeof(buf) - 1, &f); line++) {
		unsigned short vendor, device, subvendor, subdevice;
		char *p = NULL, *q = NULL;
		int offset;
		int nb;
		if (buf[0]=='#')
			continue; // skip comments

		nb = sscanf(buf, "0x%hx\t0x%hx\t0x%hx\t0x%hx\t%n", &vendor, &device, &subvendor, &subdevice, &offset);
		if (nb != 4) {
			nb = sscanf(buf, "0x%hx\t0x%hx\t%n", &vendor, &device, &offset);
			if (nb != 2) {
				fprintf(stderr, "%s %d: bad line\n", fpciusbtable, line);
				continue; // skip bad line
			}
		}
		for (unsigned int i = 0; i < entries.size(); i++) {
			pciusb_entry &e = entries[i];
			if (e.already_found)
				continue;	// skip since already found with sub ids
			if (vendor != e.vendor ||  device != e.device)
				continue; // main ids differ

			if (nb == 4 && !(subvendor == e.subvendor && subdevice == e.subdevice))
				continue; // subids differ

			if (!p) { // only calc text & module if not already done
				p = buf + offset + 1;
				q = strchr(p, '\t');
                    if (!q) // no description field?
                         q = strchr(p, '\0') - 1;
			}
			if (strncmp(p, "unknown", q-p-1)) {
				e.module.assign(p,q-p-1);
			}
			/* special case for buggy 0x0 usb entry */
			if (descr_lookup == LOAD && strlen(q) > 1 && 2 < strlen(q+2) && vendor != 0 && device != 0 && e.class_id != 0x90000d) { /* Hub class */
				//ifree(e->text); /* usb.c set it so that we display something when usbtable doesn't refer that hw*/
				e.text.assign(q+2, strlen(q)-4);
			}
			/* if subids read on pcitable line, we know that subids matches :
			   (see "subids differ" test above) */
			if (nb == 4)
				e.already_found = true;
		}
	}
	fh_close(&f);

	find_modules_through_aliases(is_pci ? "pci" : "usb", entries);

	return 1;
}
