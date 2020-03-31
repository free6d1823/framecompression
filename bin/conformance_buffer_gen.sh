#!/bin/sh
#
# Copyright:
# ----------------------------------------------------------------------------
# This confidential and proprietary software may be used only as authorized
# by a licensing agreement from ARM Limited or its affiliates.
#      (C) COPYRIGHT 2015-2017 ARM Limited or its affiliates, ALL RIGHTS RESERVED
# The entire notice above must be reproduced on all authorized copies and
# copies may only be made to the extent permitted by a licensing agreement
# from ARM Limited or its affiliates.
# ----------------------------------------------------------------------------

# Include generated format table.
# It is used to populate ADDRESSING, PROD_FMTS and CONS_FMTS from PROD_IP and CONS_IP.
# It also has functions to print the available consumers and producers.

print_prod_ips () {
  echo "Display G51 G71 GPU Heimdall ISP T760-r0px T760-r1p0 T820-r0px T820-r1p0 T830-r0px T830-r1p0 T860-r0px T860-r1p0 T860-r2p0 T880-r0px T880-r1p0 T880-r2p0 V500 V550 V61 VideoDecoder"
}
print_cons_ips () {
  echo "AnyDecoder Cetus DP500 DP550 Display G51 G71 GPU Gemini Heimdall T760-r0px T760-r1p0 T820-r0px T820-r1p0 T830-r0px T830-r1p0 T860-r0px T860-r1p0 T860-r2p0 T880-r0px T880-r1p0 T880-r2p0 V61 VideoEncoder"
}
lookup_prod_fmts () {
  PROD_IP=$1
  if [ $PROD_IP = "V500" ]
    then
    PROD_FMTS="layout1_nontiled_nonsplit_random_yuv420"
  elif [ $PROD_IP = "T760-r1p0" ]
    then
    PROD_FMTS="layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8"
  elif [ $PROD_IP = "T820-r0px" ]
    then
    PROD_FMTS="layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8"
  elif [ $PROD_IP = "T820-r1p0" ]
    then
    PROD_FMTS="layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8"
  elif [ $PROD_IP = "V61" ]
    then
    PROD_FMTS="layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout1_tiled_nonsplit_sparse_yuv420 layout1_tiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_sparse_yuv422 layout2_tiled_nonsplit_sparse_yuv422"
  elif [ $PROD_IP = "T760-r0px" ]
    then
    PROD_FMTS="layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8"
  elif [ $PROD_IP = "T830-r0px" ]
    then
    PROD_FMTS="layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8"
  elif [ $PROD_IP = "T860-r0px" ]
    then
    PROD_FMTS="layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8"
  elif [ $PROD_IP = "ISP" ]
    then
    PROD_FMTS="layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_tiled_nonsplit_sparse_r5g6b5 layout0_tiled_split_sparse_r10g10b10a2 layout0_tiled_split_sparse_r8g8b8 layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout1_tiled_nonsplit_sparse_yuv420 layout1_tiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_sparse_yuv422 layout2_tiled_nonsplit_sparse_yuv422 layout3_nontiled_split_sparse_r10g10b10a2 layout3_nontiled_split_sparse_r8g8b8 layout3_tiled_split_sparse_r10g10b10a2 layout3_tiled_split_sparse_r8g8b8 layout4_nontiled_nonsplit_sparse_r5g6b5 layout4_tiled_nonsplit_sparse_r5g6b5"
  elif [ $PROD_IP = "V550" ]
    then
    PROD_FMTS="layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_random_yuv420_10b layout2_nontiled_nonsplit_random_yuv422"
  elif [ $PROD_IP = "T860-r2p0" ]
    then
    PROD_FMTS="layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8"
  elif [ $PROD_IP = "VideoDecoder" ]
    then
    PROD_FMTS="layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout1_tiled_nonsplit_sparse_yuv420 layout1_tiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_sparse_yuv422 layout2_tiled_nonsplit_sparse_yuv422 layout5_nontiled_nonsplit_sparse_yuv420 layout5_nontiled_nonsplit_sparse_yuv420_10b layout5_tiled_nonsplit_sparse_yuv420 layout5_tiled_nonsplit_sparse_yuv420_10b layout6_nontiled_nonsplit_sparse_yuv422 layout6_tiled_nonsplit_sparse_yuv422"
  elif [ $PROD_IP = "G51" ]
    then
    PROD_FMTS="layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout0_tiled_nonsplit_sparse_r10g10b10a2 layout0_tiled_nonsplit_sparse_r5g5b5a1 layout0_tiled_nonsplit_sparse_r5g6b5 layout0_tiled_nonsplit_sparse_r8g8b8 layout0_tiled_nonsplit_sparse_r8g8b8a8 layout0_tiled_split_sparse_r10g10b10a2 layout0_tiled_split_sparse_r8g8b8 layout0_tiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout1_tiled_nonsplit_sparse_yuv420 layout1_tiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_sparse_yuv422 layout2_tiled_nonsplit_sparse_yuv422 layout3_nontiled_split_sparse_r10g10b10a2 layout3_nontiled_split_sparse_r8g8b8 layout3_nontiled_split_sparse_r8g8b8a8 layout3_tiled_split_sparse_r10g10b10a2 layout3_tiled_split_sparse_r8g8b8 layout3_tiled_split_sparse_r8g8b8a8 layout4_nontiled_nonsplit_sparse_r5g5b5a1 layout4_nontiled_nonsplit_sparse_r5g6b5 layout4_tiled_nonsplit_sparse_r5g5b5a1 layout4_tiled_nonsplit_sparse_r5g6b5"
  elif [ $PROD_IP = "T880-r1p0" ]
    then
    PROD_FMTS="layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8"
  elif [ $PROD_IP = "G71" ]
    then
    PROD_FMTS="layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_sparse_yuv422 layout3_nontiled_split_sparse_r10g10b10a2 layout3_nontiled_split_sparse_r8g8b8 layout3_nontiled_split_sparse_r8g8b8a8 layout4_nontiled_nonsplit_sparse_r5g5b5a1 layout4_nontiled_nonsplit_sparse_r5g6b5"
  elif [ $PROD_IP = "T880-r2p0" ]
    then
    PROD_FMTS="layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8"
  elif [ $PROD_IP = "GPU" ]
    then
    PROD_FMTS="layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout0_tiled_nonsplit_sparse_r10g10b10a2 layout0_tiled_nonsplit_sparse_r5g5b5a1 layout0_tiled_nonsplit_sparse_r5g6b5 layout0_tiled_nonsplit_sparse_r8g8b8 layout0_tiled_nonsplit_sparse_r8g8b8a8 layout0_tiled_split_sparse_r10g10b10a2 layout0_tiled_split_sparse_r8g8b8 layout0_tiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout1_tiled_nonsplit_sparse_yuv420 layout1_tiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_sparse_yuv422 layout2_tiled_nonsplit_sparse_yuv422 layout3_nontiled_split_sparse_r10g10b10a2 layout3_nontiled_split_sparse_r8g8b8 layout3_nontiled_split_sparse_r8g8b8a8 layout3_tiled_split_sparse_r10g10b10a2 layout3_tiled_split_sparse_r8g8b8 layout3_tiled_split_sparse_r8g8b8a8 layout4_nontiled_nonsplit_sparse_r5g5b5a1 layout4_nontiled_nonsplit_sparse_r5g6b5 layout4_tiled_nonsplit_sparse_r5g5b5a1 layout4_tiled_nonsplit_sparse_r5g6b5 layout5_nontiled_nonsplit_sparse_yuv420 layout5_nontiled_nonsplit_sparse_yuv420_10b layout5_tiled_nonsplit_sparse_yuv420 layout5_tiled_nonsplit_sparse_yuv420_10b layout6_nontiled_nonsplit_sparse_yuv422 layout6_tiled_nonsplit_sparse_yuv422"
  elif [ $PROD_IP = "Display" ]
    then
    PROD_FMTS="layout1_nontiled_nonsplit_sparse_yuv420 layout1_tiled_nonsplit_sparse_yuv420"
  elif [ $PROD_IP = "T880-r0px" ]
    then
    PROD_FMTS="layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8"
  elif [ $PROD_IP = "T830-r1p0" ]
    then
    PROD_FMTS="layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8"
  elif [ $PROD_IP = "Heimdall" ]
    then
    PROD_FMTS="layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_sparse_yuv422 layout3_nontiled_split_sparse_r10g10b10a2 layout3_nontiled_split_sparse_r8g8b8 layout3_nontiled_split_sparse_r8g8b8a8 layout4_nontiled_nonsplit_sparse_r5g5b5a1 layout4_nontiled_nonsplit_sparse_r5g6b5"
  elif [ $PROD_IP = "T860-r1p0" ]
    then
    PROD_FMTS="layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8"
  else
    echo Error: invalid producer IP $PROD_IP
    echo ""
    print_usage
    exit 1
  fi
}
lookup_cons_fmts () {
  CONS_IP=$1
  if [ $CONS_IP = "T760-r1p0" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_sparse_yuv420 layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "Display" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout0_tiled_nonsplit_sparse_r10g10b10a2 layout0_tiled_nonsplit_sparse_r5g5b5a1 layout0_tiled_nonsplit_sparse_r5g6b5 layout0_tiled_nonsplit_sparse_r8g8b8 layout0_tiled_nonsplit_sparse_r8g8b8a8 layout0_tiled_split_sparse_r10g10b10a2 layout0_tiled_split_sparse_r8g8b8 layout0_tiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout1_tiled_nonsplit_sparse_yuv420 layout1_tiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_sparse_yuv422 layout2_tiled_nonsplit_sparse_yuv422 layout3_nontiled_split_sparse_r10g10b10a2 layout3_nontiled_split_sparse_r8g8b8 layout3_nontiled_split_sparse_r8g8b8a8 layout3_tiled_split_sparse_r10g10b10a2 layout3_tiled_split_sparse_r8g8b8 layout3_tiled_split_sparse_r8g8b8a8 layout4_nontiled_nonsplit_sparse_r5g5b5a1 layout4_nontiled_nonsplit_sparse_r5g6b5 layout4_tiled_nonsplit_sparse_r5g5b5a1 layout4_tiled_nonsplit_sparse_r5g6b5 layout5_nontiled_nonsplit_sparse_yuv420 layout5_nontiled_nonsplit_sparse_yuv420_10b layout5_tiled_nonsplit_sparse_yuv420 layout5_tiled_nonsplit_sparse_yuv420_10b layout6_nontiled_nonsplit_sparse_yuv422 layout6_tiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "T820-r0px" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_sparse_yuv420 layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "Cetus" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout0_tiled_nonsplit_random_r10g10b10a2 layout0_tiled_nonsplit_random_r5g5b5a1 layout0_tiled_nonsplit_random_r5g6b5 layout0_tiled_nonsplit_random_r8g8b8 layout0_tiled_nonsplit_random_r8g8b8a8 layout0_tiled_nonsplit_sparse_r10g10b10a2 layout0_tiled_nonsplit_sparse_r5g5b5a1 layout0_tiled_nonsplit_sparse_r5g6b5 layout0_tiled_nonsplit_sparse_r8g8b8 layout0_tiled_nonsplit_sparse_r8g8b8a8 layout0_tiled_split_sparse_r10g10b10a2 layout0_tiled_split_sparse_r8g8b8 layout0_tiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_random_yuv420_10b layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout1_tiled_nonsplit_random_yuv420 layout1_tiled_nonsplit_random_yuv420_10b layout1_tiled_nonsplit_sparse_yuv420 layout1_tiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422 layout2_tiled_nonsplit_random_yuv422 layout2_tiled_nonsplit_sparse_yuv422 layout3_nontiled_split_sparse_r10g10b10a2 layout3_nontiled_split_sparse_r8g8b8 layout3_nontiled_split_sparse_r8g8b8a8 layout3_tiled_split_sparse_r10g10b10a2 layout3_tiled_split_sparse_r8g8b8 layout3_tiled_split_sparse_r8g8b8a8 layout4_nontiled_nonsplit_random_r5g5b5a1 layout4_nontiled_nonsplit_random_r5g6b5 layout4_nontiled_nonsplit_sparse_r5g5b5a1 layout4_nontiled_nonsplit_sparse_r5g6b5 layout4_tiled_nonsplit_random_r5g5b5a1 layout4_tiled_nonsplit_random_r5g6b5 layout4_tiled_nonsplit_sparse_r5g5b5a1 layout4_tiled_nonsplit_sparse_r5g6b5"
  elif [ $CONS_IP = "T820-r1p0" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_random_yuv420_10b layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "V61" ]
    then
    CONS_FMTS="layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_random_yuv420_10b layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout1_tiled_nonsplit_random_yuv420 layout1_tiled_nonsplit_random_yuv420_10b layout1_tiled_nonsplit_sparse_yuv420 layout1_tiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422 layout2_tiled_nonsplit_random_yuv422 layout2_tiled_nonsplit_sparse_yuv422 layout5_nontiled_nonsplit_random_yuv420 layout5_nontiled_nonsplit_random_yuv420_10b layout5_nontiled_nonsplit_sparse_yuv420 layout5_nontiled_nonsplit_sparse_yuv420_10b layout5_tiled_nonsplit_random_yuv420 layout5_tiled_nonsplit_random_yuv420_10b layout5_tiled_nonsplit_sparse_yuv420 layout5_tiled_nonsplit_sparse_yuv420_10b layout6_nontiled_nonsplit_random_yuv422 layout6_nontiled_nonsplit_sparse_yuv422 layout6_tiled_nonsplit_random_yuv422 layout6_tiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "VideoEncoder" ]
    then
    CONS_FMTS="layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout1_tiled_nonsplit_sparse_yuv420 layout1_tiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_sparse_yuv422 layout2_tiled_nonsplit_sparse_yuv422 layout5_nontiled_nonsplit_sparse_yuv420 layout5_nontiled_nonsplit_sparse_yuv420_10b layout5_tiled_nonsplit_sparse_yuv420 layout5_tiled_nonsplit_sparse_yuv420_10b layout6_nontiled_nonsplit_sparse_yuv422 layout6_tiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "T830-r0px" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_sparse_yuv420 layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "DP550" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_random_yuv420_10b layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "T860-r0px" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_sparse_yuv420 layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "T860-r2p0" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_random_yuv420_10b layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "DP500" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "G51" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout0_tiled_nonsplit_random_r10g10b10a2 layout0_tiled_nonsplit_random_r5g5b5a1 layout0_tiled_nonsplit_random_r5g6b5 layout0_tiled_nonsplit_random_r8g8b8 layout0_tiled_nonsplit_random_r8g8b8a8 layout0_tiled_nonsplit_sparse_r5g6b5 layout0_tiled_split_sparse_r10g10b10a2 layout0_tiled_split_sparse_r8g8b8 layout0_tiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_random_yuv420_10b layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout1_tiled_nonsplit_random_yuv420 layout1_tiled_nonsplit_random_yuv420_10b layout1_tiled_nonsplit_sparse_yuv420 layout1_tiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422 layout2_tiled_nonsplit_random_yuv422 layout2_tiled_nonsplit_sparse_yuv422 layout3_nontiled_split_sparse_r10g10b10a2 layout3_nontiled_split_sparse_r8g8b8 layout3_nontiled_split_sparse_r8g8b8a8 layout3_tiled_split_sparse_r10g10b10a2 layout3_tiled_split_sparse_r8g8b8 layout3_tiled_split_sparse_r8g8b8a8 layout4_nontiled_nonsplit_random_r5g5b5a1 layout4_nontiled_nonsplit_random_r5g6b5 layout4_nontiled_nonsplit_sparse_r5g6b5 layout4_tiled_nonsplit_random_r5g5b5a1 layout4_tiled_nonsplit_random_r5g6b5 layout4_tiled_nonsplit_sparse_r5g6b5 layout5_nontiled_nonsplit_random_yuv420 layout5_nontiled_nonsplit_random_yuv420_10b layout5_nontiled_nonsplit_sparse_yuv420 layout5_nontiled_nonsplit_sparse_yuv420_10b layout5_tiled_nonsplit_random_yuv420 layout5_tiled_nonsplit_random_yuv420_10b layout5_tiled_nonsplit_sparse_yuv420 layout5_tiled_nonsplit_sparse_yuv420_10b layout6_nontiled_nonsplit_random_yuv422 layout6_nontiled_nonsplit_sparse_yuv422 layout6_tiled_nonsplit_random_yuv422 layout6_tiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "T880-r1p0" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_random_yuv420_10b layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "G71" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_random_yuv420_10b layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422 layout3_nontiled_split_sparse_r10g10b10a2 layout3_nontiled_split_sparse_r8g8b8 layout3_nontiled_split_sparse_r8g8b8a8 layout4_nontiled_nonsplit_random_r5g5b5a1 layout4_nontiled_nonsplit_random_r5g6b5 layout4_nontiled_nonsplit_sparse_r5g6b5 layout5_nontiled_nonsplit_random_yuv420 layout5_nontiled_nonsplit_random_yuv420_10b layout5_nontiled_nonsplit_sparse_yuv420 layout5_nontiled_nonsplit_sparse_yuv420_10b layout6_nontiled_nonsplit_random_yuv422 layout6_nontiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "T880-r2p0" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_random_yuv420_10b layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "GPU" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_tiled_nonsplit_sparse_r5g6b5 layout0_tiled_split_sparse_r10g10b10a2 layout0_tiled_split_sparse_r8g8b8 layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout1_tiled_nonsplit_sparse_yuv420 layout1_tiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_sparse_yuv422 layout2_tiled_nonsplit_sparse_yuv422 layout3_nontiled_split_sparse_r10g10b10a2 layout3_nontiled_split_sparse_r8g8b8 layout3_tiled_split_sparse_r10g10b10a2 layout3_tiled_split_sparse_r8g8b8 layout4_nontiled_nonsplit_sparse_r5g6b5 layout4_tiled_nonsplit_sparse_r5g6b5 layout5_nontiled_nonsplit_sparse_yuv420 layout5_nontiled_nonsplit_sparse_yuv420_10b layout5_tiled_nonsplit_sparse_yuv420 layout5_tiled_nonsplit_sparse_yuv420_10b layout6_nontiled_nonsplit_sparse_yuv422 layout6_tiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "T760-r0px" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_nonsplit_sparse_r5g6b5 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_sparse_yuv420 layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "T880-r0px" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_sparse_yuv420 layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "AnyDecoder" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout0_tiled_nonsplit_random_r10g10b10a2 layout0_tiled_nonsplit_random_r5g5b5a1 layout0_tiled_nonsplit_random_r5g6b5 layout0_tiled_nonsplit_random_r8g8b8 layout0_tiled_nonsplit_random_r8g8b8a8 layout0_tiled_split_sparse_r10g10b10a2 layout0_tiled_split_sparse_r8g8b8 layout0_tiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_random_yuv420_10b layout1_tiled_nonsplit_random_yuv420 layout1_tiled_nonsplit_random_yuv420_10b layout2_nontiled_nonsplit_random_yuv422 layout2_tiled_nonsplit_random_yuv422 layout3_nontiled_split_sparse_r10g10b10a2 layout3_nontiled_split_sparse_r8g8b8 layout3_nontiled_split_sparse_r8g8b8a8 layout3_tiled_split_sparse_r10g10b10a2 layout3_tiled_split_sparse_r8g8b8 layout3_tiled_split_sparse_r8g8b8a8 layout4_nontiled_nonsplit_random_r5g5b5a1 layout4_nontiled_nonsplit_random_r5g6b5 layout4_tiled_nonsplit_random_r5g5b5a1 layout4_tiled_nonsplit_random_r5g6b5 layout5_nontiled_nonsplit_random_yuv420 layout5_nontiled_nonsplit_random_yuv420_10b layout5_tiled_nonsplit_random_yuv420 layout5_tiled_nonsplit_random_yuv420_10b layout6_nontiled_nonsplit_random_yuv422 layout6_tiled_nonsplit_random_yuv422"
  elif [ $CONS_IP = "Gemini" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_nonsplit_sparse_r10g10b10a2 layout0_nontiled_nonsplit_sparse_r5g5b5a1 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_nonsplit_sparse_r8g8b8 layout0_nontiled_nonsplit_sparse_r8g8b8a8 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_random_yuv420_10b layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "T830-r1p0" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_random_yuv420_10b layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "Heimdall" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_random_yuv420_10b layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422 layout3_nontiled_split_sparse_r10g10b10a2 layout3_nontiled_split_sparse_r8g8b8 layout3_nontiled_split_sparse_r8g8b8a8 layout4_nontiled_nonsplit_random_r5g5b5a1 layout4_nontiled_nonsplit_random_r5g6b5 layout4_nontiled_nonsplit_sparse_r5g6b5 layout5_nontiled_nonsplit_random_yuv420 layout5_nontiled_nonsplit_random_yuv420_10b layout5_nontiled_nonsplit_sparse_yuv420 layout5_nontiled_nonsplit_sparse_yuv420_10b layout6_nontiled_nonsplit_random_yuv422 layout6_nontiled_nonsplit_sparse_yuv422"
  elif [ $CONS_IP = "T860-r1p0" ]
    then
    CONS_FMTS="layout0_nontiled_nonsplit_random_r10g10b10a2 layout0_nontiled_nonsplit_random_r5g5b5a1 layout0_nontiled_nonsplit_random_r5g6b5 layout0_nontiled_nonsplit_random_r8g8b8 layout0_nontiled_nonsplit_random_r8g8b8a8 layout0_nontiled_nonsplit_sparse_r5g6b5 layout0_nontiled_split_sparse_r10g10b10a2 layout0_nontiled_split_sparse_r8g8b8 layout0_nontiled_split_sparse_r8g8b8a8 layout1_nontiled_nonsplit_random_yuv420 layout1_nontiled_nonsplit_random_yuv420_10b layout1_nontiled_nonsplit_sparse_yuv420 layout1_nontiled_nonsplit_sparse_yuv420_10b layout2_nontiled_nonsplit_random_yuv422 layout2_nontiled_nonsplit_sparse_yuv422"
  else
    echo Error: invalid consumer IP $CONS_IP
    echo ""
    print_usage
    exit 1
  fi
}

print_usage ()
{
    echo "Usage: $0 [OPTION]... -w WIDTH -h HEIGHT -p PROD_IP"
    echo "  or:  $0 [OPTION]... -w WIDTH -h HEIGHT -c CONS_IP"
    echo "  or:  $0 [OPTION]... -w WIDTH -h HEIGHT -p PROD_IP -c CONS_IP"
    echo "Print commands for generating a set of AFBC conformance test buffers."
    echo ""
    echo "  -w WIDTH,   generate buffers with a width of WIDTH pixels"
    echo "  -h HEIGHT,  generate buffers with a height of HEIGHT pixels"
    echo "  -p PROD_IP, filter formats for PROD_IP as producer IP"
    echo "  -c CONS_IP, filter formats for CONS_IP as consumer IP"
    echo "  -u,         generate buffers without addressing constraints"
    echo "  -r,         generate buffers with payload data in random order, with no copy block restriction"
    echo ""
    echo "  PROD_IP can be one of:"
    print_prod_ips
    echo "  CONS_IP can be one of:"
    print_cons_ips
    echo ""
    exit
}

WIDTH=
HEIGHT=
PROD_IP=Unspecified
CONS_IP=Unspecified
OPT_UNCONSTRAINED_ADDR=0
OPT_RANDOM_ADDR=0

while getopts ":w:h:p:c:ur" opt; do
    case $opt in
        w)
            WIDTH=$OPTARG
            ;;
        h)
            HEIGHT=$OPTARG
            ;;
        p)
            PROD_IP=$OPTARG
            ;;
        c)
            CONS_IP=$OPTARG
            ;;
        u)
            OPT_UNCONSTRAINED_ADDR=1
            ;;
        r)
            OPT_RANDOM_ADDR=1
            ;;
        \?)
          echo "Invalid option: -$OPTARG"
          echo ""
          print_usage
          exit
          ;;
        :)
          echo "Option -$OPTARG requires an argument"
          echo ""
          print_usage
          exit
          ;;
    esac
