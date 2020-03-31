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
#include <getopt.h>
#include <stdarg.h>
#include "afbc_common.h"
#include "afbc_decode.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

int disable_fatal_errors;
int error_happened;

void fatal_error(const char *format, ...) {
  va_list ap;

  error_happened = 1;
  if (disable_fatal_errors) return;

  va_start (ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  exit(1);
}

void report_conformance_failure() {
    printf("ERROR: This buffer does not pass conformance!\n");
    fprintf(stderr,"ERROR: This buffer does not pass conformance!\n");
}

////////////////////////////// Frame allocation

static void alloc_planes(struct afbc_frame_info *f, unsigned int *planes[4]) {
    int i;
    int n=f->ncomponents[0]+f->ncomponents[1];
    int xsubsample=sl_subsampling[f->superblock_layout]==AFBC_SUBSAMPLING_420 || sl_subsampling[f->superblock_layout]==AFBC_SUBSAMPLING_422;
    int ysubsample=sl_subsampling[f->superblock_layout]==AFBC_SUBSAMPLING_420;
    for(i=0; i<4; i++) {
        if (i<n) {
            int ww = (xsubsample && i!=0) ? (f->width+1)/2 : f->width;
            int hh = (ysubsample && i!=0) ? (f->height+1)/2 : f->height;
            planes[i]=malloc(ww*hh*sizeof(unsigned int));
            assert(planes[i]);
            memset(planes[i], 0x80, ww*hh*sizeof(unsigned int));
        } else {
            planes[i]=0;
        }
    }
}

static void copy_planes(struct afbc_frame_info *f,
                        unsigned int *dst_planes[4],
                        unsigned int *src_planes[4]) {
    int i;
    int n=f->ncomponents[0]+f->ncomponents[1];
    int xsubsample=sl_subsampling[f->superblock_layout]==AFBC_SUBSAMPLING_420 || sl_subsampling[f->superblock_layout]==AFBC_SUBSAMPLING_422;
    int ysubsample=sl_subsampling[f->superblock_layout]==AFBC_SUBSAMPLING_420;
    for(i=0; i<4; i++) {
        if (i<n) {
            int ww = (xsubsample && i!=0) ? (f->width+1)/2 : f->width;
            int hh = (ysubsample && i!=0) ? (f->height+1)/2 : f->height;
            dst_planes[i]=malloc(ww*hh*sizeof(unsigned int));
            assert(dst_planes[i]);
            memcpy(dst_planes[i], src_planes[i], ww*hh*sizeof(unsigned int));
        } else {
            dst_planes[i]=0;
        }
    }
}

static void free_planes(struct afbc_frame_info *f, unsigned int *planes[4]) {
    int i, n=f->ncomponents[0]+f->ncomponents[1];
    for(i=0; i<n; i++)
        free(planes[i]);
}


////////////////////////////// Write frame raw file

// Write plane separated raw format by interleaving input from two field planes
// Depending on inputbits for the component, 8 or 16 bit resolution is used.
static void write_plane_separated_fields_interleaved(struct afbc_frame_info *f,
        unsigned int *top_planes[4],
        unsigned int *bot_planes[4],
        FILE *outfile)
{
    int n=f->ncomponents[0]+f->ncomponents[1];
    int xsubsample=sl_subsampling[f->superblock_layout]==AFBC_SUBSAMPLING_420 || sl_subsampling[f->superblock_layout]==AFBC_SUBSAMPLING_422;
    int ysubsample=sl_subsampling[f->superblock_layout]==AFBC_SUBSAMPLING_420;
    int nfields = bot_planes?2:1;
    int i, j;

    for(i=0; i<n; i++) {
        int elemsize=(f->inputbits[i]+7)/8;
        int shift_num = f->msb_aligned_output ? elemsize*8 - f->inputbits[i]: 0;

        int w = (xsubsample && i!=0) ? (f->width+1)/2 : f->width;
        int h = (ysubsample && i!=0) ? (f->height+1)/2 : f->height;
        int size=nfields*w*h;

        void *buf=malloc(size*elemsize);

        for(j=0; j<(size/nfields); j++) {
            int x = j%w, y=j/w;
            if (elemsize==1) {
                ((unsigned char*)buf)[x+nfields*w*y]=top_planes[i][j] << shift_num;
            } else {
                ((unsigned short*)buf)[x+nfields*w*y]=top_planes[i][j] << shift_num;
            }
            if (bot_planes) {
                if (elemsize==1) {
                    ((unsigned char*)buf)[x+w*(2*y+1)]=bot_planes[i][j] << shift_num;
                } else {
                    ((unsigned short*)buf)[x+w*(2*y+1)]=bot_planes[i][j] << shift_num;
                }
            }
        }

        fwrite(buf, elemsize, size, outfile);
        free(buf);
    }
}


// An afbc file may contain a number of frames or fields. If output is plane
// separated, fields will be combined to a frame before written to the output
// file
#define FILE_MSG_TOP_FIELD 1
#define FILE_MSG_BOT_FIELD 2

static void write_plane_separated_frame(struct afbc_frame_info *f,
        unsigned int *planes[4], FILE *outfile)
{
    static int prev_type = 0;
    static unsigned int *top_planes[4] = {0};
    static unsigned int *bot_planes[4] = {0};
    int type = f->file_message&(FILE_MSG_TOP_FIELD|FILE_MSG_BOT_FIELD);

    if (type == FILE_MSG_TOP_FIELD)
    {
        if (prev_type == FILE_MSG_TOP_FIELD)
        {
            // bot field is missing, create blank bot field and write both
            // fields to outfile before handling new top field
            alloc_planes(f, bot_planes);
            write_plane_separated_fields_interleaved(f, top_planes, bot_planes, outfile);
            free_planes(f, bot_planes);
            free_planes(f, top_planes);
        }
        copy_planes(f, top_planes, planes);
    }
    else if (type == FILE_MSG_BOT_FIELD)
    {
        if (prev_type == FILE_MSG_BOT_FIELD)
        {
            // top field is missing, create blank top field
            alloc_planes(f, top_planes);
        }
        write_plane_separated_fields_interleaved(f, top_planes, planes, outfile);
        free_planes(f, top_planes);
    }
    else
    {
        write_plane_separated_fields_interleaved(f, planes, 0, outfile);
    }

    prev_type = type;
}

// Write packed raw format
// Depending on inputbits for the compoentn, 8 or 16 bit resolution is used.
static void write_packed_frame(struct afbc_frame_info *f, unsigned int *planes[4], FILE *outfile) {
    int n=f->ncomponents[0]+f->ncomponents[1];
    int i, j, elemsize=0, size, k;
    uint64_t data;
    unsigned char *buf;
    for(i=0; i<n; i++)
        elemsize+=f->inputbits[i];
    elemsize=(elemsize+7)/8; // byte align each pixel
    
    size=f->width*f->height;
    buf=malloc(size*elemsize);
    
    for(j=0; j<size; j++) {
        for(data=0, k=0, i=0; i<n; i++) {
            data |= ((uint64_t)planes[i][j])<<k;
            k+=f->inputbits[i];
        }
        for(i=0; i<elemsize; i++, data=data>>8)
            buf[j*elemsize+i]=data&0xff;
    }
    fwrite(buf, elemsize, size, outfile);
    free(buf);
}

// Write packed png format
static void write_png_frame(struct afbc_frame_info *f, char *outfile_name, unsigned int *planes[4]) {
    int n=f->ncomponents[0]+f->ncomponents[1];
    int i, j, elemsize=0, size;
    unsigned char *buf;
    elemsize=n;

    size=f->width*f->height;
    buf=malloc(size*elemsize);
    
    for(j=0; j<size; j++) {
        for(i=0; i<elemsize; i++)
            if (f->inputbits[i] <= 8) {
                buf[j*elemsize+i]=planes[i][j] << (8 - f->inputbits[i]);
            } else {
                buf[j*elemsize+i]=planes[i][j] >> (f->inputbits[i] - 8);
            }
    }

    stbi_write_png(outfile_name, f->width, f->height, f->ncomponents[0], buf, f->width*1*f->ncomponents[0]);
    free(buf);
}



static void write_r8g8b8_frame(struct afbc_frame_info *f, unsigned int *planes[4], FILE *outfile) {
    int n=f->ncomponents[0]+f->ncomponents[1];
    int i, j, elemsize=3, size, k;
    unsigned int data;
    unsigned char *buf;
    int comp_adjust[4];

    for (i = 0; i < n; ++i) {
        comp_adjust[i] = 8-f->inputbits[i];
    }

    size=f->width*f->height;
    buf=malloc(size*elemsize);

    for(j=0; j<size; j++) {
        for(data=0, k=0, i=0; i<max(n,3); i++) {
            if (comp_adjust[i] >= 0) {
                data |= (planes[i][j] << comp_adjust[i])<<k;
            } else {
                data |= (planes[i][j] >> (-comp_adjust[i]))<<k;
            }
            k+=8;
        }
        for(i=0; i<elemsize; i++, data=data>>8)
            buf[j*elemsize+i]=data&0xff;
    }

    fwrite(buf, elemsize, size, outfile);
    free(buf);
}

// no adjustments, 4 bit outputs are written non shifted
static void write_r16g16b16a16_frame(struct afbc_frame_info *f, unsigned int *planes[4], FILE *outfile) {
    int i, j, size;

    int zero = 0;

    size=f->width*f->height;

    for(j=0; j<size; j++) {
        for(i=0; i<4; i++) {
            if (f->ncomponents[0] <= i) {
                fwrite(&zero, 2, 1, outfile);
            } else {
                fwrite(&planes[i][j], 2, 1, outfile);
            }
        }
    }

}

// no adjustments, 4 bit outputs are written non shifted
static void write_r16g16b16a16_frame_shifted(struct afbc_frame_info *f, unsigned int *planes[4], FILE *outfile) {
    int i, j, size;

    int zero = 0;
    int ones = 0xffffffff;

    size=f->width*f->height;

    for(j=0; j<size; j++) {
        for(i=0; i<4; i++) {
            if (f->ncomponents[0] <= i) {
                if (i != 3) {
                    fwrite(&zero, 2, 1, outfile);
                } else {
                    fwrite(&ones, 2, 1, outfile);
                }
            } else {
                uint32_t value = planes[i][j] << (16-f->inputbits[i]);
                
                // adapt to imagemagick 
                fwrite(&value, 2, 1, outfile);
            }
        }
    }

}

////////////////////////////// AFBC decode

// Copy a superblock to the given plane separated frame.
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
        w=(f->width+subx-1)/subx;
        h=(f->height+suby-1)/suby;
        block_w=f->mb_sizew/subx;
        block_h=f->mb_sizeh/suby;
        left_crop=(f->left_crop+subx-1)/subx;
        top_crop=(f->top_crop+suby-1)/suby;

        for(j=0; j<f->ncomponents[i]; j++, k++) {
            for(yy=0; yy<block_h; yy++)
                for(xx=0; xx<block_w; xx++) {
                    x=mbx*block_w+xx-left_crop;
                    y=mby*block_h+yy-top_crop;
                    if (x>=0 && x<w && y>=0 && y<h)
                        planes[k][y*w+x]=block[k][yy*f->mb_sizew+xx];
                    //printf("%2d %2d %2d %2d %2d : %02x\n", i, j, k, xx, yy, block[k][yy][xx]);
                }
        }
    }
}

