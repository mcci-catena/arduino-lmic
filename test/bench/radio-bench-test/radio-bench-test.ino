/*

Module:	radio-bench-test.ino

Function:
	Bench test for the LMIC radio drivers' FSK receive path.

Copyright and License:
	Please see accompanying LICENSE file.

Author:
	Terry Moore, MCCI Corporation	October 2026

Description:
	Written for #1122 and #1123. It lives under test/bench, not examples,
	because it builds only for EU868, and CI compiles every example for
	every region.

	Settings: EU868, 869.525 MHz, DR7 (FSK 50 kbps). Commands on the
	console, one per line:

	fsk c		continuous receive (RADIO_RXON); prints each frame.
	fsk s		single-window receives (RADIO_RX) every 2 s.
	fsk x		stop (RADIO_RST).
	fsk d		dump LMIC radio state and the radio's status registers.
	fsk t		transmit one FSK frame.
	fsk r <n>	set rxsyms for single-window receives (bytes, for FSK).

	When a receive has been active more than 1.5 s with no completion,
	the sketch prints "STALL" and the radio registers, once per receive.
	(A continuous receive with nothing on the air reports this too.)

	Build with LMIC_ENABLE_event_logging=1 to see the driver's events;
	with the collection build script, for example:

	ARDUINO_CLI_FLAGS='--build-property compiler.c.extra_flags=-DLMIC_ENABLE_event_logging=1
	  --build-property compiler.cpp.extra_flags=-DLMIC_ENABLE_event_logging=1'

	rwc_nst.py sets up an RWC5020B to send NST FSK frames to it; see the
	comments there.

*/

#include <Arduino.h>
#include <arduino_lmic.h>
#include <arduino_lmic_hal_boards.h>
#include <SPI.h>

#define APPLICATION_VERSION ARDUINO_LMIC_VERSION_CALC(0, 1, 0, 0)

#if ! defined(CFG_eu868)
# error "build for EU868"
#endif

/****************************************************************************\
|	Event log (LMIC_ENABLE_event_logging)
\****************************************************************************/

struct logent_t {
	uint32_t	time;
	const char	*pMsg;
	uint32_t	datum;
	bool		fDatum;
};

static logent_t gLog[32];
static volatile unsigned gLogHead, gLogTail;

static void logPut(const char *pMsg, uint32_t datum, bool fDatum) {
	unsigned const next = (gLogTail + 1) % (sizeof(gLog) / sizeof(gLog[0]));
	if (next == gLogHead)
		return;
	gLog[gLogTail] = { (uint32_t) os_getTime(), pMsg, datum, fDatum };
	gLogTail = next;
}

#if LMIC_ENABLE_event_logging
extern "C" {
	void LMICOS_logEvent(const char *pMessage);
	void LMICOS_logEventUint32(const char *pMessage, uint32_t datum);
}
void LMICOS_logEvent(const char *pMessage) { logPut(pMessage, 0, false); }
void LMICOS_logEventUint32(const char *pMessage, uint32_t datum) { logPut(pMessage, datum, true); }
#endif

static void logPrintAll(void) {
	while (gLogHead != gLogTail) {
		logent_t const e = gLog[gLogHead];
		gLogHead = (gLogHead + 1) % (sizeof(gLog) / sizeof(gLog[0]));
		Serial.print("  ev "); Serial.print(e.time);
		Serial.print(": "); Serial.print(e.pMsg);
		if (e.fDatum) {
			Serial.print(" 0x"); Serial.print(e.datum, HEX);
		}
		Serial.println();
	}
}

/****************************************************************************\
|	Radio register dump
\****************************************************************************/

