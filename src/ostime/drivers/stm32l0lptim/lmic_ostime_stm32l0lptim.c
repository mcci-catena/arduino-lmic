/*

Module:  lmic_ostime_stm32l0lptim.c

Function:
	The Stm32L0Lptim ostime driver: time from LPTIM1 counting LSE.

Copyright & License:
	See accompanying LICENSE file.

Author:
	Terry Moore, MCCI	September 2026

*/

/// \file

#include "../../i/lmic_ostime_api.h"

#if defined(LMIC_OsTime_SELECTED_Stm32L0Lptim)

#include "../../../lmic/hal.h"
#include <Arduino.h>
#include "stm32/stm32_def.h"

/// \ingroup lmic_ostime_stm32l0lptim
/// \{

/****************************************************************************\
|
|	Family-specific definitions.
|
|	Everything that differs between STM32 families is collected here, so
|	that a driver for another family with an LPTIM can be cloned by
|	editing this block. The code below is common to any part with an
|	LPTIM.
|
\****************************************************************************/

/****************************************************************************\
|
|	Manifest constants & typedefs.
|
\****************************************************************************/

#if !defined(LMIC_ASSERTMSG)
# if !defined(CFG_noassert)
#  define LMIC_ASSERTMSG(cond, msg) do { if (!(cond)) lmic_hal_failed(msg ": " __FILE__, __LINE__); } while (0)
# else
#  define LMIC_ASSERTMSG(cond, msg) do {;} while (0)
# endif
#endif

typedef struct LMIC_OsTime_Stm32L0Lptim_Config_s
	{
	bool		fInitialized;		///< initially false.
	u4_t		rCfg;			///< expected CFG value
	u4_t		rArr;			///< expected ARR value
	u4_t		rIer;			///< expected IER value
	volatile u4_t	rCntExtended;		///< the upper 16 bits of the count, with
						///< bits 15..0 cleared.
	} LMIC_OsTime_Stm32L0Lptim_Config_t;

#define	LMIC_OsTime_Stm32L0Lptim_LPTIM1_INT_PRIORITY	0	// interrupt priority to set for LPTIM.

/****************************************************************************\
|
|	Variables.
|
\****************************************************************************/

// initialize all the fields to zero.
static LMIC_OsTime_Stm32L0Lptim_Config_t savedConfig = { 0 };

/****************************************************************************\
|
|	Code.
|
\****************************************************************************/

/*

Name:	LMIC_OsTime_Stm32L0Lptim_initialize()

Function:
	Prepare LPTIM1 to serve as the LMIC time base.

Definition:
	LMIC_OsTime_initialize_fn_t
		LMIC_OsTime_Stm32L0Lptim_initialize;

	void LMIC_OsTime_Stm32L0Lptim_initialize(
		void
		);

Description:

Returns:
	No explicit result.

*/

void LMIC_ABI_STD
LMIC_OsTime_Stm32L0Lptim_initialize(
	void
	)
	{
	LMIC_ASSERTMSG(! savedConfig.fInitialized, "LPTIM already initialized");

	// enable clock to LPTIM1
	__HAL_RCC_LPTIM1_CLK_ENABLE();
	// keep it clocked in sleep:
	__HAL_RCC_LPTIM1_CLK_SLEEP_ENABLE();

	// get a pointer to the register block.
	LPTIM_TypeDef * const pLptim = LPTIM1;

	// set LPTIM1 clock to LSE clock.
	__HAL_RCC_LPTIM1_CONFIG(RCC_LPTIM1CLKSOURCE_LSE);

	// disable everything so we can tweak the CFGR
	pLptim->CR = 0;

	// upcount from selected internal clock (which is LSE)
	u4_t rCfg = pLptim->CFGR & ~0x01FEEEDF;
	rCfg |=  0;
	pLptim->CFGR = savedConfig.rCfg = rCfg;

	// enable interrupts (while timer is not enabled)
	pLptim->IER = savedConfig.rIer = LPTIM_IER_ARRMIE;

	// clear out the interrrupt status register (write 1 to clear)
	pLptim->ICR = 0x7F;

	// enable the counter but don't start it
	pLptim->CR = LPTIM_CR_ENABLE;

	// allow time to settile
	delayMicroseconds(100);

	// set ARR to max value so we can count from 0 to 0xFFFF.
	// must be done after enabling.
	pLptim->ARR = savedConfig.rArr = 0xFFFF;

	// prepare for interrupts
	NVIC_SetPriority(LPTIM1_IRQn, LMIC_OsTime_Stm32L0Lptim_LPTIM1_INT_PRIORITY);

	// makes sure we can't get an interrupt while getting started.
	NVIC_DisableIRQ(LPTIM1_IRQn);

	// start in continuous mode. Note that CNTSTRT is auto-clear.
	pLptim->CR = LPTIM_CR_ENABLE | LPTIM_CR_CNTSTRT;

	// remember that we've been here.
	savedConfig.fInitialized = true;

	// all set: enable interrupts.
	NVIC_EnableIRQ(LPTIM1_IRQn);
	}

/*

Name:	LMIC_OsTime_Stm32L0Lptim_ticks()

Function:
	Return the current time as a 32-bit tick count.

Definition:
	u4_t LMIC_OsTime_Stm32L0Lptim_ticks(
		void
		);

Description:

Returns:
	A time counter in units of LMIC ticks, such that there are
	LMIC_OSTICKS_PER_SEC ticks per real-time second.

*/

