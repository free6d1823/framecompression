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
#include "afbc_encode.h"

//#define DEBUG

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

// find first one
static unsigned int bitcount(unsigned int a) {
    int i;
    for(i=31; i>=0; i--)
        if ((a>>i)&1)
            return i+1;
    return 0;
}

void select_minval(unsigned int a,
                   unsigned int b,
                   unsigned int c,
                   unsigned int d,
                   unsigned int *minval,
                   unsigned int *maxval,
                   int compbits,
                   int minval_wrap_en) {
    if (minval_wrap_en) {
        char cnt[4];
        int i;
        for(i=0; i<4; i++)
            cnt[i]=0;
        cnt[(a>>(compbits-2))&3]++;
        cnt[(b>>(compbits-2))&3]++;
        cnt[(c>>(compbits-2))&3]++;
        cnt[(d>>(compbits-2))&3]++;
        if (cnt[0]>0 && cnt[1]==0 && cnt[2]==0 && cnt[3]>0) {
            //printf("wrap around %02x %02x %02x %02x\n", a, b, c, d);
            if ((a>>(compbits-1))==0) a+=(1<<compbits);
            if ((b>>(compbits-1))==0) b+=(1<<compbits);
            if ((c>>(compbits-1))==0) c+=(1<<compbits);
            if ((d>>(compbits-1))==0) d+=(1<<compbits);
        }
    }
    *minval=min(min(a,b),min(c,d));
    *maxval=max(max(a,b),max(c,d));
}


// recursively compress the given block
// the stride of the block is (1<<maxdepth)
// for 4x4 blocks: call with depth=1, maxdepth=2, x=0, y=0, parent=0
// the result is written to the mintree and bctree
static void tree_encode(int depth, int maxdepth, int x, int y, int parent, int compbits, unsigned int *mintree, int *bctree, unsigned int *block, int minval_wrap_en) {
    unsigned int a, b, c, d, minval, maxval, mask;
    //printf("depth=%d, x=%d, y=%d, parent=%d\n", depth, x, y, parent);
    if (depth==maxdepth) {
        mintree[child(parent,0)]=block[(y+0)*(1<<maxdepth)+(x+0)];
        mintree[child(parent,1)]=block[(y+0)*(1<<maxdepth)+(x+1)];
        mintree[child(parent,2)]=block[(y+1)*(1<<maxdepth)+(x+0)];
        mintree[child(parent,3)]=block[(y+1)*(1<<maxdepth)+(x+1)];
    } else {
        tree_encode(depth+1, maxdepth, 2*x+0, 2*y+0, child(parent,0), compbits, mintree, bctree, block, minval_wrap_en);
        tree_encode(depth+1, maxdepth, 2*x+2, 2*y+0, child(parent,1), compbits, mintree, bctree, block, minval_wrap_en);
        tree_encode(depth+1, maxdepth, 2*x+0, 2*y+2, child(parent,2), compbits, mintree, bctree, block, minval_wrap_en);
        tree_encode(depth+1, maxdepth, 2*x+2, 2*y+2, child(parent,3), compbits, mintree, bctree, block, minval_wrap_en);
    }
    a=mintree[child(parent,0)];
    b=mintree[child(parent,1)];
    c=mintree[child(parent,2)];
    d=mintree[child(parent,3)];
    select_minval(a,b,c,d,&minval,&maxval,compbits, minval_wrap_en);
    mask=(1<<compbits)-1;
    mintree[child(parent,0)]=(a-minval)&mask;
    mintree[child(parent,1)]=(b-minval)&mask;
    mintree[child(parent,2)]=(c-minval)&mask;
    mintree[child(parent,3)]=(d-minval)&mask;
    bctree[parent]=bitcount(maxval-minval);
    mintree[parent]=minval;
}

