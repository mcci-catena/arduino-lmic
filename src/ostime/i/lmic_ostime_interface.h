/*

Module:  lmic_ostime_interface.h

Function:
	The ostime driver interface types.

Copyright & License:
	See accompanying LICENSE file.

Author:
	Terry Moore, MCCI	September 2026

*/

/// \file

#ifndef _lmic_ostime_interface_h_
#define _lmic_ostime_interface_h_

#ifndef _oslmic_types_h_
# include "../../lmic/oslmic_types.h"
#endif

#ifndef _lmic_env_h_
# include "../../lmic/lmic_env.h"
#endif

/*! \defgroup lmic_ostime LMIC OS Time Interface

\brief This abstract interface represents the system time base to the body of the LMIC.

\details
	Each system using the LMIC has a module that provides this interface.
	The interface is simple: initialization and timer read, a compile-
	time constant set to the number of ticks per microsecond, and interfaces
	equivalent to those provided by lmic_ostime_conv_generic.h.

*/
/// \{

/****************************************************************************\
|
|	Method-name generation.
|
\****************************************************************************/

///
/// \brief Generate a method function name without argument expansion
///
/// \param a_driver	The name of the driver implementing the method function.
/// \param a_fn		The name of the method.
///
/// \details
///	A function name is generated based on the standard pattern for the
///	OsTime driver system, namely LMIC_OsTime_{driver}_{fn}.
///
/// \see LMIC_OsTime_METHOD()
/// \hideinitializer
///
#define LMIC_OsTime_METHOD_(a_driver, a_fn)	\
	LMIC_OsTime_##a_driver##_##a_fn

///
/// \brief Generate a method function name
///
/// \param a_driver
/// \param a_fn
///
/// \details
///
/// \hideinitializer
///
#define LMIC_OsTime_METHOD(a_driver, a_fn)	\
	LMIC_OsTime_METHOD_(a_driver, a_fn)

/****************************************************************************\
|
|	The driver function types.
|
\****************************************************************************/

LMIC_BEGIN_DECLS

///
/// \brief Function type: prepare the time base for use.
///
/// \details
///	Every ostime_t driver supplies a function of this
///	type. The core uses this function to implement
///	LMIC_OsTime_initialize(). The drivers use this
///	type to declare the implementation function to
///	ensure that the types match up.
///
/// \sa LMIC_OsTime_initialize().
///
typedef void LMIC_ABI_STD
LMIC_OsTime_initialize_fn_t(
	void
	);

///
/// \brief Function type: return the current time as a 32-bit tick count.
///
/// \return
///	An unsigned 32-bit count of ticks.
///
/// \details
///	Every ostime_t driver supplies a function of this
///	type. The core uses this function to implement
///	LMIC_OsTime_initialize(). The drivers use this
///	type to declare the implementation function to
///	ensure that the types match up.
///
/// \sa LMIC_OsTime_ticks().
///
typedef u4_t LMIC_ABI_STD
LMIC_OsTime_ticks_fn_t(
	void
	);

LMIC_END_DECLS

///
/// \brief Declare the concrete functions supplied by an ostime driver.
///
/// \param a_driver
///
/// \details
///
/// \hideinitializer
///
#define LMIC_OsTime_DECLARE_DRIVER_FNS(a_driver)			\
	LMIC_BEGIN_DECLS						\
	LMIC_OsTime_initialize_fn_t					\
		LMIC_OsTime_METHOD(a_driver, initialize);		\
	LMIC_OsTime_ticks_fn_t						\
		LMIC_OsTime_METHOD(a_driver, ticks);			\
	LMIC_END_DECLS							\
	struct LMIC_OsTime_DECLARE_DRIVER_FNS_unused_##a_driver

/// \}

#endif /* _lmic_ostime_interface_h_ */

/**** end of lmic_ostime_interface.h ****/
