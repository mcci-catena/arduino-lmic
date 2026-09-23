/*

Module:  lmic_compat_v6.h

Function:
	Opt-in compatibility definitions for code written against V6.

Copyright & License:
	See accompanying LICENSE file.

Author:
	Terry Moore, MCCI	September 2026

Description:
	V7 withdrew a number of names that V6 supplied. An application that
	wants them back includes this file; nothing in the LMIC includes it.

	Names that cannot be given back are left out rather than approximated.

*/

/// \file

#ifndef _lmic_compat_v6_h_
#define _lmic_compat_v6_h_

#ifndef _lmic_h_
# include "lmic.h"
#endif

LMIC_BEGIN_DECLS

/*! \defgroup lmic_compat_v6 V6 compatibility

\brief Names withdrawn in V7, for applications that still use them.

\details
	V7 removes many names, primarily because the code reorganization
	makes them irrelevant, and we want to reduce namespace size.

	The best solution for a client program is to recode using new
	facilities that replace the withdrawn ones. But it may be
	expedient to simply continue to use the old names. You can do that
	by including "lmic_compat_v6.h" in each file that needs to use
	the old names.

*/

/// \{

/// \brief traditional name for ticks per second.
#define OSTICKS_PER_SEC LMIC_OSTICKS_PER_SEC

///
/// \def US_PER_OSTICK_EXPONENT
/// \brief Exponent used in older LMICs when LMIC_OSTICKS_PER_SEC was 62500.
///
/// \def US_PER_OSTICK
/// \brief Divisor used in older LMICs when LMIC_OSTICKS_PER_SEC was 62500.
///
#if LMIC_OSTICKS_PER_SEC == 62500
# define US_PER_OSTICK_EXPONENT	4
# define US_PER_OSTICK		16
#endif

/// \brief old low-level tick primitive provided when ostime was implemented by
/// a unitary HAL. Identical signature to LMIC_OsTime_ticks().
#define lmic_hal_ticks	LMIC_OsTime_ticks

/// \}

LMIC_END_DECLS

#endif /* _lmic_compat_v6_h_ */

/**** end of lmic_compat_v6.h ****/
