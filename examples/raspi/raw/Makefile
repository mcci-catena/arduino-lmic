# Makefile
# Sample for raw example on Raspberry Pi
# Caution: requires bcm2835 library to be already installed
# http://www.airspayce.com/mikem/bcm2835/

CC       = g++
CFLAGS   = -std=c++11 -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"$*\"
LIBS     = -lbcm2835
LMICBASE = ../../../src
INCLUDE  = -I$(LMICBASE) 

all: raw 

raspi.o: $(LMICBASE)/raspi/raspi.cpp
				$(CC) $(CFLAGS) -c $(LMICBASE)/raspi/raspi.cpp $(INCLUDE)

radio.o: $(LMICBASE)/lmic/radio.c
				$(CC) $(CFLAGS) -c $(LMICBASE)/lmic/radio.c $(INCLUDE)

oslmic.o: $(LMICBASE)/lmic/oslmic.c
				$(CC) $(CFLAGS) -c $(LMICBASE)/lmic/oslmic.c $(INCLUDE)

lmic.o: $(LMICBASE)/lmic/lmic.c
				$(CC) $(CFLAGS) -c $(LMICBASE)/lmic/lmic.c $(INCLUDE)

hal.o: $(LMICBASE)/hal/hal.cpp
				$(CC) $(CFLAGS) -c $(LMICBASE)/hal/hal.cpp $(INCLUDE)

aes.o: $(LMICBASE)/aes/lmic.c
				$(CC) $(CFLAGS) -c $(LMICBASE)/aes/lmic.c $(INCLUDE) -o aes.o

raw.o: raw.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

raw: raw.o raspi.o radio.o oslmic.o lmic.o hal.o aes.o
				$(CC) $^ $(LIBS) -o raw

clean:
				rm -rf *.o raw
