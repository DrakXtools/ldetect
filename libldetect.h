/******************************************************************************/
/* pci ************************************************************************/
/******************************************************************************/
struct pci_entry {
  unsigned short vendor; /* PCI vendor id */
  unsigned short device;

  unsigned short subvendor; /* 0xffff if not probe_type'd or no subid */
  unsigned short subdevice; /* 0xffff if not probe_type'd or no subid */
  unsigned short class; /* 0 if not probe_type'd */

  char *module;
  char *text;
};
struct pci_entries {
  struct pci_entry *entries;
  int nb;
};


extern struct pci_entries pci_probe(int probe_type); /* probe_type is boolean */
extern void pci_free(struct pci_entries entries);

extern const char *pci_class2text(unsigned short class);
