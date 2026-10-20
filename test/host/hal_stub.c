/*

Module:  hal_stub.c

Function:
	Host stand-in for the Arduino HAL, so the LMIC core links and runs
	natively in unit tests.

Copyright and License:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	September 2026

Description:
	Every HAL entry point the core calls is here, doing nothing or the
	least that lets the core proceed. The clock is a counter the test
	controls. SPI reads return zero except for the SX127x version
	register, which returns the value radio_init() checks for, so the
	radio driver believes a radio is present.

*/

#include "hal_stub.h"
#include "hal.h"
#include "lmic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/****************************************************************************\
|
|	The pin map the deprecated os_init() refers to. Never dereferenced here.
|
\****************************************************************************/

struct lmic_pinmap { int unused; };
const struct lmic_pinmap lmic_pins = { 0 };

/****************************************************************************\
|
|	Clock
|
\****************************************************************************/

static u4_t s_ticks;

void hal_stub_setTicks(u4_t ticks) {
	s_ticks = ticks;
}

void hal_stub_advanceTicks(u4_t delta) {
	s_ticks += delta;
}

// the Host ostime driver named by test/host/lmic_ostime_host.h; the core
// reads time through LMIC_OsTime_ticks(), which binds to these.
void LMIC_ABI_STD LMIC_OsTime_Host_initialize(void) {
}

u4_t LMIC_ABI_STD LMIC_OsTime_Host_ticks(void) {
	return s_ticks;
}

u4_t lmic_hal_waitUntil(u4_t time) {
	s_ticks = time;
	return 0;
}

u1_t lmic_hal_checkTimer(u4_t targettime) {
	return (s4_t)(targettime - s_ticks) <= 0;
}

/****************************************************************************\
|
|	Radio and pins
|
\****************************************************************************/

// The SX127x driver addresses registers as addr | 0x80 for a write and
// addr & 0x7F for a read, and reads back some of what it writes. Keep a
// register file so those read-backs hold; preset the version register to
// what radio_init() checks for.
//
// Two registers need to act like hardware or the driver spins forever:
// radio_init() waits for the wideband RSSI's low bit to change between two
// reads, and the image calibration waits for its RUNNING bit to clear.
enum {
	SX127X_RegOpMode = 0x01,
	SX127X_LORARegIrqFlags = 0x12,
	SX127X_OPMODE_LORA = 0x80,
	SX127X_OPMODE_MASK = 0x07,
	SX127X_OPMODE_STANDBY = 0x01,
	SX127X_OPMODE_TX = 0x03,
	SX127X_OPMODE_RX_SINGLE = 0x06,
	SX127X_IRQ_LORA_TXDONE = 0x08,
	SX127X_IRQ_LORA_RXTOUT = 0x80,
	SX127X_RegVersion = 0x42,
	SX1276_VERSION = 0x12,
	SX127X_LORARegRssiWideband = 0x2C,
	SX127X_FSKRegImageCal = 0x3B,
	SX127X_IMAGECAL_RUNNING = 0x20,
};

static u1_t s_regs[128];

// the simulated radio: with auto-complete on, a LoRa transmit finishes
// after its airtime, and a single receive times out (nothing heard)
// after RX_TIMEOUT_MS, each through radio_irq_handler_v2() from
// lmic_hal_processPendingIRQs(), the way the Arduino HAL delivers DIO
// events.
enum { RX_TIMEOUT_MS = 10 };
enum radio_pending_e { STUB_RADIO_IDLE, STUB_RADIO_TX, STUB_RADIO_RX };

static bit_t s_radioAuto;
static u1_t s_radioPending = STUB_RADIO_IDLE;
static u4_t s_radioDoneAt;

void hal_stub_setRadioAutoComplete(bit_t fEnable) {
	s_radioAuto = fEnable;
	s_radioPending = STUB_RADIO_IDLE;
}

u4_t hal_stub_radioTicksUntilEvent(void) {
	if (s_radioPending == STUB_RADIO_IDLE)
		return HAL_STUB_RADIO_IDLE;
	s4_t const d = (s4_t)(s_radioDoneAt - s_ticks);
	return d > 0 ? (u4_t)d : 0;
}

// a write to RegOpMode: start the simulated transmit or receive.
static void radioOpMode(u1_t v) {
	if (! s_radioAuto || (v & SX127X_OPMODE_LORA) == 0) {
		s_radioPending = STUB_RADIO_IDLE;
		return;
	}
	switch (v & SX127X_OPMODE_MASK) {
	case SX127X_OPMODE_TX:
		s_radioPending = STUB_RADIO_TX;
		s_radioDoneAt = s_ticks + (u4_t)calcAirTime(LMIC.rps, LMIC.dataLen);
		break;
	case SX127X_OPMODE_RX_SINGLE:
		s_radioPending = STUB_RADIO_RX;
		s_radioDoneAt = s_ticks + (u4_t)ms2osticks(RX_TIMEOUT_MS);
		break;
	default:
		s_radioPending = STUB_RADIO_IDLE;
		break;
	}
}

static void resetRegs(void) {
	memset(s_regs, 0, sizeof s_regs);
	s_regs[SX127X_RegVersion] = SX1276_VERSION;
}

void lmic_hal_init_ex(const void *pContext) {
	LMIC_UNREFERENCED_PARAMETER(pContext);
	resetRegs();
}

void lmic_hal_pin_rxtx(u1_t val) {
	LMIC_UNREFERENCED_PARAMETER(val);
}

