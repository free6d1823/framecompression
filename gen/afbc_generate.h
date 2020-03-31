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

#ifndef __AFBC_GENERATE_H__
#define __AFBC_GENERATE_H__

#include "afbc_common.h"

struct afbc_generate_cfg {
    int randomize_mem;
    int solid_probability;     // probability that a *super*block is solid color
    int copy_probability;      // probability that a *sub*block is copy
    int force_unc_probability; // probability that a *sub*block is uncompressed
    
    int force_payload_in_header;
    
    int force_transition;

    int stat_copy;
    int stat_unc;
    
    int force_malformed_header_size_prob;

    int stat_size[65];

    int rtl_addressing;
    
    int force_first_copy_blk;

    // this will override settings on copy block or uncompressed for a tile
    // if those options are not specified specifically
    int allow_supertile_guiding;

    int payload_offset;
    int payload_alignment;
    int stripe_height;
    int randomize_stripe; // randomize encode order of superblocks inside a stripe
};

// generate a block with random syntax elements
int afbc_generate_superblock(struct afbc_frame_info *f,
                             struct afbc_generate_cfg *gcfg,
                             unsigned char *buf,
                             int bufsize,
                             int mbnum,
                             int body_pos,
                             int force_unc,
                             int force_transition);

// Write fileheader to buffer based on the given frame_info struct
void afbc_write_fileheader(struct afbc_frame_info *f, int frame_size, unsigned char *buf);

int afbc_block_split_block(struct afbc_frame_info *f);
int afbc_block_split_block_alt(struct afbc_frame_info *f);

#endif
