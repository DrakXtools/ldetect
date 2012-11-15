#ifndef _LDETECT_HID
#define _LDETECT_HID

#include <iostream>
#include <iomanip>
#include <string>
#include <ostream>
#include <sstream>

#include "libldetect.h"

#pragma GCC visibility push(default)

struct kmod_ctx;
struct syfs_attribute;

namespace ldetect {
    class hidEntry {
	public:
	    hidEntry(std::string module, std::string text) EXPORTED :
		module(module), text(text) {}

	    std::string module;
	    std::string text;

	    friend std::ostream& operator<<(std::ostream& os, const hidEntry& e) EXPORTED;
    };

    class hid {
	public:
	    hid(std::string sysfs_hid_path = "/sys/bus/hid/devices") EXPORTED :
		_entries(), _sysfs_hid_path(sysfs_hid_path) {}

	    const hidEntry& operator[] (uint16_t i) const EXPORTED{
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
	    void parse_device(struct kmod_ctx *ctx, const char *dev);

	    std::vector<hidEntry> _entries;
	    std::string _sysfs_hid_path;
    };
}

#pragma GCC visibility pop

#endif
