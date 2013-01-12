#include <iomanip>
#include "common.h"
#include "pciusb.h"

namespace ldetect {

std::ostream& operator<<(std::ostream& os, const pciusbEntry& e) {
    // XXX: setw broken in uClibc++...
#ifndef __UCLIBCXX_MAJOR__
    os << std::setw(16) << std::left << (e.module.empty() ? "unknown" : e.module) << ": ";
#else
    char buf[32];
    snprintf(buf, sizeof(buf), "%-16s: ", e.module.empty() ? "unknown" : e.module.c_str());
    os << buf;
#endif

    if (!e.text.empty())
	os << e.text;
    else
	os << "unknown (" << hexFmt(e.vendor) << "/" << hexFmt(e.device) << "/" << hexFmt(e.subvendor) << "/" << hexFmt(e.subdevice) << ")";

    return os;
}

}
