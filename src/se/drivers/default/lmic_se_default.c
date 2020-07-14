/*
 * Copyright (c) 2014-2016 IBM Corporation.
 * All rights reserved.
 *
 * Copyright (c) 2016-2020 MCCI Corporation.
 * All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of the <organization> nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//! \file

//! \ingroup lmic_se_default
//! \{

#include "lmic_se_default.h"

#include "../../../lmic/lmic.h"

static LMIC_SecureElement_EUI_t       s_devEUI;
static LMIC_SecureElement_EUI_t       s_appEUI;
static LMIC_SecureElement_Aes128Key_t s_appKey;

static LMIC_SecureElement_Aes128Key_t s_nwkSKey;
static LMIC_SecureElement_Aes128Key_t s_appSKey;

static inline
uint8_t *
LMIC_SecureElement_DefaultI_GetAppEUIRef(void) {
    return s_appEUI.bytes;
}

static inline
uint8_t *
LMIC_SecureElement_DefaultI_GetDevEUIRef(void) {
    return s_devEUI.bytes;
}

static inline
uint8_t *
LMIC_SecureElement_DefaultI_GetAppKeyRef(void) {
    return s_appKey.bytes;
}

static inline
uint8_t *
LMIC_SecureElement_DefaultI_GetNwkSKeyRef(void) {
    return s_nwkSKey.bytes;
}

static inline
uint8_t *
LMIC_SecureElement_DefaultI_GetAppSKeyRef(void) {
    return s_appSKey.bytes;
}


static void appKeyToAESkey(void) {
    memcpy(AESkey, s_appKey.bytes, sizeof(s_appKey.bytes));
}


/*!

\copydoc LMIC_SecureElement_initialize_t

\par "Implementation Notes"
Initialize the default secure-element implementation for the LMIC. The
API contract for the secure element requires that the LMIC call this function
once during initialization.

\return The default implementation always succeeds.

*/
LMIC_SecureElement_Error_t
LMIC_SecureElement_Default_initialize(void) {
    return LMIC_SecureElement_Error_OK;
}

/*!

\copydoc LMIC_SecureElement_setDevEUI_t

\par "Implementation Notes"
In the default secure element, the device EUI is stored in a static variable without obfuscation.

*/

LMIC_SecureElement_Error_t
LMIC_SecureElement_Default_setDevEUI(const LMIC_SecureElement_EUI_t *pDevEUI) {
    s_devEUI = *pDevEUI;
    return LMIC_SecureElement_Error_OK;
}

/*!

\copydoc LMIC_SecureElement_setAppEUI_t

\par "Implementation Notes"
In the default secure element, the app EUI is stored in a static variable without obfuscation.

*/

LMIC_SecureElement_Error_t
LMIC_SecureElement_Default_setAppEUI(const LMIC_SecureElement_EUI_t *pAppEUI) {
    s_appEUI = *pAppEUI;
    return LMIC_SecureElement_Error_OK;
}

/*!

\copydoc LMIC_SecureElement_setAppKey_t

\par "Implementation Notes"
In the default secure element, the appkey is stored in a static variable without obfuscation.

*/

LMIC_SecureElement_Error_t
LMIC_SecureElement_Default_setAppKey(const LMIC_SecureElement_Aes128Key_t *pAppKey) {
    s_appKey = *pAppKey;
    return LMIC_SecureElement_Error_OK;
}

// ================================================================================
// BEG AES

static void micB0 (u4_t devaddr, u4_t seqno, int dndir, int len) {
    os_clearMem(AESaux,16);
    AESaux[0]  = 0x49;
    AESaux[5]  = dndir?1:0;
    AESaux[15] = len;
    os_wlsbf4(AESaux+ 6,devaddr);
    os_wlsbf4(AESaux+10,seqno);
}


static int aes_verifyMic (const u1_t *key, u4_t devaddr, u4_t seqno, int dndir, const u1_t *pdu, int len) {
    micB0(devaddr, seqno, dndir, len);
    os_copyMem(AESkey,key,16);
    return os_aes(AES_MIC, /* deconst */ (u1_t *) pdu, len) == os_rmsbf4(pdu+len);
}

