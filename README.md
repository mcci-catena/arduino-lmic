# Arduino-LMIC library ("MCCI LoRaWAN LMIC Library")

[![GitHub release](https://img.shields.io/github/release/mcci-catena/arduino-lmic.svg)](https://github.com/mcci-catena/arduino-lmic/releases/latest) [![GitHub commits](https://img.shields.io/github/commits-since/mcci-catena/arduino-lmic/latest.svg)](https://github.com/mcci-catena/arduino-lmic/compare/v6.0.1...master) [![Arduino CI](https://img.shields.io/github/actions/workflow/status/mcci-catena/arduino-lmic/ci-arduinocli.yml?branch=master)](https://github.com/mcci-catena/arduino-lmic/actions)

[![API Documentation](https://img.shields.io/badge/docs-GitHub%20Pages-blue)](https://mcci-catena.github.io/arduino-lmic/)

**Contents:**

<!-- markdownlint-disable MD033 MD004 -->
<!-- markdownlint-capture -->
<!-- markdownlint-disable -->
<!-- TOC depthFrom:2 updateOnSave:true -->

- [Introduction](#introduction)
- [Installing](#installing)
- [Getting Help](#getting-help)
- [Features](#features)
- [Documentation](#documentation)
- [Known bugs and issues](#known-bugs-and-issues)
- [Configuration](#configuration)
- [Supported hardware](#supported-hardware)
- [Pre-Integrated Boards](#pre-integrated-boards)
- [PlatformIO](#platformio)
- [Example Sketches](#example-sketches)
- [Timing](#timing)
- [Downlink data rate](#downlink-data-rate)
- [Encoding Utilities](#encoding-utilities)
- [Release History](#release-history)
- [Contributions](#contributions)
- [Trademark Acknowledgements](#trademark-acknowledgements)
- [License](#license)

<!-- /TOC -->
<!-- markdownlint-restore -->

## Introduction

The "Arduino LMIC" repository contains the IBM LMIC (LoRaWAN-MAC-in-C) library, slightly
modified to run in the Arduino environment, allowing using the SX1272,
SX1276, SX126x transceivers and compatible modules (such as some HopeRF RFM9x
modules and the Murata ABZ and 1SJ LoRa modules).

Information about the LoRaWAN protocol is summarized in [LoRaWAN-at-a-glance](doc/LoRaWAN-at-a-glance.pdf). Full information is available from the [LoRa Alliance](https://lora-alliance.org).

For support, please open an entry in the [GitHub Discussions](https://github.com/mcci-catena/arduino-lmic/discussions) tab of the LMIC page on github.

The base Arduino library mostly exposes the functions defined by LMIC. It makes no
attempt to wrap them in a higher level API that is more in the Arduino
style. To find out how to use the library itself, see the examples, or
see the PDF files in the doc subdirectory.

A separate library, [MCCI `arduino-lorawan`](https://github.com/mcci-catena/arduino-lorawan), provides a higher level, more Arduino-like wrapper which may be useful.

The examples in this library (apart from the compliance sketch) are somewhat primitive. A very complete cross-platform Arduino application based on the LMIC has been published by Leonel Lopes Parente ([`@lnlp`](https://github.com/lnlp)) as [LMIC-node](https://github.com/lnlp/LMIC-node). That application specifically targets The Things Network.

Although the wrappers in this library are designed to make the LMIC useful in the Arduino environment, the maintainers have tried to be careful to keep the core LMIC code generally useful. For example, I use this library without modification (but with wrappers) on a RISC-V platform in a non-Arduino environment.

> Note on names: the library was originally ported to Arduino by Matthijs Kooijman and Thomas Telkamp, and was named Arduino LMIC. Subsequently, MCCI did a lot of work to support other regions, and ultimately took over maintenance. The Arduino IDE doesn't like two libraries with the same name, so we had to come up with a new name. In the IDE, this library appears as "MCCI LoRaWAN LMIC Library"; but we all know it by its primary header file, which is `<arduino_lmic.h>`.

## Installing

To install this library:

- install it using the Arduino Library manager ("Sketch" -> "Include
   Library" -> "Manage Libraries..."), or
- download a zip file from GitHub using the "Download ZIP" button and
   install it using the IDE ("Sketch" -> "Include Library" -> "Add .ZIP
   Library..."
- clone this git repository into your sketchbook/libraries folder.

For more info, see [https://www.arduino.cc/en/Guide/Libraries](https://www.arduino.cc/en/Guide/Libraries).

## Getting Help

### If it's not working

Ask questions in the [GitHub Discussions](https://github.com/mcci-catena/arduino-lmic/discussions) tab of the LMIC page on github. Wireless is tricky, so don't be afraid to ask. The LMIC has been used successfully in a lot of applications, but it's common to have problems getting it working. To keep the code size down, there are not a lot of debugging features, and the features are not always easy to use.

### If you've found a bug

Raise a GitHub issue at [`github.com/mcci-catena/arduino-lmic`](https://github.com/mcci-catena/arduino-lmic/issues/).

## Features

The LMIC library provides a fairly complete LoRaWAN Class A, Class B, and Class C
implementation, supporting the EU-868, US-915, AU-921, AS-923, and IN-866 bands. Only a limited
number of features was tested using this port on Arduino hardware, so be careful when using any of the untested features.

The library has only been tested with LoRaWAN 1.0.2/1.03 networks and does not have the separated key structure defined by LoRaWAN 1.1.

What certainly works:

- Sending packets uplink, taking into account duty cycling.
- Encryption and message integrity checking.
- Custom frequencies and data rate settings.
- Over-the-air activation (OTAA / joining).
- Receiving downlink packets in the RX1 and RX2 windows.
- MAC command processing.
- Class C continuous reception (must be explicitly enabled at compile time and runtime).

What has not been tested:

- Class B operation.
- FSK has not been extensively tested. (Testing with the RedwoodComm RWC5020A analyzer in 2019 indicated that FSK downlink is stable but not reliable. This prevents successful completion of LoRaWAN pre-certification in regions that require support for FSK.)

If you try one of these untested features and it works, be sure to let
us know (creating a GitHub issue is probably the best way for that).

## Documentation

Browsable documentation (generated by Doxygen) is available at the [GitHub Pages site](https://mcci-catena.github.io/arduino-lmic/).

The [`doc/`](doc/) directory contains the full documentation set. Key references:

- [Configuration Reference](doc/configuration.md) -- all compile-time settings (region, radio, LoRaWAN version, `LMIC_ENABLE_*`/`DISABLE_*` macros)
- [Class C Guide](doc/CLASS-C.md) -- continuous receive mode configuration and API
- [Hardware Configuration](doc/HOWTO-Manually-Configure.md) -- wiring and pin mapping for non-pre-integrated boards
- [Timing](doc/timing.md) -- protocol timing, clock error, and interrupt handling
- [Encoding Utilities](doc/encoding-utilities.md) -- `sflt16`, `uflt16`, `sflt12`, `uflt12` formats with JavaScript decoders
- [Radio Driver](doc/RadioDriver.md) -- radio driver interface specification
- [Adding Regions](doc/HOWTO-ADD-REGION.md) -- step-by-step guide for new regional configurations
- [API Reference PDF](doc/LMIC-v5.0.0.pdf) -- based on the original IBM documentation (v6 update pending)

See [`doc/README.md`](doc/README.md) for the complete documentation index.

## Known bugs and issues

See the list of bugs at [`mcci-catena/arduino-lmic`](https://github.com/mcci-catena/arduino-lmic/issues).

The LoRaWAN technology for class A devices requires devices to meet hard real-time deadlines. The Arduino environment doesn't provide built-in support for this, and this port of the LMIC doesn't really ensure it, either. It is your responsibility, when constructing your application, to ensure that you call `os_runloop_once()` "often enough". The API `os_queryTimeCriticalJobs(ms2osticks(n))` can be used to check whether there are any deadlines due within the next `n` milliseconds. See [Timing](doc/timing.md) for details.

The Board Support Package V2.5.0 for the MCCI Murata-based boards ([MCCI Catena 4610](https://mcci.io/catena4610), [MCCI Catena 4612](https://mcci.io/catena4612), etc.) has a defect in clock calibration that prevents the compliance script from being used without modification. Versions V2.6.0 and later solve this issue.

## Configuration

Configuration is done by adding settings to `project_config/lmic_project_config.h`. The two settings every project must define are the **region** and the **radio transceiver**.

### Selecting the region

Define exactly one of the following in your project config:

`-D` variable | Frequency
------------|--------
`-D CFG_eu868` | EU 863-870 MHz ISM
`-D CFG_us915` | US 902-928 MHz ISM
`-D CFG_au915` | Australia 915-928 MHz ISM
`-D CFG_as923` | Asia 923 MHz ISM
`-D CFG_as923jp` | Asia 923 MHz ISM with Japan LBT
`-D CFG_kr920` | Korea 920-923 MHz ISM
`-D CFG_in866` | India 865-867 MHz ISM

As released, `project_config/lmic_project_config.h` defines `CFG_us915`.

### Selecting the radio transceiver

Define exactly one of:

- `#define CFG_sx1272_radio 1`
- `#define CFG_sx1276_radio 1`
- `#define CFG_sx1261_radio 1`
- `#define CFG_sx1262_radio 1`

If you don't define one, the library assumes SX1276.

For the full configuration reference (LoRaWAN version, interrupts, Class C, debug output, AES library, and all other compile-time options), see [Configuration Reference](doc/configuration.md).

## Supported hardware

This library is intended to be used with plain LoRa transceivers,
connected to the Arduino CPU using a SPI bus. In particular:

* The SX1272, SX1276
families are supported (which should include SX1273, SX1277, SX1278 and
SX1279 which only differ in the available frequencies, bandwidths and
spreading factors). It has been tested with both SX1272 and SX1276
chips on a variety of platforms.

* The SX1261 and SX1262 families are supported. This has been tested with Heltec and TTGo boards.

This library contains a full LoRaWAN stack and is intended to drive
these Transceivers directly. It is *not* intended to be used with
full-stack devices like the Microchip RN2483 and the Embit LR1272E.
These contain a transceiver and microcontroller that implements the
LoRaWAN stack and exposes a high-level serial interface instead of the
low-level SPI transceiver interface.

This library is intended to be used inside the Arduino environment. It
should be architecture-independent. Users have tested this on AVR, ARM, Xtensa-based, ESP32, and RISC-V based systems.

This library can be quite heavy on 8-bit systems, especially if the fairly small ATmega
328p (such as in the Arduino Uno) is used. In the default configuration,
the available 32K flash space is nearly filled up (this includes some
debug output overhead, though). By disabling some features in `project_config/lmic_project_config.h`
(like beacon tracking and ping slots, which are not needed for Class A devices),
some space can be freed up.

## Pre-Integrated Boards

There are two ways of using this library, either with pre-integrated boards or with manually configured boards.

The following boards are pre-integrated.

- Adafruit [Feather 32u4 LoRa 900 MHz][1] (SX1276)
- Adafruit [Feather M0 LoRa 900 MHz][2] (SX1276)
- MCCI Catena 4410, 4420, [4450][3], [4460][4] and [4470][5] boards (based on Adafruit Feather boards plus wings) (SX1276)
- MCCI Catena 4551, [4610][6], 4611, [4612][7], 4617, [4618][7a], 4630, [4801][8] and 4802[12] boards (based on the Murata CMWX1ZZABZ-078 module) (SX1276)
- [TTGo LoRa32 V1][10] (based on the ESP32)
- [Heltec WiFi LoRa 32 V2][11] (based on the ESP32)

[1]: https://www.adafruit.com/products/3078
[2]: https://www.adafruit.com/products/3178
[3]: https://store.mcci.com/collections/lorawan-iot-and-the-things-network/products/catena-4450-lorawan-iot-device
[4]: https://store.mcci.com/collections/lorawan-iot-and-the-things-network/products/catena-4460-sensor-wing-w-bme680
[5]: https://store.mcci.com/collections/lorawan-iot-and-the-things-network/products/mcci-catena-4470-modbus-node-for-lorawan-technology
[6]: https://store.mcci.com/collections/lorawan-iot-and-the-things-network/products/mcci-catena-4610-integrated-node-for-lorawan-technology
[7]: https://store.mcci.com/collections/lorawan-iot-and-the-things-network/products/catena-4612-integrated-lorawan-node
[7a]: https://mcci.com/lorawan/products/catena-4618/
[8]: https://store.mcci.com/collections/lorawan-iot-and-the-things-network/products/catena-4801
[10]: https://makeradvisor.com/tools/ttgo-lora32-sx1276-esp32-oled/
[11]: https://heltec.org/project/wifi-lora-32/
[12]: https://store.mcci.com/collections/lorawan-iot-and-the-things-network/products/catena-4802

> To help you know if you have to worry, we'll call such boards "pre-integrated" and prefix each section with suitable guidance.

If your board is not pre-integrated, refer to [`HOWTO-Manually-Configure.md`](doc/HOWTO-Manually-Configure.md).

## PlatformIO

For use with PlatformIO, the `lmic_project_config.h` has to be disabled with the flag `ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS`.
The settings are defined in PlatformIO by `build_flags`.

```ini
lib_deps =
    MCCI LoRaWAN LMIC library

build_flags =
    -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS
    -D CFG_eu868=1
    -D CFG_sx1276_radio=1
```

## Example Sketches

This library provides several examples.

- [`ttn-otaa.ino`](examples/ttn-otaa/ttn-otaa.ino) -- basic OTAA transmission. Tested with The Things Network.
- [`ttn-otaa-feather-us915.ino`](examples/ttn-otaa-feather-us915/ttn-otaa-feather-us915.ino) -- OTAA configured for Feather M0 LoRa, US915, TTN.
- [`ttn-otaa-halconfig-us915.ino`](examples/ttn-otaa-halconfig-us915/ttn-otaa-halconfig-us915.ino) -- OTAA using automatic HAL configuration, US915, TTN.
- [`ttn-otaa-feather-us915-dht22.ino`](examples/ttn-otaa-feather-us915-dht22/ttn-otaa-feather-us915-dht22.ino) -- OTAA with DHT22 temperature/humidity sensor.
- [`ttn-otaa-sx1262.ino`](examples/ttn-otaa-sx1262/ttn-otaa-sx1262.ino) -- OTAA demonstrating SX126x advanced initialization.
- [`raw.ino`](examples/raw/raw.ino) -- low-level non-LoRaWAN packet transmission for basic connectivity testing.
- [`raw-feather.ino`](examples/raw-feather/raw-feather.ino) -- `raw.ino` configured for Feather M0 LoRa.
- [`raw-halconfig.ino`](examples/raw-halconfig/raw-halconfig.ino) -- `raw.ino` using automatic HAL configuration.
- [`ttn-abp.ino`](examples/ttn-abp/ttn-abp.ino) -- activation by personalization (ABP). ABP should not be used with production networks; use OTAA instead.
- [`ttn-abp-feather-us915-dht22.ino`](examples/ttn-abp-feather-us915-dht22/ttn-abp-feather-us915-dht22.ino) -- ABP with DHT22 sensor, Feather M0, US915.
- [`header_test.ino`](examples/header_test/header_test.ino) -- header file regression test.
- [`compliance-otaa-halconfig.ino`](examples/compliance-otaa-halconfig/compliance-otaa-halconfig.ino) -- LoRaWAN compliance testing sketch.
- [`helium-otaa.ino`](examples/helium-otaa/helium-otaa.ino) -- OTAA configured for the Helium network.
- [`ttn-otaa-network-time.ino`](examples/ttn-otaa-network-time/ttn-otaa-network-time.ino) -- demonstrates network time requests.

## Timing

The library must track precise timing for opening RX1 and RX2 receive windows after uplink transmissions. Key points:

- Call `os_runloop_once()` frequently, especially during transmit/receive cycles. Use `os_queryTimeCriticalJobs(ms2osticks(n))` to check if the LMIC needs attention within `n` milliseconds.
- By default, the library polls DIO pins. Optionally, define `LMIC_USE_INTERRUPTS` for hardware interrupt-driven timing (requires DIO pins wired to interrupt-capable Arduino pins).
- For boards with inaccurate oscillators (100-4000 ppm), call `LMIC_setClockError()` to widen the receive window. See [Timing](doc/timing.md) for the full discussion including `LMIC_setClockError()` API reference and interrupt implementation details.

## Downlink data rate

Note that the data rate used for downlink packets in the RX2 window varies by region. Consult your network's manual for any divergences from the LoRaWAN Regional Parameters. This library assumes that the network follows the regional default.

Some networks use different values than the specification. For example, in Europe, the specification default is DR0 (SF12, 125 kHz bandwidth). However, iot.semtech.com and The Things Network both used SF9 / 125 kHz or DR3). If using over-the-air activation (OTAA), the network will download RX2 parameters as part of the JoinAccept message; the LMIC will honor the downloaded parameters.

However, when using personalized activate (ABP), it is your
responsibility to set the right settings, e.g. by adding this to your
sketch (after calling `LMIC_setSession`). `ttn-abp.ino` already does
this.

```c++
LMIC.dn2Dr = DR_SF9;
```

## Encoding Utilities

See [Encoding Utilities](doc/encoding-utilities.md) for `sflt16`, `uflt16`, `sflt12`, `uflt12` format descriptions and JavaScript decoder functions.

## Release History

Current version: **6.0.0** -- adds Class C support, secure element abstraction, SX1261/SX1262 radio drivers. **Breaking change:** `LMIC.rxtime` renamed to `LMIC.nextRxTime`.

See [CHANGELOG.md](CHANGELOG.md) for the full release history.

## Contributions

This library started from the IBM V1.5 open-source code.

- Thomas Telkamp and Matthijs Kooijman ported V1.5 to Arduino and did a lot of bug fixing.

- Terry Moore, LeRoy Leslie, Frank Rose, and ChaeHee Won did a lot of work on US support.

- Terry Moore added the AU915, AS923, KR920 and IN866 regions, and created the regionalization framework, and did corrections for LoRaWAN 1.0.3 compliance testing.

- [`@tanupoo`](https://github.com/tanupoo) of the WIDE Project debugged AS923JP and LBT support.

- [`@frazar`](https://github.com/frazar) contributed numerous features, e.g. network time support, CI testing, documentation improvements.

- [`@manuelbl`](https://github.com/manuelbl) contributed numerous ESP32-related patches and improvements.

- [`@ngraziano`](https://github.com/ngraziano) did extensive testing and contributed numerous ADR-related patches.

- Tristan Webber ([`@TristanWebber`](https://github.com/TristanWebber)) contributed sx1261 and sx1262 support.

There are many others, who have contributed code and also participated in discussions, performed testing, reported problems and results. Thanks to all who have participated. We hope to use something like [All Contributors](https://https://allcontributors.org/) to help keep this up to date, but so far the automation isn't working.

## Trademark Acknowledgements

LoRa is a registered trademark of Semtech Corporation. LoRaWAN is a registered trademark of the LoRa Alliance.

MCCI and MCCI Catena are registered trademarks of MCCI Corporation.

All other trademarks are the properties of their respective owners.

## License

The upstream files from IBM v1.6 are based on the Berkeley license,
and the merge which synchronized this repository therefore migrated
the core files to the Berkeley license. However, modifications made
in the Arduino branch were done under the Eclipse license, so the
overall license of this repository is still Eclipse Public License
v1.0. The examples which use a more liberal
license. Some of the AES code is available under the LGPL. Refer to each
individual source file for more details, but bear in mind that until
the upstream developers look into this issue, it is safest to assume
the Eclipse license applies.

### Support Open Source Hardware and Software

MCCI invests time and resources providing this open source code, please support MCCI and open-source hardware by purchasing products from MCCI, Adafruit and other open-source hardware/software vendors!
