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

	The unit tests call the back-off functions directly with chosen
	times. The long run models the LMIC's join timing for the region
	(the same delays as LMICeulike_nextJoinState() and
	LMICuslike_nextJoinState()) for 72 hours, puts each Join-Request
	through the back-off the way engineUpdate_inner() does, and checks
	the limits of TS001-1.0.4 section 7, Table 56.

*/

#include "hal_stub.h"
#include "lmic.h"
#include "lmic_bandplan.h"

#include <stdio.h>
#include <stdint.h>
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
	++s_checks;								\
	long long const a_ = (long long)(a), b_ = (long long)(b);		\
	if (a_ != b_) {								\
		++s_failures;							\
		printf("FAIL %s:%d: %s == %s: %lld != %lld\n",			\
			__FILE__, __LINE__, #a, #b, a_, b_);			\
	}									\
} while (0)

#define CHECK_LE(a, b) do {							\
	++s_checks;								\
	long long const a_ = (long long)(a), b_ = (long long)(b);		\
	if (!(a_ <= b_)) {							\
		++s_failures;							\
		printf("FAIL %s:%d: %s <= %s: %lld > %lld\n",			\
			__FILE__, __LINE__, #a, #b, a_, b_);			\
	}									\
} while (0)

#define TICKS	((s4_t) LMIC_OSTICKS_PER_SEC)

static ostime_t joinAirtime(dr_t dr) {
	return calcAirTime(setCr(updr2rps(dr), CR_4_5), LEN_JR);
}

static lmic_join_backoff_status_t status(void) {
	lmic_join_backoff_status_t s;
	LMIC_getJoinBackoffStatus(&s);
	return s;
}

// set LMIC.joinBackoff as a restore would.
static void setBackoff(ostime_t now, u4_t elapsedSec, u4_t windowStartSec, u4_t windowUsedMs) {
	memset(&LMIC.joinBackoff, 0, sizeof(LMIC.joinBackoff));
	LMIC.joinBackoff.fActive = 1;
	LMIC.joinBackoff.lastTick = now;
	LMIC.joinBackoff.elapsedSec = elapsedSec;
	LMIC.joinBackoff.windowStartSec = windowStartSec;
	LMIC.joinBackoff.windowUsedMs = windowUsedMs;
}

/****************************************************************************\
|
|	Unit tests
|
\****************************************************************************/

static void testTableChecks(void) {
	u1_t n;
	const lmic_join_backoff_step_t *p;

	CHECK(LMIC_setJoinBackoffTable(NULL, 0));
	p = LMIC_getJoinBackoffTable(&n);
	CHECK(p != NULL);
	CHECK(n >= 1);
	CHECK(LMIC_checkJoinBackoffTable(p, n));

	// the default table carries Table 56: 36 s in hour 1, 36 s in
	// hours 1-11, 8.7 s per 24 h after that.
	CHECK_EQ(p[0].startSec, 0);
	CHECK_EQ(p[0].windowSec, 3600);
	CHECK_EQ(p[0].budgetMs, 36000);
	CHECK_EQ(p[n - 1].startSec, 39600);
	CHECK_EQ(p[n - 1].windowSec, 86400);
	CHECK_EQ(p[n - 1].budgetMs, 8700);

	static const lmic_join_backoff_step_t good[] = {
		{ 0, 600, 10000, 0, 0 },
		{ 600, 0, 0, 100, 10 },
	};
	CHECK(LMIC_checkJoinBackoffTable(good, 2));

	static const lmic_join_backoff_step_t notAtZero[] = {
		{ 10, 600, 10000, 0, 0 },
	};
	static const lmic_join_backoff_step_t noFirstCap[] = {
		{ 0, 0, 0, 100, 0 },
	};
	static const lmic_join_backoff_step_t outOfOrder[] = {
		{ 0, 600, 10000, 0, 0 },
		{ 600, 0, 0, 100, 10 },
		{ 600, 0, 0, 200, 10 },
	};
	static const lmic_join_backoff_step_t smallBudget[] = {
		{ 0, 600, 1000, 0, 0 },
	};
	static const lmic_join_backoff_step_t shortWindow[] = {
		{ 0, 30, 10000, 0, 0 },
	};
	static const lmic_join_backoff_step_t bigJitter[] = {
		{ 0, 600, 10000, 10, 101 },
	};

	CHECK(! LMIC_checkJoinBackoffTable(good, 0));
	CHECK(! LMIC_checkJoinBackoffTable(NULL, 1));
	CHECK(! LMIC_checkJoinBackoffTable(good, LMIC_JOIN_BACKOFF_MAX_STEPS + 1));
	CHECK(! LMIC_checkJoinBackoffTable(notAtZero, 1));
	CHECK(! LMIC_checkJoinBackoffTable(noFirstCap, 1));
	CHECK(! LMIC_checkJoinBackoffTable(outOfOrder, 3));
	CHECK(! LMIC_checkJoinBackoffTable(smallBudget, 1));
	CHECK(! LMIC_checkJoinBackoffTable(shortWindow, 1));
	CHECK(! LMIC_checkJoinBackoffTable(bigJitter, 1));

	// a bad table is refused and the old one stays.
	CHECK(LMIC_setJoinBackoffTable(good, 2));
	CHECK(! LMIC_setJoinBackoffTable(outOfOrder, 3));
	CHECK(LMIC_getJoinBackoffTable(&n) == good);
	CHECK_EQ(n, 2);
	CHECK(LMIC_setJoinBackoffTable(NULL, 0));
	CHECK(LMIC_getJoinBackoffTable(&n) == p);
}

