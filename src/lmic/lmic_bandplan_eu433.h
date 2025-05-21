/*
* Copyright (c) 2014-2016 IBM Corporation.
* Copyright (c) 2017, 2019-2021 MCCI Corporation.
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

#ifndef _lmic_eu433_h_
# define _lmic_eu433_h_

#ifndef _lmic_eu_like_h_
# include "lmic_eu_like.h"
#endif

// return maximum frame length (including PHY header) for this data rate (eu433); 0 --> not valid dr.
uint8_t LMICeu433_maxFrameLen(uint8_t dr);
// return maximum frame length (including PHY header) for this data rate; 0 --> not valid dr.
#define LMICbandplan_maxFrameLen(dr) LMICeu433_maxFrameLen(dr)

int8_t LMICeu433_pow2dBm(uint8_t mcmd_ladr_p1);
#define pow2dBm(mcmd_ladr_p1)   LMICeu433_pow2dBm(mcmd_ladr_p1)

// Times for half symbol per DR
// Per DR table to minimize rounding errors
ostime_t LMICeu433_dr2hsym(uint8_t dr);
#define dr2hsym(dr) LMICeu433_dr2hsym(dr)

static inline int
LMICeu433_isValidBeacon1(const uint8_t *d) {
    return d[OFF_BCN_CRC1] != (u1_t)os_crc16(d, OFF_BCN_CRC1);
}

#undef LMICbandplan_isValidBeacon1
#define LMICbandplan_isValidBeacon1(pFrame) LMICeu433_isValidBeacon1(pFrame)

// override default for LMICbandplan_isFSK()
#undef LMICbandplan_isFSK
#define LMICbandplan_isFSK()    (/* RX datarate */LMIC.dndr == EU433_DR_FSK)

#define LMICbandplan_getInitialDrJoin() (EU433_DR_SF7)

void LMICeu433_setBcnRxParams(void);
#define LMICbandplan_setBcnRxParams()   LMICeu433_setBcnRxParams()

u4_t LMICeu433_convFreq(xref2cu1_t ptr);
#define LMICbandplan_convFreq(ptr)      LMICeu433_convFreq(ptr)

void LMICeu433_initJoinLoop(void);
#define LMICbandplan_initJoinLoop()     LMICeu433_initJoinLoop()

ostime_t LMICeu433_nextTx(ostime_t now);
#define LMICbandplan_nextTx(now)        LMICeu433_nextTx(now)

ostime_t LMICeu433_nextJoinState(void);
#define LMICbandplan_nextJoinState()    LMICeu433_nextJoinState()

void LMICeu433_initDefaultChannels(bit_t join);
#define LMICbandplan_initDefaultChannels(join)  LMICeu433_initDefaultChannels(join)

#undef LMICbandplan_nextJoinTime
ostime_t LMICeu433_nextJoinTime(ostime_t now);
#define LMICbandplan_nextJoinTime(now)     LMICeu433_nextJoinTime(now)

void LMICeu433_setRx1Params(void);
#define LMICbandplan_setRx1Params()     LMICeu433_setRx1Params()

#undef LMICbandplan_validDR
bit_t LMICeu433_validDR(dr_t dr);
#define LMICbandplan_validDR(dr)        LMICeu433_validDR(dr)

#endif // _lmic_eu433_h_
