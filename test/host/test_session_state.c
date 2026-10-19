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
       OFF_CLIENT_TAG = 28, OFF_CHANNELS = 44,
       OFF_JOIN_FLAGS = 216, OFF_JOIN_RESERVED = 217, OFF_BACKOFF_CAP_JITTER = 218,
       OFF_DEVNONCE_START = 220, OFF_BACKOFF_ELAPSED = 224,
       OFF_BACKOFF_WINDOW_START = 228, OFF_BACKOFF_WINDOW_USED = 232,
       OFF_BACKOFF_NEXT_OK = 236, OFF_DEVNONCE_COUNT = 240 };

enum { TEST_DEVADDR = 0x26011234, TEST_DEVNONCE = 0x1234, TEST_DEVNONCE_COUNT = 0xF345 };

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

// a session (devaddr set), a DevNonce, and a back-off in progress.
static void perturbJoinState(ostime_t now) {
	LMIC.devaddr = TEST_DEVADDR;
	CHECK(LMIC_setNextDevNonce(TEST_DEVNONCE));
	LMIC.devNonceCount = TEST_DEVNONCE_COUNT;	// as if that many were sent
	LMIC.devNonce = (u2_t)(TEST_DEVNONCE + TEST_DEVNONCE_COUNT);
	LMIC.joinBackoff.fActive = 1;
	LMIC.joinBackoff.lastTick = now;
	LMIC.joinBackoff.elapsedSec = 5000;
	LMIC.joinBackoff.windowStartSec = 3600;
	LMIC.joinBackoff.windowUsedMs = 12345;
	LMIC.joinBackoff.nextOkSec = 5600;
	LMIC.joinBackoff.capJitterSec = 77;
}

static void checkJoinStateRestored(ostime_t now) {
	LMIC_UNREFERENCED_PARAMETER(now);
	CHECK_EQ(LMIC.devNonceStart, TEST_DEVNONCE);
	CHECK_EQ(LMIC.devNonceCount, TEST_DEVNONCE_COUNT);
	CHECK_EQ(LMIC_getNextDevNonce(), (u2_t)(TEST_DEVNONCE + TEST_DEVNONCE_COUNT));
	CHECK_EQ(LMIC.joinBackoff.fActive, 1);
	CHECK_EQ(LMIC.joinBackoff.elapsedSec, 5000);
	CHECK_EQ(LMIC.joinBackoff.windowStartSec, 3600);
	CHECK_EQ(LMIC.joinBackoff.windowUsedMs, 12345);
	CHECK_EQ(LMIC.joinBackoff.nextOkSec, 5600);
	CHECK_EQ(LMIC.joinBackoff.capJitterSec, 77);
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
	CHECK_EQ(LMIC_SESSION_STATE_SIZE, 244);
	CHECK_EQ(blob[OFF_TAG], LMIC_SESSION_STATE_TAG_V3);
	CHECK_EQ(blob[OFF_SIZE], LMIC_SESSION_STATE_SIZE);
	CHECK_EQ(blob[OFF_JOIN_FLAGS], 0);	// fresh LMIC: no session, no back-off
	CHECK_EQ(blob[OFF_JOIN_RESERVED], 0);
	CHECK_EQ(blob[OFF_REGION], CFG_region);
	CHECK_EQ(blob[OFF_CHANNELS], EXPECTED_KIND);
	CHECK_EQ(blob[OFF_CHANNELS + 1], EXPECTED_KIND_SIZE);

	CHECK_EQ(LMIC_querySessionState(blob, sizeof blob, &info), LMIC_SESSION_STATE_OK);
	CHECK_EQ(info.version, LMIC_SESSION_STATE_TAG_V3);
	CHECK_EQ(info.joinFlags, 0);
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

	perturbJoinState(os_getTime());
	CHECK_EQ(LMIC_saveSessionState(blob, sizeof blob, NULL, CLIENT_TAG), LMIC_SESSION_STATE_OK);
	CHECK_EQ(blob[OFF_JOIN_FLAGS], LMIC_SESSION_STATE_JOINFLAG_SESSION | LMIC_SESSION_STATE_JOINFLAG_BACKOFF);
	CHECK_EQ(blob[OFF_DEVNONCE_START + 0], 0x34);
	CHECK_EQ(blob[OFF_DEVNONCE_START + 1], 0x12);
	CHECK_EQ(blob[OFF_DEVNONCE_START + 2], 0);
	CHECK_EQ(blob[OFF_DEVNONCE_START + 3], 0);
	CHECK_EQ(blob[OFF_DEVNONCE_COUNT + 0], 0x45);
	CHECK_EQ(blob[OFF_DEVNONCE_COUNT + 1], 0xF3);
	CHECK_EQ(blob[OFF_DEVNONCE_COUNT + 2], 0);
	CHECK_EQ(blob[OFF_DEVNONCE_COUNT + 3], 0);
	CHECK_EQ(blob[OFF_BACKOFF_CAP_JITTER], 77);
	CHECK_EQ(blob[OFF_BACKOFF_ELAPSED + 0], 5000 & 0xFF);
	CHECK_EQ(blob[OFF_BACKOFF_ELAPSED + 1], 5000 >> 8);
	CHECK_EQ(blob[OFF_BACKOFF_WINDOW_START + 1], 3600 >> 8);
	CHECK_EQ(blob[OFF_BACKOFF_WINDOW_USED + 0], 12345 & 0xFF);
	CHECK_EQ(blob[OFF_BACKOFF_NEXT_OK + 1], 5600 >> 8);
}

