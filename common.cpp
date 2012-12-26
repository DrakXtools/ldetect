#include <iomanip>
#include <iostream>

#include "common.h"
#include "gzstream.h"

namespace ldetect {

const std::string table_name_dir(std::string((getenv("SHARE_PATH") ? getenv("SHARE_PATH") : "/usr/share")).append("/ldetect-lst/"));

std::string hexFmt(uint32_t value, uint8_t w, bool prefix) {
    std::ostringstream oss(std::ostringstream::out);
    if (prefix)
    	oss << "0x";
    oss << std::setw(w) << std::setfill('0') << std::hex << value;
    return oss.str();
}

instream i_open(std::string &&name) {
    if (!name.compare(name.size()-3, 3, ".gz"))
    	return instream(new igzstream(name.c_str()));
    return instream(new std::ifstream(name.c_str()));
}

instream fh_open(std::string &&name) {
    std::string fname(table_name_dir + name);
    if (access(fname.c_str(), R_OK) != 0)
	fname += ".gz";

    return instream(i_open(fname.c_str()));
}

}
