/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 * Copyright (c) 2018 Terry Moore, MCCI
 * Copyright (c) 2026 Pontus Oldberg, iLabs
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example demonstrates how to use the LMIC sleep/wake state API to
 * safely deep-sleep a LoRaWAN ABP device between transmissions. It builds
 * on the ttn-abp example and adds:
 *
 *   - LMIC_getSleepState() / LMIC_setSleepState() to persist the LMIC
 *     state (frame counters, RX parameters, sticky MAC ACKs) across
 *     power cycles.
 *   - EV_SLEEP_READY event, which fires once LMIC has fulfilled all MAC
 *     obligations for the current wake cycle (including any MAC-command
 *     response uplinks). This is the correct and safe point to save state
 *     and enter deep sleep.
 *
 * Platform-specific deep sleep and non-volatile storage are left as stubs
 * (see saveSleepState(), loadSleepState(), enterDeepSleep()) — replace
 * these with your hardware's NVRAM/EEPROM and sleep APIs.
 *
 * Why EV_SLEEP_READY instead of EV_TXCOMPLETE?
 * --------------------------------------------
 * After EV_TXCOMPLETE the network may have sent MAC commands (e.g.
 * NewChannelReq, RxTimingSetupReq) that require an acknowledgement uplink
 * on port 0. LMIC schedules this follow-up automatically; if the device
 * sleeps at EV_TXCOMPLETE the ACK is never sent and the network will keep
 * re-sending the command every downlink. EV_SLEEP_READY fires only after
 * all such obligations are done.
 *
 * Frame counter persistence (LoRaWAN §4.3.1):
 * --------------------------------------------
 * ABP devices MUST persist seqnoUp/seqnoDn across power cycles or the
 * network server will reject frames as replays. LMIC_getSleepState()
 * captures both counters (and other session parameters); restoring them
 * with LMIC_setSleepState() after LMIC_reset() keeps the session alive.
 *
 * Do not forget to define the radio type correctly in
 * arduino-lmic/project_config/lmic_project_config.h or from your BOARDS.txt.
 *
 *******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

//
// For normal use, we require that you edit the sketch to replace FILLMEIN
// with values assigned by the TTN console. However, for regression tests,
// we want to be able to compile these scripts. The regression tests define
// COMPILE_REGRESSION_TEST, and in that case we define FILLMEIN to a non-
// working but innocuous value.
//
#ifdef COMPILE_REGRESSION_TEST
# define FILLMEIN 0
#else
# warning "You must replace the values marked FILLMEIN with real values from the TTN control panel!"
# define FILLMEIN (#dont edit this, edit the lines that use FILLMEIN)
#endif

// LoRaWAN NwkSKey, network session key
// This should be in big-endian (aka msb).
static const PROGMEM u1_t NWKSKEY[16] = { FILLMEIN };

// LoRaWAN AppSKey, application session key
// This should also be in big-endian (aka msb).
static const u1_t PROGMEM APPSKEY[16] = { FILLMEIN };

// LoRaWAN end-device address (DevAddr)
// See http://thethingsnetwork.org/wiki/AddressSpace
// The library converts the address to network byte order as needed, so this
// should be in big-endian (aka msb) too.
static const u4_t DEVADDR = FILLMEIN ; // <-- Change this address for every node!

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in arduino-lmic/project_config/lmic_project_config.h,
// otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static uint8_t mydata[] = "Hello, world!";
static osjob_t sendjob;

// How long to sleep between transmissions (seconds).
// Actual air-time may extend this due to duty-cycle enforcement.
const unsigned TX_INTERVAL = 60;

// Persisted LMIC state: frame counters, RX parameters, sticky MAC ACKs.
// Allocated in RAM; loaded from non-volatile storage on each wake.
static lmic_sleep_state_t sleepState;

// Flag: true if non-volatile storage contained a valid saved state.
static bool hasSavedState = false;

// ---------------------------------------------------------------------------
// Platform stubs — replace with your hardware's NVRAM/EEPROM and sleep API.
// ---------------------------------------------------------------------------

