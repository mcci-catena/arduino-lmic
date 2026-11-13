/*

Module:  test_downlink.c

Function:
	Host unit tests for downlink checks in decodeFrame(): the size limit
	for the receive data rate (compliance test 5.9.d), LinkADRReq at
	the end of FOpts followed by FPort 3, and the compliance downlink
	counter for frames with FPort 0 or no FPort. Also checks the AS923
	DR2 uplink limit, next to the downlink limits.

Copyright and License:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	November 2026

Description:
	Runs against the real core, compiled for the region given to make,
	with hal_stub.c in place of the Arduino HAL. The test sets up an ABP
	session, arms the stub radio with a downlink built and MIC'd here,
	sends an uplink, and looks at what the LMIC kept. Exit status is the
	number of failed checks.

*/

#include "hal_stub.h"
#include "lmic.h"
#include "lmic_bandplan.h"
#include "lmic_aes_api.h"
#include "lmic_compliance.h"

#include <stdio.h>
#include <string.h>

static int s_failures;

#define CHECK(cond) do {							\
	if (!(cond)) {								\
		++s_failures;							\
		printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);		\
	}									\
} while (0)

#define CHECK_EQ(a, b) do {							\
	long long const a_ = (long long)(a), b_ = (long long)(b);		\
	if (a_ != b_) {								\
		++s_failures;							\
		printf("FAIL %s:%d: %s == %s: %lld != %lld\n",			\
			__FILE__, __LINE__, #a, #b, a_, b_);			\
	}									\
} while (0)

#define TICKS	((u4_t) LMIC_OSTICKS_PER_SEC)

enum {
	DEVADDR = 0x26011234,
	LEN_HDR = 8,		// MHDR, DevAddr, FCtrl, FCnt
	LEN_MIC = 4,
	SKIP_RX1 = 0,		// hal_stub_setRxFrame(): deliver in RX1
	SKIP_RX2 = 1,		// let RX1 time out, deliver in RX2
};

static const u1_t kNwkSKey[16] = {
	0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
	0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};
static const u1_t kAppSKey[16] = {
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
	0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
};

static volatile int s_txComplete;

static void onEvent_(void *pUserData, ev_t ev) {
	LMIC_UNREFERENCED_PARAMETER(pUserData);
	if (ev == EV_TXCOMPLETE)
		s_txComplete = 1;
}

// build an unconfirmed downlink with FCnt LMIC.seqnoDn: FOpts, then FPort
// and nPayload bytes if port >= 0. The payload isn't encrypted; the test
// only looks at its length. Returns the frame length.
static unsigned buildDownlink(u1_t *buf, const u1_t *pFOpts, unsigned nFOpts, int port, unsigned nPayload) {
	u4_t const fcnt = LMIC.seqnoDn;
	unsigned n;

	buf[0] = HDR_FTYPE_DADN | HDR_MAJOR_V1;
	os_wlsbf4(buf + 1, DEVADDR);
	buf[5] = (u1_t)nFOpts;
	os_wlsbf2(buf + 6, (u2_t)fcnt);
	memcpy(buf + LEN_HDR, pFOpts, nFOpts);
	n = LEN_HDR + nFOpts;
	if (port >= 0) {
		buf[n++] = (u1_t)port;
		for (unsigned i = 0; i < nPayload; ++i)
			buf[n++] = (u1_t)i;
	}

	// the MIC, as the network computes it (B0 block, direction down).
	memset(AESaux, 0, 16);
	AESaux[0] = 0x49;
	AESaux[5] = 1;
	os_wlsbf4(AESaux + 6, DEVADDR);
	os_wlsbf4(AESaux + 10, fcnt);
	AESaux[15] = (u1_t)n;
	memcpy(AESkey, kNwkSKey, 16);
	os_wmsbf4(buf + n, os_aes(AES_MIC, buf, (u2_t)n));
	return n + LEN_MIC;
}