static void testInactive(void) {
	ostime_t const now = 1000 * TICKS;

	hal_stub_setTicks(now);
	LMIC_clearJoinBackoff();
	CHECK_EQ(status().fActive, 0);
	CHECK_EQ(LMICcore_joinBackoff_earliest(now, now + 5, joinAirtime(0)), now + 5);

	// a transmission while inactive is not counted.
	LMICcore_joinBackoff_noteTx(now, joinAirtime(0));
	CHECK_EQ(status().fActive, 0);
	CHECK_EQ(status().windowUsedMs, 0);
}

static void testStartResetJoined(void) {
	ostime_t const t0 = 5000 * TICKS;

	LMIC_clearJoinBackoff();
	hal_stub_setTicks(t0);
	LMICcore_joinBackoff_start(t0);
	CHECK_EQ(status().fActive, 1);

	hal_stub_setTicks(t0 + 100 * TICKS);
	CHECK_EQ(status().elapsedSec, 100);

	// a second start keeps the clock.
	LMICcore_joinBackoff_start(t0 + 100 * TICKS);
	hal_stub_setTicks(t0 + 200 * TICKS);
	CHECK_EQ(status().fActive, 1);
	CHECK_EQ(status().elapsedSec, 200);

	// LMIC_reset() clears it.
	LMIC_reset();
	CHECK_EQ(status().fActive, 0);
	LMICcore_joinBackoff_start(t0 + 200 * TICKS);

	// a Join-Accept ends it.
	LMICcore_joinBackoff_joined();
	CHECK_EQ(status().fActive, 0);
}

static void testFirstHourCap(void) {
	ostime_t const t0 = 7 * TICKS;
	ostime_t const a = ms2osticks(1000);
	ostime_t now = t0;
	int i;

	LMIC_clearJoinBackoff();
	hal_stub_setTicks(t0);
	LMICcore_joinBackoff_start(t0);

	// 1 s requests every 10 s: 35 fit under 36 s (strictly below).
	for (i = 0; i < 35; ++i) {
		now = t0 + i * 10 * TICKS;
		hal_stub_setTicks(now);
		CHECK_EQ(LMICcore_joinBackoff_earliest(now, now, a), now);
		LMICcore_joinBackoff_noteTx(now, a);
	}
	CHECK_LE(35000, status().windowUsedMs);
	CHECK_LE(status().windowUsedMs, 35000 + 35);	// ms rounding

	// the 36th waits for the next window (3600 s), plus 0-255 s jitter.
	now += 10 * TICKS;
	hal_stub_setTicks(now);
	ostime_t t = LMICcore_joinBackoff_earliest(now, now, a);
	CHECK(t - now > 0);
	CHECK_LE(t - now, 3600 * TICKS);	// never more than an hour ahead
	// ask again at that time, as engineUpdate does, until allowed.
	for (i = 0; i < 4 && t != now; ++i) {
		now = t;
		hal_stub_setTicks(now);
		t = LMICcore_joinBackoff_earliest(now, now, a);
	}
	CHECK_EQ(t, now);
	CHECK_LE(t0 + 3600 * TICKS, now);
	CHECK_LE(now, t0 + (3600 + 256) * TICKS);
}

