/*

Module:  lmic_version.h

Function:
	Version numbers for the Arduino LMIC library.

Copyright notice and license info:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	September 2026

Description:
	This file holds only the version numbers: the IBM LMIC version
	from which this library descends, and the Arduino LMIC version.
	The macros for building and comparing versions are in lmic_env.h.

	Clients that need only the version can include this file
	without including lmic.h.

*/

#ifndef _lmic_version_h_	/* prevent multiple includes */
#define _lmic_version_h_

#ifndef _lmic_env_h_
# include "lmic_env.h"
#endif

// LMIC version -- this is the IBM LMIC version
#define LMIC_VERSION_MAJOR 1
#define LMIC_VERSION_MINOR 6
#define LMIC_VERSION_BUILD 1468577746

/// Official ARDUINO LMIC version. Different from LMIC_VERSION_MAJOR etc. because those document the version released by IBM.
#define	ARDUINO_LMIC_VERSION    \
    ARDUINO_LMIC_VERSION_CALC(6, 1, 0, 7)  /* 6.1.0-pre7 */

#endif /* _lmic_version_h_ */
