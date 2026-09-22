/*

Module:  lmic_ostime_conv_generic.h

Function:
	Generic tick conversion macros, in terms of LMIC_OSTICKS_PER_SEC.

Copyright & License:
	See accompanying LICENSE file.

Author:
	Terry Moore, MCCI	September 2026

*/

/// \file

#ifndef _lmic_ostime_conv_generic_h_
#define _lmic_ostime_conv_generic_h_

#ifndef _lmic_ostime_api_h_
# include "lmic_ostime_api.h"
#endif

/*

An ostime driver header includes this file to get the tick conversion macros
expressed in terms of its LMIC_OSTICKS_PER_SEC. A driver whose tick rate admits
a cheaper form defines the nine macros itself and does not include this file.

*/

/// \brief convert microseconds to ticks, discarding any fraction
#define us2osticks(us)		((ostime_t)( ((int64_t)(us) * LMIC_OSTICKS_PER_SEC) / 1000000))

/// \brief convert milliseconds to ticks, discarding any fraction
#define ms2osticks(ms)		((ostime_t)( ((int64_t)(ms) * LMIC_OSTICKS_PER_SEC)    / 1000))

/// \brief convert seconds to ticks
#define sec2osticks(sec)	((ostime_t)( (int64_t)(sec) * LMIC_OSTICKS_PER_SEC))

/// \brief convert ticks to milliseconds, discarding any fraction
#define osticks2ms(os)		((s4_t)(((os)*(int64_t)1000    ) / LMIC_OSTICKS_PER_SEC))

/// \brief convert ticks to microseconds, discarding any fraction
#define osticks2us(os)		((s4_t)(((os)*(int64_t)1000000 ) / LMIC_OSTICKS_PER_SEC))

/// \brief convert microseconds to ticks, rounding towards positive infinity
#define us2osticksCeil(us)	((ostime_t)( ((int64_t)(us) * LMIC_OSTICKS_PER_SEC + 999999) / 1000000))

/// \brief convert microseconds to ticks, rounding to nearest
#define us2osticksRound(us)	((ostime_t)( ((int64_t)(us) * LMIC_OSTICKS_PER_SEC + 500000) / 1000000))

/// \brief convert milliseconds to ticks, rounding towards positive infinity
#define ms2osticksCeil(ms)	((ostime_t)( ((int64_t)(ms) * LMIC_OSTICKS_PER_SEC + 999) / 1000))

/// \brief convert milliseconds to ticks, rounding to nearest
#define ms2osticksRound(ms)	((ostime_t)( ((int64_t)(ms) * LMIC_OSTICKS_PER_SEC + 500) / 1000))

#endif /* _lmic_ostime_conv_generic_h_ */

/**** end of lmic_ostime_conv_generic.h ****/
