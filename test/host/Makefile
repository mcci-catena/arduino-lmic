##############################################################################
#
# Module:  test/host/Makefile
#
# Function:
#	Build and run the LMIC core's host unit tests.
#
# Copyright and License:
#	See LICENSE file accompanying this project.
#
# Author:
#	Terry Moore, MCCI Corporation	September 2026
#
# Usage:
#	make            build and run the tests for REGION (default eu868)
#	make REGION=us915
#	make RATE=32768    build and run at the STM32L0 LPTIM tick rate
#	make all-regions   run the tests for every region, at both tick rates
#	make clean
#
#	Needs gcc and g++ on the host. The whole core is compiled with the
#	region, radio and tick rate given on the command line, linked against
#	hal_stub.c in place of the Arduino HAL and lmic_ostime_host.h in place
#	of an Arduino ostime driver, and run natively. Nothing here touches
#	the Arduino toolchain.
#
##############################################################################

REGION ?= eu868
RADIO ?= sx1276
RATE ?= 62500
REGIONS := eu868 us915 au915 as923 kr920 in866
RATES := 62500 32768

TOP := ../..
BUILD := build/$(REGION)-$(RATE)

DEFS := -DARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -DCFG_$(REGION) -DCFG_$(RADIO)_radio -DCOMPILE_REGRESSION_TEST
DEFS += -DLMIC_PLATFORM_OSTIME_INCLUDE=lmic_ostime_host.h -DLMIC_OSTICKS_PER_SEC=$(RATE)
INCS := -I. -I$(TOP)/src/lmic -I$(TOP)/src/hal -I$(TOP)/src/ostime/i -I$(TOP)/src -I$(TOP)/src/aes -I$(TOP)/src/se -I$(TOP)/src/se/i

# the core is compiled quietly; the tests and stubs with warnings on.
CORE_CFLAGS := -std=gnu11 -g -O0 -w $(DEFS) $(INCS)
CORE_CXXFLAGS := -std=gnu++11 -g -O0 -w $(DEFS) $(INCS)
TEST_CFLAGS := -std=gnu11 -g -O0 -Wall -Wextra $(DEFS) $(INCS)

CORE_C := $(wildcard $(TOP)/src/lmic/*.c) $(wildcard $(TOP)/src/aes/*.c) $(shell find $(TOP)/src/se -name '*.c')
CORE_CXX := $(TOP)/src/aes/ideetron/AES-128_V10.cpp

CORE_OBJS := $(patsubst $(TOP)/%.c,$(BUILD)/core/%.o,$(CORE_C)) \
             $(patsubst $(TOP)/%.cpp,$(BUILD)/core/%.o,$(CORE_CXX))

TESTS := test_session_state
TEST_BINS := $(patsubst %,$(BUILD)/%,$(TESTS))

.PHONY: all test all-regions clean

# keep the objects between runs, so a second make rebuilds only what changed.
.SECONDARY:

all: test

test: $(TEST_BINS)
	@for t in $(TEST_BINS); do echo "== $$t"; $$t || exit 1; done

all-regions:
	@for rate in $(RATES); do for r in $(REGIONS); do $(MAKE) --no-print-directory REGION=$$r RATE=$$rate test || exit 1; done; done

$(BUILD)/core/%.o: $(TOP)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CORE_CFLAGS) -c $< -o $@

$(BUILD)/core/%.o: $(TOP)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CORE_CXXFLAGS) -c $< -o $@

$(BUILD)/hal_stub.o: hal_stub.c hal_stub.h lmic_ostime_host.h
	@mkdir -p $(dir $@)
	$(CC) $(TEST_CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.c hal_stub.h lmic_ostime_host.h
	@mkdir -p $(dir $@)
	$(CC) $(TEST_CFLAGS) -c $< -o $@

$(BUILD)/%: $(BUILD)/%.o $(BUILD)/hal_stub.o $(CORE_OBJS)
	$(CXX) $^ -o $@

clean:
	rm -rf build

### end of Makefile ###