// Set bctree to -1 for completely empty subtrees
// For 4x4 blocks: call with depth=1, maxdepth=2, parent=0
static void tree_crop(int depth, int maxdepth, int parent, int *bctree) {
    if (depth<maxdepth) {
        unsigned int empty=1, i;
        for(i=0; i<4; i++) {
            tree_crop(depth+1, maxdepth, child(parent, i), bctree);
            if (!(bctree[child(parent,i)]==-1 || (depth==maxdepth-1 && bctree[child(parent,i)]==0)))
                empty=0;
        }
        if (empty && bctree[parent]==0)
            bctree[parent]=-1;
    }
}

// Contrain bctree bottom-up to make sure bctree deltas are in the range [-2,1]
static void tree_up_constrain(int depth, int maxdepth, int parent, int *bctree) {
    //printf("depth=%d, parent=%d\n", depth, parent);
    if (depth<maxdepth) {
        int maxchild=0, i;
        for(i=0; i<4; i++) {
            tree_up_constrain(depth+1, maxdepth, child(parent, i), bctree);
            maxchild=max(maxchild, bctree[child(parent,i)]);
        }
        bctree[parent]=max(bctree[parent],maxchild-1);
    }
}

// Contrain bctree top-down to make sure bctree deltas are in the range [-2,1]
static void tree_down_constrain(int depth, int maxdepth, int parent, int *bctree) {
    //printf("depth=%d, parent=%d\n", depth, parent);
    if (depth<maxdepth) {
        unsigned int i;
        for(i=0; i<4; i++) {
            /*if (bctree[child(parent,i)] != max(bctree[child(parent,i)],bctree[parent]-2)) {
                printf("down constrain %d -> %d\n", bctree[child(parent,i)], max(bctree[child(parent,i)],bctree[parent]-2));
            }
            else {
                printf("down nope\n");
            }*/
            bctree[child(parent,i)]=max(bctree[child(parent,i)],bctree[parent]-2);
            tree_down_constrain(depth+1, maxdepth, child(parent, i), bctree);
        }
    }
}

// Check that all bctree deltas are in the range [-2,1]
static int tree_check_constrain(int depth, int maxdepth, int parent, int *bctree) {
    int bc=bctree[parent], ok=1;
    if (depth<maxdepth-1) {
        unsigned int i;
        for(i=0; i<4; i++) {
            ok = ok && tree_check_constrain(depth+1, maxdepth, child(parent, i), bctree);
            ok = ok && (bc-2)<=bctree[child(parent, i)] && bctree[child(parent, i)]<=(bc+1);
        }
    }
    return ok;
}

#ifdef GATHER_BIN_STATS
extern void register_addr_use(unsigned char *p);
static int register_turned_off = 0;
#endif

// write bits to the given buf at the given bitpos
// bufsize in byte
// return updated bitpos or -1 in case of buffer overflow
static int write_bits(unsigned char *buf, int bufsize, int bitpos, int value, int bits) {
#ifdef DEBUG
    printf("write_bits: pos=%d bits=%d data=%08x bufsize=%d\n", bitpos, bits, value&((1<<bits)-1), bufsize);
#endif
    if (bitpos==-1 || bitpos+bits>8*bufsize)
        return -1; // does not fit
    while(bits) {
        int eat;
#ifdef GATHER_BIN_STATS
        if (!register_turned_off) {            
            register_addr_use(&buf[bitpos/8]);
        }
#endif
        buf[bitpos/8]=(buf[bitpos/8] & ~((0xff<<(bitpos&7)))) | (value<<(bitpos&7));
        eat=min(bits,(8-(bitpos&7)));
        value=value>>eat;
        bitpos+=eat;
        bits-=eat;
    }
    return bitpos;
}

