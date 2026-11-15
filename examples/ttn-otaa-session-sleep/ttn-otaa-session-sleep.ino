/*******************************************************************************
 * Copyright (c) 2026 MCCI Corporation
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example joins a LoRaWAN network with OTAA and sends "Hello, world!"
 * every TX_INTERVAL seconds. It shows three things the other examples
 * don't:
 *
 * - Saving the session state (LMIC_saveSessionState()) and restoring it
 *   after a restart, so the device doesn't join again, the frame counters
 *   carry on, and no DevNonce repeats.
 * - Handling EV_DEVNONCE_EXHAUSTED.
 * - Sleeping between LMIC jobs with os_runloop_once_ex().
 *
 * The sketch keeps the saved state with stateSave() and stateLoad(). On
 * ESP32 they use the Preferences library, so the state survives a reset
 * or a power failure. On other boards they keep it in RAM, which a reset
 * clears; a device in the field replaces them with code for its own
 * non-volatile memory (EEPROM, FRAM or flash). To show a restore on any
 * board, the sketch restarts the LMIC every RESTART_EVERY uplinks: it
 * calls LMIC_reset() and restores the saved state, as setup() does after a
 * reset.
 *
 * It takes the pin map from the board support package
 * (GetPinmap_ThisBoard()), so it runs on any board that the LMIC's HAL
 * configuration knows.
 *
 * Note: the LMIC enforces the regional duty-cycle limits, but not the
 * TTN fair use policy, which this sketch probably breaks when left
 * running for long.
 *
 * To use this sketch, register the device with your network to get a
 * JoinEUI (AppEUI), DevEUI and AppKey.
 *
 * Do not forget to define the radio type correctly in
 * arduino-lmic/project_config/lmic_project_config.h or from your BOARDS.txt.
 *
 *******************************************************************************/

#include <arduino_lmic.h>
#include <hal/hal.h>
#include <arduino_lmic_hal_boards.h>
#include <SPI.h>

#if defined(ARDUINO_ARCH_ESP32)
# include <Preferences.h>
#endif

//
// For normal use, we require that you edit the sketch to replace FILLMEIN
// with values assigned by the network console. However, for regression
// tests, we want to be able to compile these scripts. The regression tests
// define COMPILE_REGRESSION_TEST, and in that case we define FILLMEIN to a
// non-working but innocuous value.
//
#ifdef COMPILE_REGRESSION_TEST
# define FILLMEIN 0
#else
# warning "You must replace the values marked FILLMEIN with real values from the network console!"
# define FILLMEIN (#dont edit this, edit the lines that use FILLMEIN)
#endif

// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from the network console, reverse the bytes.
static const u1_t PROGMEM APPEUI[8] = { FILLMEIN };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8); }

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8] = { FILLMEIN };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8); }

// This key is in big endian format; copy it from the console as it is.
static const u1_t PROGMEM APPKEY[16] = { FILLMEIN };
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16); }

static uint8_t mydata[] = "Hello, world!";
static osjob_t sendjob;

// Send every this many seconds (might become longer due to duty cycle
// limitations).
const unsigned TX_INTERVAL = 60;

// Restart the LMIC from the saved state after this many uplinks; 0 never.
const unsigned RESTART_EVERY = 3;

/****************************************************************************\
|
|   Saved state
|
\****************************************************************************/

// The session state blob doesn't hold the NetID, DevAddr or session keys,
// so the sketch saves them next to it.
struct SavedState {
    uint32_t    magic;          // kMagic if the rest is valid
    uint32_t    netid;
    uint32_t    devaddr;        // 0 if not joined
    uint8_t     nwkKey[16];
    uint8_t     artKey[16];
    uint8_t     blob[LMIC_SESSION_STATE_SIZE];
};

static const uint32_t kMagic = 0x4C4D5331;     // "LMS1"
static SavedState gState;
static unsigned gUplinks;

#if defined(ARDUINO_ARCH_ESP32)

static Preferences gPrefs;

static bool stateLoad(void *pBuf, size_t nBuf) {
    gPrefs.begin("lmic", true);
    size_t const n = gPrefs.getBytes("state", pBuf, nBuf);
    gPrefs.end();
    return n == nBuf;
}

// Each save writes flash. The ESP32 NVS spreads the writes, but a device
// that sends often should save less often than every uplink.
static void stateSave(const void *pBuf, size_t nBuf) {
    gPrefs.begin("lmic", false);
    gPrefs.putBytes("state", pBuf, nBuf);
    gPrefs.end();
}

