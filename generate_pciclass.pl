#!/usr/bin/perl

print q(/* This auto-generated from <pci.h>, don't modify! */

struct {
  unsigned short id;
  const char *name;
} pciclasses[] = {
);

/^#define PCI_CLASS_(\w+)\s+(0x\w{4})/ and print qq(  { $2, "$1" },\n) while <>;

print '
};

int nb_pciclasses = sizeof(pciclasses) / sizeof(*pciclasses);

extern const char *pci_class2text(unsigned long class_) {
  int i;
  for (i = 0; i < nb_pciclasses; i++)
    if (pciclasses[i].id == class_) return pciclasses[i].name;

  return pci_class2text(0);
}

';
