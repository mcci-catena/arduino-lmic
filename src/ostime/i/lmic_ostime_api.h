/*

Module:  lmic_ostime_api.h

Function:
	The ostime driver API.

Copyright & License:
	See accompanying LICENSE file.

Author:
	Terry Moore, MCCI	September 2026

*/

/// \file

#ifndef _lmic_ostime_api_h_
#define _lmic_ostime_api_h_

#ifndef _lmic_ostime_interface_h_
# include "lmic_ostime_interface.h"
#endif

/// \ingroup lmic_ostime
/// \{

/****************************************************************************\
|
|	Select and bind to the platform time driver.
|
|	The driver header defines LMIC_CFG_OsTime_DRIVER and
|	LMIC_OSTICKS_PER_SEC, declares its functions with
|	LMIC_OsTime_DECLARE_DRIVER_FNS(), and supplies the tick conversion
|	macros, normally by including lmic_ostime_conv_generic.h. It must
|	therefore be included after lmic_ostime_interface.h and before the
|	wrappers below.
|
\****************************************************************************/

// make sure we have a pointer to the ostime_t driver.
#if ! defined(LMIC_PLATFORM_OSTIME_INCLUDE)
# include "lmic_ostime_platform.h"
#endif

// bind the abstract API names to the concrete implementation methods
#include LMIC_STRINGIFY(LMIC_PLATFORM_OSTIME_INCLUDE)

// Check that the post-conditions were supplied by LMIC_PLATFORM_OSTIME_INCLUDE

// need a driver definition
#ifndef LMIC_CFG_OsTime_DRIVER
# error "The ostime driver header did not define LMIC_CFG_OsTime_DRIVER"
#endif

// need ticks per second
#ifndef LMIC_OSTICKS_PER_SEC
# error "The ostime driver header did not define LMIC_OSTICKS_PER_SEC"
#endif

// one tick must be 15.5 us to 100 us long. The ceiling is also bounded by
// LMICcore_rndDelay(), which casts LMIC_OSTICKS_PER_SEC to u2_t, and by the
// 32-bit arithmetic in the last step of calcAirTime(); 64516 clears both.
#if LMIC_OSTICKS_PER_SEC < 10000 || LMIC_OSTICKS_PER_SEC > 64516
# error "LMIC_OSTICKS_PER_SEC is out of range: one tick must be 15.5 us to 100 us long"
#endif

/// \brief traditional name for ticks per second.
#define OSTICKS_PER_SEC LMIC_OSTICKS_PER_SEC

// The driver must supply the tick conversions, either by including
// lmic_ostime_conv_generic.h or by defining them itself. They must be macros,
// so that we can check for them here; a driver that needs a real function
// defines a macro that forwards to it, the way oslmic.h allows for
// os_getTime().

#ifndef us2osticks
# error "The ostime driver header did not define us2osticks()"
#endif

#ifndef ms2osticks
# error "The ostime driver header did not define ms2osticks()"
#endif

#ifndef sec2osticks
# error "The ostime driver header did not define sec2osticks()"
#endif

#ifndef osticks2ms
# error "The ostime driver header did not define osticks2ms()"
#endif

#ifndef osticks2us
# error "The ostime driver header did not define osticks2us()"
#endif

#ifndef us2osticksCeil
# error "The ostime driver header did not define us2osticksCeil()"
#endif

#ifndef us2osticksRound
# error "The ostime driver header did not define us2osticksRound()"
#endif

#ifndef ms2osticksCeil
# error "The ostime driver header did not define ms2osticksCeil()"
#endif

#ifndef ms2osticksRound
# error "The ostime driver header did not define ms2osticksRound()"
#endif

/****************************************************************************\
|
|	The portable API functions.
|
\****************************************************************************/

LMIC_BEGIN_DECLS

///
/// \brief prepare the time base for use.
///
/// \details
///	The LMIC calls this function during initialization
///	to set up time-keeping operations.
///
/// \details
///	This function dispatches to a platform-specific function
///	that initializes the time-keeper. The dispatch is
///	done via a combination of macros and static inline functions;
///	we assume the compiler optimizes this to a direct call.
///
/// \sa LMIC_OsTime_initialize_fn_t().
///
static inline
void LMIC_ABI_STD
LMIC_OsTime_initialize(
	void
	)
	{
	// call the driver-supplied method.
	LMIC_OsTime_METHOD(LMIC_CFG_OsTime_DRIVER, initialize)();
	}

///
/// \brief Return the current time as a 32-bit tick count.
///
/// \return
///	An unsigned 32-bit count of ticks in platform-specific
///	units.
///
/// \details
///	This function dispatches to a platform-specific function
///	that queries whatever clock is being used. The dispatch is
///	done via a combination of macros and static inline functions;
///	we assume the compiler optimizes this to a direct call.
///
/// \sa LMIC_OsTime_ticks_fn_t().
///
static inline
u4_t LMIC_ABI_STD
LMIC_OsTime_ticks(
	void
	)
	{
	// call the driver-supplied method.
	return LMIC_OsTime_METHOD(LMIC_CFG_OsTime_DRIVER, ticks)();
	}

LMIC_END_DECLS

/// \}

#endif /* _lmic_ostime_api_h_ */

/**** end of lmic_ostime_api.h ****/
