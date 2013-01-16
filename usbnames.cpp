/*****************************************************************************/
/*
 *      names.c  --  USB name database manipulation routines
 *
 *      Copyright (C) 1999, 2000  Thomas Sailer (sailer@ife.ee.ethz.ch)
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *
 */

/*****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <cstdio>
#include <ctype.h>

#include "usbnames.h"

namespace ldetect {

/* ---------------------------------------------------------------------- */

#define HASH1  0x10
#define HASH2  0x02

static uint32_t hashnum(uint32_t num)
{
	for (uint32_t mask1 = HASH1 << 27, mask2 = HASH2 << 27;
			mask1 >= HASH1;
			mask1 >>= 1, mask2 >>= 1)
		if (num & mask1)
			num ^= mask2;
	return num & (HASHSZ-1);
}

/* ---------------------------------------------------------------------- */

#if 0
static const char *getGenericStrTable(struct genericstrtable *t[HASHSZ],
					 uint32_t idx)
{
	for (struct genericstrtable *h = t[hashnum(idx)]; h; h = h->next)
		if (h->num == idx)
			return h->name;
	return nullptr;
}

const char *usbNames::getHid(uint8_t hidd)
{
	return getGenericStrTable(_hiddescriptors, hidd);
}

const char *usbNames::getReportTag(uint8_t rt)
{
	return getGenericStrTable(_reports, rt);
}

const char *usbNames::getHuts(uint32_t data)
{
	return getGenericStrTable(_huts, data);
}

const char *usbNames::getHutus(uint32_t data)
{
	return getGenericStrTable(_hutus, data);
}

const char *usbNames::getLangId(uint16_t langid)
{
	return getGenericStrTable(_langids, langid);
}

const char *usbNames::getPhysDes(uint8_t ph)
{
	return getGenericStrTable(_physdess, ph);
}

const char *usbNames::getBias(uint8_t b)
{
	return getGenericStrTable(_biass, b);
}

const char *usbNames::getCountryCode(uint32_t countrycode)
{
	return getGenericStrTable(_countrycodes, countrycode);
}
#endif

const char *usbNames::getVendor(uint16_t vendorid)
{
	for (struct vendor *v = _vendors[hashnum(vendorid)]; v; v = v->next)
		if (v->vendorid == vendorid)
			return v->name;
	return nullptr;
}

const char *usbNames::getProduct(uint16_t vendorid, uint16_t productid)
{
	for (struct product *p = _products[hashnum((vendorid << 16) | productid)];
			p; p = p->next)
		if (p->vendorid == vendorid && p->productid == productid)
			return p->name;
	return nullptr;
}

#if 0
const char *usbNames::getClassType(uint8_t classid)
{
	for (struct class_type *c = _class_types[hashnum(classid)];
			c; c = c->next)
		if (c->classid == classid)
			return c->name;
	return nullptr;
}

const char *usbNames::getSubClass(uint8_t classid, uint8_t subclassid)
{
	for (struct subclass_type *s = _subclass_types[hashnum((classid << 8) | subclassid)];
			s; s = s->next)
		if (s->classid == classid && s->subclassid == subclassid)
			return s->name;
	return nullptr;
}

const char *usbNames::getProtocol(uint8_t classid, uint8_t subclassid, uint8_t protocolid)
{
	for (struct protocol *p = _protocols[hashnum((classid << 16) | (subclassid << 8) | protocolid)];
			p; p = p->next)
		if (p->classid == classid && p->subclassid == subclassid && p->protocolid == protocolid)
			return p->name;
	return nullptr;
}

const char *usbNames::getAudioTerminal(uint16_t termt)
{
	for (struct audioterminal *at = _audioterminals[hashnum(termt)];
			at; at = at->next)
		if (at->termt == termt)
			return at->name;
	return nullptr;
}

const char *usbNames::getVideoTerminal(uint16_t termt)
{
	for (struct videoterminal *vt = _videoterminals[hashnum(termt)];
			vt; vt = vt->next)
		if (vt->termt == termt)
			return vt->name;
	return nullptr;
}
#endif

/* ---------------------------------------------------------------------- */

