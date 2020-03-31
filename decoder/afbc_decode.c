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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include "afbc_common.h"
#include "afbc_decode.h"

void fatal_error(const char *format, ...);

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

// Find child i from parent in a tree vector representation
#define child(parent,i) (4*(parent)+1+(i))

//#define TREE_DEBUG

//----------------------------------------------------------------------------
#define AFBC_DECODE_PRINT_PREFIX "AFBC_DECODE: "
#define printf_prefix(...) printf (AFBC_DECODE_PRINT_PREFIX __VA_ARGS__)
//----------------------------------------------------------------------------


// read bits from the given buf at the given bitpos
// bufsize in byte
// update the bitpos or -1 in case of buffer overflow
// return data read
static unsigned int read_bits(unsigned char *buf, int bufsize, int *bitpos, int bits) {
    unsigned int value=0, i=0;
    if (*bitpos==-1 || *bitpos+bits>8*bufsize) {
        *bitpos=-1;
        return 0; // outside buffer
    }
    //printf_prefix("read_bits: pos=%d bits=%d ", *bitpos, bits);
    while(bits) {
        int eat=min(bits,(8-(*bitpos&7)));
        value|=((buf[*bitpos/8]>>(*bitpos&7))&((1<<eat)-1))<<i;
        *bitpos+=eat;
        bits-=eat;
        i+=eat;
    }
    //printf_prefix("data=%08x bufsize=%d\n", value, bufsize);
    return value;
}

static int read_bits_signed(unsigned char *buf, int bufsize, int *bitpos, int bits) {
    int value=read_bits(buf, bufsize, bitpos, bits);
    // Sign extend
    return (value<<(8*sizeof(int)-bits))>>(8*sizeof(int)-bits);
}

unsigned int accum_unc;
unsigned int accum_b3;
unsigned int accum_b4;
unsigned int accum_m2;
unsigned int accum_m3;
unsigned int accum_m4;
unsigned int accum_used_tree_bits;
unsigned int accum_needed_tree_bits;

static unsigned int bitcount_u(unsigned int a) {
    int i;
    for(i=31; i>=0; i--)
        if ((a>>i)&1)
            return i+1;
    return 0;
}

/*
static unsigned int bitcount_s(int a) {
    if (a < 0)  {
        return 1 + bitcount_u(-a);
     } else {
        return 1 + bitcount_u(a);
    }
}
*/

// Read bits for a mintree node (corresponding to "node_decode" in the spec)
static int read_mintree_node(struct afbc_frame_info *f, 
                             int *bctree, 
                             int parent, 
                             unsigned char *buf, 
                             int bufsize, 
                             int bitpos, 
                             int maximum_allowed_bctree,
                             unsigned int *mintree) { // Output: mintree

    int i,j;
    int zcd; // 2 bit zero code.

    if (bctree[parent]==1) {
        if (parent == 0) { accum_m3 += 4; }
        else { accum_m4 += 4; }

        for(i=0; i<4; i++) {
            // Read 1 bit from the stream. This is added to the mintree.
            mintree[child(parent,i)]=read_bits(buf, bufsize, &bitpos, 1);
        }
        // Check for error: mintree nodes should always have at least one 0
        if (f->error_check) {
            if ((mintree[child(parent,0)] == 1) && 
                (mintree[child(parent,1)] == 1) && 
                (mintree[child(parent,2)] == 1) && 
                (mintree[child(parent,3)] == 1)) {
                f->found_error = 1;
                printf("ERROR: This is not a legal AFBC buffer, at least one mintree child out of 4 must be 0\n");
            }
        }
    } else if (bctree[parent]>1) {
        if (parent == 0) { accum_m3 += 2+3*bctree[parent]; }
        else { accum_m4 += 2+3*bctree[parent]; }

        zcd=read_bits(buf, bufsize, &bitpos, 2);
        for(j=0; j<4; j++)
            mintree[child(parent,j)]=0;
        // this "if" is to mimic RTL behaviour where not enough bits may be available to decode
        // an illegal mintree:
        //  mintree diff values are only used if bctree is in the valid range
        if ((maximum_allowed_bctree == 0) || (bctree[parent] <= maximum_allowed_bctree)) {
            for(i=0; i<bctree[parent]; i++) {
                for(j=0; j<4; j++) {
                    if (j!=zcd) {
                        mintree[child(parent,j)]|=read_bits(buf, bufsize, &bitpos, 1)<<i;
                    }
                }
            }
            {
                int opposite = (~zcd)&3;
                int other_a;
                int other_b;
                if (zcd == 0 || zcd == 3) {
                    other_a = 1;
                    other_b = 2;
                } else {
                    other_a = 0;
                    other_b = 3;
                }
                accum_used_tree_bits += 3*bctree[parent];
                accum_needed_tree_bits += bitcount_u(mintree[child(parent,opposite)]) + 
                                          bitcount_u(mintree[child(parent,other_a)]) + 
                                          bitcount_u(mintree[child(parent,other_b)]);
                //printf("zcd: %d, opposite: %d, other_a: %d, other_b: %d\n", zcd, (~zcd)&3, other_a, other_b);
                //int zcd_bits = bitcount_u(mintree[child(parent,(~zcd)&3)]);
                //int corr_bits = bitcount_s(mintree[child(parent,(~zcd)&3)]-(mintree[child(parent,other_a)]+mintree[child(parent,other_b)]));
                //printf("so opposite of zcd: %d (b: %d), predicted was: %d (b: %d), correction: %d (b: %d)\n", mintree[child(parent,(~zcd)&3)], bitcount_u(mintree[child(parent,(~zcd)&3)]), mintree[child(parent,other_a)]+mintree[child(parent,other_b)], bitcount_u(mintree[child(parent,other_a)]+mintree[child(parent,other_b)]), mintree[child(parent,(~zcd)&3)]-(mintree[child(parent,other_a)]+mintree[child(parent,other_b)]), bitcount_s(mintree[child(parent,(~zcd)&3)]-(mintree[child(parent,other_a)]+mintree[child(parent,other_b)])));
                //printf(":%d\n", zcd_bits-corr_bits);
                
            } 
        } else {
            read_bits(buf, bufsize, &bitpos, 3*bctree[parent]);
        }
    }
    return bitpos;
}

