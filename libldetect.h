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
  int already_found:1;
};
struct pciusb_entries {
  struct pciusb_entry *entries;
  unsigned int nb;
};

extern void pciusb_free(struct pciusb_entries *entries);


/******************************************************************************/
/* pci ************************************************************************/
/******************************************************************************/
extern struct pciusb_entries pci_probe(void);
extern const char *pci_class2text(unsigned long class_);

/******************************************************************************/
/* usb ************************************************************************/
/******************************************************************************/
extern struct pciusb_entries usb_probe(void);

struct usb_class_text {
  const char *usb_class_text;
  const char *usb_sub_text;
  const char *usb_prot_text;
};

extern struct usb_class_text usb_class2text(unsigned long class_);

/******************************************************************************/
/* dmi ************************************************************************/
/******************************************************************************/
struct dmi_entry {
  char *constraints;
  char *module;
};
struct dmi_entries {
  struct dmi_entry *entries;
  unsigned int nb;
};

extern struct dmi_entries dmi_probe(void);
extern void dmi_entries_free(struct dmi_entries entries);
