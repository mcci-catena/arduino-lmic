/*

Module: compliance-otaa-halconfig.ino

Function:
    Test program for developing and checking LMIC compliance test support.

Copyright and License:
    Please see accompanying LICENSE file.

Author:
    Terry Moore, MCCI Corporation   March 2019

Description:
    This sketch demonstrates use of the LMIC compliance-test mode. Tested
    with an RWC5020A LoRaWAN Tester.

*/

#include <Arduino.h>

#include <arduino_lmic.h>
#include <arduino_lmic_hal_boards.h>
#include <arduino_lmic_lorawan_compliance.h>

#include <SPI.h>

//
// For compliance tests with the RWC5020A, we use the default addresses
// from the tester; except that we use APPKEY 0,..., 0, 2, to avoid
// collisions with a registered app on TTN.
//

// This EUI must be in little-endian format, so least-significant-byte
// first.  This corresponds to 0x0000000000000001
static const u1_t PROGMEM APPEUI[8]= { 1, 0, 0, 0, 0, 0, 0, 0 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// This should also be in little endian format, see above.
// This corresponds to 0x0000000000000001
static const u1_t PROGMEM DEVEUI[8]= { 1, 0, 0, 0, 0, 0, 0, 0 };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). 
static const u1_t PROGMEM APPKEY[16] = { 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 2 };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

static uint8_t mydata[] = "Hello, world!";
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 5;

// global flag for test mode.
bool g_fTestMode = false;

// forward declarations
lmic_event_cb_t myEventCb;
lmic_rxmessage_cb_t myRxMessageCb;


/*

Name:	myEventCb()

Function:
        lmic_event_cb_t myEventCb;

        extern "C" { void myEventCb(void *pUserData, ev_t ev); }

Description:
        This function is registered for event notifications from the LMIC
        during setup() processing. Its main job is to display events in a
        user-friendly way.

Returns:
        No explicit result.

*/

uint8_t lastTxChannel;
bool lastTxStart;

