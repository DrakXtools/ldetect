/******************************************************************************/
/* pciusb *********************************************************************/
/******************************************************************************/
struct pciusb_entry {
  unsigned short vendor; /* PCI vendor id */
  unsigned short device;

  unsigned short subvendor; /* 0xffff if not probe_type'd or no subid */
  unsigned short subdevice; /* 0xffff if not probe_type'd or no subid */
  unsigned long class_; /* 0 if not probe_type'd */

  unsigned short pci_bus; /* pci bus id 8 bits wide */
  unsigned short pci_device; /* pci device id 5 bits wide */
  unsigned short pci_function; /* pci function id 3 bits wide */

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
extern const char *pci_class2text(unsigned long class_);

/******************************************************************************/
/* usb ************************************************************************/
/******************************************************************************/
extern struct pciusb_entries usb_probe(void);
extern const char *usb_class2text(unsigned long class_);
