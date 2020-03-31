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
#include <stdint.h>
#include "afbc_common.h"
#include "afbc_generate.h"

int quiet_mode = 0;

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

// Apply the integer YUV transform on the given block.
// block incices are block[component][y][x]
static void apply_yuv_transform(struct afbc_frame_info *info, int block[4][16]) {
    int x, y, f=(1<<info->inputbits[1]);
    //printf("YUV: %02x %02x %02x\n", block[0][0][0], block[1][0][0], block[2][0][0]);
    for(y=0; y<4; y++)
        for(x=0; x<4; x++) {
            unsigned int R, G, B, Y, U, V;
            Y=block[0][y*4+x];
            U=block[1][y*4+x];
            V=block[2][y*4+x];
            G=((unsigned int)(4*Y-U-V+3+2*f))>>2;
            R=U+G-f;
            B=V+G-f;
            // check if the yuv values was constructed in such a way that they don't convert
            // to legal RGB values.
            if((R >= (1 << info->inputbits[0])) ||
               (G >= (1 << info->inputbits[1])) ||
               (B >= (1 << info->inputbits[2]))) {
                fatal_error("ERROR: YUV values produces rgb values out of range: YUV %d %d %d -> rgb %d %d %d\n", Y, U, V, R, G, B);
            }
            
            block[0][y*4+x]=R;
            block[1][y*4+x]=G;
            block[2][y*4+x]=B;
            // Alpha channel is left unchanged
        }
    //printf("RGB: %02x %02x %02x\n", block[0][0][0], block[1][0][0], block[2][0][0]);
}


// write bits to the given buf at the given bitpos
// bufsize in byte
// return updated bitpos or -1 in case of buffer overflow
static int write_bits(unsigned char *buf, int bufsize, int bitpos, int value, int bits) {
//    printf("write_bits: pos=%d bits=%d data=%08x bufsize=%d\n", bitpos, bits, value, bufsize);
    if ((int64_t)(bitpos==-1 || bitpos+bits)>(int64_t)8*(int64_t)bufsize)
        return -1; // does not fit
    while(bits) {
        int eat;
        buf[bitpos/8]=(buf[bitpos/8] & ~((0xff<<(bitpos&7)))) | (value<<(bitpos&7));
        eat=min(bits,(8-(bitpos&7)));
        value=value>>eat;
        bitpos+=eat;
        bits-=eat;
    }
    return bitpos;
}

// write bits for a mintree node (corresponding to "node_decode" in the spec)

static int generate_mintree_node(int *bctree, int parent, unsigned char *buf, int bufsize, int bitpos, int* diffs, int force_zero) {
    int i,j;
    if (bctree[parent]==1) {
        int all_ones = 1;
        for(i=0; i<4; i++) {
            diffs[i] = (1-force_zero)*rand() % 2;
            if (diffs[i] == 0) {
                all_ones = 0;
            }
        }
        if (all_ones) {
            // pick one at random and set to zero
            diffs[rand() % 4] = 0;
        }        
        for(i=0; i<4; i++) {
            bitpos=write_bits(buf, bufsize, bitpos, diffs[i], 1);
        }
    } else if (bctree[parent]>1) {
        int zcd;
        zcd = rand()%4;

        bitpos=write_bits(buf, bufsize, bitpos, zcd, 2);

        diffs[0] = (1-force_zero)*rand() % (1 << bctree[parent]);
        diffs[1] = (1-force_zero)*rand() % (1 << bctree[parent]);
        diffs[2] = (1-force_zero)*rand() % (1 << bctree[parent]);
        diffs[3] = (1-force_zero)*rand() % (1 << bctree[parent]);
        diffs[zcd] = 0;

        // Store nodes bit interleaved
        for(i=0; i<bctree[parent]; i++) {
            for(j=0; j<4; j++) {
                if (j!=zcd) {
                    bitpos=write_bits(buf, bufsize, bitpos, diffs[j]>>i, 1);
                }
            }
        }
    }
    return bitpos;
}

// generate random data for all pixels elements
static int generate_uncompressed_subblock(struct afbc_frame_info *info, int plane, unsigned char *buf, int bufsize) {
    int i, x, y, bitpos=0;
    
    // emit as uncompressed
    for(y=0; y<4; y++)
        for(x=0; x<4; x++)
            for(i=0; i<info->ncomponents[plane]; i++) {
                int bits=info->inputbits[info->first_component[plane]+i];
                bitpos=write_bits(buf, bufsize, bitpos, rand()%(1 << bits), bits);
            }
    if (bitpos==-1)
        return -1;
    return (bitpos+7)>>3; // byte align
}

