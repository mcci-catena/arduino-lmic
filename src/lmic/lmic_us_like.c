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

#if CFG_LMIC_US_like

#ifndef LMICuslike_getFirst500kHzDR
# error "LMICuslike_getFirst500kHzDR() not defined by bandplan"
#endif

///
/// \brief set LMIC.txChan to the next selected channel.
///
/// \param [in] start   first channel number
/// \param [in] end     one past the last channel number
///
/// \details
///     We set up a call to LMIC_findNextChannel using the channelShuffleMap and
///     the channelEnableMap. We subset these based on start and end. \p start must
///     be a multiple of 16.
///
static void setNextChannel(uint16_t start, uint16_t end, uint16_t count) {
        ASSERT(count>0);
        ASSERT(start<end);
        ASSERT(count <= (end - start));
        ASSERT((start & 0xF) == 0);
        uint16_t const mapStart = start >> 4;
        uint16_t const mapEntries = (end - start + 15) >> 4;

        int candidate = start + LMIC_findNextChannel(
                                LMIC.channelShuffleMap + mapStart,
                                LMIC.channelMap + mapStart,
                                mapEntries,
                                LMIC.txChnl == 0xFF ? -1 : LMIC.txChnl
                                );

        if (candidate >= 0)
                LMIC.txChnl = candidate;
}



bit_t LMIC_setupBand(u1_t bandidx, s1_t txpow, u2_t txcap) {
        LMIC_API_PARAMETER(bandidx);
        LMIC_API_PARAMETER(txpow);
        LMIC_API_PARAMETER(txcap);

        // nothing; just succeed.
	return 1;
}


void LMICuslike_initDefaultChannels(bit_t fJoin) {
        LMIC_API_PARAMETER(fJoin);

        // things work the same for join as normal.
        for (u1_t i = 0; i<4; i++)
                LMIC.channelMap[i] = 0xFFFF;
        LMIC.channelMap[4] = 0x00FF;
        os_clearMem(LMIC.channelShuffleMap, sizeof(LMIC.channelShuffleMap));
        LMIC.activeChannels125khz = 64;
        LMIC.activeChannels500khz = 8;
        // choose a random channel.
        LMIC.txChnl = 0xFF;
}

// verify that a given setting is permitted
bit_t LMICuslike_canMapChannels(u1_t chpage, u2_t chmap) {
	/*
	|| MCMD_LinkADRReq_ChMaskCntl_USLIKE_125ON and MCMD_LinkADRReq_ChMaskCntl_USLIKE_125OFF are special. The
	|| channel map appllies to 500kHz (ch 64..71) and in addition
	|| all channels 0..63 are turned off or on.  MCMC_LADR_CHP_BANK
	|| is also special, in that it enables subbands.
        ||
        || TODO(tmm@mcci.com) revise the 0xFF00 mask for regions with other than
        || eight 500 kHz channels.
	*/
	if (chpage < MCMD_LinkADRReq_ChMaskCntl_USLIKE_SPECIAL) {
		// operate on channels 0..15, 16..31, 32..47, 48..63, 64..71
		if (chpage == MCMD_LinkADRReq_ChMaskCntl_USLIKE_500K) {
			if (chmap & 0xFF00) {
				// those are reserved bits, fail.
				return 0;
			}
		} else {
			return 1;
		}
	} else if (chpage == MCMD_LinkADRReq_ChMaskCntl_USLIKE_BANK) {
		if ((chmap & 0xFF00) != 0) {
			// Reserved bits set, fail.
			return 0;
		}
	} else if (chpage == MCMD_LinkADRReq_ChMaskCntl_USLIKE_125ON ||
	           chpage == MCMD_LinkADRReq_ChMaskCntl_USLIKE_125OFF) {
                //
                // if disabling all 125kHz chans, you might think we must have
                // at least one 500kHz chan; but that's a local conclusion.
                // Some network servers will disable all (including 500kHz)
                // then turn things back on in the next LinkADRReq. So
                // we can't fail that here.
                //
                // But don't allow reserved bits to be set in chmap.
                //
                if ((chmap & 0xFF00) != 0)
			return 0;
	} else {
		return 0;
	}

	// if we get here, it looks legal.
	return 1;
}