// write bits for a mintree node (corresponding to "node_decode" in the spec)
static int emit_mintree_node(unsigned int *mintree, int *bctree, int parent, unsigned char *buf, int bufsize, int bitpos) {
    int i,j;
    int zcd;

    if (bctree[parent]==1) {
        for(i=0; i<4; i++) {
            bitpos=write_bits(buf, bufsize, bitpos, mintree[child(parent,i)], 1);
        }
    } else if (bctree[parent]>1) {
        for(zcd=0; zcd<4; zcd++) {
            //printf("zcd=%d parent=%d %d\n", zcd, parent, mintree[child(parent,zcd)]);
            if (mintree[child(parent,zcd)]==0)
                break;
        }
        if (zcd==4)
            fatal_error("ERROR: internal inconcistency error, no zcd found\n");
        bitpos=write_bits(buf, bufsize, bitpos, zcd, 2);
        // Store nodes bit interleaved
        for(i=0; i<bctree[parent]; i++)
            for(j=0; j<4; j++)
                if (j!=zcd)
                    bitpos=write_bits(buf, bufsize, bitpos, mintree[child(parent,j)]>>i, 1);
    }
    return bitpos;
}

// Write body data for a subblock encoded in mintrees and bctrees to the given buf
// Return the size in byte or -1 in case of buffer overflow
static int emit_compressed_subblock(struct afbc_frame_info *info, int plane, unsigned int mintrees[4][21], int bctrees[4][5], unsigned char *buf, int bufsize) {
    int i, j, bitpos=0;
    int ncomp=info->ncomponents[plane];
    for(i=0; i<ncomp; i++) {
        // Bitcount root
        bitpos=write_bits(buf, bufsize, bitpos, bctrees[i][0], log2_table[info->compbits[info->first_component[plane]+i]]);
    }

#ifdef DEBUG
    printf("bctree_l4\n");
#endif
    for(i=0; i<ncomp; i++) {
        if (bctrees[i][0]>=0) {
            // Bitcount tree leaves
            for(j=0; j<4; j++)
                bitpos=write_bits(buf, bufsize, bitpos, bctrees[i][child(0,j)]-bctrees[i][0], 2);
        }
    }

#ifdef DEBUG
    printf("mintree_root\n");
#endif
    for(i=0; i<ncomp; i++) {
        // Mintree root
        int compbits=info->compbits[info->first_component[plane]+i];
        if (bctrees[i][0]!=-2)  // if not defaultcolor
            bitpos=write_bits(buf, bufsize, bitpos, mintrees[i][0], compbits);
    }

#ifdef DEBUG
    printf("mintree_l3\n");
#endif
    // 2x2 mintree nodes
        for(i=0; i<ncomp; i++) {
            if (bctrees[i][0]>=0)
                bitpos=emit_mintree_node(mintrees[i], bctrees[i], 0, buf, bufsize, bitpos);
        }

#ifdef DEBUG
        printf("mintree_l4\n");
#endif

    // 1x1 mintree nodes
    for(j=0; j<4; j++)
        for(i=0; i<ncomp; i++)
            if (bctrees[i][0]>=0)
                bitpos=emit_mintree_node(mintrees[i], bctrees[i], child(0,j), buf, bufsize, bitpos);

    if (bitpos==-1)
        return -1;

    bitpos = ((bitpos + 7)>>3)*8;

    // If we have sbs multiplier we need to make sure the size is aligned to this
    // multiplier
    while ((bitpos/8)%info->sbs_multiplier[plane]) {
        bitpos += 8;
    }

    // We must pad to avoid the size of the uncompressed symbol
    while ((bitpos/8) < 2*info->sbs_multiplier[plane]) {
        bitpos += 8;
    }

    return bitpos>>3; // byte align
}

// Write uncompressed body data for the given subblock
// Return the size in byte or -1 in case of buffer overflow
static int emit_uncompressed_subblock(struct afbc_frame_info *info, int plane, unsigned int block[4][16], unsigned char *buf, int bufsize) {
    int i, c, bitpos=0;

    // emit as uncompressed
    for(i=0; i<16; i++)
        for(c=0; c<info->ncomponents[plane]; c++) {
            int bits=info->inputbits[info->first_component[plane]+c];
            bitpos=write_bits(buf, bufsize, bitpos, block[c][i], bits);
        }
    if (bitpos==-1)
        return -1;
    return (bitpos+7)>>3; // byte align
}

