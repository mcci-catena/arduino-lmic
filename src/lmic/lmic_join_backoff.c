/*

Module:  lmic_join_backoff.c

Function:
	Retransmission back-off for Join-Requests.

Copyright notice and license info:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	October 2026

Description:
	See lmic_join_backoff.h for the model. In brief: the table in this
	file sets, for each part of the time since T0, a cap on Join-Request
	airtime per window and a pacing rule. engineUpdate_inner() asks
	LMICcore_joinBackoff_earliest() before each Join-Request and reports
	each one sent to LMICcore_joinBackoff_noteTx().

	Times inside this module are whole seconds since T0 in a u4_t, plus
	a remainder in ticks, so nothing depends on the range of ostime_t.
	The LMIC calls in at least once an hour while a join is pending
	(earliest() never asks the LMIC to wait longer than that), well
	inside the 2^31-tick signed range at any allowed tick rate.

	The state is LMIC.joinBackoff, so LMIC_reset() clears it and the
	session state blob saves and restores it (lmic_session_state.c).

	Everything here is deterministic between transmissions: the random
	parts (pacing jitter, window jitter) are drawn when a request is
	sent. So engineUpdate_inner() can ask again as often as it likes and
	get the same answer.

*/

#include "lmic.h"

/****************************************************************************\
|
|	The default table
|
|	The limits are those of TS001-1.0.4 (LoRaWAN L2 1.0.4) section 7,
|	"Retransmissions Backoff", Table 56 "Transmit duty-cycle
|	limitations", p. 49 (spec lines 1593-1599). LoRaWAN 1.0.3 section 7,
|	p. 38 (lines 1085-1091) has the same numbers. RP002-1.0.5 refers
|	each region's Join-Request duty cycle to that chapter (for example
|	EU863-870, section 3.4.2, lines 620-623, p. 33). Table 56 applies
|	together with the local regulation, "whichever is more constraining";
|	the regional duty-cycle code still applies on top of this table.
|
|	The windows are fixed, counted from T0, as Table 56 writes them:
|	[T0, T0+1 h), [T0+1 h, T0+11 h), and [T0+11 h + N x 24 h,
|	T0+35 h + N x 24 h). Table 56 says "Transmit time < ...", so the
|	budget is a strict upper bound.
|
|	The pacing (divisor) and the first 15 minutes without pacing are
|	MCCI choices, not spec text: they spread the budget over each
|	window, so devices that lost the network together do not all use
|	their budget at the start of each window, and they keep the join
|	timing the LMIC always had for the first 15 minutes. In US915 that
|	timing uses about 19 s of the first hour's 36 s in 15 minutes.
|
\****************************************************************************/

static const lmic_join_backoff_step_t kJoinBackoffDefault[] = {
	// Table 56 row 1: "Aggregated during the first hour following
	// power-up or reset, T0 < t < T0+1, Transmit time < 36 s per hour".
	// No pacing for the first 15 minutes.
	{ /* startSec */ 0, /* windowSec */ 3600, /* budgetMs */ 36000,
	  /* divisor */ 0, /* jitterPct */ 0 },

	// Same window and budget (Table 56 row 1). Pacing 1/200: at most
	// 13.5 s in the last 45 minutes.
	{ 900, 0, 0, 200, 25 },

	// Table 56 row 2: "Aggregated during the next 10 hours,
	// T0+1 < t < T0+11, Transmit time < 36 s per 10 h".
	// Pacing 1/1200: at most 30 s in 10 h.
	{ 3600, 36000, 36000, 1200, 25 },

	// Table 56 row 3: "After the first 11 hours, aggregated over 24 h,
	// T0+11 + N x 24 < t < T0+35 + N x 24, Transmit time < 8.7 s per
	// 24 h". Pacing 1/12000: at most 7.2 s in 24 h.
	{ 39600, 86400, 8700, 12000, 25 },
};

/****************************************************************************\
|
|	State
|
\****************************************************************************/

enum {
	// earliest() never asks the LMIC to wait longer than this.
	kMaxWaitSec = 3600,
	// limits checked by LMIC_checkJoinBackoffTable(). The budget must
	// hold the longest Join-Request (1.48 s at SF12/125 kHz).
	kMinWindowSec = 60,
	kMinBudgetMs = 2000,
};

// configuration; the state is LMIC.joinBackoff.
typedef struct lmic_join_backoff_config_s lmic_join_backoff_config_t;
struct lmic_join_backoff_config_s {
	const lmic_join_backoff_step_t *pTable;
	u1_t	nTable;
	u1_t	fDisabled;
};

static lmic_join_backoff_config_t s_backoff = {
	.pTable = kJoinBackoffDefault,
	.nTable = (u1_t)(sizeof(kJoinBackoffDefault) / sizeof(kJoinBackoffDefault[0])),
};