void lmic_hal_pin_rst(u1_t val) {
	LMIC_UNREFERENCED_PARAMETER(val);
}

void lmic_hal_spi_write(u1_t cmd, const u1_t *buf, size_t len) {
	u1_t addr = cmd & 0x7F;

	// a burst write to the FIFO (register 0) stays at register 0.
	for (size_t i = 0; i < len; ++i) {
		u1_t v = buf[i];

		if (addr == SX127X_FSKRegImageCal)
			v &= (u1_t)~SX127X_IMAGECAL_RUNNING;	// calibration finishes at once
		s_regs[addr] = v;
		if (addr == SX127X_RegOpMode)
			radioOpMode(v);
		if (addr != 0)
			addr = (addr + 1) & 0x7F;
	}
}

void lmic_hal_spi_read(u1_t cmd, u1_t *buf, size_t len) {
	u1_t addr = cmd & 0x7F;

	for (size_t i = 0; i < len; ++i) {
		if (addr == SX127X_LORARegRssiWideband)
			++s_regs[addr];				// noise: a new value every read
		buf[i] = s_regs[addr];
		if (addr != 0)
			addr = (addr + 1) & 0x7F;
	}
}

void lmic_hal_disableIRQs(void) {
}

void lmic_hal_enableIRQs(void) {
}

void lmic_hal_sleep(void) {
}

void lmic_hal_processPendingIRQs(void) {
	if (s_radioPending == STUB_RADIO_IDLE || (s4_t)(s_radioDoneAt - s_ticks) > 0)
		return;

	u4_t const t = s_radioDoneAt;
	s_regs[SX127X_LORARegIrqFlags] = (s_radioPending == STUB_RADIO_TX)
		? SX127X_IRQ_LORA_TXDONE : SX127X_IRQ_LORA_RXTOUT;
	s_regs[SX127X_RegOpMode] = (u1_t)((s_regs[SX127X_RegOpMode] & ~SX127X_OPMODE_MASK) | SX127X_OPMODE_STANDBY);
	s_radioPending = STUB_RADIO_IDLE;
	radio_irq_handler_v2(0, (ostime_t)t);
}

bit_t lmic_hal_hasPendingIrqs(void) {
	return s_radioPending != STUB_RADIO_IDLE && (s4_t)(s_radioDoneAt - s_ticks) <= 0;
}

ostime_t lmic_hal_setModuleActive(bit_t val) {
	LMIC_UNREFERENCED_PARAMETER(val);
	return 0;
}

bit_t lmic_hal_queryUsingTcxo(void) {
	return 0;
}

s1_t lmic_hal_getRssiCal(void) {
	return 0;
}

uint8_t lmic_hal_getTxPowerPolicy(u1_t inputPolicy, s1_t requestedPower, u4_t freq) {
	LMIC_UNREFERENCED_PARAMETER(requestedPower);
	LMIC_UNREFERENCED_PARAMETER(freq);
	return inputPolicy;
}

void lmic_hal_failed(const char *file, u2_t line) {
	fprintf(stderr, "LMIC assertion failed: %s:%u\n", file, line);
	abort();
}

/****************************************************************************\
|
|	Client callbacks the core requires
|
\****************************************************************************/

void os_getDevEui(u1_t *buf) {
	memset(buf, 0, 8);
}

void os_getArtEui(u1_t *buf) {
	memset(buf, 0, 8);
}

void os_getDevKey(u1_t *buf) {
	memset(buf, 0, 16);
}

/****************************************************************************\
|
|	Bring-up
|
\****************************************************************************/

void hal_stub_startLmic(void) {
	if (! os_init_ex(&lmic_pins)) {
		fprintf(stderr, "os_init_ex failed\n");
		abort();
	}
	LMIC_reset();
}

/****************************************************************************\
|
|	Running the scheduler
|
\****************************************************************************/

void hal_stub_runUntil(u4_t untilTick) {
	hal_stub_runUntilStop(untilTick, NULL);
}

void hal_stub_runUntilStop(u4_t untilTick, const volatile int *pStop) {
	unsigned nStill = 0;

	for (;;) {
		u4_t q = os_runloop_once_ex();
		if (pStop != NULL && *pStop)
			return;
		u4_t const r = hal_stub_radioTicksUntilEvent();

		if (q == 0) {
			// a job is runnable or due, a radio event is due, or the
			// radio is busy (the scheduler reports 0 then, because a
			// polled HAL finds the radio's end only by asking).
			if (r == HAL_STUB_RADIO_IDLE || r == 0) {
				// a job that finds itself due again at once (the
				// engine waits for txbeg - TX_RAMPUP, then needs
				// one more tick) would spin here; real time moves
				// on, so move the clock one tick.
				if (++nStill > 4) {
					++s_ticks;
					nStill = 0;
					if ((s4_t)(untilTick - s_ticks) <= 0)
						return;
				}
				continue;
			}
			// radio busy: run any runnable jobs first, then move on.
			for (int i = 0; i < 8; ++i) {
				(void) os_runloop_once_ex();
				if (pStop != NULL && *pStop)
					return;
			}
			q = hal_stub_radioTicksUntilEvent();
			if (q == HAL_STUB_RADIO_IDLE)
				continue;
		} else if (r != HAL_STUB_RADIO_IDLE && r < q) {
			q = r;
		}

		s4_t const left = (s4_t)(untilTick - s_ticks);
		if (left <= 0)
			return;
		if (q == OS_NEXT_JOB_NEVER || q > (u4_t)left)
			q = (u4_t)left;
		s_ticks += q;
		nStill = 0;
	}
}