// Loop over all superblocks in the frame and decode them.
static void decode_blocks(struct afbc_frame_info *f, unsigned int *planes[4], unsigned char *buf, int bufsize) {
    int mbx, mby;

    // Set planes to 0x80 (gray) to have a well defined output in case mbw/mbh
    // varies (the coded_width/height may be lower than for previous frame
    int xsubsample=sl_subsampling[f->superblock_layout]==AFBC_SUBSAMPLING_420 || 
                   sl_subsampling[f->superblock_layout]==AFBC_SUBSAMPLING_422;
    int ysubsample=sl_subsampling[f->superblock_layout]==AFBC_SUBSAMPLING_420;
    int i;
    int ww;
    int hh;
    for(i=0; i<4; i++) {
        if (planes[i] == NULL) {
            continue;
        }
        ww = (xsubsample && i!=0) ? (f->width+1)/2 : f->width;
        hh = (ysubsample && i!=0) ? (f->height+1)/2 : f->height;
        memset(planes[i], 0x80, ww*hh*sizeof(unsigned int));
    }

    // Loop over superblocks
    for(mby=0; mby<f->mbh; mby++)
        for(mbx=0; mbx<f->mbw; mbx++) {
            unsigned int block[4][256];
            afbc_decode_superblock(f, buf, bufsize, mbx, mby, block);
            copy_block(f, planes, mbx, mby, block);
        }
}

