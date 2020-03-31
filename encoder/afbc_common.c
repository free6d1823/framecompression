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

#include <memory.h>
#include <assert.h>
#include <stdio.h>
#include "afbc_common.h"

#define max(a,b) ((a)>(b)?(a):(b))

// I-order
// superblock layout 0 box 444
static const struct afbc_block4x4_order g_afbc_block4x4_order_sl_0[16] =
    {{0,4,4},  {0,0,4},  {0,0,0},   {0,4,0},
     {0,8,0},  {0,12,0}, {0,12,4},  {0,8,4},
     {0,8,8},  {0,12,8}, {0,12,12}, {0,8,12},
     {0,4,12}, {0,0,12}, {0,0,8},   {0,4,8}};

// superblock layout 1 box 420
static const struct afbc_block4x4_order g_afbc_block4x4_order_sl_1[20] = 
    {{0,4,4},  {0,0,4},  {1,0,0}, {0,0,0},   {0,4,0},
     {0,8,0},  {0,12,0}, {1,4,0}, {0,12,4},  {0,8,4},
     {0,8,8},  {0,12,8}, {1,4,4}, {0,12,12}, {0,8,12},
     {0,4,12}, {0,0,12}, {1,0,4}, {0,0,8},   {0,4,8}};

// superblock layout 2 box 422
static const struct afbc_block4x4_order g_afbc_block4x4_order_sl_2[24] = 
    {{0,4,4},  {1,0,4}, {0,0,4},  {0,0,0},  {1,0,0}, {0,4,0},
     {0,8,0},  {1,4,0}, {0,12,0}, {0,12,4}, {1,4,4}, {0,8,4},
     {0,8,8},  {1,4,8}, {0,12,8}, {0,12,12},{1,4,12},{0,8,12},
     {0,4,12}, {1,0,12},{0,0,12}, {0,0,8},  {1,0,8}, {0,4,8}};

// superblock layout 3 wide 444 flat
static const struct afbc_block4x4_order g_afbc_block4x4_order_sl_3[16] =
    {{0,0,0},   {0,4,0},  {0,8,0},  {0,12,0}, {0,16,0},  {0,20,0},  {0,24,0},  {0,28,0},
     {0,0,4},   {0,4,4},  {0,8,4},  {0,12,4}, {0,16,4},  {0,20,4},  {0,24,4},  {0,28,4}};

// superblock layout 4 wide 444 16bpp optimized
static const struct afbc_block4x4_order g_afbc_block4x4_order_sl_4[16] =
    {{0,4,4},  {0,0,4},  {0,0,0},   {0,4,0}, {0,8,0},  {0,12,0}, {0,12,4},  {0,8,4},
     {0,20,4}, {0,16,4}, {0,16,0},  {0,20,0}, {0,24,0}, {0,28,0}, {0,28,4},  {0,24,4}};

// superblock layout 5 wide 420
static const struct afbc_block4x4_order g_afbc_block4x4_order_sl_5[20] =
    {{0,4,4},  {0,0,4},  {1,0,0},  {0,0,0},  {0,4,0},
     {0,8,0},  {0,12,0}, {1,4,0},  {0,12,4}, {0,8,4},
     {0,20,4}, {0,16,4}, {1,8,0},  {0,16,0}, {0,20,0},
     {0,24,0}, {0,28,0}, {1,12,0}, {0,28,4}, {0,24,4}};

// superblock layout 6 wide 422
static const struct afbc_block4x4_order g_afbc_block4x4_order_sl_6[24] =
    {{0,4,4},  {1,0,4}, {0,0,4},  {0,0,0},  {1,0,0}, {0,4,0}, {0,8,0},  {1,4,0}, {0,12,0}, {0,12,4}, {1,4,4}, {0,8,4},
     {0,20,4},  {1,8,4}, {0,16,4}, {0,16,0},{1,8,0},{0,20,0}, {0,24,0}, {1,12,0},{0,28,0}, {0,28,4}, {1,12,4},{0,24,4}};