int test_transformability(struct afbc_frame_info *info, int pixels[4][16]) {
    int p, f=(1<<info->inputbits[1]);
    for(p=0; p<16; p++) {
        unsigned int R, G, B, Y, U, V;
        Y=pixels[0][p];
        U=pixels[1][p];
        V=pixels[2][p];
        G=((unsigned int)(4*Y-U-V+3+2*f))>>2;
        R=U+G-f;
        B=V+G-f;

        // check if the yuv values was constructed in such a way that they don't convert
        // to legal RGB values.
        if((R >= (1 << info->inputbits[0])) ||
           (G >= (1 << info->inputbits[1])) ||
           (B >= (1 << info->inputbits[2]))) {
//            printf("Fail! %d %d %d (%d %d %d)\n", R, G, B, Y, U, V);
            return 0;
        } else {
//            printf("one pass!!\n");
        }
    }
//    printf("Success!\n");
    return 1;
}

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
int afbc_generate_block4x4(struct afbc_frame_info *info,
                           int plane,
                           unsigned char *buf,
                           int pixels[4][16],
                           int bufsize, 
                           long long *color,
                           int *uncompressed) {
    int bctrees[4][5];             // 1+4=5
    int i, size, p;
    int j, bitpos=0;
    int ncomp=info->ncomponents[plane];
    
    int diffs[4];
    int saved_bitpos;
    int sbsm;

    int tries_left;

    *color=0;

    // try to generate mintree until we get one that kan be transformed to rgb
    do
    {
        bitpos=0;
        for(i=0; i<ncomp; i++) {
            // bctree_l3
            bctrees[i][0] = (rand() % (info->compbits[i] + 2)) - 2;
            bitpos=write_bits(buf, bufsize, bitpos, bctrees[i][0], log2_table[info->compbits[i]]);
        }
        
        saved_bitpos = bitpos;
//        printf("bctree %d %d %d\n", bctrees[0][0], bctrees[1][0], bctrees[2][0]);
        
        
        // try for a while to generate a mintree
        tries_left = 100;
        while(--tries_left) {
            bitpos = saved_bitpos;
            
//        printf("bitpos %d\n", bitpos);
            
            for(i=0; i<ncomp; i++) {
                if (bctrees[i][0]>=0) {
                    // bctree_l4_diff
                    for(j=0; j<4; j++) {
                        bctrees[i][child(0,j)] = bctrees[i][0] + (rand()%4) - 2;
                        if (bctrees[i][child(0,j)] < 0) {
                            bctrees[i][child(0,j)] = 0;
                        }
                        bitpos=write_bits(buf, bufsize, bitpos, 
                                          bctrees[i][child(0,j)]-bctrees[i][0], 2);
                    }
                }
            }
            
            for(i=0; i<ncomp; i++) {
                //mintree_l2[i]
                int val = 0;
                if (bctrees[i][0] > -2) {
                    // if we have luma default color the other must hit their default color or not be transformable, lets help the generator
                    if ((bctrees[0][0] == -2) && !((sl_subsampling[info->superblock_layout]==AFBC_SUBSAMPLING_420) || (sl_subsampling[info->superblock_layout]==AFBC_SUBSAMPLING_422))
                        && (info->yuv_transform)) {
                        val = info->defaultcolor[i];
                    } else {
                        val = rand() % (1 << info->compbits[i]);
                    }
                    bitpos=write_bits(buf, bufsize, bitpos, val, info->compbits[i]);
                } else {
                    val = info->defaultcolor[i];
                }
                for (p = 0; p < 16; ++p) {
                    pixels[i][p] = val;
                }
            }
            
            // 2x2 mintree nodes
            
            for(i=0; i<ncomp; i++) {
                int force_zero = ((bctrees[0][0] == -2)  && !((sl_subsampling[info->superblock_layout]==AFBC_SUBSAMPLING_420) || (sl_subsampling[info->superblock_layout]==AFBC_SUBSAMPLING_422))
                                  && (info->yuv_transform));
                
                diffs[0] = diffs[1] = diffs[2] = diffs[3] = 0;
                if (bctrees[i][0] >= 0) {
                    bitpos=generate_mintree_node(bctrees[i], 0, buf, bufsize, bitpos, diffs, force_zero);
                }
                for (p = 0; p < 16; ++p) {
                    pixels[i][p] += diffs[p/4];
                }
            }
        
            // 1x1 mintree nodes
            for(j=0; j<4; j++) {
                for(i=0; i<ncomp; i++) {
                    int force_zero = (bctrees[0][0] == -2) &&
                        ((i == 1) || (i == 2));
                    
                    diffs[0] = diffs[1] = diffs[2] = diffs[3] = 0;
                    if (bctrees[i][0]>=0) {
                        bitpos=generate_mintree_node(bctrees[i], child(0,j), buf, bufsize, bitpos, diffs, force_zero);
                    }
                    for (p = 0; p < 4; ++p) {
                        pixels[i][j*4+p] += diffs[p];
                        pixels[i][j*4+p] %= (1 << info->compbits[i]);
                    }
                }
            }

            if (!info->yuv_transform || test_transformability(info, pixels)) break;
        }

        if (tries_left == 0 && !quiet_mode) {
            puts("This bctree was too hard... let's try another one!");
        }
    }
    while (tries_left == 0);
    
    if (info->yuv_transform) {
        apply_yuv_transform(info, pixels);
    }

/*    
      for(p=0; p<16; p++) {
      printf("(%x %x %x %x) ", pixels[0][p], pixels[1][p], pixels[2][p], pixels[3][p]);
      if (p%4 == 3) printf("\n");
      }
*/   
    
    // If it does not fit or if the size is greater than the threshold 
    // for uncompressed, then store the block uncompressed (before YUV transform)
    if (bitpos != -1) {
        size = (bitpos+7) >> 3; // byte align
    } else {
        size = -1;
    }
    
//    printf("size became %d (%d) %d\n", size, info->uncompressed_size[plane], plane);

    
    sbsm = info->sbs_multiplier[plane];
    
    //if size is lesser than 2*sbsm align it
    if(size < 2*sbsm)
        size = 2*sbsm;
        
    //align the size to the sbsm multiplier    and then multiply
    size = (size+(sbsm - 1))/sbsm;
    size = size * sbsm;
    

    if (size==-1 || size>=info->uncompressed_size[plane]) {
        *color=-1;
        *uncompressed=1;
        size=generate_uncompressed_subblock(info, plane, buf, bufsize);
    } else {
        *uncompressed=0;
    }
    return size;
}

