/*

Module:  lmic_join_backoff.h

Function:
	Retransmission back-off for Join-Requests: table, state and API.

Copyright notice and license info:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	October 2026

Description:
	LoRaWAN limits the aggregated airtime of Join-Requests that a device
	sends while it waits for a Join-Accept (TS001-1.0.4 section 7,
	Table 56; LoRaWAN 1.0.3 section 7). The LMIC applies those limits
	from a table of rows. Each row gives the time after T0 from which it
	applies, an optional new cap (window length and airtime budget), and
	a pacing rule (minimum spacing between Join-Requests, as a multiple
	of the request's airtime, plus random jitter).

	T0 is the first Join-Request attempt after power-up, or after the
	last successful join. The state is LMIC.joinBackoff: LMIC_reset()
	clears it, and the session state blob (lmic_session_state.h, V3)
	carries it, so an application that saves and restores that blob
	keeps the back-off across a reboot (TR007 section 3.1.2).

	The default table is kJoinBackoffDefault[] in lmic_join_backoff.c.

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

typedef struct lmic_join_backoff_state_s lmic_join_backoff_state_t;
struct lmic_join_backoff_state_s {
	ostime_t lastTick;	///< os time of the last update of elapsedSec
	u4_t	residueTicks;	///< ticks not yet counted in elapsedSec
	u4_t	elapsedSec;	///< seconds since T0
	u4_t	windowStartSec;	///< current cap window, seconds since T0
	u4_t	windowUsedMs;	///< airtime used in that window
	u4_t	nextOkSec;	///< pacing: earliest next request, seconds since T0
	u2_t	capJitterSec;	///< added to the start of the next window
	u1_t	fActive;	///< 1 from the first join attempt to a Join-Accept
};

/****************************************************************************\
|
|	The table
|
\****************************************************************************/

///
/// \brief one row of the join back-off table
///
/// A row applies from \c startSec seconds after T0 until the next row's
/// \c startSec.
///
/// If \c windowSec is not zero, the row starts a new cap: from
/// \c startSec on, time is cut into windows of \c windowSec seconds, and
/// the Join-Request airtime in each window must stay below \c budgetMs.
/// A window ends early if a later row starts a new cap. If
/// \c windowSec is zero, the row keeps the cap of the row before it, and
/// \c budgetMs is ignored.
///
/// If \c divisor is not zero, a Join-Request of airtime A sent at time t
/// holds the next one until t + A * divisor, plus a random extra of up to
/// \c jitterPct percent of that. If \c divisor is zero, the LMIC's usual
/// join timing applies.
///
typedef struct lmic_join_backoff_step_s lmic_join_backoff_step_t;
struct lmic_join_backoff_step_s {
	u4_t	startSec;	///< row applies from this many seconds after T0
	u4_t	windowSec;	///< cap window length; 0: keep previous cap
	u4_t	budgetMs;	///< airtime in a window must stay below this
	u2_t	divisor;	///< pacing: spacing = airtime * divisor; 0: none
	u1_t	jitterPct;	///< pacing jitter, percent of the spacing
};

/// \brief the most rows a table may have
enum { LMIC_JOIN_BACKOFF_MAX_STEPS = 16 };

/// \brief install a table (NULL: the default). Returns 0 and changes
///	nothing if the table is not valid. The table is not copied.
bit_t LMIC_setJoinBackoffTable(const lmic_join_backoff_step_t *pTable, u1_t nTable);

/// \brief check a table without installing it.
bit_t LMIC_checkJoinBackoffTable(const lmic_join_backoff_step_t *pTable, u1_t nTable);

/// \brief get the table in use.
const lmic_join_backoff_step_t *LMIC_getJoinBackoffTable(u1_t *pnTable);

/****************************************************************************\
|
|	Control and status
|
\****************************************************************************/

/// \brief turn the limits off (0) or on (1, the default). While off, the
///	LMIC still keeps the accounts but does not delay Join-Requests.
///	For certification testing; a deployed device leaves it on.
void LMIC_setJoinBackoffEnabled(bit_t fEnable);

/// \brief forget all back-off history: the next join attempt is a new T0.
void LMIC_clearJoinBackoff(void);

typedef struct lmic_join_backoff_status_s lmic_join_backoff_status_t;
struct lmic_join_backoff_status_s {
	u1_t	fActive;	///< 1 from first join attempt to Join-Accept
	u1_t	fEnabled;	///< see LMIC_setJoinBackoffEnabled()
	u1_t	iStep;		///< table row now in effect
	u4_t	elapsedSec;	///< seconds since T0
	u4_t	windowStartSec;	///< current cap window, seconds since T0
	u4_t	windowEndSec;
	u4_t	windowUsedMs;	///< airtime used in the current window
	u4_t	budgetMs;	///< budget of the current window
	u4_t	nextOkSec;	///< pacing: earliest next request, seconds since T0
};

/// \brief report the state as of now.
void LMIC_getJoinBackoffStatus(lmic_join_backoff_status_t *pStatus);

/****************************************************************************\
|
|	Calls from the LMIC core (lmic.c)
|
\****************************************************************************/

/// \brief a join is starting: if no back-off is in progress, this is T0.
void LMICcore_joinBackoff_start(ostime_t now);

/// \brief earliest time, not before \p want, at which a Join-Request of
///	airtime \p airtime may start. Returns \p want if the back-off does
///	not delay it. Never returns more than an hour past \p now; the
///	caller asks again when that time comes.
ostime_t LMICcore_joinBackoff_earliest(ostime_t now, ostime_t want, ostime_t airtime);

/// \brief a Join-Request of airtime \p airtime started at \p txbeg.
void LMICcore_joinBackoff_noteTx(ostime_t txbeg, ostime_t airtime);

/// \brief bring LMIC.joinBackoff.elapsedSec up to \p now (before a save).
void LMICcore_joinBackoff_sync(ostime_t now);

/// \brief a Join-Accept was accepted: the back-off is over.
void LMICcore_joinBackoff_joined(void);

LMIC_END_DECLS

#endif /* _lmic_join_backoff_h_ */
