/*
 * Copyright:
 * ----------------------------------------------------------------------------
 * This confidential and proprietary software may be used only as authorized
 * by a licensing agreement from ARM Limited or its affiliates.
 *      (C) COPYRIGHT 2012-2017 ARM Limited or its affiliates, ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorized copies and
 * copies may only be made to the extent permitted by a licensing agreement
 * from ARM Limited or its affiliates.
 * ----------------------------------------------------------------------------
 */

#ifndef __AFBC_ENCODE_H__
#define __AFBC_ENCODE_H__

#include "afbc_common.h"

// Encode the given 4x4 block to the given buf
// block incices are block[component][y][x]
// if the block is single color, then color is set to that color, otherwise
// color is set to -1. This can be used to check whether a block is identical
// to another preceding block.
// plane indicates which plane to be encoded (used in subsampled formats, 0 otherwise)
// U and V are stored in block[0] and block[1] respectively in this case.
//
// The return value is the number of bytes written to the buffer.
// -1 is returned in case of buffer overflow
int afbc_encode_block4x4(struct afbc_frame_info *info,
                         int plane,
                         unsigned int block[4][16],
                         unsigned char *buf,
                         int bufsize,
                         long long *color,
                         int *uncompressed,
                         int minval_wrap_en,
                         int rgb_defcolor_en);

// Encode the given superblock to the given mbx, mby block coordinate (in block16 units)
// Block incices are block[component][y][x]
// buf is the AFBC frame buffer
// bufsize is the size of buf in byte
// body_pos is a byte offset to where payload data will be written
// The return value is an updated body_pos
// -1 is returned in case of buffer overflow
int afbc_encode_superblock(struct afbc_frame_info *info,
                           unsigned int block[4][256],
                           unsigned char *buf,
                           int bufsize,
                           int mbnum,
                           int body_pos,
                           int minval_wrap_en,
                           int rgb_defcolor_en);

// the extended version of the above function which updates the split_block_pos.
// for standalone afbc testbench to get the position of the first half body
int afbc_encode_superblock_ext(struct afbc_frame_info *f,
                               unsigned int block[4][256],
                               unsigned char *buf,
                               int bufsize,
                               int mbnum,
                               int body_pos,
                               int minval_wrap_en,
                               int rgb_defcolor_en,
                               int* split_block_pos);

// Write fileheader to buffer based on the given frame_info struct
void afbc_write_fileheader(struct afbc_frame_info *f, int frame_size, unsigned char *buf);

int afbc_block_split_block(struct afbc_frame_info *f);
int afbc_block_split_block_alt(struct afbc_frame_info *f);

#endif
