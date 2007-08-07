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

struct utsname rel_buf;
struct module_command *commands = NULL;
struct module_options *modoptions = NULL;
struct module_alias *aliases = NULL;
struct module_blacklist *blacklist = NULL;
const char *config = NULL;

char *optstring;
char *aliasfilename, *symfilename;
int init;

static void find_modules_through_aliases(struct pciusb_entries *entries) {
     unsigned int i;
     char *dirname;

     uname(&rel_buf);
     asprintf(&dirname, "%s/%s", MODULE_DIR, rel_buf.release);
     asprintf(&aliasfilename, "%s/modules.alias", dirname);
     asprintf(&symfilename, "%s/modules.symbols", dirname);
     if (!init) {
          init = 1;
		read_toplevel_config(config, "", 0, 0,
			     &modoptions, &commands, &aliases, &blacklist);

          // We would need parse the alias db only once for speedup
/*             
		read_toplevel_config(config, "", 1, 0,
			     &modoptions, &commands, &aliases, &blacklist);
		read_config(aliasfilename, "", 1, 0,&modoptions, &commands,
			    &aliases, &blacklist);
		read_config(symfilename, "", 1, 0, &modoptions, &commands,
			    &aliases, &blacklist);
*/
     }

     for (i = 0; i < entries->nb; i++) {
          struct pciusb_entry *e = &entries->entries[i];

          // No special case found in pcitable ? Then lookup modalias for PCI devices
          if (e->module && 0 == strcmp(e->module, "unknown"))
               continue;

          char *modalias = NULL;
          char *modalias_path;
          FILE *file;
          LIST_HEAD(list);
          asprintf(&modalias_path, "/sys/bus/pci/devices/%04x:%02x:%02x.%x/modalias", e->pci_domain, e->pci_bus, e->pci_device, e->pci_function);
          file = fopen(modalias_path, "r");
          if (file) {
               size_t n, size;
               if (-1 == getline(&modalias, &n, file)) {
                    fprintf(stderr, "Unable to read modalias from %s\n", modalias_path);
                    exit(1);
               }
               size = strlen(modalias);
               if (size)
                    modalias[size-1] = 0;
          } else {
	       fprintf(stderr, "Unable to read modalias from %s\n", modalias_path);
               exit(1);
          }
          free(modalias_path);

          /* Returns the resolved alias, options */
          read_toplevel_config(config, modalias, 0,
                               0, &modoptions, &commands, &aliases, &blacklist);

          if (!aliases) {
               /* We only use canned aliases as last resort. */
               read_depends(dirname, modalias, &list);

               if (list_empty(&list)
                   && !find_command(modalias, commands))
               {
                    read_config(aliasfilename, modalias, 0,
                                0, &modoptions, &commands,
                                &aliases, &blacklist);
                    aliases = apply_blacklist(aliases, blacklist);
               }
          }
          if (aliases) {
               // take the last one because we find eg: generic/ata_generic/sata_sil
               while (aliases->next)
                    aliases = aliases->next;
               ifree(e->module);
               e->module = strdup(aliases->module);
               aliases = NULL;
          }
     }
}

extern int pciusb_find_modules(struct pciusb_entries *entries, const char *fpciusbtable, const descr_lookup descr_lookup, int is_pci) {
	fh f;
	char buf[2048];
	int line;

	f = fh_open(fpciusbtable);

	for (line = 1; fgets(buf, sizeof(buf) - 1, f.f); line++) {
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
          find_modules_through_aliases(entries);

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