/*!

\brief Calculate and check the MIC for a downlink packet (default driver).

\copydetails LMIC_SecureElement_verifyMIC_t

\par "Implementation Details" 
    This implementation requires that \p iKey be \ref LMIC_SecureElement_KeySelector_Unicast.

\retval     LMIC_SecureElement_Error_OK means that the MIC was valid.
\retval     LMIC_SecureElement_Error_InvalidMIC means that the MIC was not valid.

*/

LMIC_SecureElement_Error_t
LMIC_SecureElement_Default_verifyMIC(
    const uint8_t *pPhyPayload, uint8_t nPhyPayload,
    uint32_t devAddr, uint32_t FCntDown,
    LMIC_SecureElement_KeySelector_t iKey
) {
    if (nPhyPayload < 4 || iKey != LMIC_SecureElement_KeySelector_Unicast)
        return LMIC_SecureElement_Error_InvalidParameter;
    if (! aes_verifyMic(
            LMIC_SecureElement_DefaultI_GetNwkSKeyRef(), 
            devAddr, FCntDown, 1,
            pPhyPayload, nPhyPayload - 4
            )) {
        return LMIC_SecureElement_Error_InvalidMIC;
    } else {
        return LMIC_SecureElement_Error_OK;
    }
}

static void aes_appendMic (xref2cu1_t key, u4_t devaddr, u4_t seqno, int dndir, xref2u1_t pdu, int len) {
    micB0(devaddr, seqno, dndir, len);
    os_copyMem(AESkey,key,16);
    // MSB because of internal structure of AES
    os_wmsbf4(pdu+len, os_aes(AES_MIC, pdu, len));
}

static void aes_cipher (xref2cu1_t key, u4_t devaddr, u4_t seqno, int dndir, xref2u1_t payload, int len) {
    if( len <= 0 )
        return;
    os_clearMem(AESaux, 16);
    AESaux[0] = AESaux[15] = 1; // mode=cipher / dir=down / block counter=1
    AESaux[5] = dndir?1:0;
    os_wlsbf4(AESaux+ 6,devaddr);
    os_wlsbf4(AESaux+10,seqno);
    os_copyMem(AESkey,key,16);
    os_aes(AES_CTR, payload, len);
}

/*!

\brief Encode an uplink packet (default driver).

\copydetails LMIC_SecureElement_encodeMessage_t

\par "Implementation Details" 
    This implementation assumes that the message is formatted correctly; which
    means that \p iPayload, if less than \p nMessage, is taken as the index of
    the port number. This is used to select either \c AppSKey or \c NwkSKey to
    encode and MIC the message.

\retval     LMIC_SecureElement_Error_OK means that the MIC was valid.
\retval     LMIC_SecureElement_Error_InvalidMIC means that the MIC was not valid.

*/

LMIC_SecureElement_Error_t
LMIC_SecureElement_Default_encodeMessage(
    const uint8_t *pMessage, 
    uint8_t nMessage,
    uint8_t iPayload,
    uint8_t *pCipherTextBuffer,
    LMIC_SecureElement_KeySelector_t iKey
) {
    if (iKey != LMIC_SecureElement_KeySelector_Unicast || nMessage < 5) {
        return LMIC_SecureElement_Error_InvalidParameter;
    }
    if (pCipherTextBuffer != pMessage) {
        os_copyMem(pCipherTextBuffer, pMessage, nMessage);
    }
    const uint8_t nData = nMessage - 4;
    if (iPayload + 1 < nData) {
        // Encrypt the packet -- we have a port number and a non-empty payload.
        // We can cheat, because we are the LMIC default method, so we can use
        // LMIC.devaddr and LMIC.seqnoUp.
        aes_cipher(
            pCipherTextBuffer[iPayload] == 0 ? LMIC_SecureElement_DefaultI_GetNwkSKeyRef()
                                             : LMIC_SecureElement_DefaultI_GetAppSKeyRef(),
            LMIC.devaddr,
            LMIC.seqnoUp - 1,
            /*up*/ 0,
            pCipherTextBuffer + iPayload + 1,
            nData - iPayload - 1
            );
    }
    aes_appendMic(
        LMIC_SecureElement_DefaultI_GetNwkSKeyRef(),
        LMIC.devaddr,
        LMIC.seqnoUp - 1,
        /*up*/ 0,
        pCipherTextBuffer,
        nMessage
        );
    return LMIC_SecureElement_Error_OK;
}

