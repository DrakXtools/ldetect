#include <iomanip>
#include "common.h"
#include "libldetect.h"

namespace ldetect {

std::ostream& operator<<(std::ostream& os, const entry& e) {
    return os << std::setw(16) << std::left << e.module << ": " << e.text;
}

}
