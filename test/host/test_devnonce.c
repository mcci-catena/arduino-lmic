/*

Module:  test_devnonce.c

Function:
	Host tests for DevNonce: a random start chosen once after power-up,
	kept across LMIC_reset(), one more for each Join-Request (wrapping
	at 0xFFFF), advanced before the radio sends each Join-Request,
	carried by the V3 session state blob, and the stop once every value
	has been used; also LMIC_clearJoinState().

Copyright and License:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	October 2026

Description:
	Runs against the real core, compiled for the region given to make,
	with hal_stub.c in place of the Arduino HAL. The join tests run the
	LMIC's scheduler until the first Join-Request starts; the stub radio
	never finishes, so each test ends with LMIC_reset(). Exit status is
	the number of failed checks.

*/

#include "hal_stub.h"
#include "lmic.h"
#include "lmic_accessors.h"
#include "lmic_session_state.h"

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

#define TICKS	((s4_t) LMIC_OSTICKS_PER_SEC)

enum { OFF_JOIN_FLAGS = 216, OFF_DEVNONCE_START = 220, OFF_DEVNONCE_COUNT = 240 };

enum { MAX_NONCES = 64 };

/****************************************************************************\
|
|	Running the scheduler until the first Join-Request
|
\****************************************************************************/

static int s_nTxStart;
static int s_nJoinFailed;
static u2_t s_frameNonce;		// DevNonce in the frame at EV_TXSTART
static u4_t s_nextAtTxStart;		// LMIC_getNextDevNonce() at EV_TXSTART
static u1_t s_blobAtTxStart[LMIC_SESSION_STATE_SIZE];
static int s_nExhausted;		// EV_DEVNONCE_EXHAUSTED
static u2_t s_nonces[MAX_NONCES];	// DevNonce of each Join-Request

static void onEvent_(void *pUserData, ev_t ev) {
	LMIC_UNREFERENCED_PARAMETER(pUserData);

	if (ev == EV_TXSTART) {
		s_frameNonce = (u2_t)(LMIC.frame[OFF_JR_DEVNONCE] |
				      (LMIC.frame[OFF_JR_DEVNONCE + 1] << 8));
		if (s_nTxStart < MAX_NONCES)
			s_nonces[s_nTxStart] = s_frameNonce;
		++s_nTxStart;
		s_nextAtTxStart = LMIC_getNextDevNonce();
		// what an application does to keep DevNonce across a power
		// failure: save the blob here, before the radio transmits.
		CHECK_EQ(LMIC_saveSessionState(s_blobAtTxStart, sizeof(s_blobAtTxStart), NULL, 0),
			 LMIC_SESSION_STATE_OK);
	} else if (ev == EV_JOIN_FAILED) {
		++s_nJoinFailed;
	} else if (ev == EV_DEVNONCE_EXHAUSTED) {
		++s_nExhausted;
	}
}

// start a join and run the scheduler for up to 30 simulated seconds,
// or until the first Join-Request starts or the join fails.
static void joinUntilTxStart(void) {
	s_nTxStart = 0;
	s_nJoinFailed = 0;
	LMIC_registerEventCb(onEvent_, NULL);
	CHECK(LMIC_startJoining());

	u4_t now = (u4_t)os_getTime();
	u4_t const end = now + 30u * (u4_t)TICKS;
	while ((s4_t)(end - now) > 0 && s_nTxStart == 0 && s_nJoinFailed == 0) {
		os_runloop_once();
		now += (u4_t)TICKS / 10u;
		hal_stub_setTicks(now);
	}
}

// stop the pending join (the stub radio never completes the TX).
static void stopJoin(void) {
	os_clearCallback(&LMIC.osjob_defer);
	LMIC_reset();
	LMIC_registerEventCb(NULL, NULL);
}

/****************************************************************************\
|
|	Tests
|
\****************************************************************************/

// power-up: LMIC is static, so it starts zeroed with no DevNonce chosen.
static void powerUp(void) {
	LMIC.devNonceSet = 0;
	LMIC.devNonce = 0;
	LMIC_reset();
}