#else // ! ARDUINO_ARCH_ESP32

// Replace these with code for the board's non-volatile memory. The RAM
// copy is lost at reset. nBuf is sizeof(SavedState).
static SavedState gRamCopy;

static bool stateLoad(void *pBuf, size_t nBuf) {
    memcpy(pBuf, &gRamCopy, nBuf);
    return true;
}

static void stateSave(const void *pBuf, size_t nBuf) {
    memcpy(&gRamCopy, pBuf, nBuf);
}

#endif // ! ARDUINO_ARCH_ESP32

// Save the session state blob, and the keys saved at EV_JOINED.
static void saveSession(void) {
    size_t nUsed;
    lmic_session_state_result_t const result =
        LMIC_saveSessionState(gState.blob, sizeof(gState.blob), &nUsed, 1);

    if (result != LMIC_SESSION_STATE_OK) {
        Serial.print(F("LMIC_saveSessionState() failed: "));
        Serial.println(unsigned(result));
        return;
    }
    gState.magic = kMagic;
    stateSave(&gState, sizeof(gState));
}

// Reset the LMIC, then restore the saved state if there is any. setup()
// calls this after os_init_ex(), and restartLmic() after a run of uplinks.
static void startLmic(void) {
    // 1. Reset the MAC.
    LMIC_reset();

    bool fRestored = false;

    if (stateLoad(&gState, sizeof(gState)) && gState.magic == kMagic) {
        // 2. If the device had joined, set the session first:
        //    LMIC_setSession() resets the frame counters and channels.
        if (gState.devaddr != 0)
            LMIC_setSession(gState.netid, gState.devaddr, gState.nwkKey, gState.artKey);

        // 3. Restore the blob.
        lmic_session_state_result_t const result =
            LMIC_restoreSessionState(gState.blob, sizeof(gState.blob));

        if (result == LMIC_SESSION_STATE_OK) {
            fRestored = true;
            Serial.print(F("Restored: "));
            if (gState.devaddr != 0) {
                Serial.print(F("DevAddr "));
                Serial.print(gState.devaddr, HEX);
                Serial.print(F(", FCntUp "));
                Serial.println(LMIC_getFCntUp());
            } else {
                Serial.print(F("not joined, next DevNonce "));
                Serial.println(LMIC_getNextDevNonce());
            }
        } else {
            Serial.print(F("LMIC_restoreSessionState() failed: "));
            Serial.println(unsigned(result));
            // start again from nothing
            LMIC_reset();
            memset(&gState, 0, sizeof(gState));
        }
    } else {
        memset(&gState, 0, sizeof(gState));
    }

    if (! fRestored) {
        Serial.println(F("No saved state; the device will join"));
#if CFG_LMIC_US_like
        // Start on sub-band 2 (channels 8-15; LMIC_selectSubBand() counts
        // from 0), which TTN uses in US915 and AU915. Other networks may
        // use another sub-band. A restored session has its own channels.
        LMIC_selectSubBand(1);
#endif
    }

    // 4. Set again anything that LMIC_reset() returns to its default and
    // the blob doesn't hold, for example LMIC_setAdrMode() or
    // LMIC_setAdrAckLimits(). This sketch uses the defaults.
}

/****************************************************************************\
|
|   Events and uplinks
|
\****************************************************************************/

void do_send(osjob_t* j);

static void restartLmic(osjob_t* j) {
    Serial.println(F("Restarting the LMIC from the saved state"));
    startLmic();
    do_send(j);
}

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            LMIC_getSessionKeys(&gState.netid, &gState.devaddr, gState.nwkKey, gState.artKey);
            saveSession();
            break;
        case EV_JOIN_FAILED:
            // The LMIC keeps joining, with back-off; LMIC_stopJoining()
            // stops it.
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXSTART:
            // While joining, save the state here: the LMIC has used the
            // DevNonce and the radio hasn't sent it yet, so it can't
            // repeat after a power failure.
            if (LMIC_getOpmode() & OP_JOINING)
                saveSession();
            Serial.println(F("EV_TXSTART"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC_getTxrxFlags() & TXRX_ACK)
                Serial.println(F("Received ack"));
            if (LMIC_getFRMPayloadLen()) {
                Serial.print(F("Received "));
                Serial.print(LMIC_getFRMPayloadLen());
                Serial.println(F(" bytes of payload"));
            }
            saveSession();
            Serial.print(F("Saved: FCntUp "));
            Serial.println(LMIC_getFCntUp());

            if (RESTART_EVERY != 0 && ++gUplinks % RESTART_EVERY == 0)
                os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), restartLmic);
            else
                os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_TXCANCELED:
            Serial.println(F("EV_TXCANCELED"));
            // The uplink was dropped and no EV_TXCOMPLETE follows.
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_RXSTART:
            /* do not print anything -- it wrecks timing */
            break;
        case EV_JOIN_TXCOMPLETE:
            Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            break;
        case EV_DEVNONCE_EXHAUSTED:
            // Every DevNonce has been used, and the LMIC has stopped
            // joining. The device needs a new AppKey or JoinEUI, and then
            // LMIC_clearJoinState(). Save the state, so the device doesn't
            // try again after a reset.
            Serial.println(F("EV_DEVNONCE_EXHAUSTED"));
            saveSession();
            break;
        default:
            Serial.print(F("Event "));
            Serial.println((unsigned) ev);
            break;
    }
}

