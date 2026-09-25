/*
* Copyright (c) 2014-2016 IBM Corporation.
* Copyright (c) 2017, 2019 MCCI Corporation.
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
#include "lmic_session_state.h"

#if CFG_LMIC_EU_like

bit_t LMIC_enableSubBand(u1_t band) {
        LMIC_API_PARAMETER(band);
        return 0;
}

bit_t LMIC_disableSubBand(u1_t band) {
        LMIC_API_PARAMETER(band);
        return 0;
}

bit_t LMIC_disableChannel(u1_t channel) {
        u2_t old_chmap = LMIC.channelMap;
        LMIC.channelFreq[channel] = 0;
        LMIC.channelDrMap[channel] = 0;
        LMIC.channelMap = old_chmap & ~(1 << channel);
        return LMIC.channelMap != old_chmap;
}

// this is a no-op provided for compatibilty
bit_t LMIC_enableChannel(u1_t channel) {
        LMIC_API_PARAMETER(channel);
        return 0;
}

// check whether a map operation will work.
// chpage is 0 or 6; 6 turns all on; 0 selects channels 0..15 via mask.
// The spec is unclear as to whether we should veto a channel mask that enables
// a channel that hasn't been configured; we veto it.
bit_t LMICeulike_canMapChannels(u1_t chpage, u2_t chmap) {
    switch (chpage) {
        case MCMD_LinkADRReq_ChMaskCntl_EULIKE_DIRECT:
            // we don't allow any channel to be turned on if its frequency is zero.
            for (u1_t chnl = 0; chnl<MAX_CHANNELS; chnl++) {
                    if ((chmap & (1 << chnl)) != 0 && (LMIC.channelFreq[chnl]&~3) == 0)
                            return 0; // fail - channel is not defined
            }
            return 1;

        case MCMD_LinkADRReq_ChMaskCntl_EULIKE_ALL_ON:
            return 1;

        default:
            return 0;
    }
}

// assumes that LMICeulike_canMapChannels passed. Return true if this would
// be a valid final configuration.
// chpage is 0 or 0x60; 0x60 turns all on; 0 selects channels 0..15 via mask.
// Assumes canMapChannels has already approved this change.
bit_t LMICeulike_mapChannels(u1_t chpage, u2_t chmap) {
    switch (chpage) {
        case MCMD_LinkADRReq_ChMaskCntl_EULIKE_DIRECT:
            LMIC.channelMap = chmap;
            break;

        case MCMD_LinkADRReq_ChMaskCntl_EULIKE_ALL_ON: {
            u2_t new_chmap = 0;
            for (u1_t chnl = 0; chnl<MAX_CHANNELS; chnl++) {
                    if ((LMIC.channelFreq[chnl]&~3) != 0) {
                        new_chmap |= (1 << chnl);
                    }
            }
            LMIC.channelMap = new_chmap;
            break;
        }

        default:
            // do nothing.
            break;
    }
    return LMIC.channelMap != 0;
}

bit_t LMICeulike_isDataRateFeasible(dr_t dr) {
    // if the region uses TxpParam, then someone
    // could have changed TxDwell, which makes some
    // otherwise-legal DRs infeasible.
#if LMIC_ENABLE_TxParamSetupReq
    if (LMICbandplan_maxFrameLen(dr) == 0) {
        return 0;
    }
#endif
        for (u1_t chnl = 0; chnl < MAX_CHANNELS; ++chnl) {
                if ((LMIC.channelMap & (1 << chnl)) != 0 &&  // channel enabled
                        (LMIC.channelDrMap[chnl] & (1 << dr)) != 0)
                                return 1;
        }
        return 0;
}

#if !defined(DISABLE_JOIN)
void LMICeulike_initJoinLoop(uint8_t nDefaultChannels, s1_t adrTxPow) {
#if CFG_TxContinuousMode
        LMIC.txChnl = 0
#else
        uint16_t enableMap = (1 << nDefaultChannels) - 1;
        LMIC.channelShuffleMap = enableMap;
        LMIC.txChnl = LMIC_findNextChannel(&LMIC.channelShuffleMap, &enableMap, 1, -1);
#endif
        LMIC.adrTxPow = adrTxPow;
        // TODO(tmm@mcci.com) don't use EU directly, use a table. That
        // will allow support for EU-style bandplans with similar code.
        LMICcore_setDrJoin(DRCHG_SET, LMICbandplan_getInitialDrJoin());
        LMICbandplan_initDefaultChannels(/* put into join mode */ 1);
        ASSERT((LMIC.opmode & OP_NEXTCHNL) == 0);
        LMIC.txend = os_getTime() + LMICcore_rndDelay(8);
}
#endif // DISABLE_JOIN

