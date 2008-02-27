#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <modprobe.h>
#include "common.h"

static char *aliasdefault = NULL;

static void set_default_alias_file(void) {
	struct utsname rel_buf;
	if (!aliasdefault) {
		char *dirname;
		char *fallback_aliases = table_name_to_file("fallback-modules.alias");
		char *aliasfilename;
		struct stat st_alias, st_fallback;

		uname(&rel_buf);
		asprintf(&dirname, "%s/%s", MODULE_DIR, rel_buf.release);
		asprintf(&aliasfilename, "%s/modules.alias", dirname);
		/* fallback on ldetect-lst's modules.alias and prefer it if more recent */
		if (stat(aliasfilename, &st_alias) ||
		    (!stat(fallback_aliases, &st_fallback) && st_fallback.st_mtime > st_alias.st_mtime)) {
			aliasdefault = fallback_aliases;
		} else {
			aliasdefault = aliasfilename;
		}
		free(aliasfilename);
		free(dirname);
	}
}

static void resolve_and_set_modules_from_modalias(struct pciusb_entry *e, char *modalias) {
	struct module_command *commands = NULL;
	struct module_options *modoptions = NULL;
	struct module_alias *aliases = NULL;
	struct module_blacklist *blacklist = NULL;
	const char *config = NULL;

	/* Returns the resolved alias, options */
	read_toplevel_config(config, modalias, 0,
			     0, &modoptions, &commands, &aliases, &blacklist);

	if (!aliases) {
		/* We only use canned aliases as last resort. */
		char *alias_filelist[] = {
			"/lib/module-init-tools/ldetect-lst-modules.alias",
			aliasdefault,
			table_name_to_file("dkms-modules.alias"),
			NULL,
		};
		char **alias_file = alias_filelist;
		while (!aliases && *alias_file) {
			read_config(*alias_file, modalias, 0,
				    0, &modoptions, &commands,
				    &aliases, &blacklist);
			aliases = apply_blacklist(aliases, blacklist);
			alias_file++;
		}
	}
	if (aliases) {
		// take the last one because we find eg: generic/ata_generic/sata_sil
		while (aliases->next)
			aliases = aliases->next;
		ifree(e->module);
		e->module = strdup(aliases->module);
	}
}

static void set_modules_from_modalias_file(struct pciusb_entry *e, char *modalias_path) {
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

		resolve_and_set_modules_from_modalias(e, modalias);
		free(modalias);
	} else {
		fprintf(stderr, "Unable to read modalias from %s\n", modalias_path);
		return;
	}
}

static void find_modules_through_aliases_one(const char *bus, struct pciusb_entry *e) {
	char *modalias_path;
	asprintf(&modalias_path,
		 "/sys/bus/pci/devices/%04x:%02x:%02x.%x/modalias",
		 e->pci_domain, e->pci_bus, e->pci_device, e->pci_function);
	set_modules_from_modalias_file(e, modalias_path);
	free(modalias_path);
}

static void find_modules_through_aliases(const char *bus, struct pciusb_entries *entries) {
	set_default_alias_file();

	unsigned int i;
	for (i = 0; i < entries->nb; i++) {
		struct pciusb_entry *e = &entries->entries[i];

		// No special case found in pcitable ? Then lookup modalias for PCI devices
		if (e->module && strcmp(e->module, "unknown"))
			continue;
		find_modules_through_aliases_one(bus, e);
	}
}

extern int pciusb_find_modules(struct pciusb_entries *entries, const char *fpciusbtable, const descr_lookup descr_lookup, int is_pci) {
	fh f;
	char buf[2048];
	int line;

	f = fh_open(fpciusbtable);

	for (line = 1; fh_gets(buf, sizeof(buf) - 1, &f); line++) {
		unsigned short vendor, device, subvendor, subdevice;
		char *p = NULL, *q = NULL;
		int offset; unsigned int i;
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
		for (i = 0; i < entries->nb; i++) {
			struct pciusb_entry *e = &entries->entries[i];
			if (e->already_found)
				continue;	// skip since already found with sub ids
			if (vendor != e->vendor ||  device != e->device)
				continue; // main ids differ

			if (nb == 4 && !(subvendor == e->subvendor && subdevice == e->subdevice))
				continue; // subids differ

			if (!p) { // only calc text & module if not already done
				p = buf + offset + 1;
				q = strchr(p, '\t');
                    if (!q) // no description field?
                         q = strchr(p, '\0') - 1;
			}
			if (strncmp(p, "unknown", q-p-1)) {
				ifree(e->module);
				e->module = strndup(p,q-p-1);
			}
			/* special case for buggy 0x0 usb entry */
			if (descr_lookup == LOAD && 2 < strlen(q+2) && vendor != 0 && device != 0 && e->class_id != 0x90000d) { /* Hub class */
				ifree(e->text); /* usb.c set it so that we display something when usbtable doesn't refer that hw*/
				e->text = strndup(q+2, strlen(q)-4);
			}
			/* if subids read on pcitable line, we know that subids matches :
			   (see "subids differ" test above) */
			if (nb == 4)
				e->already_found = 1;
		}
	}
	fh_close(&f);

	/* If no special case in pcitable, then lookup modalias for PCI devices
	   (USB are already done by kernel)
	*/
	if (is_pci)
		find_modules_through_aliases("pci", entries);

	return 1;
}

extern void pciusb_initialize(struct pciusb_entry *e) {
	e->vendor = 0xffff;
	e->device = 0xffff;
	e->subvendor = 0xffff;
	e->subdevice = 0xffff;
	e->class_id = 0;
	e->pci_bus = 0xffff;
	e->pci_device = 0xffff;
	e->pci_function = 0xffff;
	e->module = NULL;
	e->text   = NULL;
	e->class  = NULL;
	e->already_found = 0;
}

extern void pciusb_free(struct pciusb_entries *entries) {
	unsigned int i;
	for (i = 0; i < entries->nb; i++) {
		struct pciusb_entry *e = &entries->entries[i];
		ifree(e->module);
		ifree(e->text);
	}
	if (entries->nb) ifree(entries->entries);
}
