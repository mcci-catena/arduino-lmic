/*

Module:  lmic_ostime_micros.cpp

Function:
	The Micros ostime driver: time from the Arduino micros() counter.

Copyright & License:
	See accompanying LICENSE file.

Author:
	Terry Moore, MCCI	September 2026

*/

/// \file

#include "../../i/lmic_ostime_api.h"

#if defined(LMIC_OsTime_SELECTED_Micros)

#include <Arduino.h>

/// \ingroup lmic_ostime_micros
/// \{

/****************************************************************************\
|
|	Manifest constants & typedefs.
|
\****************************************************************************/

/****************************************************************************\
|
|	Variables.
|
\****************************************************************************/

/****************************************************************************\
|
|	Code.
|
\****************************************************************************/

/*

Name:	LMIC_OsTime_Micros_initialize()

Function:
	Prepare the Micros time base for use.

Definition:
	LMIC_OsTime_initialize_fn_t
		LMIC_OsTime_Micros_initialize;

	void LMIC_OsTime_Micros_initialize(
		void
		);

Description:
	This function is called during LMIC startup to initialize
	the time system, as the implementation of LMIC_OsTime_initialize().

	The Arduino micros() function is always initialized prior to
	any code running, so on this platform we need do nothing.

Returns:
	No explicit result.

*/

void LMIC_ABI_STD
LMIC_OsTime_Micros_initialize(
	void
	)
	{
	// no initialization needed.
	}

/*

Name:	LMIC_OsTime_Micros_ticks()

Function:
	Return the current time as a 32-bit tick count.

Definition:
	u4_t LMIC_OsTime_Micros_ticks(
		void
		);

Description:
	This function implements LMIC_OsTime_ticks() using the Arduino
	micros() function.

Returns:
	A time counter in units of LMIC ticks, such that there are
	LMIC_OSTICKS_PER_SEC ticks per real-time second. Accuracy is
	important. If there's more than 0.5% drift, receive timing
	won't meet LoRaWAN requirements.

*/

u4_t LMIC_ABI_STD
LMIC_OsTime_Micros_ticks(
	void
	)
	{
	// double-check the exponent value.
	// 0 leads to correct, but overly complex code (it could just return
	// micros() unmodified), 8 leaves no room for the overlapping bit.
	static_assert(LMIC_OsTime_Micros_US_PER_OSTICK_EXPONENT > 0 && LMIC_OsTime_Micros_US_PER_OSTICK_EXPONENT < 8, "Invalid LMIC_OsTime_Micros_US_PER_OSTICK_EXPONENT value");

	// Because micros() is scaled down in this function, micros() will
	// overflow before the tick timer should, causing the tick timer to
	// miss a significant part of its values if not corrected. To fix
	// this, the "overflow" serves as an overflow area for the micros()
	// counter. It consists of three parts:
	//  - The LMIC_OsTime_Micros_US_PER_OSTICK_EXPONENT upper bits
	//    are effectively an extension for the micros() counter and are
	//    added to the result of this function.
	//  - The next bit overlaps with the most significant bit of
	//    micros(). This is used to detect micros() overflows.
	//  - The remaining bits are always zero.
	//
	// By comparing the overlapping bit with the corresponding bit in
	// the micros() return value, overflows can be detected and the
	// upper bits are incremented. This is done using some clever
	// bitwise operations, to remove the need for comparisons and a
	// jumps, which should result in efficient code. By avoiding shifts
	// other than by multiples of 8 as much as possible, this is also
	// efficient on AVR (which only has 1-bit shifts).
	static uint8_t overflow = 0;

	// Scaled down timestamp. The top LMIC_OsTime_Micros_US_PER_OSTICK_EXPONENT bits are 0,
	// the others will be the lower bits of our return value.
	uint32_t scaled = micros() >> LMIC_OsTime_Micros_US_PER_OSTICK_EXPONENT;
	// Most significant byte of scaled
	uint8_t msb = scaled >> 24;
	// Mask pointing to the overlapping bit in msb and overflow.
	const uint8_t mask = (1 << (7 - LMIC_OsTime_Micros_US_PER_OSTICK_EXPONENT));
	// Update overflow. If the overlapping bit is different
	// between overflow and msb, it is added to the stored value,
	// so the overlapping bit becomes equal again and, if it changed
	// from 1 to 0, the upper bits are incremented.
	overflow += (msb ^ overflow) & mask;

	// Return the scaled value with the upper bits of stored added. The
	// overlapping bit will be equal and the lower bits will be 0, so
	// bitwise or is a no-op for them.
	return scaled | ((uint32_t)overflow << 24);
	}

/// \}

#endif /* defined(LMIC_OsTime_SELECTED_Micros) */

/**** end of lmic_ostime_micros.cpp ****/
