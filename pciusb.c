#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "libldetect.h"
#include "libldetect-private.h"
#include "common.h"

typedef struct {
  FILE *f;
  enum { fh_normal, fh_pipe } type;
} fh;

static fh fh_open(char *fname) {
  fh ret;
  int length = strlen(fname);
  char *fname_gz = alloca(length + sizeof(".gz"));
  sprintf(fname_gz, "%s.gz", fname);

  if (access(fname, R_OK) == 0) {
    ret.f = fopen(fname, "r");
    ret.type = fh_normal;
  } else if (access(fname_gz, R_OK) == 0) {    
    char *cmd = alloca(sizeof("gzip -dc %s") + strlen(fname_gz));
    sprintf(cmd, "gzip -dc %s", fname_gz);
    ret.f = popen(cmd, "r");
    ret.type = fh_pipe;
  } else {
    fprintf(stderr, "Missing pciusbtable (should be %s)\n", fname);
    exit(1);
  }
  return ret;
}

static void fh_close(fh f) {
  switch (f.type) 
    {
    case fh_normal: fclose(f.f); break;
    case fh_pipe: pclose(f.f); break;
    }
}

extern int pciusb_find_modules(struct pciusb_entries entries, const char *fpciusbtable, int no_subid) {
  fh f;
  char buf[2048];
  int line;

  char *share_path = getenv("SHARE_PATH");
  char *fname;
  if (!share_path || !*share_path) share_path = "/usr/share";

  fname = alloca(strlen(share_path) + sizeof("/ldetect-lst/") + strlen(fpciusbtable)); 
  sprintf(fname, "%s/ldetect-lst/%s", share_path, fpciusbtable);

  f = fh_open(fname);

  for (line = 1; fgets(buf, sizeof(buf) - 1, f.f); line++) {
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
      if (vendor == e->vendor && device == e->device) {
	if (nb == 4 && e->subvendor == 0xffff && e->subdevice == 0xffff && !no_subid) {
	  pciusb_free(entries);
	  fh_close(f);
	  return 0; /* leave, let the caller call again with subids */
	}

	if ((nb != 4 || (subvendor == e->subvendor && subdevice == e->subdevice)) && !e->module) {
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
  fh_close(f);
  return 1;
}

extern void pciusb_initialize(struct pciusb_entry *e) {
  e->vendor = 0xffff;
  e->device = 0xffff;
  e->subvendor = 0xffff;
  e->subdevice = 0xffff;
  e->class = 0;
  e->pci_bus = 0xffff;
  e->pci_device = 0xffff;
  e->pci_function = 0xffff;
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