// unused order
static const struct afbc_block4x4_order g_afbc_block4x4_32x4_flat_order_444[16] =
    {{0,0,0},  {0,4,0},  {0,8,0},   {0,12,0},
     {0,16,0}, {0,20,0}, {0,24,0},  {0,28,0},
     {0,0,4},  {0,4,4},  {0,8,4},   {0,12,4},
     {0,16,4}, {0,20,4}, {0,24,4},  {0,28,4}};

static const struct afbc_block4x4_order g_afbc_block4x4_32x4_flat_order_8x2_444[16] =
    {{0,0,0},  {0,8,0},  {0,16,0},  {0,24,0},
     {0,0,2},  {0,8,2},  {0,16,2},  {0,24,2},
     {0,0,4},  {0,8,4},  {0,16,4},  {0,24,4},
     {0,0,6},  {0,8,6},  {0,16,6},  {0,24,6}};

static const struct afbc_block4x4_order g_afbc_block4x4_32x4_flat_order_16x1_444[16] =
    {{0,0,0},  {0,16,0},  {0,0,1},  {0,16,1},
     {0,0,2},  {0,16,2},  {0,0,3},  {0,16,3},
     {0,0,4},  {0,16,4},  {0,0,5},  {0,16,5},
     {0,0,6},  {0,16,6},  {0,0,7},  {0,16,7}};

//flat order
static const struct afbc_block4x4_order g_afbc_block4x4_order_444[16] =
    {{0,0,0},   {0,4,0},  {0,8,0},  {0,12,0},
     {0,12,4},  {0,8,4},  {0,4,4},  {0,0,4},
     {0,0,8},   {0,4,8},  {0,8,8},  {0,12,8},
     {0,12,12}, {0,8,12}, {0,4,12}, {0,0,12}};



const int superblock_width [NUM_SUPERBLOCK_LAYOUTS] = {16, 16, 16, 32, 32, 32, 32};
const int superblock_height[NUM_SUPERBLOCK_LAYOUTS] = {16, 16, 16,  8,  8,  8,  8};
const int sl_subsampling[NUM_SUPERBLOCK_LAYOUTS] = {AFBC_SUBSAMPLING_NONE, 
                                                    AFBC_SUBSAMPLING_420, 
                                                    AFBC_SUBSAMPLING_422, 
                                                    AFBC_SUBSAMPLING_NONE, 
                                                    AFBC_SUBSAMPLING_NONE, 
                                                    AFBC_SUBSAMPLING_420, 
                                                    AFBC_SUBSAMPLING_422};


const struct afbc_block4x4_order *subblock_orders[NUM_SUPERBLOCK_LAYOUTS] = {g_afbc_block4x4_order_sl_0, 
                                                                             g_afbc_block4x4_order_sl_1, 
                                                                             g_afbc_block4x4_order_sl_2, 
                                                                             g_afbc_block4x4_order_sl_3, 
                                                                             g_afbc_block4x4_order_sl_4, 
                                                                             g_afbc_block4x4_order_sl_5,
                                                                             g_afbc_block4x4_order_sl_6};

//log2(x+2)+1
const int log2_table[18] = {-1, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5};

