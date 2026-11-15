/*

Module:  test_accessors.c

Function:
	Host unit tests for the accessors in lmic_accessors.h.

Copyright and License:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	November 2026

Description:
	Runs against the real core, compiled for the region given to make,
	with hal_stub.c in place of the Arduino HAL. Exit status is the
	number of failed checks.

*/

#include "hal_stub.h"
#include "lmic.h"
#include "lmic_accessors.h"

#include <stdio.h>

static int s_failures;

#define CHECK_EQ(a, b) do {							\
	long long const a_ = (long long)(a), b_ = (long long)(b);		\
	if (a_ != b_) {								\
		++s_failures;							\
		printf("FAIL %s:%d: %s == %s: %lld != %lld\n",			\
			__FILE__, __LINE__, #a, #b, a_, b_);			\
	}									\
} while (0)

// LMIC_getRSSI() returns dBm; the radio drivers store RSSI + RSSI_OFF.
static void testRssi(void) {
	LMIC_reset();

	LMIC.rssi = (s1_t)(RSSI_OFF + -30);
	CHECK_EQ(LMIC_getRSSI(), -30);

	LMIC.rssi = (s1_t)(RSSI_OFF + -137);
	CHECK_EQ(LMIC_getRSSI(), -137);

	// the ends of the s1_t that holds the stored value.
	LMIC.rssi = 127;
	CHECK_EQ(LMIC_getRSSI(), 63);
	LMIC.rssi = -128;
	CHECK_EQ(LMIC_getRSSI(), -192);
}

// LMIC_getSNR() returns the SNR in dB times 4, as the drivers store it.
static void testSnr(void) {
	LMIC.snr = -4 * 7;
	CHECK_EQ(LMIC_getSNR(), -28);
}

int main(void) {
	hal_stub_startLmic();

	testRssi();
	testSnr();

	if (s_failures == 0)
		printf("test_accessors: all passed\n");
	return s_failures;
}

/**** end of test_accessors.c ****/
