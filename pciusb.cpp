#include <iomanip>
#include "common.h"
#include "pciusb.h"

namespace ldetect {

std::ostream& operator<<(std::ostream& os, const pciusbEntry& e) {
    os << std::setw(16) << std::left << (e.module.empty() ? "unknown" : e.module) << ": ";

    if (!e.text.empty())
	os << e.text;
    else
	os << "unknown (" << hexFmt(e.vendor) << "/" << hexFmt(e.device) << "/" << hexFmt(e.subvendor) << "/" << hexFmt(e.subdevice) << ")";

    return os;
}

}
