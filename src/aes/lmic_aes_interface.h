/*

Module:  lmic_aes_interface.h

Function:
    The API type

Copyright & License:
    See accompanying LICENSE file.

Author:
    Terry Moore, MCCI       May 2020

*/

//! \file

#ifndef _lmic_aes_interface_h_
#define _lmic_aes_interface_h_

#ifndef _lmic_env_h_
# include "../lmic/lmic_env.h"
#endif

#ifndef _oslmic_types_h_
# include "../lmic/oslmic_types.h"
#endif

LMIC_BEGIN_DECLS

/****************************************************************************\
|       Things needed for AES
\****************************************************************************/

extern u4_t AESAUX[];
extern u4_t AESKEY[];
#define AESkey ((u1_t*)AESKEY)
#define AESaux ((u1_t*)AESAUX)

#ifndef AES_ENC  // if AES_ENC is defined as macro all other values must be too
#define AES_ENC       0x00
#define AES_DEC       0x01
#define AES_MIC       0x02
#define AES_CTR       0x04
#define AES_MICNOAUX  0x08
#endif

typedef
u4_t LMIC_ABI_STD
LMIC_AES_request_t(u1_t mode, xref2u1_t buf, u2_t len);


/// \brief Declare secure element functions for a given driver
///
/// \param a_driver Fragment of driver name to insert in function names, e.g `Default`.
///
/// \details
///     The parameter is macro-expanded and then substituted to declare each
///     of the interface functions for the specified driver.
///
///     To use this, write `LMIC_AES_DECLARE_DRIVER_FNS(driverName)`, where \p driverName
///     is the name of the driver, e.g. `ibm` or `ideetron`.
/// 
#define LMIC_AES_DECLARE_DRIVER_FNS(a_driver)                                                   \
    LMIC_AES_request_t LMIC_AES_##a_driver##_request                                            \

LMIC_END_DECLS

#endif // _lmic_aes_interface_h_
