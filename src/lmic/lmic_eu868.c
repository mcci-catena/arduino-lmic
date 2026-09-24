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

#define LMIC_DR_LEGACY 0

#include "lmic_bandplan.h"

#if defined(CFG_eu868)
// ================================================================================
//
// BEG: EU868 related stuff
//

CONST_TABLE(u1_t, _DR2RPS_CRC)[] = {
        ILLEGAL_RPS,
        (u1_t)MAKERPS(SF12, BW125, CR_4_5, 0, 0),
        (u1_t)MAKERPS(SF11, BW125, CR_4_5, 0, 0),
        (u1_t)MAKERPS(SF10, BW125, CR_4_5, 0, 0),
        (u1_t)MAKERPS(SF9,  BW125, CR_4_5, 0, 0),
        (u1_t)MAKERPS(SF8,  BW125, CR_4_5, 0, 0),
        (u1_t)MAKERPS(SF7,  BW125, CR_4_5, 0, 0),
        (u1_t)MAKERPS(SF7,  BW250, CR_4_5, 0, 0),
        (u1_t)MAKERPS(FSK,  BW125, CR_4_5, 0, 0),
        ILLEGAL_RPS
};

bit_t
LMICeu868_validDR(dr_t dr) {
        // use subtract here to avoid overflow
        if (dr >= LENOF_TABLE(_DR2RPS_CRC) - 2)
                return 0;
        return TABLE_GET_U1(_DR2RPS_CRC, dr+1)!=ILLEGAL_RPS;
}

static CONST_TABLE(u1_t, maxFrameLens)[] = {
        59+5, 59+5, 59+5, 123+5, 250+5, 250+5, 250+5, 250+5
};

uint8_t LMICeu868_maxFrameLen(uint8_t dr) {
        if (dr < LENOF_TABLE(maxFrameLens))
                return TABLE_GET_U1(maxFrameLens, dr);
        else
                return 0;
}

static CONST_TABLE(s1_t, TXPOWLEVELS)[] = {
        16, 14, 12, 10, 8, 6, 4, 2
};

int8_t LMICeu868_pow2dBm(uint8_t mcmd_ladr_p1) {
        uint8_t const pindex = (mcmd_ladr_p1&MCMD_LinkADRReq_POW_MASK)>>MCMD_LinkADRReq_POW_SHIFT;
        if (pindex < LENOF_TABLE(TXPOWLEVELS)) {
                return TABLE_GET_S1(TXPOWLEVELS, pindex);
        } else {
                return -128;
        }
}

// only used in this module, but used by variant macro dr2hsym().
static CONST_TABLE(ostime_t, DR2HSYM_osticks)[] = {
        us2osticksRound(128 << 7),  // DR_SF12
        us2osticksRound(128 << 6),  // DR_SF11
        us2osticksRound(128 << 5),  // DR_SF10
        us2osticksRound(128 << 4),  // DR_SF9
        us2osticksRound(128 << 3),  // DR_SF8
        us2osticksRound(128 << 2),  // DR_SF7
        us2osticksRound(128 << 1),  // DR_SF7B
        us2osticksRound(80)         // FSK -- time for 1/2 byte (unused by LMIC)
};

ostime_t LMICeu868_dr2hsym(uint8_t dr) {
        return TABLE_GET_OSTIME(DR2HSYM_osticks, dr);
}


enum { NUM_DEFAULT_CHANNELS = 3 };
static CONST_TABLE(u4_t, iniChannelFreq)[6] = {
        // Join frequencies and duty cycle limit (0.1%)
        EU868_F1 | BAND_MILLI, EU868_F2 | BAND_MILLI, EU868_F3 | BAND_MILLI,
        // Default operational frequencies and duty cycle limit (1%)
        EU868_F1 | BAND_CENTI, EU868_F2 | BAND_CENTI, EU868_F3 | BAND_CENTI,
};

void LMICeu868_initDefaultChannels(bit_t join) {
        os_clearMem(&LMIC.channelFreq, sizeof(LMIC.channelFreq));
#if !defined(DISABLE_MCMD_DlChannelReq)
        os_clearMem(&LMIC.channelDlFreq, sizeof(LMIC.channelDlFreq));
#endif // !DISABLE_MCMD_DlChannelReq
        os_clearMem(&LMIC.channelDrMap, sizeof(LMIC.channelDrMap));
        os_clearMem(&LMIC.bands, sizeof(LMIC.bands));

        LMIC.channelMap = (1 << NUM_DEFAULT_CHANNELS) - 1;
        u1_t su = join ? 0 : NUM_DEFAULT_CHANNELS;
        for (u1_t fu = 0; fu<NUM_DEFAULT_CHANNELS; fu++, su++) {
                LMIC.channelFreq[fu] = TABLE_GET_U4(iniChannelFreq, su);
                // TODO(tmm@mcci.com): don't use EU DR directly, use something from the LMIC context or a static const
                LMIC.channelDrMap[fu] = DR_RANGE_MAP(EU868_DR_SF12, EU868_DR_SF7);
        }

        (void) LMIC_setupBand(BAND_MILLI, 14 /* dBm */, 1000 /* 0.1% */);
        (void) LMIC_setupBand(BAND_CENTI, 14 /* dBm */,  100 /* 1% */);
        (void) LMIC_setupBand(BAND_DECI,  27 /* dBm */,   10 /* 10% */);
}