void LMICeulike_updateTx(ostime_t txbeg) {
        u4_t freq = LMIC.channelFreq[LMIC.txChnl];
        // Update global/band specific duty cycle stats
        ostime_t airtime = calcAirTime(LMIC.rps, LMIC.dataLen);
        // Update channel/global duty cycle stats
        xref2band_t band = &LMIC.bands[freq & 0x3];
        LMIC.freq = freq & ~(u4_t)3;
        LMIC.txpow = band->txpow;
        band->avail = txbeg + airtime * band->txcap;
        if (LMIC.globalDutyRate != 0)
                LMIC.globalDutyAvail = txbeg + (airtime << LMIC.globalDutyRate);
}

#if !defined(DISABLE_JOIN)
//
// TODO(tmm@mcci.com):
//
// The definition of this is a little strange. this seems to return a time, but
// in reality it returns 0 if the caller should continue scanning through
// channels, and 1 if the caller has scanned all channels on this session,
// and therefore should reset to the beginning.  The IBM 1.6 code is the
// same way, so apparently I just carried this across. We should declare
// as bool_t and change callers to use the result clearly as a flag.
//
ostime_t LMICeulike_nextJoinState(uint8_t nDefaultChannels) {
        u1_t failed = 0;
        u2_t enableMap = (1 << nDefaultChannels) - 1;

        // Try each default channel with same DR
        // If all fail try next lower datarate
        if (LMIC.channelShuffleMap == 0) {
                // Lower DR every nth try (having all default channels with same DR)
                //
                // TODO(tmm@mcci.com) add new DR_REGION_JOIN_MIN instead of LORAWAN_DR0;
                // then we can eliminate the LMIC_REGION_as923 below because we'll set
                // the failed flag here. This will cause the outer caller to take the
                // appropriate join path. Or add new LMICeulike_GetLowestJoinDR()
                //
// TODO(tmm@mcci.com) - see above; please remove regional dependency from this file.
#if CFG_region == LMIC_REGION_as923
                // in the join of AS923 v1.1 or older, only DR2 is used.
                // no need to change the DR.
                LMIC.datarate = AS923_DR_SF10;
                failed = 1;
#else
                if (LMIC.datarate == LORAWAN_DR0) {
                        failed = 1; // we have tried all DR - signal EV_JOIN_FAILED
                } else {
                        LMICcore_setDrJoin(DRCHG_NOJACC, decDR((dr_t)LMIC.datarate));
                }
#endif
        }

        // find new channel, avoiding repeats.
        int newCh = LMIC_findNextChannel(&LMIC.channelShuffleMap, &enableMap, 1, LMIC.txChnl);
        if (newCh >= 0)
                LMIC.txChnl = newCh;

        // Clear OP_NEXTCHNL because join state engine controls channel hopping
        LMIC.opmode &= ~OP_NEXTCHNL;

        // Move txend to randomize synchronized concurrent joins.
        // Duty cycle is based on txend.
        ostime_t const time = LMICbandplan_nextJoinTime(os_getTime());

        // TODO(tmm@mcci.com): change delay to (0:1) secs + a known t0, but randomized;
        // starting adding a bias after 1 hour, 25 hours, etc.; and limit the duty
        // cycle on power up. For testability, add a way to set the join start time
        // externally (a test API) so we can check this feature.
        // See https://github.com/mcci-catena/arduino-lmic/issues/2
        // Current code doesn't match LoRaWAN 1.0.2 requirements.

        LMIC.txend = time +
                (isTESTMODE()
                        // Avoid collision with JOIN ACCEPT @ SF12 being sent by GW (but we missed it)
                        ? DNW2_SAFETY_ZONE
                        // Otherwise: randomize join (street lamp case):
                        // SF12:255, SF11:127, .., SF7:8secs
			//
                        : DNW2_SAFETY_ZONE + LMICcore_rndDelay(255 >> LMIC.datarate));
        // 1 - triggers EV_JOIN_FAILED event
        return failed;
}
#endif // !DISABLE_JOIN

