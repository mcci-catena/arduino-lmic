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

/// \brief turn the simulated radio on or off (off by default). When on, a
///	LoRa transmit completes after its airtime and a single receive times
///	out, both delivered by lmic_hal_processPendingIRQs(). When off, the
///	radio never completes anything.
void hal_stub_setRadioAutoComplete(bit_t fEnable);

/// \brief ticks until the simulated radio's next event, 0 if due now, or
///	HAL_STUB_RADIO_IDLE if nothing is pending.
u4_t hal_stub_radioTicksUntilEvent(void);
#define HAL_STUB_RADIO_IDLE	UINT32_C(0xFFFFFFFF)

/// \brief run the LMIC scheduler until \p untilTick (an os time), moving
///	the stub clock straight to each scheduled job or radio event.
void hal_stub_runUntil(u4_t untilTick);

/// \brief like hal_stub_runUntil(), but also return as soon as \p *pStop
///	is nonzero after a job has run (for example, set by an event
///	handler), as an application's loop would see it.
void hal_stub_runUntilStop(u4_t untilTick, const volatile int *pStop);

LMIC_END_DECLS

#endif /* _hal_stub_h_ */
