/*

Module:  lmic_accessors.h

Function:
	Accessors for the LMIC instance: getters for status and settings,
	setters for the few things a client may change directly.

Copyright notice and license info:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	September 2026

Description:
	Clients should use these instead of reading or writing fields of
	the LMIC object. The field layout changes in V7 (regional state
	moves into unions); these accessors do not.

	Where the LMIC field name differs from the LoRaWAN specification's
	name for the same thing, the accessor uses the specification's name
	(LoRaWAN 1.0.3 and Regional Parameters 1.0.3revA): FCntUp, FCntDown,
	DevAddr, NetID, DevNonce, DataRate, TXPower, ChIndex, RX2DataRate,
	FPort, FRMPayload, RSSI, SNR. Things the specification does not name
	(operating mode, radio parameter word, timing) keep LMIC names.

	Fields that exist only when a feature is compiled in (beacons,
	ping, DeviceTimeReq) have getters only when the feature is
	compiled in. Fields that depend on the region have getters that
	return false when the active region has no such field.

	See mcci-catena/arduino-lmic#1089.

*/

#ifndef _lmic_accessors_h_	/* prevent multiple includes */
#define _lmic_accessors_h_

#ifndef _lmic_h_
# include "lmic.h"
#endif

LMIC_BEGIN_DECLS

/****************************************************************************\
|
|	Region
|
\****************************************************************************/

/// \brief the kind of channel plan a region uses.
typedef enum lmic_region_kind_e {
	LMIC_REGION_KIND_CONFIGURABLE = 1,	///< up to 16 configurable channels (EU868, AS923, IN866, KR920)
	LMIC_REGION_KIND_FIXED = 2,		///< 72 or 96 fixed channels selected by mask (US915, AU915)
} lmic_region_kind_t;

/// \brief return the active region code (one of \c LMIC_REGION_*).
static inline u1_t LMIC_getRegion(void) {
	return CFG_region;
}

/// \brief return the kind of channel plan the active region uses.
static inline lmic_region_kind_t LMIC_getRegionKind(void) {
#if CFG_LMIC_EU_like
	return LMIC_REGION_KIND_CONFIGURABLE;
#else
	return LMIC_REGION_KIND_FIXED;
#endif
}

/****************************************************************************\
|
|	Radio and link status
|
\****************************************************************************/

/// \brief return the most recent frequency, in Hz.
static inline u4_t LMIC_getFrequency(void) {
	return LMIC.freq;
}

/// \brief return the current uplink data rate (DataRate).
static inline dr_t LMIC_getDataRate(void) {
	return LMIC.datarate;
}

/// \brief return the current radio parameter selection (SF, BW, CR, ...).
static inline rps_t LMIC_getRps(void) {
	return LMIC.rps;
}

/// \brief return the TXPower set by ADR (LinkADRReq), in dBm.
static inline s1_t LMIC_getAdrTXPower(void) {
	return LMIC.adrTxPow;
}

/// \brief return the transmit power the radio is told to use, in dBm.
static inline s1_t LMIC_getRadioTxPower(void) {
	return LMIC.txpow;
}

/// \brief return the RSSI of the last received frame, in dBm.
static inline s1_t LMIC_getRSSI(void) {
	return LMIC.rssi;
}

/// \brief return the SNR of the last received frame, times 4.
static inline s1_t LMIC_getSNR(void) {
	return LMIC.snr;
}

/// \brief return the channel index (ChIndex) for the next transmission.
static inline u1_t LMIC_getTxChIndex(void) {
	return LMIC.txChnl;
}

/// \brief return the RX2 data rate (RX2DataRate).
static inline dr_t LMIC_getRX2DataRate(void) {
	return LMIC.dn2Dr;
}

/// \brief return the RX2 frequency, in Hz.
static inline u4_t LMIC_getRX2Frequency(void) {
	return LMIC.dn2Freq;
}

/// \brief return the receive-window timeout, in symbols.
static inline rxsyms_t LMIC_getRxsyms(void) {
	return LMIC.rxsyms;
}

/// \brief return the last LoRa IRQ flags seen by the radio driver.
static inline u1_t LMIC_getSaveIrqFlags(void) {
	return LMIC.saveIrqFlags;
}

/// \brief return non-zero if receive IQ inversion is disabled.
static inline bit_t LMIC_getNoRxIqInversion(void) {
	return LMIC.noRXIQinversion != 0;
}

/// \brief return the listen-before-talk time, in ticks (zero if LBT is off).
static inline ostime_t LMIC_getLbtTicks(void) {
	return LMIC.lbt_ticks;
}

/// \brief return the listen-before-talk threshold, in dBm.
static inline s1_t LMIC_getLbtDbmax(void) {
	return LMIC.lbt_dbmax;
}

/****************************************************************************\
|
|	Timing
|
\****************************************************************************/

/// \brief return the time the last transmission ended.
static inline ostime_t LMIC_getTxend(void) {
	return LMIC.txend;
}

/// \brief return the time the last reception ended.
static inline ostime_t LMIC_getRxtime(void) {
	return LMIC.rxtime;
}

