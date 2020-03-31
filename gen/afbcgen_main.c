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
#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include "afbc_common.h"
#include "afbc_generate.h"

extern int quiet_mode;

void fatal_error(const char *format, ...) {
    va_list ap;
    va_start (ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    exit(1);
}

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))


#define PTR_TO_HDR_PROBABILITY 2
// Loop over all superblocks in the frame and encode them.
// Return size of encoded buffer
static int generate_blocks(struct afbc_frame_info *f, unsigned int *planes[4], unsigned char *buf, int bufsize, struct afbc_generate_cfg *gen_cfg) {
    int body_pos, body_pos_unc,body_alloc_start, body_alloc_stop,force_transition;
    int body_buf_start;

    uint32_t* prev_alloc_start = malloc(sizeof(uint32_t)*f->mbh * f->mbw);
    uint32_t* prev_alloc_stop  = malloc(sizeof(uint32_t)*f->mbh * f->mbw);

    uint32_t allocated = 0;
    uint32_t i = 0;

    // throw in a pointer to the middle of the header buffer sometimes
    uint32_t uncompressed_in_hdr_buf;
    
    int tile_uncompressed_size;
    int tsx = afbc_get_tile_size_x(f);
    int tsy = afbc_get_tile_size_y(f);

    int stripe_mbw;
    int stripe_mbh;
    int payload_stride_x;
    int payload_stride_y;
    int sx;
    int sy;


    if (sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_NONE) {
        tile_uncompressed_size = f->uncompressed_size[0]*16;
    } else if (sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_420) {
        tile_uncompressed_size = f->uncompressed_size[0]*16 +
            f->uncompressed_size[1]*4;
    } else if (sl_subsampling[f->superblock_layout] == AFBC_SUBSAMPLING_422) {
        tile_uncompressed_size = f->uncompressed_size[0]*16 +
            f->uncompressed_size[1]*8;
    }

    body_pos_unc = 0;
   
    // find random places to store the buffer for worst case size, check if the space has been used before, if it
    // has randomize again. 
    // The buffer sohuld be sufficiently large to handle the worst case of misaligned blocks.
    // Not intended to be a fast algorithm
    body_pos = f->mbh * f->mbw * AFBC_HEADER_SIZE;


    // Start of body data is just after the header area
    body_pos = afbc_mb_round(f->mbh, tsy) * afbc_mb_round(f->mbw, tsx) * AFBC_HEADER_SIZE;

    // in tiled header mode, the body buffer start address must also be aligned to 4096 bytes, so round up to next multiple of 4096
    if (f->tiled) {
        body_pos = (body_pos + 4095) & ~4095;
    }

    // store the start of the body buffer, to be used later in sparse mode
    body_buf_start = body_pos;

    if (gen_cfg->payload_alignment != 0) {
        body_buf_start = (body_buf_start + (gen_cfg->payload_alignment-1)) & ~(gen_cfg->payload_alignment-1);
    }

    body_buf_start += gen_cfg->payload_offset;
    

    tile_uncompressed_size = afbc_get_max_superblock_payloadsize(f);



    if (f->tiled) {
        // in tiled mode, default to do a stripe of 8x1 superblocks (ie one line in the tile at a time)
        stripe_mbw = afbc_get_tile_size_x(f);
        stripe_mbh = 1;

        // possibly modify stripe_height
        if (gen_cfg->stripe_height != 0) {
            stripe_mbh = gen_cfg->stripe_height / superblock_height[f->superblock_layout];
        }

        // in tiled mode, the stride between two stripes in x is the size of the paging tile
        payload_stride_x = tsx * tsy * tile_uncompressed_size;
        // stride in y is the size of one stripe
        payload_stride_y = stripe_mbw * stripe_mbh * afbc_get_max_superblock_payloadsize(f);
    }
    else {
        // in linear mode, the whole screen is treated as one stripe by default
        stripe_mbw = f->mbw;
        stripe_mbh = f->mbh;

        // possibly modify stripe_height
        if (gen_cfg->stripe_height != 0) {
            stripe_mbh = gen_cfg->stripe_height / superblock_height[f->superblock_layout];
        }

        // there is no stride in x for linear mode
        payload_stride_x = 0;
        // stride in y is the size of one stripe
        // if RTL addressing is used, each superblock size is rounded up to 128 bytes
        // otherwise, the whole payload_stride_y is rounded up to 128 bytes
        if (gen_cfg->rtl_addressing) {
            payload_stride_y = stripe_mbw * stripe_mbh * afbc_get_max_superblock_payloadsize(f);
        }
        else {
            payload_stride_y = stripe_mbw * stripe_mbh * afbc_get_max_superblock_payloadsize_no_rounding(f);
            if (gen_cfg->payload_alignment != 0) {
                payload_stride_y = (payload_stride_y + (gen_cfg->payload_alignment-1)) & ~(gen_cfg->payload_alignment-1);
            }
        }
    }


    for (sy = 0; sy < f->mbh; sy += stripe_mbh) {
        for (sx = 0; sx < f->mbw; sx += stripe_mbw) {
            // loop through all strips, and for each:
            // 1. record all superblocks in this stripe in an ordered list
            // 2. optionally shuffle superblock list
            // 3. set up body pos for the start of the stripe
            // 4. loop through the blocks

            int x, y;
            int stripe_mbxs[stripe_mbw*stripe_mbh];
            int stripe_mbys[stripe_mbw*stripe_mbh];
            int stripe_num_blocks = 0;
            int blk;

            // 1. record all superblocks in this stripe in an ordered list
            for (y = sy; y < sy + stripe_mbh; y++) {
                for (x = sx; x < sx + stripe_mbw; x++) {
                    if (x < f->mbw && y < f->mbh) {
                        stripe_mbxs[stripe_num_blocks] = x;
                        stripe_mbys[stripe_num_blocks] = y;
                        stripe_num_blocks += 1;
                    }
                }
            }

            // 2. optionally shuffle superblock list
            if (gen_cfg->randomize_stripe) {
                // shuffling works by looping through each block, and for each block find another block to swap places with
                for (blk = 0; blk < stripe_num_blocks; blk++) {
                    int target_blk = rand() % stripe_num_blocks;
                    int tmp;
             
                    // swap x
                    tmp = stripe_mbxs[target_blk];
                    stripe_mbxs[target_blk] = stripe_mbxs[blk];
                    stripe_mbxs[blk] = tmp;
                    
                    // swap y
                    tmp = stripe_mbys[target_blk];
                    stripe_mbys[target_blk] = stripe_mbys[blk];
                    stripe_mbys[blk] = tmp;
                }
            }

            // 3. set up body pos for the start of the stripe
            if (f->tiled) {
                // in tiled mode, payload_stride_y is the stride inside a tile. we also need to add the stride between rows of tiles
                int tile_row = sy / afbc_get_tile_size_y(f);
                int tile_row_size = afbc_mb_round(f->mbw, tsx) * afbc_get_tile_size_y(f) * afbc_get_max_superblock_payloadsize(f);
                body_pos = body_buf_start + (sx/stripe_mbw) * payload_stride_x + ((sy%afbc_get_tile_size_y(f))/stripe_mbh) * payload_stride_y + tile_row * tile_row_size;
            }
            else {
                body_pos = body_buf_start + (sx/stripe_mbw) * payload_stride_x + (sy/stripe_mbh) * payload_stride_y;
            }




            // 4. loop through the blocks
            for (blk = 0; blk < stripe_num_blocks; blk++) {
                int mbx = stripe_mbxs[blk];
                int mby = stripe_mbys[blk];


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

                uncompressed_in_hdr_buf = 0;

                if (!gen_cfg->randomize_mem && uncompressed_in_hdr_buf && ((f->mbh * f->mbw * AFBC_HEADER_SIZE-tile_uncompressed_size) > 0)) { 
                    body_pos= rand()%(f->mbh * f->mbw * AFBC_HEADER_SIZE-tile_uncompressed_size); 
                    prev_alloc_start[allocated]  = body_pos;
                    force_transition = gen_cfg->force_transition;
                    body_pos=afbc_generate_superblock(f, gen_cfg, buf, bufsize, mbnum, body_pos, uncompressed_in_hdr_buf, force_transition);
                    prev_alloc_stop[allocated++] = body_pos;
                } else {
            
                    if(gen_cfg->force_payload_in_header) {
                        uncompressed_in_hdr_buf = (
                            ((rand()%100) < gen_cfg->force_unc_probability) &&  
                            ((f->mbh * f->mbw)* AFBC_HEADER_SIZE > afbc_get_max_superblock_payloadsize(f))
                            );
                    
                        // First check if this is greater than zero atleast , only then move onto payload withion
                        // header generation. When (((f->mbh * f->mbw)* AFBC_HEADER_SIZE) - (afbc_get_max_superblock_payloadsize(f))) == 0
                        // the rand() function gets angry !                         
                        if(
                            (((f->mbh * f->mbw)* AFBC_HEADER_SIZE) - (afbc_get_max_superblock_payloadsize(f))) > 0
                            )
                        {
                            body_pos_unc = rand()%(((f->mbh * f->mbw)* AFBC_HEADER_SIZE) - (afbc_get_max_superblock_payloadsize(f)));
                        }
                        else
                        {
                            uncompressed_in_hdr_buf = 0;
                        }
                    
                    }else{
                        uncompressed_in_hdr_buf = 0;
                    }
                

                    if (gen_cfg->randomize_mem && !uncompressed_in_hdr_buf) {
                        while (1) {
                            int already_used = 0;

                            body_alloc_start = (f->mbh * f->mbw * AFBC_HEADER_SIZE) +
                                (rand()%((bufsize - (f->mbh * f->mbw * AFBC_HEADER_SIZE))-
                                         tile_uncompressed_size));
                        
                            body_alloc_stop = body_alloc_start + tile_uncompressed_size;

                            for (i = 0; i < allocated; ++i) {
                                // is the suggested allocation within a previous allocation
                                // prev allocations are guaranteed to me at most as big as the new allocation
//                            printf("sug %d (%d-%d)\n", body_alloc_start, prev_alloc_start[i], prev_alloc_stop[i]);
                            
                                if (((body_alloc_start <= prev_alloc_start[i]) && 
                                     (body_alloc_stop  >  prev_alloc_start[i])) ||
                                    ((body_alloc_start <= prev_alloc_stop[i]) && 
                                     (body_alloc_stop  >  prev_alloc_stop[i]))
                                    ) {
                                    already_used = 1;
                                    break;
                                }
                            }
                            if (already_used == 0) break;
                        }
                        body_pos = body_alloc_start;

                        prev_alloc_start[allocated]  = body_pos;
                    } 

                
//                printf("send body_pos 0x%x (force: %d)\n", body_pos, uncompressed_in_hdr_buf);
                    force_transition = gen_cfg->force_transition;
                    //printf("%d %d: 0x%08x\n", mbx, mby, body_pos);
                    if(gen_cfg->rtl_addressing) {
                        body_pos = body_buf_start + mbnum*tile_uncompressed_size;
                        if (afbc_generate_superblock(f, gen_cfg, buf, bufsize, mbnum, body_pos, uncompressed_in_hdr_buf, force_transition) == -1) {
                            body_pos = -1;
                        }
                        else {
                            body_pos += afbc_get_max_superblock_payloadsize(f);
                        }
                    } else if(uncompressed_in_hdr_buf){
                        body_pos_unc=afbc_generate_superblock(f, gen_cfg, buf, bufsize, mbnum, body_pos_unc, uncompressed_in_hdr_buf, force_transition);
                    }else{
                        body_pos=afbc_generate_superblock(f, gen_cfg, buf, bufsize, mbnum, body_pos, uncompressed_in_hdr_buf, force_transition);
                    }

                    if (gen_cfg->randomize_mem) {
                        prev_alloc_stop[allocated++] = body_pos;
                    }
                }
                if (body_pos==-1)
                    fatal_error("ERROR: buffer overflow, internal consistency failure\n");
            }
        }
    }
    return body_pos;
}