#if !defined(DISABLE_JOIN)
void LMICeulike_processJoinAcceptCFList(void) {
    if ( LMICbandplan_hasJoinCFlist() &&
         LMIC.frame[OFF_CFLIST + 15] == LORAWAN_JoinAccept_CFListType_FREQUENCIES) {
        u1_t dlen;
        u1_t nDefault = LMIC_queryNumDefaultChannels();

        dlen = OFF_CFLIST;
        for( u1_t chidx = nDefault; chidx < nDefault + 5; chidx++, dlen+=3 ) {
            u4_t freq = LMICbandplan_convFreq(&LMIC.frame[dlen]);
            if( freq ) {
                LMIC_setupChannel(chidx, freq, 0, -1);
#if LMIC_DEBUG_LEVEL > 1
                LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": Setup channel, idx=%d, freq=%"PRIu32"\n", os_getTime(), chidx, freq);
#endif
            }
        }
    }
}
#endif // !DISABLE_JOIN

void LMICeulike_saveAdrState(lmic_saved_adr_state_t *pStateBuffer) {
    os_copyMem(
            pStateBuffer->channelFreq,
            LMIC.channelFreq,
            sizeof(LMIC.channelFreq)
            );
    pStateBuffer->channelMap = LMIC.channelMap;
}

bit_t LMICeulike_compareAdrState(const lmic_saved_adr_state_t *pStateBuffer) {
        if (memcmp(pStateBuffer->channelFreq, LMIC.channelFreq, sizeof(LMIC.channelFreq)) != 0)
                return 1;
        return pStateBuffer->channelMap != LMIC.channelMap;
}

void LMICeulike_restoreAdrState(const lmic_saved_adr_state_t *pStateBuffer) {
    os_copyMem(
            LMIC.channelFreq,
            pStateBuffer->channelFreq,
            sizeof(LMIC.channelFreq)
            );
    LMIC.channelMap = pStateBuffer->channelMap;
}

void LMICeulike_setRx1Freq(void) {
#if !defined(DISABLE_MCMD_DlChannelReq)
        uint32_t dlFreq = LMIC.channelDlFreq[LMIC.txChnl];
        if (dlFreq != 0)
                LMIC.freq = dlFreq;
#endif // !DISABLE_MCMD_DlChannelReq
}

// Class A txDone handling for FSK.
void
LMICeulike_txDoneFSK(ostime_t delay, osjobcb_t func) {
        // one symbol == one bit at 50kHz == 20us.
        ostime_t const hsym = us2osticksRound(10);

        // start a little earlier.  PRERX_FSK is in bytes; one byte at 50 kHz == 160us
        delay -= LMICbandplan_PRERX_FSK * us2osticksRound(160);

        // set LMIC.rxtime and LMIC.rxsyms:
        LMIC.nextRxTime = LMIC.txend + LMICcore_adjustForDrift(delay, hsym, 8 * LMICbandplan_RXLEN_FSK);
        os_setTimedCallback(&LMIC.osjob, LMIC.nextRxTime - os_getRadioRxRampup(), func);
}

