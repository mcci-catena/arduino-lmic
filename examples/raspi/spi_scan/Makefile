# Makefile
# Sample for spi scan Hope RF module on Raspberry Pi
# Caution: requires bcm2835 library to be already installed
# http://www.airspayce.com/mikem/bcm2835/

CC            = g++
CFLAGS        = -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"$*\"
LIBS          = -lbcm2835 
RADIOHEADBASE = ../../..
INCLUDE       = -I$(RADIOHEADBASE)

all: spi_scan

spi_scan.o: spi_scan.c
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

spi_scan: spi_scan.o
				$(CC) $^ $(LIBS) -o spi_scan

clean:
				rm -rf *.o spi_scan
