#ifndef _LDETECT_PCIUSB
#define _LDETECT_PCIUSB

#include <iostream>
#include <iomanip>
#include <string>
#include <ostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <cstring>
#include <libkmod.h>

#include "libldetect.h"

#pragma GCC visibility push(hidden) 

namespace ldetect {

    class pciusbEntry {
	public:
	    pciusbEntry(std::string module = "", std::string text= "", std::string class_type = "") :
		module(module), text(text), class_type(class_type),
		vendor(0xffff), device(0xffff),

		subvendor(0xffff), subdevice(0xffff), class_id(0),
		bus(0xff), pciusb_device(0xff),
		already_found(false) {};
	    virtual ~pciusbEntry() {}

	    std::string module;
	    std::string text;
	    std::string class_type;

	    uint16_t vendor; /* PCI vendor id */
	    uint16_t device; /* PCI device id */

	    uint16_t subvendor; /* 0xffff if not probe_type'd or no subid */
	    uint16_t subdevice; /* 0xffff if not probe_type'd or no subid */
	    uint32_t class_id; /* 0 if not probe_type'd ; big because of USB backend */

	    uint8_t bus; /* PCI bus id 8 bits wide */
	    uint8_t pciusb_device; /* PCI device id 5 bits wide */


	    friend std::ostream& operator<<(std::ostream& os, const pciusbEntry& e);

	//protected:
	    bool already_found;
};

    class pciusb : public bus {
	public:
	    pciusb() : bus() {}
	    virtual ~pciusb() {}

	protected:
	    virtual void findModules(std::string &&fpciusbtable, bool descr_lookup) = 0;

    };
}

#pragma GCC visibility pop

#endif
