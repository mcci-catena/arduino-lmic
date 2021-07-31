/*

Module:  getconfig_ttgo_t-beam_t22_v07.cpp

Function:
        Arduino-LMIC C++ HAL pinmap for TTGO T-Beam T22_V07

Copyright & License:
        See accompanying LICENSE file.

Author:
        José Manuel Mariño Mariño, jm@marinho.com.es    July 2021
	based on previous file by German Martin, gmag11@gmail.com   June 2019

*/

#if defined(ARDUINO_TTGO_TBeam_T22_V07)

#include <arduino_lmic_hal_boards.h>
#include <Arduino.h>

#include "../lmic/oslmic.h"

#define LORA_DIO0 26
#define LORA_DIO1 33
#define LORA_DIO2 32

namespace Arduino_LMIC {

class HalConfiguration_ttgo_tbeam_t22_v07 : public HalConfiguration_t
	{
public:
	enum DIGITAL_PINS : uint8_t
		{
		PIN_SX1276_NSS = 18,
		PIN_SX1276_NRESET = 23,
		PIN_SX1276_DIO0 = LORA_DIO0,
		PIN_SX1276_DIO1 = LORA_DIO1,
		PIN_SX1276_DIO2 = LORA_DIO2,
		PIN_SX1276_ANT_SWITCH_RX = HalPinmap_t::UNUSED_PIN,
		PIN_SX1276_ANT_SWITCH_TX_BOOST = HalPinmap_t::UNUSED_PIN,
		PIN_SX1276_ANT_SWITCH_TX_RFO = HalPinmap_t::UNUSED_PIN,
		PIN_VDD_BOOST_ENABLE = HalPinmap_t::UNUSED_PIN,
		};

	virtual void begin(void) override
		{
		digitalWrite(PIN_SX1276_NSS, 1);
		pinMode(PIN_SX1276_NSS, OUTPUT);
		}

	// virtual void end(void) override

	// virtual ostime_t setModuleActive(bool state) override

	};

static HalConfiguration_ttgo_tbeam_t22_v07 myConfig;

static const HalPinmap_t myPinmap =
        {
        .nss = HalConfiguration_ttgo_tbeam_t22_v07::PIN_SX1276_NSS,
        .rxtx = HalConfiguration_ttgo_tbeam_t22_v07::PIN_SX1276_ANT_SWITCH_RX,
        .rst = HalConfiguration_ttgo_tbeam_t22_v07::PIN_SX1276_NRESET,

        .dio = {HalConfiguration_ttgo_tbeam_t22_v07::PIN_SX1276_DIO0,
				HalConfiguration_ttgo_tbeam_t22_v07::PIN_SX1276_DIO1,
				HalConfiguration_ttgo_tbeam_t22_v07::PIN_SX1276_DIO2,
               },
        .rxtx_rx_active = 0,
        .rssi_cal = 10,
        .spi_freq = 8000000,     /* 8MHz */
        .pConfig = &myConfig
        };

const HalPinmap_t * GetPinmap_ttgo_tbeam_t22_v07 (void)
	{
	return &myPinmap;
	}

}; // namespace Arduino_LMIC

#endif // ARDUINO_TTGO_TBeam_T22_V07
