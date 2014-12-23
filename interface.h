#ifndef _LDETECT_INTERFACE
#define _LDETECT_INTERFACE

#include <string>
#include <ostream>

#include "libldetect.h"

#pragma GCC visibility push(default)

namespace ldetect {
    template <class T>
    class interface {
	public:
	    interface() : _entries() {}
	    virtual ~interface() {};

	    const T& operator[] (uint16_t i) const noexcept {
		return _entries[i];
	    }

	    operator bool() const noexcept {
		return !_entries.empty();
	    }

	    bool operator! () const noexcept {
		return _entries.empty();
	    }

	    uint16_t size() const noexcept { return _entries.size(); }

	protected:
	    std::vector<T> _entries;
    };
}

#pragma GCC visibility pop

#endif
