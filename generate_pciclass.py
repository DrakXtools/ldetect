#!/usr/bin/python

import sys,re

pciclasses = []
for f in sys.argv[2:]:
    fd = open(f)
    regexp = re.compile('^#define\s+PCI_CLASS_(\w+)\s+(0x\w{4})')
    for line in fd.readlines():
        m = regexp.match(line)
        if m:
            pciclasses.append((m.groups()[1],m.groups()[0]))
    fd.close()

outstr = """
/* This auto-generated from <pci.h>, don't modify! */

#include <map>
#include "pci.h"

namespace ldetect {

static const std::string undefined("%s");

// uClibc++ doesn't support initializer lists yet :(
#ifndef __UCLIBCXX_MAJOR__
static const std::map<uint16_t,const std::string> pciClasses {
    { %s, undefined },
""" % (pciclasses[0][1], pciclasses[0][0])

for pciclass in pciclasses[1:]:
    outstr += '    { %s, "%s" },\n' % pciclass
outstr += """};

const std::string& pci_class2text(uint16_t classId) {
    std::map<uint16_t, const std::string>::const_iterator it = pciClasses.find(classId);
    return it == pciClasses.end() ? undefined : it->second;
}

#else
static const std::pair<uint16_t, const std::string> pciClasses[] {
    { %s, undefined },
""" % pciclasses[0][0]
for pciclass in pciclasses[1:]:
    outstr += '    { %s, "%s" },\n' % pciclass
outstr += """};

static uint16_t nb_classes = sizeof(pciClasses) / sizeof(*pciClasses);

const std::string& pci_class2text(uint16_t class_id) {
    for (uint16_t i = 0; i < nb_classes; i++)
        if (pciClasses[i].first == class_id) return pciClasses[i].second;

    return undefined;
}

#endif

}
"""

outfd = open(sys.argv[1], "w")
outfd.write(outstr)
outfd.close()

# vim:ts=4:sw=4:et