/****************************************************************************\
|
|	Table
|
\****************************************************************************/

bit_t LMIC_checkJoinBackoffTable(const lmic_join_backoff_step_t *pTable, u1_t nTable) {
	if (pTable == NULL || nTable == 0 || nTable > LMIC_JOIN_BACKOFF_MAX_STEPS)
		return 0;
	if (pTable[0].startSec != 0 || pTable[0].windowSec == 0)
		return 0;
	for (u1_t i = 0; i < nTable; ++i) {
		const lmic_join_backoff_step_t * const p = &pTable[i];

		if (i > 0 && p->startSec <= pTable[i - 1].startSec)
			return 0;
		if (p->windowSec != 0 &&
		    (p->windowSec < kMinWindowSec || p->budgetMs < kMinBudgetMs))
			return 0;
		if (p->jitterPct > 100)
			return 0;
	}
	return 1;
}

bit_t LMIC_setJoinBackoffTable(const lmic_join_backoff_step_t *pTable, u1_t nTable) {
	if (pTable == NULL) {
		pTable = kJoinBackoffDefault;
		nTable = (u1_t)(sizeof(kJoinBackoffDefault) / sizeof(kJoinBackoffDefault[0]));
	}
	if (! LMIC_checkJoinBackoffTable(pTable, nTable))
		return 0;
	s_backoff.pTable = pTable;
	s_backoff.nTable = nTable;
	return 1;
}

const lmic_join_backoff_step_t *LMIC_getJoinBackoffTable(u1_t *pnTable) {
	if (pnTable)
		*pnTable = s_backoff.nTable;
	return s_backoff.pTable;
}

// index of the row in effect at t.
static u1_t stepAt(u4_t t) {
	u1_t i = 0;
	for (u1_t j = 1; j < s_backoff.nTable; ++j)
		if (s_backoff.pTable[j].startSec <= t)
			i = j;
	return i;
}

// the cap window holding t: [*pStart, *pEnd), budget *pBudget.
static void windowAt(u4_t t, u4_t *pStart, u4_t *pEnd, u4_t *pBudget) {
	const lmic_join_backoff_step_t * const p = s_backoff.pTable;
	u1_t const n = s_backoff.nTable;
	u1_t cap = 0;

	for (u1_t j = 1; j < n; ++j)
		if (p[j].startSec <= t && p[j].windowSec != 0)
			cap = j;

	u4_t const w = p[cap].windowSec;
	u4_t const start = p[cap].startSec + ((t - p[cap].startSec) / w) * w;
	u4_t end = start + w;

	// a later row with its own cap ends this window early.
	for (u1_t j = cap + 1; j < n; ++j) {
		if (p[j].windowSec != 0) {
			if (p[j].startSec < end)
				end = p[j].startSec;
			break;
		}
	}

	*pStart = start;
	*pEnd = end;
	*pBudget = p[cap].budgetMs;
}

/****************************************************************************\
|
|	Time
|
\****************************************************************************/

// bring elapsedSec up to now.
static void advance(ostime_t now) {
	s4_t const d = (s4_t)(now - LMIC.joinBackoff.lastTick);

	if (d > 0) {
		u4_t const ticks = (u4_t)d + LMIC.joinBackoff.residueTicks;
		LMIC.joinBackoff.elapsedSec += ticks / (u4_t)LMIC_OSTICKS_PER_SEC;
		LMIC.joinBackoff.residueTicks = ticks % (u4_t)LMIC_OSTICKS_PER_SEC;
		LMIC.joinBackoff.lastTick = now;
	}
}

// a random number in [0, n).
static u4_t rndBelow(u4_t n) {
	u4_t const r = os_getRndU2();
	return r * (n >> 16) + ((r * (n & 0xFFFF)) >> 16);
}

static u4_t airtimeMs(ostime_t airtime) {
	// round up, so the account never runs short.
	return (u4_t)osticks2ms(airtime + (ostime_t)LMIC_OSTICKS_PER_SEC / 1000 - 1);
}

/****************************************************************************\
|
|	Core calls
|
\****************************************************************************/

void LMICcore_joinBackoff_start(ostime_t now) {
	if (LMIC.joinBackoff.fActive)
		return;
	LMIC.joinBackoff.fActive = 1;
	LMIC.joinBackoff.lastTick = now;
	LMIC.joinBackoff.residueTicks = 0;
	LMIC.joinBackoff.elapsedSec = 0;
	LMIC.joinBackoff.windowStartSec = 0;
	LMIC.joinBackoff.windowUsedMs = 0;
	LMIC.joinBackoff.nextOkSec = 0;
	LMIC.joinBackoff.capJitterSec = 0;
}

