# Makefile
# Sample for gettting device EUI from MAC address
#
# 

CC            = g++
CFLAGS        = -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"$*\"
LIBS          = -lbcm2835 
RADIOHEADBASE = ../../..
INCLUDE       = -I$(RADIOHEADBASE)

all: get_deveui

get_deveui.o: get_deveui.c
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

get_deveui: get_deveui.o
				$(CC) $^ $(LIBS) -o get_deveui

clean:
				rm -rf *.o get_deveui