static void dumpRadio(const char *pWhy) {
	Serial.print(pWhy);
	Serial.print(": t="); Serial.print(os_getTime());
	Serial.print(" radio.state=0x"); Serial.print(LMIC.radio.state, HEX);
	Serial.print(" saveIrqFlags=0x"); Serial.print(LMIC.saveIrqFlags, HEX);

#if defined(CFG_sx1261_radio) || defined(CFG_sx1262_radio)
	u1_t status = 0;
	u1_t nop = 0;
	u1_t irq[2] = { 0, 0 };
	u1_t err[3] = { 0, 0, 0 };

	lmic_hal_spi_read_sx126x(0xC0 /* GetStatus */, NULL, 0, &status, 1);
	lmic_hal_spi_read_sx126x(0x12 /* GetIrqStatus */, &nop, 1, irq, 2);
	lmic_hal_spi_read_sx126x(0x17 /* GetDeviceErrors */, NULL, 0, err, 3);

	Serial.print(" chipmode="); Serial.print((status >> 4) & 7);
	Serial.print(" cmdstatus="); Serial.print((status >> 1) & 7);
	Serial.print(" irq=0x"); Serial.print((irq[0] << 8) | irq[1], HEX);
	Serial.print(" deverr=0x"); Serial.print((err[1] << 8) | err[2], HEX);
#else
	// SX127x: RegOpMode, and the IRQ flags of both modems (the register
	// map in use depends on RegOpMode bit 7).
	u1_t opmode = 0, flags1 = 0, flags2 = 0;

	lmic_hal_spi_read(0x01 /* RegOpMode */, &opmode, 1);
	lmic_hal_spi_read(0x3E /* FSK RegIrqFlags1 */, &flags1, 1);
	lmic_hal_spi_read(0x3F /* FSK RegIrqFlags2 */, &flags2, 1);

	Serial.print(" opmode=0x"); Serial.print(opmode, HEX);
	if (opmode & 0x80) {
		u1_t loraflags = 0;
		lmic_hal_spi_read(0x12 /* LoRa RegIrqFlags */, &loraflags, 1);
		Serial.print(" lora irq=0x"); Serial.print(loraflags, HEX);
	} else {
		Serial.print(" fsk irq1=0x"); Serial.print(flags1, HEX);
		Serial.print(" irq2=0x"); Serial.print(flags2, HEX);
	}
#endif
	Serial.println();
}

/****************************************************************************\
|	Test logic
\****************************************************************************/

enum TestMode { kIdle, kContinuous, kSingle };

static TestMode gMode = kIdle;
static osjob_t radiojob;
static osjob_t singlejob;
static osjob_t watchjob;
static ostime_t gRxStart;
static bool gRxActive;
static bool gStallReported;
static uint32_t gnRx, gnTimeout, gnStall;
static rxsyms_t gRxsyms;

static void startRx(void);
static void single_func(osjob_t *job) { (void) job; startRx(); }

static void rxdone_func(osjob_t *job) {
	(void) job;
	const u1_t *pData;
	u1_t nData;

	gRxActive = false;
	// os_radio_v2() leaves the length in LMIC.radio, not LMIC.dataLen
	pData = LMIC.radio.pFrame;
	nData = LMIC.radio.dataLen;

	if (nData == 0) {
		++gnTimeout;
		Serial.print("RX timeout after ");
		Serial.print(osticks2ms(os_getTime() - gRxStart));
		Serial.print(" ms, saveIrqFlags=0x");
		Serial.println(LMIC.saveIrqFlags, HEX);
	} else {
		++gnRx;
		Serial.print("RX "); Serial.print(nData);
		Serial.print(" bytes, saveIrqFlags=0x"); Serial.print(LMIC.saveIrqFlags, HEX);
		Serial.print(" rssi="); Serial.print(int(LMIC.rssi) - RSSI_OFF);
		Serial.print(":");
		for (unsigned i = 0; i < nData; ++i) {
			Serial.print(' ');
			if (pData[i] < 16) Serial.print('0');
			Serial.print(pData[i], HEX);
		}
		Serial.println();
	}

	if (gMode == kContinuous)
		startRx();
	else if (gMode == kSingle)
		os_setTimedCallback(&singlejob, os_getTime() + ms2osticks(2000), single_func);
}