/// \brief return the time the next receive window opens.
static inline ostime_t LMIC_getNextRxTime(void) {
	return LMIC.nextRxTime;
}

/// \brief return the time at which the device may transmit again.
static inline ostime_t LMIC_getGlobalDutyAvail(void) {
	return LMIC.globalDutyAvail;
}

/****************************************************************************\
|
|	MAC state
|
\****************************************************************************/

/// \brief return the operating-mode flags (\c OP_*).
static inline u2_t LMIC_getOpmode(void) {
	return LMIC.opmode;
}

/// \brief return the transaction flags of the last TX/RX (\c TXRX_*).
static inline u2_t LMIC_getTxrxFlags(void) {
	return LMIC.txrxFlags;
}

/// \brief return the uplink frame counter (FCntUp).
static inline u4_t LMIC_getFCntUp(void) {
	return LMIC.seqnoUp;
}

/// \brief return the downlink frame counter (FCntDown).
static inline u4_t LMIC_getFCntDown(void) {
	return LMIC.seqnoDn;
}

/// \brief return the device address (DevAddr); zero if not joined.
static inline devaddr_t LMIC_getDevAddr(void) {
	return LMIC.devaddr;
}

/// \brief return the network identifier (NetID).
static inline u4_t LMIC_getNetID(void) {
	return LMIC.netid;
}

/// \brief return the last generated DevNonce.
static inline u2_t LMIC_getDevNonce(void) {
	return LMIC.devNonce;
}

/****************************************************************************\
|
|	Received data
|
\****************************************************************************/

/// \brief get the FRMPayload of the last received frame.
///
/// \return the FPort, or zero if there is no payload. On return,
///	\p *ppData points at the payload and \p *pLen holds its length.
static inline u1_t LMIC_getFRMPayload(const u1_t **ppData, u1_t *pLen) {
	if (LMIC.dataLen == 0 || LMIC.dataBeg == 0) {
		*ppData = NULL;
		*pLen = 0;
		return 0;
	}
	*ppData = LMIC.frame + LMIC.dataBeg;
	*pLen = LMIC.dataLen;
	return LMIC.frame[LMIC.dataBeg - 1];
}

/// \brief return the length of the FRMPayload of the last received frame.
static inline u1_t LMIC_getFRMPayloadLen(void) {
	return LMIC.dataLen;
}

/****************************************************************************\
|
|	Setters for raw (non-LoRaWAN) transmission
|
\****************************************************************************/

/// \brief set the frequency for a raw transmission, in Hz.
static inline void LMIC_setFrequency(u4_t freq) {
	LMIC.freq = freq;
}

/// \brief set the data rate for a raw transmission.
static inline void LMIC_setDataRate(dr_t dr) {
	LMIC.datarate = dr;
}

/// \brief set the radio parameters for a raw transmission.
static inline void LMIC_setRps(rps_t rps) {
	LMIC.rps = rps;
}

/// \brief set the transmit power the radio uses for a raw transmission, in dBm.
static inline void LMIC_setRadioTxPower(s1_t txpow) {
	LMIC.txpow = txpow;
}

/// \brief set the listen-before-talk parameters (ticks of zero turns LBT off).
static inline void LMIC_setLbt(ostime_t ticks, s1_t dbmax) {
	LMIC.lbt_ticks = ticks;
	LMIC.lbt_dbmax = dbmax;
}

/// \brief enable or disable receive IQ inversion.
static inline void LMIC_setNoRxIqInversion(bit_t fNoInversion) {
	LMIC.noRXIQinversion = fNoInversion;
}

/// \brief set the RX2 data rate (RX2DataRate).
static inline void LMIC_setRX2DataRate(dr_t dr) {
	LMIC.dn2Dr = dr;
}

/// \brief load the frame buffer for a raw transmission.
///
/// \return the number of bytes loaded, which is less than \p len if \p len
///	exceeds the frame buffer.
static inline u1_t LMIC_setRawTxData(const u1_t *pData, u1_t len) {
	// MAX_LEN_FRAME is an int enumerator, usually 255. Comparing a u1_t
	// against it directly draws -Wtype-limits ("always false") in that
	// configuration; comparing against a u1_t holding the same value does not,
	// and still clamps when MAX_LEN_FRAME is configured to be less than 255.
	u1_t const maxLen = MAX_LEN_FRAME;

	if (len > maxLen)
		len = maxLen;
	os_copyMem(LMIC.frame, pData, len);
	LMIC.dataLen = len;
	return len;
}

/// \brief get the frame received in raw mode (no LoRaWAN framing).
///
/// On return, \p *ppData points at the frame and \p *pLen holds its length.
static inline void LMIC_getRawRxData(const u1_t **ppData, u1_t *pLen) {
	*ppData = LMIC.frame;
	*pLen = LMIC.dataLen;
}

/// \brief set the time the next raw receive starts (use \c os_getTime() for now).
static inline void LMIC_setNextRxTime(ostime_t t) {
	LMIC.nextRxTime = t;
}

LMIC_END_DECLS

#endif /* _lmic_accessors_h_ */
