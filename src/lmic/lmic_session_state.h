/*

Module:  lmic_session_state.h

Function:
	Save and restore the LMIC session state as an opaque blob.

Copyright notice and license info:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	September 2026

Description:
	A client that wants a session to survive a reset or a sleep calls
	LMIC_saveSessionState() after each transaction and stores the bytes
	it gets; after the next LMIC_reset() it calls
	LMIC_restoreSessionState() with the same bytes. The blob is opaque:
	its version and region are inside it, and
	LMIC_querySessionState() reports them.

	The layout is the one Arduino-LoRaWAN has stored since 2021 as its
	SessionStateV1, byte for byte, so blobs already in the field
	restore. The LMIC reads V1 and V2 and writes V2; the two differ
	only in the header tag. In V1 the channel-group duty-cycle divisor
	was saved from the wrong field (mcci-catena/arduino-lorawan#231),
	so a V1 restore takes it from the region defaults instead.

	The channel part of the blob is a union with a discriminator and
	three variants: configurable channels (16 frequencies and groups),
	72 fixed channels, and 96 fixed channels.

	All multi-byte fields are little-endian regardless of the host,
	except the 24-bit packed frequencies, which are most-significant
	byte first in units of 100 Hz, as V1 wrote them.

	See mcci-catena/arduino-lmic#1089.

*/

#ifndef _lmic_session_state_h_	/* prevent multiple includes */
#define _lmic_session_state_h_

#include <stddef.h>

#ifndef _lmic_h_
# include "lmic.h"
#endif

LMIC_BEGIN_DECLS

/****************************************************************************\
|
|	Blob layout
|
|	Offsets are in bytes from the start of the blob. Types: u1, u2, u4 are
|	unsigned little-endian; s2 is signed little-endian; f3 is a 24-bit
|	frequency, MSB first, in units of 100 Hz.
|
|	Header and MAC state (44 bytes)
|
|	  0  u1   Tag             LMIC_SESSION_STATE_TAG_V1 or _V2
|	  1  u1   Size            total size of the blob, 216
|	  2  u1   Region          LMIC_REGION_* code
|	  3  u1   LinkDR          LMIC.datarate
|	  4  u4   FCntUp          LMIC.seqnoUp
|	  8  u4   FCntDown        LMIC.seqnoDn
|	 12  u4   gpsTime         reserved, zero
|	 16  u4   globalAvail     LMIC.globalDutyAvail - now at save, in ticks;
|	                          restored as now + value, clamped at zero
|	 20  u4   Rx2Frequency    LMIC.dn2Freq, Hz
|	 24  u4   PingFrequency   LMIC.ping.freq, or zero without DISABLE_PING
|	 28  u2   ClientTag       not interpreted (Arduino-LoRaWAN: country code)
|	 30  s2   LinkIntegrity   LMIC.adrAckReq
|	 32  u1   TxPower         LMIC.adrTxPow
|	 33  u1   Redundancy      LMIC.upRepeat (NbTrans)
|	 34  u1   DutyCycle       LMIC.globalDutyRate
|	 35  u1   Rx1DRoffset     LMIC.rx1DrOffset
|	 36  u1   Rx2DataRate     LMIC.dn2Dr
|	 37  u1   RxDelay         LMIC.rxDelay
|	 38  u1   TxParam         LMIC.txParam, or 0xFF without LMIC_ENABLE_TxParamSetupReq
|	 39  u1   BeaconChannel   LMIC.bcnChnl, or zero with DISABLE_BEACONS
|	 40  u1   PingDr          LMIC.ping.dr, or zero with DISABLE_PING
|	 41  u1   MacRxParamAns   LMIC.dn2Ans
|	 42  u1   MacDlChannelAns LMIC.macDlChannelAns
|	 43  u1   MacRxTimingSetupAns  LMIC.macRxTimingSetupAns
|
|	Channels (172 bytes, from offset 44): a union with a discriminator at
|	44 and the variant's own size at 45. The fixed-channel variants are
|	smaller than the union; the bytes after them are zero.
|
|	 44  u1   Kind            LMIC_SESSION_STATE_CHANNELS_* (the discriminator)
|	 45  u1   Size            172, 22 or 26 by kind
|
|	Configurable-channel variant (kind 0, 172 bytes; offsets from 44)
|
|	  2  --   padding, two bytes, zero
|	  4  u4   ChannelGroups   two bits per channel: channel group of channel i
|	  8  u2   ChannelMap      LMIC.channelMap
|	 10  u2   ChannelShuffleMap  LMIC.channelShuffleMap
|	 12  u2[16] ChannelDrMap  LMIC.channelDrMap
|	 44  f3[16] UplinkFreq    LMIC.channelFreq[i] & ~3
|	 92  f3[16] DownlinkFreq  LMIC.channelDlFreq[i], or zero
|	140  8x4  Groups          one per channel group, see below
|
|	Channel group entry (8 bytes)
|
|	  0  u2   txDutyDenom     LMIC.bands[i].txcap (V1: unreliable, ignored on restore)
|	  2  u1   txPower         LMIC.bands[i].txpow
|	  3  u1   lastChannel     LMIC.bands[i].lastchnl
|	  4  u4   ostimeAvail     LMIC.bands[i].avail - now at save, clamped at
|	                          zero; restored as now + value
|
|	Fixed-channel variant, 72 channels (kind 1, 22 bytes; offsets from 44)
|
|	  2  u1[10] ChannelMap    LMIC.channelMap as bytes, channel i at bit i
|	 12  u1[10] ChannelShuffleMap  LMIC.channelShuffleMap likewise
|
|	Fixed-channel variant, 96 channels (kind 2, 26 bytes): as kind 1 with
|	12-byte maps. Not produced by this version of the LMIC.
|
\****************************************************************************/