#if 0
int usbNames::getVendorString(char *buf, size_t size, uint16_t vid)
{
        const char *cp;

        if (size < 1)
                return 0;
        *buf = 0;
        if (!(cp = getVendor(vid)))
                return 0;
        return snprintf(buf, size, "%s", cp);
}

int usbNames::getProductString(char *buf, size_t size, uint16_t vid, uint16_t pid)
{
        const char *cp;

        if (size < 1)
                return 0;
        *buf = 0;
        if (!(cp = getProduct(vid, pid)))
                return 0;
        return snprintf(buf, size, "%s", cp);
}
#endif
/* ---------------------------------------------------------------------- */

int usbNames::newVendor(const char *name, uint16_t vendorid)
{
	uint32_t h = hashnum(vendorid);
	struct vendor *v;

	for (v = _vendors[h]; v; v = v->next)
		if (v->vendorid == vendorid)
			return -1;
	v = (struct vendor*)malloc(sizeof(struct vendor) + strlen(name));
	if (!v)
		return -1;
	strcpy(v->name, name);
	v->vendorid = vendorid;
	v->next = _vendors[h];
	_vendors[h] = v;
	return 0;
}

int usbNames::newProduct(const char *name, uint16_t vendorid, uint16_t productid)
{
	uint32_t h = hashnum((vendorid << 16) | productid);
	struct product *p;

	for (p = _products[h]; p; p = p->next)
		if (p->vendorid == vendorid && p->productid == productid)
			return -1;
	p = (struct product*)malloc(sizeof(struct product) + strlen(name));
	if (!p)
		return -1;
	strcpy(p->name, name);
	p->vendorid = vendorid;
	p->productid = productid;
	p->next = _products[h];
	_products[h] = p;
	return 0;
}

#if 0
int usbNames::newClassType(const char *name, uint8_t classid)
{
	uint32_t h = hashnum(classid);
	struct class_type *c;

	for (c = _class_types[h]; c; c = c->next)
		if (c->classid == classid)
			return -1;
	c = (struct class_type*)malloc(sizeof(struct class_type) + strlen(name));
	if (!c)
		return -1;
	strcpy(c->name, name);
	c->classid = classid;
	c->next = _class_types[h];
	_class_types[h] = c;
	return 0;
}

int usbNames::newSubclassType(const char *name, uint8_t classid, uint8_t subclassid)
{
	uint32_t h = hashnum((classid << 8) | subclassid);
	struct subclass_type *s;

	for (s = _subclass_types[h]; s; s = s->next)
		if (s->classid == classid && s->subclassid == subclassid)
			return -1;
	s = (struct subclass_type*)malloc(sizeof(struct subclass_type) + strlen(name));
	if (!s)
		return -1;
	strcpy(s->name, name);
	s->classid = classid;
	s->subclassid = subclassid;
	s->next = _subclass_types[h];
	_subclass_types[h] = s;
	return 0;
}

int usbNames::newProtocol(const char *name, uint8_t classid, uint8_t subclassid, uint8_t protocolid)
{
	uint32_t h = hashnum((classid << 16) | (subclassid << 8) | protocolid);
	struct protocol *p;

	for (p = _protocols[h]; p; p = p->next)
		if (p->classid == classid && p->subclassid == subclassid && p->protocolid == protocolid)
			return -1;
	p = (struct protocol*)malloc(sizeof(struct protocol) + strlen(name));
	if (!p)
		return -1;
	strcpy(p->name, name);
	p->classid = classid;
	p->subclassid = subclassid;
	p->protocolid = protocolid;
	p->next = _protocols[h];
	_protocols[h] = p;
	return 0;
}

int usbNames::newAudioTerminal(const char *name, uint16_t termt)
{
	uint32_t h = hashnum(termt);
	struct audioterminal *at;
	for (at = _audioterminals[h]; at; at = at->next)
		if (at->termt == termt)
			return -1;
	at = (struct audioterminal*)malloc(sizeof(struct audioterminal) + strlen(name));
	if (!at)
		return -1;
	strcpy(at->name, name);
	at->termt = termt;
	at->next = _audioterminals[h];
	_audioterminals[h] = at;
	return 0;
}