// Initialize the frame_info structure
void afbc_init_frame_info(struct afbc_frame_info *f,
                          int width,
                          int height,
                          int ncomponents,
                          int superblock_layout,
                          int yuv_transform,
                          int tiled,
                          int inputbits[4]) {
    int i, totbits;

    if (superblock_layout == -1) superblock_layout = 0;

    memset(f, 0, sizeof(struct afbc_frame_info));
    f->version=AFBC_VERSION;
    f->width=width;
    f->height=height;
    f->tiled=tiled;
    f->header_row_stride=0;

    f->left_crop=0;
    f->top_crop=0;
    f->mb_sizew = superblock_width [superblock_layout];
    f->mb_sizeh = superblock_height[superblock_layout];
    f->mbw=(width +f->mb_sizew-1)/f->mb_sizew;
    f->mbh=(height+f->mb_sizeh-1)/f->mb_sizeh;

    if (superblock_layout == 8) {
        f->b_sizew = 4;
        f->b_sizeh = 2;
    } else {
        f->b_sizew = 4;
        f->b_sizeh = 4;
    }

    f->superblock_layout = superblock_layout;
    f->yuv_transform = yuv_transform;
    f->sbs_multiplier[0] = 1;
    f->sbs_multiplier[1] = 1;

    f->disable_copy_rows = 0;
    f->disable_copies_crossing_8x8 = 0;
    if (sl_subsampling[superblock_layout]==AFBC_SUBSAMPLING_420 ||
        sl_subsampling[superblock_layout]==AFBC_SUBSAMPLING_422) {
        f->disable_copies_crossing_8x8 = 1;
    }

    f->decode_allow_zeroed_headers = 0;

    f->allow_solid_color = 1;
    f->allow_all_copy_blocks = 0;
    f->only_uncompressed_blocks = 0;

    f->header_align = 0;
    f->least_payload_align = 4096;
    f->found_error = 0;
    f->error_check = 0;

    f->check_first_superblock_tile_align = 0;
    f->check_payload_alignment = 0;
    f->check_payload_offset    = 0;
    f->check_sparse            = 0;
    f->check_stripe_height     = 0;
    f->stripe_bins             = 0;
    f->first_payload_ptr       = ~0;

    f->subblock_order = subblock_orders[superblock_layout];

    for(i=0; i<ncomponents; i++) 
        f->inputbits[i] = inputbits[i];
    if (sl_subsampling[superblock_layout]==AFBC_SUBSAMPLING_NONE) {
        // store all components interleaved in each subblock
        f->nsubblocks=16;
        f->nplanes=1;
        f->ncomponents[0]=ncomponents;
        f->first_component[0]=0;
        f->body_base_ptr_bits=32;
        f->subblock_size_bits=6;
        totbits=0;
        for(i=0; i<ncomponents; i++) 
            totbits+=inputbits[i];
        f->uncompressed_size[0]=(16*totbits+7)>>3;

        for(i=0; i<ncomponents; i++) 
            f->compbits[i]= inputbits[i];
        if (yuv_transform) {
            f->compbits[0] = inputbits[1];
            f->compbits[1] = inputbits[1]+1;
            f->compbits[2] = inputbits[1]+1;
        }
        f->sbs_multiplier[0] = max(1, (2*totbits + 63)/64);
    } else if (sl_subsampling[superblock_layout]==AFBC_SUBSAMPLING_420) {
        // chroma and luma are stored in separate subblocks
        assert(ncomponents==3);
        f->nsubblocks=20;      // 16 luma + 4 chroma
        f->nplanes=2;          // luma and chroma
        f->ncomponents[0]=1;   // Y
        f->ncomponents[1]=2;   // UV
        f->first_component[0]=0;     // Y
        f->first_component[1]=1;     // U
        f->body_base_ptr_bits=28;
        f->subblock_size_bits=5;
        f->uncompressed_size[0]=(16*inputbits[0]+7)>>3;
        f->uncompressed_size[1]=(16*(inputbits[1]+inputbits[2])+7)>>3;
        f->sbs_multiplier[0] = 1;
        if (inputbits[1] <= 8) {
          f->sbs_multiplier[1] = 1;
        } else {
          f->sbs_multiplier[1] = 2;
        }
        for(i=0; i<ncomponents; i++) 
            f->compbits[i]= inputbits[i];
    } else if (sl_subsampling[superblock_layout]==AFBC_SUBSAMPLING_422) {
        // chroma and luma are stored in separate subblocks
        assert(ncomponents==3);
        f->nsubblocks=24;      // 16 luma + 4 chroma
        f->nplanes=2;          // luma and chroma
        f->ncomponents[0]=1;   // Y
        f->ncomponents[1]=2;   // UV
        f->first_component[0]=0;     // Y
        f->first_component[1]=1;     // U
        f->body_base_ptr_bits=32;
        f->subblock_size_bits=4;
        f->uncompressed_size[0]=(16*inputbits[0]+7)>>3;
        f->uncompressed_size[1]=(16*(inputbits[1]+inputbits[2])+7)>>3;
        if (inputbits[1] <= 8) {
          f->sbs_multiplier[0] = 1;
        } else {
          f->sbs_multiplier[0] = 2;
        }
        if (inputbits[1] <= 8) {
          f->sbs_multiplier[1] = 2;
        } else {
          f->sbs_multiplier[1] = 3;
        }
        for(i=0; i<ncomponents; i++) {
          f->compbits[i]= inputbits[i];
        }
    } else {
        assert(0);
    }
    for(i=0; i<4; i++)
        if (i==0 || i==3)
            f->defaultcolor[i] = (1<<f->compbits[i])-1;    // Y and alpha channel
        else
            f->defaultcolor[i] = 1<<(f->compbits[i]-1);    // U and V channels

    for(i=0; i<4; i++) {
        f->actual_display_bytes[i] = 0;
        f->actual_gpu_bytes[i] = 0;
    }
}

