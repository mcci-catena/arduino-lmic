/*
    LoRa secrets file

    Copyright (c) 2021,2022 P. Oldberg <pontus@ilabs.se>

   Permission is hereby granted, free of charge, to anyone
   obtaining a copy of this document and accompanying files,
   to do whatever they want with them without any restriction,
   including, but not limited to, copying, modification and redistribution.
   NO WARRANTY OF ANY KIND IS PROVIDED.
*/

/* Here you fill in the OTAA credentials of your TTN device */
#define APPEUI_DATA     // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define DEVEUI_DATA     // 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xD5, 0xB3, 0x70
#define APPKEY_DATA     // 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX

#define REGRESSION_TEST_DATA    0

//
// For normal use, we require that you edit the sketch to replace FILLMEIN
// with values assigned by the TTN console. However, for regression tests,
// we want to be able to compile these scripts. The regression tests define
// COMPILE_REGRESSION_TEST, and in that case we define FILLMEIN to a non-
// working but innocuous value.
//
#if defined(COMPILE_REGRESSION_TEST)
#  if defined(APPEUI_DATA)
#    undef APPEUI_DATA
#    define APPEUI_DATA   REGRESSION_TEST_DATA
#  endif
#  if defined(DEVEUI_DATA)
#    undef DEVEUI_DATA
#    define DEVEUI_DATA   REGRESSION_TEST_DATA
#  endif
#  if defined(APPKEY_DATA)
#    undef APPKEY_DATA
#    define APPKEY_DATA   REGRESSION_TEST_DATA
#  endif
#endif

// EUI's must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttn console, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
// 0x70.
static const u1_t PROGMEM APPEUI[8] = { APPEUI_DATA };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8] = { DEVEUI_DATA };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
static const u1_t PROGMEM APPKEY[16] = { APPKEY_DATA };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}
