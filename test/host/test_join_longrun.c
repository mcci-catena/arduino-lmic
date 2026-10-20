/*

Module:  test_join_longrun.c

Function:
	72 hours of failed joins through the real LMIC scheduler, checking
	the Join-Request retransmission back-off and DevNonce.

Copyright and License:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	October 2026

Description:
	The LMIC is compiled for the region given to make and runs on
	hal_stub.c with the simulated radio on: each transmit completes
	after its airtime and each receive window times out, so no
	Join-Accept ever arrives. hal_stub_runUntil() drives
	os_runloop_once_ex(), moving the clock straight to each scheduled
	job or radio event. Every Join-Request is recorded at EV_TXSTART.

	The checks are the limits of TS001-1.0.4 section 7, Table 56: under
	36 s of airtime in the first hour, under 36 s in hours 1-11, under
	8.7 s in each 24 hours from hour 11 (fixed windows), and, stronger
	than Table 56 asks, under 8.7 s in any 24 hours after hour 11. Each
	Join-Request must carry the DevNonce one above the one before.

	Exit status is the number of failed checks.

*/

#include "hal_stub.h"
#include "lmic.h"

#include <stdint.h>
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

enum { RUN_HOURS = 72, MAX_TX = 8192 };

typedef struct {
	uint64_t	tick;	// ticks since T0
	u4_t		ms;	// airtime
	u2_t		nonce;	// DevNonce in the frame
} tx_t;

static tx_t s_tx[MAX_TX];
static int s_nTx;
static int s_nJoinTxComplete;
static int s_nOther;		// events that should not happen here
static int s_nBadChannel;	// Join-Requests on a channel that does not exist
static volatile int s_stopOnJoinTxComplete;	// set at EV_JOIN_TXCOMPLETE
static volatile int s_stopOnTxStart;		// set at EV_TXSTART

#if CFG_LMIC_US_like
enum { N_TX_CHANNELS = 72 };
#else
enum { N_TX_CHANNELS = MAX_CHANNELS };
#endif

// the stub clock is 32 bits; keep a 64-bit count of ticks since T0.
static u4_t s_stepTick;
static uint64_t s_stepElapsed;

static uint64_t elapsedNow(void) {
	return s_stepElapsed + (u4_t)(os_getTime() - s_stepTick);
}

static void onEvent_(void *pUserData, ev_t ev) {
	LMIC_UNREFERENCED_PARAMETER(pUserData);

	switch (ev) {
	case EV_TXSTART:
		if (LMIC.txChnl >= N_TX_CHANNELS || LMIC.freq == 0)
			++s_nBadChannel;
		if (s_nTx < MAX_TX) {
			s_tx[s_nTx].tick = elapsedNow();
			s_tx[s_nTx].ms = (u4_t)osticks2ms(calcAirTime(LMIC.rps, LMIC.dataLen));
			s_tx[s_nTx].nonce = (u2_t)(LMIC.frame[OFF_JR_DEVNONCE] |
						   (LMIC.frame[OFF_JR_DEVNONCE + 1] << 8));
		}
		++s_nTx;
		s_stopOnTxStart = 1;
		break;
	case EV_JOIN_TXCOMPLETE:
		++s_nJoinTxComplete;
		s_stopOnJoinTxComplete = 1;
		break;
	case EV_JOINING:
	case EV_JOIN_FAILED:
	case EV_RXSTART:
		break;
	default:
		++s_nOther;
		break;
	}
}

static uint64_t sec2t(u4_t s) {
	return (uint64_t)s * TICKS;
}

static u4_t airtimeIn(u4_t a, u4_t b, int *pCount) {
	u4_t ms = 0;
	int n = 0;
	for (int i = 0; i < s_nTx && i < MAX_TX; ++i) {
		if (s_tx[i].tick >= sec2t(a) && s_tx[i].tick < sec2t(b)) {
			ms += s_tx[i].ms;
			++n;
		}
	}
	if (pCount)
		*pCount = n;
	return ms;
}

static u4_t worstSliding24h(void) {
	u4_t worst = 0;
	for (int i = 0; i < s_nTx && i < MAX_TX; ++i) {
		if (s_tx[i].tick < sec2t(39600))
			continue;
		u4_t ms = 0;
		for (int j = i; j < s_nTx && j < MAX_TX && s_tx[j].tick < s_tx[i].tick + sec2t(86400); ++j)
			ms += s_tx[j].ms;
		if (ms > worst)
			worst = ms;
	}
	return worst;
}