static void testRandomStartChosenOnce(void) {
	hal_stub_startLmic();
	CHECK_EQ(LMIC.devNonceSet, 1);
	u4_t const first = LMIC_getNextDevNonce();
	CHECK(first <= 0xFFFF);

	// LMIC_reset() keeps it.
	LMIC_reset();
	LMIC_reset();
	CHECK_EQ(LMIC_getNextDevNonce(), first);

	// a new power-up chooses again; with the stub's random source, a
	// few power-ups do not all give the same value.
	int nDifferent = 0;
	for (int i = 0; i < 4; ++i) {
		powerUp();
		CHECK_EQ(LMIC.devNonceSet, 1);
		if (LMIC_getNextDevNonce() != first)
			++nDifferent;
	}
	CHECK(nDifferent > 0);
}

static void testSetGetReset(void) {
	CHECK(LMIC_setNextDevNonce(100));
	CHECK_EQ(LMIC_getNextDevNonce(), 100);
	CHECK_EQ(LMIC_getDevNonce(), 100);

	LMIC_reset();
	CHECK_EQ(LMIC_getNextDevNonce(), 100);

	// set before the first reset: the reset keeps it, no random choice.
	LMIC.devNonceSet = 0;
	CHECK(LMIC_setNextDevNonce(0));
	LMIC_reset();
	CHECK_EQ(LMIC_getNextDevNonce(), 0);

	CHECK(LMIC_setNextDevNonce(0xFFFF));
	CHECK_EQ(LMIC_getNextDevNonce(), 0xFFFF);
	CHECK(! LMIC_setNextDevNonce(0x10000));
	CHECK_EQ(LMIC_getNextDevNonce(), 0xFFFF);
}

static void testJoinUsesAndAdvances(void) {
	hal_stub_setTicks(1000u * (u4_t)TICKS);
	LMIC_reset();
	CHECK(LMIC_setNextDevNonce(0x1233));
	joinUntilTxStart();

	CHECK_EQ(s_nTxStart, 1);
	CHECK_EQ(s_frameNonce, 0x1233);
	// by EV_TXSTART, the LMIC has moved on to the next value.
	CHECK_EQ(s_nextAtTxStart, 0x1234);
	CHECK_EQ(LMIC_getDevNonce(), 0x1234);

	// the blob saved at EV_TXSTART holds the next value, not the used one.
	// (start 0x1233, one Join-Request sent).
	CHECK_EQ(s_blobAtTxStart[OFF_DEVNONCE_START + 0], 0x33);
	CHECK_EQ(s_blobAtTxStart[OFF_DEVNONCE_START + 1], 0x12);
	CHECK_EQ(s_blobAtTxStart[OFF_DEVNONCE_COUNT + 0], 1);
	CHECK_EQ(s_blobAtTxStart[OFF_DEVNONCE_COUNT + 1], 0);
	CHECK_EQ(s_blobAtTxStart[OFF_DEVNONCE_COUNT + 2], 0);
	stopJoin();
}

static void testRestoreContinues(void) {
	u1_t blob[LMIC_SESSION_STATE_SIZE];

	hal_stub_setTicks(2000u * (u4_t)TICKS);
	LMIC_reset();
	CHECK(LMIC_setNextDevNonce(7));
	joinUntilTxStart();
	CHECK_EQ(s_frameNonce, 7);
	memcpy(blob, s_blobAtTxStart, sizeof(blob));
	stopJoin();

	// power fails during the TX. At power-up LMIC_reset() picks a random
	// start; the application then restores the blob it saved.
	powerUp();
	CHECK_EQ(LMIC_restoreSessionState(blob, sizeof(blob)), LMIC_SESSION_STATE_OK);
	CHECK_EQ(LMIC_getNextDevNonce(), 8);

	// the back-off came back too: the clock started at the first attempt.
	lmic_join_backoff_status_t st;
	LMIC_getJoinBackoffStatus(&st);
	CHECK_EQ(st.fActive, 1);

	joinUntilTxStart();
	CHECK_EQ(s_nTxStart, 1);
	CHECK_EQ(s_frameNonce, 8);
	stopJoin();

	// a later LMIC_reset() keeps the continued sequence.
	LMIC_reset();
	CHECK_EQ(LMIC_getNextDevNonce(), 9);
}

