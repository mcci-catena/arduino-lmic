/*

Module:  test_session_state.c

Function:
	Host unit tests for LMIC_saveSessionState(), LMIC_restoreSessionState()
	and LMIC_querySessionState().

Copyright and License:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	September 2026

Description:
	Runs against the real core, compiled for the region given to make,
	with hal_stub.c in place of the Arduino HAL. Exit status is the
	number of failed checks.

*/

#include "hal_stub.h"
#include "lmic.h"
#include "lmic_accessors.h"
#include "lmic_session_state.h"

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

enum { CLIENT_TAG = 0xBEEF };
enum { OFF_TAG = 0, OFF_SIZE = 1, OFF_REGION = 2, OFF_FCNT_UP = 4,
       OFF_CLIENT_TAG = 28, OFF_CHANNELS = 44 };

/****************************************************************************\
|
|	Region-specific parts, chosen once at file scope.
|
\****************************************************************************/

#if CFG_LMIC_EU_like

enum { EXPECTED_KIND = LMIC_SESSION_STATE_CHANNELS_CONFIGURABLE, EXPECTED_KIND_SIZE = 172 };
enum { TEST_CHANNEL = 5, TEST_FREQ = 867100000, TEST_GROUP = 1 };
enum { OFF_GROUP_TABLE = OFF_CHANNELS + 140, GROUP_ENTRY = 8 };

// make channel state that differs from the reset state.
static void perturbChannels(ostime_t now) {
	(void) LMIC_setupChannel(TEST_CHANNEL, TEST_FREQ, DR_RANGE_MAP(0, 5), -1);
	LMIC.bands[TEST_GROUP].lastchnl = 7;
	LMIC.bands[TEST_GROUP].avail = now + 2000;
	LMIC.bands[TEST_GROUP].txcap = 123;
	LMIC.bands[TEST_GROUP].txpow = 11;
}

static void checkChannelsRestored(ostime_t now, u1_t version) {
	lmic_channel_info_t ci;

	CHECK(LMIC_queryChannel(TEST_CHANNEL, &ci));
	CHECK_EQ(ci.uplinkFreq, TEST_FREQ);
	CHECK_EQ(ci.drMap, DR_RANGE_MAP(0, 5));
	CHECK(ci.enabled);
	CHECK_EQ(LMIC.bands[TEST_GROUP].lastchnl, 7);
	CHECK_EQ(LMIC.bands[TEST_GROUP].avail, now + 2000);
	CHECK_EQ(LMIC.bands[TEST_GROUP].txpow, 11);
	if (version >= LMIC_SESSION_STATE_TAG_V2)
		CHECK_EQ(LMIC.bands[TEST_GROUP].txcap, 123);
	else
		CHECK(LMIC.bands[TEST_GROUP].txcap != 123);	// V1: region default kept
}

// V1 wrote txpow into txDutyDenom; make the blob look like that.
static void corruptDutyDenomLikeV1(u1_t *pBlob) {
	u1_t * const q = pBlob + OFF_GROUP_TABLE + GROUP_ENTRY * TEST_GROUP;
	q[0] = 11;
	q[1] = 0;
}

#else /* fixed-channel regions */

enum { EXPECTED_KIND = LMIC_SESSION_STATE_CHANNELS_FIXED72, EXPECTED_KIND_SIZE = 22 };
enum { TEST_CHANNEL = 3 };

static void perturbChannels(ostime_t now) {
	LMIC_UNREFERENCED_PARAMETER(now);
	(void) LMIC_disableChannel(TEST_CHANNEL);
	(void) LMIC_disableChannel(64 + 1);
	LMIC.channelShuffleMap[0] = 0x00F0;
}

static void checkChannelsRestored(ostime_t now, u1_t version) {
	lmic_channel_info_t ci;

	LMIC_UNREFERENCED_PARAMETER(now);
	LMIC_UNREFERENCED_PARAMETER(version);
	CHECK(LMIC_queryChannel(TEST_CHANNEL, &ci));
	CHECK(! ci.enabled);
	CHECK(LMIC_queryChannel(TEST_CHANNEL + 1, &ci));
	CHECK(ci.enabled);
	CHECK(LMIC_queryChannel(64 + 1, &ci));
	CHECK(! ci.enabled);
	CHECK_EQ(LMIC.channelShuffleMap[0], 0x00F0);
	CHECK_EQ(LMIC.activeChannels125khz, 63);
	CHECK_EQ(LMIC.activeChannels500khz, 7);
}

static void corruptDutyDenomLikeV1(u1_t *pBlob) {
	LMIC_UNREFERENCED_PARAMETER(pBlob);
}

#endif

/****************************************************************************\
|
|	Common state used by the tests
|
\****************************************************************************/