done

if [ -z "$WIDTH" ] || [ -z "$HEIGHT" ]
    then
    print_usage
    exit 1
fi

if [ "$PROD_IP" = "Unspecified" ] && [ "$CONS_IP" = "Unspecified" ] 
    then
    echo "Not both producer and consumer IP can be left blank."
    echo ""
    print_usage
    exit 1
fi




PIXEL_FORMATS=undef

if [ "$PROD_IP" = "Unspecified" ]
    then
    # sets CONS_FMTS
    lookup_cons_fmts $CONS_IP
    PROD_FMTS=$CONS_FMTS
elif [ "$CONS_IP" = "Unspecified" ]
    then
    # sets PROD_FMTS
    lookup_prod_fmts $PROD_IP
    CONS_FMTS=$PROD_FMTS
else
    # sets PROD_FMTS
    lookup_prod_fmts $PROD_IP
    # sets CONS_FMTS
    lookup_cons_fmts $CONS_IP
fi

# pick out formats that match both source and destination
FMTS=
for PROD_FMT in $PROD_FMTS
  do
  for CONS_FMT in $CONS_FMTS
    do
    if [ "$PROD_FMT" = "$CONS_FMT" ]
        then
        FMTS="$FMTS $CONS_FMT"
    fi
  done
done

if [ -z "$FMTS" ]
    then
    echo "The combination producer: $PROD_IP, consumer: $CONS_IP has no available formats."
    exit 1
