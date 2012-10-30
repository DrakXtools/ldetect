#pragma GCC visibility push(default) 
#ifndef LIBLDETECT
#define LIBLDETECT

#include <stdint.h>
#include <stdbool.h>

#define EXPORTED __attribute__((externally_visible))

struct entry {
	union {
		char *module;
		char *name;
	};
	union {
		char *constraints;
		char *text;
		char *val;
	};
};

struct entries {
	unsigned int nb;
	union {
		struct entry *entries;
		struct entry *criteria;
	};
};

void free_entries(struct entries entries) EXPORTED;

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
  bool is_pciexpress:1; /* is it PCI express */
  bool already_found:1;

  char *module;
  char *text;
  char* class;
};
struct pciusb_entries {
  unsigned int nb;
  struct pciusb_entry *entries;
};

void pciusb_free(struct pciusb_entries entries) EXPORTED;


/******************************************************************************/
/* pci ************************************************************************/
/******************************************************************************/
struct pciusb_entries pci_probe(void) EXPORTED;
const char *pci_class2text(unsigned long class_id) EXPORTED;
extern const char *proc_pci_path EXPORTED;
char *get_pci_description(int vendor_id, int device_id) EXPORTED;

/******************************************************************************/
/* usb ************************************************************************/
/******************************************************************************/
struct pciusb_entries usb_probe(void) EXPORTED;

struct usb_class_text {
  const char *usb_class_text;
  const char *usb_sub_text;
  const char *usb_prot_text;
};

struct usb_class_text usb_class2text(unsigned long class_id) EXPORTED;

extern const char *proc_usb_path EXPORTED;

/******************************************************************************/
/* dmi & hid ******************************************************************/
/******************************************************************************/

typedef struct entry dmi_hid_entry_t;
typedef struct entry criterion_t;
typedef struct entries dmi_hid_entries_t;
typedef struct entries criteria_t;

/******************************************************************************/
/* dmi ************************************************************************/
/******************************************************************************/

struct entries dmi_probe(void) EXPORTED;
extern const char *dmidecode_file EXPORTED;

/******************************************************************************/
/* hid ************************************************************************/
/******************************************************************************/

struct entries hid_probe(void) EXPORTED;
extern const char *sysfs_hid_path EXPORTED;

#endif
#pragma GCC visibility pop
