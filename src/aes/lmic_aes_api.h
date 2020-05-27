/*

Module:  lmic_aes_api.h

Function:
    The API type

Copyright & License:
    See accompanying LICENSE file.

Author:
    Terry Moore, MCCI       May 2020

*/

//! \file

#ifndef _lmic_aes_api_h_
#define _lmic_aes_api_h_

#ifndef _lmic_h_
# include "../lmic/oslmic.h"
#endif

#ifndef _lmic_aes_interface_h_
# include "lmic_aes_interface.h"
#endif

LMIC_BEGIN_DECLS

// linkage to well-known drivers. Only one is used.
LMIC_AES_request_t	os_aes_original;
LMIC_AES_request_t	os_aes_generic;

static inline
u4_t os_aes(u1_t mode, xref2u1_t buf, u2_t len) {
#ifdef USE_ORIGINAL_AES
	return  os_aes_original(mode, buf, len);
#else
	return	os_aes_generic(mode, buf, len);
#endif
}

LMIC_END_DECLS

#endif // _lmic_aes_api_h_
