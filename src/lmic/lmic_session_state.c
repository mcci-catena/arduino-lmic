/*

Module:  lmic_session_state.c

Function:
	Save and restore the LMIC session state as an opaque blob.

Copyright notice and license info:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	September 2026

Description:
	The blob layout is in lmic_session_state.h. This file handles the
	header and MAC state; the channel variant is written and read by the
	active bandplan through LMICbandplan_saveChannelState() and
	LMICbandplan_restoreChannelState().

	Fields that exist only when a feature is compiled in are reached
	through the small functions below, so the save and restore bodies
	have no conditional compilation in them.

*/

#include "lmic_session_state.h"
#include "lmic_bandplan.h"

/****************************************************************************\
|
|	Blob offsets (see the layout in lmic_session_state.h)
|
\****************************************************************************/

enum {
	OFF_TAG = 0,
	OFF_SIZE = 1,
	OFF_REGION = 2,
	OFF_LINK_DR = 3,
	OFF_FCNT_UP = 4,
	OFF_FCNT_DOWN = 8,
	OFF_GPS_TIME = 12,
	OFF_GLOBAL_AVAIL = 16,
	OFF_RX2_FREQUENCY = 20,
	OFF_PING_FREQUENCY = 24,
	OFF_CLIENT_TAG = 28,
	OFF_LINK_INTEGRITY = 30,
	OFF_TX_POWER = 32,
	OFF_REDUNDANCY = 33,
	OFF_DUTY_CYCLE = 34,
	OFF_RX1_DR_OFFSET = 35,
	OFF_RX2_DATA_RATE = 36,
	OFF_RX_DELAY = 37,
	OFF_TX_PARAM = 38,
	OFF_BEACON_CHANNEL = 39,
	OFF_PING_DR = 40,
	OFF_MAC_RX_PARAM_ANS = 41,
	OFF_MAC_DL_CHANNEL_ANS = 42,
	OFF_MAC_RX_TIMING_SETUP_ANS = 43,
	OFF_CHANNELS = 44,
	SIZE_CHANNELS = LMIC_SESSION_STATE_SIZE - OFF_CHANNELS,
};

LMIC_C_ASSERT(SIZE_CHANNELS == 172);

/****************************************************************************\
|
|	Access to fields that exist only when a feature is compiled in.
|
\****************************************************************************/

#if !defined(DISABLE_PING)
static u4_t getPingFreq(void) { return LMIC.ping.freq; }
static void setPingFreq(u4_t v) { LMIC.ping.freq = v; }
static u1_t getPingDr(void) { return LMIC.ping.dr; }
static void setPingDr(u1_t v) { LMIC.ping.dr = v; }
#else
static u4_t getPingFreq(void) { return 0; }
static void setPingFreq(u4_t v) { LMIC_UNREFERENCED_PARAMETER(v); }
static u1_t getPingDr(void) { return 0; }
static void setPingDr(u1_t v) { LMIC_UNREFERENCED_PARAMETER(v); }
#endif

#if LMIC_ENABLE_TxParamSetupReq
static u1_t getTxParam(void) { return LMIC.txParam; }
static void setTxParam(u1_t v) { LMIC.txParam = v; }
#else
static u1_t getTxParam(void) { return 0xFF; }
static void setTxParam(u1_t v) { LMIC_UNREFERENCED_PARAMETER(v); }
#endif

#if !defined(DISABLE_BEACONS)
static u1_t getBcnChnl(void) { return LMIC.bcnChnl; }
static void setBcnChnl(u1_t v) { LMIC.bcnChnl = v; }
#else
static u1_t getBcnChnl(void) { return 0; }
static void setBcnChnl(u1_t v) { LMIC_UNREFERENCED_PARAMETER(v); }
#endif

#if !defined(DISABLE_MCMD_RXParamSetupReq)
static u1_t getDn2Ans(void) { return LMIC.dn2Ans; }
static void setDn2Ans(u1_t v) { LMIC.dn2Ans = v; }
#else
static u1_t getDn2Ans(void) { return 0; }
static void setDn2Ans(u1_t v) { LMIC_UNREFERENCED_PARAMETER(v); }
#endif

