/*

Module:  lmic_secure_element_api.h

Function:
    The API type

Copyright & License:
    See accompanying LICENSE file.

Author:
    Terry Moore, MCCI       May 2020

*/

/// \file

#ifndef _lmic_secure_element_api_h_
#define _lmic_secure_element_api_h_

#ifndef _lmic_secure_element_interface_h_
# include "lmic_secure_element_interface.h"
#endif

/// \ingroup lmic_se
/// \{

/****************************************************************************\
|   The configuration. For now, we don't support anything other than
|   static binding.
\****************************************************************************/

#if ! defined(LMIC_CFG_SecureElement_DRIVER)
# define LMIC_CFG_SecureElement_DRIVER   Default
#endif

#if ! defined(LMIC_ENABLE_SecureElement_STATIC)
#define LMIC_ENABLE_SecureElement_STATIC    1
#endif

/****************************************************************************\
|       The portable API functions.
\****************************************************************************/

#if ! LMIC_ENABLE_SecureElement_STATIC
# error "This version only supports static binding to the secure element."
#endif

/*
|| Declare external linkage for the APIs
*/
#if ! LMIC_ENABLE_SecureElement_STATIC
LMIC_SecureElement_initialize_t LMIC_SecureElement_initialize;
LMIC_SecureElement_getRandomU1_t LMIC_SecureElement_getRandomU1;
LMIC_SecureElement_getRandomU2_t LMIC_SecureElement_getRandomU2;
LMIC_SecureElement_fillRandomBuffer_t LMIC_SecureElement_fillRandomBuffer;
LMIC_SecureElement_setAppKey_t LMIC_SecureElement_setAppKey;
LMIC_SecureElement_getAppKey_t LMIC_SecureElement_getAppKey;
LMIC_SecureElement_setNwkSKey_t LMIC_SecureElement_setNwkSKey;
LMIC_SecureElement_getNwkSKey_t LMIC_SecureElement_getNwkSKey;
LMIC_SecureElement_setAppSKey_t LMIC_SecureElement_setAppSKey;
LMIC_SecureElement_getAppSKey_t LMIC_SecureElement_getAppSKey;
LMIC_SecureElement_createJoinRequest_t LMIC_SecureElement_createJoinRequest;
LMIC_SecureElement_decodeJoinAccept_t LMIC_SecureElement_decodeJoinAccept;
LMIC_SecureElement_encodeMessage_t LMIC_SecureElement_encodeMessage;
LMIC_SecureElement_verifyMIC_t LMIC_SecureElement_verifyMIC;
LMIC_SecureElement_decodeMessage_t LMIC_SecureElement_decodeMessage;
LMIC_SecureElement_aes128Encrypt_t LMIC_SecureElement_aes128Encrypt;
#else // LMIC_ENABLE_SecureElement_STATIC

