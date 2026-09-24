# Timing

The library is
responsible for keeping track of time of certain network events, and scheduling
other events relative to those events. For Class A uplink transmissions, the library must note
when a packet finishes transmitting, so it can open up the RX1 and RX2
receive windows at a fixed time after the end of transmission. The library does this
by watching for rising edges on the DIO0 output of the SX127x, and noting the time.

The library observes and processes rising edges on the pins as part of `os_runloop()` processing.
This can be configured in one of two ways (see
[Controlling use of interrupts](configuration.md#controlling-use-of-interrupts)).  See [Interrupts and Arduino system timing](#interrupts-and-arduino-system-timing) for implementation details.

By default, the library polls the enabled pins to determine whether an event has occurred. This approach
allows use of any CPU pin to sense the DIOs, and makes no assumptions about
interrupts. However, it means that the end-of-transmit event is not observed
(and time-stamped) until `os_runloop_once()` is called.

Optionally, you can configure the LMIC library to use interrupts. The
interrupt handlers capture the time of
the event. Actual processing is done the next time that `os_runloop_once()`
is called, using the captured time. However, this requires that the
DIO pins be wired to Arduino pins that support rising-edge interrupts,
and it may require custom initialization code on your platform to
hook up the interrupts.

## Controlling protocol timing

The timing of end-of-transmit interrupts is used to determine when to open the downlink receive window. Because of the considerations above, some inaccuracy in the time stamp for the end-of-transmit interrupt is inevitable.

Fortunately, the timing of the receive windows at the device need not be extremely accurate; the LMIC has to turn on the receiver early enough to capture a downlink
from the gateway and must leave the receiver on long enough to compensate for timing
errors due to various inaccuracies. To make it easier for the device to catch downlinks, the gateway first transmits a preamble consisting of 8 symbols. The SX127x receiver needs to see at least 4 symbols to detect a message. The Arduino LMIC tries to enable the receiver for 6
symbol times slightly before the start of the receive window.

The HAL bases all timing on the Arduino `micros()` timer, which has a platform-specific
granularity and accuracy, and is based on the primary microcontroller clock.

If using an internal oscillator that is less than 100ppm accurate but better than 4000 ppm accurate, or if your other `loop()` processing
is time consuming, you can use [`LMIC_setClockError()`](#lmic_setclockerror)
to cause the library to leave the radio on longer. Note that for various reasons, it is not practical to set enormous clock errors. Oscillators that are 4000 ppm accurate or worse should be supplemented or disciplined with a better timing source. The LoRaWAN spec, for class B, implicitly assumes 100 ppm accuracy in the clock.

Users of older versions of the library were advised to set large clock errors if they were experiencing timing problems. However, close analysis and debugging during the preparation of v3.1.0 of this library revealed that the real errors were in the timing calculations in the library. Once those were corrected, the need for large clock error settings was reduced. It's still possible to use large clock errors if needed, but this must be enabled via a compile time switch.

An even more accurate solution could be to use a dedicated timer with an
input capture unit, that can store the timestamp of a change on the DIO0
pin (the only one that is timing-critical) entirely in hardware.
Experience shows that this is not normally required, so we leave this as
a customization to be performed on a platform-by-platform basis. We provide
a special API, `radio_irq_handler_v2(u1_t dio, ostime_t tEvent)`. This
API allows you to supply a hardware-captured time for extra accuracy.

The practical consequence of inaccurate timing is reduced battery life;
the LMIC must turn on the receiver earlier in order to be sure to capture downlink packets. However, this is a second order effect on class A devices; every receive is preceded by a transmit, which takes approximately ten times as much power per millisecond as a receive.

## `LMIC_setClockError()`

You may call this routine during initialization to inform the LMIC code about the timing accuracy of your system.

```c++
enum { MAX_CLOCK_ERROR = 65535 };

void LMIC_setClockError(
    u2_t error
);
```

This function sets the anticipated relative clock error. `MAX_CLOCK_ERROR`
represents +/- 100%, and 0 represents no additional clock compensation.
To allow for an error of 20%, you would call

```c++
LMIC_setClockError(MAX_CLOCK_ERROR * 20 / 100);
```

Setting a high clock error causes the RX windows to be opened earlier than it otherwise would be. This causes more power to be consumed. For Class A devices, this extra power is not substantial, but for Class B devices, this can be significant.

For a variety of reasons, the LMIC normally ignores clock errors greater than 4000 ppm (0.4%). The compile-time flag `LMIC_ENABLE_arbitrary_clock_error` can remove this limit. To do this, define it to a non-zero value.

This clock error is not reset by `LMIC_reset()`.

## Interrupts and Arduino system timing

The IBM LMIC used as the basis for this code disables interrupts while the radio driver is active, to prevent reentrancy via `radio_irq_handler()` at unexpected moments. It uses `os_getTime()`, and assumes that `os_getTime()` still works when interrupts were disabled. This causes problems on Arduino platforms. Most board support packages use interrupts to advance `millis()` and `micros()`, and with these BSPs, `millis()` and `micros()` return incorrect values while interrupts are disabled. Although some BSPs (like the ones provided by MCCI) provide real time correctly while interrupts are disabled, this is not portable. It's not practical to make such changes in every BSP.

To avoid this, the LMIC processes events in several steps; these steps ensure that `radio_irq_handler_v2()` is only called at predictable times.

1. If interrupts are enabled via `LMIC_USE_INTERRUPTS`, hardware interrupts catch the time of the interrupt and record that the interrupt occurred. These routines rely on hardware edge-sensitive interrupts. If your hardware interrupts are level-sensitive, you must mask the interrupt somehow at the ISR. You can't use SPI routines to talk to the radio, because this may leave the SPI system and the radio in undefined states. In this configuration, `lmic_hal_io_pollIRQs()` exists but is a no-op.

2. If interrupts are not enabled via `LMIC_USE_INTERRUPTS`, the digital I/O lines are polled every so often by calling the routine `lmic_hal_io_pollIRQs()`. This routine watches for edges on the relevant digital I/O lines, and records the time of transition.

3. The LMIC `os_runloop_once()` routine calls `lmic_hal_processPendingIRQs()`. This routine uses the timestamps captured by the hardware ISRs and `lmic_hal_io_pollIRQs()` to invoke `radio_irq_hander_v2()` with the appropriate information.  `lmic_hal_processPendingIRQs()` in turn calls `lmic_hal_io_pollIRQs()` (in case interrupts are not configured).

4. For compatibility with older versions of the Arduino LMIC, `lmic_hal_enableIRQs()` also calls `lmic_hal_io_pollIRQs()` when enabling interrupts. However, it does not dispatch the interrupts to `radio_irq_handler_v2()`; this must be done by a subsequent call to `lmic_hal_processPendingIRQs()`.
