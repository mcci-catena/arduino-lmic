# Release History

## v6.0.1

- Fix loop overflow in SX126x `writeBuffer` ([#1041](https://github.com/mcci-catena/arduino-lmic/issues/1041)). Thanks to [@Maddocrus](https://github.com/Maddocrus) for reporting ([#1013](https://github.com/mcci-catena/arduino-lmic/issues/1013)).
- Fix buffer read overrun in SX126x radio driver ([#1040](https://github.com/mcci-catena/arduino-lmic/issues/1040)).
- Replace defunct forum.mcci.io links with GitHub Discussions ([#1035](https://github.com/mcci-catena/arduino-lmic/issues/1035)).
- Install latest Doxygen 1.x in CI instead of distro package ([#1033](https://github.com/mcci-catena/arduino-lmic/issues/1033)).
- Fix Doxygen rendering of RadioDriver.md ([#1029](https://github.com/mcci-catena/arduino-lmic/issues/1029)).

## v6.0.0

- Add Class C continuous reception support (`LMIC_ENABLE_class_c`). See [doc/CLASS-C.md](doc/CLASS-C.md) for usage. ([#323](https://github.com/mcci-catena/arduino-lmic/issues/323), [#1019](https://github.com/mcci-catena/arduino-lmic/pull/1019))
- Add secure element abstraction for key storage and crypto operations (`src/se/`). ([#578](https://github.com/mcci-catena/arduino-lmic/issues/578))
- Add SX1261/SX1262 radio driver support. ([#949](https://github.com/mcci-catena/arduino-lmic/pull/949))
- Add Doxygen support (`Doxyfile`).
- **Breaking change:** `LMIC.rxtime` renamed to `LMIC.nextRxTime`.

## v5.0.1

- Work around different behavior for `int32_t` wrap in newer GCC versions. ([#968](https://github.com/mcci-catena/arduino-lmic/issues/968), `v5.0.1-pre1`)

## v5.0.0

- Enable device time request by default in config file ([#840](https://github.com/mcci-catena/arduino-lmic/issues/840)).
- Add support for SX1261/SX1262 radios ([#949](https://github.com/mcci-catena/arduino-lmic/pull/949)).
- Refactor `README.md` a little and put little used configuration info in a separate file.
- Change all exports named `hal_*` to `lmic_hal_*`. This is a breaking change, and so the version number is advanced to 5.0.0. ([#714](https://github.com/mcci-catena/arduino-lmic/issues/714))
- Fix typos in documentation ([#956](https://github.com/mcci-catena/arduino-lmic/pull/956), [#879](https://github.com/mcci-catena/arduino-lmic/pull/879)).
- Initialize DHT sensor in ttn-abp-feather-us915-dht22 example ([#902](https://github.com/mcci-catena/arduino-lmic/pull/902))
- Fix configPower for sx1272 ([#894](https://github.com/mcci-catena/arduino-lmic/pull/894))
- Enable device time by request ([#840](https://github.com/mcci-catena/arduino-lmic/pull/840))
- Correct bug in MAC Rx1DrOffset error checking for regions other than US ([#841](https://github.com/mcci-catena/arduino-lmic/issues/841)). Thanks to @GitTibbe for finding this.
- Refactor the LMIC to enable secure element support ([#578](https://github.com/mcci-catena/arduino-lmic/issues/578)).
- Add Class C support ([#323](https://github.com/mcci-catena/arduino-lmic/issues/323)).
- Start resurrecting Doxygen support.

## v4.1.1

- Fix US-like regions when network server disables all channels before setting others up ([#819](https://github.com/mcci-catena/arduino-lmic/issues/819)).
- Fix US-like regions when network server disables all banks before setting others up ([#820](https://github.com/mcci-catena/arduino-lmic/issues/820)).
- Documentation improvements in README and in code commentary.

## v4.1.0

- Adapt `ttn-otaa-network-time` example to be compatible with [PaulStoffregen/Time](https://github.com/PaulStoffregen/Time) v1.6.1, which deletes `Time.h` in favor of `TimeLib.h` [#763](https://github.com/mcci-catena/arduino-lmic/issues/763). Version is v4.0.1-pre1.
- Add support for TTGO LoRa32-OLED v2.1.6. (Thanks to [@ChrSchultz](https://github.com/ChrSchultz), [#692](https://github.com/mcci-catena/arduino-lmic/pull/692).)
- Correct max TX EIRP for Japan to 13 dBm. (Thanks to [@ryos36](https://github.com/ryos36), [#662](https://github.com/mcci-catena/arduino-lmic/pull/662).)
- Correct link in this document to the LMIC-v4.0.0 pdf ([#788](https://github.com/mcci-catena/arduino-lmic/issues/788), thanks [@lnlp](https://github.com/lnlp)).
- Warn about Feather pin wiring requirements ([#755](https://github.com/mcci-catena/arduino-lmic/issues/755), thanks to [@d-a-v](https://github.com/d-a-v)).
- Fix typos in this document ([#780](https://github.com/mcci-catena/arduino-lmic/issues/780), thanks [@PeeJay](https://github.com/PeeJay)).
- Fix additional warnings on non-ARM platforms ([#791](https://github.com/mcci-catena/arduino-lmic/issues/791), thanks [@d-a-v](https://github.com/d-a-v)).
- Allow application to set the value to be used in `DeviceStatusAns` MAC messages ([#576](https://github.com/mcci-catena/arduino-lmic/issues/576) and [#560](https://github.com/mcci-catena/arduino-lmic/issues/560), thanks to [@altishchenko](https://github.com/altishchenko)).
- Minor adjustments to the compliance sketch ([#800](https://github.com/mcci-catena/arduino-lmic/issues/576)).
- Update the LMIC reference manual to `LMIC-v4.1.0.pdf`.

## v4.0.0

Major release; changes are significant enough to be "likely breaking".

- Fix some broken documentation references [#644](https://github.com/mcci-catena/arduino-lmic/issues/644), [#646](https://github.com/mcci-catena/arduino-lmic/pulls/646), [#673](https://github.com/mcci-catena/arduino-lmic/pulls/673).
- Re-added CI testing, since Travis CI no longer works for us [#647](https://github.com/mcci-catena/arduino-lmic/issues/647); fixed AVR compliance CI compile [#679](https://github.com/mcci-catena/arduino-lmic/issues/679).
- Don't use `defined()` in macro definitions [#606](https://github.com/mcci-catena/arduino-lmic/issues/606)
- Fix a warning on AVR32 [#709](https://github.com/mcci-catena/arduino-lmic/pulls/709).
- Fix Helium link in examples [#715](https://github.com/mcci-catena/arduino-lmic/issues/715), [#718](https://github.com/mcci-catena/arduino-lmic/pulls/718).
- Remove `XCHANNEL` support from US region [#404](https://github.com/mcci-catena/arduino-lmic/issues/404)
- Assign channels randomly without replacement [#515](https://github.com/mcci-catena/arduino-lmic/issues/515), [#619](https://github.com/mcci-catena/arduino-lmic/issues/619), [#730](https://github.com/mcci-catena/arduino-lmic/issues/730).
- Don't allow `LMIC_setupChannel()` to change default channels [#722](https://github.com/mcci-catena/arduino-lmic/issues/722). Add `LMIC_queryNumDefaultChannels()` [#700](https://github.com/mcci-catena/arduino-lmic/issues/700).
- Don't accept out-of-range DRs from MAC downlink messages [#723](https://github.com/mcci-catena/arduino-lmic/issues/723)
- Adopt semantic versions completely [#726](https://github.com/mcci-catena/arduino-lmic/issues/726).
- Implement JoinAccept CFList processing for US/AU [#739](https://github.com/mcci-catena/arduino-lmic/issues/739).
- Correct JoinAccept CFList processing for AS923 [#740](https://github.com/mcci-catena/arduino-lmic/issues/740).
- Don't compile board config objects when we know for sure they'll not be used; compilers can't always tell [#736](https://github.com/mcci-catena/arduino-lmic/issues/736).

## v3.3.0

- Added [LoRaWAN-at-a-glance](doc/LoRaWAN-at-a-glance.pdf) and a [state diagram of the LMIC](doc/LMIC-FSM.pdf).
- Several PRs from Matthijs Kooijman to improve compatibility with the original Arduino LMIC ([#608](https://github.com/mcci-catena/arduino-lmic/issues/608), [#609](https://github.com/mcci-catena/arduino-lmic/issues/609)).
- Numerous documentation improvements contributed by the community ([#431](https://github.com/mcci-catena/arduino-lmic/issues/431), [#612](https://github.com/mcci-catena/arduino-lmic/issues/612), [#614](https://github.com/mcci-catena/arduino-lmic/issues/614), [#625](https://github.com/mcci-catena/arduino-lmic/issues/625)).

## v3.2.0

- [#550](https://github.com/mcci-catena/arduino-lmic/issues/550) fixes debug prints in `ttn-otaa.ino`.
- [#553](https://github.com/mcci-catena/arduino-lmic/issues/553) add full regional support to `raw.ino` and `ttn-abp.ino`.
- [#570](https://github.com/mcci-catena/arduino-lmic/issues/570) corrects handling of piggy-back MAC responses when sending an `LMIC_sendAlive()` (`OPMODE_POLL`) message.
- [#524](https://github.com/mcci-catena/arduino-lmic/issues/524) corrects handling of interrupt disable, and slightly refactors the low-level interrupt handling wrappers for clarity. With this change, `radio_irq_handler_v2()` is never called except from the run loop, and so the radio driver need not (and does not) disable interrupts. Version is v3.1.0.20.
- [#568](https://github.com/mcci-catena/arduino-lmic/issues/568) improves documentation for the radio driver.
- [#537](https://github.com/mcci-catena/arduino-lmic/pull/537) fixes a compile error in SX1272 support. (Thanks @ricaun.) Version is v3.1.0.10.

## v3.1.0

Officially adopts the changes from v3.0.99. There were dozens of changes; check the GitHub issue logs and change logs. This was a breaking release (due to changes in data layout in the LMIC structure; the structure is accessed by apps).

## v3.0.99

Pre-release for v3.1.0. Note that the behavior of the LMIC changes in important ways, as it now enforces the LoRaWAN mandated maximum frame size for a given data rate. For Class A devices, this may cause your device to go silent after join, if you're not able to handle the frame size dictated by the parameters downloaded to the device by the network during join.

- [#470](https://github.com/mcci-catena/arduino-lmic/pull/470) corrects the name of AU915 region. [#516](https://github.com/mcci-catena/arduino-lmic/pull/516) makes sure that `LMIC_REGION_au921` is defined (but deprecated) for backward compatibility.
- [#452](https://github.com/mcci-catena/arduino-lmic/pull/452) fixes a bug [#450](https://github.com/mcci-catena/arduino-lmic/issues/450) in `LMIC_clrTxData()` that would cause join hiccups if called while (1) a join was in progress and (2) a regular data packet was waiting to be uplinked after the join completes. Also fixes AS923- and AU915-specific bugs [#446](https://github.com/mcci-catena/arduino-lmic/issues/446), [#447](https://github.com/mcci-catena/arduino-lmic/issues/447), [#448](https://github.com/mcci-catena/arduino-lmic/issues/448). Version is `v3.0.99.5`.
- [#443](https://github.com/mcci-catena/arduino-lmic/pull/443) addresses a number of problems found in cooperation with [RedwoodComm](https://redwoodcomm.com). They suggested a timing improvement to speed testing; this lead to the discovery of a number of problems. Some were in the compliance framework, but one corrects timing for very high spreading factors, several ([#442](https://github.com/mcci-catena/arduino-lmic/issues/442), [#436](https://github.com/mcci-catena/arduino-lmic/issues/438), [#435](https://github.com/mcci-catena/arduino-lmic/issues/435), [#434](https://github.com/mcci-catena/arduino-lmic/issues/434) fix glaring problems in FSK support; [#249](https://github.com/mcci-catena/arduino-lmic/issues/249) greatly enhances stability by making API calls much less likely to crash the LMIC if it's active. Version is v3.0.99.3.
- [#388](https://github.com/mcci-catena/arduino-lmic/issues/388), [#389](https://github.com/mcci-catena/arduino-lmic/issues/390), [#390](https://github.com/mcci-catena/arduino-lmic/issues/390) change the LMIC to honor the maximum frame size for a given DR in the current region. This proves to be a breaking change for many applications, especially in the US, because DR0 in the US supports only an 11-byte payload, and many apps were ignoring this. Additional error codes were defined so that apps can detect and recover from this situation, but they must detect; otherwise they run the risk of being blocked from the network by the LMIC.  Because of this change, the next version of the LMIC will be V3.1 or higher, and the LMIC version for development is bumped to 3.0.99.0.
- [#401](https://github.com/mcci-catena/arduino-lmic/issues/401) adds 865 MHz through 868 MHz to the "1%" band for EU.
- [#395](https://github.com/mcci-catena/arduino-lmic/pull/395) corrects pin-mode initialization if using `lmic_hal_interrupt_init()`.
- [#385](https://github.com/mcci-catena/arduino-lmic/issues/385) corrects an error handling data rate selection for `TxParamSetupReq`, found in US-915 certification testing. (v2.3.2.71)
- [#378](https://github.com/mcci-catena/arduino-lmic/pull/378) completely reworks MAC downlink handling. Resulting code passes the LoRaWAN V1.5 EU certification test. (v2.3.2.70)
- [#360](https://github.com/mcci-catena/arduino-lmic/issues/360) adds support for the KR-920 regional plan.

## v2.3.2

- [#204](https://github.com/mcci-catena/arduino-lmic/pull/204) eliminates a warning if using a custom pin-map.
- [#206](https://github.com/mcci-catena/arduino-lmic/pull/206) updates CI testing to Arduino IDE v1.8.8.

## v2.3.1

Adds `<arduino_lmic_user_configuration.h>`, which loads the pre-processor LMIC configuration variables into scope (issue [#199](https://github.com/mcci-catena/arduino-lmic/issues/199)).

## v2.3.0

1. The pin-map is extended with an additional field `pConfig`, pointing to a C++ class instance. This instance, if provided, has extra methods for dealing with TCXO control and other fine details of operating the radio. It also gives a natural way for us to extend the behavior of the HAL.

2. Pinmaps can be pre-configured into the library, so that users don't have to do this in every sketch.

Accompanying this was a fairly large refactoring of inner header files. We now have top-level header file `<arduino_lmic_hal_configuration.h>`, which provides much the same info as the original `<hal/hal.h>`, without bringing most of the LMIC internal definitions into scope. We also changed the SPI API based on a suggestion from `@manuelbl`, making the HAL more friendly to structured BSPs (and also making the SPI API potentially faster).

## Interim bug fixes (pre-v2.3.0)

Added a new API (`radio_irq_handler_v2()`), which allows the caller to provide the timestamp of the interrupt. This allows for more accurate timing, because the knowledge of interrupt overhead can be moved to a platform-specific layer ([#148](https://github.com/mcci-catena/arduino-lmic/issues/148)). Fixed compile issues on ESP32 ([#140](https://github.com/mcci-catena/arduino-lmic/issues/140) and [#153](https://github.com/mcci-catena/arduino-lmic/issues/150)). We added ESP32 and 32u4 as targets in CI testing. We switched CI testing to Arduino IDE 1.8.7.
Fixed issue [#161](https://github.com/mcci-catena/arduino-lmic/issues/161) selecting the Japan version of as923 using `CFG_as923jp` (selecting via `CFG_as923` and `LMIC_COUNTRY_CODE=LMIC_COUNTRY_CODE_JP` worked).
Fixed [#38](https://github.com/mcci-catena/arduino-lmic/issues/38) -- now any call to lmic_hal_init() will put the NSS line in the idle (high/inactive) state. As a side effect, RXTX is initialized, and RESET code changed to set value before transitioning state. Likely no net effect, but certainly more correct.

## V2.2.2

Adds `ttn-abp-feather-us915-dht22.ino` example, and fixes some documentation typos. It also fixes encoding of the `Margin` field of the `DevStatusAns` MAC message ([#130](https://github.com/mcci-catena/arduino-lmic/issues/130)).  This makes Arduino LMIC work with networks implemented with [LoraServer](https://www.loraserver.io/).

## V2.2.1

Corrects the value of `ARDUINO_LMIC_VERSION` ([#123](https://github.com/mcci-catena/arduino-lmic/issues/123)), allows ttn-otaa-feather-us915 example to compile for the Feather 32u4 LoRa ([#116](https://github.com/mcci-catena/arduino-lmic/issues/116)), and addresses documentation issues ([#122](https://github.com/mcci-catena/arduino-lmic/issues/122), [#120](https://github.com/mcci-catena/arduino-lmic/issues/120)).

## V2.2.0

Adds encoding functions and `tn-otaa-feather-us915-dht22.ino` example. Plus a large number of issues: [#59](https://github.com/mcci-catena/arduino-lmic/issues/59), [#60](https://github.com/mcci-catena/arduino-lmic/issues/60), [#63](https://github.com/mcci-catena/arduino-lmic/issues/63), [#64](https://github.com/mcci-catena/arduino-lmic/issues/47) (listen-before-talk for Japan), [#65](https://github.com/mcci-catena/arduino-lmic/issues/65), [#68](https://github.com/mcci-catena/arduino-lmic/issues/68), [#75](https://github.com/mcci-catena/arduino-lmic/issues/75), [#78](https://github.com/mcci-catena/arduino-lmic/issues/78), [#80](https://github.com/mcci-catena/arduino-lmic/issues/80), [#91](https://github.com/mcci-catena/arduino-lmic/issues/91), [#98](https://github.com/mcci-catena/arduino-lmic/issues/98), [#101](https://github.com/mcci-catena/arduino-lmic/issues/101). Added full Travis CI testing, switched to travis-ci.com as the CI service. Prepared to publish library in the official Arduino library list.

## V2.1.5

Fixes issue [#56](https://github.com/mcci-catena/arduino-lmic/issues/56) (a documentation bug). Documentation was quickly reviewed and other issues were corrected. The OTAA examples were also updated slightly.

## V2.1.4

Fixes issues [#47](https://github.com/mcci-catena/arduino-lmic/issues/47) and [#50](https://github.com/mcci-catena/arduino-lmic/issues/50) in the radio driver for the SX1276 (both related to handling of output power control bits).

## V2.1.3

Fix for issue [#43](https://github.com/mcci-catena/arduino-lmic/issues/43): handling of `LinkAdrRequest` was incorrect for US915 and AU915; when TTN added ADR support on US and AU, the deficiency was revealed (and caused an ASSERT).

## V2.1.2

Fix for issue [#39](https://github.com/mcci-catena/arduino-lmic/issues/39) (adding a prototype for `LMIC_DEBUG_PRINTF` if needed). Fully upward compatible, so just a patch.

## V2.1.1

Same content as V2.1.2, but was accidentally released without updating `library.properties`.

## V2.1.0

Adds support for the Murata LoRaWAN module.

## V2.0.2

Adds support for additional regions.
