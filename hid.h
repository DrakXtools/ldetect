#ifndef _LDETECT_HID
#define _LDETECT_HID

#include <iostream>
#include <iomanip>
#include <string>
#include <ostream>
#include <sstream>

#include "libldetect.h"
#include "intf.h"

#pragma GCC visibility push(default)

namespace ldetect {

    class hid : public bus, public intf<entry> {
	public:
	    hid() EXPORTED {}

	    void probe(void) EXPORTED;
    };
}

#pragma GCC visibility pop

#endif
