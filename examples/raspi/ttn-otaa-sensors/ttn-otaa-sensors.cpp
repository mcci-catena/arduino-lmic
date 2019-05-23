/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with payload containing
 * value from SI7021 or HTU21D sensor Pressure from BMP180 sensor
 * using frequency and encryption settings matching those of
 * the The Things Network.
 *
 * This uses OTAA (Over-the-air activation), where where a DevEUI and
 * application key is configured, which are used in an over-the-air
 * activation procedure where a DevAddr and session keys are
 * assigned/generated for use with all further communication.
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
 * g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
 * violated by this sketch when left running for longer)!

 * To use this sketch, first register your application and device with
 * the things network, to set or generate an AppEUI, DevEUI and AppKey.
 * Multiple devices can use the same AppEUI, but each device has its own
 * DevEUI and AppKey.
 *
 * Do not forget to define the radio type correctly in LMIC src/config.h.
 *
 * This sample has been written by Charles-Henri Hallard (hallard.me)
 * It's based on original ttn-otaa sample code
 *  
 * Requires bcm2835 library to be already installed
 * http://www.airspayce.com/mikem/bcm2835/
 * use the Makefile in this directory:
 * cd examples/raspi/ttn-otaa-sensors
 * make
 * sudo ./ttn-otaa-sensors
 *
 *******************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
 
#include <lmic.h>
#include <hal/hal.h>

#include "si7021.h"
#include "bmp180.h"

// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,0x70.
static const u1_t PROGMEM APPEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8]={ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// Here on Raspi we use part of MAC Address do define devEUI so 
// This one above is not used, but you can still old method 
// reverting the comments on the 2 following line
//void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}
void os_getDevEui (u1_t* buf) { getDevEuiFromMac(buf); }

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
// The key shown here is the semtech default key.
static const u1_t PROGMEM APPKEY[16] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

// 4 int16_t values 
static uint8_t mydata[4*sizeof(int16_t)] ;
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty)
// cycle limitations).
const unsigned TX_INTERVAL = 120;

//Flag for Ctrl-C
volatile sig_atomic_t force_exit = 0;

// BMP180 library object
SFE_BMP180 bmp180;
#define ALTITUDE  98 // Chasseuil-du-Poitou FR
//#define ALTITUDE 118 // Montamise FR
//#define ALTITUDE 0 // To get sea-leval altitude
static char dev[16]="";


// LoRasPi board 
// see https://github.com/hallard/LoRasPI
#define RF_LED_PIN RPI_V2_GPIO_P1_16 // Led on GPIO23 so P1 connector pin #16
#define RF_CS_PIN  RPI_V2_GPIO_P1_24 // Slave Select on CE0 so P1 connector pin #24
#define RF_IRQ_PIN RPI_V2_GPIO_P1_22 // IRQ on GPIO25 so P1 connector pin #22
#define RF_RST_PIN RPI_V2_GPIO_P1_15 // RST on GPIO22 so P1 connector pin #15

// Pin mapping
const lmic_pinmap lmic_pins = { 
    .nss  = RF_CS_PIN,
    .rxtx = LMIC_UNUSED_PIN,
    .rst  = RF_RST_PIN,
    .dio  = {LMIC_UNUSED_PIN, LMIC_UNUSED_PIN, LMIC_UNUSED_PIN},
};
 
