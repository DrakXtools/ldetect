#ifndef _LDETECT_USB
#define _LDETECT_USB

#include <iostream>
#include <iomanip>
#include <string>
#include <ostream>
#include <sstream>

#include "libldetect.h"

#include "pciusb.h"

#pragma GCC visibility push(default)

struct pci_access;
struct pci_dev;
struct kmod_ctx;

namespace ldetect {

    struct usb_class_text {
	usb_class_text(std::string class_text="", std::string sub_text="", std::string prot_text="") EXPORTED :
	    class_text(class_text), sub_text(sub_text), prot_text(prot_text) {}

	std::string class_text;
	std::string sub_text;
	std::string prot_text;
    };

    struct usb_class_text usb_class2text(uint32_t class_id) EXPORTED;

    class usbEntry : public pciusbEntry {
	public:
	    usbEntry() EXPORTED : usb_port(0xffff) {}

	    std::string devpath; /* USB port */

	    uint16_t usb_port; /* USB port */
	    uint16_t interfaces;

	    friend std::ostream& operator<<(std::ostream& os, const usbEntry& e) EXPORTED;
    };

    class usb : public pciusb<usbEntry> {
	public:
	    usb(std::string proc_usb_path="/sys/kernel/debug/usb/devices") EXPORTED;
	    ~usb() EXPORTED;

	    const std::string& getDescription(uint16_t vendor_id, uint16_t device_id) EXPORTED;
	    void probe(void) EXPORTED;

	protected:
	    void find_modules_through_aliases(struct kmod_ctx *ctx, usbEntry &e);

	
	private:
	    std::string _proc_usb_path;
    };

}

#pragma GCC visibility pop

#endif
