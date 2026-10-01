/*

Module:  arduino_lmic.h

Function:
        Arduino-LMIC C++ top-level include file

Copyright & License:
        See accompanying LICENSE file.

Author:
        Matthijs Kooijman       2015
        Terry Moore, MCCI       November 2018

*/

#pragma once

#ifndef _ARDUINO_LMIC_H_
# define _ARDUINO_LMIC_H_

// Each header wraps its own declarations in LMIC_BEGIN_DECLS/LMIC_END_DECLS.
// There is no extern "C" here: it would put whatever the headers reach,
// Arduino.h among other things, under C linkage (#1102).
#include "lmic/lmic.h"
#include "lmic/lmic_accessors.h"
#include "lmic/lmic_session_state.h"
#include "lmic/lmic_bandplan.h"
#include "lmic/lmic_util.h"

#endif /* _ARDUINO_LMIC_H_ */
