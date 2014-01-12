#ifndef LIBLDETECT_COMMON
#define LIBLDETECT_COMMON

#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <cstring>

#include "libldetect.h"

#pragma GCC visibility push(hidden) 

namespace ldetect {

extern const std::string table_name_dir NON_EXPORTED;

std::string hexFmt(uint32_t value, uint8_t w = 4, bool prefix = true);

struct kmod_ctx* modalias_init(void) NON_EXPORTED;
std::vector<std::string> modalias_resolve_modules(struct kmod_ctx *ctx, const std::string &modalias) NON_EXPORTED;

#define MAX_DEVICES 300
#define BUF_SIZE 512

instream fh_open(std::string &&name) NON_EXPORTED;

template <class T>
void findModules(const std::string &fpciusbtable, bool descr_lookup, std::vector<T> &entries) {
    std::string buff;
    instream f = fh_open(fpciusbtable.c_str());

    for (int line = 1; getline(*f, buff) && !f->eof(); line++) {
	uint16_t vendor, device, subvendor, subdevice;
	const char *p = nullptr, *q = nullptr;
	const char *buf = buff.c_str();
	int offset;
	int nb;
	if (buf[0]=='#')
	    continue; // skip comments

	nb = sscanf(buf, "0x%hx\t0x%hx\t0x%hx\t0x%hx\t%n", &vendor, &device, &subvendor, &subdevice, &offset);
	if (nb != 4) {
	    nb = sscanf(buf, "0x%hx\t0x%hx\t%n", &vendor, &device, &offset);
	    if (nb != 2) {
		std::cerr << fpciusbtable << " " << line << ": bad line" << std::endl;
		continue; // skip bad line
	    }
	}
	for (typename std::vector<T>::iterator it = entries.begin(); it != entries.end(); ++it) {
	    T &e = *it;
	    if (e.already_found)
		continue;	// skip since already found with sub ids
	    if (vendor != e.vendor ||  device != e.device)
		continue; // main ids differ

	    if (nb == 4 && !(subvendor == e.subvendor && subdevice == e.subdevice))
		continue; // subids differ

	    if (!p) { // only calc text & module if not already done
		p = buf + offset + 1;
		q = strchr(p, '\t');
		if (!q) // no description field?
		    q = strchr(p, '\0') - 1;
	    }
	    if (strncmp(p, "unknown", q-p)) {
		e.module.assign(p,q-p);
		if (e.module.find_first_of(':') != std::string::npos)
		    std::swap(e.module, e.card);
	    }
	    /* special case for buggy 0x0 usb entry */
	    if (descr_lookup && strlen(q) > 1 && 2 < strlen(q+2) && vendor != 0 && device != 0 && e.class_id != 0x90000d) { /* Hub class */
		//ifree(e->text); /* usb.c set it so that we display something when usbtable doesn't refer that hw*/
		e.text.assign(q+2, strlen(q)-4);
	    }
	    /* if subids read on pcitable line, we know that subids matches :
	       (see "subids differ" test above) */
	    if (nb == 4)
		e.already_found = true;
	}
    }
}

}
#pragma GCC visibility pop

#endif