extern unsigned int accum_unc;
extern unsigned int accum_b3;
extern unsigned int accum_b4;
extern unsigned int accum_m2;
extern unsigned int accum_m3;
extern unsigned int accum_m4;
extern unsigned int accum_used_tree_bits;
extern unsigned int accum_needed_tree_bits;


// Loop over all superblocks in the frame and decode them.
static void decode_frame(struct afbc_frame_info *frame_info, uint64_t frame_size, unsigned int *planes[4], FILE *infile) {
    unsigned char *buf;
    size_t actual;
    
    buf=malloc(frame_size); 
    if (!buf)
        fatal_error("ERROR: failed to allocate %d byte\n", frame_size);
   
    actual=fread(buf, 1, frame_size, infile);
    if (actual!=frame_size)
        fatal_error("ERROR: failed to read %lld byte from the input file (%lld)\n", frame_size, actual);
    
    // Decode header and body data
    decode_blocks(frame_info, planes, buf, frame_size);
    
    free(buf);
}

static void skip_frame(uint64_t frame_size, FILE *infile) {
    int ret=fseek(infile, frame_size, SEEK_CUR);
    if (ret!=0)
        fatal_error("ERROR: cannot skip frame %d\n", ret);
}

// Fills in frame_info and returns frame_size
static uint64_t read_frameheader(struct afbc_frame_info *frame_info, FILE *infile) {
    int actual, n, version;
    uint64_t frame_size;
    unsigned char frameheader_buf[AFBC_FILEHEADER_SIZE];
    actual=fread(frameheader_buf, 1, AFBC_FILEHEADER_SIZE, infile);
    if (actual==0)
        return 0; // end of file
    if (actual!=AFBC_FILEHEADER_SIZE)
        fatal_error("ERROR: cannot read a complete AFBC file header %d<%d\n", actual, AFBC_FILEHEADER_SIZE);
    version=afbc_parse_fileheader(frameheader_buf, frame_info, &frame_size);
    if (version<0)
        fatal_error("ERROR: the file is not an AFBC file, or it is an unsupported version %d (tool is of version %d)\n", -version, AFBC_VERSION);
    // Read potential extended file header
    n=frameheader_buf[4]-AFBC_FILEHEADER_SIZE;
    actual=fread(frameheader_buf, 1, n, infile);
    if (actual!=n)
        fatal_error("ERROR: cannot read additional AFBC file header %d<%d\n", actual, n);
    return frame_size;
}

static void decode_resolution_str(const char *res_str, int *width, int *height) {
    const char *p;
    char width_str[128];
    const char *height_str;

    p = strchr(res_str, 'x');
    assert(p != 0 && "Invalid resolution format, use WxH, e.g. 128x64.");
    memcpy(width_str, res_str, p - res_str);
    width_str[p - res_str] = '\0';
    height_str = p + 1;

    *width = atoi(width_str);
    *height = atoi(height_str);
}