static void testPacing(void) {
	ostime_t const now = 100 * TICKS;
	ostime_t const a = ms2osticks(400);

	// put the clock at 4000 s after T0: hours 1-11, pacing 1/1200.
	hal_stub_setTicks(now);
	setBackoff(now, 4000, 0, 0);
	CHECK_EQ(status().elapsedSec, 4000);
	CHECK_EQ(status().iStep, 2);

	CHECK_EQ(LMICcore_joinBackoff_earliest(now, now, a), now);
	LMICcore_joinBackoff_noteTx(now, a);

	// spacing 0.4 s * 1200 = 480 s, plus up to 25%.
	lmic_join_backoff_status_t const s = status();
	CHECK_LE(4000 + 480, s.nextOkSec);
	CHECK_LE(s.nextOkSec, 4000 + 1 + 480 + 120);

	hal_stub_setTicks(now + TICKS);
	ostime_t const t = LMICcore_joinBackoff_earliest(now + TICKS, now + TICKS, a);
	CHECK_LE(now + 480 * TICKS, t);
	CHECK_LE(t, now + (1 + 480 + 120) * TICKS);

	// a later "want" is kept as is.
	ostime_t const later = now + 900 * TICKS;
	CHECK_EQ(LMICcore_joinBackoff_earliest(now + TICKS, later, a), later);
}

static void testDisabled(void) {
	ostime_t const t0 = 0;
	ostime_t const a = ms2osticks(2000);
	ostime_t now = t0;
	int i;

	LMIC_clearJoinBackoff();
	hal_stub_setTicks(t0);
	LMICcore_joinBackoff_start(t0);
	for (i = 0; i < 17; ++i) {
		now = t0 + i * 10 * TICKS;
		hal_stub_setTicks(now);
		LMICcore_joinBackoff_noteTx(now, a);
	}
	now += 10 * TICKS;
	hal_stub_setTicks(now);
	CHECK(LMICcore_joinBackoff_earliest(now, now, a) != now);

	LMIC_setJoinBackoffEnabled(0);
	CHECK_EQ(status().fEnabled, 0);
	CHECK_EQ(LMICcore_joinBackoff_earliest(now, now, a), now);
	LMIC_setJoinBackoffEnabled(1);
	CHECK(LMICcore_joinBackoff_earliest(now, now, a) != now);
}

static void testTickWrap(void) {
	// start just before the 32-bit tick counter wraps, then step an
	// hour at a time for 30 hours: past the 2^31-tick signed range at
	// every allowed tick rate.
	u4_t const t0 = 0xFFFFFFFFu - 10u * (u4_t)TICKS;
	u4_t now = t0;
	int h;

	LMIC_clearJoinBackoff();
	hal_stub_setTicks(t0);
	LMICcore_joinBackoff_start((ostime_t)t0);
	for (h = 1; h <= 30; ++h) {
		now += 3600u * (u4_t)TICKS;
		hal_stub_setTicks(now);
		(void) LMICcore_joinBackoff_earliest((ostime_t)now, (ostime_t)now, joinAirtime(0));
		CHECK_EQ(status().elapsedSec, 3600 * h);
	}
}

/****************************************************************************\
|
|	Long run
|
\****************************************************************************/

// a small PRNG for the modeled join delays, so a run with the back-off
// turned off sees the same delays as a run with it on.
static u4_t s_seed;

static u4_t prng(void) {
	s_seed ^= s_seed << 13;
	s_seed ^= s_seed >> 17;
	s_seed ^= s_seed << 5;
	return s_seed;
}

