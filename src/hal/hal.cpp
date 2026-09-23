/*******************************************************************************
 * Copyright (c) 2015 Matthijs Kooijman
 * Copyright (c) 2018-2026 MCCI Corporation
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * This the HAL to run LMIC on top of the Arduino environment.
 *******************************************************************************/

#include <Arduino.h>
#include <SPI.h>
// include all the lmic header files, including ../lmic/hal.h
#include "../lmic.h"
// include the C++ hal.h
#include "hal.h"
// we may need some things from stdio.
#include <stdio.h>

#include "../ostime/i/lmic_ostime_api.h"

// -----------------------------------------------------------------------------
// I/O

static const Arduino_LMIC::HalPinmap_t *plmic_pins;
static Arduino_LMIC::HalConfiguration_t *pHalConfig;
static Arduino_LMIC::HalConfiguration_t nullHalConig;
static lmic_hal_failure_handler_t* custom_hal_failure_handler = NULL;

static void lmic_hal_interrupt_init(); // Fwd declaration

static void lmic_hal_io_init () {
    // NSS and DIO0 are required, DIO1 is required for LoRa, DIO2 for FSK
    ASSERT(plmic_pins->nss != LMIC_UNUSED_PIN);
    ASSERT(plmic_pins->dio[0] != LMIC_UNUSED_PIN);
    // SX126x family can operate with a single DIO
#if (defined(CFG_sx1276_radio) || defined(CFG_sx1272_radio))
    ASSERT(plmic_pins->dio[1] != LMIC_UNUSED_PIN || plmic_pins->dio[2] != LMIC_UNUSED_PIN);
#endif

//    Serial.print("nss: "); Serial.println(plmic_pins->nss);
//    Serial.print("rst: "); Serial.println(plmic_pins->rst);
//    Serial.print("dio[0]: "); Serial.println(plmic_pins->dio[0]);
//    Serial.print("dio[1]: "); Serial.println(plmic_pins->dio[1]);
//    Serial.print("dio[2]: "); Serial.println(plmic_pins->dio[2]);

    // initialize SPI chip select to high (it's active low)
    digitalWrite(plmic_pins->nss, HIGH);
    pinMode(plmic_pins->nss, OUTPUT);

    if (plmic_pins->rxtx != LMIC_UNUSED_PIN) {
        // initialize to RX
        digitalWrite(plmic_pins->rxtx, LOW != plmic_pins->rxtx_rx_active);
        pinMode(plmic_pins->rxtx, OUTPUT);
    }
    if (plmic_pins->rst != LMIC_UNUSED_PIN) {
        // initialize RST to floating
        pinMode(plmic_pins->rst, INPUT);
    }

    if (pHalConfig->queryBusyPin() != LMIC_UNUSED_PIN) {
        pinMode(pHalConfig->queryBusyPin(), INPUT);
    }

    lmic_hal_interrupt_init();
}

// val == 1  => tx
void lmic_hal_pin_rxtx (u1_t val) {
    if (plmic_pins->rxtx != LMIC_UNUSED_PIN)
        digitalWrite(plmic_pins->rxtx, val != plmic_pins->rxtx_rx_active);
}

// set radio RST pin to given value (or keep floating!)
void lmic_hal_pin_rst (u1_t val) {
    if (plmic_pins->rst == LMIC_UNUSED_PIN)
        return;

    if(val == 0 || val == 1) { // drive pin
        digitalWrite(plmic_pins->rst, val);
        pinMode(plmic_pins->rst, OUTPUT);
    } else { // keep pin floating
        pinMode(plmic_pins->rst, INPUT);
    }
}

s1_t lmic_hal_getRssiCal (void) {
    return plmic_pins->rssi_cal;
}

//--------------------
// Interrupt handling
//--------------------
static constexpr unsigned NUM_DIO_INTERRUPT = 3;
static_assert(NUM_DIO_INTERRUPT <= NUM_DIO, "Number of interrupt-sensitive lines must be less than number of GPIOs");
static ostime_t interrupt_time[NUM_DIO_INTERRUPT] = {0};

