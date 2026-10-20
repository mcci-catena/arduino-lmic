/*

Module:  test_adr_ack_limits.c

Function:
	Host unit tests for LMIC_setAdrAckLimits() and
	LMIC_getAdrAckLimits() (issue #1154).

Copyright and License:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	October 2026

Description:
	Runs against the real core, compiled for the region given to make,
	with hal_stub.c in place of the Arduino HAL. Exit status is the
	number of failed checks.

*/

#include "hal_stub.h"
#include "lmic.h"

#include <stdio.h>

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

static void testDefaults(void) {
	u1_t limit = 0, delay = 0;

	LMIC_reset();
	LMIC_getAdrAckLimits(&limit, &delay);
	CHECK_EQ(limit, 64);
	CHECK_EQ(delay, 32);

	// link check mode starts the count at -limit
	LMIC_setLinkCheckMode(1);
	CHECK_EQ(LMIC.adrAckReq, -64);
	LMIC_setLinkCheckMode(0);
	CHECK_EQ(LMIC.adrAckReq, LINK_CHECK_OFF);
}

static void testSet(void) {
	u1_t limit = 0, delay = 0;

	LMIC_reset();
	CHECK(LMIC_setAdrAckLimits(8, 4));
	LMIC_getAdrAckLimits(&limit, &delay);
	CHECK_EQ(limit, 8);
	CHECK_EQ(delay, 4);

	LMIC_setLinkCheckMode(1);
	CHECK_EQ(LMIC.adrAckReq, -8);

	// out of range: refused, nothing changes. The count runs from
	// -limit up past delay + 720 in an s2_t, and LINK_CHECK_OFF (-128)
	// must stay out of reach, so limit is 1..127 and delay at least 1.
	CHECK(! LMIC_setAdrAckLimits(0, 4));
	CHECK(! LMIC_setAdrAckLimits(128, 4));
	CHECK(! LMIC_setAdrAckLimits(8, 0));
	LMIC_getAdrAckLimits(&limit, &delay);
	CHECK_EQ(limit, 8);
	CHECK_EQ(delay, 4);

	// LMIC_reset() puts back the defaults.
	LMIC_reset();
	LMIC_getAdrAckLimits(&limit, &delay);
	CHECK_EQ(limit, 64);
	CHECK_EQ(delay, 32);
}

static void testKeepsPlace(void) {
	// ten uplinks into the default 64: changing the limit to 16 leaves
	// six to go.
	LMIC_reset();
	LMIC_setLinkCheckMode(1);
	LMIC.adrAckReq += 10;
	CHECK_EQ(LMIC.adrAckReq, -54);
	CHECK(LMIC_setAdrAckLimits(16, 32));
	CHECK_EQ(LMIC.adrAckReq, -6);

	// already past the new limit: asking now (count 0, LINK_CHECK_CONT).
	CHECK(LMIC_setAdrAckLimits(4, 32));
	CHECK_EQ(LMIC.adrAckReq, 0);

	// a count past the limit (asking) is not moved.
	LMIC.adrAckReq = 5;
	CHECK(LMIC_setAdrAckLimits(16, 8));
	CHECK_EQ(LMIC.adrAckReq, 5);

	// link check off stays off.
	LMIC_setLinkCheckMode(0);
	CHECK(LMIC_setAdrAckLimits(16, 8));
	CHECK_EQ(LMIC.adrAckReq, LINK_CHECK_OFF);
}

int main(void) {
	hal_stub_startLmic();

	testDefaults();
	testSet();
	testKeepsPlace();

	if (s_failures == 0)
		printf("test_adr_ack_limits: all passed\n");
	return s_failures;
}