// Write uncompressed body data for the given subblock
// Return the size in byte or -1 in case of buffer overflow
static int emit_uncompressed_subblock(struct afbc_frame_info *info, int plane, unsigned char *buf, int bufsize) {
    int i, x, y, bitpos=0;
    
    // emit as uncompressed
    for(y=0; y<4; y++)
        for(x=0; x<4; x++)
            for(i=0; i<info->ncomponents[plane]; i++) {
                int bits=info->inputbits[info->first_component[plane]+i];
                bitpos=write_bits(buf, bufsize, bitpos, rand()%(1 << bits), bits);
            }
    if (bitpos==-1)
        return -1;
    return (bitpos+7)>>3; // byte align
}

// Generate the given superblock to the given mbx, mby block coordinate (in block16 units)
// Block incices are block[component][y][x]
// buf is the AFBC frame buffer
// bufsize is the size of buf in byte
// body_pos is a byte offset to where payload data will be written
// The return value is an updated body_pos
// -1 is returned in case of buffer overflow
int afbc_generate_superblock(struct afbc_frame_info *f,
                             struct afbc_generate_cfg *gcfg,
                             unsigned char *buf,
                             int bufsize,
                             int mbnum,
                             int body_pos,
                             int force_uncompressed,
                             int force_transition) {
    int i;
    long long color;
    int plane, uncompressed, subblock_size, header_bitpos=0, size=0;
    unsigned char *header_buf;
    int force_copy = 0;
    int prev_uncompressed[2] = {0,0};
    int prev_compressed[2] = {0,0};
    int sbsm;
    int subblk_type[2];
    int tmp[2];
    int copy_status;
    int body_pos_phalf;
    int force_unc_probability = gcfg->force_unc_probability;
    int copy_probability = gcfg->copy_probability;
    int force_solid_color = 0;
    int r;
    int pixels[4][16];
    int format_allows_solid_color;
    unsigned char tmp_buf[2*4*4*4*32/8]; // enough space for 4 components each of 32 bits + 100% margin

    // generate solid color block?
    if (gcfg->solid_probability >= (rand() % 101)) {
        force_solid_color = 1;
    }

    // increase the probability that some blocks a large number of copy or uncompressed blocks
    // also force the first few superblocks to be one of each extreme (all copies, all uncompressed, solid color)
    if (gcfg->allow_supertile_guiding) {
        if (mbnum == 2) {
            force_unc_probability = 100;
            force_solid_color = 0;
        } else if (mbnum == 3) {
            copy_probability = 100;
            force_solid_color = 0;
        } else if (mbnum == 4) {
            force_solid_color = 1;
        } else {
            r = rand()%100;
            if (r < 5) {
                //printf("super copy tile");
                copy_probability = rand()%101;
            } else if (r < 10) {
                //printf("super unc tile %d\n", force_unc_probability);
                force_unc_probability = rand()%101;
            }
        }
    }

    if (!quiet_mode) {
        printf("Processing %d\n", mbnum);
    }

    // Pointer to the header
    header_buf=buf + mbnum * AFBC_HEADER_SIZE;

    // only non-subsampled formats allow solid color
    format_allows_solid_color = ((f->superblock_layout == 0) ||
                                 (f->superblock_layout == 3) ||
                                 (f->superblock_layout == 4));

    // solid color? if so, just write a random color to the header and return unmodified body pos
    if (format_allows_solid_color && f->allow_solid_color && force_solid_color) {
        header_bitpos=write_bits(header_buf, 16, header_bitpos, 0, 64);
        for (i = 0; i < f->ncomponents[0] + f->ncomponents[1]; ++i) {
            header_bitpos=write_bits(header_buf, 16, header_bitpos, rand(), f->inputbits[i]);
        }
        header_bitpos=write_bits(header_buf, 16, header_bitpos, 0, 64-(f->inputbits[0]+f->inputbits[1]+f->inputbits[2]+f->inputbits[3]));
        return body_pos;
    }
    
    // Write body_base_ptr
    header_bitpos=write_bits(header_buf, 16, header_bitpos, body_pos, f->body_base_ptr_bits);

    // save the body pos + the size of half an uncompressed superblock
    // for use by the block split functionality
    // increment as if an uncompressed block for each block we go through
    body_pos_phalf = body_pos + afbc_get_max_superblock_payloadsize(f)/2;

    // Emit compressed block to temporary buffer.
    // This is done to avoid junk data in the output buffer in case we will write the block as uncompressed.
    // In addition to this, in helps avoiding overwriting payload data in tiled mode, because of out-of-order superblock generation
    memset(tmp_buf, 0, sizeof(tmp_buf));
    
    subblk_type[0] = (rand()%2)+1;
    subblk_type[1] = (rand()%2)+1;
    tmp[0] = 0;
    tmp[1] = 0;
    // Loop over all 4x4 subblocks
    for(i=0; i<f->nsubblocks; i++) {
        plane=f->subblock_order[i].plane;

        //use this to generate only if the force transition option is not specified  
        force_copy = 0;
        uncompressed = 0;     

        if(!(
               (gcfg->force_first_copy_blk) && (
                   ((sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_420) && (i<3))||
                   ((sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_422) && (i<2))||
                   ((sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_NONE) && (i==0))
                   )
               )) {
            if (!force_uncompressed && !force_transition) {
                int r = rand() % 100;
                if (r < copy_probability &&
                    !(((afbc_block_split_block(f) == i) ||
                       (afbc_block_split_block_alt(f) == i)) && f->block_split) &&
                    !(f->disable_copies_crossing_8x8 && afbc_first_block_in_8x8(f, i)) &&
                    (i != 0) && 
                    (i != 2 || sl_subsampling[f->superblock_layout] != AFBC_SUBSAMPLING_420) &&
                    (i != 1 || sl_subsampling[f->superblock_layout] != AFBC_SUBSAMPLING_422)) {
                    size = 0;
                    force_copy = 1;
                }
                else if (r < (copy_probability+force_unc_probability)) {
                    uncompressed = 1;               
                    size = f->uncompressed_size[plane];
                }
                else {
                    size=afbc_generate_block4x4(f, plane, tmp_buf, pixels, bufsize-body_pos, &color, &uncompressed);
                    force_copy = 0;
                }
            }
        }
       
        //use this if we wanted first sub-block to be copy block. 
        //This input is to test the DUT handling when the first sub-block is zero
        if(gcfg->force_first_copy_blk){
            //generate the first copy block according to the subsampling used
            if (sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_420) {
                //force first copy block,non-copy,copy block 
                //if(i<3) {
        		
                // 0 - copy block
                // 1 - Non copy block
                // 2 - Copy block
                //if(i%2 == 0) {
                //	size = 0;
                //	force_copy = 1;
                //} else {
                //	uncompressed = 1;
                //	size = f->uncompressed_size[plane];
                //}
                //}
                
                if(i < 3) {
                    copy_status = rand() % 3;
                
                    if(copy_status == 0) {
                        size = 0;
                        force_copy = 1;
                    } else if(copy_status == 1) {
                        size = f->uncompressed_size[plane];
                        uncompressed = 1;
                    } else {
                        size=afbc_generate_block4x4(f, plane, tmp_buf, pixels, bufsize-body_pos, &color, &uncompressed);
                    }
                }
                
            } else if (sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_422) {
                //force copy ,copy 
                //if(i<2){
                //	size = 0;
                //	force_copy = 1;
                //}
                
                if(i < 2) {
                    copy_status = rand() % 3;
                
                    if(copy_status == 0) {
                        size = 0;
                        force_copy = 1;
                    } else if(copy_status == 1) {
                        size = f->uncompressed_size[plane];
                        uncompressed = 1;
                    } else {
                        size=afbc_generate_block4x4(f, plane, tmp_buf, pixels, bufsize-body_pos, &color, &uncompressed);
                    }
                }
        	
            } else {
        	
                if(i == 0) {
                    copy_status = rand() % 3;
                
                    if(copy_status == 0) {
                        size = 0;
                        force_copy = 1;
                    } else if(copy_status == 1) {
                        size = f->uncompressed_size[plane];
                        uncompressed = 1;
                    } else {
                        size=afbc_generate_block4x4(f, plane, tmp_buf, pixels, bufsize-body_pos, &color, &uncompressed);
                    }
                }
            }
        }
                
        //here handle the additional generation
        if(!(
               (gcfg->force_first_copy_blk) && (
                   ((sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_420) && (i<3))||
                   ((sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_422) && (i<2))||
                   ((sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_NONE) && (i==0))
                   )
               )) {
            if(!force_uncompressed && force_transition) {
                force_copy = 0;
                uncompressed = 0;  
        	
                if((subblk_type[plane] == 0) && (i && (prev_uncompressed[plane]||prev_compressed[plane]))) {
                    force_copy = 1;
                    size = 0;
                } else if(subblk_type[plane] == 1){
                    uncompressed = 1;
                    size = f->uncompressed_size[plane];
                } else {
                    size=afbc_generate_block4x4(f, plane, tmp_buf, pixels, bufsize-body_pos, &color, &uncompressed);
                }	
        	
                //Make sure that the same sub-block type does not repeat
                tmp[plane] = rand()%3;
                while(subblk_type[plane] == tmp[plane]){
                    tmp[plane] = rand()%3;
                }
                subblk_type[plane] = tmp[plane];
            }
        } 
        
        
        if (!quiet_mode) {
            printf("i=%d plane=%d uncompressed=%d force_copy=%d body_pos=%d size=%d\n", i, plane, uncompressed, force_copy, body_pos, size);
        }
        
        if (size==-1)
            return -1;  // buffer overflow

        sbsm = f->sbs_multiplier[plane];
        
        //Size handling done generate 4x4 block itself
        //if (size<2*sbsm)
        //    size=2*sbsm;
        
        //prev_uncompressed[plane] = 0;

        // sometimes force a copy of previous block
        if (i && force_copy) {
            subblock_size=0; // copy previous
            size=0;

            gcfg->stat_copy += 1;
        } else if (uncompressed || force_uncompressed) {
            prev_uncompressed[plane] = 1;
            
            //No need to generate uncompressed sub-block payload within header case
            //just enough if we change the offset
            if(uncompressed){
            	size=emit_uncompressed_subblock(f, plane, buf+body_pos, bufsize-body_pos);
            }
            subblock_size=1; // uncompressed

            gcfg->stat_unc += 1;
        } else {
            // round up to nearest multiple of sbsm, then divide by sbsm
            subblock_size= size/ sbsm;
            prev_compressed[plane] = 1;

            // Copy compressed data to output buffer.
            memcpy(buf+body_pos, tmp_buf, size);
        }

        gcfg->stat_size[size]++;
        
        //printf("subblk index = %d , type = %d, copy = %d,uncompressed = %d \n",i,subblock_size,force_copy,uncompressed);
        if((rand()%100) < gcfg->force_malformed_header_size_prob) {
		
            //change the sub-block size if need be
            //always reduce the sub-block size, donot increase it
            if(subblock_size >= 4)
                subblock_size = subblock_size - (rand()%3);
				
            header_bitpos=write_bits(header_buf, 16, header_bitpos, subblock_size, f->subblock_size_bits);
        } else {
            header_bitpos=write_bits(header_buf, 16, header_bitpos, subblock_size, f->subblock_size_bits);
        }
        //printf("subblock_size = %d , size = %d, mbx = %d, mby =%d , subblock=%d\n",subblock_size,size,mbx,mby,i);
        body_pos+=size;

        if (f->block_split && (afbc_block_split_block(f)-1 == i)) {
            body_pos = body_pos_phalf;
        }

    }
    return body_pos;
}
