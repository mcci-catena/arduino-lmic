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
#	make all-regions   run the tests for every region
#	make clean
#
#	Needs gcc and g++ on the host. The whole core is compiled with the
#	region and radio given on the command line, linked against hal_stub.c
#	in place of the Arduino HAL, and run natively. Nothing here touches
#	the Arduino toolchain.
#
##############################################################################

REGION ?= eu868
RADIO ?= sx1276
REGIONS := eu868 us915 au915 as923 kr920 in866

TOP := ../..
BUILD := build/$(REGION)

DEFS := -DARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -DCFG_$(REGION) -DCFG_$(RADIO)_radio -DCOMPILE_REGRESSION_TEST
INCS := -I. -I$(TOP)/src/lmic -I$(TOP)/src/hal -I$(TOP)/src -I$(TOP)/src/aes -I$(TOP)/src/se -I$(TOP)/src/se/i

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

all: test

test: $(TEST_BINS)
	@for t in $(TEST_BINS); do echo "== $$t"; $$t || exit 1; done

all-regions:
	@for r in $(REGIONS); do $(MAKE) --no-print-directory REGION=$$r test || exit 1; done

$(BUILD)/core/%.o: $(TOP)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CORE_CFLAGS) -c $< -o $@

$(BUILD)/core/%.o: $(TOP)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CORE_CXXFLAGS) -c $< -o $@

$(BUILD)/hal_stub.o: hal_stub.c hal_stub.h
	@mkdir -p $(dir $@)
	$(CC) $(TEST_CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.c hal_stub.h
	@mkdir -p $(dir $@)
	$(CC) $(TEST_CFLAGS) -c $< -o $@

$(BUILD)/%: $(BUILD)/%.o $(BUILD)/hal_stub.o $(CORE_OBJS)
	$(CXX) $^ -o $@

clean:
	rm -rf build

### end of Makefile ###
