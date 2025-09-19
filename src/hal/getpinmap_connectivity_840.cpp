/*

Module:  getpinmap_connectivity_840.cpp

Function:
        Arduino-LMIC C++ HAL pinmap for iLabs Connectivity 840 board.
        

Copyright & License:
        See accompanying LICENSE file.

Author:
        Pontus Oldberg, iLabs - September 2025

*/

// Make sure only to compile when the appropriate board is enabled
#if defined(ARDUINO_CONNECTIVITY_840)

#include <arduino_lmic_hal_boards.h>
#include <Arduino.h>

#include "../lmic/oslmic.h"
#include "../hal/hal.h"

namespace Arduino_LMIC {
  class HalConfiguration_Connectivity_840_t : public HalConfiguration_t {
    public:
    	virtual void begin(void) override {
    		pinMode(PIN_SPI1_CS, OUTPUT);
    		digitalWrite(PIN_SPI1_CS, 1);
    	}

    	// virtual void end(void) override

    	// virtual ostime_t setModuleActive(bool state) override
  };

  static HalConfiguration_Connectivity_840_t myConfig;

  // Pins are defined in the Arduino variant files
  static const HalPinmap_t myPinmap = {
      .nss = PIN_SPI1_CS,
      .rxtx = LMIC_UNUSED_PIN,
      .rst = PIN_LORA_RST,
      .dio = {PIN_LORA_DIO0, PIN_LORA_DIO1, PIN_LORA_DIO2},
      .rxtx_rx_active = 0,
      .rssi_cal = 8,
      .spi = &LORA_SPI,
      .spi_freq = 8000000,
      .pConfig = &myConfig
  };

  const HalPinmap_t *GetPinmap_Connectivity_840(void) {
  	return &myPinmap;
  }
}; // namespace Arduino_LMIC

#endif  // defined(ARDUINO_CONNECTIVITY_840)
