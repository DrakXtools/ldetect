#ifndef _LDETECT_DMI
#define _LDETECT_DMI

#include <string>
#include <ostream>

#include "libldetect.h"
#include "interface.h"

namespace ldetect {
    class dmi : public bus, public interface<entry> {
	public:
	    dmi() EXPORTED;
	    ~dmi() EXPORTED;


	    void probe(void) EXPORTED;
    };
}

#pragma GCC visibility pop

#endif
