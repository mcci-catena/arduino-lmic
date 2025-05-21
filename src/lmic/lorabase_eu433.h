/*
* Copyright (c) 2014-2016 IBM Corporation.
* All rights reserved.
*
* Copyright (c) 2017, 2019 MCCI Corporation
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

#ifndef _lorabase_eu433_h_
#define _lorabase_eu433_h_

#ifndef _LMIC_CONFIG_PRECONDITIONS_H_
# include "lmic_config_preconditions.h"
#endif

/****************************************************************************\
|
| Basic definitions for EU433 (always in scope)
|
\****************************************************************************/


// Data rates (LoRaWAN Regional Parameters v1.0.3)
enum _dr_eu433_t {
    EU433_DR_SF12 = 0,
    EU433_DR_SF11,
    EU433_DR_SF10,
    EU433_DR_SF9,
    EU433_DR_SF8,
    EU433_DR_SF7,
    EU433_DR_SF7B,
    EU433_DR_FSK,
    EU433_DR_NONE,
    
    DR_PAGE_EU433 = 0x10 // moved w.r.t. eu868
};


// EU433 frequency bands (MHz)
// Constructed similarly to 868:
// 1-3 for default joins
// 4-6 for additional channels
// 6 coinciding with the beacon frequency
// J4-6 for supplemental J channels (which also get attached to CF list), assigned to biggest gap
// in reality, network server specifies these freqs
enum {
        EU433_F1 = 433175000,      // Default channel 1
        EU433_F2 = 433375000,      // Default channel 2
        EU433_F3 = 433575000,      // Default channel 3
        EU433_F4 = 433775000,
        EU433_F5 = 433975000,
        EU433_F6 = 434665000,      // Beacon/RX2 default
        EU433_J4 = 434175000,
        EU433_J5 = 434375000,
        EU433_J6 = 434575000,
};
enum {
        EU433_FREQ_MIN = 433050000,
        EU433_FREQ_MAX = 434790000  
};
enum {
        EU433_TX_EIRP_MAX_DBM = 1215      // 12.15 dBm (Maximum) EIRP. So subtract 3 dBm for a 3 dBi antenna. This is just documentation
};


enum { EU433_LMIC_REGION_EIRP = 1 };         // region uses EIRP


// Additional Definitions (unused) Compared to EU868

// Noting that we use 1% lorawan congestion-reduction 433 duty cycle, rather than 10% regulatory spec
#define EU433_CONSERVATIVE_DUTY_CYCLE_ENABLED 1

// Based on Table 30/31, payload length documentation
enum {
    EU433_MAX_PAYLOAD_LEN_REPEATER = 230,
    EU433_MAX_PAYLOAD_LEN_NO_REPEATER = 250,
    EU433_MAX_PAYLOAD_LEN_JOIN = 59  // For join requests, according to lowest (D0) rate used in join
};

#endif /* _lorabase_eu433_h_ */
