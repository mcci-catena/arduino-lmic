# APIs added in v6.1

This page covers the APIs added in v6.1. The API reference, [LMIC-v5.0.0.pdf](LMIC-v5.0.0.pdf), does not cover them yet. The header comments give the details, and Doxygen publishes them on the [GitHub Pages site](https://mcci-catena.github.io/arduino-lmic/). [CHANGELOG.md](../CHANGELOG.md) lists every change in v6.1.

All of these are declared through `arduino_lmic.h`.

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

- Save after each transaction, and store the bytes in non-volatile memory.
- After a reset, call `LMIC_reset()`, then `LMIC_restoreSessionState()` with the stored bytes, before starting a transaction or a join.
- `LMIC_querySessionState()` checks a blob and reports its version, region, channel kind, size, client tag and join flags. It does not change the LMIC.
- `clientTag` is stored in the blob and returned by the query. The LMIC does not interpret it.

The LMIC writes version 3 (224 bytes, `LMIC_SESSION_STATE_SIZE`) and reads versions 1, 2 and 3. Version 1 is the Arduino-LoRaWAN `SessionStateV1` layout. On a version 1 restore the LMIC uses the regulatory duty cycle for each channel group instead of the saved one. Version 3 adds the next DevNonce and the join back-off, and a version 3 blob saved before the device joined restores only those. To keep a DevNonce from repeating after a power failure, save on `EV_TXSTART` while joining: the LMIC reports it after it has used the DevNonce and before the radio transmits.

The functions return `LMIC_SESSION_STATE_OK` or a reason for refusing the blob, such as `LMIC_SESSION_STATE_REGION_MISMATCH`. The header gives the full list and the byte layout.

## Accessors

`src/lmic/lmic_accessors.h` has `static inline` getters for the LMIC fields that clients read, and setters for the few that a client may change. Use them instead of reading or writing `LMIC.*`: the layout of the LMIC object changes in v7, and the accessors don't. Where the LMIC's field name differs from the LoRaWAN name, the accessor uses the LoRaWAN name, for example `LMIC_getFCntUp()`, `LMIC_getDevAddr()`, `LMIC_getDataRate()`, `LMIC_getRSSI()` and `LMIC_getSNR()`.

`LMIC_getFRMPayload(&pData, &len)` returns the FPort of the last received frame, or zero if it had no payload, and sets `pData` and `len` to the payload.

For raw (non-LoRaWAN) radio use, `LMIC_setFrequency()`, `LMIC_setRps()`, `LMIC_setRadioTxPower()`, `LMIC_setRawTxData()`, `LMIC_getRawRxData()`, `LMIC_setNextRxTime()` and the others set up the radio without touching the MAC. The raw examples show how; each passes its own `osjob_t` to `os_radio_v2()`.

## Channels and channel groups

```c
u1_t  LMIC_queryChannelCount(void);
bit_t LMIC_queryChannel(u1_t channel, lmic_channel_info_t *pInfo);
```

`LMIC_queryChannelCount()` returns the number of channel indices in the active region (16 or 72). `LMIC_queryChannel()` fills `lmic_channel_info_t` with the uplink and RX1 downlink frequencies, the data-rate map, the channel group, the bandwidth, whether the channel is enabled, and whether the region fixes it. It returns zero for an index outside the region.

A channel group is the set of channels that share one regulatory duty cycle and power limit. Earlier versions called it a band, which clashed with the frequency band and the US sub-band calls. `LMIC_setupChannelGroup()` replaces `LMIC_setupBand()`, which remains as a deprecated inline function and draws a warning on GCC 6 and later and on Clang. The last parameter of `LMIC_setupChannel()` is the group.

## Sleeping between jobs

```c
u4_t os_queryNextJobTicks(void);
u4_t os_runloop_once_ex(void);
bit_t lmic_hal_hasPendingIrqs(void);
```

`os_queryNextJobTicks()` returns the number of OS ticks until the LMIC next needs to run. It returns 0 if a job is ready, and `OS_NEXT_JOB_NEVER` if nothing is scheduled. It runs no job. `os_runloop_once_ex()` runs at most one job, like `os_runloop_once()`, never calls `lmic_hal_sleep()`, and returns `os_queryNextJobTicks()` as computed after the job ran.

With the default polled DIO handling, the LMIC sees the end of a transmit or receive only by polling, so `os_queryNextJobTicks()` returns 0 while the radio is busy. With `LMIC_USE_INTERRUPTS`, the DIO interrupt wakes the caller, so it returns the time to the next job and the caller can sleep through the radio operation. Before sleeping, mask interrupts and check `lmic_hal_hasPendingIrqs()`: it is true if a DIO event has been recorded and not yet processed. `LMIC_HAL_HAS_PENDING_IRQS_QUERY` is defined when the function exists.

`os_runloop_once()` behaves as before.

## Link check

```c
void LMIC_requestLinkCheck(lmic_request_link_check_cb_t *pCallbackfn, void *pUserData);
typedef void lmic_request_link_check_cb_t(void *pUserData, int flagSuccess, u1_t margin, u1_t gwCnt);
```

`LMIC_requestLinkCheck()` puts a LinkCheckReq in FOpts of the next uplink. The LMIC calls the callback when a LinkCheckAns arrives, with the margin in dB and the number of gateways that received the uplink, or with `flagSuccess` false if the uplink completes without one. `LMIC_ENABLE_LinkCheckReq` (default 1) compiles it in; it costs about 150 bytes of flash and 7 bytes of RAM on AVR.

## Joining and DevNonce

The LMIC keeps Join-Request airtime within the limits of LoRaWAN 1.0.4 section 7. DevNonce no longer changes at every `LMIC_reset()`: the first reset after power-up picks a random start, and each Join-Request counts up from it.

```c
bit_t LMIC_stopJoining(void);
u4_t  LMIC_getNextDevNonce(void);
bit_t LMIC_setNextDevNonce(u4_t next);
void  LMIC_clearJoinState(void);
```

- `LMIC_stopJoining()` stops a join between Join-Requests and keeps the back-off and the DevNonce sequence, so a later `LMIC_startJoining()` continues them. It returns 0 and changes nothing while a Join-Request is on the air or its receive windows are open; call it again after `EV_JOIN_TXCOMPLETE`.
- `LMIC_getNextDevNonce()` and `LMIC_setNextDevNonce()` read and set the DevNonce the next Join-Request will use (0 to 0xFFFF). Setting it starts a new sequence.
- When every DevNonce in the sequence has been used, the LMIC stops joining and reports `EV_DEVNONCE_EXHAUSTED`.
- `LMIC_clearJoinState()` clears the back-off and starts a new DevNonce sequence at a random value. Call it when DevEUI, JoinEUI or AppKey changes, then save the session state.

## ADR_ACK limits

```c
bit_t LMIC_setAdrAckLimits(u1_t limit, u1_t delay);
void  LMIC_getAdrAckLimits(u1_t *pLimit, u1_t *pDelay);
```

`LMIC_setAdrAckLimits()` sets ADR_ACK_LIMIT (1 to 127) and ADR_ACK_DELAY (at least 1) at run time. LoRaWAN's defaults are 64 and 32, and `LMIC_reset()` restores them. A device that sends rarely can use smaller values to notice a lost network sooner. The function returns false, changing nothing, for values out of range.

## Data rate for a frame

```c
dr_t LMIC_feasibleDataRateForFrame(dr_t dr, u1_t payloadSize);
```

Starting at `dr`, this function steps to faster data rates until one can carry a frame with `payloadSize` bytes of payload, and returns it, or `dr` if none can. `LMIC_sendWithCallback()` uses it to raise the data rate when a frame doesn't fit; an application can call it first, to split the message instead. Earlier versions defined it but didn't declare it in `lmic.h`.

## Version macros

The version numbers are in `src/lmic/lmic_version.h`. The fourth field of `ARDUINO_LMIC_VERSION_CALC()` is the pre-release number, `pre`, and `ARDUINO_LMIC_VERSION_GET_PRE()` extracts it. `ARDUINO_LMIC_VERSION_GET_LOCAL()` remains as a deprecated alias.

## SX126x crystal trim

A board whose SX126x needs a different crystal trim overrides `querySX126xXTATrim()` and `querySX126xXTBTrim()` in its `HalConfiguration_t`, returning 0x00 to 0x3F. The default leaves the chip's reset value. [HOWTO-Manually-Configure.md](HOWTO-Manually-Configure.md) describes them.