static void decode_format_str(const char *format_str, int *ncomp, int *superblock_layout, int *inputbits) {
    if (strcmp(format_str, "r5g6b5")==0) {
        inputbits[0]=5, inputbits[1]=6, inputbits[2]=5, inputbits[3]=0;
        *ncomp = 3;
        *superblock_layout = 0;
    } else if (strcmp(format_str,"r5g5b5a1")==0) {
        inputbits[0]=5, inputbits[1]=5, inputbits[2]=5, inputbits[3]=1;
        *ncomp = 4;
        *superblock_layout = 0;
    } else if (strcmp(format_str,"r10g10b10a2")==0) {
        inputbits[0]=10, inputbits[1]=10, inputbits[2]=10, inputbits[3]=2;
        *ncomp = 4;
        *superblock_layout = 0;
    } else if (strcmp(format_str,"r11g11b10")==0) {
        inputbits[0]=11, inputbits[1]=11, inputbits[2]=10, inputbits[3]=0;
        *ncomp = 3;
        *superblock_layout = 0;
    } else if (strcmp(format_str,"r8g8b8")==0) {
        inputbits[0]=8, inputbits[1]=8, inputbits[2]=8, inputbits[3]=0;
        *ncomp = 3;
        *superblock_layout = 0;
    } else if (strcmp(format_str,"r8g8b8a8")==0) {
        inputbits[0]=8, inputbits[1]=8, inputbits[2]=8, inputbits[3]=8;
        *ncomp = 4;
        *superblock_layout = 0;
    } else if (strcmp(format_str,"r16")==0) {
        inputbits[0]=16, inputbits[1]=0, inputbits[2]=0, inputbits[3]=0;
        *ncomp = 1;
        *superblock_layout = 0;
    } else if (strcmp(format_str,"r16g16")==0) {
        inputbits[0]=16, inputbits[1]=16, inputbits[2]=0, inputbits[3]=0;
        *ncomp = 2;
        *superblock_layout = 0;
    } else if (strcmp(format_str,"r16g16b16")==0) {
        inputbits[0]=16, inputbits[1]=16, inputbits[2]=16, inputbits[3]=0;
        *ncomp = 3;
        *superblock_layout = 0;
    } else if (strcmp(format_str,"r16g16b16a16")==0) {
        inputbits[0]=16, inputbits[1]=16, inputbits[2]=16, inputbits[3]=16;
        *ncomp = 4;
        *superblock_layout = 0;
    } else if (strcmp(format_str,"r4g4b4a4")==0) {
        inputbits[0]=4, inputbits[1]=4, inputbits[2]=4, inputbits[3]=4;
        *ncomp = 4;
        *superblock_layout = 0;
    } else if (strcmp(format_str,"r8")==0) {
        inputbits[0]=8, inputbits[1]=0, inputbits[2]=0, inputbits[3]=0;
        *ncomp = 1;
        *superblock_layout = 0;
    } else if (strcmp(format_str,"r8g8")==0) {
        inputbits[0]=8, inputbits[1]=8, inputbits[2]=0, inputbits[3]=0;
        *ncomp = 2;
        *superblock_layout = 0;
    } else if (strcmp(format_str,"yuv4208b")==0) {
        inputbits[0]=8, inputbits[1]=8, inputbits[2]=8, inputbits[3]=0;
        *ncomp = 3;
        *superblock_layout = 1;

    } else if (strcmp(format_str,"yuv42010b")==0) {
        inputbits[0]=10, inputbits[1]=10, inputbits[2]=10, inputbits[3]=0;
        *ncomp = 3;
        *superblock_layout = 1;

    } else if (strcmp(format_str,"yuv4228b")==0) {
        inputbits[0]=8, inputbits[1]=8, inputbits[2]=8, inputbits[3]=0;
        *ncomp = 3;
        *superblock_layout = 2;

    } else if (strcmp(format_str,"yuv42210b")==0) {
        inputbits[0]=10, inputbits[1]=10, inputbits[2]=10, inputbits[3]=0;
        *ncomp = 3;
        *superblock_layout = 2;

    } else {
        printf("Format '%s' not supported in this mode.\n", format_str);
        assert(0 && "Format not supported");
    }
}

static int decode_int_default(const char *s, int def_value) {
    if (s) {
        return atoi(s);
    } else {
        return def_value;
    }
}
static uint64_t remaining_bytes_in_file(FILE *f) {
    uint64_t cur;
    uint64_t end;
    cur = ftell(f);
    fseek(f, 0, SEEK_END);
    end = ftell(f);
    fseek(f, cur, SEEK_SET);
    return end - cur;
}

