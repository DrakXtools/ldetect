#!/usr/bin/perl

print q(/* This auto-generated from <pci.h>, don't modify! */

static struct {
  unsigned short id;
  const char *name;
} classes[] = {
);

/^#define PCI_CLASS_(\w+)\s+(0x\w{4})/ and print qq(  { $2, "$1" },\n) while <>;

print '
};

static int nb_classes = sizeof(classes) / sizeof(*classes);

extern const char *pci_class2text(unsigned long class_) {
  int i;
  for (i = 0; i < nb_classes; i++)
    if (classes[i].id == class_) return classes[i].name;

  return pci_class2text(0);
}

';
