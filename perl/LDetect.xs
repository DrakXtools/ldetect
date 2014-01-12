extern "C" {
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
}

#undef do_open
#undef do_close

#include "hid.h"
#include "pci.h"
#include "usb.h"
#include "dmi.h"

HV* common_pciusb_hash_init(const ldetect::pciusbEntry &e) {
  HV *rh = (HV *)sv_2mortal((SV *)newHV()); 
  hv_store(rh, "vendor",         6, newSVnv(e.vendor),     0); 
  hv_store(rh, "subvendor",      9, newSVnv(e.subvendor),  0); 
  hv_store(rh, "id",             2, newSVnv(e.device),     0); 
  hv_store(rh, "subid",          5, newSVnv(e.subdevice),  0); 
  hv_store(rh, "card",           4, newSVpv(e.card.c_str(), 0);
  hv_store(rh, "driver",         6, newSVpv(!e.module.empty() ? e.module.c_str() : (!e.kmodules.empty() ? e.kmodules.front().c_str() : "unknown"), 0), 0);
  hv_store(rh, "description",   11, newSVpv(e.text.c_str(), 0),    0); 
  hv_store(rh, "pci_bus",        7, newSVnv(e.bus),    0); 
  hv_store(rh, "pci_device",    10, newSVnv(e.pciusb_device), 0); 
  return rh;
}

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
    HV * rh = common_pciusb_hash_init(e);
    hv_store(rh, "pci_domain",    10, newSVnv(e.pci_domain),      0);
    hv_store(rh, "pci_function",  12, newSVnv(e.pci_function),    0);
    hv_store(rh, "pci_revision",  12, newSVnv(e.pci_revision),    0);
    hv_store(rh, "is_pciexpress", 13, newSVnv(e.is_pciexpress),   0);
    hv_store(rh, "nice_media_type", 15, newSVpv(e.class_type.c_str(), 0),      0);
    hv_store(rh, "media_type",    10, newSVpv(ldetect::pci_class2text(e.class_id).c_str(), 0), 0); 
    PUSHs(newRV((SV *)rh));
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
    ldetect::usb_class_text s = ldetect::usb_class2text(e.class_id);
    snprintf(buf, sizeof(buf), "%s|%s|%s", s.class_text.c_str(), s.sub_text.c_str(), s.prot_text.c_str());
    HV * rh = common_pciusb_hash_init(e);
    hv_store(rh, "usb_port",       8, newSVnv(e.usb_port),   0);
    hv_store(rh, "media_type",    10, newSVpv(buf, 0),        0);
    PUSHs(newRV((SV *)rh));
  }

void
dmi_probe()
  PPCODE:
  char buf[2048];
  ldetect::dmi entries;

  entries.probe();

  EXTEND(SP, entries.size());
  for (auto i = 0; i < entries.size(); i++) {
    const ldetect::entry &e = entries[i];
    HV * rh = (HV *)sv_2mortal((SV *)newHV()); 
    hv_store(rh, "driver",         6, newSVpv(!e.module.empty() ? e.module.c_str() : (!e.kmodules.empty() ? e.kmodules.front().c_str() : "unknown"), 0), 0);
    hv_store(rh, "description", 11, newSVpv(e.text.c_str(), 0),  0); 
    PUSHs(newRV((SV *)rh));
  }


void
hid_probe()
  PPCODE:
  char buf[2048];
  ldetect::hid entries;

  entries.probe();

  EXTEND(SP, entries.size());
  for (auto i = 0; i < entries.size(); i++) {
    const ldetect::entry &e = entries[i];
    HV * rh = (HV *)sv_2mortal((SV *)newHV()); 
    hv_store(rh, "driver",         6, newSVpv(!e.module.empty() ? e.module.c_str() : (!e.kmodules.empty() ? e.kmodules.front().c_str() : "unknown"), 0), 0);
    hv_store(rh, "description", 11, newSVpv(e.text.c_str(), 0),  0); 
    PUSHs(newRV((SV *)rh));
  }
  /* vim:set ts=8 sts=2 sw=2: */
