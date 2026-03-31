/*
 * Copyright (c) 2014-2016 IBM Corporation.
 * Copyright (c) 2016-2019 MCCI Corporation.
 * Copyright (c) 2023 Tristan Webber.
 * Copyright (c) 2023 Shrunk Innovation Labs.
 * All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of the <organization> nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define LMIC_DR_LEGACY 0

#include "lmic.h"
#include "../se/i/lmic_secure_element_api.h"

#if defined(CFG_sx1261_radio) || defined(CFG_sx1262_radio)
// This driver is based on Rev. 2.1 of the Semtech SX1261/2 Data Sheet DS.SX1261-2.W.APP
// ----------------------------------------
// Command Mapping        **  Chapter 11 List of Commands
#define ResetStats                                 0x00
#define ClearIrqStatus                             0x02
#define ClearDeviceErrors                          0x07
#define SetDioIrqParams                            0x08
#define WriteRegister                              0x0D
#define WriteBuffer                                0x0E
#define GetStats                                   0x10
#define GetPacketType                              0x11
#define GetIrqStatus                               0x12
#define GetRxBufferStatus                          0x13
#define GetPacketStatus                            0x14
#define GetRssiInst                                0x15
#define GetDeviceErrors                            0x17
#define ReadRegister                               0x1D
#define ReadBuffer                                 0x1E
#define SetStandby                                 0x80
#define SetRx                                      0x82
#define SetTx                                      0x83
#define SetSleep                                   0x84
#define SetRfFrequency                             0x86
#define SetCadParams                               0x88
#define Calibrate                                  0x89
#define SetPacketType                              0x8A
#define SetModulationParams                        0x8B
#define SetPacketParams                            0x8C
#define SetTxParams                                0x8E
#define SetBufferBaseAddress                       0X8F
#define SetRxTxFallbackMode                        0x93
#define SetRxDutyCycle                             0x94
#define SetPaConfig                                0x95
#define SetRegulatorMode                           0x96
#define SetDIO3AsTcxoCtrl                          0x97
#define CalibrateImage                             0x98
#define SetDIO2AsRfSwitchCtrl                      0x9D
#define StopTimerOnPreamble                        0x9F
#define SetLoRaSymbNumTimeout                      0xA0
#define GetStatus                                  0xC0
#define SetFs                                      0xC1
#define SetCad                                     0xC5
#define SetTxContinuousWave                        0xD1
#define SetTxInfinitePreamble                      0xD2

// ----------------------------------------
// Register Mapping        **  Chapter 12 Register Table
#define HoppingEnable                              0x0385
#define PacketLength                               0x0386
#define NbHoppingBlocks                            0x0387
// Appears that there are 16 NbSymbols and Freq registers, on sequential bytes
// First and last are defined here. TODO define the rest if they are actually in use.
// Each is over multiple bytes so will also need to be split out to MSB, LSB
#define NbSymbols0                                 0x0388 // 2 bytes 0x0388 - 0x0389
#define Freq0                                      0x038A // 4 bytes 0x038A - 0x038D
#define NbSymbols15                                0x03E2 // 2 bytes 0x03E2 - 0x03E3
#define Freq15                                     0x03E4 // 4 bytes 0x03E4 - 0x03E7
#define DIOxOutputEnable                           0x0580 // Use only with Semtech-provided code samples
#define DIOxInputEnable                            0x0583 // Use only with Semtech-provided code samples
#define DIOxPullupControl                          0x0584 // Use only with Semtech-provided code samples
#define DIOxPulldownControl                        0x0585 // Use only with Semtech-provided code samples
#define WhiteningInitialMSB                        0x06B8 // Reset Value 0xX1 Note: X is an undefined value
#define WhiteningInitialLSB                        0x06B9 // Reset Value 0x00
#define CRCInitialMSB                              0x06BC // Reset Value 0x1D
#define CRCInitialLSB                              0x06BD // Reset Value 0x0F
#define CRCPolynomialMSB                           0x06BE // Reset Value 0x10
#define CRCPolynomialLSB                           0x06BF // Reset Value 0x21
#define SyncWord0                                  0x06C0
#define SyncWord1                                  0x06C1
#define SyncWord2                                  0x06C2
#define SyncWord3                                  0x06C3
#define SyncWord4                                  0x06C4
#define SyncWord5                                  0x06C5
#define SyncWord6                                  0x06C6
#define SyncWord7                                  0x06C7
#define NodeAddress                                0x06CD // Reset Value 0x00
#define BroadcastAddress                           0x06CE // Reset Value 0x00
#define IQPolaritySetup                            0x0736 // Reset Value 0x0D
#define LoRaSyncWordMSB                            0x0740 // Reset Value 0x14 (Private network). Set to 0x34 for Public Network
#define LoRaSyncWordLSB                            0x0741 // Reset Value 0x24  (Private network). Set to 0x44 for Public Network
#define RandomNumberGen0                           0x0819 // Can be used to generate a 32-bit random number
#define RandomNumberGen1                           0x081A // Can be used to generate a 32-bit random number
#define RandomNumberGen2                           0x081B // Can be used to generate a 32-bit random number
#define RandomNumberGen3                           0x081C // Can be used to generate a 32-bit random number
#define TxModulation                               0x0889 // Reset Value 0x01
#define RxGain                                     0x08AC // Reset Value 0x94 (Rx Power Saving Gain). Set to 0x96 for Rx Power Boost
#define TxClampConfig                              0x08D8 // Reset Value 0xC8
#define OCPConfig                                  0x08E7 // Reset Value for SX1261 = 0x18,  SX1262 = 0x38
#define RTCControl                                 0x0902 // Reset Value 0x00 - Use only with Semtech-provided workaround of Section 15.3
#define XTATrim                                    0x0911 // Reset Value 0x05
#define XTBTrim                                    0x0912 // Reset Value 0x05
#define DIO3OutVoltCont                            0x0920 // Reset Value 0x01 - Use only with Semtech-provided code samples
#define EventMask                                  0x0944 // Reset Value 0x00 - Use only with Semtech-provided code samples

#define LoRaSyncWordMSBPublic                      0x34
#define LoRaSyncWordMSBPrivate                     0x14
#define LoRaSyncWordLSBPublic                      0x44
#define LoRaSyncWordLSBPrivate                     0x24

// ----------------------------------------
// SetMode bytes        **  Chapter 13 Operational Modes Functions
#define STDBY_RC                                   0x00
#define STDBY_XOSC                                 0x01
#define calibParamALL                              0x7F

// ----------------------------------------
// IRQ Registers        **  Chapter 13 IRQ Registers
#define TxDone 1 << 0
#define RxDone 1 << 1
#define PreambleDetected 1 << 2
#define SyncWordValid 1 << 3
#define HeaderValid 1 << 4
#define HeaderErr 1 << 5
#define CrcErr 1 << 6
#define CadDone 1 << 7
#define CadDetected 1 << 8
#define Timeout 1 << 9
#define LrFhssHop 1 << 14

// ----------------------------------------
// IRQ bitmasks for LMIC.saveIrqFlags
#define IRQ_LORA_RXTOUT_MASK 0x80
#define IRQ_LORA_RXDONE_MASK 0x40
#define IRQ_LORA_CRCERR_MASK 0x20
#define IRQ_LORA_HEADER_MASK 0x10
#define IRQ_LORA_TXDONE_MASK 0x08
#define IRQ_LORA_CDDONE_MASK 0x04
#define IRQ_LORA_FHSSCH_MASK 0x02
#define IRQ_LORA_CDDETD_MASK 0x01

// ----------------------------------------
// PacketType Definitions        **  Chapter 13 SetPacketType
#define PACKET_TYPE_GFSK     0x00
#define PACKET_TYPE_LORA     0x01
#define PACKET_TYPE_LR_FHSS  0x02

// ----------------------------------------
// RampTime Definitions        **  Chapter 13 SetTxParams
#define SET_RAMP_10U   0x00
#define SET_RAMP_20U   0x01
#define SET_RAMP_40U   0x02
#define SET_RAMP_80U   0x03
#define SET_RAMP_200U  0x04
#define SET_RAMP_800U  0x05
#define SET_RAMP_1700U 0x06
#define SET_RAMP_3400U 0x07

// ----------------------------------------
// LoRa Modulation Params        **  Chapter 13 SetModulationParams
#define LORA_MODPARAM1_SF7       0x07
#define SX126x_MODPARAM2_BW_125  0x04
#define SX126x_MODPARAM2_BW_250  0x05
#define SX126x_MODPARAM2_BW_500  0x06
#define SX126x_MODPARAM3_CR__4_5 0x01
#define SX126x_MODPARAM3_CR__4_6 0x02
#define SX126x_MODPARAM3_CR__4_7 0x03
#define SX126x_MODPARAM3_CR__4_8 0x04

// ----------------------------------------
// LoRa Packet Params        **  Chapter 13 SetPacketParams
#define SX126x_IMPLICIT_HEADER_ON 0x01
#define CRC_CCIT_CONFIGURATION    1
#define CRC_CCIT_SEED             0x1D0F
#define CRC_CCIT_POLY             0x1021
#define CRC_IBM_CONFIGURATION     0
#define CRC_IBM_SEED              0xFFFF
#define CRC_IBM_POLY              0x8005
#define CRC_OFF                   0x00
#define CRC_1_BYTE                0x01
#define CRC_2_BYTE                0x02
#define CRC_1_BYTE_INV            0x04
#define CRC_2_BYTE_INV            0x06

// ----------------------------------------
// GetStatus        **  Chapter 13 GetStatus
#define SX126x_GETSTATUS_CHIPMODE_MASK 0x07 // Bits 6:4
#define SX126x_CHIPMODE_STDBY_RC       0x02
#define SX126x_CHIPMODE_STDBY_XOSC     0x03
#define SX126x_CHIPMODE_FS             0x04
#define SX126x_CHIPMODE_RX             0x05
#define SX126x_CHIPMODE_TX             0x06

// ----------------------------------------
// Named constants        **  Added by author for ease of reading
#define SX126X_RW_REGISTER_LEN       3
#define SX126X_W_BUFFER_LEN          2
#define SX126X_SLEEPCONFIG_LEN       1
#define SX126X_STDBYCONFIG_LEN       1
#define SX126X_TIMEOUT_LEN           3
#define SX126X_REGMODEPARAM_LEN      1
#define SX126X_CALIBPARAM_LEN        1
#define SX126X_IMAGECALPARAM_LEN     2
#define SX126X_PACONFIGPARAM_LEN     4
#define SX126X_SETIRQPARAMS_LEN      8
#define SX126X_IRQSTATUS_LEN         2
#define SX126X_CLEARIRQPARAMS_LEN    2
#define SX126X_RFFREQPARAMS_LEN      4
#define SX126X_GFSK_MODPARAMS_LEN    8
#define SX126X_LORA_MODPARAMS_LEN    4
#define SX126X_GFSK_PACKETPARAMS_LEN 9
#define SX126X_LORA_PACKETPARAMS_LEN 6
#define SX126X_RAND_SEED_LEN         16
#define SX126X_RXBUFFERSTATUS_LEN    2
#define SX126X_PACKETSTATUS_LEN      3
#define SX126X_BUFF_BASE_ADDR_LEN    2

#define SX126X_NOP         0x00
#define SX126X_FIFO_OFFSET 0x00

// ----------------------------------------
// Functions        **  Typically following the names in the Semtech data sheet

// RADIO STATE
// (initialized by radio_init(), used by radio_rand1())
static u1_t randbuf[SX126X_RAND_SEED_LEN];
static u4_t randround;
static u1_t randseed[SX126X_RAND_SEED_LEN];

// ----------------------------------------
// Chapter 13.2: Registers and Buffer Access Functions
// Write one byte `data` to register `addr`
static void writeRegister(u2_t addr, u1_t data) {
    u1_t addr_buf[SX126X_RW_REGISTER_LEN] = {
        (u1_t)(addr >> 8),
        (u1_t)(addr & 0xff),
        data,
    };

    lmic_hal_spi_write(WriteRegister, addr_buf, SX126X_RW_REGISTER_LEN);
}

// Return one byte from register `addr`
static u1_t readRegister(u2_t addr) {
    u1_t addr_buf[SX126X_RW_REGISTER_LEN] = {
        (u1_t)(addr >> 8),
        (u1_t)(addr & 0xff),
        SX126X_NOP,
    };
    u1_t buf;
    lmic_hal_spi_read_sx126x(ReadRegister, addr_buf, SX126X_RW_REGISTER_LEN, &buf, 1);
    return buf;
}

// Write `len` bytes from `buf` to the FIFO buffer starting at buffer address `addr`
static void writeBuffer(u1_t addr, xref2u1_t buf, u1_t len) {
    // Set the TX buffer base address. Leave RX base address as 0
    const u1_t baseAddr[SX126X_BUFF_BASE_ADDR_LEN] = {addr, 0};
    lmic_hal_spi_write(SetBufferBaseAddress, baseAddr, sizeof(baseAddr));

    // Prepend the offset byte to the data being written to the buffer
    // TODO(tmm@mcci.com): rewrite following #1043 to avoid extra copy,
    // extra stack use.
    u1_t new_buf[256];
    new_buf[0] = SX126X_FIFO_OFFSET;
    os_copyMem(new_buf + 1, buf, len);

    lmic_hal_spi_write(WriteBuffer, new_buf, len + 1);
}

// Read `len` bytes from the FIFO buffer to `buf` from position `offset`
static void readBuffer(u1_t offset, xref2u1_t buf, u1_t len) {
    u1_t offset_buf[SX126X_W_BUFFER_LEN] = {
        offset,
        SX126X_NOP,
    };

    lmic_hal_spi_read_sx126x(ReadBuffer, offset_buf, SX126X_W_BUFFER_LEN, buf, len);
}

// Chapter 13.1: Operational Modes Functions
// These functions have been developed to reflect the SX126x API

// Accepts 1 byte to choose warm or cold start and interrupt type. Default 0x00 for cold start
// TODO setup state machine to prevent the device from waking if it's already asleep
static void setSleep(u1_t sleepConfig) {
    // MISSING FUNCTIONALITY sleepConfig can be modified to allow warm start and set an RTC timeout
    // default coldstart sleepConfig = 0x00;
    lmic_hal_spi_write(SetSleep, &sleepConfig, SX126X_SLEEPCONFIG_LEN);
}

// Accepts StdbyConfig STDBY_RC or STDBY_XOSC
static void setStandby(u1_t stdbyConfig) {
    lmic_hal_spi_write(SetStandby, &stdbyConfig, SX126X_STDBYCONFIG_LEN);
}

static void setFs(void) {
    lmic_hal_spi_write(SetFs, NULL, 0);
}

// TODO handle the bit bashing as a macro to allow functions to be called with ms rather than 3 bytes
static void setTx(u1_t timeout[SX126X_TIMEOUT_LEN]) {
    lmic_hal_spi_write(SetTx, timeout, SX126X_TIMEOUT_LEN);
}

// TODO handle the bit bashing as a macro to allow functions to be called with ms rather than 3 bytes
static void setRx(u1_t timeout[SX126X_TIMEOUT_LEN]) {
    lmic_hal_spi_write(SetRx, timeout, SX126X_TIMEOUT_LEN);

    // Workaround 15.3 - Implicit Header Mode Timeout Behaviour
    // It is advised to add the following commands after ANY Rx with Timeout active sequence,
    // which stop the RTC and clear the timeout event, if any.
    u1_t hasTimeout = 0;
    for (u1_t i; i < SX126X_TIMEOUT_LEN; i++) {
        if ((timeout[i] != 0x00) && (timeout[i] !=0xFF)) {
            hasTimeout = 1;
            break;
        }
    }

    if (hasTimeout) {
        writeRegister(RTCControl, 0x00);
        writeRegister(EventMask, (readRegister(EventMask) | 0x02));
    }
}

// Default is LDO. DC-DC can be activated by setting regModeParam to 0x01
// NOTE: This is hardware specific and will need to be described in the pinmap files
static void setRegulatorMode(u1_t regModeParam) {
    lmic_hal_spi_write(SetRegulatorMode, &regModeParam, SX126X_REGMODEPARAM_LEN);
}

// 0x7F calibrates all. It's possible to use other bits to calibrate fewer blocks and expedite the cal but for first pass, setting as a default for ease of implementation
static void calibrate(u1_t calibParam) {
    lmic_hal_spi_write(Calibrate, &calibParam, SX126X_CALIBPARAM_LEN);
}

static void calibrateImage(void) {
    u1_t calFreq[2];

    // Values from Table 9-2 of data sheet
    // It appears the correct range of LMIC.freq is managed in LMIC
    if (LMIC.freq > 902000000) {
        calFreq[0] = 0xE1;
        calFreq[1] = 0xE9;
    } else if (LMIC.freq > 863000000) {
        calFreq[0] = 0xD7;
        calFreq[1] = 0xDB;
    } else if (LMIC.freq > 779000000) {
        calFreq[0] = 0xC1;
        calFreq[1] = 0xC5;
    } else if (LMIC.freq > 470000000) {
        calFreq[0] = 0x75;
        calFreq[1] = 0x81;
    } else {
        calFreq[0] = 0x6B;
        calFreq[1] = 0x6F;
    }

    lmic_hal_spi_write(CalibrateImage, calFreq, SX126X_IMAGECALPARAM_LEN);
}

static void setPaConfig(u1_t paDutyCycle, u1_t hpMax, u1_t deviceSel, u1_t paLut) {
    u1_t paConfigParam[SX126X_PACONFIGPARAM_LEN] = {
        paDutyCycle,
        hpMax,
        deviceSel,
        paLut,
    };

    lmic_hal_spi_write(SetPaConfig, paConfigParam, SX126X_PACONFIGPARAM_LEN);
}

// Chapter 13.3: DIO and IRQ Control Functions
static void setDioIrqParams(u2_t irqMask, u2_t dio1Mask, u2_t dio2Mask, u2_t dio3Mask) {
    u1_t irqParams[SX126X_SETIRQPARAMS_LEN] = {
        irqMask >> 8,
        irqMask & 0xFF,
        dio1Mask >> 8,
        dio1Mask & 0xFF,
        dio2Mask >> 8,
        dio2Mask & 0xFF,
        dio3Mask >> 8,
        dio3Mask & 0xFF,
    };

    lmic_hal_spi_write(SetDioIrqParams, irqParams, SX126X_SETIRQPARAMS_LEN);
}

static u2_t getIrqStatus(void) {
    u1_t nop = SX126X_NOP;
    u1_t buf[SX126X_IRQSTATUS_LEN];
    lmic_hal_spi_read_sx126x(GetIrqStatus, &nop, 1, buf, SX126X_IRQSTATUS_LEN);
    u2_t irqStatus = (buf[0] << 8) | buf[1];
    return irqStatus;
}

static void clearIrqStatus(u2_t clearIrqArg) {
    u1_t clearIrqParams[SX126X_CLEARIRQPARAMS_LEN] = {
        (u1_t)(clearIrqArg >> 8),
        (u1_t)(clearIrqArg & 0xff),
    };
    lmic_hal_spi_write(ClearIrqStatus, clearIrqParams, SX126X_CLEARIRQPARAMS_LEN);
}

// Allows direct control of RFswitch by SX126x if the hardware supports it
static void setDio2AsRfSwitchCtrl(void) {
    u1_t enable = 0x01;
    lmic_hal_spi_write(SetDIO2AsRfSwitchCtrl, &enable, 1);
}

// Allows direct control of TCXO by SX126x if the hardware supports it
static void setDIO3AsTcxoCtrl(float tcxoVoltage, u1_t delay[SX126X_TIMEOUT_LEN]) {
    u1_t tcxoVoltage_int = tcxoVoltage * 10;
    u1_t voltageParam;
    u1_t setDio3AsTcxoParam[SX126X_TIMEOUT_LEN + 1];

    switch (tcxoVoltage_int)
    {
    case 16:
        voltageParam = 0x00;
        break;
    case 17:
        voltageParam = 0x01;
        break;
    case 18:
        voltageParam = 0x02;
        break;
    case 22:
        voltageParam = 0x03;
        break;
    case 24:
        voltageParam = 0x04;
        break;
    case 27:
        voltageParam = 0x05;
        break;
    case 30:
        voltageParam = 0x06;
        break;
    case 33:
        voltageParam = 0x07;
        break;
    default:
        ASSERT(0);
    }

    for (u1_t i = 0; i < (SX126X_TIMEOUT_LEN + 1); i++) {
        setDio3AsTcxoParam[i] = (i == 0) ? voltageParam : delay[i - 1];
    }

    lmic_hal_spi_write(SetDIO3AsTcxoCtrl, setDio3AsTcxoParam, (SX126X_TIMEOUT_LEN + 1));
}

// Chapter 13.4: RF Modulation and Packet-Related Functions

static void setRfFrequency(void) {
    // set frequency: freq = (rfFreq * 32 Mhz) / (2 ^ 25)
    u4_t rfFreq = ((uint64_t)LMIC.freq << 25) / 32000000;
    u1_t rfFreqParam[SX126X_RFFREQPARAMS_LEN] = {
        (u1_t)(rfFreq >> 24),
        (u1_t)(rfFreq >> 16),
        (u1_t)(rfFreq >> 8),
        (u1_t)(rfFreq >> 0),
    };

    lmic_hal_spi_write(SetRfFrequency, rfFreqParam, SX126X_RFFREQPARAMS_LEN);
}

static void setPacketType(u1_t packetType) {
    lmic_hal_spi_write(SetPacketType, &packetType, 1);
}

static u1_t getPacketType(void) {
    u1_t nop = SX126X_NOP;
    u1_t buf;
    lmic_hal_spi_read_sx126x(GetPacketType, &nop, 1, &buf, 1);
    return(buf);
}

// The `setTxParams` function can reuse parts of `configPower` from original radio.c
// On the SX126x, power configs are simplified relative to SX127x because we
// only have one PA available. We just need to make sure that:
// 1) The selected power is within the expected range of the modem, and
// 2) The optimal PAConfig settings are selected, and
// 3) Finally, set the TxPower and RampTime
//
// We otherwise assume that the bandplan and LMIC have selected an appropriate
// power for the location and frequency
//
// The SX1261 low power PA expected range is -17 (0xEF) to +14 (0x0E), 1dB steps
// The SX1262 high power PA expected range is -9 (0xF7) to +22 (0x16), 1dB steps
// Note that the documentation contains a greater number of optimal setPaConfig
// params than is used here. This might become a TODO, to implement the rest of
// the optimal setPaParams
static void setTxParams(void) {
    s1_t setTxPower = LMIC.radio.txpow;
    #ifdef CFG_sx1261_radio
    if (setTxPower == 15) {
        setPaConfig(0x06, 0x00, 0x01, 0x01);
    } else {
        setPaConfig(0x04, 0x00, 0x01, 0x01);
    }

    if (setTxPower >= 14) {
        setTxPower = 14;
    } else if (setTxPower < -17) {
        setTxPower = -17;
    }

    #elif CFG_sx1262_radio
    setPaConfig(0x04, 0x07, 0x00, 0x01);
    if (setTxPower > 22) {
        setTxPower = 22;
    } else if (setTxPower < -9) {
        setTxPower = -9;
    }
    #endif

    u1_t txParams[2] = {
        setTxPower,
        SET_RAMP_40U,
    };

    lmic_hal_spi_write(SetTxParams, txParams, 2);

    // TODO adjust OCP based on tx power to save power
    // Eg SX1262 < 14, set ma to 80. (20 > pa >= 14), set ma to 100. >= 20 then leave at 140ma default that occurs after PAconfig
}

// The `setModulationParams` function can largely reuse `configLoraModem` from original radio.c
static void setModulationParams(u1_t packetType) {
    if (packetType == PACKET_TYPE_LORA) {

        u1_t modParams[SX126X_LORA_MODPARAMS_LEN] = {0};

        // LoRa ModParam1 - SF
        // Stored in rps as an enum and mapped to params
        sf_t sf = getSf(LMIC.rps);
        modParams[0] = (LORA_MODPARAM1_SF7 - SF7 + sf);

        // LoRa ModParam2 - BW
        bw_t const bw = getBw(LMIC.rps);
        switch (bw) {
        case BW125: modParams[1] = SX126x_MODPARAM2_BW_125; break;
        case BW250: modParams[1] = SX126x_MODPARAM2_BW_250; break;
        case BW500: modParams[1] = SX126x_MODPARAM2_BW_500; break;
        default:
            ASSERT(0);
        }

        // Workaround 15.1: Modulation quality with 500 kHz LoRa bandwidth
        if ((packetType == PACKET_TYPE_LORA) && (bw == BW500)) {
            writeRegister(TxModulation, (readRegister(TxModulation) & 0xFB));
        } else {
            writeRegister(TxModulation, (readRegister(TxModulation) | 0x04));
        }

        // LoRa ModParam3 - CR
        cr_t const cr = getCr(LMIC.rps);
        switch (cr) {
        case CR_4_5: modParams[2] = SX126x_MODPARAM3_CR__4_5; break;
        case CR_4_6: modParams[2] = SX126x_MODPARAM3_CR__4_6; break;
        case CR_4_7: modParams[2] = SX126x_MODPARAM3_CR__4_7; break;
        case CR_4_8: modParams[2] = SX126x_MODPARAM3_CR__4_8; break;
        default:
            ASSERT(0);
        }

        // LoRa ModParam4 - LDRO
        if (((sf == SF11 || sf == SF12) && bw == BW125) || (sf == SF12 && bw == BW250)) {
            modParams[3] = 0x01;
        }

        lmic_hal_spi_write(SetModulationParams, modParams, SX126X_LORA_MODPARAMS_LEN);

    } else {
        // GFSK portion NOT TESTED yet.
        u1_t modParams[SX126X_GFSK_MODPARAMS_LEN] = {0};

        // GFSK ModParam1, 2 & 3 - br
        // br = 32 * Fxtal / bit rate. Assuming Fxtal in Hz (32MHz) and bit rate in bytes
        // So br for 50kbps = 0x005000
        modParams[0] = 0x00;
        modParams[1] = 0x50;
        modParams[2] = 0x00;

        // GFSK ModParam4 - PulseShape
        // select Gaussian filter BT=0.5
        modParams[3] = 0x09;

        // GFSK ModParam5 - BW
        // select RX_BW_117300 (117.3 kHz DSB)
        modParams[4] = 0x0B;

        // GFSK ModParam6, 7 & 8 - Fdev
        // Fdev = (Frequency Deviation * 2^25) / Fxtal
        // Using + / - 25kHz, Fdev = 0x006666
        modParams[5] = 0x00;
        modParams[6] = 0x66;
        modParams[7] = 0x66;

        lmic_hal_spi_write(SetModulationParams, modParams, SX126X_GFSK_MODPARAMS_LEN);
    }
}

// The `setPacketParams` function sets several parameters that the original radio.c achieved using individual writeReg calls
static void setPacketParams(u1_t packetType, u1_t frameLength, u1_t invertIQ) {
    if (packetType == PACKET_TYPE_LORA) {

        u1_t packetParams[SX126X_LORA_PACKETPARAMS_LEN] = {0};

        // LoRa PacketParam1 (MSB) & 2 (LSB) - PreambleLength
        // The existing radio.c appears to use a default 8 bit preamble
        packetParams[0] = 0x00;
        packetParams[1] = 0x08;

        // LoRa PacketParam3 - HeaderType
        if (getIh(LMIC.rps)) {
            packetParams[2] = SX126x_IMPLICIT_HEADER_ON;
        }

        // LoRa PacketParam4, 5, 6 - PayloadLength, CRC, Invert IQ in RX
        packetParams[3] = frameLength;
        packetParams[4] = getNocrc(LMIC.rps) ? 0x00 : 0x01;
        packetParams[5] = invertIQ ? 0x01 : 0x00;

        lmic_hal_spi_write(SetPacketParams, packetParams, SX126X_LORA_PACKETPARAMS_LEN);

        // Workaround 15.4 - Optimising the Inverted IQ Operation
        if (invertIQ) {
            // When using inverted IQ polarity, bit 2 at register 0x0736 must be set to 0
            writeRegister(IQPolaritySetup, (readRegister(IQPolaritySetup) & 0xFB));
        } else {
            // When using standard IQ polarity, bit 2 at register 0x0736 must be set to 1
            writeRegister(IQPolaritySetup, (readRegister(IQPolaritySetup) | 0x04));
        }

    } else if (packetType == PACKET_TYPE_GFSK) {
        // GFSK portion NOT TESTED yet.
        u1_t packetParams[SX126X_GFSK_PACKETPARAMS_LEN] = {0};

        // GFSK PacketParam1 (MSB) & 2 (LSB) - PreambleLength
        // The existing radio.c appears to use 0x0005
        packetParams[0] = 0x00;
        packetParams[1] = 0x05;

        // GFSK PacketParam3 - PreambleDetectorLength
        // 8 bit preamble
        packetParams[2] = 0x04;

        // GFSK PacketParam4 - SyncWordLength in bits
        // The existing radio.c appears to use a 3 byte sync word
        packetParams[3] = 0x18;

        // GFSK PacketParam5 - AddrComp
        // The existing radio.c appears to disable
        packetParams[4] = 0x00;

        // GFSK PacketParam6 - PacketType
        // The existing radio.c appears to use variable length
        packetParams[5] = 0x01;

        // GFSK PacketParam7 - PayloadLength
        packetParams[6] = frameLength;

        // Section 6.2.3.5 of the Data Sheet
        // GFSK PacketParam8 - CRCType - Default values are CCIT, but must be overwritten if IBM CRC is used
        if (CRC_IBM_CONFIGURATION == 1) {
            writeRegister(CRCInitialMSB, (u1_t) ((CRC_IBM_SEED >> 8) & 0xFF));
            writeRegister(CRCInitialLSB, (u1_t) (CRC_IBM_SEED & 0xFF));
            writeRegister(CRCPolynomialMSB, (u1_t) ((CRC_IBM_POLY >> 8) & 0xFF));
            writeRegister(CRCPolynomialLSB, (u1_t) (CRC_IBM_POLY & 0xFF));
            packetParams[7] = CRC_2_BYTE;
        } else {
            packetParams[7] = CRC_2_BYTE_INV;
        }

        // GFSK PacketParam9 - WhiteningEnable
        packetParams[8] = 0x01;

        lmic_hal_spi_write(SetPacketParams, packetParams, SX126X_LORA_PACKETPARAMS_LEN);

        // Sync word is directly programmed into the device through simple register access.
        writeRegister(SyncWord0, 0xC1);
        writeRegister(SyncWord1, 0x94);
        writeRegister(SyncWord2, 0xC1);
        writeRegister(WhiteningInitialLSB, 0xFF);

        // Keep default values for whitening
    } else {
        // No other packet type supported
        ASSERT(0);
    }
}

// Sets base address for TX and RX as 0x00. Possible to set to other values, but we're not using that in this implementation
static void setBufferBaseAddress(void) {
    u1_t buf[2] = {
        0x00, // TX base address
        0x00 // RX base address
    };
    lmic_hal_spi_write(SetBufferBaseAddress, buf, 2);
}

static void setLoRaSymbNumTimeout(void) {
    u1_t buf = {(u1_t)LMIC.rxsyms};
    lmic_hal_spi_write(SetLoRaSymbNumTimeout, &buf, 1);
}

// Chapter 13.5: Communication Status Information

// Chip mode is (getStatus | 0x70)
static u1_t getStatus(void) {
    u1_t status;
    lmic_hal_spi_read_sx126x(GetStatus, NULL, 0, &status, 1);
    return status;
}

static void getDeviceErrors(xref2cu1_t errorBuf) {
    u1_t nop = SX126X_NOP;
    u1_t errors[2];
    lmic_hal_spi_read_sx126x(GetDeviceErrors, &nop, 1, errors, 2);
}

static void clearDeviceErrors(void) {
    u1_t buf[2] = {0};
    lmic_hal_spi_write(ClearDeviceErrors, buf, 2);
}

static void getRxBufferStatus(xref2u1_t rxBufferStatus) {
    u1_t nop = SX126X_NOP;
    lmic_hal_spi_read_sx126x(GetRxBufferStatus, &nop, 1, rxBufferStatus, SX126X_RXBUFFERSTATUS_LEN);
}

static void getPacketStatus(xref2u1_t rxBufferStatus) {
    u1_t nop = SX126X_NOP;
    u1_t buf[SX126X_PACKETSTATUS_LEN];
    lmic_hal_spi_read_sx126x(GetPacketStatus, &nop, 1, buf, SX126X_PACKETSTATUS_LEN);
}

// Perform radio configuration commands required at the start of tx and rx
void radio_config(void) {
    // Perform necessary operations from STDBY_RC mode
    if ((getStatus() | SX126x_GETSTATUS_CHIPMODE_MASK) != SX126x_CHIPMODE_STDBY_RC) {
        // Assume we've woken from sleep
        while (lmic_hal_radio_spi_is_busy());
        setStandby(STDBY_RC);
    }

    // If the board has RfSwitch, switch on
    if (lmic_hal_queryUsingDIO2AsRfSwitch) {
        setDio2AsRfSwitchCtrl();
    }

    // Workaround 15.2  - Better Resistance of the SX1262 Tx to Antenna Mismatch
    // This register modification must be done after a Power On Reset, or a wake-up from cold Start.
    writeRegister(TxClampConfig, (readRegister(TxClampConfig) | 0x1E));

    // Apply board-specific crystal oscillator trim values if configured.
    // Some board designs require different XTA/XTB trim capacitance than the
    // chip reset default (0x05) for adequate frequency accuracy.
    {
        uint8_t xta = lmic_hal_querySX126xXTATrim();
        uint8_t xtb = lmic_hal_querySX126xXTBTrim();
        if (xta != LMIC_HAL_SX126X_XTAL_TRIM_USE_DEFAULT)
            writeRegister(XTATrim, xta);
        if (xtb != LMIC_HAL_SX126X_XTAL_TRIM_USE_DEFAULT)
            writeRegister(XTBTrim, xtb);
    }

    // DC-DC regulator is hardware dependent
    if (lmic_hal_queryUsingDcdc()) {
        setRegulatorMode(0x01);
    }

    // If the board has TCXO, the calibration order is important.
    if (lmic_hal_queryUsingDIO3AsTCXOSwitch()) {
        float tcxoVoltage = 1.8;
        u1_t tcxoTimeout[SX126X_TIMEOUT_LEN] = {0x00, 0x01, 0x40}; //
        setDIO3AsTcxoCtrl(tcxoVoltage, tcxoTimeout);
        calibrate(calibParamALL);
        calibrateImage();
        // Clear any errors. If using TCXO an error will appear
        clearDeviceErrors();
    }

    // Return to standby, using the 32MHz oscillator
    setStandby(STDBY_XOSC);
}

// Chapter 14.2: Circuit configuration for basic tx operation
static void txlora(void) {
    // Send configuration commands to radio
    radio_config();
    setPacketType(PACKET_TYPE_LORA);
    setRfFrequency();
    setTxParams();

    writeBuffer(0x00, LMIC.frame, LMIC.dataLen);

    lmic_hal_pin_rxtx(1);

    setModulationParams(PACKET_TYPE_LORA);
    setPacketParams(PACKET_TYPE_LORA, LMIC.dataLen, LMIC.noRXIQinversion);

    // Set DioIrq params to DIO1
    u2_t clearAllIrq = 0x03FF;
    u2_t dioMask = TxDone | Timeout;
    u2_t noMask = 0;
    clearIrqStatus(clearAllIrq);
    setDioIrqParams(dioMask, dioMask, noMask, noMask);

    // Configure LoRaSyncWord registers using public network as default
    writeRegister(LoRaSyncWordMSB, LoRaSyncWordMSBPublic);
    writeRegister(LoRaSyncWordLSB, LoRaSyncWordLSBPublic);

    // Start the transmission
    if (LMIC.txend) {
        u4_t nLate = lmic_hal_waitUntil(LMIC.txend); // busy wait until exact tx time
        if (nLate) {
            LMIC.radio.txlate_ticks += nLate;
            ++LMIC.radio.txlate_count;
        }
    }
    LMICOS_logEventUint32("+Tx LoRa", LMIC.dataLen);

    // Set 10s timeout
    u1_t timeout[SX126X_TIMEOUT_LEN] = {0x09, 0xC4, 0x00};
    setTx(timeout);

#if LMIC_DEBUG_LEVEL > 0
    u1_t sf = getSf(LMIC.rps) + 6; // 1 == SF7
    u1_t bw = getBw(LMIC.rps);
    u1_t cr = getCr(LMIC.rps);
    LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": TXMODE, freq=%"PRIu32", len=%d, SF=%d, BW=%d, CR=4/%d, IH=%d\n",
           os_getTime(), LMIC.freq, LMIC.dataLen, sf,
           bw == BW125 ? 125 : (bw == BW250 ? 250 : 500),
           cr == CR_4_5 ? 5 : (cr == CR_4_6 ? 6 : (cr == CR_4_7 ? 7 : 8)),
           getIh(LMIC.rps)
   );
#endif
}

static void txfsk(void) {
    // Send configuration commands to radio
    radio_config();
    setPacketType(PACKET_TYPE_GFSK);
    setRfFrequency();
    setTxParams();

    writeBuffer(0x00, LMIC.frame, LMIC.dataLen);

    lmic_hal_pin_rxtx(1);

    setModulationParams(PACKET_TYPE_GFSK);
    setPacketParams(PACKET_TYPE_GFSK, LMIC.dataLen, LMIC.noRXIQinversion);

    // Set DioIrq params to DIO1
    u2_t clearAllIrq = 0x03FF;
    u2_t dioMask = TxDone | Timeout;
    u2_t noMask = 0;
    clearIrqStatus(clearAllIrq);
    setDioIrqParams(dioMask, dioMask, noMask, noMask);

    // now we actually start the transmission
    if (LMIC.txend) {
        u4_t nLate = lmic_hal_waitUntil(LMIC.txend); // busy wait until exact tx time
        if (nLate > 0) {
            LMIC.radio.txlate_ticks += nLate;
            ++LMIC.radio.txlate_count;
        }
    }
    LMICOS_logEventUint32("+Tx FSK", LMIC.dataLen);

    // Set 10s timeout
    u1_t timeout[SX126X_TIMEOUT_LEN] = {0x09, 0xC4, 0x00};
    setTx(timeout);
}

// start transmitter (buf=LMIC.frame, len=LMIC.dataLen)
static void starttx(void) {
    // SX127x sets sleep however this doesn't appear to be necessary for SX126x
    setStandby(STDBY_RC);

    if (LMIC.lbt_ticks > 0) {
        oslmic_radio_rssi_t rssi;
        radio_monitor_rssi(LMIC.lbt_ticks, &rssi);
#if LMIC_X_DEBUG_LEVEL > 0
        LMIC_X_DEBUG_PRINTF("LBT rssi max:min=%d:%d %d times in %d\n", rssi.max_rssi, rssi.min_rssi, rssi.n_rssi, LMIC.lbt_ticks);
#endif

        if (rssi.max_rssi >= LMIC.lbt_dbmax) {
            // complete the request by scheduling the job
            os_setCallback(&LMIC.osjob, LMIC.osjob.func);
            return;
        }
    }

    if (getSf(LMIC.rps) == FSK) { // FSK modem
        txfsk();
    } else { // LoRa modem
        txlora();
    }
    // the radio will go back to STANDBY mode as soon as the TX is finished
    // the corresponding IRQ will inform us about completion.
}

// Removed RXMODE_RSSI because with SX126x we can get the random seed from registers
enum { RXMODE_SINGLE, RXMODE_SCAN };

static CONST_TABLE(u2_t, rxlorairqmask)[] = {
    [RXMODE_SINGLE] = RxDone | Timeout,
    [RXMODE_SCAN]   = RxDone,
};

//! \brief handle late RX events.
//! \param nLate is the number of `ostime_t` ticks that the event was late.
//! \details If nLate is non-zero, increment the count of events, totalize
//! the number of ticks late, and (if implemented) adjust the estimate of
//! what would be best to return from `os_getRadioRxRampup()`.
static void rxlate(u4_t nLate) {
    if (nLate) {
        LMIC.radio.rxlate_ticks += nLate;
        ++LMIC.radio.rxlate_count;
    }
}

// start LoRa receiver (time=LMIC.rxtime, timeout=LMIC.rxsyms, result=LMIC.frame[LMIC.dataLen])
// Chapter 14.3: Circuit configuration for basic rx operation
static void rxlora(u1_t rxmode) {
    // Send configuration commands to radio
    radio_config();
    setPacketType(PACKET_TYPE_LORA);
    setRfFrequency();

    setBufferBaseAddress();

    setModulationParams(PACKET_TYPE_LORA);

    // Adapted from radio.c. Treat default as DISABLE_INVERT_IQ_ON_RX
    u1_t invertIq = 0x00;
#if !defined(DISABLE_INVERT_IQ_ON_RX)
    // DEPRECATED(tmm@mcci.com); #250. remove test, always include code in V3
    // use inverted I/Q signal (prevent mote-to-mote communication)
    // XXX: use flag to switch on/off inversion
    invertIq = LMIC.noRXIQinversion ? 0x00 : 0x01;
#endif

    setPacketParams(PACKET_TYPE_LORA, MAX_LEN_FRAME, invertIq);

    // Rx Boosted gain. Default is Rx Power saving. Uncomment if boosted is desired
    // writeRegister(RxGain, (readRegister(RxGain) | 0x96));

    lmic_hal_pin_rxtx(0);

    // Set DioIrq params to DIO1
    u2_t dioMask = RxDone | Timeout;
    u2_t noMask = 0;
    u2_t clearAllIrq = 0x03FF;
    clearIrqStatus(clearAllIrq);
    setDioIrqParams(dioMask, dioMask, noMask, noMask);

    // Configure LoRaSyncWord registers using public network as default
    setLoRaSymbNumTimeout();
    writeRegister(LoRaSyncWordMSB, LoRaSyncWordMSBPublic);
    writeRegister(LoRaSyncWordLSB, LoRaSyncWordLSBPublic);

    // now instruct the radio to receive
    if (rxmode == RXMODE_SINGLE) {
        u4_t nLate = lmic_hal_waitUntil(LMIC.rxtime);
        u1_t rxTimeoutSingle[SX126X_TIMEOUT_LEN] = {
            0x00,
            0x00,
            0x00
        };
        setRx(rxTimeoutSingle);
        LMICOS_logEventUint32("+Rx LoRa Single", nLate);
        rxlate(nLate);
#if LMIC_DEBUG_LEVEL > 0
        ostime_t now = os_getTime();
        LMIC_DEBUG_PRINTF("start single rx: now-rxtime: %"LMIC_PRId_ostime_t"\n", now - LMIC.rxtime);
#endif
    } else {
        LMICOS_logEventUint32("+Rx LoRa Continuous", rxmode);
        u1_t rxTimeoutContinuous[SX126X_TIMEOUT_LEN] = {
            0xFF,
            0xFF,
            0xFF
        };
        setRx(rxTimeoutContinuous);
    }

#if LMIC_DEBUG_LEVEL > 0
    u1_t sf = getSf(LMIC.rps) + 6; // 1 == SF7
    u1_t bw = getBw(LMIC.rps);
    u1_t cr = getCr(LMIC.rps);
    LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": %s, freq=%"PRIu32", SF=%d, BW=%d, CR=4/%d, IH=%d\n",
        os_getTime(),
        rxmode == RXMODE_SINGLE ? "RXMODE_SINGLE" : (rxmode == RXMODE_SCAN ? "RXMODE_SCAN" : "UNKNOWN_RX"),
        LMIC.freq, sf,
        bw == BW125 ? 125 : (bw == BW250 ? 250 : 500),
        cr == CR_4_5 ? 5 : (cr == CR_4_6 ? 6 : (cr == CR_4_7 ? 7 : 8)),
        getIh(LMIC.rps)
    );
#endif
}

static void rxfsk(u1_t rxmode) {
    // only single or continuous rx (no noise sampling)
    if (rxmode == RXMODE_SCAN) {
        // indicate no bytes received.
        LMIC.dataLen = 0;
        // complete the request by scheduling the job.
        os_setCallback(&LMIC.osjob, LMIC.osjob.func);
    }

    // Send configuration commands to radio
    radio_config();
    setPacketType(PACKET_TYPE_GFSK);
    setRfFrequency();

    setBufferBaseAddress();

    setModulationParams(PACKET_TYPE_GFSK);
    setPacketParams(PACKET_TYPE_GFSK, MAX_LEN_FRAME, 0);

    // Rx Boosted gain. Default is Rx Power saving. Uncomment if boosted is desired
    // writeRegister(RxGain, (readRegister(RxGain) | 0x96));

    lmic_hal_pin_rxtx(0);

    // Set DioIrq params to DIO1
    u2_t dioMask = RxDone | Timeout;
    u2_t noMask = 0;
    u2_t clearAllIrq = 0x03FF;
    clearIrqStatus(clearAllIrq);
    setDioIrqParams(dioMask, dioMask, noMask, noMask);

    // now instruct the radio to receive
    if (rxmode == RXMODE_SINGLE) {
        u4_t nLate = lmic_hal_waitUntil(LMIC.rxtime); // busy wait until exact rx time
        u1_t rxTimeoutSingle[SX126X_TIMEOUT_LEN] = {
            0x00,
            0x00,
            0x00
        };
        setRx(rxTimeoutSingle);
        LMICOS_logEventUint32("+Rx FSK", nLate);
        rxlate(nLate);
    } else {
        LMICOS_logEvent("+Rx FSK Continuous");
        u1_t rxTimeoutContinuous[SX126X_TIMEOUT_LEN] = {
            0xFF,
            0xFF,
            0xFF
        };
        setRx(rxTimeoutContinuous);
    }
}

static void startrx(u1_t rxmode) {
    // SX127x does an assert to make sure modem is in sleep. SX126x uses standby as base mode.
    // For this driver, we force mode change, rather than assert
    setStandby(STDBY_RC);

    if(getSf(LMIC.rps) == FSK) { // FSK modem
        rxfsk(rxmode);
    } else { // LoRa modem
        rxlora(rxmode);
    }
    // the radio will go back to STANDBY mode as soon as the RX is finished
    // or timed out, and the corresponding IRQ will inform us about completion.
}

// Get random seed from registers
void randomNumber(xref2u1_t randbuf) {
    // Send configuration commands to radio
    radio_config();

    u1_t rxTimeoutContinuous[SX126X_TIMEOUT_LEN] = {
        0xFF,
        0xFF,
        0xFF
    };

    setRx(rxTimeoutContinuous);

    lmic_hal_waitUntil(os_getTime() + ms2osticks(100)); // TODO 100ms seems excessive so test and reduce

    // Fill each byte with one of the RandomNumberGen registers
    for (u1_t i = 0; i < SX126X_RAND_SEED_LEN / 4; i++) {
        randbuf[i] = readRegister(RandomNumberGen0);
        randbuf[i + 4] = readRegister(RandomNumberGen1);
        randbuf[i + 8] = readRegister(RandomNumberGen2);
        randbuf[i + 12] = readRegister(RandomNumberGen3);
    }

    setStandby(STDBY_RC);
}

// requestModuleActive manages a tcxo controlled by the MCU
static void requestModuleActive(bit_t state) {
    ostime_t const ticks = lmic_hal_setModuleActive(state);

    if (ticks) {
        lmic_hal_waitUntil(os_getTime() + ticks);
    }
}

//! \brief Initialize radio at system startup.
//!
//! \details This procedure is called during initialization by the `os_init()`
//! routine. It does a hardware reset of the radio, checks the version and confirms
//! that we're operating a suitable chip, and gets a random seed from wideband
//! noise rssi. It then puts the radio to sleep.
//!
//! \result True if successful, false if it doesn't look like the right radio is attached.
//!
//! \pre
//! Preconditions must be observed, or you'll get hangs during initialization.
//!
//! - The `lmic_hal_pin_..()` functions must be ready for use.
//! - The `lmic_hal_waitUntl()` function must be ready for use. This may mean that interrupts
//!   are enabled.
//! - The `lmic_hal_spi_..()` functions must be ready for use.
//!
//! Generally, all these are satisfied by a call to `lmic_hal_init_with_pinmap()`.
//!
int radio_init(void) {
    requestModuleActive(1);

    // manually reset radio
    lmic_hal_pin_rst(0); // drive RST pin low
    lmic_hal_waitUntil(os_getTime()+ms2osticks(1)); // wait >100us
    lmic_hal_pin_rst(2); // configure RST pin floating!
    lmic_hal_waitUntil(os_getTime()+ms2osticks(5)); // wait 5ms
    while(lmic_hal_radio_spi_is_busy()); // wait for busy pin to go low

    // Check default LoRa sync word to verify the reset was successful
    u1_t syncWordMSB = readRegister(LoRaSyncWordMSB);
    if(syncWordMSB != 0x14) {
        return 0;
    }

    // seed randomness via hardware RNG
    randomNumber(randbuf);
    randomNumber(randseed);
    randbuf[0] = 16; // set initial index
    randround = 0;

    // Sleep needs to be entered from standby_RC mode
    if ((getStatus() | SX126x_GETSTATUS_CHIPMODE_MASK) != SX126x_CHIPMODE_STDBY_RC) {
        setStandby(STDBY_RC);
    }
    setSleep(0);
    return 1;
}

// return next random byte derived from seed buffer
// (buf[0] holds index of next byte to be returned)
u1_t radio_rand1(void) {
    u1_t i = randbuf[0];
    ASSERT( i != 0 );
    if(i == 16) {
        os_wlsbf4(randbuf, randround);
        ++randround;
        LMIC_SecureElement_aes128Encrypt(randseed, randbuf, randbuf); // encrypt seed with any key
        i = 0;
    }
    u1_t v = randbuf[i++];
    randbuf[0] = i;
    return v;
}

// This function does not appear to be in use but is declared in the oslmic.h header file
u1_t radio_rssi(void) {
    u1_t buf;
    u1_t nop = SX126X_NOP;
    lmic_hal_spi_read_sx126x(GetRssiInst, &nop, 1, &buf, 1);

    // Problem: Original SX127x returns raw contents of register, but:
    // For SX1276, RSSI = (freq < 525MHz) ? -157 + buf : -164 + buf
    // For SX126x, RSSI = -buf/2
    // The return value has been mapped across to be consistent with what SX127x would return
    if (LMIC.freq > 525000000) {
        buf = 164 - buf / 2;
    } else {
        buf = 157 - buf / 2;
    }

    return buf;
}

/// \brief get the current RSSI on the current channel.
///
/// monitor rssi for specified number of ostime_t ticks, and return statistics
/// This puts the radio into RX continuous mode, waits long enough for the
/// oscillators to start and the PLL to lock, and then measures for the specified
/// period of time.  The radio is then returned to idle.
///
/// RSSI returned is expressed in units of dB, and is offset according to the
/// current radio setting per section 5.5.5 of Semtech 1276 datasheet.
///
/// \param nTicks How long to monitor
/// \param pRssi pointer to structure to fill in with RSSI data.
///
void radio_monitor_rssi(ostime_t nTicks, oslmic_radio_rssi_t *pRssi) {
    u1_t rssiMax, rssiMin;
    u2_t rssiSum;
    u2_t rssiN;

    ostime_t tBegin;
    u1_t notDone;

    rxlora(RXMODE_SCAN);

    // zero the results
    rssiMax = 255;
    rssiMin = 0;
    rssiSum = 0;
    rssiN = 0;

    // wait for PLLs
    lmic_hal_waitUntil(os_getTime() + us2osticks(500));

    // scan for the desired time.
    tBegin = os_getTime();
    rssiMax = 0;

    /* Per bug report from tanupoo, it's critical that interrupts be enabled
     * in the loop below so that `os_getTime()` always advances.
     */
    do {
        ostime_t now;

        u1_t nop = SX126X_NOP;
        u1_t rssiNow;
        lmic_hal_spi_read_sx126x(GetRssiInst, &nop, 1, &rssiNow, 1);
        if (rssiMax < rssiNow)
                rssiMax = rssiNow;
        if (rssiNow < rssiMin)
                rssiMin = rssiNow;
        rssiSum += rssiNow;
        ++rssiN;
        now = os_getTime();
        notDone = now - (tBegin + nTicks) < 0;
    } while (notDone);

    // put radio back to sleep. Sleep needs to be entered from standby_RC mode
    if ((getStatus() | SX126x_GETSTATUS_CHIPMODE_MASK) != SX126x_CHIPMODE_STDBY_RC) {
        setStandby(STDBY_RC);
    }
    setSleep(0);

    // compute the results
    pRssi->max_rssi = (s2_t) (-rssiMax / 2);
    pRssi->min_rssi = (s2_t) (-rssiMin / 2);
    pRssi->mean_rssi = (s2_t) (-((rssiSum + (rssiN >> 1)) / rssiN) / 2);
    pRssi->n_rssi = rssiN;
}