static void perturbMacState(ostime_t now) {
	LMIC.seqnoUp = 0x01020304;
	LMIC.seqnoDn = 0x0A0B0C0D;
	LMIC.datarate = 3;
	LMIC.adrTxPow = 14;
	LMIC.upRepeat = 2;
	LMIC.globalDutyRate = 4;
	LMIC.rx1DrOffset = 1;
	LMIC.dn2Dr = 5;
	LMIC.rxDelay = 3;
	LMIC.adrAckReq = -5;
	LMIC.dn2Freq = 869525000;
	LMIC.globalDutyAvail = now + 1000;
	perturbChannels(now);
}

static void checkMacStateRestored(ostime_t now, u1_t version) {
	CHECK_EQ(LMIC_getFCntUp(), 0x01020304);
	CHECK_EQ(LMIC_getFCntDown(), 0x0A0B0C0D);
	CHECK_EQ(LMIC_getDataRate(), 3);
	CHECK_EQ(LMIC_getAdrTXPower(), 14);
	CHECK_EQ(LMIC.upRepeat, 2);
	CHECK_EQ(LMIC.globalDutyRate, 4);
	CHECK_EQ(LMIC.rx1DrOffset, 1);
	CHECK_EQ(LMIC_getRX2DataRate(), 5);
	CHECK_EQ(LMIC.rxDelay, 3);
	CHECK_EQ(LMIC.adrAckReq, -5);
	CHECK_EQ(LMIC_getRX2Frequency(), 869525000);
	CHECK_EQ(LMIC_getGlobalDutyAvail(), now + 1000);
	checkChannelsRestored(now, version);
}

/****************************************************************************\
|
|	Tests
|
\****************************************************************************/

static void test_size_header_and_query(void) {
	u1_t blob[LMIC_SESSION_STATE_SIZE];
	size_t nUsed = 0;
	lmic_session_state_info_t info;

	hal_stub_startLmic();
	CHECK_EQ(LMIC_getSessionStateSize(), LMIC_SESSION_STATE_SIZE);
	CHECK_EQ(LMIC_saveSessionState(blob, sizeof blob, &nUsed, CLIENT_TAG), LMIC_SESSION_STATE_OK);
	CHECK_EQ(nUsed, LMIC_SESSION_STATE_SIZE);
	CHECK_EQ(blob[OFF_TAG], LMIC_SESSION_STATE_TAG_V2);
	CHECK_EQ(blob[OFF_SIZE], LMIC_SESSION_STATE_SIZE);
	CHECK_EQ(blob[OFF_REGION], CFG_region);
	CHECK_EQ(blob[OFF_CHANNELS], EXPECTED_KIND);
	CHECK_EQ(blob[OFF_CHANNELS + 1], EXPECTED_KIND_SIZE);

	CHECK_EQ(LMIC_querySessionState(blob, sizeof blob, &info), LMIC_SESSION_STATE_OK);
	CHECK_EQ(info.version, LMIC_SESSION_STATE_TAG_V2);
	CHECK_EQ(info.region, CFG_region);
	CHECK_EQ(info.channelKind, EXPECTED_KIND);
	CHECK_EQ(info.size, LMIC_SESSION_STATE_SIZE);
	CHECK_EQ(info.clientTag, CLIENT_TAG);
}

static void test_little_endian(void) {
	u1_t blob[LMIC_SESSION_STATE_SIZE];

	hal_stub_startLmic();
	LMIC.seqnoUp = 0x01020304;
	CHECK_EQ(LMIC_saveSessionState(blob, sizeof blob, NULL, CLIENT_TAG), LMIC_SESSION_STATE_OK);
	CHECK_EQ(blob[OFF_FCNT_UP + 0], 0x04);
	CHECK_EQ(blob[OFF_FCNT_UP + 1], 0x03);
	CHECK_EQ(blob[OFF_FCNT_UP + 2], 0x02);
	CHECK_EQ(blob[OFF_FCNT_UP + 3], 0x01);
	CHECK_EQ(blob[OFF_CLIENT_TAG + 0], 0xEF);
	CHECK_EQ(blob[OFF_CLIENT_TAG + 1], 0xBE);
}

static void test_round_trip_v2(void) {
	u1_t blob[LMIC_SESSION_STATE_SIZE];
	ostime_t now;

	hal_stub_setTicks(100000);
	hal_stub_startLmic();
	now = os_getTime();
	perturbMacState(now);
	CHECK_EQ(LMIC_saveSessionState(blob, sizeof blob, NULL, CLIENT_TAG), LMIC_SESSION_STATE_OK);

	// a reset later, with the clock moved on: the deltas must be applied to the new now.
	hal_stub_advanceTicks(50000);
	hal_stub_startLmic();
	now = os_getTime();
	CHECK_EQ(LMIC_restoreSessionState(blob, sizeof blob), LMIC_SESSION_STATE_OK);
	checkMacStateRestored(now, LMIC_SESSION_STATE_TAG_V2);
}

