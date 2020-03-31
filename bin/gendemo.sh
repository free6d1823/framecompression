#!/bin/bash
#
# Copyright:
# ----------------------------------------------------------------------------
# This confidential and proprietary software may be used only as authorized
# by a licensing agreement from ARM Limited or its affiliates.
#      (C) COPYRIGHT 2015 ARM Limited or its affiliates, ALL RIGHTS RESERVED
# The entire notice above must be reproduced on all authorized copies and
# copies may only be made to the extent permitted by a licensing agreement
# from ARM Limited or its affiliates.
# ----------------------------------------------------------------------------
#

make

echo "------ Generate randbuf.afb"

# Generate.
./afbcgen -w 240 -h 240 -f r8g8b8a8 -o randbuf.afb || exit

