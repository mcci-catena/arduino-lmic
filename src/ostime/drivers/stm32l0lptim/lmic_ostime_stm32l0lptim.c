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

// The BSP's time-provider hook (mcci-catena/Arduino_Core_STM32#223), if
// this core has it. Older cores lack the header; then the driver does not
// register, and millis() stays on SysTick.
#if defined(__has_include)
# if __has_include(<ArduinoTimeProvider.h>)
#  include <ArduinoTimeProvider.h>
# endif
#endif

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
	volatile u4_t	rCntHigh;		///< bits 63..32 of the count, advanced
						///< when rCntExtended wraps.
	volatile bool	fWakeArmed;		///< a compare-match wake is armed.
	volatile bool	fCmpWritePending;	///< a CMP write has not yet synchronized.
	volatile uint64_t wakeTarget;		///< the 64-bit tick to wake at.
	LMIC_OsTime_Stm32L0Lptim_WakeCb_t *pWakeCb;	///< called from the interrupt when
						///< the wake fires.
	void		*pWakeCtx;		///< context for pWakeCb.
	} LMIC_OsTime_Stm32L0Lptim_Config_t;

// CMP when no wake is armed: the same as ARR, so the compare match
// coincides with the overflow and costs no extra wake-up.
#define	LMIC_OsTime_Stm32L0Lptim_CMP_IDLE	0xFFFFu

// a wake must be at least this many ticks ahead: a CMP write takes about
// two LSE cycles to reach the counter's clock domain (#1120).
#define	LMIC_OsTime_Stm32L0Lptim_WAKE_MIN_LEAD	4u

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

/****************************************************************************\
|
|	Time provider for millis() and micros() (#1120)
|
\****************************************************************************/

#if defined(ARDUINO_CORE_STM32_TIME_PROVIDER)

// referenced weakly, so the LMIC still links with a core whose header has
// the declaration but whose library lacks the function.
extern const ArduinoTimeProvider_t *
setArduinoTimeProvider(const ArduinoTimeProvider_t *pProvider) __attribute__((weak));

typedef struct LMIC_OsTime_Stm32L0Lptim_Provider_s
	{
	ArduinoTimeProvider_t	table;		///< must be first.
	} LMIC_OsTime_Stm32L0Lptim_Provider_t;

// the SysTick time and the LPTIM count at the moment of registration, so
// that millis() and micros() carry on from where SysTick had them instead
// of jumping back to the LPTIM's start.
static uint64_t s_baseTicks;
static uint32_t s_baseMillis;
static uint32_t s_baseMicros;

static uint64_t
LMIC_OsTime_Stm32L0Lptim_getTime(
	const ArduinoTimeProvider_t *pThis,
	ArduinoTimeSelect_t which
	)
	{
	(void) pThis;
	uint64_t const dt = LMIC_OsTime_Stm32L0Lptim_ticks64() - s_baseTicks;

	// 32768 ticks per second: 1000/32768 = 125/4096 ms per tick, and
	// 1000000/32768 = 15625/512 us per tick, so shifts do the division.
	if (which == kArduinoTime_Millis)
		return s_baseMillis + ((dt * 125u) >> 12);
	else if (which == kArduinoTime_Micros)
		return s_baseMicros + ((dt * 15625u) >> 9);
	else
		return 0;
	}

static const LMIC_OsTime_Stm32L0Lptim_Provider_t s_provider =
	{
	.table =
		{
		.nSize = sizeof(ArduinoTimeProvider_t),
		.pGetTime = LMIC_OsTime_Stm32L0Lptim_getTime,
		.pDelay = NULL,			// the core loops on millis()
		.pDelayMicroseconds = NULL,	// the core counts SysTick
		},
	};

static void
LMIC_OsTime_Stm32L0Lptim_registerTimeProvider(
	void
	)
	{
	if (setArduinoTimeProvider == NULL)
		return;

	// sample SysTick time and the LPTIM count together.
	lmic_hal_disableIRQs();
	s_baseMillis = GetCurrentMilli();
	s_baseMicros = GetCurrentMicro();
	s_baseTicks = LMIC_OsTime_Stm32L0Lptim_ticks64();
	(void) setArduinoTimeProvider(&s_provider.table);
	lmic_hal_enableIRQs();
	}

