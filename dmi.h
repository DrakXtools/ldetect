#ifndef _LDETECT_DMI
#define _LDETECT_DMI

#include <string>
#include <ostream>

#include "libldetect.h"
#include "intf.h"

namespace ldetect {
    class dmi : public bus, public intf<entry> {
	public:
	    dmi() EXPORTED {}

	    void probe(void) EXPORTED;
    };
}

#pragma GCC visibility pop

#endif