static void test_round_trip_v3(void) {
	u1_t blob[LMIC_SESSION_STATE_SIZE];
	ostime_t now;

	hal_stub_setTicks(100000);
	hal_stub_startLmic();
	now = os_getTime();
	perturbMacState(now);
	perturbJoinState(now);
	CHECK_EQ(LMIC_saveSessionState(blob, sizeof blob, NULL, CLIENT_TAG), LMIC_SESSION_STATE_OK);

	// a reset later, with the clock moved on: the deltas must be applied to the new now.
	hal_stub_advanceTicks(50000);
	hal_stub_startLmic();
	CHECK(LMIC_setNextDevNonce(0));	// as at power-up
	now = os_getTime();
	CHECK_EQ(LMIC_restoreSessionState(blob, sizeof blob), LMIC_SESSION_STATE_OK);
	checkMacStateRestored(now, LMIC_SESSION_STATE_TAG_V3);
	checkJoinStateRestored(now);
}

static void test_unjoined_v3(void) {
	u1_t blob[LMIC_SESSION_STATE_SIZE];
	lmic_session_state_info_t info;
	ostime_t now;

	hal_stub_setTicks(300000);
	hal_stub_startLmic();
	now = os_getTime();
	perturbMacState(now);
	perturbJoinState(now);
	LMIC.devaddr = 0;	// joining, no session
	CHECK_EQ(LMIC_saveSessionState(blob, sizeof blob, NULL, CLIENT_TAG), LMIC_SESSION_STATE_OK);
	CHECK_EQ(LMIC_querySessionState(blob, sizeof blob, &info), LMIC_SESSION_STATE_OK);
	CHECK_EQ(info.joinFlags, LMIC_SESSION_STATE_JOINFLAG_BACKOFF);

	// restore: join state yes, MAC state no.
	hal_stub_startLmic();
	CHECK(LMIC_setNextDevNonce(0));
	LMIC.seqnoUp = 0x55;
	now = os_getTime();
	CHECK_EQ(LMIC_restoreSessionState(blob, sizeof blob), LMIC_SESSION_STATE_OK);
	CHECK_EQ(LMIC.seqnoUp, 0x55);
	CHECK(LMIC_getDataRate() != 3 || LMIC.rxDelay != 3);
	checkJoinStateRestored(now);

	// no back-off running: the restore leaves it cleared.
	hal_stub_startLmic();
	CHECK(LMIC_setNextDevNonce(9));
	CHECK_EQ(LMIC_saveSessionState(blob, sizeof blob, NULL, CLIENT_TAG), LMIC_SESSION_STATE_OK);
	CHECK_EQ(blob[OFF_JOIN_FLAGS], 0);
	LMIC.joinBackoff.fActive = 1;
	CHECK(LMIC_setNextDevNonce(0));
	CHECK_EQ(LMIC_restoreSessionState(blob, sizeof blob), LMIC_SESSION_STATE_OK);
	CHECK_EQ(LMIC.joinBackoff.fActive, 0);
	CHECK_EQ(LMIC_getNextDevNonce(), 9);
}