#if !defined(DISABLE_MCMD_DlChannelReq)
static u1_t getMacDlChannelAns(void) { return LMIC.macDlChannelAns; }
static void setMacDlChannelAns(u1_t v) { LMIC.macDlChannelAns = v; }
#else
static u1_t getMacDlChannelAns(void) { return 0; }
static void setMacDlChannelAns(u1_t v) { LMIC_UNREFERENCED_PARAMETER(v); }
#endif

#if !defined(DISABLE_MCMD_RXTimingSetupReq)
static u1_t getMacRxTimingSetupAns(void) { return LMIC.macRxTimingSetupAns; }
static void setMacRxTimingSetupAns(u1_t v) { LMIC.macRxTimingSetupAns = v; }
#else
static u1_t getMacRxTimingSetupAns(void) { return 0; }
static void setMacRxTimingSetupAns(u1_t v) { LMIC_UNREFERENCED_PARAMETER(v); }
#endif

/****************************************************************************\
|
|	Helpers
|
\****************************************************************************/

// ticks from now until t, or zero if t is not in the future.
static u4_t ticksUntil(ostime_t t, ostime_t now) {
	ostime_t const delta = t - now;
	return delta > 0 ? (u4_t)delta : 0;
}

// the size the channel variant with a given discriminator must have, or zero.
static u1_t channelVariantSize(u1_t kind) {
	switch (kind) {
	case LMIC_SESSION_STATE_CHANNELS_CONFIGURABLE:	return 172;
	case LMIC_SESSION_STATE_CHANNELS_FIXED72:	return 22;
	case LMIC_SESSION_STATE_CHANNELS_FIXED96:	return 26;
	default:					return 0;
	}
}

/****************************************************************************\
|
|	The API
|
\****************************************************************************/

size_t LMIC_getSessionStateSize(void) {
	return LMIC_SESSION_STATE_SIZE;
}

lmic_session_state_result_t LMIC_querySessionState(
	const u1_t *pBuf, size_t nBuf, lmic_session_state_info_t *pInfo
	) {
	if (pBuf == NULL || nBuf < LMIC_SESSION_STATE_SIZE)
		return LMIC_SESSION_STATE_BUFFER_TOO_SMALL;

	u1_t const tag = pBuf[OFF_TAG];
	if (tag != LMIC_SESSION_STATE_TAG_V1 && tag != LMIC_SESSION_STATE_TAG_V2)
		return LMIC_SESSION_STATE_BAD_TAG;

	if (pBuf[OFF_SIZE] != LMIC_SESSION_STATE_SIZE)
		return LMIC_SESSION_STATE_BAD_SIZE;

	u1_t const kind = pBuf[OFF_CHANNELS];
	u1_t const kindSize = channelVariantSize(kind);
	if (kindSize == 0 || pBuf[OFF_CHANNELS + 1] != kindSize)
		return LMIC_SESSION_STATE_BAD_CHANNEL_VARIANT;

	if (pInfo != NULL) {
		pInfo->version = tag;
		pInfo->region = pBuf[OFF_REGION];
		pInfo->channelKind = kind;
		pInfo->size = pBuf[OFF_SIZE];
		pInfo->clientTag = os_rlsbf2(pBuf + OFF_CLIENT_TAG);
	}
	return LMIC_SESSION_STATE_OK;
}