// Apply the integer YUV transform on the given block.
// block incices are block[component][y][x]
static void apply_yuv_transform(struct afbc_frame_info *info, unsigned int block[4][16]) {
    int i, f=(1<<info->inputbits[1]);
    for(i=0; i<16; i++) {
#ifdef DEBUG
            printf("RGB: %d %d %d\n", block[0][i], block[1][i], block[2][i]);
#endif
            unsigned int R, G, B, Y, U, V;
            R=block[0][i];
            G=block[1][i];
            B=block[2][i];
            Y=(R+2*G+B)/4;
            U=R-G+f;
            V=B-G+f;
            block[0][i]=Y;
            block[1][i]=U;
            block[2][i]=V;
            // Alpha channel is left unchanged
#ifdef DEBUG
            printf("YUV: %d %d %d\n", block[0][i], block[1][i], block[2][i]);
#endif
        }
}

// Encode the given 4x4 block to the given buf
// block indices are block[component][y][x]
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
                         int rgb_defcolor_en) {
    unsigned int mintrees[4][21];  // 1+4+16==21
    int bctrees[4][5];             // 1+4=5
    int i, size;
    unsigned int save_block[4][16];
    unsigned char tmp_buf[4*4*4*16]; // enough space for 4 components each of 16 bits

    memcpy(save_block, block, sizeof(save_block));

    if (info->yuv_transform)
        apply_yuv_transform(info, block);

    *color=0;
    // Loop over components and create the bctree and the mintree
    for(i=0; i<info->ncomponents[plane]; i++) {
        int compbits=info->compbits[info->first_component[plane]+i];
        int defaultcolor=info->defaultcolor[info->first_component[plane]+i];

#ifdef DEBUG
        int m;

        printf("\n");
        for (m= 0; m < 16; ++m) {
            printf("(%d) ", block[i][m]);
            if (m%4==3) printf("\n");
        }
        printf("\n");
#endif

        // Create min-tree
        tree_encode(1, 2, 0, 0, 0, compbits, mintrees[i], bctrees[i], (unsigned int*)block[i], minval_wrap_en);

#ifdef DEBUG
        for (m= 0; m < 21; ++m) {
            printf("%d ", mintrees[i][m]);
        }
        printf("\n");
        for (m= 0; m < 5; ++m) {
            printf("%d ", bctrees[i][m]);
        }
        printf("\n");
#endif

        // Crop empty branches
        tree_crop(1, 2, 0, bctrees[i]);

        // Constrain the bc-tree
        tree_up_constrain(1, 2, 0, bctrees[i]);
        tree_down_constrain(1, 2, 0, bctrees[i]);
        if (!tree_check_constrain(1, 2, 0, bctrees[i]))
            fatal_error("ERROR: bctree not constrained\n");

        // Keep track of single color blocks (all components must be single color)
        if (bctrees[i][0]==-1 && *color!=-1) {
            *color=(*color<<16)|mintrees[i][0];
        } else {
            *color=-1;
        }

        // Check if the block has the default color
        if ((rgb_defcolor_en || i == 3) && bctrees[i][0]==-1 && mintrees[i][0]==(unsigned) defaultcolor) {
            bctrees[i][0]=-2;
        }
    }

    // Emit compressed block to temporary buffer.
    // This is done to avoid junk data in the output buffer in case we will write the block as uncompressed.
    memset(tmp_buf, 0, sizeof(tmp_buf));
#ifdef GATHER_BIN_STATS
    register_turned_off = 1;
#endif
    size=emit_compressed_subblock(info, plane, mintrees, bctrees, tmp_buf, bufsize);
#ifdef GATHER_BIN_STATS
    register_turned_off = 0;
#endif

    // If we only generate uncompressed block, or if it does not fit or if the size is greater than the threshold
    // for uncompressed, then store the block uncompressed (before YUV transform)
    if (info->only_uncompressed_blocks || size==-1 || size>=info->uncompressed_size[plane]) {
        *color=-1;
        *uncompressed=1;
        size=emit_uncompressed_subblock(info, plane, save_block, buf, bufsize);
    } else {
        // Copy compressed data to output buffer.
        memcpy(buf, tmp_buf, size);
#ifdef GATHER_BIN_STATS
        for (i = 0; i < size; i++) {
            register_addr_use(&buf[i]);
        }
#endif
        *uncompressed=0;
    }
    return size;
}

