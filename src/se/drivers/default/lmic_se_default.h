/*

Module:  lmic_secure_element_api.h

Function:
    Basic types from oslmic.h, shared by all layers.

Copyright & License:
    See accompanying LICENSE file.

Author:
    Terry Moore, MCCI       May 2020

*/

//! \file

#ifndef _lmic_se_default_h_
#define _lmic_se_default_h_

#ifndef _lmic_secure_element_interface_h_
# include "../../i/lmic_secure_element_interface.h"
#endif

#ifndef _lmic_aes_api_h_
# include "../../../aes/lmic_aes_api.h"
#endif

/*!

\defgroup lmic_se_default   LMIC Secure Element default implementation

\brief  This module provides a default Secure Element to the LMIC.

\details
    Many applications of LoRaWAN have no need for elaborate security
    mechanisms, and only require that the over the air security be
    implemented correctly and efficiently. The Default Secure Element
    implementation may be useful in such cases. Keys are not stored
    internally, and the AES encryption mechanisms are not necessarily
    robust against differential black-box or sideband attacks. Still,
    if unique keys are used in the device and carefully managed,
    they may provide adequate security.

    The default provider does not use non-volatile storage. It is the
    implementation's responsibility to provide keys and credentials to
    the default Secure Element whenever the system boots up.

*/


/****************************************************************************\
|       The portable API functions.
\****************************************************************************/

LMIC_SecureElement_DECLARE_DRIVER_FNS(Default);

#endif // _lmic_se_default_h_