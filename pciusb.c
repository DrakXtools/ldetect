#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libldetect.h"
#include "libldetect-private.h"
#include "common.h"

typedef struct {
  FILE *f;
  pid_t pid;
} fh;

static fh fh_open(char *fname) {
  fh ret;
  int length = strlen(fname);
  char *fname_gz = alloca(length + sizeof(".gz"));
  sprintf(fname_gz, "%s.gz", fname);

  if (access(fname, R_OK) == 0) {
    ret.f = fopen(fname, "r");
    ret.pid = 0;
  } else if (access(fname_gz, R_OK) == 0) {
    int fdno[2];
    if (pipe(fdno)) { perror("pciusb"); exit(1); }
    if ((ret.pid = fork()) != 0) {
      ret.f = fdopen(fdno[0], "r");
      close(fdno[1]);
    } else {
      char *cmd[8];
      int ip = 0;
      char *ld_loader = getenv("LD_LOADER");

      if (ld_loader && *ld_loader) {
	cmd[ip++] = ld_loader;
      }
      cmd[ip++] = "gzip";
      cmd[ip++] = "-dc";
      cmd[ip++] = fname_gz;
      cmd[ip++] = NULL;

      dup2(fdno[1], STDOUT_FILENO);
      close(fdno[0]);
      close(fdno[1]);
      execvp(cmd[0], cmd);
      perror("pciusb"); exit(2);
    }
  } else {
    fprintf(stderr, "Missing pciusbtable (should be %s)\n", fname);
    exit(1);
  }
  return ret;
}

static void fh_close(fh f) {
  fclose(f.f);
  if (f.pid > 0) waitpid(f.pid, NULL, 0);
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
      if (nb != 2) {
	if (buf[0] != '#') fprintf(stderr, "%s %d: bad line\n", fpciusbtable, line);
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
	    ifree(e->module); ifree(e->text);
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