// constructs a fake file header based on text string and parses it
static void parse_fake_file_header(struct afbc_frame_info *frame_info, const char *file_header, FILE *f, int quiet_mode) {
    char hdr_copy[128];

    int width, height;
    int color_transform;
    int block_split;
    int y_offset;
    int tiled = 0;

    int ncomponents;
    int superblock_layout;
    int inputbits[4];

    const char *res_str;
    const char *format_str;
    const char *color_transform_str;
    const char *block_split_str;
    const char *y_offset_str;
    const char *superblock_layout_str;
    const char *tiled_str;

    // split header into parts
    {
        const char *hdr_parts[7];
        char *p = hdr_copy;
        int hdr_parts_cnt = 0;
        
        int i;
        
        int inp_hdr_size;


        inp_hdr_size = strlen(file_header)+1;
        assert(inp_hdr_size < sizeof(hdr_copy));
        
        // copy header
        memcpy(hdr_copy, file_header, inp_hdr_size);
        
        for (i = 0; i < 7; i++) {
            hdr_parts[i] = 0;
        }
        
        // break up the parts of the header
        while (p != 0) {
            assert(hdr_parts_cnt < 7);
            hdr_parts[hdr_parts_cnt++] = p; // store part
            p = strchr(p, '_');
            if (p != 0) {
                *p = '\0'; // insert a null terminator between parts
                p += 1; // step past terminator
            }
        }
        
        res_str               = hdr_parts[0];
        format_str            = hdr_parts[1];
        color_transform_str   = hdr_parts[2];
        block_split_str       = hdr_parts[3];
        y_offset_str          = hdr_parts[4];
        superblock_layout_str = hdr_parts[5];
        tiled_str             = hdr_parts[6];
    }

    // parse header parts
    {
        assert(res_str != 0);
        decode_resolution_str(res_str, &width, &height);
        
        assert(format_str != 0);
        decode_format_str(format_str, &ncomponents, &superblock_layout, inputbits);
        
        color_transform   = decode_int_default(color_transform_str  , 0);
        block_split       = decode_int_default(block_split_str      , 0);
        y_offset          = decode_int_default(y_offset_str         , 0);
        superblock_layout = decode_int_default(superblock_layout_str, superblock_layout);
        tiled             = decode_int_default(tiled_str            , 0);

        assert(color_transform   >= 0 && color_transform   <=  1);
        assert(block_split       >= 0 && block_split       <=  1);
        assert(y_offset          >= 0 && y_offset          <  16);
        assert(superblock_layout >= 0 && superblock_layout <   7);
        assert(tiled             >= 0 && tiled             <=  1);
        
        if (!quiet_mode) {
            printf("Settings decoded from '%s':\n", file_header);
            printf("Size:            %dx%d\n", width, height);
            printf("Format:          %s\n", format_str);
            printf("Color Transform: %d\n", color_transform);
            printf("Header Layout:   %d\n", superblock_layout);
            printf("Tiled layout:    %d\n", tiled);
            printf("Block Split:     %d\n", block_split);
            printf("Y Offset:        %d\n", y_offset);
        }
    }

    // init frame
    {
        afbc_init_frame_info(frame_info, width, height, ncomponents, superblock_layout, color_transform, tiled, inputbits);
    
        frame_info->rtl_addressing = block_split;
        frame_info->block_split    = block_split;
        frame_info->top_crop       = y_offset;
        frame_info->tiled          = tiled;

        frame_info->frame_size     = remaining_bytes_in_file(f);
    }
}

static void print_info(struct afbc_frame_info *f, uint64_t frame_size, int frame_index);

