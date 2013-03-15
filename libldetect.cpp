#include <iomanip>
#include "common.h"
#include "libldetect.h"

namespace ldetect {

std::ostream& operator<<(std::ostream& os, const entry& e) {
    std::string kmodules;
    if (!e.kmodules.empty())
	for (std::vector<std::string>::const_iterator it = e.kmodules.begin();
		it != e.kmodules.end(); ++it) {
	    if (!kmodules.empty())
		kmodules += ",";
	    kmodules += *it;
	}
    return os << std::setw(16) << std::left << (kmodules.empty() ? (e.module.empty() ? "unknown" : e.module) : kmodules) << ": " << e.text;
}

}