void LMICcore_joinBackoff_sync(ostime_t now) {
	if (LMIC.joinBackoff.fActive)
		advance(now);
}

void LMICcore_joinBackoff_joined(void) {
	LMIC.joinBackoff.fActive = 0;
}

ostime_t LMICcore_joinBackoff_earliest(ostime_t now, ostime_t want, ostime_t airtime) {
	if (! LMIC.joinBackoff.fActive || s_backoff.fDisabled)
		return want;

	advance(now);

	u4_t const nowSec = LMIC.joinBackoff.elapsedSec;
	u4_t const ms = airtimeMs(airtime);
	s4_t const dWant = (s4_t)(want - now);
	u4_t const wantSec = nowSec + (dWant > 0
		? ((u4_t)dWant + LMIC.joinBackoff.residueTicks + (u4_t)LMIC_OSTICKS_PER_SEC - 1)
			/ (u4_t)LMIC_OSTICKS_PER_SEC
		: 0);
	u4_t t = wantSec;

	// pacing
	if (LMIC.joinBackoff.nextOkSec > t)
		t = LMIC.joinBackoff.nextOkSec;

	// cap: move to a later window until the request fits. A fresh
	// window always fits (the table check makes the budget larger than
	// any Join-Request), so this runs at most twice.
	for (int i = 0; i < 4; ++i) {
		u4_t start, end, budget;

		windowAt(t, &start, &end, &budget);
		u4_t const used = (start == LMIC.joinBackoff.windowStartSec) ? LMIC.joinBackoff.windowUsedMs : 0;
		if (used + ms < budget || used == 0)
			break;
		t = end + LMIC.joinBackoff.capJitterSec;
	}

	if (t <= wantSec)
		return want;

	if (t - nowSec > kMaxWaitSec)
		t = nowSec + kMaxWaitSec;

	// the moment elapsedSec reaches t.
	ostime_t result = now + sec2osticks(t - nowSec) - (ostime_t)LMIC.joinBackoff.residueTicks;
	if ((s4_t)(result - want) < 0)
		result = want;
	return result;
}

void LMICcore_joinBackoff_noteTx(ostime_t txbeg, ostime_t airtime) {
	if (! LMIC.joinBackoff.fActive)
		return;

	advance(txbeg);

	u4_t const now = LMIC.joinBackoff.elapsedSec;
	u4_t const ms = airtimeMs(airtime);
	u4_t start, end, budget;

	windowAt(now, &start, &end, &budget);
	if (start != LMIC.joinBackoff.windowStartSec) {
		LMIC.joinBackoff.windowStartSec = start;
		LMIC.joinBackoff.windowUsedMs = 0;
	}
	LMIC.joinBackoff.windowUsedMs += ms;

	const lmic_join_backoff_step_t * const p = &s_backoff.pTable[stepAt(now)];
	if (p->divisor != 0) {
		// whole seconds, rounded up; +1 for the part second already
		// gone since elapsedSec ticked.
		u4_t const spacing = (ms * p->divisor + 999) / 1000;
		LMIC.joinBackoff.nextOkSec = now + 1 + spacing
			+ rndBelow(spacing * p->jitterPct / 100 + 1);
	} else {
		LMIC.joinBackoff.nextOkSec = 0;
	}

	// jitter for the start of the next window, should the cap stop us.
	LMIC.joinBackoff.capJitterSec = (u2_t)os_getRndU1();
}

/****************************************************************************\
|
|	Control and status
|
\****************************************************************************/

void LMIC_setJoinBackoffEnabled(bit_t fEnable) {
	s_backoff.fDisabled = ! fEnable;
}

void LMIC_clearJoinBackoff(void) {
	LMIC.joinBackoff.fActive = 0;
}

void LMIC_getJoinBackoffStatus(lmic_join_backoff_status_t *pStatus) {
	if (pStatus == NULL)
		return;

	if (LMIC.joinBackoff.fActive)
		advance(os_getTime());

	u4_t const now = LMIC.joinBackoff.elapsedSec;
	u4_t start, end, budget;

	windowAt(now, &start, &end, &budget);

	pStatus->fActive = LMIC.joinBackoff.fActive;
	pStatus->fEnabled = ! s_backoff.fDisabled;
	pStatus->iStep = stepAt(now);
	pStatus->elapsedSec = LMIC.joinBackoff.fActive ? now : 0;
	pStatus->windowStartSec = start;
	pStatus->windowEndSec = end;
	pStatus->windowUsedMs = (LMIC.joinBackoff.fActive && start == LMIC.joinBackoff.windowStartSec)
					? LMIC.joinBackoff.windowUsedMs : 0;
	pStatus->budgetMs = budget;
	pStatus->nextOkSec = LMIC.joinBackoff.nextOkSec;
}
