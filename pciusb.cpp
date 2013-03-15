#include <iomanip>
#include "common.h"
#include "pciusb.h"

namespace ldetect {

std::ostream& operator<<(std::ostream& os, const pciusbEntry& e) {
    std::string kmodules;
    if (!e.kmodules.empty())
	for (std::vector<std::string>::const_iterator it = e.kmodules.begin();
		it != e.kmodules.end(); ++it) {
	    if (!kmodules.empty())
		kmodules += ",";
	    kmodules += *it;
	}
    os << std::setw(16) << std::left << (kmodules.empty() ? (e.module.empty() ? "unknown" : e.module) : kmodules) << ": ";


    if (!e.text.empty())
	os << e.text;
    else
	os << "unknown (" << hexFmt(e.vendor) << "/" << hexFmt(e.device) << "/" << hexFmt(e.subvendor) << "/" << hexFmt(e.subdevice) << ")";

    return os;
}

}