// Read body data for a subblock encoded in mintrees and bctrees from the given buf
// Return the size in byte or -1 in case of buffer overflow
static int read_compressed_subblock(struct afbc_frame_info *f, 
                                    int plane, 
                                    unsigned char *buf, // Encoded subblock
                                    int bufsize, 
                                    // First dimension is component in plane.
                                    // Second dimension is:
                                    //   [0]     = mintree_l2
                                    //   [1..4]  = mintree_l3 - mintree_l2
                                    //   [5..20] = mintree_l4 - mintree_l3
                                    unsigned int mintrees[4][21],
                                    int dbg_sb_idx) { // Subblock index for debug.
#ifdef TREE_DEBUG
    int k;
#endif
    int i, j, bitpos=0;
    int ncomp=f->ncomponents[plane];
    // Bit count tree.
    // First dimension is component.
    // Second dimension is:
    //   [0]    = bctree_l3
    //   [1..4] = bctree_l4
    int bctrees[4][5]; // 1+4=5
    int fcomp = f->first_component[plane];

#ifndef TREE_DEBUG
    (void) dbg_sb_idx;
#endif

    for (i = 0; i < ncomp; i++) {
        // Bitcount root
        accum_b3 += log2_table[f->compbits[fcomp+i]];
        bctrees[i][0] = read_bits(buf, bufsize, &bitpos,  log2_table[f->compbits[fcomp+i]]);
        if (bctrees[i][0]==((1<<log2_table[f->compbits[fcomp+i]])-1))
            bctrees[i][0]=-1;
        else if (bctrees[i][0]==((1<<log2_table[f->compbits[fcomp+i]])-2))
            bctrees[i][0]=-2;   // use default color
#ifdef TREE_DEBUG
        printf_prefix("sb_idx %d bctree_l3[%d]=%d\n", dbg_sb_idx, i, bctrees[i][0]);
        printf_prefix("BITPOS %d (0x%x) , bufsize %d (0x%x) line %d\n", 
                      bitpos, bitpos,bufsize,bufsize, __LINE__);
#endif
    }
    
    // Calculate bc_tree_l4.
    for (i = 0; i < ncomp; i++) {
        // Check if bctree_l3 is >= 0.
        if (bctrees[i][0]>=0) {
            // Bitcount tree leaves
            for (j = 0; j < 4; j++) {
                // bctree_l4 = bctree_l3 + bctree_l4_diff[j]
                accum_b4 += 2;
                bctrees[i][child(0,j)] = bctrees[i][0] + read_bits_signed(buf, bufsize, &bitpos, 2);
#ifdef TREE_DEBUG
                printf_prefix("BITPOS %d (0x%x) , bufsize %d (0x%x) line %d\n", 
                              bitpos, bitpos,bufsize,bufsize, __LINE__);
                printf_prefix("bctree_l4[%d][%d]=%d\n", i, child(0,j), bctrees[i][child(0,j)]);
#endif
                // Error check: a bctree l4 node must always be 0 or above
                if (f->error_check) {
                    if (bctrees[i][child(0,j)] < 0) {
                        f->found_error = 1;
                        printf("ERROR: This is not a legal AFBC buffer, bctree_l4 should be 0 or more\n");
                    }
                }

                // In RTL bctree_l4 nodes are always positive. Here we mimic this behaviour.
                // NOTE: according to the AFBC format spec. bctree_l4 nodes should
                // always be >= 0 to start with, but we do this to handle invalid data
                // streams the same in model and RTL.
                if (bctrees[i][child(0,j)] < 0) 
                    bctrees[i][child(0,j)] += 16;
                else
                    bctrees[i][child(0,j)] &= 0xf;
            }
        }
    }

    // Error check: the bctree must not resolve to value where any node is larger than the size of a
    // transformed compbits of the component
    if (f->error_check) {
        for (i = 0; i < ncomp; i++) {
            if (bctrees[i][0]>=0) {

                for (j = 0; j < 5; j++) {
                    if (bctrees[i][j] > f->compbits[i]) {
                        f->found_error = 1;
                        printf("ERROR: This is not a legal AFBC buffer, one mintree node is larger than the bitdepth of its component %d %d\n", bctrees[i][j], f->compbits[i]);
                    }
                }
            }
        }
    }

    memset(mintrees, 0, 4*21*sizeof(unsigned int));
    // Calculate mintree_l2.
    for (i = 0; i < ncomp; i++) {
        // Mintree root = mintree_l2.
        // Number of bits used for each component in the algorithm.
        int compbits = f->compbits[f->first_component[plane]+i];
        if (bctrees[i][0] != -2) {
            mintrees[i][0] = read_bits(buf, bufsize, &bitpos, compbits);
            accum_m2 += compbits;
        }
        else // -2 => Use default color.
            mintrees[i][0] = f->defaultcolor[f->first_component[plane]+i];
#ifdef TREE_DEBUG
            printf_prefix("mintree_l2[%d]=%d\n", i, mintrees[i][0]);
            printf_prefix("BITPOS %d (0x%x) , bufsize %d (0x%x) line %d\n", 
                          bitpos, bitpos,bufsize,bufsize, __LINE__);
#endif
    }
    
    // Calculate 2x2 mintree nodes = mintree_l3[0..3] - mintree_l2
    for (i = 0; i < ncomp; i++)
        if (bctrees[i][0] >= 0)
        {
            // Corresponds to "node_decode(l3)" in the spec.
            bitpos = read_mintree_node(f, bctrees[i], 
                                       0, // Parent
                                       buf, bufsize, bitpos, f->maximum_allowed_bctree,
                                       mintrees[i]); // Output : mintrees[1..4] =
                                                     // mintree_l3[0..3] - mintree_l2 = 
#ifdef TREE_DEBUG
            for(k = 0; k < 4; k++)
                printf_prefix("mintree_l3[%d][%d]=%d\n", i, child(0,k), mintrees[i][0] + mintrees[i][child(0,k)]);
            printf_prefix("BITPOS %d (0x%x) , bufsize %d (0x%x) line %d\n", 
                          bitpos, bitpos,bufsize,bufsize, __LINE__);
#endif
        }
        
    // 1x1 mintree nodes = mintree_l4[0..15] - mintree_l3[]
    for (j = 0; j < 4; j++)
        for (i = 0; i < ncomp; i++)
            if (bctrees[i][0] >= 0)
            {
                // Corresponds to "node_decode(l4)" in the spec.
                bitpos = read_mintree_node(f, bctrees[i], 
                                           child(0,j),   // Parent
                                           buf, bufsize, bitpos, f->maximum_allowed_bctree,
                                           mintrees[i]); // Output : mintrees[]
#ifdef TREE_DEBUG
                for(k=0; k<4; k++)
                    printf_prefix("mintree_l4[%d][%d]=%d\n", i, child(child(0,j),k), mintrees[i][0] + mintrees[i][child(0,j)] + mintrees[i][child(child(0,j),k)]);
                printf_prefix("BITPOS %d (0x%x) , bufsize %d (0x%x) line %d\n", 
                              bitpos, bitpos,bufsize,bufsize, __LINE__);
#endif
            }
    if (bitpos==-1)
        return -1;
    return (bitpos+7)>>3; // byte align
}