static void test_round_trip_v1(void) {
	u1_t blob[LMIC_SESSION_STATE_SIZE];
	ostime_t now;

	hal_stub_setTicks(200000);
	hal_stub_startLmic();
	now = os_getTime();
	perturbMacState(now);
	CHECK_EQ(LMIC_saveSessionState(blob, sizeof blob, NULL, CLIENT_TAG), LMIC_SESSION_STATE_OK);

	// make it look like a V1 blob from Arduino-LoRaWAN.
	blob[OFF_TAG] = LMIC_SESSION_STATE_TAG_V1;
	corruptDutyDenomLikeV1(blob);

	hal_stub_startLmic();
	now = os_getTime();
	CHECK_EQ(LMIC_restoreSessionState(blob, sizeof blob), LMIC_SESSION_STATE_OK);
	checkMacStateRestored(now, LMIC_SESSION_STATE_TAG_V1);
}

static void test_rejections(void) {
	u1_t blob[LMIC_SESSION_STATE_SIZE];
	u1_t bad[LMIC_SESSION_STATE_SIZE];
	lmic_session_state_info_t info;
	u4_t const untouched = 0x11223344;

	hal_stub_startLmic();
	CHECK_EQ(LMIC_saveSessionState(blob, sizeof blob, NULL, CLIENT_TAG), LMIC_SESSION_STATE_OK);

	CHECK_EQ(LMIC_saveSessionState(bad, sizeof bad - 1, NULL, 0), LMIC_SESSION_STATE_BUFFER_TOO_SMALL);
	CHECK_EQ(LMIC_querySessionState(blob, sizeof blob - 1, &info), LMIC_SESSION_STATE_BUFFER_TOO_SMALL);

	memcpy(bad, blob, sizeof bad);
	bad[OFF_TAG] = LMIC_SESSION_STATE_TAG_NULL;
	CHECK_EQ(LMIC_querySessionState(bad, sizeof bad, &info), LMIC_SESSION_STATE_BAD_TAG);
	bad[OFF_TAG] = 3;
	CHECK_EQ(LMIC_querySessionState(bad, sizeof bad, &info), LMIC_SESSION_STATE_BAD_TAG);

	memcpy(bad, blob, sizeof bad);
	bad[OFF_SIZE] = LMIC_SESSION_STATE_SIZE - 1;
	CHECK_EQ(LMIC_querySessionState(bad, sizeof bad, &info), LMIC_SESSION_STATE_BAD_SIZE);

	memcpy(bad, blob, sizeof bad);
	bad[OFF_CHANNELS] = 7;
	CHECK_EQ(LMIC_querySessionState(bad, sizeof bad, &info), LMIC_SESSION_STATE_BAD_CHANNEL_VARIANT);

	memcpy(bad, blob, sizeof bad);
	bad[OFF_CHANNELS + 1] = EXPECTED_KIND_SIZE + 1;
	CHECK_EQ(LMIC_querySessionState(bad, sizeof bad, &info), LMIC_SESSION_STATE_BAD_CHANNEL_VARIANT);

	// the other kind of channel variant, with a plausible size, is rejected by restore.
	memcpy(bad, blob, sizeof bad);
	if ((int)EXPECTED_KIND == (int)LMIC_SESSION_STATE_CHANNELS_CONFIGURABLE) {
		bad[OFF_CHANNELS] = LMIC_SESSION_STATE_CHANNELS_FIXED72;
		bad[OFF_CHANNELS + 1] = 22;
	} else {
		bad[OFF_CHANNELS] = LMIC_SESSION_STATE_CHANNELS_CONFIGURABLE;
		bad[OFF_CHANNELS + 1] = 172;
	}
	CHECK_EQ(LMIC_querySessionState(bad, sizeof bad, &info), LMIC_SESSION_STATE_OK);
	LMIC.seqnoUp = untouched;
	CHECK_EQ(LMIC_restoreSessionState(bad, sizeof bad), LMIC_SESSION_STATE_BAD_CHANNEL_VARIANT);
	CHECK_EQ(LMIC.seqnoUp, untouched);

	// another region's blob is rejected without touching the state.
	memcpy(bad, blob, sizeof bad);
	bad[OFF_REGION] = (u1_t)(CFG_region + 1);
	LMIC.seqnoUp = untouched;
	CHECK_EQ(LMIC_restoreSessionState(bad, sizeof bad), LMIC_SESSION_STATE_REGION_MISMATCH);
	CHECK_EQ(LMIC.seqnoUp, untouched);
}

/****************************************************************************\
|
|	Main
|
\****************************************************************************/

int main(void) {
	printf("session state tests, region code %d\n", (int)CFG_region);
	test_size_header_and_query();
	test_little_endian();
	test_round_trip_v2();
	test_round_trip_v1();
	test_rejections();
	if (s_failures == 0)
		printf("ok\n");
	else
		printf("%d failure(s)\n", s_failures);
	return s_failures;
}
