#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libldetect.h"

void pci_printit(struct pciusb_entries entries) {
  int i;
  for (i = 0; i < entries.nb; i++) {
    struct pciusb_entry e = entries.entries[i];
    printf("%-16s: %s", e.module ? e.module : "unknown", e.text);
    if (e.class) {
      const char *class = pci_class2text(e.class);
      if (strcmp(class, "NOT_DEFINED") != 0) printf(" [%s]", class);
    }
    printf("\n");
  }
}

void usb_printit(struct pciusb_entries entries) {
  int i;
  for (i = 0; i < entries.nb; i++) {
    struct pciusb_entry e = entries.entries[i];
    printf("%-16s: %s", e.module ? e.module : "unknown", e.text);
    if (e.class) printf(" [%s]", usb_class2text(e.class));
    printf("\n");
  }
}

int main(int argc, char **argv) {
  {
    struct pciusb_entries entries = pci_probe(0);
    pci_printit(entries);
    pciusb_free(entries);
  }
  {
    struct pciusb_entries entries = usb_probe();
    usb_printit(entries);
    pciusb_free(entries);
  }
  exit(0);
}
