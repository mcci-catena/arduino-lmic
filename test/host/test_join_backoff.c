/*

Module:  test_join_backoff.c

Function:
	Host tests for the Join-Request retransmission back-off
	(lmic_join_backoff.c).

Copyright and License:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	October 2026

Description:
	Runs against the real core, compiled for the region given to make,
	with hal_stub.c in place of the Arduino HAL. Exit status is the
	number of failed checks.

	The unit tests call the back-off with chosen times: the phases and
	duty rates by time since T0, start, LMIC_reset() and Join-Accept,
	the off switch, the tick counter wrap, the stale-globalDutyAvail
	guard, and save and restore through the session state blob. The
	last test runs the real scheduler with the simulated radio and
	checks the spacing between Join-Requests. test_join_longrun.c
	checks the Table 56 limits over 72 hours.

*/

#include "hal_stub.h"
#include "lmic.h"
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

// the duty rate the back-off should give at s seconds after T0.
static u1_t expectedRate(u4_t s) {
	return s < 900 ? 0 : s < 3600 ? 9 : s < 39600 ? 11 : 14;
}

// ask for the duty rate at now, with the stub clock set there.
static u1_t rateAt(ostime_t now) {
	hal_stub_setTicks((u4_t)now);
	return LMICcore_joinBackoff_dutyRate(now);
}

/****************************************************************************\
|
|	Unit tests
|
\****************************************************************************/

static void testStartResetJoined(void) {
	ostime_t const t0 = 5000 * TICKS;

	hal_stub_setTicks(t0);
	LMIC_reset();
	CHECK_EQ(LMIC.joinBackoff.fActive, 0);
	CHECK_EQ(LMICcore_joinBackoff_dutyRate(t0), 0);

	LMICcore_joinBackoff_start(t0);
	CHECK_EQ(LMIC.joinBackoff.fActive, 1);
	CHECK_EQ(LMIC.joinBackoff.phase, LMIC_JOIN_BACKOFF_PHASE_15MIN);
	CHECK_EQ(LMIC.joinBackoff.phaseStart, t0);

	// a second start keeps T0.
	LMICcore_joinBackoff_start(t0 + 100 * TICKS);
	CHECK_EQ(LMIC.joinBackoff.phaseStart, t0);

	// LMIC_reset() clears it.
	LMIC_reset();
	CHECK_EQ(LMIC.joinBackoff.fActive, 0);

	// LMIC_startJoining() starts the clock if it is not running.
	CHECK(LMIC_startJoining());
	CHECK_EQ(LMIC.joinBackoff.fActive, 1);
	os_clearCallback(&LMIC.osjob_defer);
	LMIC_reset();

	// a Join-Accept ends it, and the duty rate goes back to 0.
	LMICcore_joinBackoff_start(t0);
	LMIC.globalDutyRate = 11;
	LMICcore_joinBackoff_joined();
	CHECK_EQ(LMIC.joinBackoff.fActive, 0);
	CHECK_EQ(LMIC.globalDutyRate, 0);

	// LMIC_clearJoinBackoff() forgets it.
	LMICcore_joinBackoff_start(t0);
	LMIC_clearJoinBackoff();
	CHECK_EQ(LMIC.joinBackoff.fActive, 0);
}

static void testPhases(void) {
	static const struct { u4_t s; u1_t phase; } k[] = {
		{ 0, LMIC_JOIN_BACKOFF_PHASE_15MIN },
		{ 899, LMIC_JOIN_BACKOFF_PHASE_15MIN },
		{ 900, LMIC_JOIN_BACKOFF_PHASE_HOUR1 },
		{ 3599, LMIC_JOIN_BACKOFF_PHASE_HOUR1 },
		{ 3600, LMIC_JOIN_BACKOFF_PHASE_HOURS1_6 },
		{ 21599, LMIC_JOIN_BACKOFF_PHASE_HOURS1_6 },
		{ 21600, LMIC_JOIN_BACKOFF_PHASE_HOURS6_11 },
		{ 39599, LMIC_JOIN_BACKOFF_PHASE_HOURS6_11 },
		{ 39600, LMIC_JOIN_BACKOFF_PHASE_AFTER },
		{ 50000, LMIC_JOIN_BACKOFF_PHASE_AFTER },
	};
	ostime_t const t0 = 1000 * TICKS;

	hal_stub_setTicks(t0);
	LMIC_reset();
	LMICcore_joinBackoff_start(t0);
	for (unsigned i = 0; i < sizeof(k) / sizeof(k[0]); ++i) {
		CHECK_EQ(rateAt(t0 + (ostime_t)k[i].s * TICKS), expectedRate(k[i].s));
		CHECK_EQ(LMIC.joinBackoff.phase, k[i].phase);
	}

	// after 11 hours, time no longer matters: even after a long gap.
	CHECK_EQ(rateAt((ostime_t)((u4_t)t0 + 50000u * (u4_t)TICKS + 0x7FFFFFFFu)), 14);
	CHECK_EQ(LMIC.joinBackoff.phase, LMIC_JOIN_BACKOFF_PHASE_AFTER);
}