static void startSession(void) {
	hal_stub_startLmic();
	hal_stub_setRadioAutoComplete(1);
	LMIC_registerEventCb(onEvent_, NULL);
	LMIC_setSession(0x13, DEVADDR, (xref2u1_t)kNwkSKey, (xref2u1_t)kAppSKey);
	LMIC_setLinkCheckMode(0);
	LMIC_setAdrMode(0);
#if CFG_as923
	// AS923 starts with the uplink dwell limit on; DR0 and DR1 can't carry a frame.
	LMIC_setDrTxpow(LORAWAN_DR2, KEEP_TXPOW);
#endif
}

// send one uplink and run until EV_TXCOMPLETE.
static void uplink(void) {
	static const u1_t data[1] = { 0x55 };

	s_txComplete = 0;
	CHECK_EQ(LMIC_setTxData2(1, (xref2u1_t)data, sizeof data, 0), 0);
	hal_stub_runUntilStop((u4_t)os_getTime() + 3600u * TICKS, &s_txComplete);
	CHECK(s_txComplete);
}

// deliver a data frame of nFrame bytes in the given window; report
// whether the LMIC accepted it.
static int tryFrame(unsigned nFrame, int nSkip) {
	u1_t buf[MAX_LEN_FRAME + 8];
	u4_t const seqnoDn = LMIC.seqnoDn;
	unsigned const nPayload = nFrame - LEN_HDR - 1 - LEN_MIC;

	CHECK_EQ(buildDownlink(buf, NULL, 0, 1, nPayload), nFrame);
	hal_stub_setRxFrame(buf, nFrame, nSkip);
	uplink();
	if (LMIC.seqnoDn == seqnoDn)
		return 0;
	CHECK_EQ(LMIC.dataLen, nPayload);
	return 1;
}

static void testSize(const char *pWindow, unsigned limit, int nSkip) {
	if (limit == 0 || limit + 1 > MAX_LEN_FRAME) {
		printf("  %s limit %u: no oversized frame fits the radio buffer; skipped\n", pWindow, limit);
		return;
	}
	printf("  %s limit %u\n", pWindow, limit);
	CHECK(tryFrame(limit, nSkip));
	CHECK(! tryFrame(limit + 1, nSkip));
	// a dropped frame leaves the next one alone.
	CHECK(tryFrame(limit, nSkip));
}

static void testSizes(void) {
	startSession();
	uplink();	// sets LMIC.dndr to the RX1 data rate

	testSize("RX2", LMICbandplan_maxDownlinkFrameLen(LMIC.dn2Dr), SKIP_RX2);
	testSize("RX1", LMICbandplan_maxDownlinkFrameLen(LMIC.dndr), SKIP_RX1);

#if CFG_us915 || CFG_au915
	// DR9 is downlink only; RP002 gives 137 bytes of MACPayload.
	LMIC.dn2Dr = LORAWAN_DR9;
	CHECK_EQ(LMICbandplan_maxDownlinkFrameLen(LORAWAN_DR9), 137 + 5);
	testSize("RX2 DR9", LMICbandplan_maxDownlinkFrameLen(LORAWAN_DR9), SKIP_RX2);
#endif
}

#if CFG_LMIC_EU_like
// did the LMIC queue a LinkADRAns with this status?
static int hasLinkAdrAns(u1_t status) {
	for (unsigned i = 0; i + 1 < LMIC.pendMacLen; ++i) {
		if (LMIC.pendMacData[i] == MCMD_LinkADRAns && LMIC.pendMacData[i + 1] == status)
			return 1;
	}
	return 0;
}

