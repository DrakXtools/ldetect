/******************************************************************************/
/* pciusb *********************************************************************/
/******************************************************************************/
struct pciusb_entry {
  unsigned short vendor; /* PCI vendor id */
  unsigned short device;

  unsigned short subvendor; /* 0xffff if not probe_type'd or no subid */
  unsigned short subdevice; /* 0xffff if not probe_type'd or no subid */
  unsigned short class; /* 0 if not probe_type'd */

  char *module;
  char *text;
};
struct pciusb_entries {
  struct pciusb_entry *entries;
  int nb;
};

extern void pciusb_free(struct pciusb_entries entries);


/******************************************************************************/
/* pci ************************************************************************/
/******************************************************************************/
extern struct pciusb_entries pci_probe(int probe_type); /* probe_type is boolean */
extern const char *pci_class2text(unsigned short class);

/******************************************************************************/
/* usb ************************************************************************/
/******************************************************************************/
extern struct pciusb_entries usb_probe(void);
extern const char *usb_class2text(unsigned short class);
