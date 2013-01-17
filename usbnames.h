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
#if 1
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
#endif

	class usbNames {
	    public:

#if 0
		const std::string& getVendor(uint16_t vendorid);
		const std::string& getProduct(uint16_t vendorid, uint16_t productid);
#else
		const char *getVendor(uint16_t vendorid);
		const char *getProduct(uint16_t vendorid, uint16_t productid);
#endif
		usbNames(std::string &&n);
		~usbNames();

	    private:
#if 0
		std::map<uint16_t, std::string> _vendors;
		std::map<std::pair<uint16_t, uint16_t>, std::string> _products;
#else
		int newVendor(const char *name, uint16_t vendorid);
		int newProduct(const char *name, uint16_t vendorid, uint16_t productid);
		struct vendor *_vendors[HASHSZ] = { nullptr, };
		struct product *_products[HASHSZ] = { nullptr, };
#endif
		void parse(instream &f);
	};


}

/* ---------------------------------------------------------------------- */
#endif /* _NAMES_H */