// make a V1 or V2 blob out of a V3 one: same first 216 bytes.
static void makeOldBlob(u1_t *pBlob, u1_t tag) {
	pBlob[OFF_TAG] = tag;
	pBlob[OFF_SIZE] = LMIC_SESSION_STATE_SIZE_V2;
	memset(pBlob + LMIC_SESSION_STATE_SIZE_V2, 0xA5,
	       LMIC_SESSION_STATE_SIZE_V3 - LMIC_SESSION_STATE_SIZE_V2);
}

static void test_round_trip_v2(void) {
	u1_t blob[LMIC_SESSION_STATE_SIZE];
	lmic_session_state_info_t info;
	ostime_t now;

	hal_stub_setTicks(100000);
	hal_stub_startLmic();
	now = os_getTime();
	perturbMacState(now);
	perturbJoinState(now);
	CHECK_EQ(LMIC_saveSessionState(blob, sizeof blob, NULL, CLIENT_TAG), LMIC_SESSION_STATE_OK);
	makeOldBlob(blob, LMIC_SESSION_STATE_TAG_V2);

	// a V2 blob is complete in 216 bytes.
	CHECK_EQ(LMIC_querySessionState(blob, LMIC_SESSION_STATE_SIZE_V2, &info), LMIC_SESSION_STATE_OK);
	CHECK_EQ(info.size, LMIC_SESSION_STATE_SIZE_V2);
	CHECK_EQ(info.joinFlags, LMIC_SESSION_STATE_JOINFLAG_SESSION);

	hal_stub_advanceTicks(50000);
	hal_stub_startLmic();
	now = os_getTime();
	CHECK_EQ(LMIC_restoreSessionState(blob, LMIC_SESSION_STATE_SIZE_V2), LMIC_SESSION_STATE_OK);
	checkMacStateRestored(now, LMIC_SESSION_STATE_TAG_V2);

	// no join state in V2: back-off cleared, DevNonce random (0..0xFFFF).
	CHECK_EQ(LMIC.joinBackoff.fActive, 0);
	CHECK(LMIC_getNextDevNonce() <= 0xFFFF);
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
	makeOldBlob(blob, LMIC_SESSION_STATE_TAG_V1);
	corruptDutyDenomLikeV1(blob);

	hal_stub_startLmic();
	now = os_getTime();
	CHECK_EQ(LMIC_restoreSessionState(blob, LMIC_SESSION_STATE_SIZE_V2), LMIC_SESSION_STATE_OK);
	checkMacStateRestored(now, LMIC_SESSION_STATE_TAG_V1);
}