// Try to guess the filesize if there is no fileheader
// this is based on the assumption that we are decoding a GPU buffer and will fail in many cases
static void guess_file_header(struct afbc_frame_info *frame_info, FILE *f, int quiet_mode) {
    int width, height;
    int color_transform;
    int block_split;
    int y_offset;
    int tiled;

    int ncomponents;
    int superblock_layout;
    int inputbits[4];

    int testres;

    unsigned char *buf;

    unsigned int block[4][256];

    int expected_diff;
    int offs;
    int new_offs;
    int itr;
    int w,h;
    size_t actual;

    int file_size;

    // find out some fundamentals of the file 
    file_size = remaining_bytes_in_file(f);

    
    buf=malloc(file_size);
    if (!buf)
        fatal_error("ERROR: failed to allocate %d byte\n", file_size);

    actual=fread(buf, 1, file_size, f);
    if (actual!=file_size)
        fatal_error("ERROR: failed to read %lld byte from the input file (%lld)\n", file_size, actual);

    // default resolution
    width = 256;
    height = 256;

    // quickly determine based on common sizes
    if (file_size == 8486400) {
        width  = 1920;
        height = 1088;
        expected_diff = 1024;
    } else {
        // find out how many headers we have
        offs = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
        itr = 1;
        new_offs = buf[itr*16+0] | (buf[itr*16+1] << 8) | (buf[itr*16+2] << 16) | (buf[itr*16+3] << 24);
        expected_diff = new_offs - offs;

        // this will fail for solid color
        if ((expected_diff == 1024) || (expected_diff == 512) || (expected_diff == 256) || (expected_diff == 2048)) {
            while ((new_offs - offs) == expected_diff) {
                itr++;
                offs = new_offs;
                new_offs = buf[itr*16+0] | (buf[itr*16+1] << 8) | (buf[itr*16+2] << 16) | (buf[itr*16+3] << 24);
            }

            // try to figure out a good set of dimensions that would match this size
            // we will assume landscape mode and make it ever wider until we hopefully hit the correct size
            w = h = sqrt(itr);

            while (w*h != itr) {
                if (w*h < itr) w++;
                else h--;
            }

            if (h == 1) {
                printf("Warning: Determined the frame to be 1 superblock high, there is most likely something wrong with the header parsing");
            }

            width = w*16;
            height = h*16;
        }
    }

    // hard to guess options
    superblock_layout = 0;
    block_split = 0;
    y_offset = 0;
    tiled = 0;

    // Take a guess at the format, only does rgba8, rgb8, r8 and rgb565 for now
    if (expected_diff == 1024) {
        ncomponents = 4;
        inputbits[0] = inputbits[1] = inputbits[2] = inputbits[3] = 8;
    } else if (expected_diff == 768) {
        ncomponents = 3;
        inputbits[0] = inputbits[1] = inputbits[2] = 8;
        inputbits[3] = 0;
    } else if (expected_diff == 512) {
        ncomponents = 3;
        inputbits[0] = 5;
        inputbits[1] = 6;
        inputbits[2] = 5;
        inputbits[3] = 0;
    } else if (expected_diff == 256) {
        ncomponents = 1;
        inputbits[0] = 8;
        inputbits[1] = inputbits[2] = inputbits[3] = 0;
    } else {
        assert(0); // don't know what this could be
    }

    // for the color_transform we will simply have to try
    color_transform = 1;

    afbc_init_frame_info(frame_info, width, height, ncomponents, superblock_layout, color_transform, tiled, inputbits);
    
    frame_info->rtl_addressing = block_split;
    frame_info->block_split    = block_split;
    frame_info->top_crop       = y_offset;
    frame_info->frame_size     = file_size;

    {
        disable_fatal_errors = 1;
        testres = afbc_decode_superblock(frame_info, buf, file_size, 0, 0, block);
        if ((testres == 0) || error_happened) {
            printf("Testing with color transform disabled\n");
            error_happened = 0;

            afbc_init_frame_info(frame_info, width, height, ncomponents, superblock_layout, color_transform, tiled, inputbits);
            testres = afbc_decode_superblock(frame_info, buf, file_size, 0, 0, block);

            assert((testres != 0) && !error_happened); // Couldn't decode
        }
        disable_fatal_errors = 0;
    }

    print_info(frame_info, file_size, 0);

    rewind(f);

    free(buf);
}

static void print_info(struct afbc_frame_info *f, uint64_t frame_size, int frame_index) {
    int i;
    int n=f->ncomponents[0]+f->ncomponents[1];
    uint64_t uncompressed_size;
    printf("frame_index %d\n", frame_index);
    printf("width %d\n", f->width);
    printf("height %d\n", f->height);
    printf("left_crop %d\n", f->left_crop);
    printf("top_crop %d\n", f->top_crop);
    printf("ncomponents %d\n", n);
    printf("superblock_layout %d\n", f->superblock_layout);
    printf("tilesize %dx%d\n", afbc_get_tile_size_x(f), afbc_get_tile_size_y(f));
    printf("yuv_transform %d\n", f->yuv_transform);
    for(i=0; i<n; i++)
        printf("inputbits[%d]=%d\n", i, f->inputbits[i]);
    uncompressed_size=afbc_get_uncompressed_frame_size(f);
    printf("compressed_size %llu\n", (long long unsigned int)frame_size);
    printf("uncompressed_size %llu\n", (long long unsigned int)uncompressed_size);
    printf("compression %.f%%\n", 100.0*(long long int)(uncompressed_size-frame_size)/uncompressed_size);
    printf("bpp %.2f\n", (float)8*frame_size/(f->width*f->height));

}

static void print_post_info(struct afbc_frame_info *f, uint64_t frame_size, int frame_index) {
    printf("unc: %d\n", accum_unc);
    printf("b3: %d\n", accum_b3);
    printf("b4: %d\n", accum_b4);
    printf("m2: %d\n", accum_m2);
    printf("m3: %d\n", accum_m3);
    printf("m4: %d\n", accum_m4);
    printf("used_tree_bits: %d\n", accum_used_tree_bits);
    printf("needed_tree_bits: %d\n", accum_needed_tree_bits);
}

// Default single line info
static void print_info_single_line(struct afbc_frame_info *f, uint64_t frame_size, int frame_index) {
    int i;
    uint64_t uncompressed_size;
    int n=f->ncomponents[0]+f->ncomponents[1];
    char precision_str[20]="";
    const char *subsampling_str[3]={"", "YUV420, ", "YUV422, "};
    uncompressed_size=afbc_get_uncompressed_frame_size(f);
    for(i=0; i<n; i++)
        sprintf(precision_str+strlen(precision_str), "%d%s", f->inputbits[i], i<n-1?":":"");
    printf("Frame %d: %llu byte, %dx%d, %s%s, %.f%%, %.2f bpp\n",
            frame_index, (long long unsigned int)frame_size, f->width, f->height, subsampling_str[sl_subsampling[f->superblock_layout]], precision_str,
           100.0*(long long int)(uncompressed_size-frame_size)/uncompressed_size,
            (float)8*frame_size/(f->width*f->height));            
}



////////////////////////////// Main program