// LMICcore_rndDelay(secSpan): under a second plus 0..secSpan-1 seconds.
static uint64_t rndDelayTicks(u1_t secSpan) {
	u4_t const r = prng();
	uint64_t d = r % (u4_t)TICKS;
	if (secSpan > 0)
		d += (uint64_t)((r >> 16) % secSpan) * (uint64_t)TICKS;
	return d;
}

enum { RUN_HOURS = 72, MAX_TX = 4096, RX2_END_MS = 6050 };

typedef struct {
	uint64_t	tick;		// sim time of start, ticks since sim start
	u4_t	ms;		// airtime
} tx_t;

static tx_t s_tx[MAX_TX];
static int s_nTx;

static uint64_t sec2t(u4_t s) {
	return (uint64_t)s * (uint64_t)TICKS;
}

// sum of airtime of requests starting in [a, b) seconds after T0.
static u4_t airtimeIn(uint64_t t0, u4_t a, u4_t b, int *pCount) {
	u4_t ms = 0;
	int n = 0;
	for (int i = 0; i < s_nTx; ++i) {
		if (s_tx[i].tick >= t0 + sec2t(a) && s_tx[i].tick < t0 + sec2t(b)) {
			ms += s_tx[i].ms;
			++n;
		}
	}
	if (pCount)
		*pCount = n;
	return ms;
}

// the most airtime in any 24-hour span starting at or after hour 11.
static u4_t worstSliding24h(uint64_t t0) {
	u4_t worst = 0;
	for (int i = 0; i < s_nTx; ++i) {
		if (s_tx[i].tick < t0 + sec2t(39600))
			continue;
		u4_t ms = 0;
		for (int j = i; j < s_nTx && s_tx[j].tick < s_tx[i].tick + sec2t(86400); ++j)
			ms += s_tx[j].ms;
		if (ms > worst)
			worst = ms;
	}
	return worst;
}

// run the modeled join sequence for RUN_HOURS (or until s_tx fills, or
// until stopSec), with the back-off on or off. The sim clock starts at
// startTick so that the run crosses the tick counter wrap.
static void runJoins(u4_t seed, bit_t fBackoff, u4_t stopSec, u4_t startTick) {
	uint64_t const t0 = startTick;
	uint64_t sim = t0;
	uint64_t want;
	uint64_t bandAvail = t0;
	u4_t const tEnd = stopSec;
#if CFG_LMIC_US_like
	dr_t dr = LMICuslike_getJoin125kHzDR();
	want = t0;
#else
	dr_t dr = LMICbandplan_getInitialDrJoin();
	int nThisDr = 0;
#endif

	s_seed = seed;
	s_nTx = 0;
#if ! CFG_LMIC_US_like
	want = t0 + rndDelayTicks(8);
#endif

	LMIC_clearJoinBackoff();
	LMIC_setJoinBackoffEnabled(fBackoff);
	LMICcore_joinBackoff_start((ostime_t)(u4_t)t0);

	while (sim < t0 + sec2t(tEnd) && s_nTx < MAX_TX) {
		ostime_t const a = joinAirtime(dr);

		// engineUpdate_inner(): wait for txbeg, then ask the back-off.
		if (want > sim)
			sim = want;
		hal_stub_setTicks((u4_t)sim);
		ostime_t const now = (ostime_t)(u4_t)sim;
		ostime_t const t = LMICcore_joinBackoff_earliest(now, (ostime_t)(u4_t)want, a);
		if ((s4_t)(t - now) > 0) {
			sim += (u4_t)(t - now);	// timer fires; ask again
			continue;
		}

		// transmit now.
		s_tx[s_nTx].tick = sim;
		s_tx[s_nTx].ms = (u4_t)osticks2ms(a);
		++s_nTx;
		LMICcore_joinBackoff_noteTx(now, a);

		// end of RX2, then LMICbandplan_nextJoinState().
		uint64_t const rx2end = sim + (uint64_t)a + (uint64_t)ms2osticks(RX2_END_MS);
#if CFG_LMIC_US_like
		if (dr == LMICuslike_getJoin125kHzDR())
			dr = LMICuslike_getFirst500kHzDR();
		else
			dr = LMICuslike_getJoin125kHzDR();
		want = rx2end + rndDelayTicks((u1_t)(16 >> dr));
#else
# if defined(CFG_eu868)
		bandAvail = sim + (uint64_t)a * 1000;	// BAND_MILLI, 0.1%
# elif defined(CFG_as923)
		bandAvail = sim + (uint64_t)a * 100;	// BAND_CENTI, 1%
# endif
		if (++nThisDr == 3) {
			nThisDr = 0;
# if ! defined(CFG_as923)
			if (dr > 0)
				--dr;
# endif
		}
		want = (rx2end > bandAvail ? rx2end : bandAvail)
			+ (uint64_t)ms2osticks(3000)
			+ rndDelayTicks((u1_t)(255 >> dr));
#endif
	}
	LMIC_setJoinBackoffEnabled(1);
	(void) bandAvail;
}

