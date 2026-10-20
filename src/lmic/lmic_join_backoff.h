/*

Module:  lmic_join_backoff.h

Function:
	Retransmission back-off for Join-Requests: state and API.

Copyright notice and license info:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	October 2026

Description:
	LoRaWAN limits the aggregated airtime of Join-Requests that a device
	sends while it waits for a Join-Accept (TS001-1.0.4 section 7,
	Table 56; LoRaWAN 1.0.3 section 7): under 36 s in the first hour
	after T0, under 36 s in the next 10 hours, and under 8.7 s in each
	24 hours after that, in fixed windows counted from T0. See
	lmic_join_backoff.c for how the LMIC keeps to them.

	T0 is the first Join-Request attempt after power-up, or after the
	last successful join. The state is LMIC.joinBackoff: LMIC_reset()
	clears it, and the session state blob (lmic_session_state.h, V3)
	carries it, so an application that saves and restores that blob
	keeps the back-off across a reboot (TR007 section 3.1.2).

*/

#ifndef _lmic_join_backoff_h_	/* prevent multiple includes */
#define _lmic_join_backoff_h_

#include <stddef.h>

#ifndef _oslmic_h_
# include "oslmic.h"
#endif

LMIC_BEGIN_DECLS

/****************************************************************************\
|
|	The state (LMIC.joinBackoff)
|
\****************************************************************************/

/// \brief phases of the back-off (lmic_join_backoff_state_t::phase)
enum {
	LMIC_JOIN_BACKOFF_PHASE_15MIN = 0,	///< T0 to T0+15 min
	LMIC_JOIN_BACKOFF_PHASE_HOUR1 = 1,	///< T0+15 min to T0+1 h
	LMIC_JOIN_BACKOFF_PHASE_HOURS1_6 = 2,	///< T0+1 h to T0+6 h
	LMIC_JOIN_BACKOFF_PHASE_HOURS6_11 = 3,	///< T0+6 h to T0+11 h
	LMIC_JOIN_BACKOFF_PHASE_AFTER = 4,	///< from T0+11 h on; the last phase
};

typedef struct lmic_join_backoff_state_s lmic_join_backoff_state_t;
struct lmic_join_backoff_state_s {
	ostime_t phaseStart;	///< os time the current phase began
	u1_t	phase;		///< LMIC_JOIN_BACKOFF_PHASE_*
	u1_t	fActive;	///< 1 from the first join attempt to a Join-Accept
};

/****************************************************************************\
|
|	Control
|
\****************************************************************************/

/// \brief turn the limits off (0) or on (1, the default). While off, the
///	LMIC still keeps time but does not space Join-Requests.
///	For certification testing; a deployed device leaves it on.
void LMIC_setJoinBackoffEnabled(bit_t fEnable);

/// \brief forget all back-off history: the next join attempt is a new T0.
void LMIC_clearJoinBackoff(void);

/****************************************************************************\
|
|	Calls from the LMIC core (lmic.c, lmic_session_state.c)
|
\****************************************************************************/

/// \brief a join is starting: if no back-off is in progress, this is T0.
void LMICcore_joinBackoff_start(ostime_t now);

/// \brief the duty rate (LMIC.globalDutyRate) for a Join-Request
///	attempted at \p now: Join-Request starts are at least airtime x
///	2^rate apart. 0 when the back-off is off or not running.
u1_t LMICcore_joinBackoff_dutyRate(ostime_t now);

/// \brief bring the back-off clock up to \p now (before a save, or to
///	read LMIC.joinBackoff).
void LMICcore_joinBackoff_sync(ostime_t now);

/// \brief a Join-Accept was accepted: the back-off is over.
void LMICcore_joinBackoff_joined(void);

LMIC_END_DECLS

#endif /* _lmic_join_backoff_h_ */