static void print_usage(void) {
    printf("Usage: afbcdec -i <infile.afb> [-o <outfile.yuv>]\n");
    printf("Usage: afbcdec -i <infile.afb> [-o <outfile.raw>]\n");
    printf("Options:\n");
    printf("   -i <file>        input AFBC file\n");
    printf("   -o <file>        uncompressed output (plane separated\n");
    printf("                    for subsampled YUV formats, otherwise\n");
    printf("                    packed byte aligned pixel format)\n");
    printf("   -p               print extensive compression info\n");
    printf("   -b               The maximum bctree we allow to decode for error streams\n");
    printf("   -v               output data in r8g8b8 format that is ideal for image magic viewing\n");
    printf("   -q               quiet mode\n");
    printf("   -m               MSB align the output for non packed raw outputs\n");
    printf("   -s <frame nbr>   first frame to decode (default is 0)\n");
    printf("   -n <frames>      number of frames to decode (default is all frames)\n");
    printf("   -c               crop ignore mode - decode entire image\n");
    printf("   -h <header>      specify file header on command line. syntax:\n");
    printf("                    WxH_fmt[_colortransform[_blocksplit[_yoffset[_superblocklayout_tiledlayout]]]]\n");
    printf("                    e.g. -h 640x480_r8g8b8_1_1_0_3_1\n");
    printf("   -e               do strict error checking on the buffer\n");
    printf("   -k               specify payload offset to be taken in account when checking alignments\n");
    printf("   -t               if -e is specified, check that payload alignment\n");
    printf("                    is as specified in bytes\n");
    printf("   -r               if -e is specified, check that every payload data start on a multiple\n");
    printf("                    of uncompressed superblock size. (also called sparse mode)\n");
    printf("                    this check respects any specified payload buffer offset and alignment\n");
    printf("   -y               if -e is specified, check that all payload data is organized in stripes\n");
    printf("                    of specified height in number of pixels.\n");
    printf("                    this check respects any specified payload buffer offset and alignment\n");
    printf("   -F               if -e is specified, check that the first superblock in a paging tile is as relative offset 0\n");
    printf("   -0               if -e is specified, allow copies over 8x8 block borders. turn off solid color optimization\n");
    printf("   -1               turn off solid color optimization\n");
    printf("   -x               emit color 0 (black for RGB) whenever a zeroed header is encountered\n");
    printf(" \n");

    exit(1);
}

