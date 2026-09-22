/*

Module:  lmic_ostime_micros.h

Function:
	The Micros ostime driver: time from the Arduino micros() counter.

Copyright & License:
	See accompanying LICENSE file.

Author:
	Terry Moore, MCCI	September 2026

*/

/// \file

#ifndef _lmic_ostime_micros_h_
#define _lmic_ostime_micros_h_

#ifndef _lmic_ostime_interface_h_
# include "../../i/lmic_ostime_interface.h"
#endif

LMIC_BEGIN_DECLS

/*! \defgroup lmic_ostime_micros Generic micros()-based ostime driver

\brief Derive the LMIC time base from the Arduino micros() counter.

\details
	Arduinos with a precision time source for micros() can
	use a the platform micros() API to derive timing for the
	LMIC. This ostime driver provides a portable implementation.

*/
/// \{

///
/// \brief mark this driver as the one selected for this build.
///
/// \details
///	With the Arduino IDE, everything gets compiled. So all our
///	implementation files have an outer guard to avoid generating
///	code unless the driver is really being used.
///
#define LMIC_OsTime_SELECTED_Micros	1

/// \brief name this driver for use by LMIC_OsTime_METHOD().
#define LMIC_CFG_OsTime_DRIVER		Micros

/// \brief base-2 logarithm of the number of microseconds in one tick.
#define LMIC_OsTime_Micros_US_PER_OSTICK_EXPONENT	4

/// \brief the number of microseconds in one tick.
#define LMIC_OsTime_Micros_US_PER_OSTICK		\
	(1 << LMIC_OsTime_Micros_US_PER_OSTICK_EXPONENT)

/// \brief the tick rate, in ticks per second.
#define LMIC_OSTICKS_PER_SEC				\
	(1000000 / LMIC_OsTime_Micros_US_PER_OSTICK)

// Declare our method functions in a standard way (avoiding drift).
LMIC_OsTime_DECLARE_DRIVER_FNS(Micros);

// One of our post conditions is to define all the time conversion
// macros. Luckily for us, we're the reference implementation, so
// we can just use the generic macro set.
#include "../../i/lmic_ostime_conv_generic.h"

/// \}

LMIC_END_DECLS

#endif /* _lmic_ostime_micros_h_ */

/**** end of lmic_ostime_micros.h ****/