/// \brief size in bytes of a session state blob, either version.
enum { LMIC_SESSION_STATE_SIZE = 216 };

/// \brief header tags (blob offset 0).
enum {
	LMIC_SESSION_STATE_TAG_NULL = 0,	///< no state
	LMIC_SESSION_STATE_TAG_V1 = 1,		///< Arduino-LoRaWAN V1; txDutyDenom unreliable
	LMIC_SESSION_STATE_TAG_V2 = 2,		///< same layout; every field reliable
};

/// \brief channel variant discriminators (blob offset 44).
enum {
	LMIC_SESSION_STATE_CHANNELS_CONFIGURABLE = 0,	///< up to 16 channels with frequencies and groups
	LMIC_SESSION_STATE_CHANNELS_FIXED72 = 1,	///< 72 fixed channels, enable and shuffle masks
	LMIC_SESSION_STATE_CHANNELS_FIXED96 = 2,	///< 96 fixed channels; not produced yet
};

/// \brief results of the session state calls.
typedef enum lmic_session_state_result_e {
	LMIC_SESSION_STATE_OK = 0,
	LMIC_SESSION_STATE_BUFFER_TOO_SMALL,	///< buffer shorter than the blob
	LMIC_SESSION_STATE_BAD_TAG,		///< header tag is not a version this LMIC reads
	LMIC_SESSION_STATE_BAD_SIZE,		///< header size is not the size for the tag
	LMIC_SESSION_STATE_BAD_CHANNEL_VARIANT,	///< channel variant discriminator or size is wrong
	LMIC_SESSION_STATE_REGION_MISMATCH,	///< blob was saved for another region
	LMIC_SESSION_STATE_REGION_NOT_AVAILABLE,	///< the blob's region is not in this build
} lmic_session_state_result_t;

/// \brief what LMIC_querySessionState() reports about a blob.
typedef struct lmic_session_state_info_s {
	u1_t	version;	///< header tag, LMIC_SESSION_STATE_TAG_*
	u1_t	region;		///< LMIC_REGION_* code
	u1_t	channelKind;	///< LMIC_SESSION_STATE_CHANNELS_*
	u1_t	size;		///< total size of the blob, bytes
	u2_t	clientTag;	///< the client's tag, as passed to LMIC_saveSessionState()
} lmic_session_state_info_t;

/// \brief return the number of bytes LMIC_saveSessionState() needs.
size_t LMIC_getSessionStateSize(void);

/// \brief save the session state.
///
/// \param pBuf		receives the blob.
/// \param nBuf		size of \p pBuf; must be at least LMIC_getSessionStateSize().
/// \param pnUsed	if not NULL, receives the number of bytes written.
/// \param clientTag	stored at offset 28 and returned by the query; the LMIC
///			does not interpret it.
lmic_session_state_result_t LMIC_saveSessionState(
	u1_t *pBuf, size_t nBuf, size_t *pnUsed, u2_t clientTag
	);

/// \brief restore the session state from a blob.
///
/// The blob must pass LMIC_querySessionState() and its region must be the
/// active region. Time-relative fields are taken as deltas from now. Channel
/// groups keep their regulatory limits unless the blob is V2. Call after
/// LMIC_reset() and before starting a transaction.
lmic_session_state_result_t LMIC_restoreSessionState(
	const u1_t *pBuf, size_t nBuf
	);

/// \brief check a blob and report its version, region, kind, size and client tag.
///
/// Reads nothing but the blob; the LMIC state is not touched.
lmic_session_state_result_t LMIC_querySessionState(
	const u1_t *pBuf, size_t nBuf, lmic_session_state_info_t *pInfo
	);

LMIC_END_DECLS

#endif /* _lmic_session_state_h_ */