int usbNames::newVideoTerminal(const char *name, uint16_t termt)
{
	uint32_t h = hashnum(termt);
	struct videoterminal *vt;

	for (vt = _videoterminals[h]; vt; vt = vt->next)
		if (vt->termt == termt)
			return -1;
	vt = (struct videoterminal*)malloc(sizeof(struct videoterminal) + strlen(name));
	if (!vt)
		return -1;
	strcpy(vt->name, name);
	vt->termt = termt;
	vt->next = _videoterminals[h];
	_videoterminals[h] = vt;
	return 0;
}

static int newGenericStrtable(struct genericstrtable *t[HASHSZ],
			       const char *name, uint32_t idx)
{
	uint32_t h = hashnum(idx);
	struct genericstrtable *g;

	for (g = t[h]; g; g = g->next)
		if (g->num == idx)
			return -1;
	g = (struct genericstrtable*)malloc(sizeof(struct genericstrtable) + strlen(name));
	if (!g)
		return -1;
	strcpy(g->name, name);
	g->num = idx;
	g->next = t[h];
	t[h] = g;
	return 0;
}

int usbNames::newHid(const char *name, uint8_t hidd)
{
	return newGenericStrtable(_hiddescriptors, name, hidd);
}

int usbNames::newReportTag(const char *name, uint8_t rt)
{
	return newGenericStrtable(_reports, name, rt);
}

int usbNames::newHuts(const char *name, uint32_t data)
{
	return newGenericStrtable(_huts, name, data);
}

int usbNames::newHutus(const char *name, uint32_t data)
{
	return newGenericStrtable(_hutus, name, data);
}

int usbNames::newLangId(const char *name, uint16_t langid)
{
	return newGenericStrtable(_langids, name, langid);
}

int usbNames::newPhysDes(const char *name, uint8_t ph)
{
	return newGenericStrtable(_physdess, name, ph);
}

int usbNames::newBias(const char *name, uint8_t b)
{
	return newGenericStrtable(_biass, name, b);
}

int usbNames::newCountryCode(const char *name, uint32_t countrycode)
{
	return newGenericStrtable(_countrycodes, name, countrycode);
}
#endif

/* ---------------------------------------------------------------------- */

template <class T> void freeList(T** list)
{
	T *cur, *tmp;

	for (int i = 0; i < HASHSZ; i++) {
		cur = list[i];
		list[i] = nullptr;
		while (cur) {
			tmp = cur;
			cur = cur->next;
			free(tmp);
		}
	}
}

#define DBG(x)