// Read uncompressed body data for the given subblock
// Return the size in byte or -1 in case of buffer underrun
static int read_uncompressed_subblock(struct afbc_frame_info *f, 
                                      int plane, 
                                      unsigned char *buf, 
                                      int bufsize, 
                                      unsigned int block[4][4][4]) {
    int i, x, y, bitpos=0;
    
    // read as uncompressed
    for(y=0; y<4; y++)
        for(x=0; x<4; x++)
            for(i=0; i<f->ncomponents[plane]; i++) {
                int bits=f->inputbits[f->first_component[plane]+i];
                block[i][y][x] = read_bits(buf, bufsize, &bitpos, bits);
                accum_unc += bits;
            }
    if (bitpos==-1)
        return -1;
    return (bitpos+7)>>3; // byte align
}

// recursively decode the tree mintree to the given block
// the stride of the block is (1<<maxdepth)
// for 4x4 blocks: call with depth=0, maxdepth=2, x=0, y=0, parent=0, rootval=0
static void tree_decode(int depth, 
                        int maxdepth, 
                        int x, 
                        int y, 
                        int parent, 
                        unsigned int rootval, 
                        unsigned int *mintree, 
                        int compbits, 
                        unsigned int *block) {
    int i;
    //printf_prefix("depth=%d, x=%d, y=%d, parent=%d rootval=%02x mintree=%02x\n", depth, x, y, parent, rootval, mintree[parent]);
    if (depth == maxdepth)
        block[y*(1<<maxdepth) + x] = (rootval + mintree[parent]) & ((1<<compbits)-1);
    else
        for (i = 0; i < 4; i++)
            tree_decode(depth+1, 
                        maxdepth, 
                        2*x+(i&1), 
                        2*y+((i>>1)&1), 
                        child(parent,i), 
                        rootval + mintree[parent], 
                        mintree, 
                        compbits, 
                        block);
}