static CONST_TABLE(u2_t, LORA_RXDONE_FIXUP)[] = {
    [FSK]  =     us2osticks(0), // (   0 ticks)
    [SF7]  =     us2osticks(0), // (   0 ticks)
    [SF8]  =  us2osticks(1648), // (  54 ticks)
    [SF9]  =  us2osticks(3265), // ( 107 ticks)
    [SF10] =  us2osticks(7049), // ( 231 ticks)
    [SF11] = us2osticks(13641), // ( 447 ticks)
    [SF12] = us2osticks(31189), // (1022 ticks)
};

// called by hal ext IRQ handler
// (radio goes to stanby mode after tx/rx operations)
void radio_irq_handler(u1_t dio) {
    radio_irq_handler_v2(dio, os_getTime());
}

void radio_irq_handler_v2(u1_t dio, ostime_t now) {
    LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": IRQ handler, dio=%d\n", os_getTime(), dio);
    LMIC_API_PARAMETER(dio);

#if CFG_TxContinuousMode
    // TxContinuosMode NOT IMPLEMENTED for this SX126x driver.
    ASSERT(0);
#else /* ! CFG_TxContinuousMode */

#if LMIC_DEBUG_LEVEL > 0
    ostime_t const entry = now;
#endif

    u2_t rawFlags = getIrqStatus();

    // Map SX126x IRQ registers to 8 bit for consistency with legacy SX127x LMIC struct
    // SX127x 0 CadDetected, 1 FhssChangeChannel, 2 CadDone, 3 TxDone, 4 ValidHeader, 5 PayloadCrcError, 6 RxDone, 7 RxTimeout
    // SX126x 8 CadDetected, 14 LrFhssHop,        7 CadDone, 0 TxDone, 4 ValidHeader, 6 CrcErr,          1 RxDone, 9 Timeout
    u1_t flags = 0;
    flags |= (rawFlags & 0x0100) >> 8;
    flags |= (rawFlags & 0x4000) >> 13;
    flags |= (rawFlags & 0x0080) >> 5;
    flags |= (rawFlags & 0x0001) << 3;
    flags |= (rawFlags & 0x0010);
    flags |= (rawFlags & 0x0040) >> 1;
    flags |= (rawFlags & 0x0002) << 5;
    flags |= (rawFlags & 0x0200) >> 2;

    LMIC.saveIrqFlags = flags;

    if (getPacketType() == PACKET_TYPE_LORA) { // LORA modem
        LMICOS_logEventUint32("radio_irq_handler_v2: LoRa", flags);
        LMIC_X_DEBUG_PRINTF("IRQ=%02x\n", flags);
        LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": IRQ rawFlags=%04X\n", os_getTime(), rawFlags);
        LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": IRQ flags=%02X\n", os_getTime(), flags);
        if (flags & IRQ_LORA_TXDONE_MASK) {
            // save exact tx time
            LMIC.txend = now - us2osticks(43); // TXDONE FIXUP
        } else if (flags & IRQ_LORA_RXDONE_MASK) {
            // save exact rx time
            if (getBw(LMIC.rps) == BW125) {
                now -= TABLE_GET_U2(LORA_RXDONE_FIXUP, getSf(LMIC.rps));
            }
            LMIC.rxtime = now;
            LMIC.radio.rxtime = now;
            // read the PDU and inform the MAC that we received something
            u1_t rxBufferStatusRaw[SX126X_RXBUFFERSTATUS_LEN];
            u1_t packetStatusRaw[SX126X_PACKETSTATUS_LEN];

            getRxBufferStatus(rxBufferStatusRaw);
            LMIC.dataLen = rxBufferStatusRaw[0];
            LMIC.radio.dataLen = LMIC.dataLen;
            // now read the FIFO - use pFrame if set (for Class C), otherwise LMIC.frame
            u1_t *pFrame = (LMIC.radio.pFrame != NULL) ? LMIC.radio.pFrame : LMIC.frame;
            readBuffer(rxBufferStatusRaw[1], pFrame, LMIC.dataLen);
            // read rx quality parameters
            getPacketStatus(packetStatusRaw);
            LMIC.snr  = packetStatusRaw[1]; // SNR [dB] * 4
            u1_t const rRssi = packetStatusRaw[0]; // - RSSI [dB] * 2
            s2_t rssi = -rRssi / 2;

            LMIC_X_DEBUG_PRINTF("RX snr=%u rssi=%d\n", LMIC.snr/4, rssi);
            // ugh compatibility requires a biased range. RSSI
            LMIC.rssi = (s1_t) (RSSI_OFF + (rssi < -196 ? -196 : rssi > 63 ? 63 : rssi)); // RSSI [dBm] (-196...+63)
            LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": RXpacket, len=%d, offset=%d\n", os_getTime(), rxBufferStatusRaw[0], rxBufferStatusRaw[1]);
            LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": RXpacket, rssi=%d, snr=%d\n", os_getTime(), rssi, LMIC.snr / 4);
        } else if (flags & IRQ_LORA_RXTOUT_MASK) {
            // indicate timeout
            LMIC.dataLen = 0;
#if LMIC_DEBUG_LEVEL > 0
            ostime_t now2 = os_getTime();
            LMIC_DEBUG_PRINTF("rxtimeout: entry: %"LMIC_PRId_ostime_t" rxtime: %"LMIC_PRId_ostime_t" entry-rxtime: %"LMIC_PRId_ostime_t" now-entry: %"LMIC_PRId_ostime_t" rxtime-txend: %"LMIC_PRId_ostime_t"\n", entry,
                LMIC.rxtime, entry - LMIC.rxtime, now2 - entry, LMIC.rxtime-LMIC.txend);
#endif
        }

    } else { // FSK modem
        LMICOS_logEventUint32("radio_irq_handler_v2: LoRa", flags);
        LMIC_X_DEBUG_PRINTF("IRQ=%02x\n", flags);
        LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": IRQ rawFlags=%04X\n", os_getTime(), rawFlags);
        LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": IRQ flags=%02X\n", os_getTime(), flags);
        if (flags & IRQ_LORA_TXDONE_MASK) {
            // save exact tx time
            LMIC.txend = now;
        } else if (flags & IRQ_LORA_RXDONE_MASK) {
            // save exact rx time
            LMIC.rxtime = now;
            LMIC.radio.rxtime = now;
            // read the PDU and inform the MAC that we received something
            u1_t rxBufferStatusRaw[SX126X_RXBUFFERSTATUS_LEN];
            u1_t packetStatusRaw[SX126X_PACKETSTATUS_LEN];
            getRxBufferStatus(rxBufferStatusRaw);
            LMIC.dataLen = rxBufferStatusRaw[0];
            LMIC.radio.dataLen = LMIC.dataLen;
            // now read the FIFO - use pFrame if set (for Class C), otherwise LMIC.frame
            u1_t *pFrame = (LMIC.radio.pFrame != NULL) ? LMIC.radio.pFrame : LMIC.frame;
            readBuffer(rxBufferStatusRaw[1], pFrame, LMIC.dataLen);
            // read rx quality parameters
            LMIC.snr  = 0;              // SX126x doesn't give SNR for FSK.
            u1_t const rRssi = packetStatusRaw[2]; // - RSSI [dB] * 2
            s2_t rssi = -rRssi / 2;
            LMIC.rssi = (s1_t) (RSSI_OFF + (rssi < -196 ? -196 : rssi > 63 ? 63 : rssi)); // RSSI [dBm] (-196...+63)
        } else if (flags & IRQ_LORA_RXTOUT_MASK) {
            // indicate timeout
            LMIC.dataLen = 0;
        } else {
            // ASSERT(0);
            // we're not sure why we're here... treat as timeout.
            LMIC.dataLen = 0;
        }
    }

    // clear radio IRQ flags
    u2_t clearAllIrq = 0x03FF;
    clearIrqStatus(clearAllIrq);

    // go from standby to sleep
    // Sleep needs to be entered from standby_RC mode
    if ((getStatus() | SX126x_GETSTATUS_CHIPMODE_MASK) != SX126x_CHIPMODE_STDBY_RC) {
        setStandby(STDBY_RC);
    }
    setSleep(0);

    // mark radio as done
    LMIC.radio.state = LMIC_RADIO_EV_NONE;

    // run os job (use pRadioDoneJob if set, otherwise use legacy LMIC.osjob)
    osjob_t *pJob = LMIC.radio.pRadioDoneJob;
    if (pJob != NULL) {
        LMIC.radio.pRadioDoneJob = NULL;
        os_setCallback(pJob, pJob->func);
    } else {
        os_setCallback(&LMIC.osjob, LMIC.osjob.func);
    }
