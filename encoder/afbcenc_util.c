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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <getopt.h>
#include <stdarg.h>
#include "afbcenc_util.h"
#include "afbc_common.h"
#include "afbc_encode.h"

// Copy a superblock from the given plane separated frame.
// Handle boundary extension if the source frame size is not a multiple of 16.
static void copy_block(struct afbc_frame_info *f, unsigned int *planes[4], int mbx, int mby, unsigned int block[4][256]) {
    int i, j, k, w, h, x, y, subx, suby, xx, yy, block_w, block_h, left_crop, top_crop;
    k=0;
    for(i=0; i<f->nplanes; i++) {
        // Width and height. Consider chroma subsampling
        if (sl_subsampling[f->superblock_layout]==AFBC_SUBSAMPLING_420 && i==1) {
            subx=2;
            suby=2;
        } else if (sl_subsampling[f->superblock_layout]==AFBC_SUBSAMPLING_422 && i==1) {
            subx=2;
            suby=1;
        } else {
            subx=1;
            suby=1;
        }
        w=f->width/subx;
        h=f->height/suby;
        block_w=f->mb_sizew/subx;
        block_h=f->mb_sizeh/suby;
        left_crop=f->left_crop/subx;
        top_crop=f->top_crop/suby;

        for(j=0; j<f->ncomponents[i]; j++, k++) {
            for(yy=0; yy<block_h; yy++) {
                for(xx=0; xx<block_w; xx++) {
                    x=max(min(mbx*block_w+xx-left_crop, w-1), 0);
                    y=max(min(mby*block_h+yy-top_crop, h-1), 0);
                    block[k][yy*f->mb_sizew+xx]=planes[k][y*w+x];
                }
            }
        }
    }
}

// Loop over all superblocks in the frame and encode them.
// Return size of encoded buffer
int encode_blocks(struct afbc_frame_info *f, unsigned int *planes[4], unsigned char *buf, int bufsize, int *compdata_size) {
    int mbx, mby, body_pos, old_body_pos, tmp_body_pos;
    int tile_uncompressed_size;
    int tsx = afbc_get_tile_size_x(f);
    int tsy = afbc_get_tile_size_y(f);
    int body_buf_start;

    // Start of body data is just after the header area
    body_pos = afbc_mb_round(f->mbh, tsy) * afbc_mb_round(f->mbw, tsx) * AFBC_HEADER_SIZE;

    // in tiled header mode, the body buffer start address must also be aligned to 4096 bytes, so round up to next multiple of 4096
    if (f->tiled) {
        body_pos = (body_pos + 4095) & ~4095;
    }

    // store the start of the body buffer, to be used later in sparse mode
    body_buf_start = body_pos;

    (*compdata_size) = f->mbh * f->mbw * AFBC_HEADER_SIZE;

    tile_uncompressed_size = afbc_get_max_superblock_payloadsize(f);

    // Loop over superblocks
    for(mby=0; mby<f->mbh; mby++) {
        for(mbx=0; mbx<f->mbw; mbx++) {
            unsigned int block[4][256];

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
            mbnum = f->header_row_stride ? tile*ptilesize + (mbx % tsx) : mbnum;

            copy_block(f, planes, mbx, mby, block);

            old_body_pos = body_pos;

            if(f->rtl_addressing) {
                body_pos = body_buf_start + mbnum*tile_uncompressed_size;
                old_body_pos = body_pos;
                tmp_body_pos = afbc_encode_superblock(f, block, buf, bufsize, mbnum, body_pos, 0, 0);
                (*compdata_size) += (tmp_body_pos - old_body_pos);
                body_pos = tmp_body_pos;
            } else {
                body_pos=afbc_encode_superblock(f, block, buf, bufsize, mbnum, body_pos, 0, 0);
                (*compdata_size) += (body_pos - old_body_pos);
            }

            if (body_pos==-1)
                fatal_error("ERROR: buffer overflow, internal consistency failure\n");
        }
    }
    return body_pos;
}

// Encode the given frame and write to file.
// Return size of encoded buffer
int encode_frame(struct afbc_frame_info *f, unsigned int *planes[4], int skip_fileheader, FILE *outfile) {
    unsigned char *buf;
    int bufsize, frame_size;
    unsigned char fileheader_buf[AFBC_FILEHEADER_SIZE];
    int actual;
    int compdata_size = 0;

    bufsize=afbc_get_max_frame_size(f);

    buf=(unsigned char*) malloc(bufsize);
#ifdef GATHER_BIN_STATS
    buf_start = buf;
#endif
    memset(buf,0,bufsize);
    if (!buf)
        fatal_error("ERROR: failed to allocate %d byte\n", bufsize);

    // Encode header and body data
    frame_size=encode_blocks(f, planes, buf, bufsize, &compdata_size);

    if (f->tiled) {
        frame_size = afbc_get_max_frame_size(f);
    }

    if (outfile) {
        if (!skip_fileheader) {
            // Prepare file header
            afbc_write_fileheader(f, frame_size, fileheader_buf);
            actual=fwrite(fileheader_buf, 1, sizeof(fileheader_buf), outfile);
            if (actual!=sizeof(fileheader_buf))
                fatal_error("ERROR: failed to write %d byte to the output file (%d)\n", sizeof(fileheader_buf), actual);
        }
        actual=fwrite(buf, 1, frame_size, outfile);
        if (actual!=frame_size)
            fatal_error("ERROR: failed to write %d byte to the output file (%d)\n", frame_size, actual);
    }
    free(buf);
    return compdata_size;
}
