# Class C Support

This document describes the Class C implementation in the LMIC library.

## Overview

LoRaWAN Class C devices keep their receiver active whenever they are not transmitting. This allows them to receive downlink messages at any time, rather than only during the RX1 and RX2 windows after an uplink transmission (as with Class A devices).

Class C reception uses the RX2 channel parameters (frequency and spreading factor) for continuous reception.

## Enabling Class C

Class C support must be enabled at both compile time and runtime.

### Compile-time Configuration

Add the following to your `project_config/lmic_project_config.h`:

```c
#define LMIC_ENABLE_class_c 1
```

Or pass it as a build flag:

```shell
-D LMIC_ENABLE_class_c=1
```

**Note:** Class C support increases code size and RAM usage. Only enable it if your application requires Class C operation.

### Runtime Activation

After a successful OTAA join (or ABP session setup), call `LMIC_enableClassC()` to activate Class C mode:

```c
void onEvent(ev_t ev) {
    switch(ev) {
        case EV_JOINED:
            // Enable Class C after successful join
            LMIC_enableClassC(1);
            break;
        // ... other events
    }
}
```

To disable Class C mode and return to Class A operation:

```c
LMIC_enableClassC(0);
```

## API Reference

### LMIC_isConfiguredClassC()

```c
static inline bit_t LMIC_isConfiguredClassC(void);
```

Returns `1` if Class C support is compiled in (`LMIC_ENABLE_class_c` is non-zero), `0` otherwise. This is a compile-time check that can be used for conditional code.

### LMIC_enableClassC()

```c
bit_t LMIC_enableClassC(bit_t fOnIfTrue);
```

Enables or disables Class C mode at runtime.

- **Parameters:**
  - `fOnIfTrue`: Set to `1` to enable Class C, `0` to disable.
- **Returns:** The previous state (1 if Class C was enabled, 0 if disabled).

When Class C is enabled:
- The radio will start continuous reception on the RX2 channel after completing any Class A transaction.
- Downlink messages can be received at any time when the device is not transmitting.

When Class C is disabled:
- The device returns to Class A operation.
- The radio is only active during scheduled RX windows after uplink transmissions.

### LMIC_txrxFlags_isClassC()

```c
static inline bit_t LMIC_txrxFlags_isClassC(u1_t flags);
```

Checks if the received message was a Class C downlink (received outside the normal RX1/RX2 windows).

- **Parameters:**
  - `flags`: The `LMIC.txrxFlags` value.
- **Returns:** `1` if this was a Class C reception, `0` otherwise.

Example usage in event handler:

```c
case EV_RXCOMPLETE:
    if (LMIC_txrxFlags_isClassC(LMIC.txrxFlags)) {
        // This was a Class C downlink
        Serial.println("Class C downlink received!");
    }
    // Process LMIC.frame, LMIC.dataLen, etc.
    break;
```

## Example Usage

```c
#include <arduino_lmic.h>

void onEvent(ev_t ev) {
    switch(ev) {
        case EV_JOINED:
            Serial.println("Joined network");
            // Enable Class C after successful join
            if (LMIC_isConfiguredClassC()) {
                LMIC_enableClassC(1);
                Serial.println("Class C enabled");
            }
            break;

        case EV_RXCOMPLETE:
            Serial.print("Received ");
            Serial.print(LMIC.dataLen);
            Serial.println(" bytes");

            if (LMIC_txrxFlags_isClassC(LMIC.txrxFlags)) {
                Serial.println("(Class C downlink)");
            }

            // Process received data
            for (int i = 0; i < LMIC.dataLen; i++) {
                Serial.print(LMIC.frame[LMIC.dataBeg + i], HEX);
                Serial.print(" ");
            }
            Serial.println();
            break;

        // ... handle other events
    }
}
```

## Power Considerations

Class C operation significantly increases power consumption because the radio receiver is continuously active. This makes Class C unsuitable for battery-powered devices that need long battery life. Class C is typically used for:

- Mains-powered devices
- Devices requiring low-latency downlink communication
- Actuators that need to respond quickly to commands

## Known Limitations

- **Class B Interaction:** Class C and Class B features should not be used simultaneously.
- **Frame Counters:** As with all LoRaWAN operation, frame counters must be preserved across reboots for production deployments.

## Building for Class C

### Arduino IDE

Add to your `project_config/lmic_project_config.h`:

```c
#define CFG_us915 1              // or your region
#define CFG_sx1276_radio 1       // SX1276 recommended for Class C
#define LMIC_ENABLE_class_c 1
```

### PlatformIO

Add to your `platformio.ini`:

```ini
build_flags =
    -D CFG_us915=1
    -D CFG_sx1276_radio=1
    -D LMIC_ENABLE_class_c=1
```

Or to compile with Class C and test:

```bash
PLATFORMIO_BUILD_FLAGS='-D CFG_us915 -D CFG_sx1276_radio -D LMIC_ENABLE_class_c=1 -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' \
    platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa/ttn-otaa.ino'
```