#if !defined(LMIC_USE_INTERRUPTS)
static void lmic_hal_interrupt_init() {
    pinMode(plmic_pins->dio[0], INPUT);
    if (plmic_pins->dio[1] != LMIC_UNUSED_PIN)
        pinMode(plmic_pins->dio[1], INPUT);
    if (plmic_pins->dio[2] != LMIC_UNUSED_PIN)
        pinMode(plmic_pins->dio[2], INPUT);
    static_assert(NUM_DIO_INTERRUPT == 3, "Number of interrupt lines must be set to 3");
}

static bool dio_states[NUM_DIO_INTERRUPT] = {0};
void lmic_hal_pollPendingIRQs_helper() {
    uint8_t i;
    for (i = 0; i < NUM_DIO_INTERRUPT; ++i) {
        if (plmic_pins->dio[i] == LMIC_UNUSED_PIN)
            continue;

        if (dio_states[i] != digitalRead(plmic_pins->dio[i])) {
            dio_states[i] = !dio_states[i];
            if (dio_states[i] && interrupt_time[i] == 0) {
                ostime_t const now = os_getTime();
                interrupt_time[i] = now ? now : 1;
            }
        }
    }
}

#else
// Interrupt handlers

static void lmic_hal_isrPin0() {
    if (interrupt_time[0] == 0) {
        ostime_t now = os_getTime();
        interrupt_time[0] = now ? now : 1;
    }
}
static void lmic_hal_isrPin1() {
    if (interrupt_time[1] == 0) {
        ostime_t now = os_getTime();
        interrupt_time[1] = now ? now : 1;
    }
}
static void lmic_hal_isrPin2() {
    if (interrupt_time[2] == 0) {
        ostime_t now = os_getTime();
        interrupt_time[2] = now ? now : 1;
    }
}

typedef void (*isr_t)();
static const isr_t interrupt_fns[NUM_DIO_INTERRUPT] = {lmic_hal_isrPin0, lmic_hal_isrPin1, lmic_hal_isrPin2};
static_assert(NUM_DIO_INTERRUPT == 3, "number of interrupts must be 3 for initializing interrupt_fns[]");

static void lmic_hal_interrupt_init() {
  for (uint8_t i = 0; i < NUM_DIO_INTERRUPT; ++i) {
      if (plmic_pins->dio[i] == LMIC_UNUSED_PIN)
          continue;

      pinMode(plmic_pins->dio[i], INPUT);
      attachInterrupt(digitalPinToInterrupt(plmic_pins->dio[i]), interrupt_fns[i], RISING);
  }
}
#endif // LMIC_USE_INTERRUPTS

void lmic_hal_processPendingIRQs() {
    uint8_t i;
    for (i = 0; i < NUM_DIO_INTERRUPT; ++i) {
        ostime_t iTime;
        if (plmic_pins->dio[i] == LMIC_UNUSED_PIN)
            continue;

        // NOTE(tmm@mcci.com): if using interrupts, this next step
        // assumes uniprocessor and fairly strict memory ordering
        // semantics relative to ISRs. It would be better to use
        // interlocked-exchange, but that's really far beyond
        // Arduino semantics. Because our ISRs use "first time
        // stamp" semantics, we don't have a value-race. But if
        // we were to disable ints here, we might observe a second
        // edge that we'll otherwise miss. Not a problem in this
        // use case, as the radio won't release IRQs until we
        // explicitly clear them.
        iTime = interrupt_time[i];
        if (iTime) {
            interrupt_time[i] = 0;
            radio_irq_handler_v2(i, iTime);
        }
    }
}

// -----------------------------------------------------------------------------
// SPI

static void lmic_hal_spi_init () {
    SPI.begin();
}