static void testLongRun(void) {
	// start ten minutes before the 32-bit tick counter wraps.
	u4_t const start = 0xFFFFFFFFu - 600u * TICKS;

	hal_stub_setTicks(start);
	hal_stub_startLmic();
	hal_stub_setRadioAutoComplete(1);
	LMIC_registerEventCb(onEvent_, NULL);

	s_nTx = s_nJoinTxComplete = s_nOther = s_nBadChannel = 0;
	s_stepTick = (u4_t)os_getTime();
	s_stepElapsed = 0;
	CHECK(LMIC_startJoining());	// T0

	for (int h = 0; h < RUN_HOURS; ++h) {
		hal_stub_runUntil(s_stepTick + 3600u * TICKS);
		s_stepElapsed += (u4_t)(os_getTime() - s_stepTick);
		s_stepTick = (u4_t)os_getTime();
	}

	CHECK(s_nTx < MAX_TX);
	CHECK_EQ(s_nOther, 0);
	CHECK_EQ(s_nBadChannel, 0);
	CHECK_EQ(LMIC.devaddr, 0);
	// every request but perhaps the last ran its receive windows out.
	CHECK(s_nJoinTxComplete >= s_nTx - 1);
	CHECK(s_nJoinTxComplete <= s_nTx);

	// DevNonce goes up by one per Join-Request (wrapping).
	int nBadNonce = 0;
	for (int i = 1; i < s_nTx && i < MAX_TX; ++i)
		if (s_tx[i].nonce != (u2_t)(s_tx[i - 1].nonce + 1))
			++nBadNonce;
	CHECK_EQ(nBadNonce, 0);

	// Table 56.
	int n15, n1, n10, nDay[3];
	u4_t const ms15 = airtimeIn(0, 900, &n15);
	u4_t const ms1 = airtimeIn(0, 3600, &n1);
	u4_t const ms10 = airtimeIn(3600, 39600, &n10);
	CHECK(ms1 < 36000);
	CHECK(ms10 < 36000);
	u4_t msDay[3];
	int iDay = 0;
	for (u4_t s = 39600; s < RUN_HOURS * 3600u && iDay < 3; s += 86400, ++iDay) {
		msDay[iDay] = airtimeIn(s, s + 86400, &nDay[iDay]);
		CHECK(msDay[iDay] < 8700);
		CHECK(nDay[iDay] > 0);		// still trying
	}
	u4_t const sliding = worstSliding24h();
	CHECK(sliding < 8700);

	printf("long run through the scheduler, %d h: %d requests; "
	       "0-15 min %d req %u ms; hour 1 %d req %u ms; hours 1-11 %d req %u ms; ",
	       RUN_HOURS, s_nTx, n15, (unsigned)ms15, n1, (unsigned)ms1, n10, (unsigned)ms10);
	for (int i = 0; i < iDay; ++i)
		printf("day %d after 11 h %d req %u ms%s; ", i, nDay[i], (unsigned)msDay[i],
		       (11 + 24 * (i + 1) > RUN_HOURS) ? " (partial)" : "");
	printf("worst sliding 24 h %u ms\n", (unsigned)sliding);

	hal_stub_setRadioAutoComplete(0);
	LMIC_reset();
	LMIC_registerEventCb(NULL, NULL);
}

// LMIC_startJoining() a second time without LMIC_reset() (as
// LMIC_unjoinAndRejoin() does): every Join-Request must still go out on
// a real channel.
static void testRejoinWithoutReset(void) {
	hal_stub_setTicks(1000u * TICKS);
	hal_stub_startLmic();
	hal_stub_setRadioAutoComplete(1);
	LMIC_registerEventCb(onEvent_, NULL);
	s_nTx = s_nJoinTxComplete = s_nOther = s_nBadChannel = 0;
	s_stepTick = (u4_t)os_getTime();
	s_stepElapsed = 0;

	CHECK(LMIC_startJoining());
	hal_stub_runUntil((u4_t)os_getTime() + 600u * TICKS);
	int const nFirst = s_nTx;
	CHECK(nFirst > 0);

	LMIC_unjoin();
	CHECK(LMIC_startJoining());
	hal_stub_runUntil((u4_t)os_getTime() + 600u * TICKS);
	CHECK(s_nTx > nFirst);
	CHECK_EQ(s_nBadChannel, 0);

	hal_stub_setRadioAutoComplete(0);
	LMIC_reset();
	LMIC_registerEventCb(NULL, NULL);
}

