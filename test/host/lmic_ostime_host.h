/*

Module:  lmic_ostime_host.h

Function:
	The Host ostime driver: time from a counter the unit tests control.

Copyright and License:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	September 2026

Description:
	The host Makefile names this file in LMIC_PLATFORM_OSTIME_INCLUDE,
	so the core binds its time base to the stub clock in hal_stub.c
	instead of an Arduino driver. The tick rate comes from the Makefile
	as LMIC_OSTICKS_PER_SEC, so the same core can be tested at 62500
	(the micros() driver) and 32768 (the STM32L0 LPTIM driver).

*/

#ifndef _lmic_ostime_host_h_
#define _lmic_ostime_host_h_

#ifndef _lmic_ostime_interface_h_
# include "lmic_ostime_interface.h"
#endif

#define LMIC_OsTime_SELECTED_Host	1
#define LMIC_CFG_OsTime_DRIVER		Host

#ifndef LMIC_OSTICKS_PER_SEC
# define LMIC_OSTICKS_PER_SEC		62500
#endif

LMIC_OsTime_DECLARE_DRIVER_FNS(Host);

#include "lmic_ostime_conv_generic.h"

#endif /* _lmic_ostime_host_h_ */
