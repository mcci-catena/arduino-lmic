/*

Module:  lmic_secure_element_interface.h

Function:
    The API type

Copyright & License:
    See accompanying LICENSE file.

Author:
    Terry Moore, MCCI       May 2020

*/

/// \file

#ifndef _lmic_secure_element_interface_h_
#define _lmic_secure_element_interface_h_

#ifndef _lmic_env_h_
# include "../../lmic/lmic_env.h"
#endif

#ifndef _oslmic_types_h_
# include "../../lmic/oslmic_types.h"
#endif

/*! \defgroup lmic_se LMIC Secure Element Interface

\brief This abstract interface represents the Secure Element to the body of the LMIC.

\details
    The LMIC implementation attempts to separate the network protocol implementation
    from the low-level details of cryptography and key managements. The low-level
    details are modeled as being implemented by a _Secure Element_ with the following
    functions.

    - Storage for keys.
    - Encryption primitives that can use the keys to secure, reveal, sign or check
      messages without requiring that the key values be available to the LMIC.
    - Miscellaneous services to minimize duplication of function between
      the Secure Element and the LMIC.

    In order to minimize the code footprint, and in order to minimize the
    number of conditional compiles required in the code, Secure Element
    APIs are called in two steps.

    1. A static-inline wrapper function is called from the LMIC code.
    2. The static in-line function directly calls the similarly-named
       concrete function from the configured secure element.

    This structure allows the C compiler to optimize out the static-inline
    wrapper, and avoids adding conditional compiles or macros directly
    to the LMIC codebase.

    The \ref lmic_se_default "default Secure Element" implementation is
    software-only and uses the code from the IBM LMIC.

*/
/// \{

LMIC_BEGIN_DECLS

/// \brief Values of type and \ref LMIC_SecureElement_Error_t enumerate errors returned by the Secure Element APIs.
enum LMIC_SecureElement_Error_e {
    LMIC_SecureElement_Error_OK = 0,            ///< The operation completed successfully.
    LMIC_SecureElement_Error_InvalidParameter,  ///< Invalid parameter detected.
    LMIC_SecureElement_Error_Permission,        ///< The application doesn't have permission to do this operation
    LMIC_SecureElement_Error_NotProvisioned,    ///< The security element is not provisioned.
    LMIC_SecureElement_Error_InvalidMIC,        ///< The computed MIC was not valid.
    LMIC_SecureElement_Error_Implementation,    ///< Implementation-defined error.
};
/// \brief Controlled-width type for \ref LMIC_SecureElement_Error_e.
typedef enum LMIC_SecureElement_Error_e LMIC_SecureElement_Error_t;

/// \brief an object to carry an AES128 (16-byte) key.
typedef struct LMIC_SecureElement_Aes128Key_s {
    uint8_t bytes[16];  ///< Key data, normally stored in big-endian order.
} LMIC_SecureElement_Aes128Key_t;

/// \brief an object to carry a 64-bit EUI.
typedef struct LMIC_SecureElement_EUI_s {
    uint8_t bytes[8];   ///< EUI-64 data. Normally stored in little-endian order.
} LMIC_SecureElement_EUI_t;

/// \brief an object to carry a LoRaWAN Join Request message.
typedef struct LMIC_SecureLElement_JoinRequest_s {
    uint8_t bytes[23];  ///< Join request data, in network byte order.
} LMIC_SecureElement_JoinRequest_t;

/// \brief Values of this type and \ref LMIC_SecureElement_KeySelector_t select specific keys for encryption/decryption operations.
///
/// \details
///     LoRaWAN 1.0.3 crypto operations use network session keys and application session
///     keys. These keys are always held by the secure element; they may be injected from
///     the LMIC or from the application for device provisioning or dynamic joins to multicast
///     groups.
///
///     Devices (after provisioning) always have unicast keys. After joining a multicast
///     group, they'll have keys for that group.
///
/// \par Notes
///     LoRaWAN 1.1 has an even wider variety of encryption keys. We don't know
///     whether the API is rich enough to support 1.1 operation.

