#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libldetect.h"

#define psizeof(a) (sizeof(a) / sizeof(*(a)))

void *memdup(void *src, size_t size) {
  void *r = malloc(size);
  memcpy(r, src, size);
  return r;
}
void ifree(void *p) {
  if (p) { free(p); p = NULL; }
}

static void pci_find_modules(struct pci_entries entries) {
  FILE *f;
  const char *fpcitable = "/usr/share/ldetect-lst/pcitable";
  char buf[2048];
  int line;

  if (!(f = fopen(fpcitable, "r"))) {
    fprintf(stderr, "Missing pcitable (should be %s)\n", fpcitable);
    exit(1);
  }  
  for (line = 1; fgets(buf, sizeof(buf) - 1, f); line++) {
    unsigned short vendor, device, subvendor, subdevice;
    int offset, i;
    int nb = sscanf(buf, "0x%hx\t0x%hx\t0x%hx\t0x%hx\t%n", &vendor, &device, &subvendor, &subdevice, &offset);
    if (nb != 4) {
      nb = sscanf(buf, "0x%hx\t0x%hx\t%n", &vendor, &device, &offset);
      if (nb != 2 && buf[0] != '#') {
	fprintf(stderr, "%s %d: bad line\n", fpcitable, line);
	continue;
      }
    }
    for (i = 0; i < entries.nb; i++) {
      struct pci_entry *e = &entries.entries[i];
      if (vendor == e->vendor && device == e->device && 
	  (nb != 4 || (subvendor == e->subvendor && subdevice == e->subdevice)) && !e->module) {
	  char *p = buf + offset + 1;
	  char *q = strchr(p, '\t');
	  if (q) {
	    q[-1] = 0;
	    q[strlen(q)-1] = 0;
	    e->module = strcmp(p, "unknown") ? strdup(p) : NULL;
	    e->text = strdup(q+2);
	  }
      }
    }      
  }
}

extern struct pci_entries pci_probe(int probe_type) {
  FILE *f;
  char buf[512];
  unsigned short devbusfn;
  unsigned int id;
  struct pci_entry t[100];
  struct pci_entries r;

  if (!(f = fopen("/proc/bus/pci/devices", "r"))) exit(1);

  for (r.nb = 0; fgets(buf, sizeof(buf) - 1, f) && r.nb < psizeof(t); r.nb++) {
    sscanf(buf, "%hx %x", &devbusfn, &id);
    t[r.nb].vendor = id >> 16;
    t[r.nb].device = id & 0xffff;
    t[r.nb].module = NULL;
    t[r.nb].text   = NULL;
    {
      char file[25];
      FILE *f;
      snprintf(file, sizeof(file), "/proc/bus/pci/%02x/%02x.%d", devbusfn >> 8, (devbusfn & 0xff) >> 3, (devbusfn & 0xff) & 0x7);
      if (probe_type && (f = fopen(file, "r"))) {
        if (fseek(f, 10, SEEK_SET) == 0) 
	  fread(&t[r.nb].class, 2, 1, f);
        if (fseek(f, 0x2c, SEEK_SET) == 0) 
	  fread(&t[r.nb].subvendor, 2, 1, f), fread(&t[r.nb].subdevice, 2, 1, f);
      } else {
	t[r.nb].subvendor = 0xffff;
	t[r.nb].subdevice = 0xffff;
	t[r.nb].class     = 0;
      }
    }
  }
  fclose(f);
  r.entries = memdup(t, sizeof(struct pci_entry) * r.nb);

  pci_find_modules(r);
  return r;
}

extern void pci_free(struct pci_entries entries) {
  int i;
  for (i = 0; i < entries.nb; i++) {
    struct pci_entry e = entries.entries[i];
    ifree(e.module);
    ifree(e.text);
  }
  ifree(entries.entries);
}