// map channels. return true if configuration looks valid.
bit_t LMICuslike_mapChannels(u1_t chpage, u2_t chmap) {
	/*
	|| MCMD_LinkADRReq_ChMaskCntl_USLIKE_125ON and MCMD_LinkADRReq_ChMaskCntl_USLIKE_125OFF are special. The
	|| channel map appllies to 500kHz (ch 64..71) and in addition
	|| all channels 0..63 are turned off or on.  MCMC_LADR_CHP_BANK
	|| is also special, in that it enables subbands.
	*/
	if (chpage == MCMD_LinkADRReq_ChMaskCntl_USLIKE_BANK) {
		// each bit enables a bank of channels
		for (u1_t subband = 0; subband < 8; ++subband, chmap >>= 1) {
			if (chmap & 1) {
				LMIC_enableSubBand(subband);
			} else {
				LMIC_disableSubBand(subband);
			}
		}
        } else {
                u1_t base, top;

                if (chpage < MCMD_LinkADRReq_ChMaskCntl_USLIKE_SPECIAL) {
                        // operate on channels 0..15, 16..31, 32..47, 48..63
                        // note that the chpage hasn't been shifted right, so
                        // it's really the base.
                        base = chpage;
                        top = base + 16;
                        if (base == 64) {
                                top = 72;
                        }
                } else /* if (chpage == MCMD_LinkADRReq_ChMaskCntl_USLIKE_125ON ||
                        chpage == MCMD_LinkADRReq_ChMaskCntl_USLIKE_125OFF) */ {
                        u1_t const en125 = chpage == MCMD_LinkADRReq_ChMaskCntl_USLIKE_125ON;

                        // enable or disable all 125kHz channels
                        for (u1_t chnl = 0; chnl < 64; ++chnl) {
                                if (en125)
                                        LMIC_enableChannel(chnl);
                                else
                                        LMIC_disableChannel(chnl);
                        }

                        // then apply mask to top 8 channels.
                        base = 64;
                        top = 72;
                }

                // apply chmap to channels in [base..top-1].
                // Use enable/disable channel to keep activeChannel counts in sync.
                for (u1_t chnl = base; chnl < top; ++chnl, chmap >>= 1) {
                        if (chmap & 0x0001)
                                LMIC_enableChannel(chnl);
                        else
                                LMIC_disableChannel(chnl);
                }
        }

        LMICOS_logEventUint32("LMICuslike_mapChannels", ((u4_t)LMIC.activeChannels125khz << 16u)|(LMIC.activeChannels500khz << 0u));
	return (LMIC.activeChannels125khz > 0) || (LMIC.activeChannels500khz > 0);
}

// US does not have duty cycling - return now as earliest TX time
// but also do the channel hopping dance.
ostime_t LMICuslike_nextTx(ostime_t now) {
        // TODO(tmm@mcci.com): use a static const for US-like
        if (LMIC.datarate >= LMICuslike_getFirst500kHzDR()) { // 500kHz
                if (LMIC.activeChannels500khz > 0) {
                        setNextChannel(64, 64 + 8, LMIC.activeChannels500khz);
                } else if (LMIC.activeChannels125khz > 0) {
                        LMIC.datarate = lowerDR(LMICuslike_getFirst500kHzDR(), 1);
                        setNextChannel(0, 64, LMIC.activeChannels125khz);
                        LMICOS_logEvent("LMICuslike_nextTx: no 500k, choose 125k");
                } else {
                        LMICOS_logEvent("LMICuslike_nextTx: no channels at all (500)");
                }
        }
        else { // 125kHz
                if (LMIC.activeChannels125khz > 0) {
                        setNextChannel(0, 64, LMIC.activeChannels125khz);
                } else if (LMIC.activeChannels500khz > 0) {
                        LMIC.datarate = LMICuslike_getFirst500kHzDR();
                        setNextChannel(64, 64 + 8, LMIC.activeChannels500khz);
                        LMICOS_logEvent("LMICuslike_nextTx: no 125k, choose 500k");
                } else {
                        LMICOS_logEvent("LMICuslike_nextTx: no channels at all (125)");
                }
        }
        return now;
}

bit_t LMICuslike_isDataRateFeasible(dr_t dr) {
        if (dr >= LMICuslike_getFirst500kHzDR()) { // 500kHz
                return LMIC.activeChannels500khz > 0;
        } else {
                return LMIC.activeChannels125khz > 6;
        }
}