/*!

\brief Decode a received message (default driver).

\copydetails LMIC_SecureElement_decodeMessage_t

\par "Implementation Details"
    - This implementation only supports \p iKey == \ref LMIC_SecureElement_KeySelector_Unicast.
    - The API forces us to do a little extra work, by reaching into the message to
      find the port number. We are careful not to index off the buffer, but we
      don't examine the message structure in detail.

*/

LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_Default_decodeMessage(
    const uint8_t *pPhyPayload, uint8_t nPhyPayload,
    uint32_t devAddr, uint32_t FCntDown,
    LMIC_SecureElement_KeySelector_t iKey, 
    uint8_t *pClearTextBuffer
) {
    if (iKey != LMIC_SecureElement_KeySelector_Unicast ||
        nPhyPayload < 4) {
        return LMIC_SecureElement_Error_InvalidParameter;
    }
    nPhyPayload -= 4;

    if (pClearTextBuffer != pPhyPayload) {
        os_copyMem(pClearTextBuffer, pPhyPayload, nPhyPayload);
    }

    u1_t FOptsLen = pClearTextBuffer[OFF_DAT_FCT] & FCT_OPTLEN;
    u1_t portOffset = OFF_DAT_OPTS + FOptsLen;
    int port = 0;

    // at this moment, for LoRaWAN 1.1, we'd decrypt the
    // from OFF_DAT_FCT for FOptsLen using the NwkSKey.

    // determine the port number, which in turn determines
    // the key to be used in the next step.
    if (portOffset < nPhyPayload) {
        port = pClearTextBuffer[portOffset];
        ++portOffset;
    }

    if (portOffset < nPhyPayload) {
        aes_cipher(
            port != 0 ? LMIC_SecureElement_DefaultI_GetAppSKeyRef()
                      : LMIC_SecureElement_DefaultI_GetNwkSKeyRef(),
            devAddr,
            FCntDown,
            /* downlink */ 1,
            pClearTextBuffer + portOffset,
            nPhyPayload - portOffset
            );
    }

    return LMIC_SecureElement_Error_OK;
}


static void aes_appendMic0 (xref2u1_t pdu, int len) {
    appKeyToAESkey();
    os_wmsbf4(pdu+len, os_aes(AES_MIC|AES_MICNOAUX, pdu, len));  // MSB because of internal structure of AES
}

/*!

\brief Prepare a join request packet (default driver).

\copydetails LMIC_SecureElement_createJoinRequest_t

\par "Implementation Details" 
    This implementation only supports \p joinFormat == \ref LMIC_SecureElement_JoinFormat_JoinRequest10.

*/

LMIC_SecureElement_Error_t
LMIC_SecureElement_Default_createJoinRequest(
    uint8_t *pJoinRequestBytes, 
    LMIC_SecureElement_JoinFormat_t joinFormat
) {
    if (joinFormat != LMIC_SecureElement_JoinFormat_JoinRequest10) {
        return LMIC_SecureElement_Error_InvalidParameter;
    }
    u1_t ftype = HDR_FTYPE_JREQ;

    xref2u1_t d = pJoinRequestBytes;
    d[OFF_JR_HDR] = ftype;
    memcpy(d + OFF_JR_ARTEUI, s_appEUI.bytes, sizeof(s_appEUI.bytes));
    memcpy(d + OFF_JR_DEVEUI, s_devEUI.bytes, sizeof(s_devEUI.bytes));
    os_wlsbf2(d + OFF_JR_DEVNONCE, LMIC.devNonce);
    aes_appendMic0(d, OFF_JR_MIC);

    LMIC.devNonce++;
    DO_DEVDB(LMIC.devNonce,devNonce);
    return LMIC_SecureElement_Error_OK;
}

