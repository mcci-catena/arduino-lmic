/*

Module:  lmic_env.h

Function:
	Sets up macros etc. to make things a little easier for portabilty

Copyright notice and license info:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	November 2018

Description:
	This file is an adaptation of MCCI's standard IOCTL framework.
	We duplicate a bit of functionality that we might get from other
	libraries, so that the LMIC library can continue to stand alone.

*/

#ifndef _lmic_env_h_	/* prevent multiple includes */
#define _lmic_env_h_

/*

Macro:	LMIC_C_ASSERT()

Function:
	Declaration-like macro that will cause a compile error if arg is FALSE.

Definition:
	LMIC_C_ASSERT(
		BOOL fErrorIfFalse
		);

Description:
	This macro, if used where an external reference declarataion is
	permitted, will either compile cleanly, or will cause a compilation
	error. The results of using this macro where a declaration is not
	permitted are unspecified.

	This is different from #if !(fErrorIfFalse) / #error in that the
	expression is evaluated by the compiler rather than by the pre-
	processor. Therefore things like sizeof() can be used.

Returns:
	No explicit result -- either compiles cleanly or causes a compile
	error.

*/

#ifndef LMIC_C_ASSERT
# define LMIC_C_ASSERT(e)	\
 void  LMIC_C_ASSERT__(int LMIC_C_ASSERT_x[(e) ? 1: -1])
#endif

/****************************************************************************\
|
|	Define the begin/end declaration tags for C++ co-existance
|
\****************************************************************************/

#ifdef __cplusplus
# define LMIC_BEGIN_DECLS	extern "C" {
# define LMIC_END_DECLS	}
#else
# define LMIC_BEGIN_DECLS	/* nothing */
# define LMIC_END_DECLS	/* nothing */
#endif

//----------------------------------------------------------------------------
// Annotations to avoid various "unused" warnings. These must appear as a
// statement in the function body; the macro annotates the variable to quiet
// compiler warnings.  The way this is done is compiler-specific, and so these
// definitions are fall-backs, which might be overridden.
//
// Although these are all similar, we don't want extra macro expansions,
// so we define each one explicitly rather than relying on a common macro.
//----------------------------------------------------------------------------

// signal that a parameter is intentionally unused.
#ifndef LMIC_UNREFERENCED_PARAMETER
# define LMIC_UNREFERENCED_PARAMETER(v)      do { (void) (v); } while (0)
#endif

// an API parameter is a parameter that is required by an API definition, but
// happens to be unreferenced in this implementation. This is a stronger
// assertion than LMIC_UNREFERENCED_PARAMETER(): this parameter is here
// becuase of an API contract, but we have no use for it in this function.
#ifndef LMIC_API_PARAMETER
# define LMIC_API_PARAMETER(v)               do { (void) (v); } while (0)
#endif

// an intentionally-unreferenced variable.
#ifndef LMIC_UNREFERENCED_VARIABLE
# define LMIC_UNREFERENCED_VARIABLE(v)       do { (void) (v); } while (0)
#endif

// we have three (!) debug levels (LMIC_DEBUG_LEVEL > 0, LMIC_DEBUG_LEVEL > 1,
// and LMIC_X_DEBUG_LEVEL > 0. In each case we might have parameters or
// or varables that are only refereneced at the target debug level.

// Parameter referenced only if debugging at level > 0.
#ifndef LMIC_DEBUG1_PARAMETER
# if LMIC_DEBUG_LEVEL > 0
#  define LMIC_DEBUG1_PARAMETER(v)           do { ; } while (0)
# else
#  define LMIC_DEBUG1_PARAMETER(v)           do { (void) (v); } while (0)
# endif
#endif

// variable referenced only if debugging at level > 0
#ifndef LMIC_DEBUG1_VARIABLE
# if LMIC_DEBUG_LEVEL > 0
#  define LMIC_DEBUG1_VARIABLE(v)            do { ; } while (0)
# else
#  define LMIC_DEBUG1_VARIABLE(v)            do { (void) (v); } while (0)
# endif
#endif

