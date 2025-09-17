/*

Module:  getconfig_catena5230.cpp

Function:
        Arduino-LMIC C++ HAL pinmaps for various boards

Copyright & License:
        See accompanying LICENSE file.

Author:
        murali, MCCI       Aug 2025

*/

#if defined(ARDUINO_MCCI_CATENA_5230) || \
    /* legacy names */ \
    defined(ARDUINO_CATENA_5230)

#include <arduino_lmic_hal_boards.h>
#include <Arduino.h>
#include "hal.h"

#include "../lmic/oslmic.h"

namespace Arduino_LMIC {

class HalConfiguration_Catena5230_t : public HalConfiguration_t
        {
public:
        enum DIGITAL_PINS : uint8_t
                {
                PIN_SX1262_NSS = D7,
                PIN_SX1262_NRESET = D8,
                PIN_SX1262_DIO0 = LMIC_UNUSED_PIN,
                PIN_SX1262_DIO1 = D25,
                PIN_SX1262_DIO2 = LMIC_UNUSED_PIN,
                PIN_SX1262_ANT_SWITCH_RX = D26,
                PIN_SX1262_ANT_SWITCH_TX_BOOST = LMIC_UNUSED_PIN,
                PIN_SX1262_ANT_SWITCH_TX_RFO = LMIC_UNUSED_PIN,
                PIN_VDD_BOOST_ENABLE = LMIC_UNUSED_PIN,
                PIN_TCXO_VDD = LMIC_UNUSED_PIN,
                };

        virtual void begin(void) override
                {
                digitalWrite(PIN_TCXO_VDD, 0);
                pinMode(PIN_TCXO_VDD, OUTPUT);
                }

        virtual void end(void) override
                {
                digitalWrite(PIN_TCXO_VDD, 0);
                pinMode(PIN_TCXO_VDD, INPUT);
                }

        virtual bool queryUsingTcxo(void) override { return true; };

        virtual ostime_t setModuleActive(bool state) override
                {
                ostime_t result;
                const int oldState = digitalRead(PIN_TCXO_VDD);

                // if turning on, we need to delay.
                result = 0;
                if (state && ! oldState)
                        result = ms2osticksCeil(3);

                if (state != oldState)
                        digitalWrite(PIN_TCXO_VDD, state);

                return result;
                }
        };

// save some typing by bringing the pin numbers into scope
static HalConfiguration_Catena5230_t myConfig;

static const HalPinmap_t myPinmap =
        {
        .nss = HalConfiguration_Catena5230_t::PIN_SX1262_NSS,      // chip select is D7
        .rxtx = HalConfiguration_Catena5230_t::PIN_SX1262_ANT_SWITCH_RX, // RXTX is D26
        .rst = HalConfiguration_Catena5230_t::PIN_SX1262_NRESET,   // NRESET is D8

        .dio = {LMIC_UNUSED_PIN,
                HalConfiguration_Catena5230_t::PIN_SX1262_DIO1,    // DIO1 (IRQ) is D25
                LMIC_UNUSED_PIN,
               },
        .rxtx_rx_active = 1,
        .rssi_cal = 10,
        .spi_freq = 8000000,     /* 8MHz */
        .pConfig = &myConfig
        };

const HalPinmap_t *GetPinmap_Catena5230(void)
        {
        return &myPinmap;
        }

}; // namespace Arduino_LMIC

#endif /* defined(ARDUINO_CATENA_4611) || defined(ARDUINO_CATENA_5230) */
