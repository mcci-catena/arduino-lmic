#!/bin/bash

##############################################################################
#
# Module: ttn-abp-feather-us915-dht22/extra/ci/lmic-filter.sh
#
# Function:
#	This script must be sourced; it sets variables used by other
#	scripts in this directory.
#
# Usage:
#	source ci/lmic-filter.sh
#
# Copyright and License:
#	See accompanying LICENSE.md file
#
# Author:
#	Terry Moore, MCCI	February 2021
#
##############################################################################

#### use the common code.
# shellcheck source=../../../../ci/lmic-filter-common.sh
source "$(dirname "$MCCI_CI_FILTER_NAME")"/../../../../ci/lmic-filter-common.sh

#### this sketch uses ABP, so on AVR also build without join, to fit
#### the feather32u4's flash (the sketch expects DISABLE_JOIN; see its
#### comments).
eval "$(declare -f _lmic_filter | sed '1s/_lmic_filter/_lmic_filter_common/')"

function _lmic_filter {
	if [[ "$1" = "projcfg" && "$MCCI_CI_ARCH" = "avr" ]]; then
		shift 2
		_projcfg_class_a "$@" "DISABLE_LMIC_FAILURE_TO" "DISABLE_JOIN"
	else
		_lmic_filter_common "$@"
	fi
}

#### end of file ####