static void testTickWrap(void) {
	// start just before the 32-bit tick counter wraps, then step an
	// hour at a time for 30 hours: past the 2^31-tick signed range at
	// every allowed tick rate.
	u4_t const t0 = 0xFFFFFFFFu - 10u * (u4_t)TICKS;

	hal_stub_setTicks(t0);
	LMIC_reset();
	LMICcore_joinBackoff_start((ostime_t)t0);
	for (u4_t h = 1; h <= 30; ++h) {
		u4_t const now = t0 + h * 3600u * (u4_t)TICKS;
		CHECK_EQ(rateAt((ostime_t)now), expectedRate(h * 3600));
	}
}

static void testDisabled(void) {
	ostime_t const t0 = 0;

	hal_stub_setTicks(t0);
	LMIC_reset();
	LMICcore_joinBackoff_start(t0);
	LMIC_setJoinBackoffEnabled(0);
	CHECK_EQ(rateAt(t0 + 7200 * TICKS), 0);
	CHECK_EQ(LMIC.joinBackoff.phase, LMIC_JOIN_BACKOFF_PHASE_HOURS1_6);	// time still kept
	LMIC_setJoinBackoffEnabled(1);
	CHECK_EQ(rateAt(t0 + 7300 * TICKS), 11);
}

static void testStaleAvail(void) {
	// up for a long time before the first join: LMIC_reset() left
	// globalDutyAvail at 0, which, more than 2^31 ticks ago, compares as
	// future. T0 starts it at now.
	ostime_t const late = (ostime_t)0xC0000000u;
	hal_stub_setTicks((u4_t)late);
	LMIC_reset();
	CHECK_EQ(LMIC.globalDutyAvail, 0);
	LMICcore_joinBackoff_start(late);
	CHECK_EQ(LMIC.globalDutyAvail, late);

	ostime_t const now = 20000 * TICKS;

	hal_stub_setTicks(now);
	LMIC_reset();
	LMICcore_joinBackoff_start(now);

	// a past globalDutyAvail is brought up to now, so it cannot age
	// into a false future after the tick counter wraps.
	LMIC.globalDutyAvail = now - 5 * 3600 * TICKS;
	(void) rateAt(now);
	CHECK_EQ(LMIC.globalDutyAvail, now);

	// a future one (the spacing still running) is left alone.
	LMIC.globalDutyAvail = now + 10 * TICKS;
	(void) rateAt(now + TICKS);
	CHECK_EQ(LMIC.globalDutyAvail, now + 10 * TICKS);
}