#else /* ! defined(ARDUINO_CORE_STM32_TIME_PROVIDER) */

static void
LMIC_OsTime_Stm32L0Lptim_registerTimeProvider(
	void
	)
	{
	}

#endif /* defined(ARDUINO_CORE_STM32_TIME_PROVIDER) */

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

	// unlock the backup domain and start the LSE oscillator; LPTIM1's
	// kernel clock won't run (and its registers won't latch writes)
	// until LSE is actually running.
	__HAL_RCC_PWR_CLK_ENABLE();
	HAL_PWR_EnableBkUpAccess();

	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	LMIC_ASSERTMSG(HAL_RCC_OscConfig(&RCC_OscInitStruct) == HAL_OK, "LSE start failed");

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
	// ARRM extends the count; CMPM serves requestWakeAt() (#1120). IER
	// may be written only while the LPTIM is disabled, so both are
	// enabled here, once.
	pLptim->IER = savedConfig.rIer = LPTIM_IER_ARRMIE | LPTIM_IER_CMPMIE;

	// clear out the interrrupt status register (write 1 to clear)
	pLptim->ICR = 0x7F;

	// enable the counter but don't start it
	pLptim->CR = LPTIM_CR_ENABLE;

	// allow time to settile
	delayMicroseconds(100);

	// set ARR to max value so we can count from 0 to 0xFFFF.
	// must be done after enabling.
	pLptim->ARR = savedConfig.rArr = 0xFFFF;

	// the ARR write crosses into the LPTIM kernel (LSE) clock domain and
	// isn't visible on readback until the hardware confirms it; without
	// this wait, a ticks() call made immediately after initialize()
	// returns can still see the register's pre-write value.
	while (! (pLptim->ISR & LPTIM_ISR_ARROK))
		/* wait for the write to synchronize */;
	pLptim->ICR = LPTIM_ICR_ARROKCF;

	// no wake armed: compare at the overflow.
	pLptim->CMP = LMIC_OsTime_Stm32L0Lptim_CMP_IDLE;
	while (! (pLptim->ISR & LPTIM_ISR_CMPOK))
		/* wait for the write to synchronize */;
	pLptim->ICR = LPTIM_ICR_CMPOKCF | LPTIM_ICR_CMPMCF;

	// prepare for interrupts
	NVIC_SetPriority(LPTIM1_IRQn, LMIC_OsTime_Stm32L0Lptim_LPTIM1_INT_PRIORITY);

	// LPTIM1 wakes the CPU from Stop through EXTI line 29. Unmask it here
	// rather than rely on the reset value of EXTI_IMR: if it were masked,
	// the ARRM interrupt would not wake the CPU, and every overflow but
	// the first during a sleep would be lost (#1116). ticks() checks that
	// it stays unmasked.
	EXTI->IMR |= EXTI_IMR_IM29;

	// makes sure we can't get an interrupt while getting started.
	NVIC_DisableIRQ(LPTIM1_IRQn);

	// start in continuous mode. Note that CNTSTRT is auto-clear.
	pLptim->CR = LPTIM_CR_ENABLE | LPTIM_CR_CNTSTRT;

	// CNTSTRT is cleared by hardware only after a synchronization delay
	// of about two kernel (LSE) clock cycles; wait for it to clear so a
	// ticks() call made immediately after initialize() returns doesn't
	// see CR still carrying CNTSTRT and trip the config-check assert.
	while (pLptim->CR & LPTIM_CR_CNTSTRT)
		/* wait for the bit to self-clear */;

	// remember that we've been here.
	savedConfig.fInitialized = true;

	// all set: enable interrupts.
	NVIC_EnableIRQ(LPTIM1_IRQn);

	// let millis() and micros() come from this clock (#1120).
	LMIC_OsTime_Stm32L0Lptim_registerTimeProvider();
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