enum LMIC_SecureElement_KeySelector_e {
    LMIC_SecureElement_KeySelector_Unicast = 0, //!< The operation should use the unicast keys
    LMIC_SecureElement_KeySelector_Mc0,         //!< The operation should use the keys for multicast session Mc0
    LMIC_SecureElement_KeySelector_Mc1,         //!< The operation should use the keys for multicast session Mc1
    LMIC_SecureElement_KeySelector_Mc2,         //!< The operation should use the keys for multicast session Mc2
    LMIC_SecureElement_KeySelector_Mc3,         //!< The operation should use the keys for multicast session MC3
    LMIC_SecureElement_KeySelector_AppKey,      //!< The operation should use the (LoRaWAN 1.0.3) Application Key
    LMIC_SecureElement_KeySelector_SIZE         //!< One greater than maximum value in this `enum`.
};
/// \brief Controlled-width type for \ref LMIC_SecureElement_KeySelector_e.
typedef uint8_t LMIC_SecureElement_KeySelector_t;

/// \brief  Names for values of \ref LMIC_SecureElement_KeySelector_t
///
/// \details
///     This is a single string initializer of type `char []` (or
///     `const char []`). It can be used to initialize a string
///     containing names of each of the values of \ref LMIC_SecureElement_KeySelector_t.
///     It saves space by combining the values without requiring pointers to
///     each value, at the cost of run-time indexing into the string
///     in order to extract the value.
///
/// \par Example
///     \code
///     const char KeyNames[] = LMIC_SecureElement_KeySelector_NAME_MULTISZ_INIT;
///     \endcode
///
/// \hideinitializer
///
#define LMIC_SecureElement_KeySelector_NAME_MULTISZ_INIT    \
    "Unicast\0"                                             \
    "Mc0\0"                                                 \
    "Mc1\0"                                                 \
    "Mc2\0"                                                 \
    "Mc3\0"

/// \brief Values of this type and \ref LMIC_SecureElement_JoinFormat_t select specific formats of Join Request.
///
/// \details
///     LoRaWAN 1.1 defines 4 formats of (re)join request messages. Values of this
///     type are used to select the appropriate message format.  LoRaWAN 1.0.3 Devices
///     only use LMIC_SecureElement_JoinFormat_JoinRequest.
///
///     Because C doesn't specify the underlying type used for a given enum, we normally
///     use a separate type \ref \ref LMIC_SecureElement_JoinFormat_t to carry numbers of
///     this value, forcing it to \c uint8_t.

enum LMIC_SecureElement_JoinFormat_e {
    LMIC_SecureElement_JoinFormat_JoinRequest10,        //!< Basic Join Request (1.0.3)
    LMIC_SecureElement_JoinFormat_JoinRequest11,        //!< Basic Join Request (1.1) -- no difference on encode, enables 1.1 features on decode. See \ref LMIC_SecureElement_decodeJoinAccept_t.
    LMIC_SecureElement_JoinFormat_RejoinRequest0,       //!< Rejoin-request type 0
    LMIC_SecureElement_JoinFormat_RejoinRequest1,       //!< Rejoin-request type 1
    LMIC_SecureElement_JoinFormat_RejoinRequest2,       //!< Rejoin-request type 2
    LMIC_SecureElement_JoinFormat_SIZE                  //!< One greater than maximum value in this `enum`.
};
/// \brief Controlled-width type for \ref LMIC_SecureElement_JoinFormat_e.
typedef uint8_t LMIC_SecureElement_JoinFormat_t;


/****************************************************************************\
|       The API function signatures.
\****************************************************************************/