// parameter referenced only if debugging at level > 1
#ifndef LMIC_DEBUG2_PARAMETER
# if LMIC_DEBUG_LEVEL > 1
#  define LMIC_DEBUG2_PARAMETER(v)           do { ; } while (0)
# else
#  define LMIC_DEBUG2_PARAMETER(v)           do { (void) (v); } while (0)
# endif
#endif

// variable referenced only if debugging at level > 1
#ifndef LMIC_DEBUG2_VARIABLE
# if LMIC_DEBUG_LEVEL > 1
#  define LMIC_DEBUG2_VARIABLE(v)            do { ; } while (0)
# else
#  define LMIC_DEBUG2_VARIABLE(v)            do { (void) (v); } while (0)
# endif
#endif

// parameter referenced only if LMIC_X_DEBUG_LEVEL > 0
#ifndef LMIC_X_DEBUG_PARAMETER
# if LMIC_X_DEBUG_LEVEL > 0
#  define LMIC_X_DEBUG_PARAMETER(v)           do { ; } while (0)
# else
#  define LMIC_X_DEBUG_PARAMETER(v)           do { (void) (v); } while (0)
# endif
#endif

// variable referenced only if LMIC_X_DEBUG_LEVEL > 0
#ifndef LMIC_X_DEBUG_VARIABLE
# if LMIC_X_DEBUG_LEVEL > 0
#  define LMIC_X_DEBUG_VARIABLE(v)            do { ; } while (0)
# else
#  define LMIC_X_DEBUG_VARIABLE(v)            do { (void) (v); } while (0)
# endif
#endif

// parameter referenced only if EV() macro is enabled (which it never is)
// TODO(tmm@mcci.com) take out the EV() framework as it requires C++, and
// this code is really C-99 to its bones.
#ifndef LMIC_EV_PARAMETER
# define LMIC_EV_PARAMETER(v)                 do { (void) (v); } while (0)
#endif

// variable referenced only if EV() macro is defined.
#ifndef LMIC_EV_VARIABLE
# define LMIC_EV_VARIABLE(v)                  do { (void) (v); } while (0)
#endif

/*

Macro:	LMIC_ABI_STD

Index:	Macro:	LMIC_ABI_VARARGS

Function:
	Annotation macros to force a particular binary calling sequence.

Definition:
	#define LMIC_ABI_STD		compiler-specific
	#define	LMIC_ABI_VARARGS 	compiler-specific

Description:
	These macros are used when declaring a function type, and indicate
	that a particular calling sequence is to be used. They are normally
	used between the type portion of the function declaration and the
	name of the function.  For example:

	typedef void LMIC_ABI_STD myCallBack_t(void);

	It's important to use this in libraries on platforms with multiple
	calling sequences, because different components can be compiled with
	different defaults.

Returns:
	Not applicable.

*/

/* ABI marker for normal (fixed parameter count) functions -- used for function types */
#ifndef LMIC_ABI_STD
# ifdef _MSC_VER
#  define LMIC_ABI_STD	__stdcall
# else
#  define LMIC_ABI_STD	/* nothing */
# endif
#endif

/* ABI marker for VARARG functions -- used for function types */
#ifndef LMIC_ABI_VARARGS
# ifdef _MSC_VER
#  define LMIC_ABI_VARARGS	__cdecl
# else
#  define LMIC_ABI_VARARGS	/* nothing */
# endif
#endif

/*

Macro:	LMIC_DECLARE_FUNCTION_WEAK()

Function:
	Declare an external function as a weak reference.

Definition:
	#define LMIC_DECLARE_FUNCTION_WEAK(ReturnType, FunctionName, Params) ...

Description:
	This macro generates a weak reference to the specified function.

Example:
	LMIC_DECLARE_FUNCTION_WEAK(void, onEvent, (ev_t e));

	This saya that onEvent is a weak external reference. When calling
	onEvent, you must always first check whether it's supplied:

	if (onEvent != NULL)
		onEvent(e);

Returns:
	This macro expands to a declaration, without a trailing semicolon.

Notes:
	This form allows for compilers that use _Pragma(weak, name) instead
	of inline attributes.

*/