uint32_t afbc_mb_round(int mb, int q) {
    return (mb/q + ((mb%q) ? 1 : 0))*q;
}

uint64_t afbc_get_max_frame_size(struct afbc_frame_info *f) {
    uint64_t header_buffer_size;
    uint64_t body_buffer_size;
    uint64_t body_buf_alignment_overhead;

    header_buffer_size = afbc_mb_round(f->mbw, afbc_get_tile_size_x(f)) * 
                         afbc_mb_round(f->mbh, afbc_get_tile_size_y(f)) * 
                         AFBC_HEADER_SIZE;
    body_buffer_size   = afbc_mb_round(f->mbw, afbc_get_tile_size_x(f)) * 
                         afbc_mb_round(f->mbh, afbc_get_tile_size_y(f)) * 
                         afbc_get_max_superblock_payloadsize(f);

    // in tiled mode we also need to add the overhead of aligning the body buffer to a multiple of 4096
    if (f->tiled) {
        uint64_t body_buffer_start;
        // the body buffer starts at the first 4096-aligned address after the header buffer
        body_buffer_start = ((header_buffer_size + 4095) & ~4095);

        body_buf_alignment_overhead = body_buffer_start - header_buffer_size;
    }
    else {
        body_buf_alignment_overhead = 0;
    }

    return header_buffer_size + body_buffer_size + body_buf_alignment_overhead;
}

uint64_t afbc_get_max_superblock_payloadsize(struct afbc_frame_info *f) {
    return (afbc_get_max_superblock_payloadsize_no_rounding(f) + 127) & (~0x7full); // size rounded up to 128B alignment
}

uint64_t afbc_get_max_superblock_payloadsize_no_rounding(struct afbc_frame_info *f) {
    int i;
    uint64_t body_size = 0;
    for(i=0; i<f->nsubblocks; i++)
        body_size+=f->uncompressed_size[(int)f->subblock_order[i].plane];
    return body_size;
}

uint64_t afbc_get_uncompressed_frame_size(struct afbc_frame_info *f) {
    // calculate size of uncompressed data
    return f->mbw * f->mbh * afbc_get_max_superblock_payloadsize_no_rounding(f);
}

uint32_t afbc_get_tile_size_x(struct afbc_frame_info *f) {
    if (!f->tiled && !f->header_row_stride)   return 1;
    if (!f->tiled && f->header_row_stride)    return f->header_row_stride;
    if (f->inputbits[0]+f->inputbits[1]+
        f->inputbits[2]+f->inputbits[3] > 32) return 4;
    else                                      return 8;
}

uint32_t afbc_get_tile_size_y(struct afbc_frame_info *f) {
    if (!f->tiled)                            return 1;
    if (f->inputbits[0]+f->inputbits[1]+
        f->inputbits[2]+f->inputbits[3] > 32) return 4;
    else                                      return 8;
}

uint32_t afbc_get_tile_size_z(struct afbc_frame_info *f) {
    (void) f;
    return 1;
}

// Check if the subblock index is the first in a 8x8
int afbc_first_block_in_8x8(struct afbc_frame_info *f, int blk_idx) {
    if (sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_NONE) {
        return blk_idx % 4 == 0;
    } else if (sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_420) {
        return (blk_idx % 5 == 0) || ((blk_idx-2) % 5 == 0);
    } else {// if (sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_422) {
        return (blk_idx % 6 == 0) || ((blk_idx-1) % 6 == 0);
    }
}