#if (defined(CFG_sx1261_radio) || defined(CFG_sx1262_radio))
bit_t lmic_hal_radio_spi_is_busy() {
    // SX126x uses BUSY pin
    return digitalRead(pHalConfig->queryBusyPin()) ? true : false;
}
#else
// supply a definition just in case, because the declaration is not conditional
bit_t lmic_hal_radio_spi_is_busy() {
    return false;
}
#endif // (defined(CFG_sx1261_radio) || defined(CFG_sx1262_radio))

static void lmic_hal_spi_trx(u1_t cmd, u1_t* buf, size_t len, bit_t is_read) {
    uint32_t spi_freq;
    u1_t nss = plmic_pins->nss;

    if ((spi_freq = plmic_pins->spi_freq) == 0)
        spi_freq = LMIC_SPI_FREQ;

    SPISettings settings(spi_freq, MSBFIRST, SPI_MODE0);
    SPI.beginTransaction(settings);
    digitalWrite(nss, 0);

    // SX126x modems use BUSY pin. Only interact with SPI when BUSY goes LOW 
#if (defined(CFG_sx1261_radio) || defined(CFG_sx1262_radio))
    while (lmic_hal_radio_spi_is_busy());
#endif

    SPI.transfer(cmd);

    for (; len > 0; --len, ++buf) {
        u1_t data = is_read ? 0x00 : *buf;
        data = SPI.transfer(data);
        if (is_read)
            *buf = data;
    }

    digitalWrite(nss, 1);
    SPI.endTransaction();
}

void lmic_hal_spi_write(u1_t cmd, const u1_t* buf, size_t len) {
    lmic_hal_spi_trx(cmd, (u1_t*)buf, len, 0);
}

void lmic_hal_spi_read(u1_t cmd, u1_t* buf, size_t len) {
    lmic_hal_spi_trx(cmd, buf, len, 1);
}

// SX126x modems behave slightly differently to SX127x. They will often need to transfer multiple bytes before reading
#if (defined(CFG_sx1261_radio) || defined(CFG_sx1262_radio))
void lmic_hal_spi_read_sx126x(u1_t cmd, u1_t* addr, size_t addr_len, u1_t* buf, size_t buf_len) {
    uint32_t spi_freq;
    u1_t nss = plmic_pins->nss;

    if ((spi_freq = plmic_pins->spi_freq) == 0)
        spi_freq = LMIC_SPI_FREQ;

    SPISettings settings(spi_freq, MSBFIRST, SPI_MODE0);
    SPI.beginTransaction(settings);
    digitalWrite(nss, 0);

    while (lmic_hal_radio_spi_is_busy());

    SPI.transfer(cmd);

    // Transfer address and NOP bits 
    for (; addr_len > 0; --addr_len, ++addr) {
        u1_t addr_byte = *addr;
        SPI.transfer(addr_byte);
    }

    // Read buf_len bytes to buf
    for (; buf_len > 0; --buf_len, ++buf) {
        u1_t data = 0x00;
        data = SPI.transfer(data);
        *buf = data;
    }

    digitalWrite(nss, 1);
    SPI.endTransaction();
}
#endif

// -----------------------------------------------------------------------------
// TIME

// Returns the number of ticks until time. Negative values indicate that
// time has already passed.
static s4_t delta_time(u4_t time) {
    return (s4_t)(time - LMIC_OsTime_ticks());
}

// deal with boards that are stressed by no-interrupt delays #529, etc.
#if defined(ARDUINO_DISCO_L072CZ_LRWAN1)
# define HAL_WAITUNTIL_DOWNCOUNT_MS 16      // on this board, 16 ms works better
# define HAL_WAITUNTIL_DOWNCOUNT_THRESH ms2osticks(16)  // as does this threashold.
#else
# define HAL_WAITUNTIL_DOWNCOUNT_MS 8       // on most boards, delay for 8 ms
# define HAL_WAITUNTIL_DOWNCOUNT_THRESH ms2osticks(9) // but try to leave a little slack for final timing.
#endif

