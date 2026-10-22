/*

Module:  test_mac_split.c

Function:
	Host tests for #1160: MAC data that doesn't fit with the application
	payload goes in a frame of its own, ahead of the payload.

Copyright and License:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	October 2026

Description:
	Runs the real scheduler on hal_stub.c with the simulated radio on,
	in an ABP session. Each uplink is recorded at EV_TXSTART. A downlink
	can be put in the next receive window with
	hal_stub_injectDownlink(). Exit status is the number of failed
	checks.

*/

#include "hal_stub.h"
#include "lmic.h"
#include "lmic_bandplan.h"
#include "lmic_aes_api.h"

#include <stdio.h>
#include <string.h>

static int s_failures;
static int s_checks;

#define CHECK(cond) do {							\
	++s_checks;								\
	if (!(cond)) {								\
		++s_failures;							\
		printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);		\
	}									\
} while (0)

#define CHECK_EQ(a, b) do {							\
	long long const a_ = (long long)(a), b_ = (long long)(b);		\
	++s_checks;								\
	if (a_ != b_) {								\
		++s_failures;							\
		printf("FAIL %s:%d: %s == %s: %lld != %lld\n",			\
			__FILE__, __LINE__, #a, #b, a_, b_);			\
	}									\
} while (0)

#define TICKS	((u4_t) LMIC_OSTICKS_PER_SEC)

