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

#include "si7021.h"

/* ======================================================================
Function: si7021_checkCRC
Purpose : check the CRC of received data
Input   : value read from sensor
Output  : CRC read from sensor
Comments: 0 if okay
====================================================================== */
uint8_t si7021_checkCRC(uint16_t data, uint8_t check)
{
  uint32_t remainder, divisor;

  //Pad with 8 bits because we have to add in the check value
  remainder = (uint32_t)data << 8; 

  // From: http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
  // POLYNOMIAL = 0x0131 = x^8 + x^5 + x^4 + 1 : http://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks
  // 0x988000 is the 0x0131 polynomial shifted to farthest left of three bytes
  divisor = (uint32_t) 0x988000;

  // Add the check value
  remainder |= check; 

  // Operate on only 16 positions of max 24. 
  // The remaining 8 are our remainder and should be zero when we're done.
  for (uint8_t i = 0 ; i < 16 ; i++) {
    //Check if there is a one in the left position
    if( remainder & (uint32_t)1<<(23 - i) ) 
      remainder ^= divisor;

    //Rotate the divisor max 16 times so that we have 8 bits left of a remainder
    divisor >>= 1; 
  }
  return ((uint8_t) remainder);
}

/* ======================================================================
Function: si7021_StartConv
Purpose : return temperature or humidity measured 
Input   : data type SI7021_READ_HUM or SI7021_READ_TEMP
Output  : BCM2835_I2C_REASON_OK if okay
Comments: internal values of temp and rh are set
====================================================================== */
uint8_t si7021_StartConv(si7021_e datatype, int16_t * value)
{
  double data;
  uint16_t raw ;
  uint8_t checksum;
  uint8_t error;
  uint8_t buf[3];

	buf[0] = datatype == SI7021_READ_HUM ? SI7021_MEASURE_HUM : SI7021_MEASURE_TEMP;
  error = bcm2835_i2c_write((const char *) buf, 1);
	if (error != BCM2835_I2C_REASON_OK )
    return error;

  // Wait for data to become available
  // always use time out in loop to avoid
  // potential lockup (here 90ms (6*15ms))
  usleep(100000);

  error = bcm2835_i2c_read( (char *) buf, sizeof(buf));
	if (error != BCM2835_I2C_REASON_OK ) {
    return error;
	}
	
  // read raw value
  raw  = ( buf[0] << 8) | buf[1] ;
  checksum = buf[2];

  // Check CRC of data received
  if(si7021_checkCRC(raw, checksum) != 0) {
		printf("CRC Error %02X\n", checksum);
		return -1; 
  }

  if (datatype == SI7021_READ_HUM) {
    // Convert value to Humidity percent (*100)
    // for 43.21%rh value will be 4321
    data = -600 + (( raw * 12500 ) / 65536 ) ;

    // Datasheet says doing this check
    if (data>10000) data = 10000;
    if (data<0)   data = 0;

    // save value
    *value = (int16_t) data;

  } else {
    // Convert value to Temperature (*100)
    // for 23.45C value will be 2345
    data =  (( raw * 17572) / 65536) - 4685;

    // save value
    *value = (int16_t) data;
	}

	return error;
}


/* ======================================================================
Function: si7021_readRegister
Purpose : read the user register from the sensor
Input   : user register value filled by function
Output  : BCM2835_I2C_REASON_OK if okay
Comments: -
====================================================================== */
uint8_t si7021_readRegister(uint8_t * value)
{
  uint8_t error ;
	uint8_t buf[1];
		
	buf[0] = SI7021_READ_REG;
  error = bcm2835_i2c_write((const char *) buf, 1);
	if (error != BCM2835_I2C_REASON_OK )
    return error;

  error = bcm2835_i2c_read( (char*) value, 1);
	printf("Error=%d Read Buf[0] = %02X\n", error, *value);

  return error;  
}

/* ======================================================================
Function: si7021_readValues
Purpose : read temperature and humidity from SI7021 sensor
Input   : -
Output  : 0 if okay
Comments: -
====================================================================== */
uint8_t si7021_readValues(int16_t * temp, int16_t * hum)
{
  uint8_t error = 0;

  // start humidity conversion
  error |= si7021_StartConv(SI7021_READ_HUM, temp);

  // start temperature conversion
  error |= si7021_StartConv(SI7021_READ_TEMP, hum);

  return error;
}

/* ======================================================================
Function: si7021_setResolution
Purpose : Sets the sensor resolution to one of four levels 
Input   : see #define is .h file, default is SI7021_RESOLUTION_14T_12RH
Output  : temperature or humidity
Comments: BCM2835_I2C_REASON_OK if okay
====================================================================== */
uint8_t si7021_setResolution(uint8_t res)
{
	uint8_t buf[2];
  uint8_t reg;
  uint8_t error;

  // Get the current register value
  error = si7021_readRegister(&reg);
  if ( error == BCM2835_I2C_REASON_OK ) {
    // remove resolution bits
    reg &= SI7021_RESOLUTION_MASK ; 
	
    // Write the new resolution bits but clear unused before
		buf[0] = SI7021_WRITE_REG;
		buf[1] = reg | ( res &= ~SI7021_RESOLUTION_MASK);
		error = bcm2835_i2c_write( (char *) buf, 2);
  } 
  return error;
}


/* ======================================================================
Function: si7021_getID
Purpose : get the device ID
Input   : -
Output  : device ID
Comments: Return 0 if an error occured reading device ID
====================================================================== */
uint8_t si7021_getID( void )
{
  uint8_t id=0;
  uint8_t error;
	uint8_t buf[2];
	
	buf[0] = SI7021_READ_2ND_ID_1;
	buf[1] = SI7021_READ_2ND_ID_2;
	error = bcm2835_i2c_write( (char *) buf, 2);
  if ( error == BCM2835_I2C_REASON_OK ) {
		error = bcm2835_i2c_read( (char*) buf, 1);
		if ( error == BCM2835_I2C_REASON_OK ) {
			id = buf[0];
		}
	}
	return id;
 } 

	