static void testLongRun(void) {
	static const u4_t seeds[] = { 1, 2, 3, 0x12345678, 0xCAFEF00D };
	u4_t worst1 = 0, worst10 = 0, worstDay = 0, worstSliding = 0;
	int n15 = 0, n1 = 0, n10 = 0, nDay = 0;

	for (unsigned k = 0; k < sizeof(seeds) / sizeof(seeds[0]); ++k) {
		u4_t const startTick = 0xFFFFFFFFu - 600u * (u4_t)TICKS;
		static tx_t base[MAX_TX];
		int nBase;

		// with the back-off off, record the first 15 minutes.
		runJoins(seeds[k], 0, 900, startTick);
		(void) airtimeIn(startTick, 0, 900, &nBase);
		memcpy(base, s_tx, sizeof(s_tx[0]) * (size_t)nBase);

		runJoins(seeds[k], 1, RUN_HOURS * 3600, startTick);
		CHECK(s_nTx < MAX_TX);

		uint64_t const t0 = startTick;
		int c15, c1, c10, cDay;

		// the first 15 minutes are the LMIC's usual join timing.
		(void) airtimeIn(t0, 0, 900, &c15);
		CHECK_EQ(c15, nBase);
		for (int i = 0; i < nBase && i < c15; ++i)
			CHECK_EQ(s_tx[i].tick, base[i].tick);

		// TS001-1.0.4 Table 56, row 1: < 36 s in the first hour.
		u4_t const ms1 = airtimeIn(t0, 0, 3600, &c1);
		CHECK(ms1 < 36000);
		// row 2: < 36 s from hour 1 to hour 11.
		u4_t const ms10 = airtimeIn(t0, 3600, 39600, &c10);
		CHECK(ms10 < 36000);
		// row 3: < 8.7 s in each day from hour 11 + N * 24.
		for (u4_t s = 39600; s < RUN_HOURS * 3600; s += 86400) {
			u4_t const ms = airtimeIn(t0, s, s + 86400, &cDay);
			CHECK(ms < 8700);
			if (ms > worstDay) { worstDay = ms; nDay = cDay; }
		}
		// stronger than Table 56 asks: any 24 hours after hour 11.
		u4_t const sliding = worstSliding24h(t0);
		CHECK(sliding < 8700);

		if (c15 > n15) n15 = c15;
		if (ms1 > worst1) { worst1 = ms1; n1 = c1; }
		if (ms10 > worst10) { worst10 = ms10; n10 = c10; }
		if (sliding > worstSliding) worstSliding = sliding;
	}

	printf("long run, worst of %u seeds: 0-15 min %d req; hour 1 %d req %u ms; "
	       "hours 1-11 %d req %u ms; day after 11 h %d req %u ms; "
	       "sliding 24 h %u ms\n",
	       (unsigned)(sizeof(seeds) / sizeof(seeds[0])),
	       n15, n1, (unsigned)worst1, n10, (unsigned)worst10,
	       nDay, (unsigned)worstDay, (unsigned)worstSliding);
}

int main(void) {
	hal_stub_startLmic();

	testTableChecks();
	testInactive();
	testStartResetJoined();
	testFirstHourCap();
	testPacing();
	testDisabled();
	testTickWrap();
	testLongRun();

	if (s_failures == 0)
		printf("test_join_backoff: all %d checks passed\n", s_checks);
	else
		printf("test_join_backoff: %d of %d checks failed\n", s_failures, s_checks);
	return s_failures;
}