fi

# seed=seconds since the Epoch
SEED=$(date +%s)

# generate AFBC buffers for all formats
GEN_CMDS=
DEC_CMDS=
for FMT in $FMTS
  do
  LAYOUT_STR=$(echo $FMT | cut -d "_" -f 1)
  TILED_STR=$(echo $FMT | cut -d "_" -f 2)
  SPLIT_STR=$(echo $FMT | cut -d "_" -f 3)
  ADDRESSING=$(echo $FMT | cut -d "_" -f 4)
  PIX_FMT=$(echo $FMT | cut -d "_" -f 5-)

  if [ "$OPT_UNCONSTRAINED_ADDR" = 1 ]
      then
      ADDRESSING="unconstrained"
  fi
  
if [ "$OPT_RANDOM_ADDR" = 1 ]
      then
      ADDRESSING="random"
  fi


  LAYOUT_NUM=$(echo $LAYOUT_STR | cut -d t -f 2)
  LAYOUT="-j $LAYOUT_NUM"

  IS_WIDE=1
  if [ "$LAYOUT_NUM" = "0" ] || [ "$LAYOUT_NUM" = "1" ] || [ "$LAYOUT_NUM" = "2" ]
  then
      IS_WIDE=0
  fi

  IS_COLOR_TRANS=1
  case "$PIX_FMT" in
      *yuv*)
      IS_COLOR_TRANS=0
      ;;
  esac
  IS_TILED=1
  if [ "$TILED_STR" = "nontiled" ]
      then
      IS_TILED=0
  fi
  IS_SPLIT=1
  if [ "$SPLIT_STR" = "nonsplit" ]
      then
      IS_SPLIT=0
  fi
  PIX_FMT_HDR=$PIX_FMT
  if [ "$PIX_FMT" = "yuv420" ] || [ "$PIX_FMT" = "yuv422" ]
      then
      # append 8b
      PIX_FMT_HDR="$PIX_FMT""8b"
  elif [ "$PIX_FMT" = "yuv420_10b" ] || [ "$PIX_FMT" = "yuv422_10b" ]
      then
      # replace _10b with 10b
      PIX_FMT_HDR=$(echo $PIX_FMT | sed 's/_//')
  fi

  HEADERLESS="$WIDTH""x$HEIGHT""_$PIX_FMT_HDR""_$IS_COLOR_TRANS""_$IS_SPLIT""_0_$LAYOUT_NUM""_$IS_TILED"

  # turn on solid color when tiled is turned on, and vice versa
  SOLID_COLOR_GEN="-S 0"
  SOLID_COLOR_DEC="-1"
  TILED=""
  if [ "$TILED_STR" = "tiled" ]
      then
      SOLID_COLOR_GEN=""
      SOLID_COLOR_DEC=""
      TILED="-T"
  fi
  SPLIT=""
  if [ "$SPLIT_STR" = "split" ]
      then
      SPLIT="-l"
  fi

  BASE_FILENAME="$PIX_FMT""_$LAYOUT_STR""_$TILED_STR""_$SPLIT_STR""_$ADDRESSING""_$WIDTH""x$HEIGHT""_$SEED"

  AFB_FILE="$BASE_FILENAME.afb"
  DEC_FILE="$BASE_FILENAME.bin"

  ALSO_WITHOUT_COPY_RESTRICTION=0

  if [ "$ADDRESSING" = "packed" ]
      then
      # 64 h stripe, 128B alignment of each stripe
      GEN_ADDR="-y 64 -R -t 128"
      DEC_ADDR="-y 64 -R -t 128"
  elif [ "$ADDRESSING" = "sparse" ]
      then
      # header alignment 64B, 128B alignment of payload data
      GEN_ADDR="-r -t 128"
      DEC_ADDR="-r -t 128"
  elif [ "$ADDRESSING" = "unconstrained" ]
      then
      # for 3rd party encoders not using ARM address patterns, no particular constraints on the buffer
      GEN_ADDR=""
      DEC_ADDR=""
  elif [ "$ADDRESSING" = "random" ]
      then
      # random addressing.
      # set stripe height to whole buffer height in linear mode, and to tile height in tiled mode
      STRIPE_HEIGHT=$HEIGHT
      if [ "$IS_TILED" = "1" ] && [ "$IS_WIDE" = "1" ]
      then
          STRIPE_HEIGHT="64"
      elif [ "$IS_TILED" = "1" ] && [ ! "$IS_WIDE" = "1" ]
      then
          STRIPE_HEIGHT="128"
      fi

      GEN_ADDR="-y $STRIPE_HEIGHT -R"
      # also generate no copy block restriction for yuv formats with superblock layout 1 or 2
      if ([ "$LAYOUT_NUM" = 1 ] || [ "$LAYOUT_NUM" = 2 ]) && [ "$IS_TILED" = "0" ]
      then
          case "$PIX_FMT" in
              *yuv*)
              ALSO_WITHOUT_COPY_RESTRICTION=1
              ;;
          esac
      fi
      DEC_ADDR=""
  fi

  GEN_CMD="./afbcgen $TILED $SOLID_COLOR_GEN $SPLIT $LAYOUT $GEN_ADDR -e $SEED -z -q -w $WIDTH -h $HEIGHT -f $PIX_FMT -o $AFB_FILE"
  DEC_CMD="./afbcdec -q -h $HEADERLESS $SOLID_COLOR_DEC -e $DEC_ADDR -i $AFB_FILE -o $DEC_FILE"

  GEN_CMDS="$GEN_CMDS""$GEN_CMD\n"
  DEC_CMDS="$DEC_CMDS""$DEC_CMD\n"

  if [ "$ALSO_WITHOUT_COPY_RESTRICTION" = 1 ]
      then
      GEN_CMD="./afbcgen -0 $TILED $SOLID_COLOR_GEN $SPLIT $LAYOUT $GEN_ADDR -e $SEED -z -q -w $WIDTH -h $HEIGHT -f $PIX_FMT -o no_copy_restriction_$AFB_FILE"
      DEC_CMD="./afbcdec -0 -q -h $HEADERLESS $SOLID_COLOR_DEC -e $DEC_ADDR -i no_copy_restriction_$AFB_FILE -o no_copy_restriction_$DEC_FILE"
      
      GEN_CMDS="$GEN_CMDS""$GEN_CMD\n"
      DEC_CMDS="$DEC_CMDS""$DEC_CMD\n"
  fi


  # increase seed
  SEED=$(echo "$SEED + 1" | bc)
  
done


echo "# The following commands will generate the appropriate encoded buffers:"
echo ""
printf "$GEN_CMDS"

echo ""
echo "# They can then be decoded by the following commands:"
echo ""
printf "$DEC_CMDS"

echo ""
echo "# For an AFBC decoder conformance test, both sets of buffers are needed."
echo "# For an AFBC encoder conformance test, only the decoded buffers are needed."
