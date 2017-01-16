#ifndef _LDETECT_HID
#define _LDETECT_HID

#include <iostream>
#include <iomanip>
#include <string>
#include <ostream>
#include <sstream>

#include "libldetect.h"
#include "interface.h"

#pragma GCC visibility push(default)

namespace ldetect {

    class hid : public bus, public interface<entry> {
	public:
	    hid() EXPORTED;
	    ~hid() EXPORTED;

	    void probe(void) EXPORTED;
    };
}

#pragma GCC visibility pop

#endif
