#ifndef _LDETECT_PCI
#define _LDETECT_PCI

#include <iostream>
#include <iomanip>
#include <string>
#include <ostream>
#include <sstream>

#include "pciusb.h"

#pragma GCC visibility push(default)

struct pci_access;
struct pci_dev;
struct kmod_ctx;

namespace ldetect {
    const std::string& pci_class2text(uint16_t class_id) __attribute__((externally_visible));

    class pciEntry : public pciusbEntry {
	public:
	    pciEntry(pci_dev *dev, pci_access *pacc) EXPORTED;

#if 0
	    uint16_t vendor; /* PCI vendor id */
	    uint16_t device; /* PCI device id */
#endif

	    uint16_t pci_domain; /* PCI domain id (16 bits wide in libpci) */
	    uint8_t pci_function; /* PCI function id 3 bits wide */
	    uint8_t pci_revision; /* PCI revision 8 bits wide */

	    bool is_pciexpress; /* is it PCI express */

	    std::string verbose() const EXPORTED;
	    std::string rev() const EXPORTED;
	    friend std::ostream& operator<<(std::ostream& os, const pciEntry& e) EXPORTED;
    };


    class pci : public pciusb<pciEntry> {
	public:
	    pci(std::string proc_pci_path="/proc/bus/pci/devices") EXPORTED;
	    ~pci() EXPORTED;

	    const std::string& getDescription(uint16_t vendor_id, uint16_t device_id) EXPORTED;
	    void probe(void) EXPORTED;

	protected:
	    void find_modules_through_aliases(struct kmod_ctx *ctx, pciEntry &e);

	
	private:
	    struct pci_access *_pacc;
    };

}

#pragma GCC visibility pop

#endif
