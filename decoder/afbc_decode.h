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

#ifndef __AFBC_DECODE_H__
#define __AFBC_DECODE_H__

#include "afbc_common.h"

// Decode the 4x4 block from the given buf
// block incices are block[component][y][x]
// plane should be set to the plane associated with the subblock (luma=0, chroma=1)
// The return value is the number of bytes read from the buffer.
// This should correspond to the subblock_size written in the header.
// -1 is returned in case of buffer overflow
int afbc_decode_block4x4(struct afbc_frame_info *f,
                         int plane,
                         unsigned char *buf,
                         int bufsize,
                         int uncompressed,
                         unsigned int block[4][4][4],
                         int dbg_sb_idx); // Subblock index for debug.

// Parse the given header block. The header_buf should
// be of size AFBC_HEADER_SIZE.
// Output in the body base pointer, offset, size and uncompressed arrays the
// corresponding data for all subblocks for the given
// header. The number of subblocks are given by f->nsubblocks.
// The return value is the total size in bytes for the corresponding encoded data.
int afbc_parse_header_block(struct afbc_frame_info *f,
                            unsigned char *header_buf,
                            unsigned int *body_base_ptr,
                            uint64_t *subblock_offset,
                            unsigned char *subblock_size,
                            unsigned char *subblock_uncompressed);

                            
// Parse the header and calculate the subblock offset and size for the 
// given subblock index.
// The offset is returned. This is the offset from the start of the
// compressed frame.
// *size will contain the size in byte of the subblock
// *uncompress is set to 1 if the subblock is stored as uncompressed, otherwise 0.
uint64_t afbc_get_subblock_offset(struct afbc_frame_info *f,
                                      unsigned char *header_buf,
                                      int subblock_index,
                                      int *size,
                                      int *uncompressed);

// Decode the given superblock to the given header and body buffer.
// mbx an mby in 16 pixel units
// buf contains the AFBC compressed data, header and body
// Block incices are block[component][y][x]
int afbc_decode_superblock(struct afbc_frame_info *f,
                           unsigned char *buf,
                           int bufsize,
                           int mbx,
                           int mby,
                           unsigned int block[4][256]);

// Parse the fileheader in buf. The size of buf should be AFBC_FILEHEADER_SIZE
// Output frame_info struct and the frame_size.
// Returns 0 if successful.
int afbc_parse_fileheader(unsigned char *buf, struct afbc_frame_info *f, uint64_t *frame_size);

// Print that conformance test failed
void report_conformance_failure();


#endif
