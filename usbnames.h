/*****************************************************************************/

/*
 *      names.h  --  USB name database manipulation routines
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

#ifndef _NAMES_H
#define _NAMES_H

#include <sys/types.h>
#include <cstdint>
#include "common.h"

/* ---------------------------------------------------------------------- */

namespace ldetect {
#define HASHSZ 16

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

#if 0
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
#endif

	class usbNames {
	    public:
		const char *getVendor(uint16_t vendorid);
		const char *getProduct(uint16_t vendorid, uint16_t productid);
#if 0
		const char *getClassType(uint8_t classid);
		const char *getSubClass(uint8_t classid, uint8_t subclassid);
		const char *getProtocol(uint8_t classid, uint8_t subclassid,
				uint8_t protocolid);
		const char *getAudioTerminal(uint16_t termt);
		const char *getVideoTerminal(uint16_t termt);
		const char *getHid(uint8_t hidd);
		const char *getReportTag(uint8_t rt);
		const char *getHuts(unsigned int data);
		const char *getHutus(unsigned int data);
		const char *getLangId(uint16_t langid);
		const char *getPhysDes(uint8_t ph);
		const char *getBias(uint8_t b);
		const char *getCountryCode(unsigned int countrycode);

		int getVendorString(char *buf, size_t size, uint16_t vid);
		int getProductString(char *buf, size_t size, uint16_t vid, uint16_t pid);
#endif
		
		usbNames(std::string &&n);
		~usbNames();

	    private:
		int newVendor(const char *name, uint16_t vendorid);
		int newProduct(const char *name, uint16_t vendorid, uint16_t productid);
#if 0
		int newClassType(const char *name, uint8_t classid);
		int newSubclassType(const char *name, uint8_t classid, uint8_t subclassid);
		int newProtocol(const char *name, uint8_t classid, uint8_t subclassid, uint8_t protocolid);
		int newAudioTerminal(const char *name, uint16_t termt);
		int newVideoTerminal(const char *name, uint16_t termt);

		int newHid(const char *name, uint8_t hidd);
		int newReportTag(const char *name, uint8_t rt);
		int newHuts(const char *name, unsigned int data);
		int newHutus(const char *name, unsigned int data);
		int newLangId(const char *name, uint16_t langid);
		int newPhysDes(const char *name, uint8_t ph);
		int newBias(const char *name, uint8_t b);
		int newCountryCode(const char *name, unsigned int countrycode);
#endif

		struct vendor *_vendors[HASHSZ] = { nullptr, };
		struct product *_products[HASHSZ] = { nullptr, };
#if 0
		struct class_type *_class_types[HASHSZ] = { nullptr, };
		struct subclass_type *_subclass_types[HASHSZ] = { nullptr, };
		struct protocol *_protocols[HASHSZ] = { nullptr, };
		struct audioterminal *_audioterminals[HASHSZ] = { nullptr, };
		struct videoterminal *_videoterminals[HASHSZ] = { nullptr, };
		struct genericstrtable *_hiddescriptors[HASHSZ] = { nullptr, };
		struct genericstrtable *_reports[HASHSZ] = { nullptr, };
		struct genericstrtable *_huts[HASHSZ] = { nullptr, };
		struct genericstrtable *_biass[HASHSZ] = { nullptr, };
		struct genericstrtable *_physdess[HASHSZ] = { nullptr, };
		struct genericstrtable *_hutus[HASHSZ] = { nullptr, };
		struct genericstrtable *_langids[HASHSZ] = { nullptr, };
		struct genericstrtable *_countrycodes[HASHSZ] = { nullptr, };
#endif
		void parse(instream &f);
	};


}

/* ---------------------------------------------------------------------- */
#endif /* _NAMES_H */