// generate a frame with random content
static int generate_frame(struct afbc_frame_info *f, unsigned int *planes[4], int skip_fileheader, FILE *outfile, struct afbc_generate_cfg *gen_cfg) {
    unsigned char *buf;
    int bufsize, frame_size;
    unsigned char fileheader_buf[AFBC_FILEHEADER_SIZE];
    int actual;

    bufsize=2*afbc_get_max_frame_size(f);
    //printf("w=%d h=%d nsubblocks=%d superblock_layout=%d nplanes=%d uncompressed_size[0]=%d\n", 
    //    frame_info->width,
    //    frame_info->height,
    //    frame_info->nsubblocks,
    //    frame_info->superblock_layout,
    //    frame_info->nplanes,
    //    frame_info->uncompressed_size[0]);
    //printf("max_frame_size=%d\n", bufsize);
    buf=malloc(bufsize);
    if (!buf)
        fatal_error("ERROR: failed to allocate %d byte\n", bufsize);
    
    // Encode header and body data
    generate_blocks(f, planes, buf, bufsize, gen_cfg);

    frame_size=2*afbc_get_max_frame_size(f);

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

    free(buf);
    return frame_size;
}

static void print_usage(void) {
    printf("Usage: afbcgen [options] -w <width> -h <height> -f <format> -o <outfile.afb>\n");
    printf("Options:\n");
    printf("   -o <file>        resulting AFBC file\n");
    printf("   -w <widht>       specify width\n");
    printf("   -h <height>      specify height\n");
    printf("   -p               print extensive compression info\n");
    printf("   -q               quiet mode\n");
    printf("   -l               block split mode\n");
    printf("   -f               specify output format (ex. \"r8g8b8\" \"yuv422\" \"yuv420_10b\")\n");
    printf("   -e               force a seed\n");
    printf("   -j               Specify a superblock layout. The selected superblock layout must match the subsampling of the input format after conversion\n");
    printf("   -s <frame nbr>   first frame to encode (default is 0)\n");
    printf("   -n <frames>      number of frames to encode (default is all frames)\n");
    printf("   -z               skip AFBC file header\n");
    printf("   -m               Randomize the location in memory\n");
    printf("   -r               Use addressing with the sparse rtl mode where each payload is spaced away the size of 256 pixels from each other\n");
    printf("   -c <copy prob>   Set the likelihood of a copy being forced on a 4x4 in percent\n");
    printf("   -u <unc prob>    Set the likelihood of uncompressed being forced for a whole superblock\n");
    printf("   -d               disable yuv transform\n");
    printf("   -x               Force different transitions on the sub-block payload type in the header\n");
    printf("   -a               Force the payload in the header region\n");
//    printf("   -b               Malform the sub-block size in the header\n");
    printf("   -g               Force the first first block in every plane to be a copy block\n");
    printf("   -k               Specify an offset in bytes that is added after aligning payload addresses\n");
    printf("   -t               Specify a payload alignment. The alignment applied to the start of the buffer and/or each stripe\n");
    printf("   -y               Specify stripe height in pixels (height must be a multiple of superblock height). Blocks within a stripe are packed together in a semi-random order\n");
    printf("   -R               Randomize the order of encoded payload data in a stripe\n");
    printf("   -0               allow copies over 8x8 block borders\n");
    printf("   -S <solid prob>  Set the likelihood of a superblock using solid color optimation. Set to 0 to turn off this feature\n");
    printf("   -T               Use tiled mode\n");
    exit(1);
}


