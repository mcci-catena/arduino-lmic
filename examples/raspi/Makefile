# Makefile
# Caution: requires bcm2835 library to be already installed
# http://www.airspayce.com/mikem/bcm2835/
BCM2835 = 1.59

include Makefile.env

all: /usr/local/lib/libbcm2835.a arduino-lmic.a

arduino-lmic.a: raspi.o radio.o oslmic.o lmic.o hal.o aes.o \
		lmic_as923.o lmic_au921.o lmic_eu868.o lmic_in866.o lmic_us915.o \
		lmic_us_like.o lmic_eu_like.o
	$(AR) rcs $@ $^

raspi.o: $(LMICBASE)/raspi/raspi.cpp
	$(CXX) $(CFLAGS) -c $^ $(INCLUDE)

radio.o: $(LMICBASE)/lmic/radio.c
	$(CC) $(CFLAGS) -c $^ $(INCLUDE)

oslmic.o: $(LMICBASE)/lmic/oslmic.c
	$(CC) $(CFLAGS) -c $^ $(INCLUDE)

lmic.o: $(LMICBASE)/lmic/lmic.c
	$(CC) $(CFLAGS) -c $^ $(INCLUDE)

hal.o: $(LMICBASE)/hal/hal.cpp
	$(CXX) $(CFLAGS) -c $^ $(INCLUDE)

aes.o: $(LMICBASE)/aes/lmic.c
	$(CC) $(CFLAGS) -c $^ $(INCLUDE) -o aes.o

lmic_as923.o: $(LMICBASE)/lmic/lmic_as923.c
	$(CC) $(CFLAGS) -c $^ $(INCLUDE)

lmic_au921.o: $(LMICBASE)/lmic/lmic_au921.c
	$(CC) $(CFLAGS) -c $^ $(INCLUDE)

lmic_eu868.o: $(LMICBASE)/lmic/lmic_eu868.c
	$(CC) $(CFLAGS) -c $^ $(INCLUDE)

lmic_in866.o: $(LMICBASE)/lmic/lmic_in866.c
	$(CC) $(CFLAGS) -c $^ $(INCLUDE)

lmic_us915.o: $(LMICBASE)/lmic/lmic_us915.c
	$(CC) $(CFLAGS) -c $^ $(INCLUDE)

lmic_us_like.o: $(LMICBASE)/lmic/lmic_us_like.c
	$(CC) $(CFLAGS) -c $^ $(INCLUDE)

lmic_eu_like.o: $(LMICBASE)/lmic/lmic_eu_like.c
	$(CC) $(CFLAGS) -c $^ $(INCLUDE)

/usr/local/lib/libbcm2835.a:
	[ -f bcm2835-$(BCM2835).tar.gz ] || wget http://www.airspayce.com/mikem/bcm2835/bcm2835-$(BCM2835).tar.gz
	if [ ! -f $@ ]; then \
		set -e; \
		tar xvfz bcm2835-$(BCM2835).tar.gz; \
		cd bcm2835-$(BCM2835); \
		./configure && make && sudo make install; \
	fi