static void testSaveRestore(void) {
	ostime_t const t0 = 12345;
	u1_t blob[LMIC_SESSION_STATE_SIZE];
	lmic_session_state_info_t info;

	hal_stub_setTicks(t0);
	LMIC_reset();
	LMICcore_joinBackoff_start(t0);
	ostime_t const t = t0 + 7000 * TICKS;	// hours 1-6, 3400 s in
	CHECK_EQ(rateAt(t), 11);
	LMIC.globalDutyAvail = t + 100 * TICKS;	// spacing: 100 s to go

	// not joined: the blob carries the join state only.
	CHECK_EQ(LMIC_saveSessionState(blob, sizeof(blob), NULL, 0), LMIC_SESSION_STATE_OK);
	CHECK_EQ(LMIC_querySessionState(blob, sizeof(blob), &info), LMIC_SESSION_STATE_OK);
	CHECK_EQ(info.joinFlags, LMIC_SESSION_STATE_JOINFLAG_BACKOFF);

	// a reboot: power off for an unknown time, LMIC_reset(), restore.
	hal_stub_setTicks(77);
	LMIC_reset();
	CHECK_EQ(LMIC.joinBackoff.fActive, 0);
	ostime_t const tRestore = os_getTime();	// LMIC_reset() moves the stub clock
	CHECK_EQ(LMIC_restoreSessionState(blob, sizeof(blob)), LMIC_SESSION_STATE_OK);
	CHECK_EQ(LMIC.joinBackoff.fActive, 1);
	CHECK_EQ(LMIC.joinBackoff.phase, LMIC_JOIN_BACKOFF_PHASE_HOURS1_6);
	CHECK_EQ(tRestore - LMIC.joinBackoff.phaseStart, 3400 * TICKS);
	CHECK_EQ(LMIC.globalDutyAvail - tRestore, 100 * TICKS);

	// the clock runs on from the restore: hours 6-11 begin 14600 s
	// later (T0 + 21600 s; the save was at T0 + 7000 s).
	CHECK_EQ(rateAt(tRestore + 14599 * TICKS), 11);
	CHECK_EQ(LMIC.joinBackoff.phase, LMIC_JOIN_BACKOFF_PHASE_HOURS1_6);
	CHECK_EQ(rateAt(tRestore + 14600 * TICKS), 11);
	CHECK_EQ(LMIC.joinBackoff.phase, LMIC_JOIN_BACKOFF_PHASE_HOURS6_11);

	// an LMIC_startJoining() after the restore keeps the restored T0.
	ostime_t const ps = LMIC.joinBackoff.phaseStart;
	LMICcore_joinBackoff_start(tRestore + 14700 * TICKS);
	CHECK_EQ(LMIC.joinBackoff.phaseStart, ps);
	LMIC_reset();
}

/****************************************************************************\
|
|	Spacing, through the real scheduler
|
\****************************************************************************/

enum { MAX_TX = 64 };
static ostime_t s_txTime[MAX_TX];
static ostime_t s_txAirtime[MAX_TX];
static int s_nTx;

static void onEvent_(void *pUserData, ev_t ev) {
	LMIC_UNREFERENCED_PARAMETER(pUserData);
	if (ev == EV_TXSTART) {
		if (s_nTx < MAX_TX) {
			s_txTime[s_nTx] = os_getTime();
			s_txAirtime[s_nTx] = calcAirTime(LMIC.rps, LMIC.dataLen);
		}
		++s_nTx;
	}
}

static void testSpacing(void) {
	u4_t const start = 100000u * (u4_t)TICKS;

	hal_stub_setTicks(start);
	LMIC_reset();
	hal_stub_setRadioAutoComplete(1);
	LMIC_registerEventCb(onEvent_, NULL);
	s_nTx = 0;
	CHECK(LMIC_startJoining());

	// put the back-off two hours in: hours 1-6, rate 11.
	LMIC.joinBackoff.phase = LMIC_JOIN_BACKOFF_PHASE_HOURS1_6;
	LMIC.joinBackoff.phaseStart = (ostime_t)start - 3600 * TICKS;

	for (int h = 0; h < 3; ++h)
		hal_stub_runUntil((u4_t)os_getTime() + 3600u * (u4_t)TICKS);

	CHECK(s_nTx >= 2);
	CHECK(s_nTx < MAX_TX);
	// the engine starts a transmit up to its TX ramp-up (a few ms)
	// before the time it computed, so allow 20 ms.
	int nClose = 0;
	for (int i = 1; i < s_nTx && i < MAX_TX; ++i)
		if (s_txTime[i] - s_txTime[i - 1] < (s_txAirtime[i - 1] << 11) - ms2osticks(20))
			++nClose;
	CHECK_EQ(nClose, 0);
	printf("spacing at rate 11: %d Join-Requests in 3 h\n", s_nTx);

	hal_stub_setRadioAutoComplete(0);
	LMIC_reset();
	LMIC_registerEventCb(NULL, NULL);
}

int main(void) {
	hal_stub_startLmic();

	testStartResetJoined();
	testPhases();
	testTickWrap();
	testDisabled();
	testStaleAvail();
	testSaveRestore();
	testSpacing();

	if (s_failures == 0)
		printf("test_join_backoff: all %d checks passed\n", s_checks);
	else
		printf("test_join_backoff: %d of %d checks failed\n", s_failures, s_checks);
	return s_failures;
}