// On which block does the second half start
int afbc_block_split_block(struct afbc_frame_info *f) {
    if (sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_NONE) {
        return 8;
    } else if (sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_420) {
        return 10;
    } else {// if (sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_422) {
        return 12;
    }
}

int afbc_block_split_block_alt(struct afbc_frame_info *f) {
    if (sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_NONE) {
        return 8; // no alternativ block
    } else if (sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_420) {
        return 12; // the first chroma block
    } else {// if (sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_422) {
        return 13;
    }
}


int round_up(int bytes, int align) {
    if (bytes%align != 0) {
        return bytes + align - (bytes%align);
    } else {
        return bytes;
    }
}

void update_statistics(struct afbc_frame_info *f, int bytes, int* sizes) {
    int i, b;
    for (i = 1; i < 2; ++i) {
        int burst_size = 1 << (5+i);
        int tot;
        int oddness;

        // for encode we will get one final burst for each tile we need to round up
        f->actual_gpu_bytes[i] += round_up(bytes, burst_size);

        // for the display controller it is a bit more complicated
        // if we read 8 lines at a time we need to know exactly which tiles has which sizes
        tot = 0;
        for (b = 0; b < f->nsubblocks/2; ++b) {
            tot += sizes[b];
        }
        f->actual_display_bytes[i] += round_up(tot, burst_size);

        oddness = (burst_size-tot)%burst_size;

        // add another round up to account for the fact that odd sizes here has to be
        // read again when the second set of lines is read
        f->actual_display_bytes[i] += round_up(oddness, burst_size);

        tot = 0;
        for (b = f->nsubblocks/2; b < f->nsubblocks; ++b) {
            tot += sizes[b];
        }
        tot -= oddness;
        if (tot > 0) {
            f->actual_display_bytes[i] += round_up(tot, burst_size);
        }

        // add size of header, assume this is fetched efficiently
        f->actual_display_bytes[i] +=16;
        f->actual_gpu_bytes[i] +=16;
    }
}

