# Manual configuration

If your desired transceiver board is not pre-integrated, you need to provide the library with the required information.

> If you're using a [pre-integrated board](../README.md#pre-integrated-boards), you can skip this document.

## Transceiver Wiring

> If you're using a [pre-integrated board](../README.md#pre-integrated-boards), ignore this section.

You may need to wire up your transceiver. The exact
connections are a bit dependent on the transceiver board and Arduino
used, so this section tries to explain what each connection is for and
in what cases it is (not) required.

Note that the SX127x module runs at 3.3V and likely does not like 5V on
its pins (though the datasheet is not say anything about this, and my
transceiver did not obviously break after accidentally using 5V I/O for
a few hours). To be safe, make sure to use a level shifter, or an
Arduino running at 3.3V. The Semtech evaluation board has 100 ohm resistors in
series with all data lines that might prevent damage, but I would not
count on that.

You must select the proper radio for your board in [`lmic_project_config.h`](../project_config/lmic_project_config.h), using a `#define` to define the appropriate symbol chosen from `CFG_sx1272_radio`, `CFG_sx1276_radio`, `CFG_sx1261_radio` or `CFG_sx1262_radio`.

## Power

> If you're using a [pre-integrated board](../README.md#pre-integrated-boards), ignore this section.

The SX127x transceivers need a supply voltage between 1.8V and 3.9V.
Using a 3.3V supply is typical. Some modules have a single power pin
(like the HopeRF modules, labeled 3.3V) but others expose multiple power
pins for different parts (like the Semtech evaluation board that has
`VDD_RF`, `VDD_ANA` and `VDD_FEM`), which can all be connected together.
Any *GND* pins need to be connected to the Arduino *GND* pin(s).