int main (int argc, char *argv[]) {
    int c, w=0, h=0, skip_fileheader=0;
    char *fileformat_name=0;
    char *outfile_name=0;
    FILE *outfile;
    unsigned int *planes[4];
    int inputbits[4];
    int disable_yuv_transform = 0;
    int block_split = 0;
    int superblock_layout = -1; // default is to pick the layout from format
    struct afbc_frame_info frame_info;
    struct afbc_generate_cfg gen_cfg;
    int x, y;
    int disable_copies_crossing_8x8_if_yuv = 1;
    int tiled = 0;
    int allow_solid_color = 1;

    gen_cfg.allow_supertile_guiding = 1;
    gen_cfg.randomize_mem = 0;
    gen_cfg.copy_probability = 10;
    gen_cfg.force_unc_probability = 10;
    gen_cfg.solid_probability = 5;
    gen_cfg.force_transition = 0;
    gen_cfg.force_payload_in_header = 0;
    gen_cfg.force_malformed_header_size_prob = 0;
    gen_cfg.force_first_copy_blk = 0;
    gen_cfg.rtl_addressing = 0;
    gen_cfg.payload_offset = 0;
    gen_cfg.payload_alignment = 0;
    gen_cfg.stripe_height = 0;
    gen_cfg.randomize_stripe = 0;

    gen_cfg.stat_copy = 0;
    gen_cfg.stat_unc  = 0;

    for (x = 0; x < 65; ++x) {
        gen_cfg.stat_size[x] = 0;
    }

    // Parse command line arguments
    opterr = 0;
    while ((c = getopt (argc, argv, "e:b:f:o:w:c:u:h:j:k:t:y:S:0dlarmgqzxT?R")) != -1)
        switch (c) {
        case 'w':
            w = atoi(optarg);
            break;
        case 'e':
            srand(atoi(optarg));
            break;
        case 'h':
            h = atoi(optarg);
            break;
        case 'd':
            disable_yuv_transform=1;
            break;
        case 'l':
            block_split=1;
            gen_cfg.rtl_addressing=1;
            break;
        case 'o':
            outfile_name=optarg;
            break;
        case 'f':
            fileformat_name=optarg;
            break;
        case 'j':
            superblock_layout = atoi(optarg);
            break;
        case 'm':
            gen_cfg.randomize_mem = 1;
            break;
        case 'c':
            gen_cfg.allow_supertile_guiding = 0;
            gen_cfg.copy_probability = atoi(optarg);
            break;
        case 'u':
            gen_cfg.allow_supertile_guiding = 0;
            gen_cfg.force_unc_probability = atoi(optarg);
            break;
        case 'r':
            gen_cfg.rtl_addressing=1;
            break;
        case 'q':
            quiet_mode=1;
            break;
        case 'z':
            skip_fileheader=1;
            break;
        case 'x':
            gen_cfg.force_transition = 1;
            break;
        case 'a':
            gen_cfg.force_payload_in_header = 1;
            break;
        case 'b':
            gen_cfg.force_malformed_header_size_prob = atoi(optarg);
            break;
        case 'g':
            gen_cfg.allow_supertile_guiding = 0;
            gen_cfg.force_first_copy_blk = 1;
            break;
        case 'k':
            gen_cfg.payload_offset = atoi(optarg);
            break;
        case 't':
            gen_cfg.payload_alignment = atoi(optarg);
            break;
        case 'y':
            gen_cfg.stripe_height = atoi(optarg);
            break;
        case 'R':
            gen_cfg.randomize_stripe = 1;
            break;
        case '0':
            disable_copies_crossing_8x8_if_yuv = 0;
            break;
        case 'S':
            gen_cfg.solid_probability = atoi(optarg);
            allow_solid_color = gen_cfg.solid_probability != 0;
            break;
        case 'T':
            tiled = 1;
            break;
        case '?':
        default:
            print_usage();
            exit(0);
        }
    
    if (!w || !h || !fileformat_name  || !outfile_name) {
        
        
        if(!w)
            printf("width \n");
        	
        if(!h)
            printf("height \n");
        	
        if(!fileformat_name)
            printf("fileformat_name \n");
        	
        if(!outfile_name)
            printf("outfile_name \n");
        	
        print_usage();
        		
        exit(0);
    }

    if (strcmp(fileformat_name,"r5g6b5")==0) {
        inputbits[0]=5, inputbits[1]=6, inputbits[2]=5, inputbits[3]=0;
        afbc_init_frame_info(&frame_info, w, h, 3, superblock_layout, !disable_yuv_transform, tiled, inputbits);
    } else if (strcmp(fileformat_name,"r5g5b5a1")==0) {
        inputbits[0]=5, inputbits[1]=5, inputbits[2]=5, inputbits[3]=1;
        afbc_init_frame_info(&frame_info, w, h, 4, superblock_layout, !disable_yuv_transform, tiled, inputbits);
    } else if (strcmp(fileformat_name,"r10g10b10a2")==0) {
        inputbits[0]=10, inputbits[1]=10, inputbits[2]=10, inputbits[3]=2;
        afbc_init_frame_info(&frame_info, w, h, 4, superblock_layout, !disable_yuv_transform, tiled, inputbits);
    } else if (strcmp(fileformat_name,"r11g11b10")==0) {
        inputbits[0]=11, inputbits[1]=11, inputbits[2]=10, inputbits[3]=0;
        afbc_init_frame_info(&frame_info, w, h, 3, superblock_layout, 0, tiled, inputbits);
    } else if (strcmp(fileformat_name,"r8g8b8")==0) {
        inputbits[0]=8, inputbits[1]=8, inputbits[2]=8, inputbits[3]=0;
        afbc_init_frame_info(&frame_info, w, h, 3, superblock_layout, !disable_yuv_transform, tiled, inputbits);
    } else if (strcmp(fileformat_name,"r8g8b8a8")==0) {
        inputbits[0]=8, inputbits[1]=8, inputbits[2]=8, inputbits[3]=8;
        afbc_init_frame_info(&frame_info, w, h, 4, superblock_layout, !disable_yuv_transform, tiled, inputbits);
    } else if (strcmp(fileformat_name,"r4g4b4a4")==0) {
        inputbits[0]=4, inputbits[1]=4, inputbits[2]=4, inputbits[3]=4;
        afbc_init_frame_info(&frame_info, w, h, 4, superblock_layout, !disable_yuv_transform, tiled, inputbits);
    } else if (strcmp(fileformat_name,"r8")==0) {
        inputbits[0]=8, inputbits[1]=0, inputbits[2]=0, inputbits[3]=0;
        afbc_init_frame_info(&frame_info, w, h, 1, superblock_layout, 0, tiled, inputbits);
    } else if (strcmp(fileformat_name,"r8g8")==0) {
        inputbits[0]=8, inputbits[1]=8, inputbits[2]=0, inputbits[3]=0;
        afbc_init_frame_info(&frame_info, w, h, 2, superblock_layout, 0, tiled, inputbits);
    } else if (strncmp(fileformat_name,"yuv", 3)==0) {
        if (strstr(fileformat_name, "10b") != 0) {
            inputbits[0]=10, inputbits[1]=10, inputbits[2]=10, inputbits[3]=0;
        } else {
            inputbits[0]=8, inputbits[1]=8, inputbits[2]=8, inputbits[3]=0;
        }
        if (strstr(fileformat_name, "420") != 0) {
            if (superblock_layout == -1) {
                superblock_layout = 1;
            }
        }
        else {
            if (superblock_layout == -1) {
                superblock_layout = 2;
            }
        }
        afbc_init_frame_info(&frame_info, w, h, 3, superblock_layout, 0, tiled, inputbits);
    } else {
        fatal_error("ERROR: unknown input file format %s\n", fileformat_name);
    }

    if (!disable_copies_crossing_8x8_if_yuv && (sl_subsampling[frame_info.superblock_layout]==AFBC_SUBSAMPLING_420 ||
                                                sl_subsampling[frame_info.superblock_layout]==AFBC_SUBSAMPLING_422)) {
        frame_info.disable_copies_crossing_8x8 = 0;
    }

    frame_info.block_split = block_split;

    frame_info.allow_solid_color = allow_solid_color;

    if (outfile_name) {
        outfile=fopen(outfile_name, "wb");
        if (!outfile)
            fatal_error("ERROR: cannot open outfile %s\n", outfile_name);
    } else {
        fatal_error("ERROR: cannot open outfile %s\n", outfile_name);
        return -1;
    }

    generate_frame(&frame_info, planes, skip_fileheader, outfile, &gen_cfg);

    if (!quiet_mode) {
        int total_blocks = (w/4)*(h/4);
        if (sl_subsampling[frame_info.superblock_layout] == AFBC_SUBSAMPLING_420) {
            total_blocks += (w/4/2)*(h/4/2);
        }
        else if (sl_subsampling[frame_info.superblock_layout] == AFBC_SUBSAMPLING_422) {
            total_blocks += (w/4/2)*(h/4);
        }

        printf("============================\nGenerate:\n%d copy block\n%d uncompressed block\n%d blocks total\n", gen_cfg.stat_copy, gen_cfg.stat_unc, total_blocks);
        
        printf("\nDistribution of block size:\n");
        for (y = 10; y >= 0; --y) {
            for (x = 0; x < 65; ++x) {
                if (gen_cfg.stat_size[x]*frame_info.uncompressed_size[0]*5 > y*((w/4)*(h/4))) {
                    printf("#");
                } else {
                    printf(" ");
                }
            }
            printf("\n");
        }
    }

    return 0;
}