#endif /* ! CFG_TxContinuousMode */
}

///
/// \brief Check if transmit is active
///
static inline bit_t os_radio_isTxActive(lmic_radio_state_t state) {
    return (state & LMIC_RADIO_EV_TXSTART) != 0 &&
           (state & LMIC_RADIO_EV_TXDONE) == 0;
}

///
/// \brief Check if receive is active
///
static inline bit_t os_radio_isRxActive(lmic_radio_state_t state) {
    return (state & LMIC_RADIO_EV_RXSTART) != 0 &&
           (state & LMIC_RADIO_EV_RXDONE) == 0;
}

///
/// \brief Check if any radio operation is active
///
static inline bit_t os_radio_isStateActive(lmic_radio_state_t state) {
    return os_radio_isTxActive(state) || os_radio_isRxActive(state);
}

///
/// \brief Reset the radio and clear state
///
static void os_radio_reset(void) {
    // Sleep needs to be entered from standby_RC mode
    if ((getStatus() | SX126x_GETSTATUS_CHIPMODE_MASK) != SX126x_CHIPMODE_STDBY_RC) {
        setStandby(STDBY_RC);
    }
    setSleep(0x00);
    LMIC.radio.state = LMIC_RADIO_EV_NONE;
    LMIC.radio.pRadioDoneJob = NULL;
}

