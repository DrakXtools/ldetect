#ifndef _LDETECT_PCIUSB
#define _LDETECT_PCIUSB

#include <iostream>
#include <iomanip>
#include <string>
#include <ostream>
#include <sstream>
#include <vector>


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <libkmod.h>



#include "common.h"

#pragma GCC visibility push(hidden) 

namespace ldetect {

    class pciusbEntry {
	public:
	    pciusbEntry(std::string module = "", std::string text= "", std::string class_type = "") :
		module(module), text(text), class_type(class_type),
		vendor(0xffff), device(0xffff),

		subvendor(0xffff), subdevice(0xffff), class_id(0),
		bus(0xff), pciusb_device(0xff),
		already_found(false) {};
	    virtual ~pciusbEntry() {}

	    std::string module;
	    std::string text;
	    std::string class_type;

	    uint16_t vendor; /* PCI vendor id */
	    uint16_t device; /* PCI device id */

	    uint16_t subvendor; /* 0xffff if not probe_type'd or no subid */
	    uint16_t subdevice; /* 0xffff if not probe_type'd or no subid */
	    uint32_t class_id; /* 0 if not probe_type'd ; big because of USB backend */

	    uint8_t bus; /* PCI bus id 8 bits wide */
	    uint8_t pciusb_device; /* PCI device id 5 bits wide */

	    friend std::ostream& operator<<(std::ostream& os, const pciusbEntry& e);


	//protected:
	    bool already_found;
};

    template <class T>
    class pciusb {
	public:
	    pciusb() : _entries() {}
	    virtual ~pciusb() {}

	    const T& operator[] (uint16_t i) const {
		return _entries[i];
	    }

	    operator bool() const throw() {
		return !_entries.empty();
	    }

	    bool operator! () const throw() {
		return _entries.empty();
	    }

	    uint16_t size() const throw() { return _entries.size(); }

	    virtual void probe(void) = 0;

	protected:
	    std::vector<T> _entries;

	    int findModules(const char *fpciusbtable, bool descr_lookup) {
		fh f;
		char buf[2048];

		f = fh_open(fpciusbtable);

		for (int line = 1; fh_gets(buf, sizeof(buf) - 1, &f); line++) {
		    unsigned short vendor, device, subvendor, subdevice;
		    char *p = NULL, *q = NULL;
		    int offset;
		    int nb;
		    if (buf[0]=='#')
			continue; // skip comments

		    nb = sscanf(buf, "0x%hx\t0x%hx\t0x%hx\t0x%hx\t%n", &vendor, &device, &subvendor, &subdevice, &offset);
		    if (nb != 4) {
			nb = sscanf(buf, "0x%hx\t0x%hx\t%n", &vendor, &device, &offset);
			if (nb != 2) {
			    fprintf(stderr, "%s %d: bad line\n", fpciusbtable, line);
			    continue; // skip bad line
			}
		    }
		    for (unsigned int i = 0; i < _entries.size(); i++) {
			T &e = _entries[i];
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
			if (strncmp(p, "unknown", q-p-1)) {
			    e.module.assign(p,q-p-1);
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
		fh_close(&f);

		struct kmod_ctx *ctx = modalias_init();

		for (unsigned int i = 0; i < _entries.size(); i++) {
		    T &e = _entries[i];

		    // No special case found in pcitable ? Then lookup modalias for PCI devices
		    if (!e.module.empty() && e.module != "unknown")
			continue;
		    find_modules_through_aliases(ctx, e);
		}

		modalias_cleanup(ctx);

		return 1;
	    }

	    void set_modules_from_modalias_file(struct kmod_ctx *ctx, T &e, const std::string &modalias_path) {
		FILE *file;
		file = fopen(modalias_path.c_str(), "r");
		if (file) {
		    char *modalias = NULL;
		    size_t n, size;
		    if (-1 == getline(&modalias, &n, file)) {
			std::cerr << "Unable to read modalias from " << modalias_path << std::endl;;
			fclose(file);
			return;
		    }
		    fclose(file);
		    size = strlen(modalias);
		    if (size)
			modalias[size-1] = 0;

		    e.module = modalias_resolve_module(ctx, modalias);
		    free(modalias);
		} else {
		    std::cerr << "Unable to read modalias from " << modalias_path << std::endl;
		    return;
		}
	    }

	    virtual void find_modules_through_aliases(struct kmod_ctx *ctx, T &e) = 0;

    };
}

#pragma GCC visibility pop

#endif