The SX126x transceivers are able to be physically configured to use a DC-DC buck converter or linear regulator LDO type of voltage regulation. The DC-DC regulator is enabled by wiring an inductor between the `VREG` and `DCC_SW` pins, and if this is the case, it will generally be described in the technical literature for the board. The library selects LDO by default, however DC-DC can be configured as described in [Advanced initialization](HOWTO-Manually-Configure.md#advanced-initialization).

### SPI

> If you're using a [pre-integrated board](../README.md#pre-integrated-boards), ignore this section.

The primary way of communicating with the transceiver is through SPI
(Serial Peripheral Interface). This uses four pins: MOSI, MISO, SCK and
SS. The former three need to be directly connected: so MOSI to MOSI,
MISO to MISO, SCK to SCK. Where these pins are located on your Arduino
varies, see for example the "Connections" section of the [Arduino SPI
documentation](SPI).

The SS (slave select) connection is a bit more flexible. On the SPI
slave side (the transceiver), this must be connected to the pin
(typically) labeled *NSS*. On the SPI master (Arduino) side, this pin
can connect to any I/O pin. Most Arduinos also have a pin labeled "SS",
but this is only relevant when the Arduino works as an SPI slave, which
is not the case here. Whatever pin you pick, you need to tell the
library what pin you used through the pin mapping (see [below](#pin-mapping)).

[SPI]: https://www.arduino.cc/en/Reference/SPI

## DIO pins

> If you're using a [pre-integrated board](../README.md#pre-integrated-boards), ignore this section.

The DIO (digital I/O) pins on the SX127x can be configured
for various functions. The LMIC library uses them to get instant status
information from the transceiver. For example, when a LoRa transmission
starts, the DIO0 pin is configured as a TxDone output. When the
transmission is complete, the DIO0 pin is made high by the transceiver,
which can be detected by the LMIC library.

The LMIC library needs only access to DIO0, DIO1 and DIO2, the other
DIOx pins can be left disconnected. On the Arduino side, they can
connect to any I/O pin. If interrupts are used, the accuracy of timing
will be improved, particularly the rest of your `loop()` function has
lengthy calculations; but in that case, the enabled DIO pins must all
support rising-edge interrupts. See the [Timing](#timing) section below.

In LoRa mode the DIO pins are used as follows:

* DIO0: TxDone and RxDone
* DIO1: RxTimeout

In FSK mode they are used as follows::

* DIO0: PayloadReady and PacketSent
* DIO2: TimeOut

Both modes need only 2 pins, but the transceiver does not allow mapping
them in such a way that all needed interrupts map to the same 2 pins.
So, if both LoRa and FSK modes are used, all three pins must be
connected.

The pins used on the Arduino side should be configured in the pin
mapping in your sketch, by setting the values of `lmic_pinmap::dio[0]`, `[1]`, and `[2]` (see [below](#pin-mapping)).

The SX126x transceivers differ substantially from the SX127x transceivers in their I/O principle of operation. The modem uses a BUSY control line to indicate whether the transceiver is ready to accept a command from the host controller. The three DIOs (DIO1, DIO2 and DIO3) are all able to be configured as IRQs, however DIO2 and DIO3 can also be used to control an external [RfSwitch](HOWTO-Manually-Configure.md#rxtx) and TCXO respectively. For this reason, the driver used in this library exclusively uses DIO1 for interrupts. In the event a user of the library identifies a board that is not compatible with this arrangement, please create an issue.

## Reset

> If you're using a [pre-integrated board](../README.md#pre-integrated-boards), ignore this section.

The transceiver has a reset pin that can be used to explicitly reset
it. The LMIC library uses this to ensure the chip is in a consistent
state at startup. In practice, this pin can be left disconnected, since
the transceiver will already be in a sane state on power-on, but
connecting it might prevent problems in some cases.

On the Arduino side, any I/O pin can be used. The pin number used must
be configured in the pin mapping `lmic_pinmap::rst` field (see [below](#pin-mapping)).

## RXTX

> If you're using a [pre-integrated board](../README.md#pre-integrated-boards), ignore this section.


The transceiver contains two separate antenna connections: One for RX
and one for TX. A typical transceiver board contains an antenna switch
chip, that allows switching a single antenna between these RX and TX
connections.  Such a antenna switcher can typically be told what
position it should be through an input pin, often labeled *RXTX*.

The easiest way to control the antenna switch is to use the *RXTX* pin
on the SX127x transceiver. This pin is automatically set high during TX
and low during RX. For example, the HopeRF boards seem to have this
connection in place, so they do not expose any *RXTX* pins and the pin
can be marked as unused in the pin mapping.

Some boards do expose the antenna switcher pin, and sometimes also the
SX127x *RXTX* pin. For example, the SX1272 evaluation board calls the
former *FEM_CTX* and the latter *RXTX*. Again, simply connecting these
together with a jumper wire is the easiest solution.

Alternatively, or if the SX127x *RXTX* pin is not available, LMIC can be
configured to control the antenna switch. Connect the antenna switch
control pin (e.g. *FEM_CTX* on the Semtech evaluation board) to any I/O
pin on the Arduino side, and configure the pin used in the pin map (see
[below](#pin-mapping)).

The configuration entry `lmic_pinmap::rxtx` configures the pin to be used for the *RXTX* control function, in terms of the Arduino `wire.h` digital pin number. If set to `LMIC_UNUSED_PIN`, then the library assumes that software does not need to control the antenna switch.

## RXTX Polarity

> If you're using a [pre-integrated board](../README.md#pre-integrated-boards), ignore this section.

If an external switch is used, you also must specify the polarity. Some modules want *RXTX* to be high for transmit, low for receive; Others want it to be low for transmit, high for receive. The Murata module, for example, requires that *RXTX* be *high* for receive, *low* for transmit.

The configuration entry `lmic_pinmap::rxtx_rx_active` should be set to the state to be written to the *RXTX* pin to make the receiver active. The opposite state is written to make the transmitter active. If `lmic_pinmap::rxtx` is `LMIC_UNUSED_PIN`, then the value of `lmic_pinmap::rxtx_rx_active` is ignored.

## Pin mapping

> If you're using a [pre-integrated board](../README.md#pre-integrated-boards), ignore this section.

Refer to the documentation on your board for the required settings.

Remember, for pre-integrated boards, you don't worry about this.

We have details for the following manually-configured boards here:

- [LoRa Nexus by Ideetron](#lora-nexus-by-ideetron)

If your board is not configured, you need at least to provide your own `lmic_pinmap`. As described above, a variety of configurations are possible. To tell the LMIC library how your board is configured, you must declare a variable containing a pin mapping struct in your sketch file.  If you call `os_init()` to initialize the LMIC, you must name this structure `lmic_pins`. If you call `os_init_ex()`, you may name the structure what you like, but you pass a pointer as the parameter to `os_init_ex()`.

Here's an example of a simple initialization:

```c++
  lmic_pinmap lmic_pins = {
    .nss = 6,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 5,
    .dio = {2, 3, 4},
    // optional: set polarity of rxtx pin.
    .rxtx_rx_active = 0,
    // optional: set RSSI cal for listen-before-talk
    // this value is in dB, and is added to RSSI
    // measured prior to decision.
    // Must include noise guardband! Ignored in US,
    // EU, IN, other markets where LBT is not required.
    .rssi_cal = 0,
    // optional: override LMIC_SPI_FREQ if non-zero
    .spi_freq = 0,
  };
```

The names refer to the pins on the transceiver side, the numbers refer
to the Arduino pin numbers (to use the analog pins, use constants like
`A0`). For the DIO pins, the three numbers refer to DIO0, DIO1 and DIO2
respectively. Any pins that are not needed should be specified as
`LMIC_UNUSED_PIN`. The NSS and dio0 pins are required. The others can
potentially left out (depending on the environments and requirements,
see the notes above for when a pin can or cannot be left out).

## Advanced initialization

> If you're using a [pre-integrated board](../README.md#pre-integrated-boards), ignore this section.

In some boards require much more advanced management. The LMIC has a very flexible framework to support this, but it requires you to do some C++ work.

1. You must define a new class derived from `Arduino_LMIC::HalConfiguration_t`. (We'll call this `cMyHalConfiguration_t`).

2. This class *may* define overrides for several methods (discussed below).

3. You must create an instance of your class, e.g.

    ```c++
    cMyHalConfiguration_t myHalConfigInstance;
    ```

4. You add another entry in your `lmic_pinmap`, `pConfig = &myHalConfigInstance`, to link your pin-map to your object.

The full example looks like this:

```c++
class cMyHalConfiguration_t : public Arduino_LMIC::HalConfiguration_t
  {
public:
  // ...
  // put your method function override declarations here.

  // this example uses RFO at 10 dBm or less, PA_BOOST up to 17 dBm,
  // or the high-power mode above 17 dBm. In other words, it lets the
  // LMIC-determined policy determine what's to be done.

  virtual TxPowerPolicy_t getTxPowerPolicy(
    TxPowerPolicy_t policy,
    int8_t requestedPower,
    uint32_t frequency
    ) override
    {
    return policy;
    }
  };
```

> [!NOTE]
> Any manually configured board using SX126x transceivers will need
> to use advanced configuration. At a minimum, `queryBusyPin(void)`
> will need to be overridden to ensure the host is able to communicate
> with the radio.

## HalConfiguration_t methods

- `ostime_t setModuleActive(bool state)` is called by the LMIC to make the module active or to deactivate it (the value of `state` is true to activate).  The implementation must turn power to the module on and otherwise prepare for it to go to work, and must return the number of OS ticks to wait before starting to use the radio.

- `void begin(void)` is called during initialization, and is your code's chance to do any early setup.

- `void end(void)` is (to be) called during late shutdown.  (Late shutdown is not implemented yet; but we wanted to add the API for consistency.)

- `bool queryUsingTcxo(void)` shall return `true` if the module uses a TCXO; `false` otherwise.

- `TxPowerPolicy_t getTxPowerPolicy(TxPowerPolicy_t policy, int8_t requestedPower, uint32_t frequency)` allows you to override the LMIC's selection of transmit power. If not provided, the default method forces the LMIC to use PA_BOOST mode. (We chose to do this because we found empirically that the Hope RF module doesn't support RFO, and because legacy LMIC code never used anything except PA_BOOST mode.)

- `queryBusyPin(void)` shall return the host controller pin that is wired to the busy pin of a SX126x family transceiver. This is mandatory for all SX126x transceivers because their communications with the host controller is different than the SX127x family.

- `queryUsingDcdc(void)` shall return true when a SX126x transceiver is wired to use the DC-DC buck converter for voltage regulation. The default (linear regulator LDO) will work in all cases, however where the DC-DC regulator is physically and used in software, improved power consumption is achieved.

- `queryUsingDIO2AsRfSwitch(void)` shall return true when a transceiver is physically configured to switch an external antenna with DIO2. When this configuration is physically enabled and selected in software, the host will allow the radio to control the RfSwitch, generally resulting in better control timing.

- `queryUsingDIO3AsTCXOSwitch(void)` shall return true when a SX126x transceiver is physically configured with DIO3 driving an external temperature controlled crystal oscillator (TXCO).

- `querySX126xXTATrim(void)` and `querySX126xXTBTrim(void)` return the crystal oscillator trim capacitance values for the SX126x XTA and XTB pins (registers 0x0911 and 0x0912). Valid trim values are 0x00 through 0x3F; the chip reset default is 0x05 for both. Return `kSX126xXtalTrimUseDefault` (0xFF) to leave the chip reset value untouched (this is the default). Some board designs require different trim values for adequate frequency accuracy.

Caution: the LMIC has no way of knowing whether the mode you return makes sense. Use of 20 dBm mode without limiting duty cycle can over-stress your module. The LMIC currently does not have any code to duty-cycle US transmissions at 20 dBm. If properly limiting transmissions to 400 milliseconds, a 1% duty-cycle means at most one message every 40 seconds. This shouldn't be a problem in practice, but buggy upper level software still might do things more rapidly.

<!-- there are links to the following section, so be careful when renaming -->
## Example: LoRa Nexus by Ideetron

This board uses the following pin mapping:

```c++
  const lmic_pinmap lmic_pins = {
      .nss = 10,
      .rxtx = LMIC_UNUSED_PIN,
      .rst = LMIC_UNUSED_PIN, // hardwired to AtMega RESET
      .dio = {4, 5, 7},
  };
```