void os_radio(u1_t mode) {
    // populate LMIC.radio fields from LMIC for compatibility
    LMIC.radio.freq = LMIC.freq;
    LMIC.radio.pFrame = LMIC.frame;
    LMIC.radio.rxtime = LMIC.rxtime;
    LMIC.radio.rps = LMIC.rps;
    LMIC.radio.rxsyms = LMIC.rxsyms;
    LMIC.radio.dataLen = LMIC.dataLen;
    // Note: LMIC.radio.txpow is already set by lmic.c before calling os_radio()
    LMIC.radio.flags = 0;
    if (LMIC.noRXIQinversion) {
        LMIC.radio.flags |= LMIC_RADIO_FLAGS_NO_RX_IQ_INVERSION;
    }

    os_radio_v2(mode, &LMIC.osjob);
}

///
/// \brief Initiate a radio operation.
///
/// \param mode [in]    Selects the operation to be performed.
/// \param pJob [inout]  Job to be scheduled when operation completes.
///
/// \details
///     The requested radio operation is initiated. Some operations complete
///     immediately; others require hardware to do work, and don't complete until
///     an interrupt occurs. In that case, `pJob` is scheduled.
///     Because the
///     interrupt may occur right away, it's important that the caller initialize
///     `pJob` before calling this routine.
///
///     - `RADIO_RST` causes the radio to be put to sleep. No interrupt follows;
///     when control returns, the radio is ready for the next operation. `pJob`
///     is not scheduled. If an operation was pending, it is canceled.
///
///     - `RADIO_TX` and `RADIO_TX_AT` launch the transmission of a frame. An interrupt will
///     occur, which will cause `pJob` to be scheduled with its current
///     function.
///
///     - `RADIO_RX`, `RADIO_RX_ON` and `RADIO_RX_ON_C` launch either timed or
///     untimed receives. An interrupt will occur when a packet is received or
///     the receive times out, which will cause `pJob` to be scheduled with
///     its current function. `RADIO_RX_ON_C` is used to schedule a request that
///     might be redundant, for class C support.
///
void os_radio_v2(u1_t mode, osjob_t *pJob) {
    // handle redundant class-C requests.
    if (mode == RADIO_RXON_C &&
        os_radio_isRxActive(LMIC.radio.state) &&
        pJob == LMIC.radio.pRadioDoneJob) {
        LMICOS_logEventUint32("redundant class C RX", LMIC.radio.state);
        return;
    }

    // handle requests while radio is active: cancel.
    if (mode != RADIO_RST) {
        if (LMIC.radio.state != LMIC_RADIO_EV_NONE) {
            LMICOS_logEventUint32("request while radio active", LMIC.radio.state);
            // recurse and kill the pending activity
            os_radio_reset();
        }
        // record job
        LMIC.radio.pRadioDoneJob = pJob;
    }

    switch (mode) {
      case RADIO_RST:
        os_radio_reset();
        break;

      case RADIO_TX:
        // transmit frame now
        LMIC.txend = 0;
        LMIC.radio.state = LMIC_RADIO_EV_TXSTART;
        starttx(); // buf=LMIC.frame, len=LMIC.dataLen
        break;

      case RADIO_TX_AT:
        // make sure transmit time is non-zero to force
        // scheduling at a specific time.
        if (LMIC.txend == 0)
            LMIC.txend = 1;
        LMIC.radio.state = LMIC_RADIO_EV_TXSTART;
        starttx();
        break;

      case RADIO_RX:
        // receive frame now (exactly at rxtime)
        LMIC.radio.state = LMIC_RADIO_EV_RXSTART;
        startrx(RXMODE_SINGLE); // buf=LMIC.frame, time=LMIC.rxtime, timeout=LMIC.rxsyms
        break;

      case RADIO_RXON:
      case RADIO_RXON_C:
        // start scanning for beacon now (or class C continuous RX)
        LMIC.radio.state = LMIC_RADIO_EV_RXSTART;
        startrx(RXMODE_SCAN); // buf=LMIC.frame
        break;
    }
}

ostime_t os_getRadioRxRampup(void) {
    return RX_RAMPUP_DEFAULT + us2osticks(12480); // SX126x is 780 ticks slower than SX127x to wake from sleep @ 240MHz
}
#endif // defined(CFG_sx1261_radio) || defined(CFG_sx1262_radio)