//
// Channel query
//

// the bandwidth implied by a channel's data-rate map, or "by data rate" if mixed.
static u1_t LMICeulike_bandwidthFromDrMap(u2_t drMap) {
        u1_t result = LMIC_CHANNEL_BW_BY_DATARATE;

        for (dr_t dr = 0; dr < 16; ++dr) {
                if ((drMap & (1u << dr)) == 0)
                        continue;

                rps_t const rps = updr2rps(dr);
                u1_t bw;

                if (getSf(rps) == FSK)
                        return LMIC_CHANNEL_BW_BY_DATARATE;

                switch (getBw(rps)) {
                case BW125: bw = LMIC_CHANNEL_BW_125kHz; break;
                case BW250: bw = LMIC_CHANNEL_BW_250kHz; break;
                case BW500: bw = LMIC_CHANNEL_BW_500kHz; break;
                default:    return LMIC_CHANNEL_BW_BY_DATARATE;
                }

                if (result == LMIC_CHANNEL_BW_BY_DATARATE)
                        result = bw;
                else if (result != bw)
                        return LMIC_CHANNEL_BW_BY_DATARATE;
        }
        return result;
}

u1_t LMIC_queryChannelCount(void) {
        return MAX_CHANNELS;
}

bit_t LMIC_queryChannel(u1_t channel, lmic_channel_info_t *pInfo) {
        if (channel >= MAX_CHANNELS || pInfo == NULL)
                return 0;

        u4_t const freqBand = LMIC.channelFreq[channel];

        pInfo->uplinkFreq = freqBand & ~(u4_t)3;
        pInfo->group = (u1_t)(freqBand & 3);
#if !defined(DISABLE_MCMD_DlChannelReq)
        pInfo->downlinkFreq = LMIC.channelDlFreq[channel] != 0
                                ? LMIC.channelDlFreq[channel]
                                : pInfo->uplinkFreq;
#else
        pInfo->downlinkFreq = pInfo->uplinkFreq;
#endif
        pInfo->drMap = LMIC.channelDrMap[channel];
        pInfo->enabled = (LMIC.channelMap & (1u << channel)) != 0;
        pInfo->isDefault = channel < LMIC_queryNumDefaultChannels();
        pInfo->bandwidth = LMICeulike_bandwidthFromDrMap(pInfo->drMap);
        return 1;
}

//
// Session state: the configurable-channel variant
//

#if !defined(DISABLE_MCMD_DlChannelReq)
static u4_t getChannelDlFreq(u1_t ch) { return LMIC.channelDlFreq[ch]; }
static void setChannelDlFreq(u1_t ch, u4_t f) { LMIC.channelDlFreq[ch] = f; }
#else
static u4_t getChannelDlFreq(u1_t ch) { LMIC_UNREFERENCED_PARAMETER(ch); return 0; }
static void setChannelDlFreq(u1_t ch, u4_t f) { LMIC_UNREFERENCED_PARAMETER(ch); LMIC_UNREFERENCED_PARAMETER(f); }
#endif

// offsets within the variant; see the layout in lmic_session_state.h
enum {
        SS_KIND = 0,
        SS_SIZE = 1,
        SS_GROUPS = 4,
        SS_CHMAP = 8,
        SS_SHUFFLE = 10,
        SS_DRMAP = 12,
        SS_UPFREQ = 44,
        SS_DLFREQ = 92,
        SS_GROUP_TABLE = 140,
        SS_GROUP_ENTRY = 8,
        SS_VARIANT_SIZE = 172,
};

// 24-bit frequency, most-significant byte first, in units of 100 Hz.
static void putFreq24(u1_t *p, u4_t freqHz) {
        u4_t const v = freqHz / 100;
        p[0] = (u1_t)(v >> 16);
        p[1] = (u1_t)(v >> 8);
        p[2] = (u1_t)v;
}

