#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libldetect.h"

int verboze = 0;
int full_probe = 0;

void usage(void) {
  printf("usage: lspcidrake [-v] [-f]\n");
  exit(0);
}

void print_name(struct pciusb_entry *e) {
  printf("%-16s: ", e->module ? e->module : "unknown");
  if (e->text)
    printf(e->text);
  else
    printf("unknown (%04x/%04x/%04x/%04x)", e->vendor, e->device, e->subvendor, e->subdevice);
}

void print_id(struct pciusb_entry *e) {
  if (verboze && e->text) {
    printf(" (vendor:%04x device:%04x", e->vendor, e->device);
    if (e->subvendor != 0xffff || e->subdevice != 0xffff)
      printf(" subv:%04x subd:%04x", e->subvendor, e->subdevice);
    printf(")");
  }
}

void pci_printit(struct pciusb_entries *entries) {
  int i;
  for (i = 0; i < entries->nb; i++) {
    struct pciusb_entry *e = &entries->entries[i];
    print_name(e);
    if (e->class_) {
      const char *class_ = pci_class2text(e->class_);
      if (strcmp(class_, "NOT_DEFINED") != 0) printf(" [%s]", class_);
    }
    print_id(e);
    printf("\n");
  }
}

void usb_printit(struct pciusb_entries *entries) {
  int i;
  for (i = 0; i < entries->nb; i++) {
    struct pciusb_entry *e = &entries->entries[i];
    print_name(e);
    if (e->class_) printf(" [%s]", usb_class2text(e->class_));
    print_id(e);
    printf("\n");
  }
}

int main(int argc, char **argv) {
  char ** ptr = argv;
  while (ptr && *ptr) {
    if (!strcmp(*ptr, "-h") || !strcmp(*ptr, "--help"))
      usage();
    if (!strcmp(*ptr, "-v")) {
      verboze = 1;
      full_probe = 1;
    }
    if (!strcmp(*ptr, "-f"))
      full_probe = 1;
    ptr++;
  }

  struct pciusb_entries entries = pci_probe(full_probe);
  pci_printit(&entries);
  pciusb_free(&entries);

  entries = usb_probe();
  usb_printit(&entries);
  pciusb_free(&entries);
  exit(0);
}
