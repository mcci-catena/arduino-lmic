# Sample for ttn-abp example on Raspberry Pi
# Caution: requires bcm2835 library to be already installed
# http://www.airspayce.com/mikem/bcm2835/

include ../Makefile.env
LMICBASE = ../../../src

all: ttn-abp

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $(INCLUDE) $<

%: %.o ../arduino-lmic.a
	$(CXX) $^ $(LIBS) -o $@

clean:
	rm -rf *.o ttn-abp
