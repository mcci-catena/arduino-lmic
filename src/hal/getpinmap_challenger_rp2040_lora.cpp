/*

Module:  getpinmap_challenger_rp2040_lora.cpp

Function:
        Arduino-LMIC C++ HAL pinmap for iLabs Challenger RP2040 LoRa boards.
        https://ilabs.se/product/challenger-rp2040-lora

Copyright & License:
        See accompanying LICENSE file.

Author:
        Pontus Oldberg, iLabs - August 2022

*/

// Make sure only to compiler when the appropriate board is enabled
#if defined(ARDUINO_CHALLENGER_2040_LORA_RP2040)

#include <arduino_lmic_hal_boards.h>
#include <Arduino.h>

#include "../lmic/oslmic.h"
#include "../hal/hal.h"

namespace Arduino_LMIC {
  class HalConfiguration_Challenger_RP2040_LoRa_t : public HalConfiguration_t {
    public:
    	virtual void begin(void) override {
    		pinMode(RFM95W_SS, OUTPUT);
    		digitalWrite(RFM95W_SS, 1);
    	}

    	// virtual void end(void) override

    	// virtual ostime_t setModuleActive(bool state) override
  };

  static HalConfiguration_Challenger_RP2040_LoRa_t myConfig;

  // Pins are defined in the Arduino variant files
  static const HalPinmap_t myPinmap = {
      .nss = RFM95W_SS,
      .rxtx = LMIC_UNUSED_PIN,
      .rst = RFM95W_RST,
      .dio = {RFM95W_DIO0, RFM95W_DIO1, RFM95W_DIO2},
      .rxtx_rx_active = 0,
      .rssi_cal = 8,              // LBT cal for the Challenger RP2040 LoRa
      .spi = &SPI1,
      .spi_freq = 10000000,
      .pConfig = &myConfig
  };

  const HalPinmap_t *GetPinmap_Challenger_RP2040_LoRa(void) {
  	return &myPinmap;
  }
}; // namespace Arduino_LMIC

#endif  // defined(ARDUINO_CHALLENGER_2040_LORA_RP2040)