#define LMIC_DECLARE_FUNCTION_WEAK(a_ReturnType, a_FunctionName, a_Params)	\
	a_ReturnType __attribute__((__weak__)) a_FunctionName a_Params

/*

Macro:	ARDUINO_LMIC_VERSION_CALC()

Index:	Macro:	ARDUINO_LMIC_VERSION_GET_MAJOR()
	Macro:	ARDUINO_LMIC_VERSION_GET_MINOR()
	Macro:	ARDUINO_LMIC_VERSION_GET_PATCH()
	Macro:	ARDUINO_LMIC_VERSION_GET_LOCAL()
	Macro:	ARDUINO_LMIC_VERSION_TO_ORDINAL()
	Macro:	ARDUINO_LMIC_VERSION_COMPARE_LT()
	Macro:	ARDUINO_LMIC_VERSION_COMPARE_LE()
	Macro:	ARDUINO_LMIC_VERSION_COMPARE_GT()
	Macro:	ARDUINO_LMIC_VERSION_COMPARE_GE()

Function:
	LMIC semantic version calculations.

Definition:
	#define ARDUINO_LMIC_VERSION_CALC(major, minor, patch, local) ...
	#define ARDUINO_LMIC_VERSION_GET_MAJOR(version_uint32) ...
	#define ARDUINO_LMIC_VERSION_GET_MINOR(version_uint32) ...
	#define ARDUINO_LMIC_VERSION_GET_PATCH(version_uint32) ...
	#define	ARDUINO_LMIC_VERSION_GET_LOCAL(version_uint32) ...
	#define ARDUINO_LMIC_VERSION_TO_ORDINAL(version_uint32) ...
	#define	ARDUINO_LMIC_VERSION_COMPARE_LT(version1, version2) ...
	#define	ARDUINO_LMIC_VERSION_COMPARE_LE(version1, version2) ...
	#define	ARDUINO_LMIC_VERSION_COMPARE_GT(version1, version2) ...
	#define	ARDUINO_LMIC_VERSION_COMPARE_GE(version1, version2) ...

Description:
	These macros are used for creating and manipulating semantic version
	constants.

	SemanticVersions according to https://semver.org/v2 have
	up to four parts: major, minor, patch, and pre-release. If a semantic
	version string has a pre-release, it sorts before the equivalent
	version string without a pre-release; otherwise version strings sort
	lexicographically.

	To make compile time operations easier, we limit the four fields to
	eight bits. We use `LOCAL` for the pre-release number; if non-zero,
	the version is a pre-release.

	To avoid confusion, we represent the version fields in a uint32_t,
	exactly as given. However, this means that the versions can't
	be sorted directly because the 32-bit numbers corresponding to
	pre-releases are greater than the 32-bit number representing the
	final release.

	To ease comparisons, ARDUINO_LMIC_VERSION_TO_ORDINAL() turns a
	direct representation of the four fields into a uint32_t which
	can be compared to any other version ordinal using normal integer
	comparisons. The four comparison macros use this macro to
	return a boolean result.

	All these macros can be used at compile time.

Returns:
	ARDUINO_LMIC_VERSION_CALC() returns a 32-bit version number.
	ARDUINO_LMIC_VERSION_GET_MAJOR(), MINOR(), PATCH(), and LOCAL()
	return an 8-bit number extracted from the corresponding field.
	ARDUINO_LMIC_VERSION_TO_ORDINAL() returns a 32-bit ordinal.
	ARDUINO_LMIC_VERSION_COMPARE_LT(), LE(), GT(), GE() return
	booleans.

Notes:
	In most other MCCI packages, ARDUINO_LMIC_VERSION_GET_LOCAL()
	would be called ARDUINO_LMIC_VERSION_GET_PRE().

	The standard way to format versions is:

		{major}.{minor}.{patch}[-pre{pre}]

*/

/// \brief generate version uint32_t from components.
#define ARDUINO_LMIC_VERSION_CALC(major, minor, patch, local)	\
	((((major)*UINT32_C(1)) << 24) | (((minor)*UINT32_C(1)) << 16) | (((patch)*UINT32_C(1)) << 8) | (((local)*UINT32_C(1)) << 0))