// read the count: returns bits 31..0, and sets *pHigh (if not NULL) to
// bits 63..32, read consistently with them.
static u4_t
LMIC_OsTime_Stm32L0Lptim_read(
	u4_t *pHigh
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
	    pLptim->IER !=savedConfig.rIer ||
	    (EXTI->IMR & EXTI_IMR_IM29) == 0)
		{
		lmic_hal_failed("unexpected change to LPTIM1 config: " __FILE__, __LINE__);
		}

	/* now: read the register to initialized */
	u4_t rCntHighCandidate = savedConfig.rCntHigh;
	u4_t rCntExtendedCandidate = savedConfig.rCntExtended;
	u4_t rCntCandidate = pLptim->CNT & 0xFFFFu;

	/* the double read can go faster if we're preemptible. */
	if (fLptim1CanPreempt)
		{
		for (;;)
			{
			u4_t const rCntNow = pLptim->CNT & 0xFFFFu;
			u4_t const rCntExtendedNow = savedConfig.rCntExtended;
			u4_t const rCntHighNow = savedConfig.rCntHigh;

			if (rCntNow == rCntCandidate && rCntExtendedNow == rCntExtendedCandidate &&
			    rCntHighNow == rCntHighCandidate)
				break;

			rCntCandidate = rCntNow;
			rCntExtendedCandidate = rCntExtendedNow;
			rCntHighCandidate = rCntHighNow;
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
			u4_t const rCntHighNow = savedConfig.rCntHigh;
			bool const fArrmNow = !! (pLptim->ISR & LPTIM_ISR_ARRM);

			if (rCntNow == rCntCandidate && rCntExtendedNow == rCntExtendedCandidate &&
			    rCntHighNow == rCntHighCandidate && fArrmCandidate == fArrmNow)
				break;

			rCntCandidate = rCntNow;
			rCntExtendedCandidate = rCntExtendedNow;
			rCntHighCandidate = rCntHighNow;
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
			if (rCntExtendedCandidate == 0)
				savedConfig.rCntHigh = ++rCntHighCandidate;
			}
		}

	// finally: prepare the result and return.
	if (pHigh != NULL)
		*pHigh = rCntHighCandidate;
	return rCntCandidate | rCntExtendedCandidate;
	}

u4_t LMIC_ABI_STD
LMIC_OsTime_Stm32L0Lptim_ticks(
	void
	)
	{
	return LMIC_OsTime_Stm32L0Lptim_read(NULL);
	}

/*

Name:	LMIC_OsTime_Stm32L0Lptim_ticks64()

Function:
	Return the current time as a 64-bit tick count.

Definition:
	uint64_t LMIC_OsTime_Stm32L0Lptim_ticks64(
		void
		);

Description:
	The same count as LMIC_OsTime_Stm32L0Lptim_ticks(), which returns its
	low 32 bits, carried on to 64 bits so that it does not wrap (the
	32-bit count wraps every 36.4 hours at 32768 ticks per second). Like
	ticks(), it may be called with interrupts masked and from interrupt
	handlers.

Returns:
	The tick count since initialization.

*/

uint64_t LMIC_ABI_STD
LMIC_OsTime_Stm32L0Lptim_ticks64(
	void
	)
	{
	u4_t high;
	u4_t const low = LMIC_OsTime_Stm32L0Lptim_read(&high);

	return ((uint64_t) high << 32) | low;
	}

// write CMP, first waiting for any earlier write to finish.
static void
LMIC_OsTime_Stm32L0Lptim_writeCmp(
	u4_t value
	)
	{
	LPTIM_TypeDef * const pLptim = LPTIM1;

	if (savedConfig.fCmpWritePending)
		{
		while (! (pLptim->ISR & LPTIM_ISR_CMPOK))
			/* wait for the earlier write */;
		}
	pLptim->ICR = LPTIM_ICR_CMPOKCF;
	pLptim->CMP = value;
	savedConfig.fCmpWritePending = true;
	}

/*

Name:	LMIC_OsTime_Stm32L0Lptim_setWakeCallback()

Function:
	Name the function to call when an armed wake fires.

Definition:
	void LMIC_OsTime_Stm32L0Lptim_setWakeCallback(
		LMIC_OsTime_Stm32L0Lptim_WakeCb_t *pCb,
		void *pCtx
		);

Description:
	pCb is called from the LPTIM1 interrupt, with pCtx and the 64-bit
	tick count read in the interrupt. It must be short; typically it
	sets a flag for the application's main loop. NULL removes it.

Returns:
	No explicit result.

*/

