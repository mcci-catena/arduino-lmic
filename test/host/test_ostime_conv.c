/*

Module:  test_ostime_conv.c

Function:
	Host check of the tick conversion macros at the build's tick rate.

Copyright and License:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	September 2026

Description:
	The Makefile builds this once per tick rate (LMIC_OSTICKS_PER_SEC,
	via the Host ostime driver). It checks identities, rounding direction,
	round trips, monotonicity, sign and overflow limits, and prints the
	tick value of every wait in the SX127x receive path so the rates can
	be compared. Exit status is the number of failed checks.

*/

#include "oslmic.h"

#include <stdio.h>
#include <stdint.h>

static int s_failures;

#define CHECK(cond) do { if (!(cond)) { ++s_failures; printf("FAIL %d: %s\n", __LINE__, #cond); } } while (0)
#define SHOW(expr) printf("  %-44s = %ld ticks = %ld us\n", #expr, (long)(expr), (long)osticks2us(expr))

int main(void) {
	long const rate = LMIC_OSTICKS_PER_SEC;
	printf("LMIC_OSTICKS_PER_SEC = %ld (one tick = %.3f us)\n", rate, 1e6 / rate);

	/* identities */
	CHECK(sec2osticks(1) == rate);
	CHECK(ms2osticks(1000) == rate);
	CHECK(us2osticks(1000000) == rate);
	CHECK(osticks2ms(rate) == 1000);
	CHECK(osticks2us(rate) == 1000000);
	CHECK(us2osticks(0) == 0 && ms2osticks(0) == 0 && sec2osticks(0) == 0);

	/* rounding direction: floor, ceil, round */
	CHECK(us2osticks(1) == 0);
	CHECK(us2osticksCeil(1) == 1);
	CHECK(us2osticksRound(1) == 0);
	CHECK(ms2osticksCeil(1) >= ms2osticks(1));
	CHECK(us2osticksCeil(43) >= us2osticks(43));
	CHECK(ms2osticksRound(1) >= ms2osticks(1) && ms2osticksRound(1) <= ms2osticksCeil(1));

	/* round trips: converting back never gains, and loses at most one tick.
	   The tick loss is real when a tick is not a whole number of
	   microseconds (32768: 30.518 us); nothing in the core does this. */
	for (long us = 1; us <= 20000000; us = us * 3 + 7) {
		ostime_t const t = us2osticks(us);
		long const back = osticks2us(t);
		CHECK(back <= us);
		CHECK(us - back < 1000000 / rate + 1);	/* lost less than one tick */
		CHECK(us2osticks(back) <= t && t - us2osticks(back) <= 1);
	}
	for (long ms = 1; ms <= 3600000; ms = ms * 3 + 1) {
		ostime_t const t = ms2osticks(ms);
		CHECK(osticks2ms(t) <= ms && ms - osticks2ms(t) <= 1);
	}

	/* monotonic */
	CHECK(us2osticks(1000) < us2osticks(1001) || rate < 1000000);
	CHECK(ms2osticks(999) < ms2osticks(1000));
	CHECK(sec2osticks(59) < sec2osticks(60));

	/* the largest values the LMIC uses fit in ostime_t (s4_t). osticks2us()
	   returns an s4_t, so it is good to 2147 s of ticks and no further. */
	CHECK(sec2osticks(60 * 60) > 0);				/* an hour */
	CHECK(sec2osticks(0x7FFFFFFF / rate) > 0);			/* the wrap limit */
	CHECK(osticks2us(sec2osticks(2000)) == 2000000000);
	CHECK(osticks2ms(0x7FFFFFFF) > 0);				/* no overflow in the 64-bit product */

	/* the sign survives negative deltas (used for drift arithmetic) */
	CHECK(osticks2ms(-rate) == -1000);
	CHECK(us2osticks(-1000000) == -rate);

	printf("waits in the SX127x receive path at this rate:\n");
	SHOW(us2osticks(10000));		/* RX_RAMPUP_DEFAULT */
	SHOW(us2osticks(2000));			/* LMICbandplan_RX_EXTRA_MARGIN_osticks */
	SHOW(ms2osticksCeil(5));		/* Catena 4610 TCXO_DELAY_MS */
	SHOW(ms2osticks(1));			/* os_radio_reset() settle */
	SHOW(ms2osticks(9));			/* HAL_WAITUNTIL_DOWNCOUNT_THRESH */
	SHOW(us2osticks(43));			/* TXDONE fixup */
	SHOW(us2osticks(500));			/* SX127X_RX_POWER_UP */
	SHOW(us2osticksRound(128 << 3));	/* half symbol SF7 BW125: 512 us */
	SHOW(us2osticksRound(128 << 8));	/* half symbol SF12 BW125: 16384 us */
	SHOW(us2osticks(128 << 3));		/* same, truncated */

	if (s_failures == 0)
		printf("ok\n");
	else
		printf("%d failure(s)\n", s_failures);
	return s_failures;
}
