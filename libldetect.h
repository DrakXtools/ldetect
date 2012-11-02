#ifndef LIBLDETECT
#define LIBLDETECT

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>

#pragma GCC visibility push(default) 
#define EXPORTED __attribute__((externally_visible))

namespace ldetect {

class entry {
	public:
		entry(std::string name = "", std::string val = "") : name(name), val(val) {};
		std::string name;
		std::string val;


};

/******************************************************************************/
/* pciusb *********************************************************************/
/******************************************************************************/
class pciusb_entry {
	public:
		pciusb_entry(std::string module = "", std::string text= "", std::string class_type = "") :
			module(module), text(text), class_type(class_type),
			vendor(0xffff), device(0xffff),	subvendor(0xffff), subdevice(0xffff), class_id(0),
			pci_domain(0), pci_bus(0xff), pci_device(0xff), pci_function(0xff), pci_revision(0),
			usb_port(0xffff), is_pciexpress(false), already_found(false) {};

		std::string module;
		std::string text;
		std::string class_type;

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
};

/******************************************************************************/
/* pci ************************************************************************/
/******************************************************************************/
std::vector<pciusb_entry> *pci_probe(void) EXPORTED;
const char *pci_class2text(unsigned long class_id) EXPORTED;
extern const char *proc_pci_path EXPORTED;
const std::string& get_pci_description(uint16_t vendor_id, uint16_t device_id) EXPORTED;

/******************************************************************************/
/* usb ************************************************************************/
/******************************************************************************/
std::vector<pciusb_entry> *usb_probe(void) EXPORTED;

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

/******************************************************************************/
/* dmi ************************************************************************/
/******************************************************************************/

std::vector <entry>* dmi_probe(void) EXPORTED;
extern const char *dmidecode_file EXPORTED;

/******************************************************************************/
/* hid ************************************************************************/
/******************************************************************************/

std::vector <entry>* hid_probe(void) EXPORTED;
extern const char *sysfs_hid_path EXPORTED;

}

#endif
