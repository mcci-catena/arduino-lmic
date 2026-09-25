/*

Module:  hal_stub.h

Function:
	Controls for the host stand-in for the Arduino HAL.

Copyright and License:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	September 2026

*/

#ifndef _hal_stub_h_
#define _hal_stub_h_

#include "oslmic.h"

LMIC_BEGIN_DECLS

/// \brief set the clock the stub returns from lmic_hal_ticks().
void hal_stub_setTicks(u4_t ticks);

/// \brief advance the stub clock.
void hal_stub_advanceTicks(u4_t delta);

/// \brief bring up the LMIC on the stub: os_init_ex() and LMIC_reset().
void hal_stub_startLmic(void);

LMIC_END_DECLS

#endif /* _hal_stub_h_ */
