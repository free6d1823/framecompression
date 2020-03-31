#!/bin/bash
#
# Copyright:
# ----------------------------------------------------------------------------
# This confidential and proprietary software may be used only as authorized
# by a licensing agreement from ARM Limited or its affiliates.
#      (C) COPYRIGHT 2013-2015 ARM Limited or its affiliates, ALL RIGHTS RESERVED
# The entire notice above must be reproduced on all authorized copies and
# copies may only be made to the extent permitted by a licensing agreement
# from ARM Limited or its affiliates.
# ----------------------------------------------------------------------------
#

make

echo "------ Encode seemore.afb"

# Encode. Also store raw file using -r option which can be used to for comparisson
# to decode output
./afbcenc -r seemore.enc -i seemore.png -o seemore.afb || exit

