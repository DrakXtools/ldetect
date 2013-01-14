#include <iomanip>
#include "common.h"
#include "libldetect.h"

namespace ldetect {

std::ostream& operator<<(std::ostream& os, const entry& e) {
    // XXX: setw broken in uClibc++...
#ifndef __UCLIBCXX_MAJOR__
    return os << std::setw(16) << std::left << e.module << ": " << e.text;
#else
    char buf[32];
    snprintf(buf, sizeof(buf), "%-16s: ", e.module.c_str());
    return os << buf << e.text;
#endif
}

}
