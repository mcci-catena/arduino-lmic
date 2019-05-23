/*
	si7021.c
	SI7021 Temperature and Humidity sensor for Raspberry PI
	Charles-Henri Hallard from http://ch2i.eu

	version 1.0 2013/09/20 initial version
	Verison 1.1.2 - Updated for Arduino 1.6.4 5/2015
	Version 1.1.3 - Updated for Raspberry PI by Charles-Henri Hallard (hallard.me)
	
	Our example code uses the "beerware" license. You can do anything
	you like with this code. No really, anything. If you find it useful,
	buy me a (root) beer someday.
	
	August 2016 - Charles-Henri Hallard : Addapted to Raspberry PI

	Requires bcm2835 library to be already installed
  http://www.airspayce.com/mikem/bcm2835/
 
*/

#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#include <bcm2835.h>

// device ID identifier
#define ID_SI7021 0x15
#define ID_HTU21D 0x32

// ======================================
// SI7021 sensor 
// ======================================
#define SI7021_I2C_ADDRESS    0x40 // I2C address for the sensor
#define SI7021_MEASURE_TEMP0  0xE0 // Can be read only after a RH conversion done
#define SI7021_MEASURE_TEMP   0xE3 // Default hold
#define SI7021_MEASURE_HUM    0xE5 // Default hold
#define SI7021_MEASURE_NOHOLD 0x80 // NO HOLD Bit flag
#define SI7021_WRITE_REG      0xE6
#define SI7021_READ_REG       0xE7
#define SI7021_SOFT_RESET     0xFE
#define SI7021_READ_1ST_ID_1  0xFA
#define SI7021_READ_1ST_ID_2  0x0F
#define SI7021_READ_2ND_ID_1  0xFC
#define SI7021_READ_2ND_ID_2  0xC9
#define SI7021_READ_FW_REV_1  0x84
#define SI7021_READ_FW_REV_2  0xB8

// SI7021 Sensor resolution
// default at power up is SI7021_RESOLUTION_14T_12RH
#define SI7021_RESOLUTION_14T_12RH 0x00 // 12 bits RH / 14 bits Temp
#define SI7021_RESOLUTION_13T_10RH 0x80 // 10 bits RH / 13 bits Temp
#define SI7021_RESOLUTION_12T_08RH 0x01 //  8 bits RH / 12 bits Temp
#define SI7021_RESOLUTION_11T_11RH 0x81 // 11 bits RH / 11 bits Temp

#define SI7021_RESOLUTION_MASK 0B01111110

// The type of measure we want to trigger on sensor
typedef enum {
  SI7021_READ_TEMP,
  SI7021_READ_HUM
} 
si7021_e;

// SI7021 temperature / humidity sensor related
uint8_t si7021_StartConv(si7021_e datatype, int16_t * value);
uint8_t si7021_readValues(int32_t * value);
uint8_t si7021_setResolution(uint8_t res);
uint8_t si7021_getID(void);

double getHumidity(void);
double getTemperature(void);
