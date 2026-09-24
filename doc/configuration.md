# Configuration Reference

A number of features can be enabled or disabled at compile time.
This is done by adding the desired settings to the file
`project_config/lmic_project_config.h`. The `project_config`
directory is the only directory that contains files that you
should edit to match your project; we organize things this way
so that your local changes are more clearly separated from
the distribution files. The Arduino environment doesn't give
us a better way to do this, unless you change `BOARDS.txt`.

Unlike other ports of the LMIC code, in this port, you should not edit `src/lmic/config.h` to configure this package. The intention is that you'll edit the `project_config/lmic_project_config.h` (if using the Arduino environment), or change compiler command-line input (if using PlatformIO, make, etc.).

For PlatformIO, the `lmic_project_config.h` has to be disabled with the flag `ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS`. The settings are defined in PlatformIO by `build_flags`. See the [PlatformIO section](../README.md#platformio) of the README for an example.

## Selecting the LoRaWAN Version

This library implements V1.0.3 of the LoRaWAN specification. However, it can also be used with V1.0.2. The only significant change when selecting V1.0.2 is that the US accepted power range in MAC commands is 10 dBm to 30 dBm; whereas in V1.0.3 the accepted range 2 dBm to 30 dBm.

The default LoRaWAN version, if no version is explicitly selected, is V1.0.3.

`LMIC_LORAWAN_SPEC_VERSION` is defined as an integer reflecting the targeted spec version; it will be set to `LMIC_LORAWAN_SPEC_VERSION_1_0_2` or `LMIC_LORAWAN_SPEC_VERSION_1_0_3`. Arithmetic comparisons can be done on these version numbers: and we guarantee `LMIC_LORAWAN_SPEC_VERSION_1_0_3 > LMIC_LORAWAN_SPEC_VERSION_1_0_2`, but the details of the how the versions are encoded may change, and your code should not rely upon the details.

### Selecting V1.0.2

In `project_config/lmic_project_config.h`, add:

```c
#define LMIC_LORAWAN_SPEC_VERSION   LMIC_LORAWAN_SPEC_VERSION_1_0_2
```

On your compiler command line, add:

```shell
-D LMIC_LORAWAN_SPEC_VERSION=LMIC_LORAWAN_SPEC_VERSION_1_0_2
```

### Selecting V1.0.3

In `project_config/lmic_project_config.h`, add:

```c
#define LMIC_LORAWAN_SPEC_VERSION    LMIC_LORAWAN_SPEC_VERSION_1_0_3
```

On your compiler command line, add:

```shell
-D LMIC_LORAWAN_SPEC_VERSION=LMIC_LORAWAN_SPEC_VERSION_1_0_3
```

This is the default.

## Selecting the LoRaWAN Region Configuration

The library supports the following regions:

`-D` variable | CFG region name | CFG region value | LoRaWAN Regional Spec 1.0.3 Reference| Frequency
------------|-----------------|:----------------:|:-------------------:|--------
`-D CFG_eu868` | `LMIC_REGION_eu868` | 1 | 2.2 | EU 863-870 MHz ISM
`-D CFG_us915` | `LMIC_REGION_us915` | 2 | 2.3 | US 902-928 MHz ISM
`-D CFG_au915` | `LMIC_REGION_au915` | 5 | 2.6 | Australia 915-928 MHz ISM
`-D CFG_as923` | `LMIC_REGION_as923` | 7 | 2.8 | Asia 923 MHz ISM
`-D CFG_as923jp` | `LMIC_REGION_as923` and `LMIC_COUNTRY_CODE_JP` | 7 | 2.8 | Asia 923 MHz ISM with Japan listen-before-talk (LBT) rules
`-D CFG_kr920` | `LMIC_REGION_kr920` | 8 | 2.9 | Korea 920-923 MHz ISM
`-D CFG_in866` | `LMIC_REGION_in866` | 9 | 2.10 | India 865-867 MHz ISM

The library requires that the compile environment or the project config file define exactly one of `CFG_...` variables. As released, `project_config/lmic_project_config.h` defines `CFG_us915`.  If you build with PlatformIO or other environments, and you do not provide a pointer to the platform config file, `src/lmic/config.h` will define `CFG_eu868`.

MCCI BSPs add menu entries to the Arduino IDE so you can select the target region interactively.

The library changes configuration pretty substantially according to the region selected, and this affects the symbols in-scope in your sketches and `.cpp` files. Some of the differences are listed below. This list is not comprehensive, and is subject to change in future major releases.

### eu868, as923, in866, kr920

If the library is configured for EU868, AS923, or IN866 operation, we make
the following changes:

- Add the API `LMIC_setupBand()`.
- Add the constants `MAX_CHANNELS`, `MAX_BANDS`, `LIMIT_CHANNELS`, `BAND_MILLI`,
`BAND_CENTI`, `BAND_DECI`, and `BAND_AUX`.

### us915, au915

If the library is configured for US915 operation, we make the following changes:

- Add the APIs `LMIC_enableChannel()`,
`LMIC_enableSubBand()`, `LMIC_disableSubBand()`, and `LMIC_selectSubBand()`.
- Add a number of additional `DR_...` symbols.

## Selecting the target radio transceiver

You should define one of the following variables. If you don't, the library assumes
sx1276. There is a runtime check to make sure the actual transceiver matches the library
configuration.

`#define CFG_sx1272_radio 1`

Configures the library for use with an sx1272 transceiver.

`#define CFG_sx1276_radio 1`

Configures the library for use with an sx1276 transceiver.

`#define CFG_sx1261_radio 1`

Configures the library for use with an sx1261 transceiver.

`#define CFG_sx1262_radio 1`

Configures the library for use with an sx1262 transceiver.

## Controlling use of interrupts

`#define LMIC_USE_INTERRUPTS`

If defined, configures the library to use interrupts for detecting events from the transceiver. If left undefined, the library will poll for events from the transceiver.  See [Timing](timing.md) for more info. Be aware that interrupts are not tested or supported on many platforms.

## Disabling PING

`#define DISABLE_PING`

If defined, removes all code needed for Class B downlink during ping slots (PING).  Removes the APIs `LMIC_setPingable()` and `LMIC_stopPingable()`.
Class A devices don't support PING, so defining `DISABLE_PING` is often a good idea.

By default, PING support is included in the library.

## Disabling Beacons

`#define DISABLE_BEACONS`

If defined, removes all code needed for handling beacons. Removes the APIs `LMIC_enableTracking()` and `LMIC_disableTracking()`.

Enabling beacon handling allows tracking of network time, and is required if you want to enable downlink during ping slots. However, many networks don't support Class B devices. Class A devices don't support tracking beacons, so defining `DISABLE_BEACONS` might be a good idea.

By default, beacon support is included in the library.

## Enabling Class C

`#define LMIC_ENABLE_class_c 1`

If defined to a non-zero value, enables Class C continuous reception support. Class C devices keep the receiver active whenever not transmitting, allowing them to receive downlink messages at any time (not just during the RX1/RX2 windows after an uplink).

When enabled, you must also call `LMIC_enableClassC(1)` at runtime (typically in your `EV_JOINED` event handler) to activate Class C mode. See [CLASS-C.md](CLASS-C.md) for detailed usage information.

By default, Class C support is disabled (`LMIC_ENABLE_class_c` is 0) to minimize code size and RAM usage on constrained devices.

## Enabling/Disabling Network Time Support

`#define LMIC_ENABLE_DeviceTimeReq  number  /* boolean: 0 or non-zero */`

Disable or enable support for device network-time requests (LoRaWAN MAC request 0x0D). If zero, support is disabled. If non-zero, support is enabled.

If disabled, stub routines are provided that will return failure (so you don't need conditional compiles in client code).

By default, device network-time requests were disabled in versions prior to `v4.2.0-pre1`. As of v4.2.0-pre1, the default is that device network-time requests are enabled.

## Rarely changed variables

The remaining variables are rarely used, but we list them here for completeness.

### Changing debug output

`#define LMIC_PRINTF_TO SerialLikeObject`

This variable should be set to the name of a `Serial`-like object (any subclass of Arduino's `Print` class), used for printing messages. If this variable is set, any calls to the standard `printf` function (or more generally all writes to the global `stdout` file descriptor) will redirected to the specified stream.

When this is not defined, `printf` and `stdout` are untouched and their behavior might vary among boards (and could print to somewhere, but also throw away output or crash). So *if* you want to use `printf` or `LMIC_DEBUG_LEVEL`, make sure to also define this.

### Getting debug from the RF library

`#define LMIC_DEBUG_LEVEL number /* 0, 1, or 2 */`

This variable determines the amount of debug output to be produced by the library. The default is `0`.

If `LMIC_DEBUG_LEVEL` is zero, no output is produced. If `1`, limited output is produced. If `2`, more extensive output is produced.

Note that debug output will influence the timing of various parts of the library and could introduce timing problems (especially in the RX window timing), so use it carefully.

Debug output is generated using the standard `printf` function, so unless your environment already redirects `printf` / `stdout` somewhere, you should also configure `LIMC_PRINTF_TO`.

### Selecting the AES library

The library comes with two AES implementations. The original implementation is better on
ARM processors because it's faster, but it's larger. For smaller AVR8 processors, a
second library ("IDEETRON") is provided that has a smaller code footprint.
You may define one of the following variables to choose the AES implementation. If you don't,
the library uses the IDEETRON version.

`#define USE_ORIGINAL_AES`

If defined, the original AES implementation is used.

`#define USE_IDEETRON_AES`

If defined, the IDEETRON AES implementation is used.

### Defining the OS Tick Frequency

`#define US_PER_OSTICK_EXPONENT number`

This variable should be set to the base-2 logarithm of the number of microseconds per OS tick. The default is 4,
which indicates that each tick corresponds to 16 microseconds (because 16 == 2^4).

### Setting the SPI-bus frequency

`#define LMIC_SPI_FREQ floatNumber`

This variable sets the default frequency for the SPI bus connection to the transceiver. The default is `1E6`, meaning 1 MHz. However, this can be overridden by the contents of the `lmic_pinmap` structure, and we recommend that you use that approach rather than editing the `project_config/lmic_project_config.h` file.

### Changing handling of runtime assertion failures

The variables `LMIC_FAILURE_TO` and `DISABLE_LMIC_FAILURE_TO`
control the handling of runtime assertion failures. By default, assertion messages are displayed using
the `Serial` object. You can define LMIC_FAILURE_TO to be the name of some other `Print`-like object. You can
also define `DISABLE_LMIC_FAILURE_TO` to any value, in which case assert failures will silently halt execution.

### Disabling JOIN

`#define DISABLE_JOIN`

If defined, removes code needed for OTAA activation. Removes the APIs `LMIC_startJoining()` and `LMIC_tryRejoin()`.

### Disabling Class A MAC commands

`DISABLE_MCMD_DutyCycleReq`, `DISABLE_MCMD_RXParamSetupReq`, `DISABLE_MCMD_RXTimingSetupReq`, `DISABLE_MCMD_NewChannelReq`, and `DISABLE_MCMD_DlChannelReq` respectively disable code for various Class A MAC commands.

### Disabling Class B MAC commands

`DISABLE_MCMD_PingSlotChannelReq` disables the PING_SET MAC commands. It's implied by `DISABLE_PING`.

`ENABLE_MCMD_BeaconTimingAns` enables the next-beacon start command. It's disabled by default, and overridden (if enabled) by `DISABLE_BEACON`. (This command is deprecated.)

### Disabling user events

Code to handle registered callbacks for transmit, receive, and events can be suppressed by setting `LMIC_ENABLE_user_events` to zero.  This C preprocessor macro is always defined as a post-condition of `#include "config.h"`; if non-zero, user events are supported, if zero, user events are not-supported.  The default is to support user events.

### Disabling external reference to `onEvent()`

In V3 of the LMIC, you do not need to define a function named `onEvent`. The LMIC will notice that there's no such function, and will suppress the call. However, be cautious -- in a large software package, `onEvent()` may be defined for some other purpose. The LMIC has no way of knowing that this is not the LMIC's `onEvent`, so it will call the function, and this may cause problems.

All reference to `onEvent()` can be suppressed by setting `LMIC_ENABLE_onEvent` to 0. This C preprocessor macro is always defined as a post-condition of `#include "config.h"`; if non-zero, a weak reference to `onEvent()` will be used; if zero, the user `onEvent()` function is not supported, and the client must register an event handler explicitly.  See the PDF documentation for details on `LMIC_registerEventCb()`.

### Enabling long messages

By default, LMIC allows messages up to 255 bytes, as defined in the LoRaWAN standard and required by compliance testing. To save RAM for simple devices, this can be limited using the `LMIC_MAX_FRAME_LENGTH` macro. This macro defines the length of the full frame, the maximum payload size is a bit smaller (and can be read from the `MAX_LEN_PAYLOAD` constant).

This value controls both the TX and RX buffers, so reducing it by 1 saves 2 bytes of RAM. The value should be not be set too small, since that can prevent properly receiving network downlinks (e.g. join accepts or MAC commands). Using `#define LMIC_MAX_FRAME_LENGTH 64` is common and should be big enough for most operation, while saving 384 bytes of RAM.

Originally, this was configured using the `LMIC_ENABLE_long_messages` macro, which is still supported for compatibility. Setting `LMIC_ENABLE_long_messages` to 0 is equivalent to setting `LMIC_MAX_FRAME_LENGTH` to 64.

### Enabling LMIC event logging calls

When debugging the LMIC, debug prints change timing, and can make things not work at all. The LMIC has embedded optional calls to capture debug information that can be printed out later, when the LMIC is not active. Logging is enabled by setting `LMIC_ENABLE_event_logging` to 1. The default is not to log. This C preprocessor macro is always defined as a post-condition of `#include "config.h"`.

The compliance test script includes a suitable logging implementation; the other example scripts do not.

### Special purpose

`#define DISABLE_INVERT_IQ_ON_RX` disables the inverted Q-I polarity on RX. **Use of this variable is deprecated, see issue [#250](https://github.com/mcci-catena/arduino-lmic/issues/250).** Rather than defining this, set the value of `LMIC.noRXIQinversion`. If set non-zero, receive will be non-inverted. End-devices will be able to receive messages from each other, but will not be able to hear the gateway (other than Class B beacons)aa. If set zero, (the default), end devices will only be able to hear gateways, not each other.