bit_t LMIC_setupBand(u1_t bandidx, s1_t txpow, u2_t txcap) {
        if (bandidx > BAND_AUX) return 0;
        //band_t* b = &LMIC.bands[bandidx];
        xref2band_t b = &LMIC.bands[bandidx];
        b->txpow = txpow;
        b->txcap = txcap;
        b->avail = os_getTime();
        b->lastchnl = os_getRndU1() % MAX_CHANNELS;
        return 1;
}

// this table is from highest to lowest
static CONST_TABLE(u4_t, bandAssignments)[] = {
  870000000 /* .. and above */    | BAND_MILLI,
  869700000 /* .. 869700000 */    | BAND_CENTI,
  869650000 /* .. 869700000 */    | BAND_MILLI,
  869400000 /* .. 869650000 */    | BAND_DECI,
  868600000 /* .. 869640000 */    | BAND_MILLI,
  865000000 /* .. 868400000 */    | BAND_CENTI,
};

///
/// \brief query number of default channels.
///
u1_t LMIC_queryNumDefaultChannels() {
        return NUM_DEFAULT_CHANNELS;
}

///
/// \brief LMIC_setupChannel for EU 868
///
/// \note according to LoRaWAN 1.0.3 section 5.6, "the acceptable range
///     for **ChIndex** is N to 16", where N is our \c NUM_DEFAULT_CHANNELS.
///     This routine is used internally for MAC commands, so we enforce
///     this for the extenal API as well.
///
bit_t LMIC_setupChannel(u1_t chidx, u4_t freq, u2_t drmap, s1_t band) {
        // zero the band bits in freq, just in case.
        freq &= ~3;

        if (chidx < NUM_DEFAULT_CHANNELS) {
                // can't do anything to a default channel.
                return 0;
        }
        bit_t fEnable = (freq != 0);
        if (chidx >= MAX_CHANNELS)
                return 0;

        if (band == -1) {
                for (u1_t i = 0; i < LENOF_TABLE(bandAssignments); ++i) {
                        const u4_t thisFreqBand = TABLE_GET_U4(bandAssignments, i);
                        const u4_t thisFreq = thisFreqBand & ~3;
                        if (freq >= thisFreq) {
                                band = ((u1_t)thisFreqBand & 3);
                                break;
                        }
                }

                // if we didn't identify a frequency, it's millis.
                if (band == -1) {
                        band = BAND_MILLI;
                }
        }

        if ((u1_t)band > BAND_AUX)
                return 0;

        freq |= band;

        LMIC.channelFreq[chidx] = freq;
        LMIC.channelDrMap[chidx] = drmap == 0 ? DR_RANGE_MAP(EU868_DR_SF12, EU868_DR_SF7) : drmap;
        if (fEnable)
                LMIC.channelMap |= 1 << chidx;  // enabled right away
        else
                LMIC.channelMap &= ~(1 << chidx);
        return 1;
}



u4_t LMICeu868_convFreq(xref2cu1_t ptr) {
        u4_t freq = (os_rlsbf4(ptr - 1) >> 8) * 100;
        if (freq < EU868_FREQ_MIN || freq > EU868_FREQ_MAX)
                freq = 0;
        return freq;
}

ostime_t LMICeu868_nextJoinTime(ostime_t time) {
        // is the avail time in the future?
        if ((s4_t) (time - LMIC.bands[BAND_MILLI].avail) < 0)
                // yes: then wait until then.
                time = LMIC.bands[BAND_MILLI].avail;

        return time;
}

