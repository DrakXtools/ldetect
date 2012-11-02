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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

#ifdef HAVE_LIBZ
#include <zlib.h>
#define 	usb_file			gzFile
#define 	usb_fopen(path, mode) 		gzopen(path, mode)
#define 	usb_fgets(s, size, stream)	gzgets(stream, s, size)
#define 	usb_close(f)			gzclose(f)
#else
#define 	usb_file			FILE *
#define 	usb_fopen(path, mode)		fopen(path, mode)
#define 	usb_fgets(s, size, stream)	fgets(s, size, stream)
#define 	usb_close(f)			fclose(f)
#endif

#include "names.h"

namespace ldetect {

/* ---------------------------------------------------------------------- */

struct vendor {
	struct vendor *next;
	uint16_t vendorid;
	char name[1];
};

struct product {
	struct product *next;
	uint16_t vendorid, productid;
	char name[1];
};

struct class_type {
	struct class_type *next;
	uint8_t classid;
	char name[1];
};

struct subclass_type {
	struct subclass_type *next;
	uint8_t classid, subclassid;
	char name[1];
};

struct protocol {
	struct protocol *next;
	uint8_t classid, subclassid, protocolid;
	char name[1];
};

struct audioterminal {
	struct audioterminal *next;
	uint16_t termt;
	char name[1];
};

struct videoterminal {
	struct videoterminal *next;
	uint16_t termt;
	char name[1];
};

struct genericstrtable {
	struct genericstrtable *next;
	unsigned int num;
	char name[1];
};

/* ---------------------------------------------------------------------- */

#define HASH1  0x10
#define HASH2  0x02
#define HASHSZ 16

static unsigned int hashnum(unsigned int num)
{
	for (unsigned int mask1 = HASH1 << 27, mask2 = HASH2 << 27;
			mask1 >= HASH1;
			mask1 >>= 1, mask2 >>= 1)
		if (num & mask1)
			num ^= mask2;
	return num & (HASHSZ-1);
}

/* ---------------------------------------------------------------------- */

static struct vendor *vendors[HASHSZ] = { NULL, };
static struct product *products[HASHSZ] = { NULL, };
static struct class_type *class_types[HASHSZ] = { NULL, };
static struct subclass_type *subclass_types[HASHSZ] = { NULL, };
static struct protocol *protocols[HASHSZ] = { NULL, };
static struct audioterminal *audioterminals[HASHSZ] = { NULL, };
static struct videoterminal *videoterminals[HASHSZ] = { NULL, };
static struct genericstrtable *hiddescriptors[HASHSZ] = { NULL, };
static struct genericstrtable *reports[HASHSZ] = { NULL, };
static struct genericstrtable *huts[HASHSZ] = { NULL, };
static struct genericstrtable *biass[HASHSZ] = { NULL, };
static struct genericstrtable *physdess[HASHSZ] = { NULL, };
static struct genericstrtable *hutus[HASHSZ] = { NULL, };
static struct genericstrtable *langids[HASHSZ] = { NULL, };
static struct genericstrtable *countrycodes[HASHSZ] = { NULL, };

/* ---------------------------------------------------------------------- */

static const char *names_genericstrtable(struct genericstrtable *t[HASHSZ],
					 unsigned int idx)
{
	for (struct genericstrtable *h = t[hashnum(idx)]; h; h = h->next)
		if (h->num == idx)
			return h->name;
	return NULL;
}

const char *names_hid(uint8_t hidd)
{
	return names_genericstrtable(hiddescriptors, hidd);
}

const char *names_reporttag(uint8_t rt)
{
	return names_genericstrtable(reports, rt);
}

const char *names_huts(unsigned int data)
{
	return names_genericstrtable(huts, data);
}

const char *names_hutus(unsigned int data)
{
	return names_genericstrtable(hutus, data);
}

const char *names_langid(uint16_t langid)
{
	return names_genericstrtable(langids, langid);
}

const char *names_physdes(uint8_t ph)
{
	return names_genericstrtable(physdess, ph);
}

const char *names_bias(uint8_t b)
{
	return names_genericstrtable(biass, b);
}

const char *names_countrycode(unsigned int countrycode)
{
	return names_genericstrtable(countrycodes, countrycode);
}

const char *names_vendor(uint16_t vendorid)
{
	for (struct vendor *v = vendors[hashnum(vendorid)]; v; v = v->next)
		if (v->vendorid == vendorid)
			return v->name;
	return NULL;
}

const char *names_product(uint16_t vendorid, uint16_t productid)
{
	for (struct product *p = products[hashnum((vendorid << 16) | productid)];
			p; p = p->next)
		if (p->vendorid == vendorid && p->productid == productid)
			return p->name;
	return NULL;
}

const char *names_class_type(uint8_t classid)
{
	for (struct class_type *c = class_types[hashnum(classid)];
			c; c = c->next)
		if (c->classid == classid)
			return c->name;
	return NULL;
}

const char *names_subclass(uint8_t classid, uint8_t subclassid)
{
	for (struct subclass_type *s = subclass_types[hashnum((classid << 8) | subclassid)];
			s; s = s->next)
		if (s->classid == classid && s->subclassid == subclassid)
			return s->name;
	return NULL;
}

const char *names_protocol(uint8_t classid, uint8_t subclassid, uint8_t protocolid)
{
	for (struct protocol *p = protocols[hashnum((classid << 16) | (subclassid << 8) | protocolid)];
			p; p = p->next)
		if (p->classid == classid && p->subclassid == subclassid && p->protocolid == protocolid)
			return p->name;
	return NULL;
}

const char *names_audioterminal(uint16_t termt)
{
	for (struct audioterminal *at = audioterminals[hashnum(termt)];
			at; at = at->next)
		if (at->termt == termt)
			return at->name;
	return NULL;
}

const char *names_videoterminal(uint16_t termt)
{
	for (struct videoterminal *vt = videoterminals[hashnum(termt)];
			vt; vt = vt->next)
		if (vt->termt == termt)
			return vt->name;
	return NULL;
}

/* ---------------------------------------------------------------------- */

int get_vendor_string(char *buf, size_t size, uint16_t vid)
{
        const char *cp;

        if (size < 1)
                return 0;
        *buf = 0;
        if (!(cp = names_vendor(vid)))
                return 0;
        return snprintf(buf, size, "%s", cp);
}

int get_product_string(char *buf, size_t size, uint16_t vid, uint16_t pid)
{
        const char *cp;

        if (size < 1)
                return 0;
        *buf = 0;
        if (!(cp = names_product(vid, pid)))
                return 0;
        return snprintf(buf, size, "%s", cp);
}

/* ---------------------------------------------------------------------- */

static int new_vendor(const char *name, uint16_t vendorid)
{
	unsigned int h = hashnum(vendorid);
	struct vendor *v;

	for (v = vendors[h]; v; v = v->next)
		if (v->vendorid == vendorid)
			return -1;
	v = (struct vendor*)malloc(sizeof(struct vendor) + strlen(name));
	if (!v)
		return -1;
	strcpy(v->name, name);
	v->vendorid = vendorid;
	v->next = vendors[h];
	vendors[h] = v;
	return 0;
}

static int new_product(const char *name, uint16_t vendorid, uint16_t productid)
{
	unsigned int h = hashnum((vendorid << 16) | productid);
	struct product *p;

	for (p = products[h]; p; p = p->next)
		if (p->vendorid == vendorid && p->productid == productid)
			return -1;
	p = (struct product*)malloc(sizeof(struct product) + strlen(name));
	if (!p)
		return -1;
	strcpy(p->name, name);
	p->vendorid = vendorid;
	p->productid = productid;
	p->next = products[h];
	products[h] = p;
	return 0;
}

static int new_class_type(const char *name, uint8_t classid)
{
	unsigned int h = hashnum(classid);
	struct class_type *c;

	for (c = class_types[h]; c; c = c->next)
		if (c->classid == classid)
			return -1;
	c = (struct class_type*)malloc(sizeof(struct class_type) + strlen(name));
	if (!c)
		return -1;
	strcpy(c->name, name);
	c->classid = classid;
	c->next = class_types[h];
	class_types[h] = c;
	return 0;
}

static int new_subclass_type(const char *name, uint8_t classid, uint8_t subclassid)
{
	unsigned int h = hashnum((classid << 8) | subclassid);
	struct subclass_type *s;

	for (s = subclass_types[h]; s; s = s->next)
		if (s->classid == classid && s->subclassid == subclassid)
			return -1;
	s = (struct subclass_type*)malloc(sizeof(struct subclass_type) + strlen(name));
	if (!s)
		return -1;
	strcpy(s->name, name);
	s->classid = classid;
	s->subclassid = subclassid;
	s->next = subclass_types[h];
	subclass_types[h] = s;
	return 0;
}

static int new_protocol(const char *name, uint8_t classid, uint8_t subclassid, uint8_t protocolid)
{
	unsigned int h = hashnum((classid << 16) | (subclassid << 8) | protocolid);
	struct protocol *p;

	for (p = protocols[h]; p; p = p->next)
		if (p->classid == classid && p->subclassid == subclassid && p->protocolid == protocolid)
			return -1;
	p = (struct protocol*)malloc(sizeof(struct protocol) + strlen(name));
	if (!p)
		return -1;
	strcpy(p->name, name);
	p->classid = classid;
	p->subclassid = subclassid;
	p->protocolid = protocolid;
	p->next = protocols[h];
	protocols[h] = p;
	return 0;
}

static int new_audioterminal(const char *name, uint16_t termt)
{
	unsigned int h = hashnum(termt);
	struct audioterminal *at;
	for (at = audioterminals[h]; at; at = at->next)
		if (at->termt == termt)
			return -1;
	at = (struct audioterminal*)malloc(sizeof(struct audioterminal) + strlen(name));
	if (!at)
		return -1;
	strcpy(at->name, name);
	at->termt = termt;
	at->next = audioterminals[h];
	audioterminals[h] = at;
	return 0;
}

static int new_videoterminal(const char *name, uint16_t termt)
{
	unsigned int h = hashnum(termt);
	struct videoterminal *vt;

	for (vt = videoterminals[h]; vt; vt = vt->next)
		if (vt->termt == termt)
			return -1;
	vt = (struct videoterminal*)malloc(sizeof(struct videoterminal) + strlen(name));
	if (!vt)
		return -1;
	strcpy(vt->name, name);
	vt->termt = termt;
	vt->next = videoterminals[h];
	videoterminals[h] = vt;
	return 0;
}

static int new_genericstrtable(struct genericstrtable *t[HASHSZ],
			       const char *name, unsigned int idx)
{
	unsigned int h = hashnum(idx);
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

static int new_hid(const char *name, uint8_t hidd)
{
	return new_genericstrtable(hiddescriptors, name, hidd);
}

static int new_reporttag(const char *name, uint8_t rt)
{
	return new_genericstrtable(reports, name, rt);
}

static int new_huts(const char *name, unsigned int data)
{
	return new_genericstrtable(huts, name, data);
}

static int new_hutus(const char *name, unsigned int data)
{
	return new_genericstrtable(hutus, name, data);
}

static int new_langid(const char *name, uint16_t langid)
{
	return new_genericstrtable(langids, name, langid);
}

static int new_physdes(const char *name, uint8_t ph)
{
	return new_genericstrtable(physdess, name, ph);
}
static int new_bias(const char *name, uint8_t b)
{
	return new_genericstrtable(biass, name, b);
}

static int new_countrycode(const char *name, unsigned int countrycode)
{
	return new_genericstrtable(countrycodes, name, countrycode);
}

/* ---------------------------------------------------------------------- */

static void free_vendor(void)
{
	struct vendor *cur, *tmp;

	for (int i = 0; i < HASHSZ; i++) {
		cur = vendors[i];
		vendors[i] = NULL;
		while (cur) {
			tmp = cur;
			cur = cur->next;
			free(tmp);
		}
	}
}

static void free_product(void)
{
	struct product *cur, *tmp;

	for (int i = 0; i < HASHSZ; i++) {
		cur = products[i];
		products[i] = NULL;
		while (cur) {
			tmp = cur;
			cur = cur->next;
			free(tmp);
		}
	}
}

static void free_class_type(void)
{
	struct class_type *cur, *tmp;

	for (int i = 0; i < HASHSZ; i++) {
		cur = class_types[i];
		class_types[i] = NULL;
		while (cur) {
			tmp = cur;
			cur = cur->next;
			free(tmp);
		}
	}
}

static void free_subclass_type(void)
{
	struct subclass_type *cur, *tmp;

	for (int i = 0; i < HASHSZ; i++) {
		cur = subclass_types[i];
		subclass_types[i] = NULL;
		while (cur) {
			tmp = cur;
			cur = cur->next;
			free(tmp);
		}
	}
}

static void free_protocol(void)
{
	struct protocol *cur, *tmp;

	for (int i = 0; i < HASHSZ; i++) {
		cur = protocols[i];
		protocols[i] = NULL;
		while (cur) {
			tmp = cur;
			cur = cur->next;
			free(tmp);
		}
	}
}

static void free_audioterminal(void)
{
	struct audioterminal *cur, *tmp;

	for (int i = 0; i < HASHSZ; i++) {
		cur = audioterminals[i];
		audioterminals[i] = NULL;
		while (cur) {
			tmp = cur;
			cur = cur->next;
			free(tmp);
		}
	}
	return;
}

static void free_videoterminal(void)
{
	struct videoterminal *cur, *tmp;

	for (int i = 0; i < HASHSZ; i++) {
		cur = videoterminals[i];
		videoterminals[i] = NULL;
		while (cur) {
			tmp = cur;
			cur = cur->next;
			free(tmp);
		}
	}
}

static void _free_genericstrtable(struct genericstrtable *t[HASHSZ])
{
	struct genericstrtable *cur, *tmp;

	for (int i = 0; i < HASHSZ; i++) {
		cur = t[i];
		t[i] = NULL;
		while (cur) {
			tmp = cur;
			cur = cur->next;
			free(tmp);
		}
	}
}

static void free_genericstrtable(void)
{
	_free_genericstrtable(hiddescriptors);
	_free_genericstrtable(reports);
	_free_genericstrtable(huts);
	_free_genericstrtable(biass);
	_free_genericstrtable(physdess);
	_free_genericstrtable(hutus);
	_free_genericstrtable(langids);
	_free_genericstrtable(countrycodes);
}

#define DBG(x)

static void parse(usb_file f)
{
	char buf[512], *cp;
	unsigned int linectr = 0;
	int lastvendor = -1;
	int lastclass_type = -1;
	int lastsubclass_type = -1;
	int lasthut = -1;
	int lastlang = -1;
	unsigned int u;

	while (usb_fgets(buf, sizeof(buf), f)) {
		linectr++;
		/* remove line ends */
		cp = strchr(buf, 13);
		if (cp)
			*cp = 0;
		cp = strchr(buf, 10);
		if (cp)
			*cp = 0;
		if (buf[0] == '#' || !buf[0])
			continue;
		cp = buf;
		if (buf[0] == 'P' && buf[1] == 'H' && buf[2] == 'Y' && buf[3] == 'S' && buf[4] == 'D' &&
		    buf[5] == 'E' && buf[6] == 'S' && /*isspace(buf[7])*/ buf[7] == ' ') {
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
			if (new_physdes(cp, u))
				fprintf(stderr, "Duplicate Physdes  type spec at line %u terminal type %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u physdes type %02x %s\n", linectr, u, cp));
			continue;

		}
		if (buf[0] == 'P' && buf[1] == 'H' && buf[2] == 'Y' && /*isspace(buf[3])*/ buf[3] == ' ') {
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
			if (new_physdes(cp, u))
				fprintf(stderr, "Duplicate PHY type spec at line %u terminal type %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u PHY type %02x %s\n", linectr, u, cp));
			continue;

		}
		if (buf[0] == 'B' && buf[1] == 'I' && buf[2] == 'A' && buf[3] == 'S' && /*isspace(buf[4])*/ buf[4] == ' ') {
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
			if (new_bias(cp, u))
				fprintf(stderr, "Duplicate BIAS  type spec at line %u terminal type %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u BIAS type %02x %s\n", linectr, u, cp));
			continue;

		}
		if (buf[0] == 'L' && /*isspace(buf[1])*/ buf[1] == ' ') {
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
			if (new_langid(cp, u))
				fprintf(stderr, "Duplicate LANGID spec at line %u language-id %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u LANGID %02x %s\n", linectr, u, cp));
			lasthut = lastclass_type = lastvendor = lastsubclass_type = -1;
			lastlang = u;
			continue;
		}
		if (buf[0] == 'C' && /*isspace(buf[1])*/ buf[1] == ' ') {
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
			if (new_class_type(cp, u))
				fprintf(stderr, "Duplicate class spec at line %u class %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u class %02x %s\n", linectr, u, cp));
			lasthut = lastlang = lastvendor = lastsubclass_type = -1;
			lastclass_type = u;
			continue;
		}
		if (buf[0] == 'A' && buf[1] == 'T' && isspace(buf[2])) {
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
			if (new_audioterminal(cp, u))
				fprintf(stderr, "Duplicate audio terminal type spec at line %u terminal type %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u audio terminal type %02x %s\n", linectr, u, cp));
			continue;
		}
		if (buf[0] == 'V' && buf[1] == 'T' && isspace(buf[2])) {
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
			if (new_videoterminal(cp, u))
				fprintf(stderr, "Duplicate video terminal type spec at line %u terminal type %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u video terminal type %02x %s\n", linectr, u, cp));
			continue;
		}
		if (buf[0] == 'H' && buf[1] == 'C' && buf[2] == 'C' && isspace(buf[3])) {
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
			if (new_countrycode(cp, u))
				fprintf(stderr, "Duplicate HID country code at line %u country %02u %s\n", linectr, u, cp);
			DBG(printf("line %5u keyboard country code %02u %s\n", linectr, u, cp));
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
			if (new_vendor(cp, u))
				fprintf(stderr, "Duplicate vendor spec at line %u vendor %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u vendor %04x %s\n", linectr, u, cp));
			lastvendor = u;
			lasthut = lastlang = lastclass_type = lastsubclass_type = -1;
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
				if (new_product(cp, lastvendor, u))
					fprintf(stderr, "Duplicate product spec at line %u product %04x:%04x %s\n", linectr, lastvendor, u, cp);
				DBG(printf("line %5u product %04x:%04x %s\n", linectr, lastvendor, u, cp));
				continue;
			}
			if (lastclass_type != -1) {
				if (new_subclass_type(cp, lastclass_type, u))
					fprintf(stderr, "Duplicate subclass spec at line %u class %02x:%02x %s\n", linectr, lastclass_type, u, cp);
				DBG(printf("line %5u subclass %02x:%02x %s\n", linectr, lastclass_type, u, cp));
				lastsubclass_type = u;
				continue;
			}
			if (lasthut != -1) {
				if (new_hutus(cp, (lasthut << 16)+u))
					fprintf(stderr, "Duplicate HUT Usage Spec at line %u\n", linectr);
				continue;
			}
			if (lastlang != -1) {
				if (new_langid(cp, lastlang+(u<<10)))
					fprintf(stderr, "Duplicate LANGID Usage Spec at line %u\n", linectr);
				continue;
			}
			fprintf(stderr, "Product/Subclass spec without prior Vendor/Class spec at line %u\n", linectr);
			continue;
		}
		if (buf[0] == '\t' && buf[1] == '\t' && isxdigit(buf[2])) {
			/* protocol spec */
			u = strtoul(buf+2, &cp, 16);
			while (isspace(*cp))
				cp++;
			if (!*cp) {
				fprintf(stderr, "Invalid protocol spec at line %u\n", linectr);
				continue;
			}
			if (lastclass_type != -1 && lastsubclass_type != -1) {
				if (new_protocol(cp, lastclass_type, lastsubclass_type, u))
					fprintf(stderr, "Duplicate protocol spec at line %u class %02x:%02x:%02x %s\n", linectr, lastclass_type, lastsubclass_type, u, cp);
				DBG(printf("line %5u protocol %02x:%02x:%02x %s\n", linectr, lastclass_types, lastsubclass_type, u, cp));
				continue;
			}
			fprintf(stderr, "Protocol spec without prior Class and Subclass spec at line %u\n", linectr);
			continue;
		}
		if (buf[0] == 'H' && buf[1] == 'I' && buf[2] == 'D' && /*isspace(buf[3])*/ buf[3] == ' ') {
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
			if (new_hid(cp, u))
				fprintf(stderr, "Duplicate HID type spec at line %u terminal type %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u HID type %02x %s\n", linectr, u, cp));
			continue;

		}
		if (buf[0] == 'H' && buf[1] == 'U' && buf[2] == 'T' && /*isspace(buf[3])*/ buf[3] == ' ') {
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
			if (new_huts(cp, u))
				fprintf(stderr, "Duplicate HUT type spec at line %u terminal type %04x %s\n", linectr, u, cp);
			lastlang = lastclass_type = lastvendor = lastsubclass_type = -1;
			lasthut = u;
			DBG(printf("line %5u HUT type %02x %s\n", linectr, u, cp));
			continue;

		}
		if (buf[0] == 'R' && buf[1] == ' ') {
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
			if (new_reporttag(cp, u))
				fprintf(stderr, "Duplicate Report type spec at line %u terminal type %04x %s\n", linectr, u, cp);
			DBG(printf("line %5u Report type %02x %s\n", linectr, u, cp));
			continue;

		}
		fprintf(stderr, "Unknown line at line %u\n", linectr);
	}
}

/* ---------------------------------------------------------------------- */

int names_init(const char *n)
{
	usb_file f;

	f = usb_fopen(n, "r");
	if (!f)
		return errno;

	parse(f);
	usb_close(f);
	return 0;
}

void names_exit(void)
{
	free_vendor();
	free_product();
	free_class_type();
	free_subclass_type();
	free_protocol();
	free_audioterminal();
	free_videoterminal();
	free_genericstrtable();
}

}
