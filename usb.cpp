#include <cstdio>
#include <cstring>
#include <cerrno>
#include <vector>
#include "libldetect.h"
#include "common.h"
#include "names.h"

static const char proc_usb_path_default[] = "/sys/kernel/debug/usb/devices";
const char *proc_usb_path = NULL;

static void build_text(pciusb_entry *e, std::string &vendor_text, std::string &product_text) {
	if (e) {
		if(vendor_text.empty())
			vendor_text = names_vendor(e->vendor);
		if(product_text.empty())
			product_text = names_product(e->vendor, e->device);

#ifndef __UCLIBCXX_MAJOR__
		e->text = std::move(std::string(vendor_text.empty() ? "Unknown" : vendor_text).append("|").append(product_text.empty() ? "Unknown": product_text));
#else
		std::swap(e->text,std::string(vendor_text.empty() ? "Unknown" : vendor_text).append("|").append(product_text.empty() ? "Unknown": product_text));
#endif

		vendor_text.clear();
		product_text.clear();
	}
}

std::vector<pciusb_entry>* usb_probe(void) {
	FILE *f;
	char buf[BUF_SIZE];
	int line;
	std::vector<pciusb_entry> *entries;
	pciusb_entry *e = NULL;
	std::string vendor_text, product_text;

	names_init("/usr/share/usb.ids");
	if (!(f = fopen(proc_usb_path ? proc_usb_path : proc_usb_path_default, "r"))) {
		if (proc_usb_path) {
			fprintf(stderr, "Unable to open \"%s\": %s\n"
				    "You may have passed a wrong argument to the \"-u\" option.\n",
				    strerror(errno), proc_usb_path);
		}
		entries = NULL;
		goto exit;
	}

	entries = new std::vector<pciusb_entry>(0);

	/* for further information on the format parsed by this state machine,
	 * read /usr/share/doc/kernel-doc-X.Y.Z/usb/proc_usb_info.txt */  
	for(line = 1; fgets(buf, sizeof(buf) - 1, f) && entries->size() < MAX_DEVICES; line++) {
		switch (buf[0]) {
		case 'T': {
			build_text(e, vendor_text, product_text);
			entries->push_back(pciusb_entry());
			e = &entries->back();

			if (!sscanf(buf, "T:  Bus=%02hhu Lev=%*02d Prnt=%*04d Port=%02hu Cnt=%*02d Dev#=%3hhu Spd=%*3s MxCh=%*2d", &e->pci_bus, &e->usb_port, &e->pci_device) == 3)
				fprintf(stderr, "%s %d: unknown ``T'' line\n", proc_usb_path, line);
			break;
		}
		case 'P': {
			if (!sscanf(buf, "P:  Vendor=%hx ProdID=%hx", &e->vendor, &e->device) == 2)
				fprintf(stderr, "%s %d: unknown ``P'' line\n", proc_usb_path, line);
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
#ifdef __UCLIBCXX_MAJOR___
					e->module = std::move(driver);
#else
					std::swap(e->module,driver);
#endif
				}
				/* see linux/sound/usb/usbaudio.c::usb_audio_ids */
				if (e->class_id == (0x1*0x100+ 0x01)) /* USB_AUDIO_CLASS*0x100 + USB_SUBCLASS_AUDIO_CONTROL*/
					e->module = "snd_usb_audio";

			} else if (sscanf(buf, "I:  If#=%*2d Alt=%*2d #EPs=%*2d Cls=%02x(%*5c) Sub=%02x Prot=%02x Driver=%s",
				    &class_id, &sub, &prot, const_cast<char*>(driver.data())) >= 3) {
				/* Ignore interfaces not active */
			} else fprintf(stderr, "%s %d: unknown ``I'' line\n", proc_usb_path, line);
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

	build_text(&entries->back(), vendor_text, product_text);

	fclose(f);

	pciusb_find_modules(*entries, "usbtable", DO_NOT_LOAD, 0);

exit:
	names_exit();
	return entries;
}

