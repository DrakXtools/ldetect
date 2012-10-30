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
#include <stdint.h>

/* ---------------------------------------------------------------------- */

const char *names_vendor(uint16_t vendorid);
const char *names_product(uint16_t vendorid, uint16_t productid);
const char *names_class_type(uint8_t classid);
const char *names_subclass_type(uint8_t classid, uint8_t subclassid);
const char *names_protocol(uint8_t classid, uint8_t subclassid,
				  uint8_t protocolid);
const char *names_audioterminal(uint16_t termt);
const char *names_videoterminal(uint16_t termt);
const char *names_hid(uint8_t hidd);
const char *names_reporttag(uint8_t rt);
const char *names_huts(unsigned int data);
const char *names_hutus(unsigned int data);
const char *names_langid(uint16_t langid);
const char *names_physdes(uint8_t ph);
const char *names_bias(uint8_t b);
const char *names_countrycode(unsigned int countrycode);

int get_vendor_string(char *buf, size_t size, uint16_t vid);
int get_product_string(char *buf, size_t size, uint16_t vid, uint16_t pid);

int names_init(const char *n);
void names_exit(void);

/* ---------------------------------------------------------------------- */
#endif /* _NAMES_H */