/* ======================================================================
Function: getBMP180Values
Purpose : Return temperature and pressure from BPM180 sensor
Input   : Altitude
Output  : 0 if error otherwise
					pressure * 10 filled with absolute pressure or sea-leval 
          temperature * 100 filled with temperature
Comments: if altitude = 0 then return sea level pressure
          value are returned as int16_t to preserve paylaod size
====================================================================== */
uint8_t getBMP180Values(double altitude, int16_t * pressure, int16_t * temperature)
{
  char status;
  double T,P,p0;

  // You must first get a temperature measurement to perform a pressure reading.
  
  // Start a temperature measurement:
  // If request is successful, the number of ms to wait is returned.
  // If request is unsuccessful, 0 is returned.
  status = bmp180.startTemperature();
  if (status != 0) {
    // Wait for the measurement to complete:
    usleep( status * 1000);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Use '&T' to provide the address of T to the function.
    // Function returns 1 if successful, 0 if failure.
    status = bmp180.getTemperature(T);
    if (status != 0) {
      // Start a pressure measurement:
      // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
      // If request is successful, the number of ms to wait is returned.
      // If request is unsuccessful, 0 is returned.
			
			// Print out the measurement:
      //printf("BMP180 Temperature: %.2f deg C\n", T);
			*temperature = (int16_t) (T*100);

      status = bmp180.startPressure(3);
      if (status != 0) {
        // Wait for the measurement to complete:
        usleep(status*1000);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Use '&P' to provide the address of P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.
        status = bmp180.getPressure(P,T);
				
        if (status != 0) {
					// Print out the measurement:
          //printf("absolute pressure: %.1f mb\n", P );
					
					// The pressure sensor returns abolute pressure, which varies with altitude.
          // To remove the effects of altitude, use the sealevel function and your current altitude.
          // This number is commonly used in weather reports.
          // Parameters: P = absolute pressure in mb, ALTITUDE = current altitude in m.
          // Result: p0 sea-level compensated pressure in mb
					p0 = bmp180.sealevel(P, altitude);
          //printf("relative (sea-level) pressure: %.1f mb\n", p0);

          // On the other hand, if you want to determine your altitude from the pressure reading,
          // use the altitude function along with a baseline pressure (sea-level or other).
          // Parameters: P = absolute pressure in mb, p0 = baseline pressure in mb.
          // Result: altitude in m.
          //printf("computed altitude: %.1f meters\n", bmp180.altitude(P,p0));
					if ( altitude==0) {
						*pressure = (int16_t) (P*10) ;
					} else {
						*pressure = (int16_t) (p0*10);
					}
					return 1;
        } else {
					printf("error retrieving pressure measurement\n");
				}
      } else {
				printf("error starting pressure measurement\n");
			}
		} else {
			printf("error retrieving temperature measurement\n");
		}
	} else {
		printf("error starting temperature measurement\n");
	}
	
	return 0;
}