static void testLinkAdrBeforePort(int port) {
	u1_t const allAck = MCMD_LinkADRAns_PowerACK | MCMD_LinkADRAns_DataRateACK | MCMD_LinkADRAns_ChannelACK;
	// the current data rate, power index 0, the enabled channels, NbTrans 1.
	u1_t fopts[5] = { MCMD_LinkADRReq, 0, 0, 0, 0x01 };
	u1_t buf[32];
	unsigned n;

	// a new session each time: a queued LinkADRAns starts an uplink of its own.
	startSession();
	uplink();
	fopts[1] = (u1_t)(LMIC.datarate << MCMD_LinkADRReq_DR_SHIFT);
	os_wlsbf2(fopts + 2, (u2_t)LMIC.channelMap);
	n = buildDownlink(buf, fopts, sizeof fopts, port, 2);
	hal_stub_setRxFrame(buf, n, SKIP_RX1);
	uplink();
	printf("  LinkADRReq then FPort %d: pendMac", port);
	for (unsigned i = 0; i < LMIC.pendMacLen; ++i)
		printf(" %02X", LMIC.pendMacData[i]);
	printf("\n");
	CHECK(hasLinkAdrAns(allAck));
}
#endif // CFG_LMIC_EU_like

static int s_nRxMessage;

static void onRxMessage_(void *pUserData, uint8_t port, const uint8_t *pMessage, size_t nMessage) {
	LMIC_UNREFERENCED_PARAMETER(pUserData);
	++s_nRxMessage;
	(void) LMIC_complianceRxMessage(port, pMessage, nMessage);
}

// deliver a downlink in RX1 and check whether the compliance downlink
// counter counted it.
static void testCountFrame(const char *pName, int port, unsigned nPayload, int fCounted) {
	u1_t buf[32];
	unsigned const count = LMIC_Compliance.downlinkCount;
	int const nRxMessage = s_nRxMessage;
	unsigned const n = buildDownlink(buf, NULL, 0, port, nPayload);

	hal_stub_setRxFrame(buf, n, SKIP_RX1);
	uplink();
	printf("  compliance count, %s: %u -> %u\n", pName, count, (unsigned)LMIC_Compliance.downlinkCount);
	CHECK_EQ(s_nRxMessage, nRxMessage + 1);
	CHECK_EQ(LMIC_Compliance.downlinkCount, count + (fCounted ? 1 : 0));
}

// the tester counts every downlink except those with FPort 0
// (compliance tests 4.2.b and 5.10.a).
static void testComplianceCount(void) {
	startSession();
	LMIC_registerRxMessageCb(onRxMessage_, NULL);
	uplink();
	testCountFrame("no FPort", -1, 0, 1);
	testCountFrame("FPort 0", 0, 0, 0);
	testCountFrame("FPort 1", 1, 2, 1);
}

#if CFG_as923
// with the uplink dwell limit off, AS923 DR2 carries 123 bytes of
// MACPayload, 115 of them FRMPayload (RP002-1.0.3 Table 75).
static void testAs923UplinkDr2(void) {
	static u1_t data[116];

	startSession();
	LMIC.txParam = 0;	// no dwell limits, MaxEIRP 8 dBm
	LMIC_setDrTxpow(LORAWAN_DR2, KEEP_TXPOW);
	printf("  AS923 DR2 uplink, no dwell limit: limit %u\n", LMICbandplan_maxFrameLen(LORAWAN_DR2));
	CHECK_EQ(LMIC_setTxData2_strict(1, data, 116, 0), LMIC_ERROR_TX_NOT_FEASIBLE);
	s_txComplete = 0;
	hal_stub_runUntilStop((u4_t)os_getTime() + 60u * TICKS, &s_txComplete);
	s_txComplete = 0;
	CHECK_EQ(LMIC_setTxData2_strict(1, data, 115, 0), LMIC_ERROR_SUCCESS);
	hal_stub_runUntilStop((u4_t)os_getTime() + 3600u * TICKS, &s_txComplete);
	CHECK(s_txComplete);
}
#endif // CFG_as923

static void testLinkAdr(void) {
#if CFG_LMIC_EU_like
	testLinkAdrBeforePort(1);
	testLinkAdrBeforePort(MCMD_LinkADRReq);	// FPort 3, the command byte
#else
	printf("  LinkADRReq: EU-like regions only; skipped\n");
#endif
}

int main(void) {
	testSizes();
	testLinkAdr();
	testComplianceCount();
#if CFG_as923
	testAs923UplinkDr2();
#endif

	if (s_failures == 0)
		printf("test_downlink: all passed\n");
	return s_failures;
}
