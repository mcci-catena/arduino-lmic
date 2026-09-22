/*

Module:  lmic_ostime_stm32l0lptim.h

Function:
	The Stm32L0Lptim ostime driver: time from LPTIM1 counting LSE.

Copyright & License:
	See accompanying LICENSE file.

Author:
	Terry Moore, MCCI	September 2026

*/

/// \file

#ifndef _lmic_ostime_stm32l0lptim_h_
#define _lmic_ostime_stm32l0lptim_h_

#ifndef _lmic_ostime_interface_h_
# include "../../i/lmic_ostime_interface.h"
#endif

LMIC_BEGIN_DECLS

/*! \defgroup lmic_ostime_stm32l0lptim STM32L0 LPTIM1 ostime driver

\brief Derive the LMIC time base from LPTIM1, counting the 32768 Hz LSE.

\details
	The STM32L0 system oscillator is free-running, and drifts far more
	than LoRaWAN receive-window timing wants. It also stops in Stop mode,
	so the application has to repair the tick on wake. LPTIM1 can count
	the LSE directly, keeps counting through Stop mode, and is accurate
	to the crystal.

	LPTIM1 counts 16 bits; this driver extends the count to 32 bits in
	software.

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
#define LMIC_OsTime_SELECTED_Stm32L0Lptim	1

/// \brief name this driver for use by LMIC_OsTime_METHOD().
#define LMIC_CFG_OsTime_DRIVER			Stm32L0Lptim

/// \brief the tick rate, in ticks per second: the LSE frequency.
#define LMIC_OSTICKS_PER_SEC			32768

// Declare our method functions in a standard way (avoiding drift).
LMIC_OsTime_DECLARE_DRIVER_FNS(Stm32L0Lptim);

// One of our post conditions is to define all the time conversion
// macros. This driver has no reason to do anything special, so we
// use the generic macro set.
#include "../../i/lmic_ostime_conv_generic.h"

/// \}

LMIC_END_DECLS

#endif /* _lmic_ostime_stm32l0lptim_h_ */

/**** end of lmic_ostime_stm32l0lptim.h ****/