// Apply the integer YUV transform on the given block.
// block incices are block[component][y][x]
static void apply_yuv_transform(struct afbc_frame_info *info, unsigned int block[4][4][4]) {
    int x, y, f=(1<<info->inputbits[1]);
    //printf_prefix("YUV: %02x %02x %02x\n", block[0][0][0], block[1][0][0], block[2][0][0]);
    for(y=0; y<4; y++)
        for(x=0; x<4; x++) {
            unsigned int R, G, B, Y, U, V;
            Y=block[0][y][x];
            U=block[1][y][x];
            V=block[2][y][x];
            G=((unsigned int)(4*Y-U-V+3+2*f))>>2;
            R=U+G-f;
            B=V+G-f;
            // check if the yuv values was constructed in such a way that they don't convert
            // to legal RGB values.
            if((R >= ((unsigned int) 1 << info->inputbits[0])) ||
               (G >= ((unsigned int) 1 << info->inputbits[1])) ||
               (B >= ((unsigned int) 1 << info->inputbits[2]))) {
                if (info->error_check) {
                    report_conformance_failure();
                }
                fatal_error("ERROR: YUV values produces rgb values out of range: YUV %d %d %d -> rgb %d %d %d\n", Y, U, V, R, G, B);
            }
            
           
            block[0][y][x]=R;
            block[1][y][x]=G;
            block[2][y][x]=B;
            // Alpha channel is left unchanged

        }
    //printf_prefix("RGB: %02x %02x %02x\n", block[0][0][0], block[1][0][0], block[2][0][0]);
}

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
                         int dbg_sb_idx) { // Subblock index for debug.
    int size;
    if (uncompressed) {
        size=read_uncompressed_subblock(f, plane, buf, bufsize, block);
    } else {
        unsigned int mintrees[4][21];  // 1+4+16==21
        int i;
        
        
        // Parse body data. Write to mintree.
        size = read_compressed_subblock(f, plane, buf, bufsize, mintrees, dbg_sb_idx);
        
        for (i = 0; i < f->ncomponents[plane]; i++) {
            int compbits = f->compbits[f->first_component[plane]+i];
            tree_decode(0,    // Depth
                        2,    // Maxdepth
                        0, 0, // x, y
                        0,    // Parent
                        0,    // Rootval
                        mintrees[i], 
                        compbits, 
                        (unsigned int*)block[i]);
        }
                
        if (f->yuv_transform)
            apply_yuv_transform(f, block);
    }

    return size;
}

