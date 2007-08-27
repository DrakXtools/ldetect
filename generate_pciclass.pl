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

#pragma GCC visibility push(default) 
extern const char *pci_class2text(unsigned long class_id) {
  int i;
  for (i = 0; i < nb_classes; i++)
    if (classes[i].id == class_id) return classes[i].name;

  return pci_class2text(0);
}
#pragma GCC visibility pop

';
