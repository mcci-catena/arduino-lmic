#!/bin/bash

##############################################################################
#
# Module: ttn-otaa-feather-us915/extra/ci/lmic-filter.sh
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

#### end of file ####
