/*

Module:  lmic_first.cpp

Function:
	Compile check: a C++ file whose first include is arduino_lmic.h.

Copyright and License:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	September 2026

Description:
	Client code often includes arduino_lmic.h before Arduino.h, so
	whatever the LMIC headers reach must not fall under C linkage
	(issue #1102). This file fails to compile on cores whose Arduino.h
	pulls in C++ templates if that regresses. It defines nothing the
	sketch uses.

*/

#include <arduino_lmic.h>

extern "C" u4_t lmic_header_test_probe(void);

u4_t lmic_header_test_probe(void) {
	return LMIC_getFCntUp();
}
