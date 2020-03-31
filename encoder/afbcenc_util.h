/*
 * Copyright:
 * ----------------------------------------------------------------------------
 * This confidential and proprietary software may be used only as authorized
 * by a licensing agreement from ARM Limited or its affiliates.
 *      (C) COPYRIGHT 2016-2017 ARM Limited or its affiliates, ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorized copies and
 * copies may only be made to the extent permitted by a licensing agreement
 * from ARM Limited or its affiliates.
 * ----------------------------------------------------------------------------
 */

#ifndef __AFBCENC_UTIL_H__
#define __AFBCENC_UTIL_H__

#include "afbc_common.h"
#include "afbc_encode.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

void fatal_error(const char *format, ...);

// Loop over all superblocks in the frame and encode them.
// Return size of encoded buffer
int encode_blocks(struct afbc_frame_info *f, unsigned int *planes[4], unsigned char *buf, int bufsize, int *compdata_size);

// Encode the given frame and write to file.
// Return size of encoded buffer
int encode_frame(struct afbc_frame_info *f, unsigned int *planes[4], int skip_fileheader, FILE *outfile);

#endif