void myEventCb(void *pUserData, ev_t ev) {
    if (ev == EV_TXSTART) {
        lastTxStart == true;
        Serial.print(F("."));
    } else {
        if (lastTxStart) {
            Serial.println();
            lastTxStart = false;
        }
        Serial.print(os_getTime());
        Serial.print(F(": "));
    }
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

            // this is our moment to set up pre-join network parameters.
            setupForNetwork();
            break;

        case EV_JOINED:
            Serial.print(F("EV_JOINED: ch "));
            Serial.println(lastTxChannel);
            {
              u4_t netid = 0;
              devaddr_t devaddr = 0;
              u1_t nwkKey[16];
              u1_t artKey[16];
              LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
              Serial.print("netid: ");
              Serial.println(netid, DEC);
              Serial.print("devaddr: ");
              Serial.println(devaddr, HEX);
              Serial.print("artKey: ");
              for (int i=0; i<sizeof(artKey); ++i) {
                if (i != 0)
                  Serial.print("-");
                Serial.print(artKey[i], HEX);
              }
              Serial.println("");
              Serial.print("nwkKey: ");
              for (int i=0; i<sizeof(nwkKey); ++i) {
                      if (i != 0)
                              Serial.print("-");
                      Serial.print(nwkKey[i], HEX);
              }
              Serial.println("");
            }
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_RFU1:
        ||     Serial.println(F("EV_RFU1"));
        ||     break;
        */
        case EV_JOIN_FAILED:
            // this event is just advisory (though it looks fearsome) --
            // we've just wrapped through all our join cycles, and
            // we will start over. You could use this to disable the radio
            // for a while.
            Serial.println(F("EV_JOIN_FAILED"));
            break;

        case EV_REJOIN_FAILED:
            // this event means that someone tried a rejoin, and it failed.
            // it doesn't really mean anything bad, it's just advisory.
            Serial.println(F("EV_REJOIN_FAILED"));
            break;

        case EV_TXCOMPLETE:
            Serial.print(F("EV_TXCOMPLETE: ch "));
            Serial.print(lastTxChannel);
            if (LMIC.txrxFlags & TXRX_ACK)
                Serial.print(F("; Received ack"));
            Serial.println(F("."));
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            // this event means that we saturated the uplink or downlink counter.
            // this forces the entire LMIC to be reset. A join is necessary. If
            // join is not configured, then time is up for this device.
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_SCAN_FOUND:
        ||    Serial.println(F("EV_SCAN_FOUND"));
        ||    break;
        */
        case EV_TXSTART:
            // this event tells us that a transmit is about to start.
            // but printing here is bad for timing.
            lastTxChannel = LMIC.txChnl;
            break;
        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

/*

Name:   myRxMessageCb()

Function:
        Handle received LoRaWAN downlink messages.

Definition:
        lmic_rxmessage_cb_t myRxMessageCb;

        extern "C" {
            void myRxMessageCb(
                void *pUserData,
                uint8_t port,
                const uint8_t *pMessage,
                size_t nMessage
                ); 
        }

Description:
        This function is called whenever a non-Join downlink message
        is received over LoRaWAN by LMIC. Its job is to invoke the
        compliance handler (if compliance support is needed), and
        then decode any non-compliance messages.

Returns:
        No explicit result.

*/

void myRxMessageCb(
    void *pUserData,
    uint8_t port,
    const uint8_t *pMessage,
    size_t nMessage
) {
    lmic_compliance_rx_action_t const action = LMIC_complianceRxMessage(port, pMessage, nMessage);
    switch (action) {
        case LMIC_COMPLIANCE_RX_ACTION_START: {
            Serial.println(F("Enter test mode"));
            os_clearCallback(&sendjob);
            g_fTestMode = true;
            return;
        }
        case LMIC_COMPLIANCE_RX_ACTION_END: {
            Serial.println(F("Exit test mode"));
            g_fTestMode = false;
            // we're in the LMIC, we don't want to send from here. Schedule a job.
            os_setTimedCallback(&sendjob, os_getTime() + ms2osticks(10), do_send);
            return;
        }
        case LMIC_COMPLIANCE_RX_ACTION_IGNORE: {
            if (port == LORAWAN_PORT_COMPLIANCE) {
                Serial.print(F("Received test packet "));
                if (nMessage > 0)
                    Serial.print(pMessage[0], HEX);
                Serial.print(F(" length 0x"));
                Serial.println((unsigned) nMessage);
            }
            return;
        }
        default:
            // continue.
            break;
    }

    Serial.print(F("Received message on port "));
    Serial.print(port);
    Serial.print(F(": "));
    Serial.print(unsigned(nMessage));
    Serial.println(F(" bytes"));
    }

lmic_txmessage_cb_t sendComplete;

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else if (g_fTestMode) {
        Serial.println(F("test mode, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        if (LMIC_sendWithCallback(1, mydata, sizeof(mydata)-1, 0, sendComplete, j) == 0) {
            Serial.println(F("Packet queued"));
        } else {
            Serial.println(F("Packet queue failure; sleeping"));
            sendComplete(j, 0);
        }
    }
}

void sendComplete(
        void *pUserData,
        int fSuccess
) {
    osjob_t * const j = (osjob_t *) pUserData;

    if (! fSuccess)
        Serial.println(F("sendComplete: uplink failed"));

    if (! g_fTestMode) {
            // Schedule next transmission
            os_setTimedCallback(j, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
    }        
}

void myFail(const char *pMessage) {
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) {
        // alert
        Serial.println(pMessage);
        // flash lights, sleep.
        for (int i = 0; i < 5; ++i) {
            digitalWrite(LED_BUILTIN, 1);
            delay(100);
            digitalWrite(LED_BUILTIN, 0);
            delay(900);
        }
    }
}

void setup() {
    delay(5000);
    while (! Serial)
        ;
    Serial.begin(115200);
    Serial.println(F("Starting"));

    // LMIC init using the computed target
    const auto pPinMap = Arduino_LMIC::GetPinmap_ThisBoard();

    // don't die mysteriously; die noisily.
    if (pPinMap == nullptr) {
        myFail("board not known to library; add pinmap or update getconfig_thisboard.cpp");
    }

    // now that we have a pinmap, initalize the low levels accordingly.
    os_init_ex(pPinMap);

    // LMIC_reset() doesn't affect callbacks, so we can do this first.
    if (! (LMIC_registerRxMessageCb(myRxMessageCb, /* userData */ nullptr) && 
           LMIC_registerEventCb(myEventCb, /* userData */ nullptr))) {
        myFail("couldn't register callbacks");
    }

    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // do the network-specific setup.
    setupForNetwork();

    // Start job (sending automatically starts OTAA too)
    do_send(&sendjob);
}

void setupForNetwork(void) {
#if defined(CFG_us915)
    LMIC_setLinkCheckMode(0);
    LMIC_setDrTxpow(DR_SF7, 14);
    LMIC_selectSubBand(1);
#endif
}

void loop() {
    os_runloop_once();
}