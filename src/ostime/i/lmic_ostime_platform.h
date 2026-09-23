/*

Module:  lmic_ostime_platform.h

Function:
	Select the ostime driver for this platform.

Copyright & License:
	See accompanying LICENSE file.

Author:
	Terry Moore, MCCI	September 2026

*/

/// \file

#ifndef _lmic_ostime_platform_h_
#define _lmic_ostime_platform_h_

#include <Arduino.h>

/*

This file maps compiler and BSP predefines onto the name of a header that
supplies the ostime driver: the tick rate LMIC_OSTICKS_PER_SEC and the
LMIC_OsTime_<driver>_* functions. It is the only file in the LMIC that tests
vendor-specific predefines for the timekeeping functions; everything else
works from LMIC_PLATFORM_OSTIME_INCLUDE.

*/

#if defined (LMIC_PLATFORM_OSTIME_INCLUDE)
  /* we're lucky: the build system injected directly so we don't need to guess */
#elif defined(_mcci_arduino_version) && defined(ARDUINO_ARCH_STM32)
  /* MCCI STM32L0 BSP: LPTIM1 counting LSE, 32768 ticks/second. */
# define LMIC_PLATFORM_OSTIME_INCLUDE	../drivers/stm32l0lptim/lmic_ostime_stm32l0lptim.h
#elif defined(ARDUINO)
  /* any other Arduino BSP: scale down micros(), 62500 ticks/second. */
# define LMIC_PLATFORM_OSTIME_INCLUDE	../drivers/micros/lmic_ostime_micros.h
#else
/*

We reach this point when the LMIC is built outside the Arduino environment --
ARDUINO is not defined, so we cannot assume that micros() exists -- and no rule
above matched. The LMIC has no way to determine where time comes from on this
platform, and it will not pick one by default.

To fix this, define LMIC_PLATFORM_OSTIME_INCLUDE, either on the compiler command
line or in your lmic_project_config.h, naming a header that supplies an ostime
driver. src/ostime/drivers/micros/lmic_ostime_micros.h is the simplest example.
If your platform belongs in the list above, add a rule here instead.

*/
# error "No ostime driver for this platform: see the comment above this line."
#endif

#endif /* _lmic_ostime_platform_h_ */

/**** end of lmic_ostime_platform.h ****/