void usbNames::parse(instream &f)
{
	char buf[512], *cp;
	uint32_t linectr = 0;
	int lastvendor = -1;
#if 0
	int lastclass_type = -1;
	int lastsubclass_type = -1;
	int lasthut = -1;
	int lastlang = -1;
#endif
	uint32_t u;

	while (f->getline(buf, sizeof(buf)) && !f->eof()) {
		linectr++;
		if (buf[0] == '#' || !buf[0])
			continue;
		cp = buf;
		if (buf[0] == 'P' && buf[1] == 'H' && buf[2] == 'Y' && buf[3] == 'S' && buf[4] == 'D' &&
		    buf[5] == 'E' && buf[6] == 'S' && /*isspace(buf[7])*/ buf[7] == ' ') {
#if 0
			cp = buf + 8;
			while (isspace(*cp))
				cp++;
			if (!isxdigit(*cp)) {
				fprintf(stderr, "Invalid Physdes type at line %u\n", linectr);
				continue;
			}
			u = strtoul(cp, &cp, 16);
			while (isspace(*cp))
				cp++;
			if (!*cp) {
				fprintf(stderr, "Invalid Physdes type at line %u\n", linectr);
				continue;
			}
			if (newPhysDes(cp, u))
				fprintf(stderr, "Duplicate Physdes  type spec at line %u terminal type %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u physdes type %02x %s\n", linectr, u, cp));
#endif
			continue;

		}
		if (buf[0] == 'P' && buf[1] == 'H' && buf[2] == 'Y' && /*isspace(buf[3])*/ buf[3] == ' ') {
#if 0
			cp = buf + 4;
			while (isspace(*cp))
				cp++;
			if (!isxdigit(*cp)) {
				fprintf(stderr, "Invalid PHY type at line %u\n", linectr);
				continue;
			}
			u = strtoul(cp, &cp, 16);
			while (isspace(*cp))
				cp++;
			if (!*cp) {
				fprintf(stderr, "Invalid PHY type at line %u\n", linectr);
				continue;
			}
			if (newPhysDes(cp, u))
				fprintf(stderr, "Duplicate PHY type spec at line %u terminal type %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u PHY type %02x %s\n", linectr, u, cp));
#endif
			continue;

		}
		if (buf[0] == 'B' && buf[1] == 'I' && buf[2] == 'A' && buf[3] == 'S' && /*isspace(buf[4])*/ buf[4] == ' ') {
#if 0
			cp = buf + 5;
			while (isspace(*cp))
				cp++;
			if (!isxdigit(*cp)) {
				fprintf(stderr, "Invalid BIAS type at line %u\n", linectr);
				continue;
			}
			u = strtoul(cp, &cp, 16);
			while (isspace(*cp))
				cp++;
			if (!*cp) {
				fprintf(stderr, "Invalid BIAS type at line %u\n", linectr);
				continue;
			}
			if (newBias(cp, u))
				fprintf(stderr, "Duplicate BIAS  type spec at line %u terminal type %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u BIAS type %02x %s\n", linectr, u, cp));
#endif
			continue;

		}
		if (buf[0] == 'L' && /*isspace(buf[1])*/ buf[1] == ' ') {
#if 0
			cp =  buf+2;
			while (isspace(*cp))
				cp++;
			if (!isxdigit(*cp)) {
				fprintf(stderr, "Invalid LANGID spec at line %u\n", linectr);
				continue;
			}
			u = strtoul(cp, &cp, 16);
			while (isspace(*cp))
				cp++;
			if (!*cp) {
				fprintf(stderr, "Invalid LANGID spec at line %u\n", linectr);
				continue;
			}
			if (newLangId(cp, u))
				fprintf(stderr, "Duplicate LANGID spec at line %u language-id %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u LANGID %02x %s\n", linectr, u, cp));
			lasthut = lastclass_type = lastvendor = lastsubclass_type = -1;
			lastlang = u;
#else
			lastvendor = -1;
#endif
			continue;
		}
		if (buf[0] == 'C' && /*isspace(buf[1])*/ buf[1] == ' ') {
#if 0
			/* class_type spec */
			cp = buf+2;
			while (isspace(*cp))
				cp++;
			if (!isxdigit(*cp)) {
				fprintf(stderr, "Invalid class spec at line %u\n", linectr);
				continue;
			}
			u = strtoul(cp, &cp, 16);
			while (isspace(*cp))
				cp++;
			if (!*cp) {
				fprintf(stderr, "Invalid class spec at line %u\n", linectr);
				continue;
			}
			if (newClassType(cp, u))
				fprintf(stderr, "Duplicate class spec at line %u class %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u class %02x %s\n", linectr, u, cp));
			lasthut = lastlang = lastvendor = lastsubclass_type = -1;
			lastclass_type = u;
#else
			lastvendor = -1;
#endif
			continue;
		}
		if (buf[0] == 'A' && buf[1] == 'T' && isspace(buf[2])) {
#if 0
			/* audio terminal type spec */
			cp = buf+3;
			while (isspace(*cp))
				cp++;
			if (!isxdigit(*cp)) {
				fprintf(stderr, "Invalid audio terminal type at line %u\n", linectr);
				continue;
			}
			u = strtoul(cp, &cp, 16);
			while (isspace(*cp))
				cp++;
			if (!*cp) {
				fprintf(stderr, "Invalid audio terminal type at line %u\n", linectr);
				continue;
			}
			if (newAudioTerminal(cp, u))
				fprintf(stderr, "Duplicate audio terminal type spec at line %u terminal type %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u audio terminal type %02x %s\n", linectr, u, cp));
