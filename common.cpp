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
		ret.gztype = GZIP;
		ret.u.gzip_fh.f = fopen(fname.c_str(), "r");
		ret.u.gzip_fh.pid = 0;
	}
#ifdef HAVE_LIBZ
		else {
		    std::string fname_gz(fname + ".gz");

                        ret.gztype = ZLIB;
                        ret.u.zlib_fh = gzopen(fname_gz.c_str(), "r");
                        if (!ret.u.zlib_fh) {
                                perror("pciusb");
                                exit(3);
                        }
                }
#endif

	return ret;
}

char* fh_gets(char *line, int size, fh *f) {
        char *ret = nullptr;
        switch (f->gztype) {
        case ZLIB:
#ifdef HAVE_LIBZ
                ret = gzgets(f->u.zlib_fh, line, size);
                break;
#endif
        case GZIP:
                ret = fgets(line, size, f->u.gzip_fh.f);
                break;
        }
        return ret;
}

int fh_close(fh *f) {
        int ret = EOF;
        switch (f->gztype) {
        case ZLIB:
#ifdef HAVE_LIBZ
                ret = gzclose(f->u.zlib_fh);
                break;
#endif
        case GZIP:
                ret = fclose(f->u.gzip_fh.f);
                if (f->u.gzip_fh.pid > 0)
                        waitpid(f->u.gzip_fh.pid, nullptr, 0);
                break;
        }
        return ret;
}

}
