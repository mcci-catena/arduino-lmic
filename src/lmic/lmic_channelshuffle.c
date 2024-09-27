/*

Module:	lmic_channelshuffle.c

Function:
    Channel scheduling without replacement.

Copyright and License:
    This file copyright (C) 2021 by

        MCCI Corporation
        3520 Krums Corners Road
        Ithaca, NY  14850

    See accompanying LICENSE file for copyright and license information.

Author:
    Terry Moore, MCCI Corporation	April 2021

*/

#include "lmic.h"
#include <string.h>

/****************************************************************************\
|
|   Manifest constants and local declarations.
|
\****************************************************************************/

static unsigned sidewaysSum16(const uint16_t *pMask, uint16_t nEntries);
static unsigned findNthSetBit(const uint16_t *pMask, uint16_t bitnum);

/****************************************************************************\
|
|   Read-only data.
|
\****************************************************************************/

/****************************************************************************\
|
|   Variables.
|
\****************************************************************************/

/*

Name:	LMIC_findNextChannel()

Function:
    Scan a shuffle mask, and select a channel (without replacement).

Definition:
    int LMIC_findNextChannel(
                uint16_t *pShuffleMask,
                const uint16_t *pEnableMask,
                uint16_t nEntries,
                int lastChannel
                );

Description:
    pShuffleMask and pEnableMask are bit vectors. Channels correspond to
    bits in little-endian order; entry [0] has channels 0 through 15, entry
    [1] channels 16 through 31, and so forth. nEntries specifies the number
    of entries in the mask vectors.  The enable mask is 1 for a given channel
    if that channel is eligible for selection, 0 otherwise.

    This routine selects channels from the shuffle mask until all entries
    are exhausted; it then refreshes the shuffle mask from the enable mask.

    If it refreshes the channel mask, lastChannel is taken as a channel number
    that is to be avoided in the next selection. (This is to avoid back-to-back
    use of a channel across a refresh boundary.) Otherwise lastChannel is
    ignored. This avoidance can be suppresed by setting lastChannel to -1.
    If only one channel is enabled, lastChannel is also ignored. If lastChannel
    is actually disabled, lastChannel is also ignored.

Returns:
    A channel number, in 0 .. nEntries-1, or -1 if the enable mask is
    identically zero.

Notes:
    This routine is somewhat optimized for AVR processors, which don't have
    multi-bit shifts.

*/

int LMIC_findNextChannel(
    uint16_t *pShuffleMask,
    const uint16_t *pEnableMask,
    uint16_t nEntries,
    int lastChannel
) {
    unsigned nSet16;
    uint16_t saveLastChannelVal;

    // in case someone has changed the enable mask, update
    // the shuffle mask so there are no disable bits set.
    for (unsigned i = 0; i < nEntries; ++i) {
        pShuffleMask[i] &= pEnableMask[i];
    }

    // count the set bits in the shuffle mask (with a factor of 16 for speed)
    nSet16 = sidewaysSum16(pShuffleMask, nEntries);

    // if zero, copy the enable mask to the shuffle mask, and recount
    if (nSet16 == 0) {
        memcpy(pShuffleMask, pEnableMask, nEntries * sizeof(*pShuffleMask));
        nSet16 = sidewaysSum16(pShuffleMask, nEntries);
    } else {
        // don't try to skip the last channel because it can't be chosen.
        lastChannel = -1;
    }

    // if still zero, return -1.
    if (nSet16 == 0) {
        return -1;
    }

    // if we have to skip a channel, and we have more than one choice, turn off
    // the last channel bit. Post condition: if we really clered a bit,
    // saveLastChannelVal will be non-zero.
    saveLastChannelVal = 0;
    if (nSet16 > 16 && lastChannel >= 0 && lastChannel <= (int)(nEntries * 16)) {
        uint16_t const saveLastChannelMask = (1 << (lastChannel & 0xF));

        saveLastChannelVal = pShuffleMask[lastChannel >> 4] & saveLastChannelMask;
        pShuffleMask[lastChannel >> 4] &= ~saveLastChannelMask;

        // if we cleared a bit, reduce the count.
        if (saveLastChannelVal > 0)
            nSet16 -= 16;
    }

    if (saveLastChannelVal == 0) {
        // We didn't eliminate a channel, so we don't have to worry.
        lastChannel = -1;
    }

    // get a random number
    unsigned choice = os_getRndU2() % ((uint16_t)nSet16 >> 4);

    // choose a bit based on set bit
    unsigned channel = findNthSetBit(pShuffleMask, choice);
    pShuffleMask[channel / 16] ^= (1 << (channel & 0xF));

    // handle channel skip
    if (lastChannel >= 0) {
        pShuffleMask[lastChannel >> 4] |= saveLastChannelVal;
    }
    return channel;
}

static unsigned sidewaysSum16(const uint16_t *pMask, uint16_t nEntries) {
    unsigned result;

    result = 0;
    for (; nEntries > 0; --nEntries, ++pMask)
        {
        uint16_t v = *pMask;

        // the following is an adaptation of Knuth 7.1.3 (62). To avoid
        // lots of shifts (slow on AVR, and code intensive) and table lookups,
        // we sum popc * 16, then divide by 16.

        // sum adjacent bits, making a series of 2-bit sums
        v = v - ((v >> 1) & 0x5555u);
        v = (v & 0x3333u) + ((v >> 2) & 0x3333u);
        // this assumes multiplies are essentialy free; 
        v = (v & 0xF0F0u) + ((v & 0x0F0Fu) * 16u);
        // Accumulate result, but note it's times 16.
        // AVR compiler should optimize the x8 shift.
        result += (v & 0xFF) + (v >> 8);
        }

    //
    return result;
}

static unsigned findNthSetBit(const uint16_t *pMask, uint16_t bitnum) {
    unsigned result;
    result = 0;
    bitnum = bitnum * 16;
    for (;; result += 16) {
        uint16_t m = *pMask++;
        if (m == 0)
            continue;
        uint16_t v = m - ((m >> 1) & 0x5555u);
        v = (v & 0x3333u) + ((v >> 2) & 0x3333u);
        // this assumes multiplies are essentialy free; 
        v = (v & 0xF0F0u) + ((v & 0x0F0Fu) * 16u);
        // Accumulate result, but note it's times 16.
        // AVR compiler should optimize the x8 shift.
        v = (v & 0xFF) + (v >> 8);
        if (v <= bitnum)
            bitnum -= v;
        else {
            // the selected bit is in this word. We need to count.
            while (bitnum > 0) {
                m &= m - 1;
                bitnum -= 16;
            }
            // now the lsb of m is our choice.
            // get a mask, then use Knuth 7.1.3 (59) to find the
            // bit number.
            m &= -m;
            result += ((m & 0x5555u) ? 0 : 1)
                    + ((m & 0x3333u) ? 0 : 2)
                    + ((m & 0x0F0Fu) ? 0 : 4)
                    + ((m & 0x00FFu) ? 0 : 8)
                    ;
            break;
        }
    }

    return result;
}