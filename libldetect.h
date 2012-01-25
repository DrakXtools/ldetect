#pragma GCC visibility push(default) 
#ifndef LIBLDETECT
#define LIBLDETECT

#include <stdint.h>

/******************************************************************************/
/* pciusb *********************************************************************/
/******************************************************************************/
struct pciusb_entry {
  uint16_t vendor; /* PCI vendor id */
  uint16_t device; /* PCI device id */

  uint16_t subvendor; /* 0xffff if not probe_type'd or no subid */
  uint16_t subdevice; /* 0xffff if not probe_type'd or no subid */
  unsigned long class_id; /* 0 if not probe_type'd ; big because of USB backend */

  uint16_t pci_domain; /* PCI domain id (16 bits wide in libpci) */
  uint8_t pci_bus; /* PCI bus id 8 bits wide */
  uint8_t pci_device; /* PCI device id 5 bits wide */
  uint8_t pci_function; /* PCI function id 3 bits wide */
  uint8_t pci_revision; /* PCI revision 8 bits wide */

  unsigned short usb_port; /* USB port */
  uint8_t is_pciexpress:1; /* is it PCI express */
  uint8_t already_found:1;

  char *module;
  char *text;
  char* class;
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
extern char *get_pci_description(int vendor_id, int device_id);

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
