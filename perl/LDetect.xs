extern "C" {
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
}

#undef do_open
#undef do_close

#include <cstdint>
#include <stdio.h>
#include <iostream>
#include <cassert>
#include <string.h>
#include <unistd.h>
#include "libldetect.h"
#include "hid.h"
#include "pci.h"
#include "usb.h"
#include "dmi.h"

MODULE = LDetect		PACKAGE = LDetect

const char*
get_pci_description(U16 vendor_id, U16 device_id)
  CODE:
  ldetect::pci p;
  RETVAL = p.getDescription(vendor_id, device_id).c_str();
  OUTPUT:
  RETVAL 

const char*
usb_class2text(U32 class_id)
  PPCODE:
  ldetect::usb u;

  ldetect::usb_class_text uct = ldetect::usb_class2text(class_id);
  EXTEND(SP, 3);

  PUSHs(sv_2mortal(newSVpv(uct.class_text.c_str(), 0)));
  PUSHs(sv_2mortal(newSVpv(uct.sub_text.c_str(), 0)));
  PUSHs(sv_2mortal(newSVpv(uct.prot_text.c_str(), 0)));

void
pci_probe()
  PPCODE:
  char buf[2048];
  ldetect::pci entries;

  entries.probe();

  EXTEND(SP, entries.size());
  for (auto i = 0; i < entries.size(); i++) {
    const ldetect::pciEntry &e = entries[i];

    snprintf(buf, sizeof(buf), "%04x\t%04x\t%04x\t%04x\t%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%s\t%s", 
	e.vendor, e.device, e.subvendor, e.subdevice, e.pci_domain, e.bus,
	e.pciusb_device, e.pci_function, e.pci_revision, e.is_pciexpress,
	ldetect::pci_class2text(e.class_id).c_str(), e.class_type.c_str(), e.module.empty() ? "unknown" : e.module.c_str(), e.text.c_str());
    PUSHs(sv_2mortal(newSVpv(buf, 0)));
  }

void
usb_probe()
  PPCODE:
  char buf[2048];
  ldetect::usb entries;

  entries.probe();

  EXTEND(SP, entries.size());
  for (auto i = 0; i < entries.size(); i++) {
    const ldetect::usbEntry &e = entries[i];
    ldetect::usb_class_text class_text = ldetect::usb_class2text(e.class_id);
    snprintf(buf, sizeof(buf), "%04x\t%04x\t%s|%s|%s\t%s\t%s\t%d\t%d\t%d", 
	e.vendor, e.device, class_text.class_text.c_str(), class_text.sub_text.c_str(),
	class_text.prot_text.c_str(), e.module.empty() ? "unknown" : e.module.c_str(), e.text.c_str(),
	e.bus, e.pciusb_device, e.usb_port);
    PUSHs(sv_2mortal(newSVpv(buf, 0)));
  }

void
dmi_probe()
  PPCODE:
  char buf[2048];
  ldetect::dmi entries;

  entries.probe();

  EXTEND(SP, entries.size());
  for (auto i = 0; i < entries.size(); i++) {
    const ldetect::dmiEntry &e = entries[i];

    snprintf(buf, sizeof(buf), "%s\t%s", 
	entries[i].module.c_str(), entries[i].text.c_str());
    PUSHs(sv_2mortal(newSVpv(buf, 0)));
  }


void
hid_probe()
  PPCODE:
  char buf[2048];
  ldetect::hid entries;

  entries.probe();

  EXTEND(SP, entries.size());
  for (auto i = 0; i < entries.size(); i++) {
    const ldetect::hidEntry &e = entries[i];

    snprintf(buf, sizeof(buf), "%s\t%s", 
	entries[i].module.c_str(), entries[i].text.c_str());
    PUSHs(sv_2mortal(newSVpv(buf, 0)));
  }
  /* vim:set ts=8 sts=2 sw=2: */