///
/// \brief change the TX channel given the desired tx time.
///
/// \param [in] now is the time at which we want to transmit. In fact, it's always
///     the current time.
///
/// \returns the actual time at which we can transmit. \c LMIC.txChnl is set to the
///     selected channel.
///
/// \details
///     We scan all the channels, creating a mask of all enabled channels that are
///     feasible at the earliest possible time. We then randomly choose one from
///     that, updating the shuffle mask.
///
///     One sublety is that we have to cope with an artifact of the shuffler.
///     It will zero out bits for candidates that are real candidates, but
///     not in the time window, and not consider them as early as it should.
///     So we keep a mask of all feasible channels, and make sure that they
///     remain set in the shuffle mask if appropriate.
///
ostime_t LMICeu868_nextTx(ostime_t now) {
        ostime_t mintime = now + /*8h*/sec2osticks(28800);
        u2_t availMap;
        u2_t feasibleMap;
        u1_t bandMap;

        // set mintime to the earliest time of all enabled channels
        // (can't just look at bands); and for a given channel, we
        // can't tell if we're ready till we've checked all possible
        // avail times.
        bandMap = 0;
        for (u1_t chnl = 0; chnl < MAX_CHANNELS; ++chnl) {
                u2_t chnlBit = 1 << chnl;

                // none at any higher numbers?
                if (LMIC.channelMap < chnlBit)
                        break;

                // not enabled?
                if ((LMIC.channelMap & chnlBit) == 0)
                        continue;

                // not feasible?
                if ((LMIC.channelDrMap[chnl] & (1 << (LMIC.datarate & 0xF))) == 0)
                        continue;

                u1_t const band = LMIC.channelFreq[chnl] & 0x3;
                u1_t const thisBandBit = 1 << band;
                // already considered?
                if ((bandMap & thisBandBit) != 0)
                        continue;

                // consider this band.
                bandMap |= thisBandBit;

                // enabled, not considered, feasible: adjust the min time.
                if ((s4_t)(mintime - LMIC.bands[band].avail) > 0)
                        mintime = LMIC.bands[band].avail;
        }

        // make a mask of candidates available for use
        availMap = 0;
        feasibleMap = 0;
        for (u1_t chnl = 0; chnl < MAX_CHANNELS; ++chnl) {
                u2_t chnlBit = 1 << chnl;

                // none at any higher numbers?
                if (LMIC.channelMap < chnlBit)
                        break;

                // not enabled?
                if ((LMIC.channelMap & chnlBit) == 0)
                        continue;

                // not feasible?
                if ((LMIC.channelDrMap[chnl] & (1 << (LMIC.datarate & 0xF))) == 0)
                        continue;

                // This channel is feasible. But might not be available.
                feasibleMap |= chnlBit;

                // not available yet?
                u1_t const band = LMIC.channelFreq[chnl] & 0x3;
                if ((s4_t)(LMIC.bands[band].avail - mintime) > 0)
                        continue;

                // ok: this is a candidate.
                availMap |= chnlBit;
        }

        // find the next available chennel.
        u2_t saveShuffleMap = LMIC.channelShuffleMap;
        int candidateCh = LMIC_findNextChannel(&LMIC.channelShuffleMap, &availMap, 1, LMIC.txChnl == 0xFF ? -1 : LMIC.txChnl);

        // restore bits in the shuffleMap that were on, but might have reset
        // if availMap was used to refresh shuffleMap. These are channels that
        // are feasble but not yet candidates due to band saturation
        LMIC.channelShuffleMap |= saveShuffleMap & feasibleMap & ~availMap;

        if (candidateCh >= 0) {
                // update the channel; otherwise we'll just use the
                // most recent one.
                LMIC.txChnl = candidateCh;
        }
        return mintime;
}


#if !defined(DISABLE_BEACONS)
void LMICeu868_setBcnRxParams(void) {
        LMIC.dataLen = 0;
        LMIC.freq = FREQ_BCN;
        LMIC.rps = setIh(setNocrc(dndr2rps((dr_t)DR_BCN), 1), LEN_BCN);
}
#endif // !DISABLE_BEACONS

#if !defined(DISABLE_JOIN)
ostime_t LMICeu868_nextJoinState(void) {
        return LMICeulike_nextJoinState(NUM_DEFAULT_CHANNELS);
}
#endif // !DISABLE_JOIN

// set the Rx1 dndr, rps.
void LMICeu868_setRx1Params(void) {
    u1_t const txdr = LMIC.dndr;
    s1_t drOffset;
    s1_t candidateDr;

    LMICeulike_setRx1Freq();

    if ( LMIC.rx1DrOffset <= 5)
        drOffset = (s1_t) LMIC.rx1DrOffset;
    else
        // make a reasonable assumption for unspecified value.
        drOffset = 5;

    candidateDr = (s1_t) txdr - drOffset;
    if (candidateDr < LORAWAN_DR0)
            candidateDr = 0;
    else if (candidateDr > LORAWAN_DR7)
            candidateDr = LORAWAN_DR7;

    LMIC.dndr = (u1_t) candidateDr;
    LMIC.rps = dndr2rps(LMIC.dndr);
}

void
LMICeu868_initJoinLoop(void) {
        LMICeulike_initJoinLoop(NUM_DEFAULT_CHANNELS, /* adr dBm */ EU868_TX_EIRP_MAX_DBM);
}

//
// END: EU868 related stuff
//
// ================================================================================
#endif