static void test_rejections(void) {
	u1_t blob[LMIC_SESSION_STATE_SIZE];
	u1_t bad[LMIC_SESSION_STATE_SIZE];
	lmic_session_state_info_t info;
	u4_t const untouched = 0x11223344;

	hal_stub_startLmic();
	LMIC.devaddr = TEST_DEVADDR;	// a session, so restore reads the channels
	CHECK_EQ(LMIC_saveSessionState(blob, sizeof blob, NULL, CLIENT_TAG), LMIC_SESSION_STATE_OK);

	CHECK_EQ(LMIC_saveSessionState(bad, sizeof bad - 1, NULL, 0), LMIC_SESSION_STATE_BUFFER_TOO_SMALL);
	CHECK_EQ(LMIC_querySessionState(blob, sizeof blob - 1, &info), LMIC_SESSION_STATE_BUFFER_TOO_SMALL);

	memcpy(bad, blob, sizeof bad);
	bad[OFF_TAG] = LMIC_SESSION_STATE_TAG_NULL;
	CHECK_EQ(LMIC_querySessionState(bad, sizeof bad, &info), LMIC_SESSION_STATE_BAD_TAG);
	bad[OFF_TAG] = 4;
	CHECK_EQ(LMIC_querySessionState(bad, sizeof bad, &info), LMIC_SESSION_STATE_BAD_TAG);

	// V3 join state: unknown flag bits, reserved byte, impossible DevNonce.
	memcpy(bad, blob, sizeof bad);
	bad[OFF_JOIN_FLAGS] = 0x04;
	CHECK_EQ(LMIC_querySessionState(bad, sizeof bad, &info), LMIC_SESSION_STATE_BAD_JOIN_STATE);
	memcpy(bad, blob, sizeof bad);
	bad[OFF_JOIN_RESERVED] = 1;
	CHECK_EQ(LMIC_querySessionState(bad, sizeof bad, &info), LMIC_SESSION_STATE_BAD_JOIN_STATE);
	memcpy(bad, blob, sizeof bad);
	bad[OFF_DEVNONCE_START + 0] = 0;
	bad[OFF_DEVNONCE_START + 1] = 0;
	bad[OFF_DEVNONCE_START + 2] = 1;	// start 0x10000
	CHECK_EQ(LMIC_querySessionState(bad, sizeof bad, &info), LMIC_SESSION_STATE_BAD_JOIN_STATE);
	bad[OFF_DEVNONCE_START + 0] = 0xFF;
	bad[OFF_DEVNONCE_START + 1] = 0xFF;
	bad[OFF_DEVNONCE_START + 2] = 0;	// start 0xFFFF
	CHECK_EQ(LMIC_querySessionState(bad, sizeof bad, &info), LMIC_SESSION_STATE_OK);
	memset(bad + OFF_DEVNONCE_COUNT, 0, 4);
	bad[OFF_DEVNONCE_COUNT + 2] = 1;	// count 0x10000: all used, allowed
	CHECK_EQ(LMIC_querySessionState(bad, sizeof bad, &info), LMIC_SESSION_STATE_OK);
	bad[OFF_DEVNONCE_COUNT + 0] = 1;	// count 0x10001
	CHECK_EQ(LMIC_querySessionState(bad, sizeof bad, &info), LMIC_SESSION_STATE_BAD_JOIN_STATE);

	// a V2 blob shorter than 216, a V3 blob shorter than 244.
	memcpy(bad, blob, sizeof bad);
	makeOldBlob(bad, LMIC_SESSION_STATE_TAG_V2);
	CHECK_EQ(LMIC_querySessionState(bad, LMIC_SESSION_STATE_SIZE_V2 - 1, &info), LMIC_SESSION_STATE_BUFFER_TOO_SMALL);
	CHECK_EQ(LMIC_querySessionState(blob, LMIC_SESSION_STATE_SIZE_V3 - 1, &info), LMIC_SESSION_STATE_BUFFER_TOO_SMALL);
	// a V3 tag with the V2 size, and the reverse.
	memcpy(bad, blob, sizeof bad);
	bad[OFF_SIZE] = LMIC_SESSION_STATE_SIZE_V2;
	CHECK_EQ(LMIC_querySessionState(bad, sizeof bad, &info), LMIC_SESSION_STATE_BAD_SIZE);
	memcpy(bad, blob, sizeof bad);
	bad[OFF_TAG] = LMIC_SESSION_STATE_TAG_V2;
	CHECK_EQ(LMIC_querySessionState(bad, sizeof bad, &info), LMIC_SESSION_STATE_BAD_SIZE);

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
	test_round_trip_v3();
	test_unjoined_v3();
	test_round_trip_v2();
	test_round_trip_v1();
	test_rejections();
	if (s_failures == 0)
		printf("ok\n");
	else
		printf("%d failure(s)\n", s_failures);
	return s_failures;
}