static u4_t getFreq24(const u1_t *p) {
        return (((u4_t)p[0] << 16) | ((u4_t)p[1] << 8) | p[2]) * 100;
}

void LMICeulike_saveChannelState(u1_t *pVariant, ostime_t now) {
        u4_t groups = 0;

        pVariant[SS_KIND] = LMIC_SESSION_STATE_CHANNELS_CONFIGURABLE;
        pVariant[SS_SIZE] = SS_VARIANT_SIZE;

        for (u1_t ch = 0; ch < MAX_CHANNELS; ++ch) {
                u4_t const freqGroup = LMIC.channelFreq[ch];

                groups |= (freqGroup & 3) << (2 * ch);
                os_wlsbf2(pVariant + SS_DRMAP + 2 * ch, LMIC.channelDrMap[ch]);
                putFreq24(pVariant + SS_UPFREQ + 3 * ch, freqGroup & ~(u4_t)3);
                putFreq24(pVariant + SS_DLFREQ + 3 * ch, getChannelDlFreq(ch));
        }
        os_wlsbf4(pVariant + SS_GROUPS, groups);
        os_wlsbf2(pVariant + SS_CHMAP, LMIC.channelMap);
        os_wlsbf2(pVariant + SS_SHUFFLE, LMIC.channelShuffleMap);

        for (u1_t g = 0; g < MAX_BANDS; ++g) {
                u1_t * const q = pVariant + SS_GROUP_TABLE + SS_GROUP_ENTRY * g;
                band_t const * const pGroup = &LMIC.bands[g];
                ostime_t const delta = pGroup->avail - now;

                os_wlsbf2(q, pGroup->txcap);
                q[2] = (u1_t)pGroup->txpow;
                q[3] = pGroup->lastchnl;
                os_wlsbf4(q + 4, delta > 0 ? (u4_t)delta : 0);
        }
}

bit_t LMICeulike_restoreChannelState(const u1_t *pVariant, ostime_t now, u1_t version) {
        if (pVariant[SS_KIND] != LMIC_SESSION_STATE_CHANNELS_CONFIGURABLE ||
            pVariant[SS_SIZE] != SS_VARIANT_SIZE)
                return 0;

        // the channels LMIC_reset() left enabled are the region defaults; leave them alone.
        u2_t const resetMap = LMIC.channelMap;
        u4_t const groups = os_rlsbf4(pVariant + SS_GROUPS);

        LMIC.channelMap |= os_rlsbf2(pVariant + SS_CHMAP);
        LMIC.channelShuffleMap = os_rlsbf2(pVariant + SS_SHUFFLE);

        for (u1_t ch = 0; ch < MAX_CHANNELS; ++ch) {
                if ((resetMap & (1u << ch)) != 0)
                        continue;

                (void) LMIC_setupChannel(
                        ch,
                        getFreq24(pVariant + SS_UPFREQ + 3 * ch),
                        os_rlsbf2(pVariant + SS_DRMAP + 2 * ch),
                        (s1_t)((groups >> (2 * ch)) & 3)
                        );
                setChannelDlFreq(ch, getFreq24(pVariant + SS_DLFREQ + 3 * ch));
        }

        for (u1_t g = 0; g < MAX_BANDS; ++g) {
                const u1_t * const q = pVariant + SS_GROUP_TABLE + SS_GROUP_ENTRY * g;
                band_t * const pGroup = &LMIC.bands[g];

                // V1 saved txpow into this slot; keep the region default then.
                if (version >= LMIC_SESSION_STATE_TAG_V2)
                        pGroup->txcap = os_rlsbf2(q);
                pGroup->txpow = (s1_t)q[2];
                pGroup->lastchnl = q[3];
                pGroup->avail = now + (ostime_t)os_rlsbf4(q + 4);
        }
        return 1;
}

#endif // CFG_LMIC_EU_like