static int aes_verifyMic0 (xref2u1_t pdu, int len) {
    appKeyToAESkey();
    return os_aes(AES_MIC|AES_MICNOAUX, pdu, len) == os_rmsbf4(pdu+len);
}



static void aes_encrypt (xref2u1_t pdu, int len) {
    appKeyToAESkey();
    os_aes(AES_ENC, pdu, len);
}



static void aes_sessKeys (u2_t devnonce, xref2cu1_t artnonce, xref2u1_t nwkkey, xref2u1_t artkey) {
    os_clearMem(nwkkey, 16);
    nwkkey[0] = 0x01;
    os_copyMem(nwkkey+1, artnonce, LEN_ARTNONCE+LEN_NETID);
    os_wlsbf2(nwkkey+1+LEN_ARTNONCE+LEN_NETID, devnonce);
    os_copyMem(artkey, nwkkey, 16);
    artkey[0] = 0x02;

    appKeyToAESkey();
    os_aes(AES_ENC, nwkkey, 16);
    appKeyToAESkey();
    os_aes(AES_ENC, artkey, 16);
}

/*!

\brief Decode a join accept packet (default driver).

\copydetails LMIC_SecureElement_decodeJoinAccept_t

\par "Implementation Details" 
    This implementation only supports \p joinFormat == \ref LMIC_SecureElement_JoinFormat_JoinRequest10.

*/

LMIC_SecureElement_Error_t
LMIC_SecureElement_Default_decodeJoinAccept(
    const uint8_t *pJoinAcceptBytes, uint8_t nJoinAcceptBytes,
    uint8_t *pJoinAcceptClearText,
    LMIC_SecureElement_JoinFormat_t joinFormat
) {
    if (joinFormat != LMIC_SecureElement_JoinFormat_JoinRequest10) {
        return LMIC_SecureElement_Error_InvalidParameter;
    }
    if (pJoinAcceptClearText != pJoinAcceptBytes) {
        os_copyMem(pJoinAcceptClearText, pJoinAcceptClearText, nJoinAcceptBytes);
    }
    aes_encrypt(pJoinAcceptClearText+1, nJoinAcceptBytes-1);
    if( !aes_verifyMic0(pJoinAcceptClearText, nJoinAcceptBytes-4) ) {
        return LMIC_SecureElement_Error_InvalidMIC;
    }

    // Derive session keys.
    // devNonce already incremented when JOIN REQ got sent off
    aes_sessKeys(
        LMIC.devNonce-1,
        &pJoinAcceptClearText[OFF_JA_ARTNONCE],
        LMIC_SecureElement_DefaultI_GetNwkSKeyRef(), 
        LMIC_SecureElement_DefaultI_GetAppSKeyRef()
        );

    return LMIC_SecureElement_Error_OK;
}

// END AES
// ================================================================================


/*!

\copydoc LMIC_SecureElement_aes128Encrypt_t

\par "Implementation Details"
    This implementation uses os_aes() to do the encryption, which is mapped either
    to os_aes_original() or os_aes_generic(), depending on the AES engine selected
    at compile time.

\post
    \ref AESKey is set to the contents of \p pKey.

*/

LMIC_SecureElement_Error_t
LMIC_SecureElement_Default_aes128Encrypt(
    const uint8_t *pKey, const uint8_t *pInput, uint8_t *pOutput
) {
    if (pInput != pOutput) {
        os_copyMem(pOutput, pInput, 16);
    }
    os_copyMem(AESkey, pKey, 16);
    os_aes(AES_ENC, pOutput, 16);
}

// end of group lmic_se_default.
//! \}