void do_send(osjob_t* j) {
    // LMIC_queryTxReady() is false while an uplink is queued or in progress,
    // and then EV_TXCOMPLETE schedules the next one.
    if (! LMIC_queryTxReady()) {
        Serial.println(F("LMIC busy, not sending"));
        return;
    }

    lmic_tx_error_t const err = LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
    if (err == LMIC_ERROR_SUCCESS) {
        Serial.println(F("Packet queued"));
    } else {
        // EV_TXCOMPLETE might not follow, so schedule the next try here.
        Serial.print(F("LMIC_setTxData2() failed: "));
        Serial.println(err);
        os_setTimedCallback(j, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
    }
}

/****************************************************************************\
|
|   Setup and the run loop
|
\****************************************************************************/

void setup() {
    delay(5000);
    // Wait for a USB host to open the serial port, so that the log starts
    // at the beginning. A board with no USB host stops here; remove the
    // wait for a device that runs on its own.
    while (! Serial)
        ;
    Serial.begin(115200);
    Serial.println(F("Starting"));

    const lmic_pinmap *pPinMap = Arduino_LMIC::GetPinmap_ThisBoard();

    // don't die mysteriously; die noisily.
    if (pPinMap == nullptr) {
        pinMode(LED_BUILTIN, OUTPUT);
        for (;;) {
            for (int i = 0; i < 5; ++i) {
                digitalWrite(LED_BUILTIN, 1);
                delay(100);
                digitalWrite(LED_BUILTIN, 0);
                delay(900);
            }
            Serial.println(F("board not known to library; add pinmap or update getconfig_thisboard.cpp"));
        }
    }

    os_init_ex(pPinMap);

    // If the board's clock is less accurate than the LMIC assumes, the LMIC
    // misses its receive windows and never joins. Such a board needs
    // LMIC_setClockError() here; see doc/timing.md. For example, an MCCI
    // Catena 4612 needs LMIC_setClockError(4 * MAX_CLOCK_ERROR / 1000)
    // unless the sketch calibrates its system clock.

    startLmic();

    // The first uplink also starts the join, if the device hasn't joined.
    do_send(&sendjob);
}

// Sleep until the next LMIC job, or until an interrupt. Called with
// interrupts masked by lmic_hal_disableIRQs().
//
// On Arm Cortex-M, __WFI() wakes when an interrupt becomes pending, even
// while it is masked, and the interrupt runs at lmic_hal_enableIRQs(). The
// system tick wakes it every millisecond, so this saves only some power;
// it keeps micros(), and so os_getTime(), running. A board with a low-power
// timer would arm it for `ticks` (no more than 30 minutes, and none for
// OS_NEXT_JOB_NEVER) and enter a deeper sleep, but must then keep
// os_getTime() counting, or the LMIC misses its receive windows.
//
// On other processors this does nothing, so the loop runs without sleeping.
static void sleepUpTo(u4_t ticks) {
    (void) ticks;
#if defined(__arm__)
    __WFI();
#endif
}

void loop() {
    u4_t const ticks = os_runloop_once_ex();

    if (ticks == 0)
        return;                 // a job is ready, or the radio is busy

    // the application's own work goes here; it can lower ticks

    lmic_hal_disableIRQs();     // no interrupt between the check and the sleep
    if (! lmic_hal_hasPendingIrqs())
        sleepUpTo(ticks);
    lmic_hal_enableIRQs();
}