static void startRx(void) {
	radiojob.func = rxdone_func;
	gRxStart = os_getTime();
	gRxActive = true;
	gStallReported = false;
	if (gMode == kContinuous) {
		LMIC_setNextRxTime(gRxStart);
		os_radio_v2(RADIO_RXON, &radiojob);
	} else {
		LMIC.radio.rxtime = gRxStart + ms2osticks(5);
		LMIC.radio.rxsyms = gRxsyms;
		os_radio_v2(RADIO_RX, &radiojob);
	}
}

static void txdone_func(osjob_t *job) {
	(void) job;
	gRxActive = false;
	Serial.println("TX done");
	if (gMode != kIdle)
		startRx();
}

static void doTx(void) {
	static const u1_t frame[] = "MCCI FSK test frame";
	os_radio(RADIO_RST);
	gRxActive = false;
	LMIC_setRawTxData(frame, sizeof(frame) - 1);
	radiojob.func = txdone_func;
	os_radio_v2(RADIO_TX, &radiojob);
	Serial.println("TX");
}

static void watch_func(osjob_t *job) {
	if (gRxActive && ! gStallReported &&
	    os_getTime() - gRxStart > ms2osticks(1500)) {
		gStallReported = true;
		++gnStall;
		dumpRadio("STALL");
	}
	os_setTimedCallback(job, os_getTime() + ms2osticks(250), watch_func);
}

static void command(int c, long arg) {
	switch (c) {
	case 'c':
		os_radio(RADIO_RST);
		os_clearCallback(&singlejob);
		gMode = kContinuous;
		Serial.println("mode: continuous");
		startRx();
		break;
	case 's':
		os_radio(RADIO_RST);
		gMode = kSingle;
		Serial.println("mode: single window");
		startRx();
		break;
	case 'x':
		os_clearCallback(&singlejob);
		gMode = kIdle;
		os_radio(RADIO_RST);
		gRxActive = false;
		Serial.println("mode: idle");
		break;
	case 'd':
		dumpRadio("DUMP");
		Serial.print("rx="); Serial.print(gnRx);
		Serial.print(" timeout="); Serial.print(gnTimeout);
		Serial.print(" stall="); Serial.println(gnStall);
		break;
	case 't':
		doTx();
		break;
	case 'r':
		gRxsyms = (rxsyms_t) arg;
		Serial.print("rxsyms="); Serial.println(gRxsyms);
		break;
	default:
		break;
	}
}

void setup() {
	delay(3000);
	while (! Serial)
		/* wait for the host */;
	Serial.begin(115200);
	Serial.println("radio-bench-test: EU868 869.525 MHz DR7 FSK");

	const Arduino_LMIC::HalPinmap_t *pPinMap = Arduino_LMIC::GetPinmap_ThisBoard();
	if (pPinMap == nullptr) {
		Serial.println("board not known to library");
		for (;;) ;
	}
	os_init_ex(pPinMap);
	LMIC_setClockError(10 * 65536 / 100);

	LMIC_setFrequency(869525000);
	LMIC_setDataRate(EU868_DR_FSK);
	LMIC_setRps(updr2rps(LMIC_getDataRate()));
	LMIC_setRadioTxPower(14);

	Serial.print("event logging: ");
	Serial.println(LMIC_ENABLE_event_logging);
	Serial.println("commands: fsk c|s|x|d|t|r <rxsyms>");
	os_setCallback(&watchjob, watch_func);
}

void loop() {
	os_runloop_once();
	logPrintAll();
	static char line[32];
	static unsigned nLine;
	while (Serial.available() > 0) {
		int const c = Serial.read();
		if (c == '\r' || c == '\n') {
			line[nLine] = '\0';
			// commands are "fsk <letter> [number]"; anything else is ignored
			if (nLine >= 5 && strncmp(line, "fsk ", 4) == 0)
				command(line[4], atol(line + 5));
			nLine = 0;
		} else if (nLine < sizeof(line) - 1) {
			line[nLine++] = (char) c;
		}
	}
}
