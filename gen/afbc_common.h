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

#ifndef __AFBC_COMMON_H__
#define __AFBC_COMMON_H__

#include <stdint.h>

#define AFBC_VERSION 5
#define AFBC_FILEHEADER_SIZE 32
#define AFBC_HEADER_SIZE 16

#define AFBC_SUBSAMPLING_NONE 0
#define AFBC_SUBSAMPLING_420  1
#define AFBC_SUBSAMPLING_422  2

#define NUM_SUPERBLOCK_LAYOUTS 7

// The subblock order lookup table entry
struct afbc_block4x4_order {
    char plane;
    char x;
    char y;
};

struct stripe_bin_t {
    uint64_t start_addr;
    uint64_t end_addr;
    int remaining_superblocks;
};

//log2(x+2)+1
extern const int log2_table[18];

// AFBC frame level info
// This struct is essentially an expanded version of the AFBC file header
// It controls the operation of the encoder/decoder with regards to
// format details and as well as geometrical information such as width
// and height.
struct afbc_frame_info {
    uint64_t frame_size;                    // total frame size (to check subblocks offset ranges)
    int superblock_layout;                  // which superblock layout is used
    int yuv_transform;                      // apply integer yuv transform before compressing
    int nsubblocks;                         // number of subblocks
    int nplanes;                            // number of planes, 1 or 2. 2 is used for YUV formats
    int ncomponents[2];                     // number of components per plane
    int first_component[2];                 // the index of the first component for the given plane
    int uncompressed_size[2];               // number of byte for an uncompressed block for a given plane
    int body_base_ptr_bits;                 // number of bits for body_base_ptr field
    int subblock_size_bits;                 // number of bits for subblock_size field
    int sbs_multiplier[2];                  // The value to multipie the subblock size with in the header
    int inputbits[4];                       // uncompressed bit depth per component
    int compbits[4];                        // compressed bit depth per component
    int defaultcolor[4];                    // default color per component
    const struct afbc_block4x4_order *subblock_order;   // the subblock order lookup table
    int version;                            // AFBC version
    int width;                              // width in pixels
    int height;                             // height in pixels
    int mbw;                                // width in superblock units
    int mbh;                                // height in superblock units
    int mb_sizew;                           // width of a superblock
    int mb_sizeh;                           // height of a superblock
    int b_sizew;                            // width of the smallest coding unit
    int b_sizeh;                            // height of the innter coding unit (i.e. the 4x4 block in tradfitional AFBC)
    int left_crop;                          // in pixels, <16
    int top_crop;                           // in pixels, <16
    int crop_ignore;                        // flag to ignore the crop and decode whole image
    int header_row_stride;                  // A non AFBC standard feature which enables a header row stride between each
                                            // header row. The header row stride is specified in number of headers per
                                            // header row. A value of 0 indicates that this feature is disabled.
                                            // NOTE: This feature is currently only supported in AFBC encode.
    int tiled;                              // An AFBC 1.2 tiled buffer with header tiles and pagingblocks
    int allow_solid_color;                  // AFBC 1.2 solid color optimization turned on. Only used by gen/encode
    int allow_all_copy_blocks;              // Enabled copy blocks of all pattern, even non-single colored. Only used by encode
    int only_uncompressed_blocks;           // Only generate uncompressed blocks. Only used by encode
    
    int actual_display_bytes[4];            // Keep track of how many bytes a row based display controller
                                            // would have to read to fetch the content in sparse mode
                                            // this is based on a 8 line row buffer
    int actual_gpu_bytes[4];                // Keep track of how many bytes a GPU would have to write given 
                                            // different strides

    int msb_aligned_output;                 // Align output to msb for example 10 bit is shifted 6 bits

    int file_message;                       // Used to indicate non standard properties of this file
    int maximum_allowed_bctree;             // The maximum bctree we allow to decode. 0 means this is limited 
                                            // by  the c implemntation. This can be used to mimic rtl behaviour
    int block_split;                        // the block split mode is enabled for this buffer
    int disable_copy_rows;                  // disable copy blocks between upper and lower half, similar to the split block mode
    int disable_copies_crossing_8x8;        // disable copies crossing the border of 8x8 blocks
    int rtl_addressing;                     //Use addressing with the sparse rtl mode where each payload is spaced away the size of 256 pixels from each other
    int header_align;                       // which alignment is the header buffer expected to be stored at
    int least_payload_align;                // which is the smallest payload alignment found
    int found_error;                        // at least one error was found
    int decode_allow_zeroed_headers;        // Indicates that the decoder should accept a header of all zeroes, and return 0
    int error_check;                        // Enable the checking for a legal AFBC buffers. While some buffers may be decodable
                                            // they may not necessarily be legal afbc buffers
    int check_first_superblock_tile_align;  // check that the first superblock in a paging tile is at the start of the paging tile
    int check_payload_alignment;            // check that payload alignment is as specified in bytes
    int check_payload_offset;               // specify payload offset to be taken in account when checking alignments
    int check_sparse;                       // check that payload buffer is written in sparse mode
    int check_stripe_height;                // check that all payload data is organized in stripes, stripe height in pixels
    struct stripe_bin_t *stripe_bins;       // bins to accumulate stripe info in
    unsigned int first_payload_ptr;         // address of first payload data found
};


// Return the header tile and pagingblock size based on current settings
uint32_t afbc_get_tile_size_x(struct afbc_frame_info *f);
uint32_t afbc_get_tile_size_y(struct afbc_frame_info *f);
uint32_t afbc_get_tile_size_z(struct afbc_frame_info *f);
uint32_t afbc_mb_round(int mb, int q);

// Return max (i.e. uncompressed) compressed frame size in byte.
uint64_t afbc_get_max_frame_size(struct afbc_frame_info *f);

// Return max (i.e. uncompressed) superblock payload size in byte.
uint64_t afbc_get_max_superblock_payloadsize(struct afbc_frame_info *f);
uint64_t afbc_get_max_superblock_payloadsize_no_rounding(struct afbc_frame_info *f);
int afbc_first_block_in_8x8(struct afbc_frame_info *f, int blk_idx);

// Return uncompressed frame size in bytes
uint64_t afbc_get_uncompressed_frame_size(struct afbc_frame_info *f);

// Initialize the frame_info structure from basic info
void afbc_init_frame_info(struct afbc_frame_info *f,
                          int width,
                          int height,
                          int ncomponents,
                          int superblock_layout,
                          int yuv_transform,
                          int tiled,
                          int inputbits[4]);


extern const int superblock_width [NUM_SUPERBLOCK_LAYOUTS];
extern const int superblock_height[NUM_SUPERBLOCK_LAYOUTS];
extern const int superblock_depth[NUM_SUPERBLOCK_LAYOUTS];
extern const int sl_subsampling[NUM_SUPERBLOCK_LAYOUTS];

#endif
