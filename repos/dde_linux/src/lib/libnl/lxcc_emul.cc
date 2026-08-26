/*
 * \brief  Linux emulation code
 * \author Josef Soentgen
 * \date   2014-07-28
 */

/*
 * Copyright (C) 2014-2017 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2 or later.
 */

/* Genode includes */
#include <base/log.h>
#include <util/string.h>


/**************
 ** stdlib.h **
 **************/

static char getenv_NLDBG[]          = "1";
static char getenv_HZ[]             = "100";
static char getenv_TICKS_PER_USEC[] = "10000";


extern "C" char *getenv(const char *name)
{
	if (Genode::strcmp(name, "NLDBG") == 0)          return getenv_NLDBG;
	if (Genode::strcmp(name, "HZ") == 0)             return getenv_HZ;
	if (Genode::strcmp(name, "TICKS_PER_USEC") == 0) return getenv_TICKS_PER_USEC;

	return nullptr;
}

/*************
 ** netdb.h **
 *************/

extern "C" {
struct protoent;
struct protoent *getprotobynumber (int __proto) { (void)__proto; return nullptr; }
} /* extern "C" */


/***********
 ** libnl **
 ***********/

/*
 * Silence undefined references that no longer get garbage collected
 * due to libnl now being a shared library.
 */
extern "C" {
struct nl_addr;
void *nl_addr_get_binary_addr(struct nl_addr *addr) { return nullptr; }
unsigned int nl_addr_get_len(struct nl_addr *addr) { return 0; }
unsigned int nl_hash_any(const void *key, unsigned long length, unsigned int base) { return 0; }
} /* extern "C" */