// Check if block i is the last block before a split block
// and the options is enabled
int afbc_is_last_block_before_split(struct afbc_frame_info *f,
                                    int i) {
    if (!f->block_split) return 0;

    if (sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_NONE) {
        return (i == 7);
    } else if (sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_420) {
        return (i == 9);
    } else {// if (sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_422) {
        return (i == 11);
    }
}


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
                            unsigned char *subblock_uncompressed) {
    unsigned int total_size = 0;
    int bitpos=0, i, size, plane;
    unsigned int last_byte_offset;
    uint64_t offset=0;
    int prev_index[2]={-1, -1};
    int solid_color_block = 0;
    subblock_offset[0]=0;
    subblock_offset[1]=0; // YUV422 chroma
    subblock_offset[2]=0; // YUV420 chroma
    subblock_size[0]=0;
    subblock_size[1]=0; // YUV422 chroma
    subblock_size[2]=0; // YUV420 chroma
    
    *body_base_ptr=read_bits(header_buf, 16, &bitpos, f->body_base_ptr_bits);

    // Cumulative sum of subblock_size
    for(i=0; i<f->nsubblocks; i++) {
        plane=f->subblock_order[i].plane;
        if (prev_index[plane] == -1)
            prev_index[plane] = i;
        size=read_bits(header_buf, 16, &bitpos, f->subblock_size_bits);

        if (i == 0 && size == 0 && f->decode_allow_zeroed_headers) {
            solid_color_block = 1;
        }

        // Check for errors. A subblock may never be larger than an uncompressed block would be
        if (f->error_check && !solid_color_block) {
            if (size*f->sbs_multiplier[plane] >= f->uncompressed_size[plane]) {
                f->found_error = 1;
                printf("ERROR: This is not a legal AFBC buffer, the size of subblock %d is %d maximum allowed %d\n", i, size*f->sbs_multiplier[plane], f->uncompressed_size[plane]);
            }
            if (size == 0 && f->disable_copies_crossing_8x8 && afbc_first_block_in_8x8(f, i)) {
                f->found_error = 1;
                printf("ERROR: This is not a legal AFBC buffer, subblock %d not allowed to be a copy block\n", i);
            }
        }

        subblock_uncompressed[i] = size==1;
        if (subblock_uncompressed[i])
            size=f->uncompressed_size[plane];

        if (size==0) {
            subblock_offset[i]=subblock_offset[prev_index[plane]];
            subblock_size[i]=subblock_size[prev_index[plane]];
            subblock_uncompressed[i]=subblock_uncompressed[prev_index[plane]];
        } else {
            subblock_offset[i]=*body_base_ptr+offset;
            if (subblock_uncompressed[i]) {
                subblock_size[i]=size;
            } else {
                subblock_size[i]=size*f->sbs_multiplier[plane];
            }
            prev_index[plane]=i;
            offset+=subblock_size[i];
            total_size += subblock_size[i];
        }

        if (afbc_is_last_block_before_split(f, i)) {
            offset = afbc_get_max_superblock_payloadsize(f)/2;
        }
    }

    // Check for error: the last read byte of a frame must never be >= 2^32 bytes away from the start of the header_buffer
    if (f->error_check) {
        last_byte_offset = total_size + *body_base_ptr;
        // check if upper bit flipped from 1 to 0, to avoid integer size issues
        if (((0x80000000 & *body_base_ptr) == 0x80000000) && ((0x80000000 & last_byte_offset) != 0x80000000)) {
            f->found_error = 1;
            printf("ERROR: This is not a legal AFBC buffer, all payload data needs to be within 2^32 bytes from the start of the header buffer. Some payload data straddles the end of the 2^32 byte region: payload data start: 0x%x, size: 0x%x\n", *body_base_ptr, total_size);
        }

        if (subblock_size[0] == 0) {
            // For a solid color block the offset is expected to be 0
            if (*body_base_ptr != 0) {
                f->found_error = 1;
                printf("ERROR: The header indicates a solid color block but the payload offset is not 0.\n");
            }

            if (!((f->superblock_layout == 0) ||
                  (f->superblock_layout == 3) ||
                  (f->superblock_layout == 4))) {
                f->found_error = 1;
                printf("ERROR: Solid color blocks is only allowed for non subsampled superblock layouts.\n");
            }
        }
    }

    return total_size;
}


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
                                      int *uncompressed) {
    unsigned int body_base_ptr;
    uint64_t subblock_offset[24];
    unsigned char subblock_size[24];
    unsigned char subblock_uncompressed[24];
    afbc_parse_header_block(f, header_buf, &body_base_ptr, subblock_offset, subblock_size, subblock_uncompressed);
    *size=subblock_size[subblock_index];
    *uncompressed=subblock_uncompressed[subblock_index];
    
    return subblock_offset[subblock_index];
}