/*!

\defgroup lmic_se_driver APIs for secure-element drivers

\brief  Linkage to secure element implementations

\details
    Function types are used to enforce consistency between the API wrappers
    and driver implementations. We follow MCCI's convention of declaring
    function types rather than pointer-to-function types; this lets one type
    name be used both for declaring pointers to functions and for declaring
    that a given function implements a given API.

    Given that consistency, we further define macros that will generate all
    prototypes for a given driver's methods using consistent names
    (\ref LMIC_SecureElement_DECLARE_DRIVER_FNS).

    The framework implementation currently requires that the secure element
    driver be selected at compile time. However, the architecture allows
    an alternate implementation using pointers and dynamic linkage.

*/

/// \{

/// \brief Initialize the Secure Element
///
/// \returns \ref LMIC_SecureElement_Error_OK for success, some other code for failure.
///
typedef LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_initialize_t(void);

/// \brief Return a random integer uniformly distributed in [0..255].
typedef uint8_t LMIC_ABI_STD
LMIC_SecureElement_getRandomU1_t(void);

/// \brief Return a random integer uniformly distributed in [0..65535].
typedef uint16_t LMIC_ABI_STD
LMIC_SecureElement_getRandomU2_t(void);

/// \brief Fill buffer with random independently distributed integers, each in [0..255].
/// \param buffer [out] Pointer to buffer to be filled.
/// \param nBuffer [in] Number of bytes to be filled.
///
/// \p nBuffer is limited to 255, for maximum implementation flexibility.
///
/// \returns \ref LMIC_SecureElement_Error_OK for success, some other code for failure.
///
typedef LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_fillRandomBuffer_t(uint8_t *buffer, uint8_t nBuffer);

/// \brief Set application key.
/// \param pAppKey [in] Points to 16-byte application key.
///
/// \returns \ref LMIC_SecureElement_Error_OK for success, some other code for failure.
///
typedef LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_setAppKey_t(const LMIC_SecureElement_Aes128Key_t *pAppKey);

/// \brief Get application key.
/// \param pAppKey [out]    Points to buffer to be filled with application key.
///
/// \returns \ref LMIC_SecureElement_Error_OK for success, some other code for failure.
///     Many secure elements will fail this request, because their purpose in life
///     is guarding the application key and preventing tampering.
///
typedef LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_getAppKey_t(LMIC_SecureElement_Aes128Key_t *pAppKey);

///\brief Set network session key.
///
///\param  pNwkSKey [in]   session key to be associated with \p iKey.
///\param  iKey [in]       key discriminator.
///
/// \returns \ref LMIC_SecureElement_Error_OK for success, some other code for failure.
///
typedef LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_setNwkSKey_t(const LMIC_SecureElement_Aes128Key_t *pNwkSKey, LMIC_SecureElement_KeySelector_t iKey);

/// \brief Get network session key.
///
/// \param  pNwkSKey [in]   provides the value of the network session key.
/// \param  iKey [in]       key discriminator.
///
/// \returns \ref LMIC_SecureElement_Error_OK for success, some other code for failure.
///     Many secure elements will fail this request, because their purpose in life
///     is guarding keys and preventing tampering.
///
typedef LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_getNwkSKey_t(LMIC_SecureElement_Aes128Key_t *pNwkSKey, LMIC_SecureElement_KeySelector_t iKey);

/// \brief Set application session key
///
/// \param  pAppSKey [in]   session key to be associated with \p iKey.
/// \param  iKey [in]       key discriminator.
///
/// \returns \ref LMIC_SecureElement_Error_OK for success, some other code for failure.
///
typedef LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_setAppSKey_t(const LMIC_SecureElement_Aes128Key_t *pAppSKey, LMIC_SecureElement_KeySelector_t iKey);

/// \brief Get application session key.
///
/// \param  pAppSKey [out]  set to the value of the application session key.
/// \param  iKey [in]       key discriminator.
///
/// \returns \ref LMIC_SecureElement_Error_OK for success, some other code for failure.
///     Many secure elements will fail this request, because their purpose in life
///     is guarding keys and preventing tampering.
///
typedef LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_getAppSKey_t(LMIC_SecureElement_Aes128Key_t *pAppSKey, LMIC_SecureElement_KeySelector_t iKey);

