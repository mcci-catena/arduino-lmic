/*

Module:  getpinmap_challengerzp_rp2350_lora.cpp

Function:
        Arduino-LMIC C++ HAL pinmap for iLabs ChallengerZP RP2350 LoRa board.
        

Copyright & License:
        See accompanying LICENSE file.

Author:
        Pontus Oldberg, iLabs - March 2026

*/

// Make sure only to compiler when the appropriate board is enabled
#if defined(ARDUINO_CHALLENGERZP_2350_LORA_RP2350)

#include <arduino_lmic_hal_boards.h>
#include <Arduino.h>

#include "../lmic/oslmic.h"
#include "../hal/hal.h"

namespace Arduino_LMIC {
  class HalConfiguration_ChallengerZP_RP2350_LoRa_t : public HalConfiguration_t {
    public:
        enum DIGITAL_PINS : uint8_t {
                SX1262_NSS = PIN_SX1262_SS,
                SX1262_NRESET = PIN_SX1262_RSTN,
                SX1262_BUSY = PIN_SX1262_BUSY,
                SX1262_DIO1 = PIN_SX1262_DIO1,
                SX1262_DIO2 = HalPinmap_t::UNUSED_PIN,
                SX1262_DIO3 = HalPinmap_t::UNUSED_PIN,
                SX1262_ANT_SWITCH_RX = HalPinmap_t::UNUSED_PIN,
                SX1262_ANT_SWITCH_TX_BOOST = HalPinmap_t::UNUSED_PIN,
                SX1262_ANT_SWITCH_TX_RFO = HalPinmap_t::UNUSED_PIN,
                VDD_BOOST_ENABLE = HalPinmap_t::UNUSED_PIN,
        };


    	virtual void begin(void) override {
    		pinMode(HalConfiguration_ChallengerZP_RP2350_LoRa_t::SX1262_NSS, OUTPUT);
    		digitalWrite(HalConfiguration_ChallengerZP_RP2350_LoRa_t::SX1262_NSS, 1);
    	}

        virtual u1_t queryBusyPin(void) override { return HalConfiguration_ChallengerZP_RP2350_LoRa_t::SX1262_BUSY; };
        
        virtual bool queryUsingDcdc(void) override { return true; };

        virtual bool queryUsingDIO2AsRfSwitch(void) override { return true; };

        virtual bool queryUsingDIO3AsTCXOSwitch(void) override { return false; };
  };

  static HalConfiguration_ChallengerZP_RP2350_LoRa_t myConfig;

  // Pins are mapped in enum from the definition in the Arduino variant files
  static const HalPinmap_t myPinmap = {
      .nss = HalConfiguration_ChallengerZP_RP2350_LoRa_t::SX1262_NSS,
      .rxtx = LMIC_UNUSED_PIN,
      .rst = HalConfiguration_ChallengerZP_RP2350_LoRa_t::SX1262_NRESET,
      .dio = {
        HalPinmap_t::UNUSED_PIN,  // DIO0 (not used by SX1262)
        HalConfiguration_ChallengerZP_RP2350_LoRa_t::SX1262_DIO1,  // DIO1 (RX/TX interrupt)
        HalPinmap_t::UNUSED_PIN   // DIO2
      },
      .rxtx_rx_active = 0,
      .rssi_cal = 8,              // LBT cal for the Challenger RP2040 LoRa
      .spi = &SPI1,
      .spi_freq = 10000000,
      .pConfig = &myConfig
  };

  const HalPinmap_t *GetPinmap_ChallengerZP_RP2350_LoRa(void) {
  	return &myPinmap;
  }
}; // namespace Arduino_LMIC

#endif  // defined(ARDUINO_CHALLENGERZP_2350_LORA_RP2350)
