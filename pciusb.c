#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libldetect.h"
#include "libldetect-private.h"
#include "common.h"

extern void pciusb_find_modules(struct pciusb_entries entries, const char *fpciusbtable) {
  FILE *f;
  char buf[2048];
  int line;

  if (!(f = fopen(fpciusbtable, "r"))) {
    fprintf(stderr, "Missing pciusbtable (should be %s)\n", fpciusbtable);
    exit(1);
  }  
  for (line = 1; fgets(buf, sizeof(buf) - 1, f); line++) {
    unsigned short vendor, device, subvendor, subdevice;
    int offset, i;
    int nb = sscanf(buf, "0x%hx\t0x%hx\t0x%hx\t0x%hx\t%n", &vendor, &device, &subvendor, &subdevice, &offset);
    if (nb != 4) {
      nb = sscanf(buf, "0x%hx\t0x%hx\t%n", &vendor, &device, &offset);
      if (nb != 2 && buf[0] != '#') {
	fprintf(stderr, "%s %d: bad line\n", fpciusbtable, line);
	continue;
      }
    }
    for (i = 0; i < entries.nb; i++) {
      struct pciusb_entry *e = &entries.entries[i];
      if (vendor == e->vendor && device == e->device && 
	  (nb != 4 || (subvendor == e->subvendor && subdevice == e->subdevice)) && !e->module) {
	  char *p = buf + offset + 1;
	  char *q = strchr(p, '\t');
	  if (q) {
	    q[-1] = 0;
	    q[strlen(q)-2] = 0;
	    e->module = strcmp(p, "unknown") ? strdup(p) : NULL;
	    e->text = strdup(q+2);
	  }
      }
    }      
  }
}

extern void pciusb_initialize(struct pciusb_entry *e) {
  e->vendor = 0xffff;
  e->device = 0xffff;
  e->subvendor = 0xffff;
  e->subdevice = 0xffff;
  e->class = 0;
  e->module = NULL;
  e->text   = NULL;
}

extern void pciusb_free(struct pciusb_entries entries) {
  int i;
  for (i = 0; i < entries.nb; i++) {
    struct pciusb_entry e = entries.entries[i];
    ifree(e.module);
    ifree(e.text);
  }
  ifree(entries.entries);
}