/// \brief Get device EUI.
///
/// \param  pDevEUI [out]   device EUI.
///
/// \returns \ref LMIC_SecureElement_Error_OK for success, some other code for failure.
///     Many secure elements will fail this request, because their purpose in life
///     is guarding keys and preventing tampering.
///
typedef LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_getDevEUI_t(LMIC_SecureElement_EUI_t *pDevEUI);

/// \brief Set device EUI.
///
/// \param  pDevEUI [in]    Device key for this secure element.
///
/// \returns \ref LMIC_SecureElement_Error_OK for success, some other code for failure.
///
typedef LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_setDevEUI_t(const LMIC_SecureElement_EUI_t *pDevEUI);

/// \brief Get application EUI.
///
/// \param  pAppEUI [out]   application EUI for this secure element.
///
/// \returns \ref LMIC_SecureElement_Error_OK for success, some other code for failure.
///     Many secure elements will fail this request, because their purpose in life
///     is guarding keys and preventing tampering.
///
typedef LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_getAppEUI_t(LMIC_SecureElement_EUI_t *pAppEUI);

/// \brief Set application EUI.
///
/// \param  pAppEUI [in]    application EUI for this secure element.
///
/// \returns \ref LMIC_SecureElement_Error_OK for success, some other code for failure.
///
typedef LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_setAppEUI_t(const LMIC_SecureElement_EUI_t *pAppEUI);

/// \brief Create a join request.
///
/// \param pJoinRequestBytes [out]  Buffer to be filled with the join request.
/// \param joinFormat [in]          Join type selector.
///
/// \details
///     The buffer at * \p pJoinRequestBytes must be at least
///     sizeof(\ref LMIC_SecureElement_JoinRequest_t::bytes) long; it is filled with
///     a \c JoinRequest message, encrypted with the keys suitable to the specified
///     context. \p joinFormat is provided for future use in LoRaWAN 1.1 systems; it selects
///     the keys to be used and the format of the message.  It shall be set to
///     \ref LMIC_SecureElement_JoinFormat_JoinRequest10.
///
/// \returns
///     The result is \ref LMIC_SecureElement_Error_OK for success, some other value for failure.
///
/// \retval LMIC_SecureElement_Error_InvalidParameter indicates that \p iKey was not
///         valid.
///
typedef LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_createJoinRequest_t(
    uint8_t *pJoinRequestBytes, LMIC_SecureElement_JoinFormat_t joinFormat
);

/// \brief Decode a join accept message; capture keys.
///
/// \param  pJoinAcceptBytes [in]       Buffer with raw join accept message
/// \param  nJoinAcceptBytes [in]       Number of bytes
/// \param  pJoinAcceptClearText [out]  Buffer to be filled with decrypted message; same size as \p pJoinAcceptBytes.
/// \param  joinFormat [in]             Type used on message that triggered this join accept.
///
/// \details
///     The Join-Accept message is decoded using the keys designated by the \p joinFormat, and
///     the clear text is placed in \p pJoinAcceptClearText.
///
///     The LMIC presently only supports LoRaWAN 1.0.3, so \p joinFormat must be
///     \ref LMIC_SecureElement_JoinFormat_JoinRequest10.
///
/// \returns
///     The result is \ref LMIC_SecureElement_Error_OK for success, some other value for
///     failure. If successful, the appropriate session keys are generated and stored in
///     the secure element.
///
typedef LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_decodeJoinAccept_t(
    const uint8_t *pJoinAcceptBytes, uint8_t nJoinAcceptBytes,
    uint8_t *pJoinAcceptClearText,
    LMIC_SecureElement_JoinFormat_t joinFormat
);