// Decode the given superblock to the given header and body buffer.
// mbx an mby in 16 pixel units
// buf contains the AFBC compressed data, header and body
// Block incices are block[component][y][x]
int afbc_decode_superblock(struct afbc_frame_info *f,
                            unsigned char *buf,
                            int bufsize,
                            int mbx,
                            int mby,
                            unsigned int block[4][256]) {
    int x, y, j, i, plane, size;
    unsigned int subblock[4][4][4];
    unsigned char *header_buf;
    unsigned int body_base_ptr;
    uint64_t subblock_offset[24];
    unsigned char subblock_size[24];
    unsigned char uncompressed[24];
    int sbsm;
    int tsx = afbc_get_tile_size_x(f);
    int tsy = afbc_get_tile_size_y(f);

    int ptilesize = tsx*tsy;
    int tile  = ((mby/tsy * afbc_mb_round(f->mbw, tsx)/tsx) + mbx/tsx);

    // assuming no interleave with depth since this is not standardized for AFBC yet
    int swiz = (f->superblock_layout >= 3 ? 
                // yxyyxx for 32x8 layouts
                ((((mbx&3) << 0) |
                  ((mby&3) << 2) |
                  ((mbx&4) << 2) |
                  ((mby&4) << 3))) :
                // yxyxyxyx for 16x16 layouts
                ((((mbx&1) << 0) |
                  ((mby&1) << 1) |
                  ((mbx&2) << 1) |
                  ((mby&2) << 2) |
                  ((mbx&4) << 2) |
                  ((mby&4) << 3)))) & (ptilesize-1);
    
    int mbnum = tile*ptilesize | swiz;

    // Parse header
    header_buf=buf + mbnum * AFBC_HEADER_SIZE;
    afbc_parse_header_block(f, header_buf, &body_base_ptr, subblock_offset, subblock_size, uncompressed);

    // If the first subblock_size is 0 we have a solid color superblock
    // this is true for all superblock layouts
    if (subblock_size[0] == 0) {
        int bitptr = 64;
        if (!f->decode_allow_zeroed_headers) {
            fatal_error("ERROR: Header block for superblock mbx=%d mby=%d is all zeroes; not a valid header in AFBC version < 1.2.\n", mbx, mby);
        }

        for (plane = 0; plane < f->nplanes; ++plane) {
            for(j=0; j<f->ncomponents[plane]; j++) {
                int xx,yy;
                int val;
                int k=f->first_component[plane]+j;
                // read the value form the header
                val = read_bits(header_buf, 16, &bitptr, f->inputbits[k]);
                for(yy=0; yy<f->mb_sizeh; yy++)
                    for(xx=0; xx<f->mb_sizew; xx++) {
                        block[k][(yy)*f->mb_sizew+xx]=val;
                    }
            }
        }

        return 0;
    }

    if (f->tiled) {
        // for tiled formats the buffers has to reside within a paging block based on the size of the header tile

        uint64_t start_of_body_buf;
        uint64_t byte_size_of_paging_tile;

        uint64_t start_of_paging_tile;
        uint64_t past_end_of_paging_tile;

        uint64_t start_of_superblock;
        uint64_t past_end_of_superblock;

        uint64_t superblock_size;


        superblock_size = afbc_get_max_superblock_payloadsize(f);

        // the body buffer starts at the address immediately after the header buffer, rounded up to a multiple of 4096
        start_of_body_buf = afbc_mb_round(f->mbh, tsy) * afbc_mb_round(f->mbw, tsx) * AFBC_HEADER_SIZE;
        start_of_body_buf = (start_of_body_buf + 4095) & ~4095;

        // byte size of paging tile is the number of superblocks in a tile multiplied by the byte size of a superblock
        byte_size_of_paging_tile = ptilesize*superblock_size;

        // then calculate the valid range of a tile
        start_of_paging_tile    = start_of_body_buf + tile       * byte_size_of_paging_tile;
        past_end_of_paging_tile = start_of_body_buf + (tile + 1) * byte_size_of_paging_tile;

        // possible byte range for this superblock
        start_of_superblock = body_base_ptr;
        past_end_of_superblock = body_base_ptr + superblock_size;

        if (start_of_superblock < start_of_paging_tile || past_end_of_superblock > past_end_of_paging_tile) {
            fatal_error("ERROR: Superblock %d, %d is outside its paging tile. Valid range of the paging tile is %llu-%llu, but superblock spans %llu-%llu.\n",
                        mbx, mby,
                        (long long unsigned int)start_of_paging_tile, (long long unsigned int)(past_end_of_paging_tile-1),
                        (long long unsigned int)start_of_superblock, (long long unsigned int)(past_end_of_superblock-1));
        }

        // optional first superblock alignment check for tiled formats
        if (f->error_check && f->check_first_superblock_tile_align && (mbnum % ptilesize) == 0 && start_of_superblock != start_of_paging_tile) {
            f->found_error = 1;
            printf("ERROR: Superblock %d, %d is the first superblock in a paging tile, but its payload data isn't at the beginning of the paging tile.\n",
                   mbx, mby
                );
            printf("ERROR: Paging tile start is %llu, but superblock start is %llu.\n",
                   (long long unsigned int)start_of_paging_tile,
                   (long long unsigned int)start_of_superblock
                );
        }
    }


    if (f->error_check) {
        if (f->check_payload_alignment != 0 && f->check_stripe_height == 0 && mbx == 0 && mby == 0) {
            // check that the payload buffer is properly aligned
            unsigned int ptr = body_base_ptr;
            ptr -= f->check_payload_offset;
            if (ptr % f->check_payload_alignment != 0) {
                f->found_error = 1;
                printf("ERROR: The payload buffer is not aligned to %d bytes. It starts at byte %d.\n", f->check_payload_alignment, ptr);
            }
        }

        if (f->check_sparse) {
            // check that the superblock is on a sparse address
            uint64_t expected_alignment = afbc_get_max_superblock_payloadsize(f);
            unsigned int ptr = body_base_ptr;
            unsigned int offset_ptr;
            ptr -= f->check_payload_offset;

            if (f->first_payload_ptr == ~0u || ptr < f->first_payload_ptr) {
                f->first_payload_ptr = ptr;
            }

            // offset of this payload data vs earlier seen so far
            offset_ptr = ptr - f->first_payload_ptr;

            if (offset_ptr % expected_alignment != 0) {
                f->found_error = 1;
                printf("ERROR: Superblock %d in the payload buffer not spaced a multiple of %llu bytes vs another block.\n", mbx + mby * f->mbw, (long long unsigned int)expected_alignment);
            }
        }

        if (f->check_stripe_height) {
            // check that the superblock is part of the appropriate stripe

            // possible byte range for this superblock
            uint64_t superblock_size = afbc_get_max_superblock_payloadsize(f);
            unsigned int ptr = body_base_ptr;
            uint64_t end_ptr = ptr + superblock_size;

            // rows of superblocks per stripe
            int sbh = f->check_stripe_height / superblock_height[f->superblock_layout];
            // which stripe are we on
            int stripe_n = mby / sbh;

            struct stripe_bin_t *sb = &f->stripe_bins[stripe_n];

            if (sb->start_addr == ~0ull || ptr < sb->start_addr) {
                sb->start_addr = ptr;
            }
            if (sb->end_addr == ~0ull || end_ptr > sb->end_addr) {
                sb->end_addr = end_ptr;
            }

            sb->remaining_superblocks -= 1;

            if (sb->remaining_superblocks == 0) {
                // this is the last block! verify alignment and stripe size

                uint64_t stripe_size = sb->end_addr - sb->start_addr;
                uint64_t expected_size = superblock_size*f->mbw*sbh;

                if (f->check_payload_alignment != 0) {
                    uint64_t check_ptr = sb->start_addr;
                    check_ptr -= f->check_payload_offset;

                    if (check_ptr % f->check_payload_alignment != 0) {
                        f->found_error = 1;
                        printf("ERROR: Stripe %d in the payload buffer is not aligned to expected %d bytes.\n", stripe_n, f->check_payload_alignment);
                    }
                }

                if (stripe_size > expected_size) {
                    f->found_error = 1;
                    printf("ERROR: Stripe %d covers %llu bytes, which is larger than expected %llu bytes\n", stripe_n, (long long unsigned int)stripe_size, (long long unsigned int)expected_size);
                }
            }
        }
    }


    
//    printf_prefix("Block (%d,%d)\n", mbx, mby);

#ifdef TREE_DEBUG
    printf_prefix("Block (%d,%d)\n", mbx, mby);
    for (i = 0; i < f->nsubblocks; i++) {
      printf_prefix("Subblock #%d Size   = %d  0x%x\n", i, subblock_size[i], subblock_size[i]);
      printf_prefix("Subblock #%d Offset = %lld  0x%llx\n", i, subblock_offset[i], subblock_offset[i]);
      printf_prefix("Subblock #%d First payload  = 0x%02x\n", i, buf[subblock_offset[i]]);
    }
#endif
    
    // Loop over all 4x4 subblocks
    for(i=0; i<f->nsubblocks; i++) {
        plane=f->subblock_order[i].plane;
        x=f->subblock_order[i].x;
        y=f->subblock_order[i].y;

        //printf_prefix("(%d,%d) i=%d subblock_offset=%llu subblock_size=%d uncompressed=%d\n", mbx, mby, i, (long long unsigned int)subblock_offset[i], subblock_size[i], uncompressed[i]);
        if (subblock_offset[i]+subblock_size[i] > f->frame_size)
        {
            fatal_error("ERROR: Subblock outside the frame mbx=%d mby=%d i=%d "
                "subblock_offset=%llu subblock_size=%d ucompressed=%d "
                "frame_size=%llu\n",
                mbx, mby, i, subblock_offset[i], subblock_size[i],
                        uncompressed[i], (long long unsigned int)f->frame_size);

            // fill subblock with zeroes
            for(j=0; j<f->ncomponents[plane]; j++) {
                int xx,yy;
                int k=f->first_component[plane]+j;
                for(yy=0; yy<4; yy++)
                    for(xx=0; xx<4; xx++) {
                        block[k][(y+yy)*f->mb_sizew+x+xx]=0;
                    }
            }
            continue;
        }
        size = afbc_decode_block4x4(f, 
                                    plane, 
                                    buf+subblock_offset[i], 
                                    subblock_size[i], 
                                    uncompressed[i], 
                                    subblock,
                                    i);

        if (subblock_offset[i] >= (unsigned int) bufsize) {
            fatal_error("ERROR: Buffer size is %d but current subblock offset is %d.",
                        subblock_offset[i], bufsize);
        }
        // make sure we have used the minimum size dictated for a format
        if (size >= 0 && size<2*f->sbs_multiplier[plane])
            size=2*f->sbs_multiplier[plane];

        sbsm = f->sbs_multiplier[plane];
        // Don't perform rounding up to multiple of sb size multiplier if uncompressed.
        if (uncompressed[i]) {
            sbsm = 1;
        }
        if (size == -1 || (((size + (sbsm - 1)) / sbsm) * sbsm) != subblock_size[i])
        {
            if (f->error_check) {
                report_conformance_failure();
            }
            fatal_error("ERROR: Subblock size mismatch mbx=%d mby=%d i=%d "
                "subblock_offset=%llu subblock_size=%d ucompressed=%d "
                "decoded size=%d\n",
                mbx, mby, i, (long long unsigned int)subblock_offset[i], subblock_size[i],
                uncompressed[i], size);
        }
        
        // Copy subblock
        for(j=0; j<f->ncomponents[plane]; j++) {
            int xx,yy;
            int k=f->first_component[plane]+j;
            for(yy=0; yy<f->b_sizeh; yy++)
                for(xx=0; xx<f->b_sizew; xx++) {
                    block[k][(y+yy)*f->mb_sizew+x+xx]=subblock[j][yy][xx];
                    //printf_prefix(">>> %2d %2d %2d : %02x\n", j, y+yy, x+xx, subblock[j][yy][xx]);
                }
        }
    }
    return 1;
}