/// \brief extract major field from version uint32_t
#define	ARDUINO_LMIC_VERSION_GET_MAJOR(v)	\
	((((v)*UINT32_C(1)) >> 24u) & 0xFFu)

/// \brief extract minor field from version uint32_t
#define	ARDUINO_LMIC_VERSION_GET_MINOR(v)	\
	((((v)*UINT32_C(1)) >> 16u) & 0xFFu)

/// \brief extract patch field from version uint32_t
#define	ARDUINO_LMIC_VERSION_GET_PATCH(v)	\
	((((v)*UINT32_C(1)) >> 8u) & 0xFFu)

/// \brief extract pre-release field from version uint32_t
#define	ARDUINO_LMIC_VERSION_GET_LOCAL(v)	\
	((v) & 0xFFu)

/// \brief convert a semantic version to an ordinal integer.
#define ARDUINO_LMIC_VERSION_TO_ORDINAL(v)  \
        (((v) & 0xFFFFFF00u) | (((v) - 1) & 0xFFu))

/// \brief compare two semantic versions
/// \return \c true if \p a is less than \p b (as a semantic version).
#define ARDUINO_LMIC_VERSION_COMPARE_LT(a, b)   \
        (ARDUINO_LMIC_VERSION_TO_ORDINAL(a) < ARDUINO_LMIC_VERSION_TO_ORDINAL(b))

/// \brief compare two semantic versions
/// \return \c true if \p a is less than or equal to \p b (as a semantic version).
#define ARDUINO_LMIC_VERSION_COMPARE_LE(a, b)   \
        (ARDUINO_LMIC_VERSION_TO_ORDINAL(a) <= ARDUINO_LMIC_VERSION_TO_ORDINAL(b))

/// \brief compare two semantic versions
/// \return \c true if \p a is greater than \p b (as a semantic version).
#define ARDUINO_LMIC_VERSION_COMPARE_GT(a, b)   \
        (ARDUINO_LMIC_VERSION_TO_ORDINAL(a) > ARDUINO_LMIC_VERSION_TO_ORDINAL(b))

/// \brief compare two semantic versions
/// \return \c true if \p a is greater than or equal to \p b (as a semantic version).
#define ARDUINO_LMIC_VERSION_COMPARE_GE(a, b)   \
        (ARDUINO_LMIC_VERSION_TO_ORDINAL(a) >= ARDUINO_LMIC_VERSION_TO_ORDINAL(b))

/*

Macro:	LMIC_STRINGIFY()

Index:	Macro:	LMIC_STRINGIFY_()

Function:
	Utility facility to convert values to C strings.

Definition:
	#define LMIC_STRINGIFY(x) ...
	#define LMIC_STRINGIFY_(inner_x) ...

Description:
	These macros are used as wrappers for the C preprocessor unary
	`#` operator. #x only works in a macro, so in normal situations,
	a helper macro is needed. In the general case where your value
	might be or might contain macros that need to be expanded,
	LMIC_STRINGIFY() is used. If you need to have the literal value
	of the parameter converted to a string without macro expansion,
	LMIC_STRINGIFY_() is used.

Returns:
	A C string value.

*/

///
/// \brief helper macro for LMIC_STRINGIFY()
///
/// \param inner_x 	is the value to be converted to a string. It is not macro
///			expanded before conversion.
///
#define LMIC_STRINGIFY_(inner_x) #inner_x

///
/// \brief macro-expand and convert value to C string (in double quotes)
///
/// \param x is the value to be converted to a string. It will be macro expanded first.
///
/// \details
///	This relies on obscure details of how macro argument expansion works. If we
///	expanded here with `#x`, LMIC_STRINGIFY(__LINE__) would return literally
///	`"__LINE__"`. But when we go to call the second macro LMIC_STRINGIFY_(),
///	the preprocessor expands __LINE__ (to 386 if that's the line number) and
///	then invokes the second macro. LMIC_STRINGIFY_ doesn't try to expand its
///	parameter, and so the result is "386".
///
#define LMIC_STRINGIFY(x) LMIC_STRINGIFY_(x)

#endif /* _lmic_env_h_ */