/// \brief Encode an application uplink message.
///
/// \param  pMessage [in]       Pointer to `MHDR` byte of message to encode.
/// \param  nMessage [in]       Number of bytes, not including the MIC.
/// \param  iPayload [in]       Index of payload field in the message.
/// \param  pCipherTextBuffer [in]  Pointer to buffer to receive phy message.
/// \param  iKey [in]           Key discriminator.
///
/// \details
///     The output buffer must be \p nBuffer + 4 bytes long.
///     \c FCntUp in the message may be ignored by the Secure Element and replaced by SE's idea of
///     the uplink frame count. SE will increment its internal \c FCntUp. To implement \c NbTrans > 1,
///     caller shall cache the encoded message and retransmit it, rather than re-encoding it.
///     \p pCipherTextBuffer may be the same as \p pMessage; in this case the update is done in-place
///     if possible. Otherwise the blocks shall be strictly non-overlapping.
///
/// \returns
///     The result is \ref LMIC_SecureElement_Error_OK for success, some other value for failure.
///
typedef LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_encodeMessage_t(const uint8_t *pMessage, uint8_t nMessage, uint8_t iPayload, uint8_t *pCipherTextBuffer, LMIC_SecureElement_KeySelector_t iKey);

/// \brief Verify the MIC of a downlink message.
///
/// \param  pPhyPayload [in]    Pointer to `MHDR` byte of received PHY message.
/// \param  nPhyPayload [in]    Number of bytes, including the MIC.
/// \param  devAddr [in]        Device address
/// \param  FCntDown [in]       The downlink frame counter to be used with this message
/// \param  iKey [in]           Key discriminator.
///
/// \details
///     The MIC code for the message is calculated using the appropriate key and algorithm.
///
/// \returns
///     Returns \ref LMIC_SecureElement_Error_OK if the MIC matched, or an error code related to
///     the failure. Secure Element drivers are not rigorously tested to return the same error
///     codes in all situations, so any result other than \ref LMIC_SecureElement_Error_OK must
///     be treated as an error.
///
typedef LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_verifyMIC_t(
    const uint8_t *pPhyPayload,
    uint8_t nPhyPayload,
    uint32_t devAddr,
    uint32_t FCntDown,
    LMIC_SecureElement_KeySelector_t iKey
);

/// \brief Decode an application message.
///
/// \param  pPhyPayload [in]    Pointer to `MHDR` byte of received PHY message.
/// \param  nPhyPayload [in]    Number of bytes, including the MIC.
/// \param  devAddr [in]        Device address
/// \param  FCntDown [in]       The downlink frame counter to be used with this message
/// \param  iKey [in]           Key discriminator.
/// \param  pClearTextBuffer [out] Buffer to be used for the output data. Must be
///                             nPhyPayload - 4 bytes long.
///
/// \details
///     The payload is decrypted and placed in \p pClearTextBuffer. The MIC is assumed to be
///     valid. The output buffer must be \p nBuffer - 4 bytes long (the MIC is not appended).
///     \p ClearTextdBuffer may be the same as pPhyPayload, in which
///     case the update is done in-place if possible. Otherwise the input and output blocks shall
///     be strictly non-overlapping.
///
/// \returns
///     Returns \ref LMIC_SecureElement_Error_OK for success, some other value for failure.
///
typedef LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_decodeMessage_t(
    const uint8_t *pPhyPayload, uint8_t nPhyPayload,
    uint32_t devAddr, uint32_t FCntDown,
    LMIC_SecureElement_KeySelector_t iKey,
    uint8_t *pClearTextBuffer
    );

/// \brief Perform an AES128 encryption.
/// \param  pKey [in]       Encryption key (16 bytes).
/// \param  pInput [in]     Clear text (16 bytes).
/// \param  pOutput [out]   Cipher text (16 bytes).
///
/// \details
///     This API is used for beacon calculations and other
///     pseudo-random operations. Key, input buffer, output buffer are all 16 bytes long.
///     pOutput and pInput may be the same buffer; otherwise they must point to non-overlapping
///     regions of memory.  pKey must not overlap pOutput.  None of pKey, pInput, pOutput
///     may be NULL.
///
/// \retval LMIC_SecureElement_Error_OK if the encryption was performed
/// \retval LMIC_SecureElement_Error_InvalidParameter if any of the parameters is invalid.
///
typedef LMIC_SecureElement_Error_t LMIC_ABI_STD
LMIC_SecureElement_aes128Encrypt_t(const uint8_t *pKey, const uint8_t *pInput, uint8_t *pOutput);

