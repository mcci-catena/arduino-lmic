/*

Module:	lmic_implementation.h

Function:
	Internal file containing LMIC implementation APIs

Copyright and License:
	This file copyright (C) 2022 by

		MCCI Corporation
		3520 Krums Corners Road
		Ithaca, NY  14850

	See accompanying LICENSE file for copyright and license information.

Author:
	Terry Moore, MCCI Corporation	February 2022

*/

#ifndef _lmic_implementation_h_
#define _lmic_implementation_h_	/* prevent multiple includes */

#pragma once

#ifndef _lmic_h_
# include "lmic.h"
#endif

LMIC_BEGIN_DECLS

static inline bit_t LMICJ_isShutdown(void) {
    return (LMIC.opmode & OP_SHUTDOWN) != 0;
}

static inline bit_t LMICJ_isFsmIdle() {
    return ! (LMIC.opmode & (OP_SCAN|OP_TXRXPEND|OP_SHUTDOWN));
}

static inline bit_t LMICJ_isTxPathBusy(void) {
    return (LMIC.opmode & (OP_POLL | OP_TXDATA | OP_JOINING | OP_TXRXPEND)) != 0;
}

static inline bit_t LMICJ_isTxRequested(void) {
    return (LMIC.opmode & (OP_JOINING|OP_REJOIN|OP_TXDATA|OP_POLL)) != 0;
}

static inline bit_t LMICJ_isActiveBeaconTracking(void) {
#if defined(DISABLE_BEACON)
    return 0;
#else
    return (LMIC.opmode & OP_TRACK) != 0;
#endif
}

static inline bit_t LMICJ_isEnabledBeaconTracking(void) {
#if defined(DISABLE_BEACON) || defined(DISABLE_PING)
    return 0;
#else
    return (LMIC.opmode & (OP_SCAN | OP_TRACK)) != 0;
#endif
}

///
/// \brief return \c true if Class B has been enabled.
///
/// \note This means both that tracking has been started, and LMIC_setPingable()
///     has been called. It prevents class C operation. However, it doesn't mean
///     that we've locked onto the beacon yet; the ClassB bit will not necessarily
///     be set in an uplink.
///
static inline bit_t LMICJ_isEnabledClassB(void) {
#if defined(DISABLE_BEACON) || defined(DISABLE_PING)
    return 0;
#else
    return (LMIC.opmode & (OP_SCAN | OP_TRACK)) != 0 && (LMIC.opmode & OP_PINGABLE) != 0;
#endif
}

///
/// \brief return \c true if Class B is being reported active in uplinks.
///
static inline bit_t LMICJ_isActiveClassB(void) {
#if defined(DISABLE_BEACON) || defined(DISABLE_PING)
    return 0;
#else
    return (LMIC.opmode & (OP_TRACK | OP_PINGABLE)) == (OP_TRACK | OP_PINGABLE);
#endif // DISABLE_BEACKON
}

LMIC_END_DECLS

#endif /* _lmic_implementation_h_ */
