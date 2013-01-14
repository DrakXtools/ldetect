#ifndef _LDETECT_INTF
#define _LDETECT_INTF

#include <string>
#include <ostream>

#include "libldetect.h"

#pragma GCC visibility push(default)

namespace ldetect {
    template <class T>
    class intf {
	public:
	    intf() : _entries() {}
	    virtual ~intf() {};

	    const T& operator[] (uint16_t i) const {
		return _entries[i];
	    }

	    operator bool() const throw() {
		return !_entries.empty();
	    }

	    bool operator! () const throw() {
		return _entries.empty();
	    }

	    uint16_t size() const throw() { return _entries.size(); }

	//protected:
	    std::vector<T> _entries;
    };
}

#pragma GCC visibility pop

#endif