// stop a join after an unanswered Join-Request, wait, and start again:
// the back-off and the DevNonce sequence go on where they were.
static void testStopJoining(void) {
	ostime_t t0;

	hal_stub_setTicks(5000u * TICKS);
	hal_stub_startLmic();
	hal_stub_setRadioAutoComplete(1);
	LMIC_registerEventCb(onEvent_, NULL);
	s_nTx = s_nJoinTxComplete = s_nOther = s_nBadChannel = 0;
	s_stepTick = (u4_t)os_getTime();
	s_stepElapsed = 0;

	// no join running: nothing to do.
	CHECK_EQ(LMIC_stopJoining(), 1);

	// while the first Join-Request is on the air: refused, no change.
	s_stopOnTxStart = 0;
	t0 = os_getTime();
	CHECK(LMIC_startJoining());	// T0
	hal_stub_runUntilStop((u4_t)os_getTime() + 600u * TICKS, &s_stopOnTxStart);
	CHECK_EQ(s_nTx, 1);
	CHECK(LMIC.opmode & OP_TXRXPEND);
	u2_t const opmode = LMIC.opmode;
	CHECK_EQ(LMIC_stopJoining(), 0);
	CHECK_EQ(LMIC.opmode, opmode);

	// after its receive windows, from the main loop: stopped.
	s_stopOnJoinTxComplete = 0;
	hal_stub_runUntilStop((u4_t)os_getTime() + 600u * TICKS, &s_stopOnJoinTxComplete);
	CHECK_EQ(s_nJoinTxComplete, 1);
	CHECK_EQ(s_nTx, 1);
	u4_t const next = LMIC_getNextDevNonce();
	CHECK_EQ(LMIC.joinBackoff.fActive, 1);
	CHECK_EQ(LMIC_stopJoining(), 1);
	CHECK_EQ(LMIC.opmode & (OP_JOINING|OP_TXRXPEND), 0);
	CHECK_EQ(LMIC_stopJoining(), 1);	// again: nothing to do

	// two hours: nothing goes out, nothing changes but the clock.
	hal_stub_runUntil((u4_t)os_getTime() + 3600u * TICKS);
	hal_stub_runUntil((u4_t)os_getTime() + 3600u * TICKS);
	CHECK_EQ(s_nTx, 1);
	CHECK_EQ(LMIC_getNextDevNonce(), next);
	// same T0: two hours on, the clock is in hours 1-6, which began
	// at T0 + 1 h.
	LMICcore_joinBackoff_sync(os_getTime());
	CHECK_EQ(LMIC.joinBackoff.fActive, 1);
	CHECK_EQ(LMIC.joinBackoff.phase, LMIC_JOIN_BACKOFF_PHASE_HOURS1_6);
	CHECK_EQ(LMIC.joinBackoff.phaseStart, t0 + 3600 * (ostime_t)TICKS);
	CHECK_EQ(LMIC.devaddr, 0);

	// start again: the next Join-Request carries the next DevNonce, on
	// a real channel, and the back-off keeps its T0.
	s_stopOnTxStart = 0;
	CHECK(LMIC_startJoining());
	hal_stub_runUntilStop((u4_t)os_getTime() + 7200u * TICKS, &s_stopOnTxStart);
	CHECK_EQ(s_nTx, 2);
	CHECK_EQ(s_tx[1].nonce, (u2_t)next);
	CHECK_EQ(LMIC_getNextDevNonce(), (u2_t)(next + 1));
	CHECK_EQ(s_nBadChannel, 0);
	CHECK_EQ(LMIC.joinBackoff.phaseStart, t0 + 3600 * (ostime_t)TICKS);	// not restarted
	CHECK_EQ(LMIC.globalDutyRate, 11);	// hours 1-11 spacing

	// and joining goes on from there.
	hal_stub_runUntil((u4_t)os_getTime() + 1800u * TICKS);
	CHECK(s_nTx > 2);
	CHECK_EQ(s_nBadChannel, 0);
	int nBadNonce = 0;
	for (int i = 1; i < s_nTx && i < MAX_TX; ++i)
		if (s_tx[i].nonce != (u2_t)(s_tx[i - 1].nonce + 1))
			++nBadNonce;
	CHECK_EQ(nBadNonce, 0);

	hal_stub_setRadioAutoComplete(0);
	LMIC_reset();
	LMIC_registerEventCb(NULL, NULL);
}

int main(void) {
	testStopJoining();
	testRejoinWithoutReset();
	testLongRun();

	if (s_failures == 0)
		printf("test_join_longrun: all %d checks passed\n", s_checks);
	else
		printf("test_join_longrun: %d of %d checks failed\n", s_failures, s_checks);
	return s_failures;
}
