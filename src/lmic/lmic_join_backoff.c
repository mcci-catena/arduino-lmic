/*

Module:  lmic_join_backoff.c

Function:
	Retransmission back-off for Join-Requests.

Copyright notice and license info:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	October 2026

Description:
	The limits are those of TS001-1.0.4 (LoRaWAN L2 1.0.4) section 7,
	"Retransmissions Backoff", Table 56 "Transmit duty-cycle
	limitations", p. 49 (spec lines 1593-1599); LoRaWAN 1.0.3 section 7,
	p. 38 has the same numbers. RP002-1.0.5 refers each region's
	Join-Request duty cycle to that chapter. Table 56 applies together
	with the local regulation, "whichever is more constraining"; the
	regional duty-cycle code still applies on top of this.

	The windows are fixed, counted from T0, as Table 56 writes them:

	  [T0, T0+1 h)                          transmit time < 36 s
	  [T0+1 h, T0+11 h)                     transmit time < 36 s
	  [T0+11 h + N x 24 h, T0+35 h + N x 24 h)   transmit time < 8.7 s

	How the LMIC keeps to them: while joining, it sets the global duty
	rate (LMIC.globalDutyRate, the mechanism of DutyCycleReq) by the
	time since T0, so that each Join-Request start is at least airtime
	x 2^rate after the one before. If starts in a span of length W are
	at least A x 2^rate apart, the airtime in that span is at most
	W / 2^rate plus one request. With a Join-Request of at most 1.5 s
	(SF12, 125 kHz):

	  time since T0     rate   bound on airtime
	  0 to 15 min       0      the LMIC's usual join timing
	  15 min to 1 h     9      2700 s / 512 + 1.5 s = 6.8 s, so the hour
	                           stays under 36 s if the first 15 minutes
	                           use under 29 s (US915 about 18 s, AU915
	                           about 25 s, EU-like regions less)
	  1 h to 11 h       11     36000 s / 2048 + 1.5 s = 19.1 s < 36 s
	  after 11 h        14     86400 s / 16384 + 1.5 s = 6.8 s < 8.7 s,
	                           in any 24 hours, fixed or sliding

	The spacing also spreads requests over each window, so devices that
	lost the network together do not all spend a window's budget at its
	start. The randomization is the LMIC's own: the random delays of
	LMICbandplan_nextJoinState() set when each request would go out, and
	the duty rate only moves it later. The rates and the first 15
	minutes without spacing are MCCI choices, not spec text; the host
	tests check the Table 56 limits through the real scheduler.

	The state is a phase number and the os time the phase began. The
	hours 1-11 window is two 5-hour phases, so no phase is longer than
	the signed range of ostime_t (2^31 ticks is 9.2 h at the fastest
	allowed tick rate); a phase ends when the LMIC next looks after its
	end. While joining the LMIC looks at least every 6.8 h (the longest
	spacing, 1.5 s x 2^14); while a join is stopped (LMIC_stopJoining())
	it looks every hour. After 11 hours the phase stays the same, so time
	no longer matters.

*/

#include "lmic.h"

// length of each phase but the last, in os ticks.
static CONST_TABLE(ostime_t, phaseTicks)[] = {
	sec2osticks(15 * 60),		// 0: first 15 minutes
	sec2osticks(45 * 60),		// 1: rest of hour 1
	sec2osticks(5 * 3600),		// 2: hours 1-6
	sec2osticks(5 * 3600),		// 3: hours 6-11
};

// duty rate of each phase: starts at least airtime x 2^rate apart.
static CONST_TABLE(u1_t, phaseRate)[] = { 0, 9, 11, 11, 14 };

static bit_t s_fDisabled;

// move to the phase that holds now, and keep globalDutyAvail from going
// stale (a past value only matters while it compares as past).
static void advance(ostime_t now) {
	lmic_join_backoff_state_t * const s = &LMIC.joinBackoff;

	if ((s4_t)(LMIC.globalDutyAvail - now) < 0)
		LMIC.globalDutyAvail = now;

	while (s->phase < LMIC_JOIN_BACKOFF_PHASE_AFTER) {
		ostime_t const len = TABLE_GET_OSTIME(phaseTicks, s->phase);
		if ((s4_t)(now - s->phaseStart) < len)
			break;
		s->phaseStart += len;
		++s->phase;
	}
}

void LMICcore_joinBackoff_start(ostime_t now) {
	lmic_join_backoff_state_t * const s = &LMIC.joinBackoff;

	if (s->fActive)
		return;
	s->fActive = 1;
	s->phase = LMIC_JOIN_BACKOFF_PHASE_15MIN;
	s->phaseStart = now;
	// globalDutyAvail may be far in the past (LMIC_reset() leaves it 0,
	// and the device may have been up for days), so far that it compares
	// as future. Start it at now; advance() keeps it there.
	LMIC.globalDutyAvail = now;
}

void LMICcore_joinBackoff_sync(ostime_t now) {
	if (LMIC.joinBackoff.fActive)
		advance(now);
}

void LMICcore_joinBackoff_joined(void) {
	LMIC.joinBackoff.fActive = 0;
	LMIC.globalDutyRate = 0;
}

u1_t LMICcore_joinBackoff_dutyRate(ostime_t now) {
	lmic_join_backoff_state_t * const s = &LMIC.joinBackoff;

	if (! s->fActive)
		return 0;
	advance(now);
	if (s_fDisabled)
		return 0;
	return TABLE_GET_U1(phaseRate, s->phase);
}

void LMIC_setJoinBackoffEnabled(bit_t fEnable) {
	s_fDisabled = ! fEnable;
}

void LMIC_clearJoinBackoff(void) {
	LMIC.joinBackoff.fActive = 0;
}