#endif
			continue;
		}
		if (buf[0] == 'V' && buf[1] == 'T' && isspace(buf[2])) {
#if 0
			/* video terminal type spec */
			cp = buf+3;
			while (isspace(*cp))
				cp++;
			if (!isxdigit(*cp)) {
				fprintf(stderr, "Invalid video terminal type at line %u\n", linectr);
				continue;
			}
			u = strtoul(cp, &cp, 16);
			while (isspace(*cp))
				cp++;
			if (!*cp) {
				fprintf(stderr, "Invalid video terminal type at line %u\n", linectr);
				continue;
			}
			if (newVideoTerminal(cp, u))
				fprintf(stderr, "Duplicate video terminal type spec at line %u terminal type %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u video terminal type %02x %s\n", linectr, u, cp));
#endif
			continue;
		}
		if (buf[0] == 'H' && buf[1] == 'C' && buf[2] == 'C' && isspace(buf[3])) {
#if 0
			/* HID Descriptor bCountryCode */
			cp =  buf+3;
			while (isspace(*cp))
				cp++;
			if (!isxdigit(*cp)) {
				fprintf(stderr, "Invalid HID country code at line %u\n", linectr);
				continue;
			}
			u = strtoul(cp, &cp, 10);
			while (isspace(*cp))
				cp++;
			if (!*cp) {
				fprintf(stderr, "Invalid HID country code at line %u\n", linectr);
				continue;
			}
			if (newCountryCode(cp, u))
				fprintf(stderr, "Duplicate HID country code at line %u country %02u %s\n", linectr, u, cp);
			DBG(printf("line %5u keyboard country code %02u %s\n", linectr, u, cp));
#endif
			continue;
		}
		if (isxdigit(*cp)) {
			/* vendor */
			u = strtoul(cp, &cp, 16);
			while (isspace(*cp))
				cp++;
			if (!*cp) {
				fprintf(stderr, "Invalid vendor spec at line %u\n", linectr);
				continue;
			}
			if (newVendor(cp, u))
				fprintf(stderr, "Duplicate vendor spec at line %u vendor %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u vendor %04x %s\n", linectr, u, cp));
			lastvendor = u;
#if 0
			lasthut = lastlang = lastclass_type = lastsubclass_type = -1;
#endif
			continue;
		}
		if (buf[0] == '\t' && isxdigit(buf[1])) {
			/* product or subclass spec */
			u = strtoul(buf+1, &cp, 16);
			while (isspace(*cp))
				cp++;
			if (!*cp) {
				fprintf(stderr, "Invalid product/subclass spec at line %u\n", linectr);
				continue;
			}
			if (lastvendor != -1) {
				if (newProduct(cp, lastvendor, u))
					fprintf(stderr, "Duplicate product spec at line %u product %04x:%04x %s\n", linectr, lastvendor, u, cp);
				DBG(printf("line %5u product %04x:%04x %s\n", linectr, lastvendor, u, cp));
				continue;
			}
#if 0
			if (lastclass_type != -1) {
				if (newSubclassType(cp, lastclass_type, u))
					fprintf(stderr, "Duplicate subclass spec at line %u class %02x:%02x %s\n", linectr, lastclass_type, u, cp);
				DBG(printf("line %5u subclass %02x:%02x %s\n", linectr, lastclass_type, u, cp));
				lastsubclass_type = u;
				continue;
			}
			if (lasthut != -1) {
				if (newHutus(cp, (lasthut << 16)+u))
					fprintf(stderr, "Duplicate HUT Usage Spec at line %u\n", linectr);
				continue;
			}
			if (lastlang != -1) {
				if (newLangId(cp, lastlang+(u<<10)))
					fprintf(stderr, "Duplicate LANGID Usage Spec at line %u\n", linectr);
				continue;
			}
			fprintf(stderr, "Product/Subclass spec without prior Vendor/Class spec at line %u\n", linectr);
#endif
			continue;
		}
		if (buf[0] == '\t' && buf[1] == '\t' && isxdigit(buf[2])) {
#if 0
			/* protocol spec */
			u = strtoul(buf+2, &cp, 16);
			while (isspace(*cp))
				cp++;
			if (!*cp) {
				fprintf(stderr, "Invalid protocol spec at line %u\n", linectr);
				continue;
			}
			if (lastclass_type != -1 && lastsubclass_type != -1) {
				if (newProtocol(cp, lastclass_type, lastsubclass_type, u))
					fprintf(stderr, "Duplicate protocol spec at line %u class %02x:%02x:%02x %s\n", linectr, lastclass_type, lastsubclass_type, u, cp);
				DBG(printf("line %5u protocol %02x:%02x:%02x %s\n", linectr, lastclass_types, lastsubclass_type, u, cp));
				continue;
			}
			fprintf(stderr, "Protocol spec without prior Class and Subclass spec at line %u\n", linectr);
#endif
			continue;
		}
		if (buf[0] == 'H' && buf[1] == 'I' && buf[2] == 'D' && /*isspace(buf[3])*/ buf[3] == ' ') {
#if 0
			cp = buf + 4;
			while (isspace(*cp))
				cp++;
			if (!isxdigit(*cp)) {
				fprintf(stderr, "Invalid HID type at line %u\n", linectr);
				continue;
			}
			u = strtoul(cp, &cp, 16);
			while (isspace(*cp))
				cp++;
			if (!*cp) {
				fprintf(stderr, "Invalid HID type at line %u\n", linectr);
				continue;
			}
			if (newHid(cp, u))
				fprintf(stderr, "Duplicate HID type spec at line %u terminal type %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u HID type %02x %s\n", linectr, u, cp));