// Parse the fileheader in buf. 
// Output frame_info struct and the frame_size.
// Returns 0 if successful.
int afbc_parse_fileheader(unsigned char *buf, struct afbc_frame_info *f, uint64_t *frame_size) {
    int version, width, height, ncomponents, superblock_layout, yuv_transform, inputbits[4], mbw, mbh, block_split, tiled;
    int left_crop, top_crop, file_message;
    uint32_t frame_size_32b;


    if (buf[0]!='A' || buf[1]!='F' || buf[2]!='B' || buf[3]!='C' || buf[4] < AFBC_FILEHEADER_SIZE)
        return -1;
    version = buf[6];
    
    // handle AFBC file format version 3, 4 or 5
    if (version != 3 && version != 4 && version != 5) {
        return -version;
    }
    // directly allocating to 64b variable incorrectly sign extends
    frame_size_32b= (buf[11]<<24) | (buf[10]<<16) | (buf[9]<<8) | (buf[8]<<0);
    *frame_size = frame_size_32b;
    ncomponents=buf[12];
    superblock_layout=buf[13];
    yuv_transform=buf[14];
    block_split=buf[15];
    if (version == 3) {
        block_split=0;
    }
    inputbits[0]=buf[16];
    inputbits[1]=buf[17];
    inputbits[2]=buf[18];
    inputbits[3]=buf[19];
    mbw=(buf[21]<<8)+buf[20];
    mbh=(buf[23]<<8)+buf[22];
    width=(buf[25]<<8)+buf[24];
    height=(buf[27]<<8)+buf[26];
    left_crop=buf[28];
    top_crop=buf[29];
    if (version == 5) {
        tiled=buf[30];
    } else {
        tiled = 0;
    }
    file_message=buf[31];
    if (f->crop_ignore)
    {
        width = mbw*superblock_width[superblock_layout];
        height = mbh*superblock_height[superblock_layout];
        left_crop=0;
        top_crop=0;
    }
    afbc_init_frame_info(f, width, height, ncomponents, superblock_layout, yuv_transform, 
                         tiled, inputbits);
    f->block_split = block_split;
    f->frame_size = *frame_size;    
    f->version=version;
    f->left_crop=left_crop;
    f->top_crop=top_crop;
    f->mbw=mbw;
    f->mbh=mbh;
    f->file_message=file_message;
    return 0;
}
