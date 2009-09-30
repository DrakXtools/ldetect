#pragma GCC visibility push(default) 
#ifndef LIBLDETECT
#define LIBLDETECT

/******************************************************************************/
/* pciusb *********************************************************************/
/******************************************************************************/
struct pciusb_entry {
  unsigned short vendor; /* PCI vendor id */
  unsigned short device;

  unsigned short subvendor; /* 0xffff if not probe_type'd or no subid */
  unsigned short subdevice; /* 0xffff if not probe_type'd or no subid */
  unsigned long class_id; /* 0 if not probe_type'd */

  unsigned short pci_domain; /* pci domain id (16 bits wide in libpci) */
  unsigned short pci_bus; /* pci bus id 8 bits wide */
  unsigned short pci_device; /* pci device id 5 bits wide */
  unsigned short pci_function; /* pci function id 3 bits wide */
  unsigned short pci_revision:8; /* pci revision 8 bits wide */

  unsigned short usb_port; /* pci function id 3 bits wide */
  unsigned short is_pciexpress:1; /* is it PCI express */

  char *module;
  char *text;
  char* class;
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
extern const char *pci_class2text(unsigned long class_id);
extern char *proc_pci_path;

/******************************************************************************/
/* usb ************************************************************************/
/******************************************************************************/
extern struct pciusb_entries usb_probe(void);

struct usb_class_text {
  const char *usb_class_text;
  const char *usb_sub_text;
  const char *usb_prot_text;
};

extern struct usb_class_text usb_class2text(unsigned long class_id);

extern char *proc_usb_path;

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
extern char *dmidecode_file;

/******************************************************************************/
/* hid ************************************************************************/
/******************************************************************************/
struct hid_entry {
  char *module;
  char *text;
};
struct hid_entries {
  struct hid_entry *entries;
  unsigned int nb;
};

extern struct hid_entries hid_probe(void);
extern void hid_entries_free(struct hid_entries *entries);
extern const char *sysfs_hid_path;

#endif
#pragma GCC visibility pop