// Save sleepState to non-volatile storage.
// Called from EV_SLEEP_READY before entering deep sleep.
static void saveSleepState() {
    // Example (EEPROM):
    //   EEPROM.put(0, sleepState);
    //   EEPROM.commit();
    //
    // TODO: implement for your platform.
}

// Load previously saved state into sleepState.
// Returns true if valid data was found, false on first boot or after erasure.
static bool loadSleepState() {
    // Example (EEPROM):
    //   EEPROM.get(0, sleepState);
    //   return /* validate magic/CRC */ true;
    //
    // TODO: implement for your platform.
    return false;
}

// Enter deep sleep for the given number of seconds, then reset/wake.
// On many platforms a watchdog timer or RTC alarm triggers a full reset,
// so execution resumes from setup() on the next wake cycle.
static void enterDeepSleep(unsigned seconds) {
    // Example (ESP32):
    //   esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
    //   esp_deep_sleep_start();
    //
    // TODO: implement for your platform.
    // Fallback: busy-wait (for testing on platforms without real sleep support).
    delay((unsigned long)seconds * 1000UL);
    do_send(&sendjob);
}

// ---------------------------------------------------------------------------

// Pin mapping — adjust for your board.
const lmic_pinmap lmic_pins = {
    .nss = 8,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 4,
    .dio = {6, 5, LMIC_UNUSED_PIN},
};

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
                Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
                Serial.print(F("Received "));
                Serial.print(LMIC.dataLen);
                Serial.println(F(" bytes of payload"));
            }
            // Do NOT sleep here. If the network sent MAC commands (e.g.
            // NewChannelReq, RxTimingSetupReq), LMIC needs to send an
            // acknowledgement uplink first. EV_SLEEP_READY fires once
            // all obligations are fulfilled.
            break;
        case EV_SLEEP_READY:
            // All MAC obligations for this wake cycle are done — safe to
            // persist state and power down.
            Serial.println(F("EV_SLEEP_READY"));
            LMIC_getSleepState(&sleepState);
            saveSleepState();
            Serial.flush();
            enterDeepSleep(TX_INTERVAL);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        case EV_TXCANCELED:
            Serial.println(F("EV_TXCANCELED"));
            break;
        case EV_RXSTART:
            /* do not print anything -- it wrecks timing */
            break;
        case EV_JOIN_TXCOMPLETE:
            Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            break;
        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
        Serial.println(F("Packet queued"));
    }
}

void setup() {
    while (!Serial);
    Serial.begin(115200);
    delay(100);
    Serial.println(F("Starting"));

    #ifdef VCC_ENABLE
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);
    #endif

    // LMIC init
    os_init();
    LMIC_reset();

    // Restore session parameters from non-volatile storage.
    // This MUST be called after LMIC_reset() and LMIC_setSession() so that
    // frame counters and RX parameters survive power cycles.
    hasSavedState = loadSleepState();
    if (hasSavedState) {
        Serial.println(F("Restored LMIC sleep state"));
        LMIC_setSleepState(&sleepState);
    }

    // Set static session parameters.
    #ifdef PROGMEM
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession(0x13, DEVADDR, nwkskey, appskey);
    #else
    LMIC_setSession(0x13, DEVADDR, NWKSKEY, APPSKEY);
    #endif

    #if defined(CFG_eu868)
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);
    #elif defined(CFG_us915) || defined(CFG_au915)
    LMIC_selectSubBand(1);
    #elif defined(CFG_as923)
    // ... extra definitions for channels 2..n here
    #elif defined(CFG_kr920)
    // ... extra definitions for channels 3..n here.
    #elif defined(CFG_in866)
    // ... extra definitions for channels 3..n here.
    #else
    # error Region not supported
    #endif

    LMIC_setLinkCheckMode(0);
    LMIC.dn2Dr = DR_SF9;
    LMIC_setDrTxpow(DR_SF7, 14);

    // Start first transmission
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}