lmic_session_state_result_t LMIC_saveSessionState(
	u1_t *pBuf, size_t nBuf, size_t *pnUsed, u2_t clientTag
	) {
	if (pnUsed != NULL)
		*pnUsed = 0;
	if (pBuf == NULL || nBuf < LMIC_SESSION_STATE_SIZE)
		return LMIC_SESSION_STATE_BUFFER_TOO_SMALL;

	ostime_t const now = os_getTime();

	pBuf[OFF_TAG] = LMIC_SESSION_STATE_TAG_V2;
	pBuf[OFF_SIZE] = LMIC_SESSION_STATE_SIZE;
	pBuf[OFF_REGION] = CFG_region;
	pBuf[OFF_LINK_DR] = LMIC.datarate;
	os_wlsbf4(pBuf + OFF_FCNT_UP, LMIC.seqnoUp);
	os_wlsbf4(pBuf + OFF_FCNT_DOWN, LMIC.seqnoDn);
	os_wlsbf4(pBuf + OFF_GPS_TIME, 0);
	os_wlsbf4(pBuf + OFF_GLOBAL_AVAIL, ticksUntil(LMIC.globalDutyAvail, now));
	os_wlsbf4(pBuf + OFF_RX2_FREQUENCY, LMIC.dn2Freq);
	os_wlsbf4(pBuf + OFF_PING_FREQUENCY, getPingFreq());
	os_wlsbf2(pBuf + OFF_CLIENT_TAG, clientTag);
	os_wlsbf2(pBuf + OFF_LINK_INTEGRITY, (u2_t)LMIC.adrAckReq);
	pBuf[OFF_TX_POWER] = (u1_t)LMIC.adrTxPow;
	pBuf[OFF_REDUNDANCY] = LMIC.upRepeat;
	pBuf[OFF_DUTY_CYCLE] = LMIC.globalDutyRate;
	pBuf[OFF_RX1_DR_OFFSET] = LMIC.rx1DrOffset;
	pBuf[OFF_RX2_DATA_RATE] = LMIC.dn2Dr;
	pBuf[OFF_RX_DELAY] = LMIC.rxDelay;
	pBuf[OFF_TX_PARAM] = getTxParam();
	pBuf[OFF_BEACON_CHANNEL] = getBcnChnl();
	pBuf[OFF_PING_DR] = getPingDr();
	pBuf[OFF_MAC_RX_PARAM_ANS] = getDn2Ans();
	pBuf[OFF_MAC_DL_CHANNEL_ANS] = getMacDlChannelAns();
	pBuf[OFF_MAC_RX_TIMING_SETUP_ANS] = getMacRxTimingSetupAns();

	os_clearMem(pBuf + OFF_CHANNELS, SIZE_CHANNELS);
	LMICbandplan_saveChannelState(pBuf + OFF_CHANNELS, now);

	if (pnUsed != NULL)
		*pnUsed = LMIC_SESSION_STATE_SIZE;
	return LMIC_SESSION_STATE_OK;
}

lmic_session_state_result_t LMIC_restoreSessionState(
	const u1_t *pBuf, size_t nBuf
	) {
	lmic_session_state_info_t info;
	lmic_session_state_result_t const result = LMIC_querySessionState(pBuf, nBuf, &info);

	if (result != LMIC_SESSION_STATE_OK)
		return result;
	if (info.region != CFG_region)
		return LMIC_SESSION_STATE_REGION_MISMATCH;

	ostime_t const now = os_getTime();

	// the channel variant first: if it is not this build's kind, change nothing.
	if (! LMICbandplan_restoreChannelState(pBuf + OFF_CHANNELS, now, info.version))
		return LMIC_SESSION_STATE_BAD_CHANNEL_VARIANT;

	LMIC.datarate = pBuf[OFF_LINK_DR];
	LMIC.seqnoUp = os_rlsbf4(pBuf + OFF_FCNT_UP);
	LMIC.seqnoDn = os_rlsbf4(pBuf + OFF_FCNT_DOWN);
	LMIC.globalDutyAvail = now + (ostime_t)os_rlsbf4(pBuf + OFF_GLOBAL_AVAIL);
	LMIC.dn2Freq = os_rlsbf4(pBuf + OFF_RX2_FREQUENCY);
	setPingFreq(os_rlsbf4(pBuf + OFF_PING_FREQUENCY));
	LMIC.adrAckReq = (s2_t)os_rlsbf2(pBuf + OFF_LINK_INTEGRITY);
	LMIC.adrTxPow = (s1_t)pBuf[OFF_TX_POWER];
	LMIC.upRepeat = pBuf[OFF_REDUNDANCY];
	LMIC.globalDutyRate = pBuf[OFF_DUTY_CYCLE];
	LMIC.rx1DrOffset = pBuf[OFF_RX1_DR_OFFSET];
	LMIC.dn2Dr = pBuf[OFF_RX2_DATA_RATE];
	LMIC.rxDelay = pBuf[OFF_RX_DELAY];
	setTxParam(pBuf[OFF_TX_PARAM]);
	setBcnChnl(pBuf[OFF_BEACON_CHANNEL]);
	setPingDr(pBuf[OFF_PING_DR]);
	setDn2Ans(pBuf[OFF_MAC_RX_PARAM_ANS]);
	setMacDlChannelAns(pBuf[OFF_MAC_DL_CHANNEL_ANS]);
	setMacRxTimingSetupAns(pBuf[OFF_MAC_RX_TIMING_SETUP_ANS]);

	return LMIC_SESSION_STATE_OK;
}