#if !defined(DISABLE_JOIN)
void LMICuslike_initJoinLoop(void) {
        // set an initial condition so that setNextChannel()'s preconds are met
        LMIC.txChnl = 0xFF;

        // then chose a new channel.  This gives us a random first channel for
        // the join. The join logic uses the current txChnl,
        // then changes after the rx window expires; so we need to set a valid
        // starting point.
        setNextChannel(0, 64, LMIC.activeChannels125khz);

        // make sure LMIC.txend is valid.
        LMIC.txend = os_getTime();
        ASSERT((LMIC.opmode & OP_NEXTCHNL) == 0);

        // make sure the datarate is set to DR2 per LoRaWAN regional reqts V1.0.2,
        // section 2.*.2
        LMICcore_setDrJoin(DRCHG_SET, LMICbandplan_getInitialDrJoin());

        // TODO(tmm@mcci.com) need to implement the transmit randomization and
        // duty cycle restrictions from LoRaWAN V1.0.2 section 7.
}
#endif // !DISABLE_JOIN

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
ostime_t LMICuslike_nextJoinState(void) {
        // Try the following:
        //   DR0 (SF10)  on a random channel 0..63
        //      (honoring enable mask)
        //   DR4 (SF8C)  on a random 500 kHz channel 64..71
        //      (always determined by
        //       previously selected
        //       125 kHz channel)
        //
        u1_t failed = 0;
        // TODO(tmm@mcci.com) parameterize for US-like
        if (LMIC.datarate != LMICuslike_getFirst500kHzDR()) {
                // assume that 500 kHz equiv of last 125 kHz channel
                // is also enabled, and use it next.
                LMIC.txChnl_125kHz = LMIC.txChnl;
                LMIC.txChnl = 64 + (LMIC.txChnl >> 3);
                LMICcore_setDrJoin(DRCHG_SET, LMICuslike_getFirst500kHzDR());
        }
        else {
                // restore invariant
                LMIC.txChnl = LMIC.txChnl_125kHz;
                setNextChannel(0, 64, LMIC.activeChannels125khz);

                // TODO(tmm@mcci.com) parameterize
                s1_t dr = LMICuslike_getJoin125kHzDR();
                if ((++LMIC.txCnt & 0x7) == 0) {
                        failed = 1; // All DR exhausted - signal failed
                }
                LMICcore_setDrJoin(DRCHG_SET, dr);
        }
        // tell the main loop that we've already selected a channel.
        LMIC.opmode &= ~OP_NEXTCHNL;

        // TODO(tmm@mcci.com): change delay to (0:1) secs + a known t0, but randomized;
        // starting adding a bias after 1 hour, 25 hours, etc.; and limit the duty
        // cycle on power up. For testability, add a way to set the join start time
        // externally (a test API) so we can check this feature.
        // See https://github.com/mcci-catena/arduino-lmic/issues/2
	// Current code doesn't match LoRaWAN 1.0.2 requirements.

        LMIC.txend = os_getTime() +
                (isTESTMODE()
                        // Avoid collision with JOIN ACCEPT being sent by GW (but we missed it - GW is still busy)
                        ? DNW2_SAFETY_ZONE
                        // Otherwise: randomize join (street lamp case):
                        // SF10:16, SF9=8,..SF8C:1secs
                        : LMICcore_rndDelay(16 >> LMIC.datarate));
        // 1 - triggers EV_JOIN_FAILED event
        return failed;
}
#endif

#if !defined(DISABLE_JOIN)
void LMICuslike_processJoinAcceptCFList(void) {
    if ( LMICbandplan_hasJoinCFlist() &&
         LMIC.frame[OFF_CFLIST + 15] == LORAWAN_JoinAccept_CFListType_MASK ) {
        u1_t dlen;

        dlen = OFF_CFLIST;
        for( u1_t chidx = 0; chidx < 8 * sizeof(LMIC.channelMap); chidx += 16, dlen += 2 ) {
            u2_t mask = os_rlsbf2(&LMIC.frame[dlen]);
#if LMIC_DEBUG_LEVEL > 1
            LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": Setup channel mask, group=%u, mask=%04x\n", os_getTime(), chidx, mask);
#endif
            for ( u1_t chnum = chidx; chnum < chidx + 16; ++chnum, mask >>= 1) {
                if (chnum >= 72) {
                    break;
                } else if (mask & 1) {
                    LMIC_enableChannel(chnum);
                } else {
                    LMIC_disableChannel(chnum);
                }
            }
        }
    }
}
#endif // !DISABLE_JOIN

void LMICuslike_saveAdrState(lmic_saved_adr_state_t *pStateBuffer) {
        os_copyMem(
                pStateBuffer->channelMap,
                LMIC.channelMap,
                sizeof(LMIC.channelMap)
        );
        pStateBuffer->activeChannels125khz = LMIC.activeChannels125khz;
        pStateBuffer->activeChannels500khz = LMIC.activeChannels500khz;
}

void LMICuslike_restoreAdrState(const lmic_saved_adr_state_t *pStateBuffer) {
        os_copyMem(
                LMIC.channelMap,
                pStateBuffer->channelMap,
                sizeof(LMIC.channelMap)
        );
        LMIC.activeChannels125khz = pStateBuffer->activeChannels125khz;
        LMIC.activeChannels500khz = pStateBuffer->activeChannels500khz;
}


bit_t LMICuslike_compareAdrState(const lmic_saved_adr_state_t *pStateBuffer) {
        return memcmp(pStateBuffer->channelMap, LMIC.channelMap, sizeof(LMIC.channelMap)) != 0;
}

#endif // CFG_LMIC_US_like
