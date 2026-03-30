# Radio Driver parameters

## Radio Driver Operation

The LMIC radio driver operates asynchronously. Operations are started by calling the `os_radio()` function with a parameter describing the operation to be performed.

Various parameters in the LMIC structure are as input to control the operation; others are updated to return results.

### `os_radio(RADIO_RST)`

The radio is reset, and put to sleep. This operation is synchronous.

### `os_radio(RADIO_TX)`

A frame is transmitted. The parameters are given in "Common parameters" and "Transmit parameters" below.

When the operation completes, `LMIC.osjob` is scheduled.

### `os_radio(RADIO_RX)`

A single frame is received at the specified time, and the radio is put back to sleep if no frame is found.

When the operation completes, `LMIC.osjob` is scheduled.

### `os_radio(RADIO_RXON)`

The radio is placed in continuous receive mode. If a frame is received, `LMIC.osjob` is scheduled. Continuous receive is explicitly canceled by calling `os_radio(RADIO_RST)` or implicitly by calling any other radio function.

This operation is not supported in FSK mode.

### `os_radio(RADIO_RXON_C)`

This operation is identical to `os_radio(RADIO_RXON)`, but redundant calls are suppressed.  That is, if the radio is already in receive mode, the radio stays in receive mode without any change in state.  In contrast, `RADIO_RXON` expects the radio to be idle, and will issue an assertion failure if the radio is in any other state.

### `os_radio(RADIO_TX_AT)`

This is like `os_radio(RADIO_TX)`, but the transmission is scheduled at `LMIC.txend`.

## Common parameters

### `LMIC.radio.rps` (IN)

This is the "radio parameter setting", and it encodes several radio settings.

- Spreading factor: FSK or SF7..12
- Bandwidth: 125, 250 or 500 kHz
- Coding Rate: 4/5, 4/6, 4/7 or 4/8.
- CRC enabled/disabled
- Implicit header mode on/off. (If on, receive length must be known in advance.)

### `LMIC.radio.freq` (IN)

This specifies the frequency, in Hertz.

### `LMIC.saveIrqFlags` (OUT)

Updated for LoRa operations only; the IRQ flags at the time of interrupt.

### `LMIC.radio.pRadioDoneJob` (IN/OUT)

When asynchronous operations complete, `LMIC.radio.pRadioDoneJob->func` is used as the callback function, and `LMIC.radio.pRadioDoneJob` is used to schedule the work.

## Transmit parameters

### `LMIC.radio.txpow` (IN)

This specifies the transmit power in dBm.

### `LMIC.radio.pFrame[]` (IN)

The array of data to be sent.

### `LMIC.radio.datalen` (IN)

The length of the array to be sent.

### `LMIC.txend` (IN, OUT)

For `RADIO_TX_AT`, an input parameter, for the scheduled TX time.

For all transmissions, updated to the OS time at which the TX end interrupt was recognized.

### `LMIC.lbt_ticks` (IN)

How long to monitor for LBT, in OS ticks.

### `LMIC.lbt_dbmax` (IN)

Maximum RSSI on channel before transmit.

## Receive parameters

### `LMIC.radio.pFrame[]` (OUT)

Filled with data received.

### `LMIC.radio.dataLen` (OUT)

Set to number of bytes received in total.

### `LMIC.radio.rxtime` (IN/OUT)

Input: When to start receiving, in OS tick time. Only used for `os_radio(RADIO_RX)`; not used for continuous receive.

Output: time of RXDONE interrupt. (Note: FSK timeout doesn't currently set this cell on RX timeout.)

### `LMIC.radio.rxsyms` (IN)

The timeout in symbols. Only used for `os_radio(RADIO_RX)`; not used for continuous receive.

### `LMIC.radio.flags` (IN)

If the bit `LMIC_RADIO_FLAGS_NO_RX_IQ_INVERSION` is set, disable IQ inversion during receive.

### `LMIC.snr` (OUT)

Set to SNR * 4 after LoRa receive. (Set to 0 for FSK receive.)

### `LMIC.rssi` (OUT)

Set to RSSI + `RSSI_OFF` after LoRa receive. (Set to 0 for FSK receive; `RSSI_OFF` is 64.) You must subtract RSSI_OFF from `LMIC.rssi` to get the RSSI in dB.
