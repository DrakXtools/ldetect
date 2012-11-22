#include <cstdio>
#include <cstring>
#include <cerrno>
#include <vector>
#include <libkmod.h>
#include <dirent.h>

#include "common.h"
#include "names.h"

#include "usb.h"
#include "libldetect.h"

namespace ldetect {

std::string usbEntry::sysfsName() const {
    std::ostringstream devname(std::ostringstream::out);
    devname << bus << "-" << usb_port + 1;

//    printf("vendor: %hu device: %hu subvendor: %hu subdevice: %hu class_id: %u bus: %hhu pciusb_device: %hhu usb_port: %hu\n", vendor,device,subvendor,subdevice,class_id,bus,pciusb_device,usb_port);
/*    devname << hexFmt(e.pci_domain, 4, false) << ":" <<  hexFmt(e.bus, 2, false) <<
	":" << hexFmt(e.pciusb_device, 2, false) << "." << hexFmt(e.pci_function, 0, false);*/
    return devname.str();
}


std::ostream& operator<<(std::ostream& os, const usbEntry& e) {
    os << static_cast<const pciusbEntry&>(e);
    struct usb_class_text s = usb_class2text(e.class_id);
    if (!s.class_text.empty()) {
	os << " [" << s.class_text;
	if (!s.sub_text.empty()) os << "|" << s.sub_text;
	if (!s.prot_text.empty()) os << "|" << s.prot_text;
	os << "]";
    }
    return os;
}

usb::usb(std::string proc_usb_path) : pciusb("usb"), _proc_usb_path(proc_usb_path) {
    names_init("/usr/share/usb.ids");
}

usb::~usb() {
    names_exit();
}

static void build_text(pciusbEntry *e, std::string &vendor_text, std::string &product_text) {
	if (e) {
		if(vendor_text.empty())
			vendor_text = names_vendor(e->vendor);
		if(product_text.empty())
			product_text = names_product(e->vendor, e->device);

		e->text = std::string(vendor_text.empty() ? "Unknown" : vendor_text).append("|").append(product_text.empty() ? "Unknown": product_text);
		vendor_text.clear();
		product_text.clear();
	}
}
void usb::probe(void) {
	FILE *f;
	char buf[BUF_SIZE];
	int line;
	usbEntry *e = nullptr;
	std::string vendor_text, product_text;

	if (!(f = fopen(_proc_usb_path.c_str(), "r")))
	    std::cerr << "Unable to open \"" << strerror(errno) << "\": " << _proc_usb_path << std::endl <<
		    "You may have passed a wrong argument to the \"-u\" option." << std::endl;

	/* for further information on the format parsed by this state machine,
	 * read /usr/share/doc/kernel-doc-X.Y.Z/usb/proc_usb_info.txt */  
	for(line = 1; fgets(buf, sizeof(buf) - 1, f) && _entries.size() < MAX_DEVICES; line++) {
		switch (buf[0]) {
		case 'T': {
			build_text(e, vendor_text, product_text);
			_entries.push_back(usbEntry());
			e = &_entries.back();

			if (!sscanf(buf, "T:  Bus=%02hhu Lev=%*02d Prnt=%*04d Port=%02hu Cnt=%*02d Dev#=%3hhu Spd=%*3s MxCh=%*2d", &e->bus, &e->usb_port, &e->pciusb_device) == 3)
				fprintf(stderr, "%s %d: unknown ``T'' line\n", _proc_usb_path.c_str(), line);
			break;
		}
		case 'P': {
			if (!sscanf(buf, "P:  Vendor=%hx ProdID=%hx", &e->vendor, &e->device) == 2)
				fprintf(stderr, "%s %d: unknown ``P'' line\n", _proc_usb_path.c_str(), line);
			break;
		}
		case 'I': if (e->class_id == 0 || e->module.empty()) {
			std::string driver(64, '\0');
			unsigned int class_id, sub, prot = 0;
			if (sscanf(buf, "I:* If#=%*2d Alt=%*2d #EPs=%*2d Cls=%02x(%*5c) Sub=%02x Prot=%02x Driver=%s",
				    &class_id, &sub, &prot, const_cast<char*>(driver.data())) >= 3) {
				unsigned long cid = (class_id * 0x100 + sub) * 0x100 + prot;
				if (e->class_id == 0)
					e->class_id = cid;
				if (driver.compare(0,6, "(none)")) {
					/* Get current class if we are on the first one having used by a driver */
					e->class_id = cid;
					/* replace '-' characters with '_' to be compliant with modnames from modaliases */
					size_t pos = 0;
					while (((pos = driver.find_first_of('-',pos))!= std::string::npos)) {
						driver[pos++] = '_';
					}
					// lame..
					driver.resize(driver.find_first_of('\0', 0));
					e->module = driver;
				}
				/* see linux/sound/usb/usbaudio.c::usb_audio_ids */
				if (e->class_id == (0x1*0x100+ 0x01)) /* USB_AUDIO_CLASS*0x100 + USB_SUBCLASS_AUDIO_CONTROL*/
					e->module += "snd_usb_audio";

			} else if (sscanf(buf, "I:  If#=%*2d Alt=%*2d #EPs=%*2d Cls=%02x(%*5c) Sub=%02x Prot=%02x Driver=%s",
				    &class_id, &sub, &prot, const_cast<char*>(driver.data())) >= 3) {
				/* Ignore interfaces not active */
			} else fprintf(stderr, "%s %d: unknown ``I'' line\n", _proc_usb_path.c_str(), line);
			break;
		}
		case 'S': {
			int offset;
			char dummy;
			size_t length = strlen(buf) - 1;
			if (sscanf(buf, "S:  Manufacturer=%n%c", &offset, &dummy) == 1) {
				buf[length] = '\0'; /* removing '\n' */
				vendor_text.assign(buf, offset, length);
			} else if (sscanf(buf, "S:  Product=%n%c", &offset, &dummy) == 1) {
				buf[length] = '\0'; /* removing '\n' */
				product_text.assign(buf, offset, length);
			}
		}
		}
	}

	build_text(&_entries.back(), vendor_text, product_text);

	fclose(f);

	findModules("usbtable", false);

}

void usb::find_modules_through_aliases(struct kmod_ctx *ctx, usbEntry &e) {
    DIR *dir;
    struct dirent *dent;

    std::ostringstream usb_prefix(std::ostringstream::out);
    usb_prefix << e.bus << "-";
    /* USB port is indexed from 0 in procfs, from 1 in sysfs */
    std::ostringstream sysfs_path(std::ostringstream::out);
    sysfs_path << "/sys/bus/usb/devices/" << e.bus << "-" << e.usb_port + 1;

    dir = opendir(sysfs_path.str().c_str());
    if (!dir) {
	std::cout << "failed: " << sysfs_path.str() << std::endl;
	return;
    }

    while ((dent = readdir(dir)) != nullptr) {
	if ((dent->d_type == DT_DIR) &&
		!strncmp(usb_prefix.str().c_str(), dent->d_name, usb_prefix.str().size())) {
	    std::ostringstream modalias_path(std::ostringstream::out);
	    modalias_path << "/" << dent->d_name << "/modalias";

	    std::cout << "find_modules_through_aliases: " << modalias_path.str() << std::endl;
	    //set_modules_from_modalias_file(ctx, e, modalias_path.str());
	    /* maybe we would need a "other_modules" field in pciusb_entry 
	       to list modules from all USB interfaces */
	    if (!e.module.empty())
		break;
	}
    }
    closedir(dir);
}

}