/* ======================================================================
Function: do_send
Purpose : Measures sensors values and Send a LoraWAN packet 
Input   : osjob_t * 
Output  : -
Comments: -
====================================================================== */
void do_send(osjob_t* j) {
		int16_t bmp_pres, bmp_temp, si_temp, si_hum;
		char strTime[16];
		getSystemTime(strTime , sizeof(strTime));
    printf("%s: ", strTime);
	
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        printf("OP_TXRXPEND, not sending\n");
    } else {
			// Clear our payload buffer
			memset(mydata,0,sizeof(mydata));
			
      printf("Packet queued => ");
			// select BMP180 I2C device
			bcm2835_i2c_setSlaveAddress	(BMP180_ADDR);
			if ( getBMP180Values(ALTITUDE, &bmp_pres, &bmp_temp) != 0 ) {
				printf("BMP180:%.2fC %.1fmb", bmp_temp/100.0f, bmp_pres/10.0f);
				mydata[0] = bmp_temp >> 8; 		// MSB
				mydata[1] = bmp_temp & 0xFF; 	// LSB
				mydata[2] = bmp_pres >> 8; 		// MSB
				mydata[3] = bmp_pres & 0xFF; 	// LSB
			}

			// select SI7021 I2C device
			bcm2835_i2c_setSlaveAddress	(SI7021_I2C_ADDRESS);
			printf("  %s:", dev);
			if (si7021_StartConv(SI7021_READ_TEMP, &si_temp)==BCM2835_I2C_REASON_OK ) {
				printf("%.2fC ", si_temp /100.0f);
				mydata[4] = si_temp >> 8; 		// MSB
				mydata[5] = si_temp & 0xFF; 	// LSB
			}
			if ( si7021_StartConv(SI7021_READ_HUM, &si_hum)==BCM2835_I2C_REASON_OK ) {
				printf("%.1f%%rh", si_hum / 100.0f);
				mydata[6] = si_hum >> 8; 		// MSB
				mydata[7] = si_hum & 0xFF; 	// LSB
			}
			
      printf("\n");
			
      // Prepare upstream data transmission at the next possible time.
      LMIC_setTxData2(1, mydata, sizeof(mydata), 0);
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

/* ======================================================================
Function: onEvent
Purpose : callback fired by LMIC stack for event management
Input   : ev_t 
Output  : -
Comments: -
====================================================================== */
void onEvent (ev_t ev) {
	
	char strTime[16];
	getSystemTime(strTime , sizeof(strTime));
  printf("%s: ", strTime);
 
  switch(ev) {
		
		case EV_JOINED:
			printf("EV_JOINED\n");

			// Disable link check validation (automatically enabled
			// during join, but not supported by TTN at this time).
			LMIC_setLinkCheckMode(0);
		break;
		
		case EV_TXCOMPLETE:
			printf("EV_TXCOMPLETE (includes waiting for RX windows)\n");
			if (LMIC.txrxFlags & TXRX_ACK)
				printf("%s Received ack\n", strTime);
			if (LMIC.dataLen) {
				printf("%s Received %d bytes of payload\n", strTime, LMIC.dataLen);
			}
			// Light Off LED
			digitalWrite(RF_LED_PIN, LOW);
			// Schedule next transmission
			os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
		break;

		case EV_SCAN_TIMEOUT: 	printf("EV_SCAN_TIMEOUT\n");	break;
		case EV_BEACON_FOUND:		printf("EV_BEACON_FOUND\n");	break;
		case EV_BEACON_MISSED:	printf("EV_BEACON_MISSED\n");	break;
		case EV_BEACON_TRACKED:	printf("EV_BEACON_TRACKED\n");break;
		case EV_JOINING:				printf("EV_JOINING\n"); 			break;
		case EV_RFU1: 					printf("EV_RFU1\n");					break;
		case EV_JOIN_FAILED:		printf("EV_JOIN_FAILED\n");		break;
		case EV_REJOIN_FAILED:	printf("EV_REJOIN_FAILED\n"); break;
		case EV_LOST_TSYNC:			printf("EV_LOST_TSYNC\n");		break;
		case EV_RESET:					printf("EV_RESET\n");					break;
		case EV_RXCOMPLETE:			printf("EV_RXCOMPLETE\n");		break;
		case EV_LINK_DEAD:			printf("EV_LINK_DEAD\n");			break;
		case EV_LINK_ALIVE:			printf("EV_LINK_ALIVE\n");		break;
		default:	
			printf("Unknown event\n");	
		break;
  }
}


/* ======================================================================
Function: sig_handler
Purpose : Intercept CTRL-C keyboard to close application
Input   : signal received
Output  : -
Comments: -
====================================================================== */
void sig_handler(int sig)
{
  printf("\nBreak received, exiting!\n");
  force_exit=true;
}

/* ======================================================================
Function: main
Purpose : Main routine
Input   : -
Output  : -
Comments: -
====================================================================== */
int main ()
{
	// caught CTRL-C to do clean-up
  signal(SIGINT, sig_handler);
    
  printf("%s Starting\n", __BASEFILE__);

	// Display Hardware RFM95 configuration
	printConfig(RF_LED_PIN);
  printKeys();

	// Init GPIO bcm
	if (!bcm2835_init()) {
		fprintf( stderr, "bcm2835_init() Failed\n\n" );
		return 1;
	}

	// Light on LED
	pinMode(RF_LED_PIN, OUTPUT);
	digitalWrite(RF_LED_PIN, HIGH);
	
	if (!bcm2835_i2c_begin()) {
		fprintf( stderr, "bcm2835_i2c_begin() failed. Are you running as root??\n");
		return 1;
	}
	
	// Set I2C speed to 100KHz
	bcm2835_i2c_set_baudrate(100000);

	// Init BMP180 device and check it's here
  printf("Checking BMP180 device...");
	if (bmp180.begin()) {
    printf("found\n");
	} else {
    printf("fail, check wiring\n\n");
  }
	
	// select SI7021 I2C device
  printf("Checking SI7021 or HTU21D device...");
	bcm2835_i2c_setSlaveAddress	(SI7021_I2C_ADDRESS);
	uint8_t id = si7021_getID();
	// Highest resolution and check device is here
	if ( id == ID_SI7021 ) {
		strcpy(dev, "SI7021");
    printf("%s found\n", dev );

	} else if ( id == ID_HTU21D ) {
		strcpy(dev, "HTU21D");
    printf("%s found\n", dev);
	} else {
		strcpy(dev, "Error");
    printf("No SI7021 or HTU21D detected, check wiring\n\n");
  }
	
  // LMIC init
  os_init();

  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
	
  // Start job (sending automatically starts OTAA too)
	// Then on transmit will reset it's own send 
  do_send(&sendjob);

	// Main loop until CTRL-C
	while (!force_exit) {

		os_runloop_once();
      
		// We're on a multitasking OS let some time for others
		// Without this one CPU is 99% and with this one just 3%
		// On a Raspberry PI 3
		usleep(1000);
	}
	
	// We're here because we need to exit, do it clean

  // Light off on board LED
  digitalWrite(RF_LED_PIN, LOW);
	
	// Release I2C and BCM2835
	bcm2835_i2c_end();  
  bcm2835_close();	
	return 0;
}