/*
|| Use static linkage for the APIs.
*/
#define LMIC_SecureElement_METHOD_(a_driver, a_fn)  \
    (LMIC_SecureElement_##a_driver##_##a_fn)
#define LMIC_SecureElement_METHOD(a_driver, a_fn)  \
    LMIC_SecureElement_METHOD_(a_driver, a_fn)

LMIC_SecureElement_DECLARE_DRIVER_FNS(Default);

/// \copydoc LMIC_SecureElement_initialize_t
static inline
LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_initialize(void) {
    LMIC_SecureElement_METHOD(LMIC_CFG_SecureElement_DRIVER, initialize)();
}

/// \copydoc LMIC_SecureElement_getRandomU1_t
static inline
uint8_t LMIC_ABI_STD
LMIC_SecureElement_getRandomU1(void) {
    return LMIC_SecureElement_METHOD(LMIC_CFG_SecureElement_DRIVER, getRandomU1)();
}

/// \copydoc LMIC_SecureElement_getRandomU2_t
static inline
uint16_t LMIC_ABI_STD
LMIC_SecureElement_getRandomU2(void) {
    return LMIC_SecureElement_METHOD(LMIC_CFG_SecureElement_DRIVER, getRandomU2)();
}

/// \copydoc LMIC_SecureElement_fillRandomBuffer_t
static inline
LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_fillRandomBuffer(uint8_t *buffer, uint8_t nBuffer) {
    return LMIC_SecureElement_METHOD(LMIC_CFG_SecureElement_DRIVER, fillRandomBuffer)(buffer, nBuffer);
}

/// \copydoc LMIC_SecureElement_setAppKey_t
static inline
LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_setAppKey(const LMIC_SecureElement_Aes128Key_t *pAppKey) {
    return LMIC_SecureElement_METHOD(LMIC_CFG_SecureElement_DRIVER, setAppKey)(pAppKey);
}

/// \copydoc LMIC_SecureElement_getAppKey_t
static inline
LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_getAppKey(LMIC_SecureElement_Aes128Key_t *pAppKey) {
    return LMIC_SecureElement_METHOD(LMIC_CFG_SecureElement_DRIVER, getAppKey)(pAppKey);
}

/// \copydoc LMIC_SecureElement_setNwkSKey_t
static inline
LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_setNwkSKey(const LMIC_SecureElement_Aes128Key_t *pNwkSKey, LMIC_SecureElement_KeySelector_t iKey) {
    return LMIC_SecureElement_METHOD(LMIC_CFG_SecureElement_DRIVER, setNwkSKey)(pNwkSKey, iKey);
}

/// \copydoc LMIC_SecureElement_getNwkSKey_t
static inline
LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_getNwkSKey(LMIC_SecureElement_Aes128Key_t *pNwkSKey, LMIC_SecureElement_KeySelector_t iKey) {
    return LMIC_SecureElement_METHOD(LMIC_CFG_SecureElement_DRIVER, getNwkSKey)(pNwkSKey, iKey);
}

/// \copydoc LMIC_SecureElement_setAppSKey_t
static inline
LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_setAppSKey(const LMIC_SecureElement_Aes128Key_t *pAppSKey, LMIC_SecureElement_KeySelector_t iKey) {
    return LMIC_SecureElement_METHOD(LMIC_CFG_SecureElement_DRIVER, setAppSKey)(pAppSKey, iKey);
}

/// \copydoc LMIC_SecureElement_getAppSKey_t
static inline
LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_getAppSKey(LMIC_SecureElement_Aes128Key_t *pAppSKey, LMIC_SecureElement_KeySelector_t iKey) {
    return LMIC_SecureElement_METHOD(LMIC_CFG_SecureElement_DRIVER, getAppSKey)(pAppSKey, iKey);
}

/// \copydoc LMIC_SecureElement_createJoinRequest_t
static inline
LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_createJoinRequest(
    uint8_t *pJoinRequestBytes, LMIC_SecureElement_JoinFormat_t joinFormat
) {
    return LMIC_SecureElement_METHOD(LMIC_CFG_SecureElement_DRIVER, createJoinRequest)(pJoinRequestBytes, joinFormat);
}

/// \copydoc LMIC_SecureElement_decodeJoinAccept_t
static inline
LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_decodeJoinAccept(
    const uint8_t *pJoinAcceptBytes, uint8_t nJoinAcceptBytes,
    uint8_t *pJoinAcceptClearText,
    LMIC_SecureElement_JoinFormat_t joinFormat
) {
    return LMIC_SecureElement_METHOD(LMIC_CFG_SecureElement_DRIVER, decodeJoinAccept)(
        pJoinAcceptBytes, nJoinAcceptBytes, pJoinAcceptClearText, joinFormat
        );
}

/// \copydoc LMIC_SecureElement_encodeMessage_t
static inline
LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_encodeMessage(const uint8_t *pMessage, uint8_t nMessage, uint8_t iPayload, uint8_t *pCipherTextBuffer, LMIC_SecureElement_KeySelector_t iKey) {
    return LMIC_SecureElement_METHOD(LMIC_CFG_SecureElement_DRIVER, encodeMessage)(
        pMessage, nMessage, iPayload, pCipherTextBuffer, iKey
        );
}

/// \copydoc LMIC_SecureElement_verifyMIC_t
static inline
LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_verifyMIC(
    const uint8_t *pPhyPayload,
    uint8_t nPhyPayload,
    uint32_t devAddr,
    uint32_t FCntDown,
    LMIC_SecureElement_KeySelector_t iKey
) {
    return LMIC_SecureElement_METHOD(LMIC_CFG_SecureElement_DRIVER, verifyMIC)(
        pPhyPayload, nPhyPayload, devAddr, FCntDown, iKey
        );
}

/// \copydoc LMIC_SecureElement_decodeMessage_t
static inline
LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_decodeMessage(
    const uint8_t *pPhyPayload, uint8_t nPhyPayload,
    uint32_t devAddr, uint32_t FCntDown,
    LMIC_SecureElement_KeySelector_t iKey, 
    uint8_t *pClearTextBuffer
) {
    return LMIC_SecureElement_METHOD(LMIC_CFG_SecureElement_DRIVER, decodeMessage)(
        pPhyPayload, nPhyPayload, devAddr, FCntDown, iKey, pClearTextBuffer
        );
}

/// \copydoc LMIC_SecureElement_aes128Encrypt_t
static inline
LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_aes128Encrypt(const uint8_t *pKey, const uint8_t *pInput, uint8_t *pOutput) {
    return LMIC_SecureElement_METHOD(LMIC_CFG_SecureElement_DRIVER, aes128Encrypt)(
        pKey, pInput, pOutput
        );
}

#endif // LMIC_ENABLE_SecureElement_STATIC

LMIC_END_DECLS

// end group lmic_se
/// \}

#endif // _lmic_secure_element_api_h_