u4_t lmic_hal_waitUntil (u4_t time) {
    s4_t delta = delta_time(time);
    // check for already too late.
    if (delta < 0)
        return -delta;

    // From delayMicroseconds docs: Currently, the largest value that
    // will produce an accurate delay is 16383. Also, STM32 does a better
    // job when delay is less than 10,000 us; so reduce in steps.
    // It's nice to use delay() for the longer times.
    while (delta > HAL_WAITUNTIL_DOWNCOUNT_THRESH) {
        // deliberately delay 8ms rather than 9ms, so we
        // will exit loop with delta typically positive.
        // Depends on BSP keeping time accurately even if interrupts
        // are disabled.
        delay(HAL_WAITUNTIL_DOWNCOUNT_MS);
        // re-synchronize.
        delta = delta_time(time);
    }

    // For the final portion, delta_time() works if interrupts are enabled.
    // The Arduino LMIC is quite careful to keep interrupts enabled.
    while (delta_time(time) > 0)
        /* loop */;

    // The API says we're supposed to return the number of ticks we're late.
    // That's a holdover from older designs. In the current LMIC, callers only
    // require that we return at or after the specified time. The above code
    // guarantees that. We return 0 to indicate that we're not "late".
    return 0;
}

// check and rewind for target time
u1_t lmic_hal_checkTimer (u4_t time) {
    // No need to schedule wakeup, since we're not sleeping
    return delta_time(time) <= 0;
}

static uint8_t irqlevel = 0;

void lmic_hal_disableIRQs () {
    noInterrupts();
    irqlevel++;
}

void lmic_hal_enableIRQs () {
    if(--irqlevel == 0) {
        interrupts();

#if !defined(LMIC_USE_INTERRUPTS)
        // Instead of using proper interrupts (which are a bit tricky
        // and/or not available on all pins on AVR), just poll the pin
        // values. Since os_runloop disables and re-enables interrupts,
        // putting this here makes sure we check at least once every
        // loop.
        //
        // As an additional bonus, this prevents the can of worms that
        // we would otherwise get for running SPI transfers inside ISRs.
        // We merely collect the edges and timestamps here; we wait for
        // a call to lmic_hal_processPendingIRQs() before dispatching.
        lmic_hal_pollPendingIRQs_helper();
#endif /* !defined(LMIC_USE_INTERRUPTS) */
    }
}

uint8_t lmic_hal_getIrqLevel(void) {
    return irqlevel;
}

void lmic_hal_sleep () {
    // Not implemented
}

// -----------------------------------------------------------------------------

#if defined(LMIC_PRINTF_TO)
#if !defined(__AVR)
static ssize_t uart_putchar (void *, const char *buf, size_t len) {
    return LMIC_PRINTF_TO.write((const uint8_t *)buf, len);
}

static cookie_io_functions_t functions =
 {
     .read = NULL,
     .write = uart_putchar,
     .seek = NULL,
     .close = NULL
 };

void lmic_hal_printf_init() {
    stdout = fopencookie(NULL, "w", functions);
    if (stdout != nullptr) {
        setvbuf(stdout, NULL, _IONBF, 0);
    }
}
#else // defined(__AVR)
static int uart_putchar (char c, FILE *)
{
    LMIC_PRINTF_TO.write(c) ;
    return 0 ;
}

void lmic_hal_printf_init() {
    // create a FILE structure to reference our UART output function
    static FILE uartout;
    memset(&uartout, 0, sizeof(uartout));

    // fill in the UART file descriptor with pointer to writer.
    fdev_setup_stream (&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);

    // The uart is the standard output device STDOUT.
    stdout = &uartout ;
}

#endif // !defined(ESP8266) || defined(ESP31B) || defined(ESP32)
#endif // defined(LMIC_PRINTF_TO)

void lmic_hal_init (void) {
    // use the global constant
    Arduino_LMIC::lmic_hal_init_with_pinmap(&lmic_pins);
}

