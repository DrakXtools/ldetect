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

#ifdef __UCLIBCXX_MAJOR__
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
#else
static const std::string emptyString;
const std::string& usbNames::getVendor(uint16_t vendorId)
{
    std::map<uint16_t, std::string>::const_iterator it = _vendors.find(vendorId);
    return it == _vendors.end() ? emptyString : it->second;
}

const std::string& usbNames::getProduct(uint16_t vendorId, uint16_t productId)
{
    std::map<std::pair<uint16_t,uint16_t>, std::string>::const_iterator it = _products.find(std::pair<uint16_t, uint16_t>(vendorId, productId));
    return it == _products.end() ? emptyString : it->second;
}

#endif

#define DBG(x)

void usbNames::parse(instream &f)
{
	char buf[512], *cp;
	uint32_t linectr = 0;
	int lastvendor = -1;
	uint32_t u;

	while (f->getline(buf, sizeof(buf)) && !f->eof()) {
		linectr++;
		if (buf[0] == '#' || !buf[0])
			continue;
		cp = buf;
		if (buf[0] == 'P' && buf[1] == 'H' && buf[2] == 'Y' && buf[3] == 'S' && buf[4] == 'D' &&
		    buf[5] == 'E' && buf[6] == 'S' && /*isspace(buf[7])*/ buf[7] == ' ') {
			continue;
		}
		if (buf[0] == 'P' && buf[1] == 'H' && buf[2] == 'Y' && /*isspace(buf[3])*/ buf[3] == ' ') {
			continue;
		}
		if (buf[0] == 'B' && buf[1] == 'I' && buf[2] == 'A' && buf[3] == 'S' && /*isspace(buf[4])*/ buf[4] == ' ') {
			continue;
		}
		if (buf[0] == 'L' && /*isspace(buf[1])*/ buf[1] == ' ') {
			lastvendor = -1;
			continue;
		}
		if (buf[0] == 'C' && /*isspace(buf[1])*/ buf[1] == ' ') {
			lastvendor = -1;
			continue;
		}
		if (buf[0] == 'A' && buf[1] == 'T' && isspace(buf[2])) {
			continue;
		}
		if (buf[0] == 'V' && buf[1] == 'T' && isspace(buf[2])) {
			continue;
		}
		if (buf[0] == 'H' && buf[1] == 'C' && buf[2] == 'C' && isspace(buf[3])) {
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
#ifdef __UCLIBCXX_MAJOR__
			if (newVendor(cp, u))
				fprintf(stderr, "Duplicate vendor spec at line %u vendor %04x %s\n", linectr, u, cp);
#else
			_vendors[u] = cp;
#endif
			DBG(printf("line %5u vendor %04x %s\n", linectr, u, cp));
			lastvendor = u;
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
#ifdef __UCLIBCXX_MAJOR__
			    if (newProduct(cp, lastvendor, u))
				fprintf(stderr, "Duplicate product spec at line %u product %04x:%04x %s\n", linectr, lastvendor, u, cp);
#else
			    _products[std::pair<uint16_t,uint16_t>(lastvendor, u)] = cp;
#endif
				DBG(printf("line %5u product %04x:%04x %s\n", linectr, lastvendor, u, cp));
				continue;
			}
			continue;
		}
		if (buf[0] == '\t' && buf[1] == '\t' && isxdigit(buf[2])) {
			continue;
		}
		if (buf[0] == 'H' && buf[1] == 'I' && buf[2] == 'D' && /*isspace(buf[3])*/ buf[3] == ' ') {
			continue;
		}
		if (buf[0] == 'H' && buf[1] == 'U' && buf[2] == 'T' && /*isspace(buf[3])*/ buf[3] == ' ') {
			lastvendor = -1;
			continue;

		}
		if (buf[0] == 'R' && buf[1] == ' ') {
			continue;
		}
		fprintf(stderr, "Unknown line at line %u\n", linectr);
	}
}

/* ---------------------------------------------------------------------- */

usbNames::usbNames(std::string &&n)
#ifndef __UCLIBCXX_MAJOR__
    : _vendors(), _products()
#endif
{
	instream f = i_open(n.c_str());

	parse(f);
}

usbNames::~usbNames()
{
#ifdef __UCLIBCXX_MAJOR__
	freeList(_vendors);
	freeList(_products);
#endif
}

}
