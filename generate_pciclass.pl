#!/usr/bin/perl

print q(/* This auto-generated from <pci.h>, don't modify! */

#include <map>
#include <cstdint>
#include "libldetect.h"

namespace ldetect {

static const std::map<uint16_t,const char*> pciClasses {
);

/^#define\s+PCI_CLASS_(\w+)\s+(0x\w{4})/ and print qq(    { $2, "$1" },\n) while <>;

print '};

#pragma GCC visibility push(default) 
const char *pci_class2text(uint16_t classId) {
    std::map<uint16_t, const char*>::const_iterator it = pciClasses.find(classId);
    return it == pciClasses.end() ? "NOT_DEFINED" : it->second;
}
#pragma GCC visibility pop

}
';