int afbc_encode_superblock_is_solid(struct afbc_frame_info *f,
                                    unsigned int block[4][256]) {


    int plane, i, j;
    int pixels_in_plane = 256;

    // only allowed for non subsampled layouts
    if ((f->superblock_layout != 0) &&
        (f->superblock_layout != 3) &&
        (f->superblock_layout != 4)) {
        return 0;
    }
    
    for (plane = 0; plane < f->nplanes; ++plane) {
        for (j = f->first_component[plane]; j < f->first_component[plane] + f->ncomponents[plane]; ++j) {
            pixels_in_plane = (sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_420 ? 64 : 
                                sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_422 ? 128 :
                                256);
            for (i = 1; i < pixels_in_plane; ++i) {
                if (block[j][i] != block[j][0]) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

// the extended version of the below function which updates the split_block_pos.
// for standalone afbc testbench to get the position of the first half body
int afbc_encode_superblock_ext(struct afbc_frame_info *f,
                               unsigned int block[4][256],
                               unsigned char *buf,
                               int bufsize,
                               int mbnum,
                               int body_pos,
                               int minval_wrap_en,
                               int rgb_defcolor_en,
                               int* split_block_pos) {
    int x, y, i, j;
    long long prev_color[2], color;
    int prev_subblock_valid[2];
    unsigned int prev_subblock[2][4][16];
    unsigned int subblock[4][16];
    int plane, uncompressed, subblock_size, header_bitpos=0, size;
    unsigned char *header_buf;
    int body_pos_phalf;

    int pre_body_pos = body_pos;
    int sizes[32];
    int format_allows_solid_color;

    prev_subblock_valid[0] = 0;
    prev_subblock_valid[1] = 0;

    // Pointer to the header
    header_buf=buf + mbnum * AFBC_HEADER_SIZE;

    // only non-subsampled formats allow solid color
    format_allows_solid_color = ((f->superblock_layout == 0) ||
                                 (f->superblock_layout == 3) ||
                                 (f->superblock_layout == 4));

    // First chck if all the pixels of the superblock has the same pixel values, then
    // the color can be endoded in the header
    if (format_allows_solid_color && f->allow_solid_color && afbc_encode_superblock_is_solid(f, block)) {
        header_bitpos=write_bits(header_buf, 16, header_bitpos, 0, 64);
        for (i = 0; i < f->ncomponents[0] + f-> ncomponents[1]; ++i) {
            header_bitpos=write_bits(header_buf, 16, header_bitpos, block[i][0], f->inputbits[i]);
        }
        header_bitpos=write_bits(header_buf, 16, header_bitpos, 0, 64-(f->inputbits[0]+f->inputbits[1]+f->inputbits[2]+f->inputbits[3]));
        return body_pos;
    }

    // Write body_base_ptr
    header_bitpos=write_bits(header_buf, 16, header_bitpos, body_pos, f->body_base_ptr_bits);

    // save the body pos + the size of half an uncompressed superblock
    // for use by the block split functionality
    body_pos_phalf = body_pos + ((int)afbc_get_max_superblock_payloadsize(f))/2;

    for(i=0; i<2; i++)
        prev_color[i]=-1;
    // Loop over all 4x4 subblocks
    for(i=0; i<f->nsubblocks; i++) {
        int subblock_identical_to_previous = 1;

        plane=f->subblock_order[i].plane;
        x=f->subblock_order[i].x;
        y=f->subblock_order[i].y;

        // Copy subblock
        for(j=0; j<f->ncomponents[plane]; j++) {
            int xx,yy;
            int k=f->first_component[plane]+j;
            for(yy=0; yy<f->b_sizeh; yy++)
                for(xx=0; xx<f->b_sizew; xx++) {
                    subblock[j][yy*f->b_sizew+xx]=block[k][(y+yy)*f->mb_sizew+x+xx];
                }
        }

        // Compare subblock with previous subblock
        subblock_identical_to_previous = 1;
        if (f->allow_all_copy_blocks && prev_subblock_valid[plane]) {
            for(j=0; j<f->ncomponents[plane]; j++) {
                int xx,yy;
                for(yy=0; yy<f->b_sizeh; yy++) {
                    for(xx=0; xx<f->b_sizew; xx++) {
                        if (subblock[j][yy*f->b_sizew+xx] != prev_subblock[plane][j][yy*f->b_sizew+xx]) {
                            subblock_identical_to_previous = 0;
                        }
                    }
                }
            }
        }
        else {
            subblock_identical_to_previous = 0;
        }

        // Store subblock as "previous" for next iteration
        for(j=0; j<f->ncomponents[plane]; j++) {
            int xx,yy;
            int k=f->first_component[plane]+j;
            for(yy=0; yy<f->b_sizeh; yy++)
                for(xx=0; xx<f->b_sizew; xx++) {
                    prev_subblock[plane][j][yy*f->b_sizew+xx]=block[k][(y+yy)*f->mb_sizew+x+xx];
                }
        }
        prev_subblock_valid[plane] = 1;

        size=afbc_encode_block4x4(f, plane, subblock, buf+body_pos, bufsize-body_pos, &color, &uncompressed, minval_wrap_en, rgb_defcolor_en);
        //printf("i=%d plane=%d uncompressed=%d body_pos=%d size=%d\n", i, plane, uncompressed, body_pos, size);
        sizes[i] = size;

        if (size==-1)
            return -1;  // buffer overflow

        // the size 1 is not allowed to be sent in a stream. In certain cases
        // (with sbs multiplier it is possible to send blocks a size that is 2 or
        // less (which will be encoded as 1 in the header due to the sbs_multiplier)
        // In these cases we need to raise the size to 4 (2 sent in the header multiplied by 2 (for example))
        if (size<2*f->sbs_multiplier[plane])
            size=2*f->sbs_multiplier[plane];

        // To allow copy blocks of single colores subblocks, even if allow_all_copy_blocks == 0, OR in the old condition
        subblock_identical_to_previous |= color==prev_color[plane] && color!=-1;

        // Check if we can reuse previous subblock encoding
        if (subblock_identical_to_previous && !(
                ((afbc_block_split_block(f) == i) ||
                 (afbc_block_split_block_alt(f) == i)) && f->disable_copy_rows) && !(
                     f->disable_copies_crossing_8x8 && afbc_first_block_in_8x8(f, i))) {
            memset(buf+body_pos, 0, size); // first erase the data we wrote for this block
            subblock_size=0; // copy previous
            size=0;
        } else if (uncompressed) {
            subblock_size=1; // uncompressed
        } else {
            subblock_size=size;
            // sbs multiplier rounding already done in emit_compressed_subblock
            subblock_size = (subblock_size) / (f->sbs_multiplier[plane]);
            if (subblock_size == 1) subblock_size = 2;
        }
        prev_color[plane]=color;
        //printf("subblock_size %2d %2d\n", i, subblock_size);
        // apply the sub block size multiplier to determine the size of the block

        header_bitpos=write_bits(header_buf, 16, header_bitpos, subblock_size, f->subblock_size_bits);
        body_pos+=size;

        if (f->block_split && (afbc_block_split_block(f)-1 == i)) {
            if (split_block_pos) {
                *split_block_pos = body_pos; // store the splitted block position
            }
            body_pos = body_pos_phalf;
        }
    }

    update_statistics(f, body_pos - pre_body_pos, sizes);

    return body_pos;
}

// Encode the given superblock to the given mbx, mby block coordinate (in block16 units)
// Block incices are block[component][y][x]
// buf is the AFBC frame buffer
// bufsize is the size of buf in byte
// body_pos is a byte offset to where payload data will be written
// The return value is an updated body_pos
// -1 is returned in case of buffer overflow
int afbc_encode_superblock(struct afbc_frame_info *f,
                           unsigned int block[4][256],
                           unsigned char *buf,
                           int bufsize,
                           int mbnum,
                           int body_pos,
                           int minval_wrap_en,
                           int rgb_defcolor_en) {
    return afbc_encode_superblock_ext(f, block, buf, bufsize, mbnum, body_pos, minval_wrap_en, rgb_defcolor_en, 0);
}


// Write fileheader to buffer based on the given frame_info struct
void afbc_write_fileheader(struct afbc_frame_info *f, int frame_size, unsigned char *buf) {
    buf[0]  = 'A';
    buf[1]  = 'F';
    buf[2]  = 'B';
    buf[3]  = 'C';
    buf[4]  = AFBC_FILEHEADER_SIZE;
    buf[5]  = 0;    // Should be zero
    buf[6]  = AFBC_VERSION;
    buf[7]  = 0;    // Should be zero
    buf[8]  = (frame_size>>0)&0xff;
    buf[9]  = (frame_size>>8)&0xff;
    buf[10] = (frame_size>>16)&0xff;
    buf[11] = (frame_size>>24)&0xff;
    buf[12] = f->ncomponents[0] + f->ncomponents[1];
    buf[13] = f->superblock_layout;
    buf[14] = f->yuv_transform;
    buf[15] = f->block_split;
    buf[16] = f->inputbits[0];
    buf[17] = f->inputbits[1];
    buf[18] = f->inputbits[2];
    buf[19] = f->inputbits[3];
    buf[20] = f->mbw&0xff;
    buf[21] = (f->mbw>>8)&0xff;
    buf[22] = f->mbh&0xff;
    buf[23] = (f->mbh>>8)&0xff;

    buf[24] = f->width&0xff;
    buf[25] = (f->width>>8)&0xff;
    buf[26] = f->height&0xff;
    buf[27] = (f->height>>8)&0xff;

    buf[28] = f->left_crop;
    buf[29] = f->top_crop;
    buf[30] = f->tiled;
    buf[31] = f->file_message;

    assert(AFBC_FILEHEADER_SIZE==32);
}