int main (int argc, char *argv[]) {
    int c, print_extensive_info=0, quiet_mode=0;
    uint64_t frame_size;
    int start_frame=0, number_frames=1000000, frame_index;
    int error_check = 0;
    int check_payload_alignment = 0;
    int check_payload_offset = 0;
    int check_sparse = 0;
    int check_stripe_height = 0;
    char *file_extension=0;
    char *infile_name=0, *outfile_name=0;
    FILE *infile;
    FILE *outfile;
    unsigned int *planes[4];
    int crop_ignore=0;
    struct afbc_frame_info frame_info;
    int force_r8g8b8_output=0;
    int force_r16g16b16a16_output=0;
    int force_r16g16b16a16_output_shifted=0;
    int disable_copies_crossing_8x8_if_yuv=1;
    int decode_allow_zeroed_headers = 1;
    int check_first_superblock_tile_align = 0;

    int msb_align=0;

    int maximum_allowed_bctree = 0;

    const char *cmd_line_file_header=0;
    int unknown_file_header=0;

    // Parse command line arguments
    opterr = 0;
    while ((c = getopt (argc, argv, "s:n:i:o:b:h:k:t:y:F01xrpmuvwWqec?")) != -1)
    switch (c) {
        case 'i':
            infile_name=optarg;
            break;
        case 'o':
            outfile_name=optarg;
            file_extension=strrchr(outfile_name,'.');
            break;
        case 'b':
            maximum_allowed_bctree=atoi(optarg);
            break;
        case 'u':
            unknown_file_header=1;
            break;
        case 'p':
            print_extensive_info=1;
            break;
        case 'm':
            msb_align=1;
            break;
        case 'q':
            quiet_mode=1;
            break;
        case 'v':
            force_r8g8b8_output=1;
            break;
        case 'w':
            force_r16g16b16a16_output=1;
            break;
        case 'W':
            force_r16g16b16a16_output_shifted=1;
            break;
        case 's':
            start_frame=atoi(optarg);
            break;
        case 'n':
            number_frames=atoi(optarg);
            break;
        case 'c':
            crop_ignore=1;
            break;
        case 'e':
            error_check=1;
            break;
        case 'h':
            cmd_line_file_header=optarg;
            break;
        case 'k':
            check_payload_offset=atoi(optarg);
            break;
        case 't':
            check_payload_alignment=atoi(optarg);
            break;
        case 'r':
            check_sparse=1;
            break;
        case 'y':
            check_stripe_height=atoi(optarg);
            break;
        case 'F':
            check_first_superblock_tile_align = 1;
            break;
        case '0':
            disable_copies_crossing_8x8_if_yuv = 0;
            decode_allow_zeroed_headers = 0;
            break;
        case '1':
            decode_allow_zeroed_headers = 0;
            break;
        case 'x':
            fprintf(stderr, "WARNING: This options is deprecated, with the superblock is solid optimization. Behaviour is expected to be the same though\n");
            break;
        case '?':
        default:
            print_usage();
            exit(0);
    }

    if (!infile_name) {
        fprintf(stderr, "ERROR: No input file given\n");
        print_usage();
        exit(1);
    }

    // Open AFBC file
    infile=fopen(infile_name, "rb");
    if (!infile)
        fatal_error("ERROR: cannot open %s\n", infile_name);

    if (outfile_name) {
        // Allow output to a non raw format if the file ending is a few recognized formats
        if (strcmp(file_extension,".png")==0) {
            outfile = 0;
        } else {
            outfile=fopen(outfile_name, "wb");
            if (!outfile)
                fatal_error("ERROR: cannot open %s\n", outfile_name);
        }
    } else
        outfile=0;
    
    // Read first frame header
    frame_info.crop_ignore = crop_ignore;
    if (cmd_line_file_header) {
        parse_fake_file_header(&frame_info, cmd_line_file_header, infile, quiet_mode);
        frame_size=frame_info.frame_size;
    } else if (unknown_file_header) {
        guess_file_header(&frame_info, infile, quiet_mode);
        frame_size=frame_info.frame_size;
    } else {
        frame_size=read_frameheader(&frame_info, infile);
    }

    if (!disable_copies_crossing_8x8_if_yuv && (sl_subsampling[frame_info.superblock_layout]==AFBC_SUBSAMPLING_420 ||
                                                sl_subsampling[frame_info.superblock_layout]==AFBC_SUBSAMPLING_422)) {
        frame_info.disable_copies_crossing_8x8 = 0;
    }

    frame_info.decode_allow_zeroed_headers = decode_allow_zeroed_headers;
    
    frame_info.error_check = error_check;

    frame_info.msb_aligned_output      = msb_align;
    frame_info.check_payload_alignment = check_payload_alignment;
    frame_info.check_payload_offset    = check_payload_offset;
    frame_info.check_sparse            = check_sparse;
    frame_info.check_stripe_height     = check_stripe_height;
    frame_info.check_first_superblock_tile_align = check_first_superblock_tile_align;
    if (frame_info.check_stripe_height != 0) {
        int i;
        // rows of superblocks per stripe
        int sbh = frame_info.check_stripe_height / superblock_height[frame_info.superblock_layout];

        int stripe_count = (frame_info.mbh + sbh - 1) / sbh;
        frame_info.stripe_bins = malloc(sizeof(struct stripe_bin_t)*stripe_count);
        for (i = 0; i < stripe_count; i++) {
            int stripe_height = sbh;
            int block_count;
            // need to special handle last stripe, in case it doesn't divide frame height cleanly
            if (i == stripe_count-1) {
                int nsbh = frame_info.mbh % stripe_height;
                if (nsbh != 0) {
                    stripe_height = nsbh;
                }
            }
            block_count = stripe_height * frame_info.mbw;
            frame_info.stripe_bins[i].start_addr = ~0ull;
            frame_info.stripe_bins[i].end_addr = ~0ull;
            frame_info.stripe_bins[i].remaining_superblocks = block_count;
        }
    }
    
    // Read input frames
    frame_index=0;
    while(frame_size>0 && number_frames>0) {
        if (frame_index>=start_frame) {
            if (print_extensive_info)
                print_info(&frame_info, frame_size, frame_index);
            else if (!quiet_mode)
                print_info_single_line(&frame_info, frame_size, frame_index);
            
            // Allocate buffers
            alloc_planes(&frame_info, planes);
            
            // Set debug options
            frame_info.maximum_allowed_bctree = maximum_allowed_bctree;

            // Read and decode the frame
            decode_frame(&frame_info, frame_size, planes, infile);
            
            // Write output file
            if (outfile) {

                if (force_r16g16b16a16_output_shifted) {
                    write_r16g16b16a16_frame_shifted(&frame_info, planes, outfile);
                } else if (force_r16g16b16a16_output) {
                    write_r16g16b16a16_frame(&frame_info, planes, outfile);
                } else if (force_r8g8b8_output) {
                    write_r8g8b8_frame(&frame_info, planes, outfile);
                } else if (sl_subsampling[frame_info.superblock_layout]!=AFBC_SUBSAMPLING_NONE) {
                    write_plane_separated_frame(&frame_info, planes, outfile);
                } else {
                    write_packed_frame(&frame_info, planes, outfile);
                }
            } else if (outfile_name) {
                if (strcmp(file_extension,".png")==0) {
                    write_png_frame(&frame_info, outfile_name, planes);
                }
            }
            
            free_planes(&frame_info, planes);

            if (print_extensive_info)
                print_post_info(&frame_info, frame_size, frame_index);
            
            number_frames--;
        } else {
            // Skip frame
            skip_frame(frame_size, infile);
        }
        
        frame_index++;
        
        // Try to read one additional frame ("motion AFBC")
        frame_info.crop_ignore = crop_ignore;
        frame_size=read_frameheader(&frame_info, infile);
    }

    // If error checking is enabled we should check the following things
    if (frame_info.error_check) {
        if (frame_info.found_error) {
            report_conformance_failure();

            // Check that are not made:
            // * We can not check the header buffer alignment since this is decided on where 
            // the buffer was placed in memory
            return -1;
        } else {
            printf("This buffer passes as a legal AFBC buffer\n");
        }
    }
    
    return 0;
} 
