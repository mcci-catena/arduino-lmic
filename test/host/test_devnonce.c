/*

Module:  test_devnonce.c

Function:
	Host tests for DevNonce: a random start chosen once after power-up,
	kept across LMIC_reset(), one more for each Join-Request (wrapping
	at 0xFFFF), and advanced before the radio sends each Join-Request.

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

/****************************************************************************\
|
|	Running the scheduler until the first Join-Request
|
\****************************************************************************/

static int s_nTxStart;
static int s_nJoinFailed;
static u2_t s_frameNonce;		// DevNonce in the frame at EV_TXSTART
static u4_t s_nextAtTxStart;		// LMIC_getNextDevNonce() at EV_TXSTART

static void onEvent_(void *pUserData, ev_t ev) {
	LMIC_UNREFERENCED_PARAMETER(pUserData);

	if (ev == EV_TXSTART) {
		++s_nTxStart;
		s_frameNonce = (u2_t)(LMIC.frame[OFF_JR_DEVNONCE] |
				      (LMIC.frame[OFF_JR_DEVNONCE + 1] << 8));
		s_nextAtTxStart = LMIC_getNextDevNonce();
	} else if (ev == EV_JOIN_FAILED) {
		++s_nJoinFailed;
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
	stopJoin();
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

int main(void) {
	testRandomStartChosenOnce();
	testSetGetReset();
	testJoinUsesAndAdvances();
	testWraps();

	if (s_failures == 0)
		printf("test_devnonce: all %d checks passed\n", s_checks);
	else
		printf("test_devnonce: %d of %d checks failed\n", s_failures, s_checks);
	return s_failures;
}