u4_t LMIC_ABI_STD
LMIC_OsTime_Stm32L0Lptim_ticks(
	void
	)
	{
	LMIC_ASSERTMSG(savedConfig.fInitialized, "not initialized");

	// get a pointer to the register block.
	LPTIM_TypeDef * const pLptim = LPTIM1;

	// assume an LPTIM1 int cannot preempt us.
	bool fLptim1CanPreempt = false;

	// are interrupts enabled? if so, need to dig deeeper.
	// this check assumes we're on a Cortex M0 etc. where the
	// PRIMASK() is only one bit.
	if (__get_PRIMASK() == 0)
		{
		// determine the current CM0 urgency.
		u4_t const ipsr = __get_IPSR();	// 0: thread mode; >= 16 == current IRQ number

		// in thread mode, we can be preempted
		if (ipsr == 0)
			fLptim1CanPreempt = true;
		// if we're handling a lower priority interrupt, we might be preemptible by LPTIM1
		else if (ipsr >= 16)
			{
			// find the priority of the interrupt we're handling
			u4_t intPrio = NVIC_GetPriority(
						(IRQn_Type) (ipsr - 16)
						);

			// compare to what we set for the LMIC
			if (intPrio > LMIC_OsTime_Stm32L0Lptim_LPTIM1_INT_PRIORITY)
				fLptim1CanPreempt = true;
			}
		}

	// This is an unconditional assert: confirm the hardware state.
	// Others might mess with it; if so, we'll have bad trouble here.
	if (pLptim->CFGR != savedConfig.rCfg ||
	    pLptim->CR != LPTIM_CR_ENABLE ||
	    pLptim->ARR != savedConfig.rArr ||
	    pLptim->IER !=savedConfig.rIer)
		{
		lmic_hal_failed("unexpected change to LPTIM1 config: " __FILE__, __LINE__);
		}

	/* now: read the register to initialized */
	u4_t rCntExtendedCandidate = savedConfig.rCntExtended;
	u4_t rCntCandidate = pLptim->CNT & 0xFFFFu;

	/* the double read can go faster if we're preemptible. */
	if (fLptim1CanPreempt)
		{
		for (;;)
			{
			u4_t const rCntNow = pLptim->CNT & 0xFFFFu;
			u4_t const rCntExtendedNow = savedConfig.rCntExtended;

			if (rCntNow == rCntCandidate && rCntExtendedNow == rCntExtendedCandidate)
				break;

			rCntCandidate = rCntNow;
			rCntExtendedCandidate = rCntExtendedNow;
			}
		}
	else
		{
		//
		// if not preemptible and an LPTIM1 interrupts is pending, then there is
		// a pending update to rCntExtendedCandidate. There's no race because an
		// interrupt can't happen until the caller becomes preemptible again.
		// The tick handler also consumes the ARRM bit explicitly; so if we clear
		// here and there's an interrupt anyway later due to the pending ARRM,
		// the ISR will return without incrementing.
		//
		// Since most of the time we should be preemptible, we don't
		// burden the preemptible path with the extra polling.
		//
		bool fArrmCandidate = !! (pLptim->ISR & LPTIM_ISR_ARRM);

		for (;;)
			{
			u4_t const rCntNow = pLptim->CNT & 0xFFFFu;
			u4_t const rCntExtendedNow = savedConfig.rCntExtended;
			bool const fArrmNow = !! (pLptim->ISR & LPTIM_ISR_ARRM);

			if (rCntNow == rCntCandidate && rCntExtendedNow == rCntExtendedCandidate && fArrmCandidate == fArrmNow)
				break;

			rCntCandidate = rCntNow;
			rCntExtendedCandidate = rCntExtendedNow;
			fArrmCandidate = fArrmNow;
			}

		// We loop until both reads have ARRM the same state. Thus if ARRM
		// is set, then rCntCandidate is after the rollover. So we consume
		// the ARRM event.
		if (fArrmCandidate)
			{
			// it is indeed pending. allow for a routine that might
			// loop forever with interrupts disabled, even though we
			// disapprove, by consuming the interrupt here and
			// incrementing the count.
			pLptim->ICR = LPTIM_ICR_ARRMCF;
			rCntExtendedCandidate += 0x10000u;
			savedConfig.rCntExtended = rCntExtendedCandidate;
			}
		}

	// finally: prepare the result and return.
	return rCntCandidate | rCntExtendedCandidate;
	}

/*

Name:	LPTIM1_IRQHandler()

Function:
	Advance the high half of the 32-bit tick count.

Definition:
	void LPTIM1_IRQHandler(
		void
		);

Description:

Returns:
	No explicit result.

Notes:
	This is the one symbol in this driver that cannot carry an LMIC
	prefix: the name is fixed by the startup vector table.

*/

void
LPTIM1_IRQHandler(
	void
	)
	{
	NVIC_ClearPendingIRQ(LPTIM1_IRQn);

	if (LPTIM1->ISR & LPTIM_ISR_ARRM) //If there was a compare match (which overflowed)
		{
		// clear the request
		LPTIM1->ICR = LPTIM_ICR_ARRMCF;

		savedConfig.rCntExtended += 0x10000;
		}
	}

/// \}

#endif /* defined(LMIC_OsTime_SELECTED_Stm32L0Lptim) */

/**** end of lmic_ostime_stm32l0lptim.c ****/
