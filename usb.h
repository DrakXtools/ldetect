#ifndef _LDETECT_USB
#define _LDETECT_USB

#include <iomanip>
#include <string>
#include <ostream>
#include <sstream>

#include "libldetect.h"

#include "pciusb.h"
#include "interface.h"
#include "usbnames.h"

#pragma GCC visibility push(default)

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
	    usbEntry() EXPORTED : devpath(), usb_port(0xffff), interfaces(0xffff) {}

	    std::string devpath; /* USB port */

	    uint16_t usb_port; /* USB port */
	    uint16_t interfaces;

	    friend std::ostream& operator<<(std::ostream& os, const usbEntry& e) EXPORTED;
    };

    class usb : public pciusb, public interface<usbEntry> {
	public:
	    usb() EXPORTED;
	    ~usb() EXPORTED;

	    void probe(void) EXPORTED;

	protected:
	    void findModules(std::string &&fpciusbtable, bool descr_lookup);

	private:
	    usbNames _names;
	
    };

}

#pragma GCC visibility pop

#endif