static void testWraps(void) {
	hal_stub_setTicks(3000u * (u4_t)TICKS);
	LMIC_reset();
	LMIC_clearJoinBackoff();
	CHECK(LMIC_setNextDevNonce(0xFFFF));
	joinUntilTxStart();

	// 0xFFFF goes out, and the next is 0: LoRaWAN 1.0.3 servers reject
	// only values they remember, so there is no stop at the top.
	CHECK_EQ(s_nTxStart, 1);
	CHECK_EQ(s_nJoinFailed, 0);
	CHECK_EQ(s_frameNonce, 0xFFFF);
	CHECK_EQ(s_nextAtTxStart, 0);
	stopJoin();

	CHECK_EQ(LMIC_getNextDevNonce(), 0);
	joinUntilTxStart();
	CHECK_EQ(s_frameNonce, 0);
	CHECK_EQ(LMIC_getNextDevNonce(), 1);
	stopJoin();
}

/****************************************************************************\
|
|	Through the real scheduler, with the simulated radio
|
\****************************************************************************/

// start a join with the simulated radio on, and run failed joins for up
// to maxHours, stopping early once nStop Join-Requests have gone out.
static void runFailedJoins(int maxHours, int nStop) {
	s_nTxStart = 0;
	s_nJoinFailed = 0;
	s_nExhausted = 0;
	LMIC_registerEventCb(onEvent_, NULL);
	hal_stub_setRadioAutoComplete(1);
	CHECK(LMIC_startJoining());
	for (int h = 0; h < maxHours && s_nTxStart < nStop; ++h)
		hal_stub_runUntil((u4_t)os_getTime() + 3600u * (u4_t)TICKS);
}

static void endFailedJoins(void) {
	hal_stub_setRadioAutoComplete(0);
	stopJoin();
}

// count Join-Requests in s_nonces that do not follow the one before.
static int nonceBreaks(void) {
	int n = 0;
	for (int i = 1; i < s_nTxStart && i < MAX_NONCES; ++i)
		if (s_nonces[i] != (u2_t)(s_nonces[i - 1] + 1))
			++n;
	return n;
}

static void testExhaustionStop(void) {
	u1_t blob[LMIC_SESSION_STATE_SIZE];
	u2_t const start = 0xABCD;
	u4_t const count = 65530;

	hal_stub_setTicks(10000u * (u4_t)TICKS);
	LMIC_reset();
	LMIC_clearJoinBackoff();

	// a saved state with 65530 of the 65536 DevNonces used.
	CHECK_EQ(LMIC_saveSessionState(blob, sizeof(blob), NULL, 0), LMIC_SESSION_STATE_OK);
	blob[OFF_DEVNONCE_START + 0] = (u1_t)start;
	blob[OFF_DEVNONCE_START + 1] = (u1_t)(start >> 8);
	blob[OFF_DEVNONCE_COUNT + 0] = (u1_t)count;
	blob[OFF_DEVNONCE_COUNT + 1] = (u1_t)(count >> 8);
	blob[OFF_DEVNONCE_COUNT + 2] = (u1_t)(count >> 16);
	blob[OFF_DEVNONCE_COUNT + 3] = 0;
	CHECK_EQ(LMIC_restoreSessionState(blob, sizeof(blob)), LMIC_SESSION_STATE_OK);
	CHECK_EQ(LMIC_getNextDevNonce(), (u2_t)(start + count));

	// a day of failed joins: exactly six more go out, then the stop.
	runFailedJoins(24, 1000);
	CHECK_EQ(s_nTxStart, 6);
	CHECK_EQ(s_nonces[0], (u2_t)(start + count));
	CHECK_EQ(s_nonces[5], (u2_t)(start - 1));	// the last unused value
	CHECK_EQ(nonceBreaks(), 0);
	CHECK_EQ(s_nExhausted, 1);
	CHECK_EQ(LMIC.opmode & OP_JOINING, 0);
	CHECK_EQ(LMIC.devNonceCount, 0x10000);

	// joining again stops at once, with the event again.
	s_nExhausted = 0;
	runFailedJoins(1, 1000);
	CHECK_EQ(s_nTxStart, 0);
	CHECK_EQ(s_nExhausted, 1);

	// LMIC_clearJoinState() clears the stop; joining restarts.
	LMIC_clearJoinState();
	CHECK_EQ(LMIC.devNonceCount, 0);
	runFailedJoins(1, 3);
	CHECK(s_nTxStart >= 3);
	CHECK_EQ(s_nExhausted, 0);
	CHECK_EQ(s_nonces[0], LMIC.devNonceStart);
	endFailedJoins();

	// so does LMIC_setNextDevNonce().
	LMIC.devNonceCount = 0x10000;
	CHECK(LMIC_setNextDevNonce(0x0100));
	runFailedJoins(1, 2);
	CHECK(s_nTxStart >= 2);
	CHECK_EQ(s_nonces[0], 0x0100);
	CHECK_EQ(s_nExhausted, 0);
	endFailedJoins();
}