enum { DEVADDR = 0x26012345, APP_PORT = 1 };
static u1_t s_nwkSKey[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
static u1_t s_appSKey[16] = { 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };

// a low data rate for the region, and a high one.
#if CFG_LMIC_US_like && defined(CFG_us915)
enum { DR_LOW = LORAWAN_DR0, DR_HIGH = LORAWAN_DR3 };
#elif defined(CFG_au915) || defined(CFG_as923)
enum { DR_LOW = LORAWAN_DR2, DR_HIGH = LORAWAN_DR5 };
#else
enum { DR_LOW = LORAWAN_DR0, DR_HIGH = LORAWAN_DR5 };
#endif

/****************************************************************************\
|
|	Recording uplinks and events
|
\****************************************************************************/

enum { MAX_TX = 8 };
typedef struct {
	u1_t	len;
	u1_t	frame[256];
} tx_t;

static tx_t s_tx[MAX_TX];
static int s_nTx;
static int s_nTxComplete;
static int s_txCompleteAfterTx;	// s_nTx when EV_TXCOMPLETE came
static int s_nRxComplete;
static int s_nTimeCb;
static int s_timeCbSuccess;
static int s_timeCbAfterTx;	// s_nTx when the time callback came
static int s_nOther;
static volatile int s_done;

static void onEvent_(void *pUserData, ev_t ev) {
	LMIC_UNREFERENCED_PARAMETER(pUserData);
	switch (ev) {
	case EV_TXSTART:
		if (s_nTx < MAX_TX) {
			s_tx[s_nTx].len = LMIC.dataLen;
			memcpy(s_tx[s_nTx].frame, LMIC.frame, LMIC.dataLen);
		}
		++s_nTx;
		break;
	case EV_TXCOMPLETE:
		++s_nTxComplete;
		s_txCompleteAfterTx = s_nTx;
		s_done = 1;
		break;
	case EV_RXCOMPLETE:
		++s_nRxComplete;
		break;
	case EV_RXSTART:
		break;
	default:
		++s_nOther;
		break;
	}
}

static void onNetworkTime(void *pUserData, int flagSuccess) {
	LMIC_UNREFERENCED_PARAMETER(pUserData);
	++s_nTimeCb;
	s_timeCbSuccess = flagSuccess;
	s_timeCbAfterTx = s_nTx;
}

static u1_t foptsLen(const tx_t *t) {
	return t->frame[OFF_DAT_FCT] & 0x0F;
}

// FRMPayload length (with no port, -1).
static int payloadLen(const tx_t *t) {
	return (int)t->len - (OFF_DAT_OPTS + foptsLen(t) + 4) - 1;
}

static int port(const tx_t *t) {
	return payloadLen(t) < 0 ? -1 : t->frame[OFF_DAT_OPTS + foptsLen(t)];
}

/****************************************************************************\
|
|	Setup
|
\****************************************************************************/

static void startSession(dr_t dr) {
	hal_stub_setTicks(1000u * TICKS);
	hal_stub_startLmic();
	LMIC_setSession(0x13, DEVADDR, s_nwkSKey, s_appSKey);
	LMIC_setAdrMode(0);
	LMIC_setLinkCheckMode(0);
	LMIC_setDrTxpow(dr, 14);
	LMIC_registerEventCb(onEvent_, NULL);
	hal_stub_setRadioAutoComplete(1);
	s_nTx = s_nTxComplete = s_nRxComplete = s_nTimeCb = s_nOther = 0;
	s_txCompleteAfterTx = s_timeCbAfterTx = -1;
	s_timeCbSuccess = 0;
	s_done = 0;
}

static void endSession(void) {
	hal_stub_setRadioAutoComplete(0);
	LMIC_reset();
	LMIC_registerEventCb(NULL, NULL);
}

// run until EV_TXCOMPLETE, then two more minutes (to see that nothing
// else goes out), or at most an hour.
static void runToCompletion(void) {
	hal_stub_runUntilStop((u4_t)os_getTime() + 3600u * TICKS, &s_done);
	hal_stub_runUntil((u4_t)os_getTime() + 120u * TICKS);
}

// put the MIC on a downlink of n bytes, FCntDown 0, with the NwkSKey.
static u1_t addDownlinkMic(u1_t *d, u1_t n) {
	memset(AESaux, 0, 16);
	AESaux[0] = 0x49;
	AESaux[5] = 1;
	os_wlsbf4(AESaux + 6, DEVADDR);
	os_wlsbf4(AESaux + 10, 0);
	AESaux[15] = n;
	memcpy(AESkey, s_nwkSKey, 16);
	os_wmsbf4(d + n, os_aes(AES_MIC, d, n));
	return n + 4;
}

// an application downlink on port 5 with two bytes, FCntDown 0.
static void injectAppDownlink(void) {
	u1_t d[1 + 4 + 1 + 2 + 1 + 2 + 4];
	u1_t n = 0;

	d[n++] = HDR_FTYPE_DADN | HDR_MAJOR_V1;
	os_wlsbf4(d + n, DEVADDR); n += 4;
	d[n++] = 0;			// FCtrl
	os_wlsbf2(d + n, 0); n += 2;	// FCntDown
	d[n++] = 5;			// FPort
	d[n++] = 0x12;
	d[n++] = 0x34;
	// encrypt FRMPayload with the AppSKey (dir = down, FCnt 0)
	memset(AESaux, 0, 16);
	AESaux[0] = AESaux[15] = 1;
	AESaux[5] = 1;
	os_wlsbf4(AESaux + 6, DEVADDR);
	os_wlsbf4(AESaux + 10, 0);
	memcpy(AESkey, s_appSKey, 16);
	os_aes(AES_CTR, d + n - 2, 2);
	n = addDownlinkMic(d, n);
	hal_stub_injectDownlink(d, n);
}

// a DeviceTimeAns downlink, FCntDown 0, MIC with the NwkSKey.
static void injectDeviceTimeAns(void) {
	u1_t d[1 + 4 + 1 + 2 + 6 + 4];
	u1_t n = 0;

	d[n++] = HDR_FTYPE_DADN | HDR_MAJOR_V1;
	os_wlsbf4(d + n, DEVADDR); n += 4;
	d[n++] = 6;			// FCtrl: FOptsLen 6
	os_wlsbf2(d + n, 0); n += 2;	// FCntDown
	d[n++] = MCMD_DeviceTimeAns;
	os_wlsbf4(d + n, 1300000000); n += 4;	// GPS seconds
	d[n++] = 0x80;			// half a second
	n = addDownlinkMic(d, n);
	hal_stub_injectDownlink(d, n);
}

/****************************************************************************\
|
|	Tests
|
\****************************************************************************/

static u1_t s_payload[MAX_LEN_PAYLOAD];

// the largest payload at dr: the frame is FHDR (7) + MHDR + port + MIC.
static u1_t maxPayload(dr_t dr) {
	return (u1_t)(LMICbandplan_maxFrameLen(dr) - (OFF_DAT_OPTS + 5));
}

// DeviceTimeReq pending, payload as large as the data rate allows: the
// DeviceTimeReq goes first, alone; the payload next; one EV_TXCOMPLETE.
static void testSplit(void) {
	startSession(DR_LOW);
	u1_t const n = maxPayload(DR_LOW);
	memset(s_payload, 0xA5, n);

	LMIC_requestNetworkTime(onNetworkTime, NULL);
	injectDeviceTimeAns();		// answer in the first receive window
	CHECK_EQ(LMIC_setTxData2_strict(APP_PORT, s_payload, n, 0), LMIC_ERROR_SUCCESS);
	runToCompletion();

	CHECK_EQ(s_nTx, 2);
	CHECK_EQ(foptsLen(&s_tx[0]), 1);
	CHECK_EQ(s_tx[0].frame[OFF_DAT_OPTS], MCMD_DeviceTimeReq);
	CHECK_EQ(port(&s_tx[0]), -1);
	CHECK_EQ(foptsLen(&s_tx[1]), 0);
	CHECK_EQ(port(&s_tx[1]), APP_PORT);
	CHECK_EQ(payloadLen(&s_tx[1]), n);
	CHECK(s_tx[1].len <= LMICbandplan_maxFrameLen(DR_LOW));

	// one completion, with the payload's uplink.
	CHECK_EQ(s_nTxComplete, 1);
	CHECK_EQ(s_txCompleteAfterTx, 2);
	CHECK_EQ(s_nOther, 0);

	// the network time came in the first frame's window.
	CHECK_EQ(s_nTimeCb, 1);
	CHECK_EQ(s_timeCbSuccess, 1);
	CHECK_EQ(s_timeCbAfterTx, 1);
	lmic_time_reference_t ref;
	CHECK(LMIC_getNetworkTimeReference(&ref));
	CHECK_EQ(ref.tNetwork, 1300000000);
	CHECK_EQ(LMIC.txMacOnly, 0);
	endSession();
}

// the same with one byte less: everything goes in one frame, as before.
static void testFits(void) {
	startSession(DR_LOW);
	u1_t const n = maxPayload(DR_LOW) - 1;
	memset(s_payload, 0x5A, n);

	LMIC_requestNetworkTime(onNetworkTime, NULL);
	CHECK_EQ(LMIC_setTxData2_strict(APP_PORT, s_payload, n, 0), LMIC_ERROR_SUCCESS);
	runToCompletion();

	CHECK_EQ(s_nTx, 1);
	CHECK_EQ(foptsLen(&s_tx[0]), 1);
	CHECK_EQ(s_tx[0].frame[OFF_DAT_OPTS], MCMD_DeviceTimeReq);
	CHECK_EQ(port(&s_tx[0]), APP_PORT);
	CHECK_EQ(payloadLen(&s_tx[0]), n);
	CHECK_EQ(s_nTxComplete, 1);
	CHECK_EQ(s_txCompleteAfterTx, 1);
	CHECK_EQ(s_nTimeCb, 1);		// no answer: reported as failed
	CHECK_EQ(s_timeCbSuccess, 0);
	endSession();
}

// more MAC data than FOpts holds goes as a port 0 payload, ahead of the
// application payload.
static void testPort0(void) {
	startSession(DR_HIGH);

	// 15 bytes of pending answers plus a DeviceTimeReq: 16 bytes.
	for (u1_t i = 0; i < LWAN_FCtrl_FOptsLen_MAX; ++i)
		LMIC.pendMacData[i] = MCMD_LinkCheckReq;
	LMIC.pendMacLen = LWAN_FCtrl_FOptsLen_MAX;
	LMIC.pendMacPiggyback = 1;
	LMIC_requestNetworkTime(onNetworkTime, NULL);

	memset(s_payload, 0x11, 10);
	CHECK_EQ(LMIC_setTxData2_strict(APP_PORT, s_payload, 10, 0), LMIC_ERROR_SUCCESS);
	runToCompletion();

	CHECK_EQ(s_nTx, 2);
	CHECK_EQ(foptsLen(&s_tx[0]), 0);
	CHECK_EQ(port(&s_tx[0]), 0);
	CHECK_EQ(payloadLen(&s_tx[0]), LWAN_FCtrl_FOptsLen_MAX + 1);
	CHECK_EQ(foptsLen(&s_tx[1]), 0);
	CHECK_EQ(port(&s_tx[1]), APP_PORT);
	CHECK_EQ(payloadLen(&s_tx[1]), 10);
	CHECK_EQ(s_nTxComplete, 1);
	CHECK_EQ(s_txCompleteAfterTx, 2);
	CHECK_EQ(LMIC.pendMacLen, 0);
	endSession();
}

// a payload on port 0 can't carry FOpts: the MAC data goes first.
static void testAppPort0(void) {
	startSession(DR_HIGH);
	LMIC_requestNetworkTime(onNetworkTime, NULL);
	memset(s_payload, 0x22, 4);
	CHECK_EQ(LMIC_setTxData2_strict(0, s_payload, 4, 0), LMIC_ERROR_SUCCESS);
	runToCompletion();

	CHECK_EQ(s_nTx, 2);
	CHECK_EQ(foptsLen(&s_tx[0]), 1);
	CHECK_EQ(port(&s_tx[0]), -1);
	CHECK_EQ(foptsLen(&s_tx[1]), 0);
	CHECK_EQ(port(&s_tx[1]), 0);
	CHECK_EQ(payloadLen(&s_tx[1]), 4);
	CHECK_EQ(s_nTxComplete, 1);
	endSession();
}

// a payload too large on its own fails as before, and the MAC data stays
// pending for the next uplink.
static void testPayloadTooLarge(void) {
	startSession(DR_LOW);
	u1_t const n = maxPayload(DR_LOW) + 1;
	memset(s_payload, 0x33, n);

	LMIC_requestNetworkTime(onNetworkTime, NULL);
	CHECK_EQ(LMIC_setTxData2_strict(APP_PORT, s_payload, n, 0), LMIC_ERROR_TX_NOT_FEASIBLE);
	CHECK_EQ(s_nTx, 0);
	CHECK_EQ(LMIC.txDeviceTimeReqState, lmic_RequestTimeState_tx);

	// the next uplink carries it.
	s_done = 0;
	CHECK_EQ(LMIC_setTxData2_strict(APP_PORT, s_payload, 1, 0), LMIC_ERROR_SUCCESS);
	runToCompletion();
	CHECK_EQ(s_nTx, 1);
	CHECK_EQ(foptsLen(&s_tx[0]), 1);
	CHECK_EQ(s_tx[0].frame[OFF_DAT_OPTS], MCMD_DeviceTimeReq);
	endSession();
}

// a confirmed uplink split the same way: the MAC-only frame is not
// confirmed, the payload is, and its retransmissions carry no new split.
static void testConfirmed(void) {
	startSession(DR_LOW);
	u1_t const n = maxPayload(DR_LOW);
	memset(s_payload, 0x44, n);

	LMIC_requestNetworkTime(onNetworkTime, NULL);
	CHECK_EQ(LMIC_setTxData2_strict(APP_PORT, s_payload, n, 1), LMIC_ERROR_SUCCESS);
	hal_stub_runUntilStop((u4_t)os_getTime() + 4u * 3600u * TICKS, &s_done);

	CHECK(s_nTx >= 3);	// MAC-only, then the payload and its retries
	CHECK_EQ(s_tx[0].frame[OFF_DAT_HDR] & HDR_FTYPE, HDR_FTYPE_DAUP);
	for (int i = 1; i < s_nTx && i < MAX_TX; ++i) {
		CHECK_EQ(s_tx[i].frame[OFF_DAT_HDR] & HDR_FTYPE, HDR_FTYPE_DCUP);
		CHECK_EQ(port(&s_tx[i]), APP_PORT);
	}
	CHECK_EQ(s_nTxComplete, 1);
	CHECK((LMIC.txrxFlags & TXRX_NACK) != 0);
	endSession();
}

// NbTrans 2: the MAC-only frame goes once, the payload twice.
static void testRepeat(void) {
	startSession(DR_LOW);
	LMIC.upRepeat = 2;
	u1_t const n = maxPayload(DR_LOW);
	memset(s_payload, 0x55, n);

	LMIC_requestNetworkTime(onNetworkTime, NULL);
	CHECK_EQ(LMIC_setTxData2_strict(APP_PORT, s_payload, n, 0), LMIC_ERROR_SUCCESS);
	runToCompletion();

	CHECK_EQ(s_nTx, 3);
	CHECK_EQ(port(&s_tx[0]), -1);
	CHECK_EQ(port(&s_tx[1]), APP_PORT);
	CHECK_EQ(port(&s_tx[2]), APP_PORT);
	// a repeat has the same FCnt
	CHECK_EQ(os_rlsbf2(s_tx[1].frame + OFF_DAT_SEQNO), os_rlsbf2(s_tx[2].frame + OFF_DAT_SEQNO));
	CHECK_EQ(os_rlsbf2(s_tx[0].frame + OFF_DAT_SEQNO) + 1, os_rlsbf2(s_tx[1].frame + OFF_DAT_SEQNO));
	CHECK_EQ(s_nTxComplete, 1);
	CHECK_EQ(s_txCompleteAfterTx, 3);
	endSession();
}

// an application downlink in the MAC-only frame's window is delivered at
// once (EV_RXCOMPLETE), and the payload still goes, with one EV_TXCOMPLETE.
static int s_nRxMessage;
static int s_rxPort;
static u1_t s_rxData[2];

static void onRxMessage(void *pUserData, u1_t port, const u1_t *pMessage, size_t nMessage) {
	LMIC_UNREFERENCED_PARAMETER(pUserData);
	++s_nRxMessage;
	s_rxPort = port;
	if (nMessage == 2)
		memcpy(s_rxData, pMessage, 2);
}

static void testDownlinkAfterMacOnly(void) {
	startSession(DR_LOW);
	s_nRxMessage = 0;
	LMIC_registerRxMessageCb(onRxMessage, NULL);
	u1_t const n = maxPayload(DR_LOW);
	memset(s_payload, 0x66, n);

	LMIC_requestNetworkTime(onNetworkTime, NULL);
	injectAppDownlink();
	CHECK_EQ(LMIC_setTxData2_strict(APP_PORT, s_payload, n, 0), LMIC_ERROR_SUCCESS);
	runToCompletion();

	CHECK_EQ(s_nTx, 2);
	CHECK_EQ(s_nRxComplete, 1);
	CHECK_EQ(s_nRxMessage, 1);
	CHECK_EQ(s_rxPort, 5);
	CHECK_EQ(s_rxData[0], 0x12);
	CHECK_EQ(s_rxData[1], 0x34);
	CHECK_EQ(port(&s_tx[1]), APP_PORT);
	CHECK_EQ(payloadLen(&s_tx[1]), n);
	CHECK_EQ(s_nTxComplete, 1);
	CHECK_EQ(s_txCompleteAfterTx, 2);
	LMIC_registerRxMessageCb(NULL, NULL);
	endSession();
}

int main(void) {
	testSplit();
	testFits();
	testPort0();
	testAppPort0();
	testPayloadTooLarge();
	testConfirmed();
	testRepeat();
	testDownlinkAfterMacOnly();

	if (s_failures == 0)
		printf("test_mac_split: all %d checks passed\n", s_checks);
	else
		printf("test_mac_split: %d of %d checks failed\n", s_failures, s_checks);
	return s_failures;
}
