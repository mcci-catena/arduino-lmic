# APIs added in v6.1

This page covers the APIs added in v6.1. The API reference, [LMIC-v5.0.0.pdf](LMIC-v5.0.0.pdf), does not cover them yet. The header comments give the details, and Doxygen publishes them on the [GitHub Pages site](https://mcci-catena.github.io/arduino-lmic/). [CHANGELOG.md](../CHANGELOG.md) lists every change in v6.1.

Arduino sketches get all of these by including `arduino_lmic.h`. Other ports that include `lmic/lmic.h` must also include `lmic/lmic_accessors.h` and `lmic/lmic_session_state.h`.

## Upgrading from 6.0.x

A sketch that builds with 6.0.x builds with 6.1, but these changes can affect it:

- **Join back-off.** The LMIC keeps Join-Request airtime within the limits of LoRaWAN 1.0.4 section 7: under 36 s in the first hour, under 36 s in the next 10 hours, and under 8.7 s in each 24 hours after that. A device whose joins keep failing now waits longer between retries than it did. `LMIC_reset()` clears the back-off, and so does `LMIC_clearJoinBackoff()`. `LMIC_setJoinBackoffEnabled(0)` turns it off, for certification testing only.
- **DevNonce.** DevNonce no longer changes at every `LMIC_reset()`. It starts at a random value and counts up with each Join-Request. Unless the sketch saves and restores the session state (below), each power-up still starts at a new random value, and a LoRaWAN 1.0.4 network server may reject a DevNonce it has seen before.
- **Deprecated names.** `LMIC_setupBand()` is now `LMIC_setupChannelGroup()`, and `ARDUINO_LMIC_VERSION_GET_LOCAL()` is now `ARDUINO_LMIC_VERSION_GET_PRE()`. The old names still work. They draw a deprecation warning on GCC 6 and later and on Clang, if compiler warnings are enabled; the Arduino IDE turns warnings off by default.
- **C linkage.** `arduino_lmic.h` no longer wraps its includes in `extern "C"`; each LMIC header wraps its own declarations. Sketches need no change. A port that adds a C header to the LMIC must wrap it in `LMIC_BEGIN_DECLS` and `LMIC_END_DECLS`.
- **Direct use of `LMIC.*`.** Use the accessors (below) instead. The layout of the LMIC object changes in v7, and the accessors don't.

## Session state

`src/lmic/lmic_session_state.h` saves the LoRaWAN session as an opaque blob of bytes, so a client can keep it across a reset or a sleep without knowing how the LMIC lays out its state.

```c
size_t LMIC_getSessionStateSize(void);
lmic_session_state_result_t LMIC_saveSessionState(
	u1_t *pBuf, size_t nBuf, size_t *pnUsed, u2_t clientTag);
lmic_session_state_result_t LMIC_restoreSessionState(
	const u1_t *pBuf, size_t nBuf);
lmic_session_state_result_t LMIC_querySessionState(
	const u1_t *pBuf, size_t nBuf, lmic_session_state_info_t *pInfo);
```

The blob holds the frame counters, the MAC settings the network has sent, the channels, the next DevNonce and the join back-off. It does not hold the NetID, DevAddr or session keys; the client saves those itself, from `LMIC_getSessionKeys()`.

To save:

- On `EV_JOINED`, save the NetID, DevAddr and keys from `LMIC_getSessionKeys()`.
- After each transaction (`EV_TXCOMPLETE`), save the blob in non-volatile memory.
- While joining, also save the blob on `EV_TXSTART` when `LMIC_getOpmode() & OP_JOINING` is set. The LMIC reports `EV_TXSTART` after it has used the DevNonce and before the radio transmits, so the saved DevNonce doesn't repeat after a power failure.

To restore, after a reset:

1. `LMIC_reset()`.
2. If a session was saved, `LMIC_setSession()` with the saved NetID, DevAddr and keys.
3. `LMIC_restoreSessionState()` with the saved blob. This must come after `LMIC_setSession()`, which resets the frame counters and channels.
4. Set again anything that `LMIC_reset()` returns to its default and the blob doesn't hold, such as `LMIC_setAdrMode()` and `LMIC_setAdrAckLimits()`.

Then start the first transaction or join.

`LMIC_querySessionState()` checks a blob and reports its version, region, channel kind, size, client tag and join flags. It does not change the LMIC. `clientTag` is stored in the blob and returned by the query; the LMIC does not interpret it.

The LMIC writes version 3 (224 bytes, `LMIC_SESSION_STATE_SIZE`) and reads versions 1, 2 and 3. Version 1 is the Arduino-LoRaWAN `SessionStateV1` layout. On a version 1 restore the LMIC uses the regulatory duty cycle for each channel group instead of the saved one. Version 3 adds the next DevNonce and the join back-off. A version 3 blob saved before the device joined restores only those, and the sketch skips step 2.

The functions return `LMIC_SESSION_STATE_OK` or a reason for refusing the blob, such as `LMIC_SESSION_STATE_REGION_MISMATCH`. The header gives the full list and the byte layout.

## Accessors

`src/lmic/lmic_accessors.h` has `static inline` getters for the LMIC fields that clients read, and setters for the few that a client may change. Use them instead of reading or writing `LMIC.*`. Where the LMIC's field name differs from the LoRaWAN name, the accessor uses the LoRaWAN name, for example `LMIC_getFCntUp()`, `LMIC_getDevAddr()`, `LMIC_getDataRate()`, `LMIC_getRSSI()` and `LMIC_getSNR()`.

`LMIC_getFRMPayload(&pData, &len)` returns the FPort of the last received frame, or zero if it had no payload, and sets `pData` and `len` to the payload.

For raw (non-LoRaWAN) radio use, `LMIC_setFrequency()`, `LMIC_setRps()`, `LMIC_setRadioTxPower()`, `LMIC_setRawTxData()`, `LMIC_getRawRxData()`, `LMIC_setNextRxTime()` and the others set up the radio without touching the MAC. The raw examples show how; each passes its own `osjob_t` to `os_radio_v2()`.

## Channels and channel groups

```c
u1_t  LMIC_queryChannelCount(void);
bit_t LMIC_queryChannel(u1_t channel, lmic_channel_info_t *pInfo);
```

`LMIC_queryChannelCount()` returns the number of channel indices in the active region (16 or 72). `LMIC_queryChannel()` fills `lmic_channel_info_t` with the uplink and RX1 downlink frequencies, the data-rate map, the channel group, the bandwidth, whether the channel is enabled, and whether the region fixes it. It returns zero for an index outside the region.

A channel group is the set of channels that share one regulatory duty cycle and power limit. Earlier versions called it a band, which clashed with the frequency band and the US sub-band calls. `LMIC_setupChannelGroup()` replaces `LMIC_setupBand()`. The last parameter of `LMIC_setupChannel()` is the group.

## Sleeping between jobs

```c
u4_t os_queryNextJobTicks(void);
u4_t os_runloop_once_ex(void);
bit_t lmic_hal_hasPendingIrqs(void);
```

`os_queryNextJobTicks()` returns the number of OS ticks until the LMIC next needs to run; `osticks2ms()` converts it to milliseconds. It returns 0 if a job is ready, and `OS_NEXT_JOB_NEVER` if nothing is scheduled. It runs no job. `os_runloop_once_ex()` runs at most one job, like `os_runloop_once()`, never calls `lmic_hal_sleep()`, and returns `os_queryNextJobTicks()` as computed after the job ran. `os_runloop_once()` behaves as before.

With the default polled DIO handling, the LMIC sees the end of a transmit or receive only by polling, so `os_queryNextJobTicks()` returns 0 while the radio is busy. With `LMIC_USE_INTERRUPTS`, the DIO interrupt wakes the caller, so it returns the time to the next job and the caller can sleep through the radio operation. `lmic_hal_hasPendingIrqs()` is true if a DIO event has been recorded and not yet processed. `LMIC_HAL_HAS_PENDING_IRQS_QUERY` is defined when it exists.

A `loop()` that sleeps between jobs:

```c++
void loop() {
    u4_t ticks = os_runloop_once_ex();

    if (ticks == 0)
        return;                 // a job is ready, or the radio is busy

    // the application's own work goes here; it can lower ticks

    lmic_hal_disableIRQs();     // no interrupt between the check and the sleep
    if (! lmic_hal_hasPendingIrqs())
        sleepUpTo(ticks);       // board-specific; see below
    lmic_hal_enableIRQs();
}
```

`sleepUpTo()` is the application's. It arms a wake-up timer for `ticks` (none for `OS_NEXT_JOB_NEVER`) and sleeps until that timer or any interrupt. On Arm Cortex-M, `__WFI()` does this: it wakes when an interrupt becomes pending even though `lmic_hal_disableIRQs()` has masked it, and the interrupt runs at `lmic_hal_enableIRQs()`. On AVR, the sleep instruction must directly follow `sei`, so the masked check and the sleep take board-specific code.

`os_getTime()` must keep counting during the sleep, or the LMIC misses its receive windows. The Arduino HAL takes it from `micros()`, which stops in the deeper sleep modes of many MCUs; `__WFI()` sleep keeps it running. The Arduino HAL also needs `os_getTime()` called at least every 30 minutes to follow the wrap of `micros()`, so cap the sleep at that.

## Link check

```c
void LMIC_requestLinkCheck(lmic_request_link_check_cb_t *pCallbackfn, void *pUserData);
typedef void lmic_request_link_check_cb_t(void *pUserData, int flagSuccess, u1_t margin, u1_t gwCnt);
```

`LMIC_requestLinkCheck()` puts a LinkCheckReq in FOpts of the next uplink. The LMIC calls the callback when a LinkCheckAns arrives, with the margin in dB and the number of gateways that received the uplink, or with `flagSuccess` false if the uplink completes without one. If a request is already pending, or link check is compiled out, the LMIC calls the callback at once with `flagSuccess` false; that doesn't mean the link was lost. `LMIC_ENABLE_LinkCheckReq` (default 1) compiles it in; it costs about 150 bytes of flash and 7 bytes of RAM on AVR.

## Joining and DevNonce

The upgrade notes above describe the join back-off and the DevNonce sequence.

```c
bit_t LMIC_stopJoining(void);
u4_t  LMIC_getNextDevNonce(void);
bit_t LMIC_setNextDevNonce(u4_t next);
void  LMIC_clearJoinState(void);
void  LMIC_clearJoinBackoff(void);
void  LMIC_setJoinBackoffEnabled(bit_t fEnable);
```

- `LMIC_stopJoining()` stops a join between Join-Requests and keeps the back-off and the DevNonce sequence, so a later `LMIC_startJoining()` continues them. It returns 0 and changes nothing while a Join-Request is on the air or its receive windows are open; call it again after `EV_JOIN_TXCOMPLETE`. While the device is not joined, anything that runs the MAC, for example `LMIC_setTxData2()`, starts a new join.
- `LMIC_getNextDevNonce()` and `LMIC_setNextDevNonce()` read and set the DevNonce the next Join-Request will use (0 to 0xFFFF). Setting it starts a new sequence. `LMIC_setNextDevNonce()` returns 0, changing nothing, for a value out of range.
- When every DevNonce in the sequence has been used, the LMIC stops joining and reports `EV_DEVNONCE_EXHAUSTED`.
- `LMIC_clearJoinState()` clears the back-off and starts a new DevNonce sequence at a random value. Call it when DevEUI, JoinEUI or AppKey changes, then save the session state.
- `LMIC_clearJoinBackoff()` forgets the back-off history, so the next join attempt starts the limits again. `LMIC_setJoinBackoffEnabled(0)` stops the LMIC from spacing Join-Requests; it is for certification testing, and a deployed device leaves it on.

## ADR_ACK limits

```c
bit_t LMIC_setAdrAckLimits(u1_t limit, u1_t delay);
void  LMIC_getAdrAckLimits(u1_t *pLimit, u1_t *pDelay);
```

`LMIC_setAdrAckLimits()` sets ADR_ACK_LIMIT (1 to 127) and ADR_ACK_DELAY (at least 1) at run time. LoRaWAN's defaults are 64 and 32. A device that sends rarely can use smaller values to notice a lost network sooner. The function returns false, changing nothing, for values out of range. `LMIC_reset()` restores the defaults, and the session state doesn't hold the limits, so set them again after a reset or a restore.

## Data rate for a frame

```c
dr_t LMIC_feasibleDataRateForFrame(dr_t dr, u1_t payloadSize);
```

Starting at `dr`, this function steps to faster data rates until one can carry a frame with `payloadSize` bytes of payload, and returns it, or `dr` if none can. `LMIC_sendWithCallback()` uses it to raise the data rate when a frame doesn't fit; an application can call it first, to split the message instead. Earlier versions defined it but didn't declare it in `lmic.h`.

## Version macros

The version numbers are in `src/lmic/lmic_version.h`. The fourth field of `ARDUINO_LMIC_VERSION_CALC()` is the pre-release number, `pre`, and `ARDUINO_LMIC_VERSION_GET_PRE()` extracts it. `ARDUINO_LMIC_VERSION_GET_LOCAL()` remains as a deprecated alias.

## SX126x crystal trim

A board whose SX126x needs a different crystal trim overrides `querySX126xXTATrim()` and `querySX126xXTBTrim()` in its `HalConfiguration_t`, returning 0x00 to 0x3F. The default leaves the chip's reset value. [HOWTO-Manually-Configure.md](HOWTO-Manually-Configure.md) describes them.