static void testWrapThroughScheduler(void) {
	hal_stub_setTicks(20000u * (u4_t)TICKS);
	LMIC_reset();
	LMIC_clearJoinBackoff();
	CHECK(LMIC_setNextDevNonce(0xFFF0));

	// 0xFFF0 .. 0xFFFF, then 0x0000, 0x0001: through the wrap, no stop.
	runFailedJoins(48, 18);
	CHECK(s_nTxStart >= 18);
	CHECK_EQ(s_nonces[0], 0xFFF0);
	CHECK_EQ(s_nonces[15], 0xFFFF);
	CHECK_EQ(s_nonces[16], 0x0000);
	CHECK_EQ(s_nonces[17], 0x0001);
	CHECK_EQ(nonceBreaks(), 0);
	CHECK_EQ(s_nExhausted, 0);
	CHECK(LMIC.opmode & OP_JOINING);
	endFailedJoins();
}

static void testClearJoinState(void) {
	u1_t blob[LMIC_SESSION_STATE_SIZE];

	hal_stub_setTicks(30000u * (u4_t)TICKS);
	LMIC_reset();
	LMIC_clearJoinBackoff();

	// half an hour of failed joins: the back-off runs, DevNonces are used.
	runFailedJoins(1, 5);
	hal_stub_setRadioAutoComplete(0);
	lmic_join_backoff_status_t st;
	LMIC_getJoinBackoffStatus(&st);
	CHECK_EQ(st.fActive, 1);
	CHECK(LMIC.devNonceCount >= 5);
	u2_t const oldStart = LMIC.devNonceStart;
	stopJoin();
	LMIC_getJoinBackoffStatus(&st);
	CHECK_EQ(st.fActive, 0);	// LMIC_reset() clears the back-off ...
	CHECK(LMIC.devNonceCount >= 5);	// ... but not the DevNonce sequence

	// a session: LMIC_clearJoinState() must not touch it.
	LMICcore_joinBackoff_start(os_getTime());
	LMIC.devaddr = 0x26011234;
	LMIC.seqnoUp = 0x55;
	LMIC.seqnoDn = 0x66;

	LMIC_clearJoinState();
	LMIC_getJoinBackoffStatus(&st);
	CHECK_EQ(st.fActive, 0);
	CHECK_EQ(LMIC.devNonceCount, 0);
	CHECK(LMIC.devNonceStart != oldStart);
	CHECK_EQ(LMIC_getNextDevNonce(), LMIC.devNonceStart);
	CHECK_EQ(LMIC.devaddr, 0x26011234);
	CHECK_EQ(LMIC.seqnoUp, 0x55);
	CHECK_EQ(LMIC.seqnoDn, 0x66);

	// the saved V3 blob shows it: session yes, back-off no, count 0.
	CHECK_EQ(LMIC_saveSessionState(blob, sizeof(blob), NULL, 0), LMIC_SESSION_STATE_OK);
	CHECK_EQ(blob[OFF_JOIN_FLAGS], LMIC_SESSION_STATE_JOINFLAG_SESSION);
	CHECK_EQ(blob[OFF_DEVNONCE_START + 0], (u1_t)LMIC.devNonceStart);
	CHECK_EQ(blob[OFF_DEVNONCE_START + 1], (u1_t)(LMIC.devNonceStart >> 8));
	CHECK_EQ(blob[OFF_DEVNONCE_COUNT + 0], 0);
	CHECK_EQ(blob[OFF_DEVNONCE_COUNT + 1], 0);
	CHECK_EQ(blob[OFF_DEVNONCE_COUNT + 2], 0);

	LMIC_reset();
}

int main(void) {
	testRandomStartChosenOnce();
	testSetGetReset();
	testJoinUsesAndAdvances();
	testRestoreContinues();
	testWraps();
	testExhaustionStop();
	testWrapThroughScheduler();
	testClearJoinState();

	if (s_failures == 0)
		printf("test_devnonce: all %d checks passed\n", s_checks);
	else
		printf("test_devnonce: %d of %d checks failed\n", s_failures, s_checks);
	return s_failures;
}
