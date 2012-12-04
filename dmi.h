#ifndef _LDETECT_DMI
#define _LDETECT_DMI

#include <string>
#include <ostream>

#include "libldetect.h"

#include "pciusb.h"

namespace ldetect {
    class dmiEntry {
	public:
	    dmiEntry(std::string module, std::string text) EXPORTED :
		module(module), text(text) {}

	    std::string module;
	    std::string text;

	    friend std::ostream& operator<<(std::ostream& os, const dmiEntry& e) EXPORTED;
    };

    class dmi {
	public:
	    dmi() EXPORTED : _entries() {}

	    const dmiEntry& operator[] (uint16_t i) const EXPORTED {
		return _entries[i];
	    }

	    operator bool() const throw() EXPORTED {
		return !_entries.empty();
	    }

	    bool operator! () const throw() EXPORTED {
		return _entries.empty();
	    }

	    uint16_t size() const throw() EXPORTED {
		return _entries.size();
	    }

	    void probe(void) EXPORTED;

	private:

	    std::vector<dmiEntry> _entries;
    };
}

#pragma GCC visibility pop

#endif