// lmic_hal_init_ex is a C API routine, written in C++, and it's called
// with a pointer to an lmic_pinmap.
void lmic_hal_init_ex (const void *pContext) {
    const lmic_pinmap * const pHalPinmap = (const lmic_pinmap *) pContext;
    if (! Arduino_LMIC::lmic_hal_init_with_pinmap(pHalPinmap)) {
        lmic_hal_failed(__FILE__, __LINE__);
    }
}

// C++ API: initialize the HAL properly with a configuration object
namespace Arduino_LMIC {
bool lmic_hal_init_with_pinmap(const HalPinmap_t *pPinmap)
    {
    if (pPinmap == nullptr)
        return false;

    // set the static pinmap pointer.
    plmic_pins = pPinmap;

    // set the static HalConfiguration pointer.
    HalConfiguration_t * const pThisHalConfig = pPinmap->pConfig;

    if (pThisHalConfig != nullptr)
        pHalConfig = pThisHalConfig;
    else
        pHalConfig = &nullHalConig;

    pHalConfig->begin();

    // configure radio I/O and interrupt handler
    lmic_hal_io_init();
    // configure radio SPI
    lmic_hal_spi_init();
    // configure time-keeping subsystem
    LMIC_OsTime_initialize();
#if defined(LMIC_PRINTF_TO)
    // printf support
    lmic_hal_printf_init();
#endif
    // declare success
    return true;
    }
}; // namespace Arduino_LMIC


void lmic_hal_failed (const char *file, u2_t line) {
    if (custom_hal_failure_handler != NULL) {
        (*custom_hal_failure_handler)(file, line);
    }

#if defined(LMIC_FAILURE_TO)
    LMIC_FAILURE_TO.println("FAILURE ");
    LMIC_FAILURE_TO.print(file);
    LMIC_FAILURE_TO.print(':');
    LMIC_FAILURE_TO.println(line);
    LMIC_FAILURE_TO.flush();
#endif

    lmic_hal_disableIRQs();

    // Infinite loop
    while (1) {
        ;
    }
}

void lmic_hal_set_failure_handler(const lmic_hal_failure_handler_t* const handler) {
    custom_hal_failure_handler = handler;
}

ostime_t lmic_hal_setModuleActive (bit_t val) {
    // setModuleActive() takes a c++ bool, so
    // it effectively says "val != 0". We
    // don't have to.
    return pHalConfig->setModuleActive(val);
}

bit_t lmic_hal_queryUsingTcxo(void) {
    return pHalConfig->queryUsingTcxo();
}

bit_t lmic_hal_queryUsingDcdc(void) {
    return pHalConfig->queryUsingDcdc();
}

bit_t lmic_hal_queryUsingDIO2AsRfSwitch(void) {
    return pHalConfig->queryUsingDIO2AsRfSwitch();
}

bit_t lmic_hal_queryUsingDIO3AsTCXOSwitch(void) {
    return pHalConfig->queryUsingDIO3AsTCXOSwitch();
}

// Verify C++ and C sentinel values for SX126x crystal trim match.
static_assert(
    Arduino_LMIC::HalConfiguration_t::kSX126xXtalTrimUseDefault == LMIC_HAL_SX126X_XTAL_TRIM_USE_DEFAULT,
    "C++ and C sentinel values for SX126x crystal trim must match"
    );

uint8_t lmic_hal_querySX126xXTATrim(void) {
    return pHalConfig->querySX126xXTATrim();
}

uint8_t lmic_hal_querySX126xXTBTrim(void) {
    return pHalConfig->querySX126xXTBTrim();
}

uint8_t lmic_hal_getTxPowerPolicy(
    u1_t inputPolicy,
    s1_t requestedPower,
    u4_t frequency
    ) {
    return (uint8_t) pHalConfig->getTxPowerPolicy(
                        Arduino_LMIC::HalConfiguration_t::TxPowerPolicy_t(inputPolicy),
                        requestedPower,
                        frequency
                        );
}
