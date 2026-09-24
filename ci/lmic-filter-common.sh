#!/bin/bash

##############################################################################
#
# Module: lmic-filter-common.sh
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

#### Capture the file path ####
MCCI_THISFILE="$0"

#### mandatory function: do the filtering
function _lmic_filter {
	declare -r CMD="$1"
	shift
	case "$CMD" in

	# return 0 (success) if should process this sketch.
	"process")
		case "$MCCI_CI_ARCH:$(basename "$1")" in
			# we need to skip this sketch until the SAMD
			# bsp is updated; the Time library uses prog_read_ptr()
			# which is broken in v2.3.0
			"samd:ttn-otaa-network-time.ino")
				return 1
				;;
			# some of the feather sketches fail on non-Feathers
			"esp32:raw-feather.ino" | \
			"esp32:ttn-otaa-feather-us915.ino")
				return 1
				;;
			# some of the feather sketches fail on non-Feathers
			"stm32:raw-feather.ino" | \
			"stm32:ttn-otaa-feather-us915.ino")
				return 1
				;;
			# compliance-otaa-halconfig exceeds flash on AVR (feather32u4)
			"avr:compliance-otaa-halconfig.ino")
				return 1
				;;
			*)
				return 0
				;;
		esac
		;;

	# print 1 if must use projcfg; 0 if not forced.
	"use-projcfg")
		if [[ "$MCCI_CI_ARCH" = "avr" ]]; then
			echo 1
		else
			echo 0
		fi
		;;

	# call the suitable flavor of _projcfg.
	"projcfg")
		declare -r LMIC_FILTER_SKETCH="$1"
		_debug _lmic_filter: LMIC_FILTER_SKETCH="$LMIC_FILTER_SKETCH"
		shift
		if [[ "$MCCI_CI_ARCH" = "avr" ]]; then
			_projcfg_class_a "$@"  "DISABLE_LMIC_FAILURE_TO"
		else
			_projcfg "$@"
		fi
		;;
	*)
		_error "_lmic_filter: unknown command:" "$@"
		;;
	esac
}
