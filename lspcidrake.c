#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libldetect.h"

void pci_printit(struct pci_entries entries) {
  int i;
  for (i = 0; i < entries.nb; i++) {
    struct pci_entry e = entries.entries[i];
    printf("%s:\t%s", e.module ? e.module : "unknown", e.text);
    if (e.class) {
      const char *class = pci_class2text(e.class);
      if (strcmp(class, "NOT_DEFINED") != 0) printf(" [%s]", class);
    }
    printf("\n");
  }
}

int main(int argc, char **argv) {
  struct pci_entries entries = pci_probe(1);
  pci_printit(entries);
  pci_free(entries);
  exit(0);
}