/****************************************************************************\
|       The portable API functions.
\****************************************************************************/

/// \brief Declare secure element functions for a given driver
///
/// \param a_driver Fragment of driver name to insert in function names, e.g `Default`.
///
/// \details
///     The parameter is macro-expanded and then substituted to declare each
///     of the interface functions for the specified driver.
///
/// \par Example
/// \parblock
///     \code
///     LMIC_SecureElement_DECLARE_DRIVER_DNS(MyDriver);
///     \endcode
///
///     This expands to a series of function declarations:
///
///     \code
///     LMIC_SecureElement_initialize_t LMIC_SecureElement_MyDriver_initialize;
///     LMIC_SecureElement_getRandomU1_t LMIC_SecureElement_MyDriver_getRandomU1;
///     /* ... */
///     LMIC_SecureElement_aes128Encrypt_t LMIC_SecureElement_MyDriver_aes128Encrypt;
///     \endcode
/// \endparblock
///
/// \hideinitializer
///
#define LMIC_SecureElement_DECLARE_DRIVER_FNS(a_driver)                                         \
    LMIC_SecureElement_initialize_t LMIC_SecureElement_##a_driver##_initialize;                 \
    LMIC_SecureElement_getRandomU1_t LMIC_SecureElement_##a_driver##_getRandomU1;               \
    LMIC_SecureElement_getRandomU2_t LMIC_SecureElement_##a_driver##_getRandomU2;               \
    LMIC_SecureElement_fillRandomBuffer_t LMIC_SecureElement_##a_driver##_fillRandomBuffer;     \
    LMIC_SecureElement_setAppKey_t LMIC_SecureElement_##a_driver##_setAppKey;                   \
    LMIC_SecureElement_getAppKey_t LMIC_SecureElement_##a_driver##_getAppKey;                   \
    LMIC_SecureElement_setAppEUI_t LMIC_SecureElement_##a_driver##_setAppEUI;                   \
    LMIC_SecureElement_getAppEUI_t LMIC_SecureElement_##a_driver##_getAppEUI;                   \
    LMIC_SecureElement_setDevEUI_t LMIC_SecureElement_##a_driver##_setDevEUI;                   \
    LMIC_SecureElement_getDevEUI_t LMIC_SecureElement_##a_driver##_getDevEUI;                   \
    LMIC_SecureElement_setNwkSKey_t LMIC_SecureElement_##a_driver##_setNwkSKey;                 \
    LMIC_SecureElement_getNwkSKey_t LMIC_SecureElement_##a_driver##_getNwkSKey;                 \
    LMIC_SecureElement_setAppSKey_t LMIC_SecureElement_##a_driver##_setAppSKey;                 \
    LMIC_SecureElement_getAppSKey_t LMIC_SecureElement_##a_driver##_getAppSKey;                 \
    LMIC_SecureElement_createJoinRequest_t LMIC_SecureElement_##a_driver##_createJoinRequest;   \
    LMIC_SecureElement_decodeJoinAccept_t LMIC_SecureElement_##a_driver##_decodeJoinAccept;     \
    LMIC_SecureElement_encodeMessage_t LMIC_SecureElement_##a_driver##_encodeMessage;           \
    LMIC_SecureElement_verifyMIC_t LMIC_SecureElement_##a_driver##_verifyMIC;                   \
    LMIC_SecureElement_decodeMessage_t LMIC_SecureElement_##a_driver##_decodeMessage;           \
    LMIC_SecureElement_aes128Encrypt_t LMIC_SecureElement_##a_driver##_aes128Encrypt

LMIC_END_DECLS

// end group lmic_se_driver
/// \}
// end group lmic_se
/// \}

#endif // _lmic_secure_element_interface_h_
