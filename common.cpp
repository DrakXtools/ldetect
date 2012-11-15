#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <iomanip>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "common.h"

namespace ldetect {

std::string table_name_to_file(std::string name) {

    const char *share_path = getenv("SHARE_PATH");
    std::string fname(share_path ? share_path : "/usr/share");
    fname.append("/ldetect-lst/").append(name);

    return fname;
}

std::string hexFmt(uint32_t value, uint8_t w, bool prefix) {
    std::ostringstream oss(std::ostringstream::out);
    if (prefix)
    	oss << "0x";
    oss << std::setw(w) << std::setfill('0') << std::hex << value;
    return oss.str();
}

fh fh_open(std::string name) {
    fh ret;
    std::string fname = table_name_to_file(name);

    if (access(fname.c_str(), R_OK) == 0) {
	/* prefer gzip type when not compressed, more direct than zlib access */
	ret.compressed = false;
	ret.f = fopen(fname.c_str(), "r");
    }
#ifdef HAVE_LIBZ
    else {
	std::string fname_gz(fname + ".gz");
	if (access(fname_gz.c_str(), R_OK) != 0) {
	    std::cerr << "Missing " << name << " (should be " << fname_gz << ")" << std::endl;
	    exit(1);
	}
	else
	    ret.compressed = true;


	ret.zlib_fh = gzopen(fname_gz.c_str(), "r");
	if (!ret.zlib_fh) {
	    perror("pciusb");
	    exit(3);
	}
    }
#else
    else {
	std::cerr << "Missing " << name << " (should be " << fname << ")" << std::endl;
	exit(1);
    }


#endif

    return ret;
}

char* fh_gets(char *line, int size, fh &f) {
#ifdef HAVE_LIBZ
    if (f.compressed)
	return gzgets(f.zlib_fh, line, size);
#endif
    return fgets(line, size, f.f);
}

int fh_close(fh &f) {
#ifdef HAVE_LIBZ
    if (f.compressed)
	return gzclose(f.zlib_fh);
#endif
    return fclose(f.f);
}

}