#endif
			continue;

		}
		if (buf[0] == 'H' && buf[1] == 'U' && buf[2] == 'T' && /*isspace(buf[3])*/ buf[3] == ' ') {
#if 0
			cp = buf + 4;
			while (isspace(*cp))
				cp++;
			if (!isxdigit(*cp)) {
				fprintf(stderr, "Invalid HUT type at line %u\n", linectr);
				continue;
			}
			u = strtoul(cp, &cp, 16);
			while (isspace(*cp))
				cp++;
			if (!*cp) {
				fprintf(stderr, "Invalid HUT type at line %u\n", linectr);
				continue;
			}
			if (newHuts(cp, u))
				fprintf(stderr, "Duplicate HUT type spec at line %u terminal type %04x %s\n", linectr, u, cp);
			lastlang = lastclass_type = lastvendor = lastsubclass_type = -1;
			lasthut = u;
			DBG(printf("line %5u HUT type %02x %s\n", linectr, u, cp));
#else
			lastvendor = -1;
#endif
			continue;

		}
		if (buf[0] == 'R' && buf[1] == ' ') {
#if 0
			cp = buf + 2;
			while (isspace(*cp))
				cp++;
			if (!isxdigit(*cp)) {
				fprintf(stderr, "Invalid Report type at line %u\n", linectr);
				continue;
			}
			u = strtoul(cp, &cp, 16);
			while (isspace(*cp))
				cp++;
			if (!*cp) {
				fprintf(stderr, "Invalid Report type at line %u\n", linectr);
				continue;
			}
			if (newReportTag(cp, u))
				fprintf(stderr, "Duplicate Report type spec at line %u terminal type %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u Report type %02x %s\n", linectr, u, cp));
#endif
			continue;

		}
		fprintf(stderr, "Unknown line at line %u\n", linectr);
	}
}

/* ---------------------------------------------------------------------- */

usbNames::usbNames(std::string &&n)
{
	instream f = i_open(n.c_str());

	parse(f);
}

usbNames::~usbNames()
{
	freeList(_vendors);
	freeList(_products);
#if 0
	freeList(_class_types);
	freeList(_subclass_types);
	freeList(_protocols);
	freeList(_audioterminals);
	freeList(_videoterminals);
	freeList(_hiddescriptors);
	freeList(_reports);
	freeList(_huts);
	freeList(_biass);
	freeList(_physdess);
	freeList(_hutus);
	freeList(_langids);
	freeList(_countrycodes);
#endif
}

}
