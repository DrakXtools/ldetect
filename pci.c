#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libldetect.h"
#include "libldetect-private.h"
#include "common.h"

static void pci_find_modules(struct pciusb_entries entries) {
  char *share_path = getenv("SHARE_PATH");
  char *pcitable;

  if (!share_path || !*share_path) share_path = "/usr/share";
  pcitable = alloca(strlen(share_path) + 32); /* enough for /ldetect-lst/pcitable and EOS */
  sprintf(pcitable, "%s/ldetect-lst/pcitable", share_path);

  pciusb_find_modules(entries, pcitable);
}

extern struct pciusb_entries pci_probe(int probe_type) {
  FILE *f;
  char buf[512];
  unsigned short devbusfn;
  unsigned int id;
  struct pciusb_entry t[100];
  struct pciusb_entries r;

  if (!(f = fopen("/proc/bus/pci/devices", "r"))) exit(1);

  for (r.nb = 0; fgets(buf, sizeof(buf) - 1, f) && r.nb < psizeof(t); r.nb++) {
    struct pciusb_entry e;
    pciusb_initialize(&e);

    sscanf(buf, "%hx %x", &devbusfn, &id);
    e.vendor = id >> 16;
    e.device = id & 0xffff;
    {
      char file[25];
      FILE *f;
      snprintf(file, sizeof(file), "/proc/bus/pci/%02x/%02x.%d", devbusfn >> 8, (devbusfn & 0xff) >> 3, (devbusfn & 0xff) & 0x7);
      if (probe_type && (f = fopen(file, "r"))) {
        if (fseek(f, 10, SEEK_SET) == 0) 
	  fread(&e.class, 2, 1, f);
        if (fseek(f, 0x2c, SEEK_SET) == 0) 
	  fread(&e.subvendor, 2, 1, f), fread(&e.subdevice, 2, 1, f);

	if ((e.subvendor == 0 && e.subdevice == 0) ||
	    (e.subvendor == e.vendor && e.subdevice == e.device)) {
	  e.subvendor = 0xffff;
	  e.subdevice = 0xffff;
	}
	fclose(f);
      }
    }
    t[r.nb] = e;
  }
  fclose(f);
  r.entries = memdup(t, sizeof(struct pciusb_entry) * r.nb);

  pci_find_modules(r);
  return r;
}