void LMIC_ABI_STD
LMIC_OsTime_Stm32L0Lptim_setWakeCallback(
	LMIC_OsTime_Stm32L0Lptim_WakeCb_t *pCb,
	void *pCtx
	)
	{
	lmic_hal_disableIRQs();
	savedConfig.pWakeCb = pCb;
	savedConfig.pWakeCtx = pCtx;
	lmic_hal_enableIRQs();
	}

/*

Name:	LMIC_OsTime_Stm32L0Lptim_requestWakeAt()

Function:
	Arm a wake-up at a given 64-bit tick, using the LPTIM1 compare match.

Definition:
	bool LMIC_OsTime_Stm32L0Lptim_requestWakeAt(
		uint64_t target
		);

Description:
	When the count reaches target, the LPTIM1 interrupt disarms the wake
	and calls the wake callback. On a Catena 5230 the interrupt reads the
	count one tick (30.5 us) after the target. The interrupt wakes the CPU from Sleep
	or Stop (EXTI line 29). A target more than one 16-bit wrap away is
	fine: the compare matches once per wrap, and the interrupt fires the
	wake only when the full count has reached the target. A new request
	replaces the previous one.

Returns:
	true if armed; false if target is less than a few ticks away (or
	past), in which case nothing is armed and the caller should treat the
	time as already reached.

*/

bool LMIC_ABI_STD
LMIC_OsTime_Stm32L0Lptim_requestWakeAt(
	uint64_t target
	)
	{
	bool result;

	lmic_hal_disableIRQs();
	uint64_t const now = LMIC_OsTime_Stm32L0Lptim_ticks64();

	if (target < now + LMIC_OsTime_Stm32L0Lptim_WAKE_MIN_LEAD)
		{
		savedConfig.fWakeArmed = false;
		result = false;
		}
	else
		{
		savedConfig.wakeTarget = target;
		savedConfig.fWakeArmed = true;
		LMIC_OsTime_Stm32L0Lptim_writeCmp((u4_t) target & 0xFFFFu);
		result = true;
		}
	lmic_hal_enableIRQs();
	return result;
	}

/*

Name:	LMIC_OsTime_Stm32L0Lptim_cancelWake()

Function:
	Disarm a wake requested by LMIC_OsTime_Stm32L0Lptim_requestWakeAt().

Definition:
	void LMIC_OsTime_Stm32L0Lptim_cancelWake(
		void
		);

Returns:
	No explicit result.

*/

void LMIC_ABI_STD
LMIC_OsTime_Stm32L0Lptim_cancelWake(
	void
	)
	{
	lmic_hal_disableIRQs();
	if (savedConfig.fWakeArmed)
		{
		savedConfig.fWakeArmed = false;
		LMIC_OsTime_Stm32L0Lptim_writeCmp(LMIC_OsTime_Stm32L0Lptim_CMP_IDLE);
		}
	lmic_hal_enableIRQs();
	}

/*

Name:	LPTIM1_IRQHandler()

Function:
	Advance the bits of the tick count above the 16 that LPTIM1 counts.

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

	u4_t const isr = LPTIM1->ISR;

	if (isr & LPTIM_ISR_ARRM) // the counter reached ARR and wrapped
		{
		// clear the request
		LPTIM1->ICR = LPTIM_ICR_ARRMCF;

		savedConfig.rCntExtended += 0x10000;
		if (savedConfig.rCntExtended == 0)
			++savedConfig.rCntHigh;
		}

	if (isr & LPTIM_ISR_CMPM) // the counter matched CMP (#1120)
		{
		LPTIM1->ICR = LPTIM_ICR_CMPMCF;

		if (savedConfig.fWakeArmed)
			{
			uint64_t const now = LMIC_OsTime_Stm32L0Lptim_ticks64();

			// the compare matches once per wrap; fire only when the
			// full count has reached the target.
			if (now >= savedConfig.wakeTarget)
				{
				savedConfig.fWakeArmed = false;
				LMIC_OsTime_Stm32L0Lptim_writeCmp(LMIC_OsTime_Stm32L0Lptim_CMP_IDLE);
				if (savedConfig.pWakeCb != NULL)
					(*savedConfig.pWakeCb)(savedConfig.pWakeCtx, now);
				}
			}
		}
	}

/// \}

#endif /* defined(LMIC_OsTime_SELECTED_Stm32L0Lptim) */

/**** end of lmic_ostime_stm32l0lptim.c ****/
