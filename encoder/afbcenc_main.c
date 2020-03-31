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
#include "afbc_encode.h"
#include "afbcenc_util.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void fatal_error(const char *format, ...) {
  va_list ap;
  va_start (ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  exit(1);
}

////////////////////////////// Read YUV420 file

// Returns 0 if end-of-file, otherwise 1
static int read_yuv420(FILE *f,
                       int w, int h,
                       unsigned int *planes[4],
                       int nbr_of_bytes_per_px, // Number of bytes per pixel.
                                                // 1 in 8 bit mode.
                                                // 2 in 9-16 bit mode. Assumes valid data at LSB.
                       int shift_num) { // number of bits to right-shift read result

    const int subsample[3]={0,1,1};
    int i, j, actual;
    for(i=0; i<3; i++) {
        int min = 999999;
        int max = 0;

        int size=(w>>subsample[i])*(h>>subsample[i]);
        actual=fread(planes[i], nbr_of_bytes_per_px, size, f);


        if (i==0 && actual==0)
            return 0;
        if (actual!=size)
            fatal_error("ERROR: incomplete YUV frame\n");
        if (nbr_of_bytes_per_px == 1) {
          // Convert from 8 to 32-bit inplace
          for(j=size-1; j>=0; j--) {
              planes[i][j]=((unsigned char*)planes[i])[j] >> shift_num;
              //printf("%2d %2d %02x\n", i, j, planes[i][j]);
          }
        }
        if (nbr_of_bytes_per_px == 2) {
          // Convert from 16 to 32-bit inplace
          for(j=size-1; j>=0; j--) {
              planes[i][j]=((unsigned short*)planes[i])[j] >> shift_num;

              if (planes[i][j] > max) max = planes[i][j];
              if (planes[i][j] < min) min = planes[i][j];
              //printf("%2d %2d %02x\n", i, j, planes[i][j]);
          }
        }
    }
    return 1;
}

////////////////////////////// Read YUV422 file
// 3 plane version

// Returns 0 if end-of-file, otherwise 1
static int read_yuv422(FILE *f,
                       int w, int h,
                       unsigned int *planes[4],
                       int nbr_of_bytes_per_px, // Number of bytes per pixel.
                                                // 1 in 8 bit mode.
                                                // 2 in 9-16 bit mode. Assumes valid data at LSB.
                       int shift_num) { // number of bits to right-shift read result
    const int xsubsample[3]={0,1,1};
    int i, j, actual;
    for(i=0; i<3; i++) {
        int size=(w>>xsubsample[i])*h;
        actual=fread((unsigned char*)planes[i], nbr_of_bytes_per_px, size, f);
        if (i==0 && actual==0)
            return 0;
        if (actual!=size)
            fatal_error("ERROR: incomplete YUV frame\n");
        if (nbr_of_bytes_per_px == 1) {
          // Convert from 8 to 16-bit inplace
          for(j=size-1; j>=0; j--) {
              planes[i][j]=((unsigned char*)planes[i])[j] >> shift_num;
              //printf("%2d %2d %02x\n", i, j, planes[i][j]);
          }
        }
        if (nbr_of_bytes_per_px == 2) {
          // Convert from 16 to 32-bit inplace
          for(j=size-1; j>=0; j--) {
              planes[i][j]=((unsigned short*)planes[i])[j] >> shift_num;
              //printf("%2d %2d %02x\n", i, j, planes[i][j]);
          }
        }
    }
    return 1;
}

////////////////////////////// Read RGB file

// Returns 0 if end-of-file, otherwise 1
static int read_rgb(FILE *f, int w, int h, unsigned int *planes[4], int inputbits[4]) {
    int i, j, actual;
    int size;
    unsigned int components, bytes = 0;
    unsigned char* buf;
    unsigned char* tmp;
    for (components = 0; components < 4; components++)
    {
        if (inputbits[components] == 0)
            break;
        bytes += inputbits[components];
    }
    assert(bytes % 8 == 0);
    bytes /= 8;
    size = bytes*w*h;

    buf = (unsigned char*)malloc(size);
    if (!buf)
        fatal_error("ERROR: unable to alloc buffer");

    tmp = buf;
    actual=fread(buf, 1, size, f);
    if (actual==0) {
        free(buf);
        return 0;
    }
    if (actual!=size)
        fatal_error("ERROR: incomplete RGB frame\n");

    for (j=0; j<w*h; j++) {
        uint64_t pxl = 0;
        for(i=0; i<bytes; i++) {
            pxl |= ((uint64_t)(*tmp++)) << (i*8);
        }
        for(i=0; i<components; i++) {
            uint64_t mask = (1<<inputbits[i]) -1;
            planes[i][j] = pxl & mask;
            pxl >>= inputbits[i];
            //printf("%2d %2d %02x\n", i, j, planes[i][j]);
        }
    }
    free(buf);
    return 1;
}


////////////////////////////// Write plane separated raw file

// Write plane separated raw format
// Depending on inputbits for the compoentn, 8 or 16 bit resolution is used.
static void write_plane_separated_frame(struct afbc_frame_info *f, unsigned int *planes[4], FILE *outfile) {
    int n=f->ncomponents[0]+f->ncomponents[1];
    int xsubsample=sl_subsampling[f->superblock_layout]==AFBC_SUBSAMPLING_420 || sl_subsampling[f->superblock_layout]==AFBC_SUBSAMPLING_422;
    int ysubsample=sl_subsampling[f->superblock_layout]==AFBC_SUBSAMPLING_420;
    int i, j;
    for(i=0; i<n; i++) {
        int elemsize=(f->inputbits[i]+7)/8;
        int w = (xsubsample && i!=0) ? (f->width+1)/2 : f->width;
        int h = (ysubsample && i!=0) ? (f->height+1)/2 : f->height;
        int size=w*h;
        void *buf=malloc(size*elemsize);
        for(j=0; j<size; j++)
            if (elemsize==1)
                ((unsigned char*)buf)[j]=planes[i][j];
            else
                ((unsigned int*)buf)[j]=planes[i][j];
        fwrite(buf, elemsize, size, outfile);
        free(buf);
    }
}

// This is implemented to simplify the generation of YUV content
// it has not been tested for precision
static void yuv_trans(int bits, int* d, int full_range, int rec709) {
    float Y, U, V;
    float R, G, B;

    float weight;

    float kr, kb;

    R = d[0];
    G = d[1];
    B = d[2];

    weight = (1 << (bits-8)); // weight =1 at 8 bit, =4 at 10 bit.

    if (rec709) {
        // 709
        kr = 0.2126;
        kb = 0.0722;
    } else {
        // 601
        kr = 0.299;
        kb = 0.114;
    }

    Y = kr*R+(1-kr-kb)*G+kb*B;
    U = 0.5*(B-Y)/(1-kb);
    V = 0.5*(R-Y)/(1-kr);

    if (full_range) {
        // convert from the narrow range to full range
        // i.e. Y is [16..235] in narrow
        // U,V    is [16..240] in narrow
        // in full range they are both [from 0..255]
        Y *= weight;
        U *= weight;
        V *= weight;

        U += 128.0*weight;
        V += 128.0*weight;

        if (Y > ((1 << bits)-1)) Y = ((1 << bits)-1);
        if (Y < 0.0)   Y = 0.0;
        if (U > ((1 << bits)-1)) U = ((1 << bits)-1);
        if (U < 0.0)   U = 0.0;
        if (V > ((1 << bits)-1)) V = ((1 << bits)-1);
        if (V < 0.0)   V = 0.0;
    } else {
        Y = Y*220.0/256.0;
        U = U*225.0/256.0;
        V = V*225.0/256.0;

        Y += 16.0;
        U += 128.0;
        V += 128.0;

        Y *= weight;
        U *= weight;
        V *= weight;

        // simply multiplying with the weight (rather than extentending or other)
        // is the approprioate method. BT.2020 looks to use 240*4 as the peak for luma though
        if (Y > weight*235.0) Y = weight*235.0;
        if (Y < weight*16.0)  Y = weight*16.0;
        if (U > weight*240.0) U = weight*240.0;
        if (U < weight*16.0)  U = weight*16.0;
        if (V > weight*240.0) V = weight*240.0;
        if (V < weight*16.0)  V = weight*16.0;
    }

    d[0] = Y;
    d[1] = U;
    d[2] = V;
}

static void yuv_manip_420(int bits, uint32_t width, uint32_t height, unsigned int *planes[3], unsigned int *yuv_planes[3],
                          int full_range, int rec709) {
    int c, x,y;
    int yuv[3];

    for(y=0; y<height; y++) {
        for(x=0; x<width; x++) {
            for (c = 0; c < 3; ++c) {
                yuv[c] = planes[c][y*width+x];
            }

            yuv_trans(bits, yuv, full_range, rec709);

            planes[0][y*width+x] = yuv[0];

            // keep it spaced out before subsampling
            planes[1][y*width+x] = yuv[1];
            planes[2][y*width+x] = yuv[2];
        }
    }


    //subsample simple melt
    for(y=0; y<height; y+=2) {
        for(x=0; x<width; x+=2) {
            for (c = 1; c < 3; ++c) {
                int chr;
                chr = planes[c][y*width+x] +
                    planes[c][(y+0)*width+(x+1)] +
                    planes[c][(y+1)*width+(x+0)] +
                    planes[c][(y+1)*width+(x+1)];

                chr /= 4;
                yuv_planes[c][(y/2)*width/2+(x/2)] = chr;
            }
        }
    }
}

static void yuv_manip_422(int bits, uint32_t width, uint32_t height, unsigned int *planes[3], unsigned int *yuv_planes[3],
                          int full_range, int rec709) {
    int c, x,y;
    int yuv[3];

    for(y=0; y<height; y++) {
        for(x=0; x<width; x++) {
            for (c = 0; c < 3; ++c) {
                yuv[c] = planes[c][y*width+x];
            }

            yuv_trans(bits, yuv, full_range, rec709);

            planes[0][y*width+x] = yuv[0];

            // keep it spaced out before subsampling
            planes[1][y*width+x] = yuv[1];
            planes[2][y*width+x] = yuv[2];
        }
    }


    //subsample simple melt
    for(y=0; y<height; y+=1) {
        for(x=0; x<width; x+=2) {
            for (c = 1; c < 3; ++c) {
                int chr;
                chr = planes[c][y*width+x] +
                    planes[c][(y+0)*width+(x+1)];

                chr /= 2;
                yuv_planes[c][(y)*width/2+(x/2)] = chr;
            }
        }
    }
}

// Write packed raw format
// Depending on inputbits for the compoentn, 8 or 16 bit resolution is used.
static void write_packed_frame(struct afbc_frame_info *f, unsigned int *planes[4], FILE *outfile) {
    int n=f->ncomponents[0]+f->ncomponents[1];
    int i, j, elemsize=0, size, k;
    unsigned int data;
    unsigned char *buf;
    for(i=0; i<n; i++)
        elemsize+=f->inputbits[i];
    elemsize=(elemsize+7)/8; // byte align each pixel

    size=f->width*f->height;
    buf=malloc(size*elemsize);

    for(j=0; j<size; j++) {
        for(data=0, k=0, i=0; i<n; i++) {
            data |= planes[i][j]<<k;
            k+=f->inputbits[i];
        }
        for(i=0; i<elemsize; i++, data=data>>8)
            buf[j*elemsize+i]=data&0xff;
    }
    fwrite(buf, elemsize, size, outfile);
    free(buf);
}

////////////////////////////// Frame allocation

static void alloc_planes(unsigned int *planes[4], int n, int w, int h, int xsubsample, int ysubsample) {
    int i;
    for(i=0; i<4; i++) {
        if (i<n) {
            int ww = (xsubsample && i!=0) ? (w+1)/2 : w;
            int hh = (ysubsample && i!=0) ? (h+1)/2 : h;
            planes[i]=malloc(ww*hh*sizeof(unsigned int));
            assert(planes[i]);
        } else {
            planes[i]=0;
        }
    }
}

////////////////////////////// AFBC encode

#ifdef GATHER_BIN_STATS
static unsigned char *buf_start;
int *bins_1B ;
int *bins_32B;
int *bins_64B;
void register_addr_use(unsigned char *p) {
    unsigned long long offset = p - buf_start;
    bins_1B [offset   ] += 1;
    bins_32B[offset/32] += 1;
    bins_64B[offset/64] += 1;
}
#endif

// Grep friendly extensive info
static void print_comp_info(struct afbc_frame_info *f, int frame_size, char *name) {
    int i, uncompressed_size;
    uncompressed_size=afbc_get_uncompressed_frame_size(f);
    printf("%s,", name);
    printf("%d,", uncompressed_size);
    printf("%d,%%,", frame_size);
    for (i = 0; i < 3; ++i) {
        printf("%d,%%,", f->actual_display_bytes[i]);
    }
    for (i = 0; i < 3; ++i) {
        printf("%d,%%,", f->actual_gpu_bytes[i]);
    }
    printf("\n");
}
static void print_info(struct afbc_frame_info *f, int frame_size, int frame_index) {
    int i, uncompressed_size;
    int n=f->ncomponents[0]+f->ncomponents[1];
    printf("frame_index %d\n", frame_index);
    printf("width %d\n", f->width);
    printf("height %d\n", f->height);
    printf("left_crop %d\n", f->left_crop);
    printf("top_crop %d\n", f->top_crop);
    printf("ncomponents %d\n", n);
    printf("superblock_layout %d\n", f->superblock_layout);
    printf("yuv_transform %d\n", f->yuv_transform);
    for(i=0; i<n; i++)
        printf("inputbits[%d]=%d\n", i, f->inputbits[i]);
    uncompressed_size=afbc_get_uncompressed_frame_size(f);
    printf("compressed_size %d\n", frame_size);
    printf("uncompressed_size %d\n", uncompressed_size);
    printf("compression %.f%%\n", 100.0*(uncompressed_size-frame_size)/uncompressed_size);
    printf("bpp %.2f\n", (float)8*frame_size/(f->width*f->height));
    /*for (i = 0; i < 4; ++i) {
        printf("%d\n", f->actual_display_bytes[i]);
    }
    for (i = 0; i < 4; ++i) {
        printf("%d\n", f->actual_gpu_bytes[i]);
    }*/
}

// Default single line info
static void print_info_single_line(struct afbc_frame_info *f, int frame_size, int frame_index) {
    int i, uncompressed_size;
    int n=f->ncomponents[0]+f->ncomponents[1];
    char precision_str[20]="";
    const char *subsampling_str[3]={"", "YUV420, ", "YUV422, "};
    uncompressed_size=afbc_get_uncompressed_frame_size(f);
    for(i=0; i<n; i++)
        sprintf(precision_str+strlen(precision_str), "%d%s", f->inputbits[i], i<n-1?":":"");
    printf("Frame %d: %d byte, %dx%d, %s%s, %.f%%, %.2f bpp\n",
            frame_index, frame_size, f->width, f->height, subsampling_str[sl_subsampling[f->superblock_layout]], precision_str,
            100.0*(uncompressed_size-frame_size)/uncompressed_size,
            (float)8*frame_size/(f->width*f->height));
}

////////////////////////////// Main program

static void print_usage(void) {
    printf("Usage: afbcenc -i <infile.png> [-o <outfile.afb>]\n");
    printf("Usage: afbcenc -w <width> -h <height> -i <infile.yuv> [-o <outfile.afb>]\n");
    printf("Options:\n");
    printf("   -i <file>        input file (YUV and PNG formats supported)\n");
    printf("   -o <file>        resulting AFBC file\n");
    printf("   -w <width>       specify width, required for YUV input\n");
    printf("   -h <height>      specify height, required for YUV input\n");
    printf("   -p               print extensive compression info\n");
    printf("   -q               quiet mode\n");
    printf("   -j               disable copy blocks between the upper and lower half of the supertile\n");
    printf("   -l               block split mode (will implicitly trigger sparse mode -c)\n");
    printf("   -f               force the output format for example r5g5b5a1 use -u to avoid truncating the indata\n");
    printf("   -y               The .yuv file should be parsed as YUV422\n");
    printf("   -M               MSB align the output for non packed raw inputs\n");
    printf("   -s <frame nbr>   first frame to encode (default is 0)\n");
    printf("   -n <frames>      number of frames to encode (default is all frames)\n");
    printf("   -r <outfile.raw> write raw plane separated file containing the uncompressed frame (useful for PNG input)\n");
    printf("   -z               skip AFBC file header\n");
    printf("   -t               truncate PNG to R5G6B5\n");
    printf("   -d               disable yuv transform\n");
    printf("   -c               Use addressing with the sparse rtl mode where each payload is spaced away the size of 1 superblock from each other\n");
    printf("   -a               Specify a superblock layout. The selected superblock layout needs to match the subsampling of the input format after conversion\n");
    printf("   -0               allow copies over 8x8 block borders\n");
    printf("   -C               Turn on perfect copy block detection\n");
    printf("   -S               Turn off solid color optimization\n");
    printf("   -T               Use tiled mode\n");
    printf("   -U               Force all blocks to be either uncompressed or copy\n");
    exit(1);
}

typedef enum {
    FORMAT_NONE,
    FORMAT_PNG,
    FORMAT_YUV420,
    FORMAT_YUV422,
    FORMAT_RGB
} format_t;

int main (int argc, char *argv[]) {
    int c, w=0, h=0, d=1, print_extensive_info=0, frame_size, skip_fileheader=0, quiet_mode=0, truncate_png=0;
    int start_frame=0, number_frames=1000000, frame_index;
    int yuv422=0;
    int do_not_truncate = 0;
    int full_range_yuv = 0;
    int rec709 = 0;
    int disable_copy_rows = 0;
    char *fileformat_name="r5g6b5";
    format_t format = FORMAT_NONE;
    char *infile_name=0, *outfile_name=0, *rawfile_name=0;
    char *file_extension=0;
    char *force_file_extension=0;
    int use_forced_file_extension = 0;
    FILE *infile, *outfile, *rawfile;
    unsigned int *planes[4];
    unsigned int *yuv_planes[4];
    int disable_yuv_transform = 0;
    int inputbits[4];
    int superblock_layout = -1; // default is to pick the layout
    int file_message = 0;
    int block_split = 0;
    int statsprint = 0;
    int volume = 1;
    int tiled = 0;
    int rtl_addressing = 0;
    int disable_copies_crossing_8x8_if_yuv = 1;
    int allow_solid_color = 1;
    int allow_all_copy_blocks = 0;
    int only_uncompressed_blocks = 0;
    int msb_align = 0;
    struct afbc_frame_info frame_info;

    // Parse command line arguments
    opterr = 0;
    while ((c = getopt (argc, argv, "a:s:n:i:m:f:r:o:1:3:w:h:M0dlupvyqrjcztCSTU?")) != -1)
        switch (c) {
        case 'a':
            superblock_layout = atoi(optarg);
            break;
        case 'w':
            w = atoi(optarg);
            break;
        case 'c':
            rtl_addressing=1;
            break;
        case 'h':
            h = atoi(optarg);
            break;
        case '3':
            volume = 1;
            superblock_layout = 8;
            d = atoi(optarg);
            break;
        case '1':
            use_forced_file_extension = 1;
            force_file_extension=optarg;
            break;
        case 'i':
            infile_name=optarg;
            file_extension=strrchr(infile_name,'.');
            break;
        case 'o':
            outfile_name=optarg;
            break;
        case 'm':
            file_message=atoi(optarg);
            break;
        case 'f':
            truncate_png=1;
            fileformat_name=optarg;
            break;
        case 'p':
            print_extensive_info=1;
            break;
        case 'y':
            yuv422 = 1;
            break;
        case 'M':
            msb_align=1;
            break;
        case 'q':
            quiet_mode=1;
            break;
        case 'l':
            block_split=1;
            rtl_addressing=1; // also enable sparse mode
            break;
        case 'v':
            statsprint=1;
            break;
        case 'd':
            disable_yuv_transform=1;
            break;
        case 'u':
            do_not_truncate=1;
            break;
        case 'j':
            disable_copy_rows=1;
            break;
        case 'r':
            rawfile_name=optarg;
            break;
        case 'z':
            skip_fileheader=1;
            break;
        case 's':
            start_frame=atoi(optarg);
            break;
        case 'n':
            number_frames=atoi(optarg);
            break;
        case 't':
            truncate_png=1;
            break;
        case '0':
            disable_copies_crossing_8x8_if_yuv = 0;
            break;
        case 'C':
            allow_all_copy_blocks = 1;
            break;
        case 'S':
            allow_solid_color = 0;
            break;
        case 'T':
            tiled = 1;
            rtl_addressing = 1;
            break;
        case 'U':
            only_uncompressed_blocks = 1;
            break;
        case '?':
        default:
            print_usage();
            exit(0);
    }

    if (use_forced_file_extension) {
        file_extension = force_file_extension;
    }

    if (!infile_name) {
        fprintf(stderr, "ERROR: No input file given\n");
        print_usage();
        exit(1);
    }

    // Open files
    infile=fopen(infile_name, "rb");
    if (!infile)
        fatal_error("ERROR: cannot open infile %s\n", infile_name);

    if (outfile_name) {
        outfile=fopen(outfile_name, "wb");
        if (!outfile)
            fatal_error("ERROR: cannot open outfile %s\n", outfile_name);
    } else
        outfile=0;

    if (rawfile_name) {
        rawfile=fopen(rawfile_name, "wb");
        if (!rawfile)
            fatal_error("ERROR: cannot open rawfile %s\n", rawfile_name);
    } else {
        rawfile=0;
    }

    // Allocate buffers and read source frame
    if (strcmp(file_extension,".png")==0 || strcmp(file_extension,".tga")==0 ||
        strcmp(file_extension,".jpg")==0 || strcmp(file_extension,".bmp")==0) {
        int n, m, i;
        unsigned char *data;
        int bitdepth = 8;
        int bytes;

        format = FORMAT_PNG;

        if (w!=0 || h!=0)
            fatal_error("ERROR: width and height should not be specified for PNG or other input files\n");
        if (start_frame>0)
            fatal_error("ERROR: multiple frames are not supported for the PNG or other formats\n");
        number_frames=1;

        bitdepth = stbi_depth(infile_name);
        if (bitdepth == -1) bitdepth = 8;
        data = stbi_load(infile_name, &w, &h, &n, 0);

        if (n==1) {
            inputbits[0]=bitdepth, inputbits[1]=0, inputbits[2]=0, inputbits[3]=0;
            disable_yuv_transform = 1;
        }
        else if (n==3)
            inputbits[0]=bitdepth, inputbits[1]=bitdepth, inputbits[2]=bitdepth, inputbits[3]=0;
        else if (n==4)
            inputbits[0]=bitdepth, inputbits[1]=bitdepth, inputbits[2]=bitdepth, inputbits[3]=bitdepth;
        else
            fatal_error("ERROR: unsupported number of planes in PNG %d\n", n);
        alloc_planes(planes, n, w, h, 0, 0);

        bytes = (bitdepth+7)/8;
        for (i = 0; i < w*h; ++i) {
            for (m = 0; m < n; ++m) {
                planes[m][i] = data[(i*n+m)*bytes] | (bytes > 1 ? data[(i*n+m)*bytes+1] << 8 : 0);
            }
        }

        if (!truncate_png) {
            afbc_init_frame_info(&frame_info, w, h, n, superblock_layout, !disable_yuv_transform, tiled, inputbits);
        } else {
            int x, y, j, k;

            if (strcmp(fileformat_name,"r5g6b5")==0) {
                n=min(n,3);
                inputbits[0]=5, inputbits[1]=6, inputbits[2]=5, inputbits[3]=0;
                afbc_init_frame_info(&frame_info, w, h, 3, superblock_layout, !disable_yuv_transform, tiled, inputbits);
            } else if (strcmp(fileformat_name,"r5g5b5a1")==0) {
                n=min(n,4);
                inputbits[0]=5, inputbits[1]=5, inputbits[2]=5, inputbits[3]=1;
                afbc_init_frame_info(&frame_info, w, h, 4, superblock_layout, !disable_yuv_transform, tiled, inputbits);
            } else if (strcmp(fileformat_name,"r10g10b10a2")==0) {
                n=min(n,4);
                inputbits[0]=10, inputbits[1]=10, inputbits[2]=10, inputbits[3]=2;
                afbc_init_frame_info(&frame_info, w, h, 4, superblock_layout, !disable_yuv_transform, tiled, inputbits);
            } else if (strcmp(fileformat_name,"r11g11b10")==0) {
                n=min(n,3);
                inputbits[0]=11, inputbits[1]=11, inputbits[2]=10, inputbits[3]=0;
                afbc_init_frame_info(&frame_info, w, h, 3, superblock_layout, 0, tiled, inputbits);
            } else if (strcmp(fileformat_name,"r8g8b8")==0) {
                n=3;
                inputbits[0]=8, inputbits[1]=8, inputbits[2]=8, inputbits[3]=0;
                afbc_init_frame_info(&frame_info, w, h, 3, superblock_layout, !disable_yuv_transform, tiled, inputbits);
            } else if (strcmp(fileformat_name,"r8g8b8a8")==0) {
                inputbits[0]=8, inputbits[1]=8, inputbits[2]=8, inputbits[3]=8;
                afbc_init_frame_info(&frame_info, w, h, 4, superblock_layout, !disable_yuv_transform, tiled, inputbits);
            } else if (strcmp(fileformat_name,"r4g4b4a4")==0) {
                inputbits[0]=4, inputbits[1]=4, inputbits[2]=4, inputbits[3]=4;
                afbc_init_frame_info(&frame_info, w, h, 4, superblock_layout, !disable_yuv_transform, tiled, inputbits);
            } else if (strcmp(fileformat_name,"r8")==0) {
                n=1;
                inputbits[0]=8, inputbits[1]=0, inputbits[2]=0, inputbits[3]=0;
                afbc_init_frame_info(&frame_info, w, h, 1, superblock_layout, 0, tiled, inputbits);
            } else if (strcmp(fileformat_name,"r8g8")==0) {
                n=2;
                inputbits[0]=8, inputbits[1]=8, inputbits[2]=0, inputbits[3]=0;
                afbc_init_frame_info(&frame_info, w, h, 2, superblock_layout, 0, tiled, inputbits);
            } else if (strncmp(fileformat_name,"yuv", 3)==0) {
                // decode for all yuv formats
                int tenbit = 0;

                n=3;
                if (strstr(fileformat_name, "10b") != 0) {
                    inputbits[0]=10, inputbits[1]=10, inputbits[2]=10, inputbits[3]=0;
                    tenbit = 1;
                } else {
                    inputbits[0]=8, inputbits[1]=8, inputbits[2]=8, inputbits[3]=0;
                    tenbit = 0;
                }

                if (strstr(fileformat_name, "420") != 0) {
                    if (tenbit) {
                        alloc_planes(yuv_planes, 3, w*2, h*2, 1, 1);
                    } else {
                        alloc_planes(yuv_planes, 3, w, h,  1, 1);
                    }
                } else {
                    if (tenbit) {
                        alloc_planes(yuv_planes, 3, w*2, h*2, 1, 0);
                    } else {
                        alloc_planes(yuv_planes, 3, w, h, 1, 0);
                    }
                }

                if (strstr(fileformat_name, "f") != 0) {
                    full_range_yuv = 1;
                }

                if (strstr(fileformat_name, "rec709") != 0) {
                    rec709 = 1;
                }

                // common afbc activities
                if (strncmp(fileformat_name,"yuv422", 6)==0) {
                    yuv_manip_422(inputbits[0], w, h, planes, yuv_planes, full_range_yuv, rec709);
                } else {
                    yuv_manip_420(inputbits[0], w, h, planes, yuv_planes, full_range_yuv, rec709);
                }
                free(yuv_planes[0]);
                free(planes[1]);
                free(planes[2]);
                planes[1] = yuv_planes[1];
                planes[2] = yuv_planes[2];
                if (strncmp(fileformat_name,"yuv422", 6)==0) {
                    if (superblock_layout == -1) superblock_layout = 2;
                } else {
                    if (superblock_layout == -1) superblock_layout = 1;
                }

                afbc_init_frame_info(&frame_info, w, h, 3, superblock_layout, 0, tiled, inputbits);

            } else {
                fatal_error("ERROR: unknown input file format %s\n", fileformat_name);
            }
            if (!do_not_truncate && (strncmp(fileformat_name,"yuv", 3)!=0)) {
                printf("Truncating frame input PNG to %s\n", fileformat_name);

                // Truncate the frame
                j=0;
                for(y=0; y<h; y++) {
                    for(x=0; x<w; x++,j++) {
                        for (k = 0; k < n; ++k) {
                            if (inputbits[k] > bitdepth) {
                                planes[k][j]=planes[k][j]<<(inputbits[k]-bitdepth);
                            } else {
                                planes[k][j]=planes[k][j]>>(bitdepth-inputbits[k]);
                            }
                        }
                    }
                }
            }
        }
    } else if (strcmp(file_extension,".yuv")==0 && !yuv422) {
        if (w==0 || h==0)
          fatal_error("ERROR: width and height must be specified for YUV input files\n");
        if (strstr(infile_name, "10b")) {
          alloc_planes(planes, 3, w*2, h*2, 1, 1);
          inputbits[0]=10, inputbits[1]=10, inputbits[2]=10, inputbits[3]=0;
        } else if (strstr(infile_name, "16b")) {
          alloc_planes(planes, 3, w*2, h*2, 1, 1);
          inputbits[0]=16, inputbits[1]=16, inputbits[2]=16, inputbits[3]=0;
        } else {
          alloc_planes(planes, 3, w, h, 1, 1);
          inputbits[0]=8, inputbits[1]=8, inputbits[2]=8, inputbits[3]=0;
        }
        if (superblock_layout == -1) { superblock_layout = 1;}
        afbc_init_frame_info(&frame_info, w, h, 3, superblock_layout, 0, tiled, inputbits);
        format=FORMAT_YUV420;
    } else if (strcmp(file_extension,".yuv")==0 && yuv422) {
        if (w==0 || h==0)
            fatal_error("ERROR: width and height must be specified for YUV input files\n");
        if (strstr(infile_name, "10b")) {
          alloc_planes(planes, 3, w*2, h*2, 1, 0);
          inputbits[0]=10, inputbits[1]=10, inputbits[2]=10, inputbits[3]=0;
        } else {
          alloc_planes(planes, 3, w, h, 1, 0);
          inputbits[0]=8, inputbits[1]=8, inputbits[2]=8, inputbits[3]=0;
        }
        if (superblock_layout == -1) { superblock_layout = 2;}
        afbc_init_frame_info(&frame_info, w, h, 3, superblock_layout, 0, tiled, inputbits);
        format=FORMAT_YUV422;
    } else if (strcmp(file_extension,".r5g6b5")==0) {
        if (w==0 || h==0)
            fatal_error("ERROR: width and height must be specified for RGB input files\n");
        alloc_planes(planes, 3, w, h, 0, 0);
        inputbits[0]=5, inputbits[1]=6, inputbits[2]=5, inputbits[3]=0;
        afbc_init_frame_info(&frame_info, w, h, 3, superblock_layout, !disable_yuv_transform, tiled, inputbits);
        format=FORMAT_RGB;
    } else if (strcmp(file_extension,".r5g5b5a1")==0) {
        if (w==0 || h==0)
            fatal_error("ERROR: width and height must be specified for RGB input files\n");
        alloc_planes(planes, 4, w, h, 0, 0);
        inputbits[0]=5, inputbits[1]=5, inputbits[2]=5, inputbits[3]=1;
        afbc_init_frame_info(&frame_info, w, h, 4, superblock_layout, !disable_yuv_transform, tiled, inputbits);
        format=FORMAT_RGB;
    } else if (strcmp(file_extension,".r10g10b10a2")==0) {
        if (w==0 || h==0)
            fatal_error("ERROR: width and height must be specified for RGB input files\n");
        alloc_planes(planes, 4, w, h, 0, 0);
        inputbits[0]=10, inputbits[1]=10, inputbits[2]=10, inputbits[3]=2;
        afbc_init_frame_info(&frame_info, w, h, 4, superblock_layout, !disable_yuv_transform, tiled, inputbits);
        format=FORMAT_RGB;
    } else if (strcmp(file_extension,".r11g11b10")==0) {
        if (w==0 || h==0)
            fatal_error("ERROR: width and height must be specified for RGB input files\n");
        alloc_planes(planes, 3, w, h, 0, 0);
        inputbits[0]=11, inputbits[1]=11, inputbits[2]=10, inputbits[3]=0;
        afbc_init_frame_info(&frame_info, w, h, 3, superblock_layout, !disable_yuv_transform, tiled, inputbits);
        format=FORMAT_RGB;
    } else if (strcmp(file_extension,".r8g8b8")==0) {
        if (w==0 || h==0)
            fatal_error("ERROR: width and height must be specified for RGB input files\n");
        alloc_planes(planes, 3, w, h, 0, 0);
        inputbits[0]=8, inputbits[1]=8, inputbits[2]=8, inputbits[3]=0;
        afbc_init_frame_info(&frame_info, w, h, 3, superblock_layout, !disable_yuv_transform, tiled, inputbits);
        format=FORMAT_RGB;
    } else if (strcmp(file_extension,".r8g8b8a8")==0) {
        if (w==0 || h==0)
            fatal_error("ERROR: width and height must be specified for RGB input files\n");
        alloc_planes(planes, 4, w, h, 0, 0);
        inputbits[0]=8, inputbits[1]=8, inputbits[2]=8, inputbits[3]=8;
        afbc_init_frame_info(&frame_info, w, h, 4, superblock_layout, !disable_yuv_transform, tiled, inputbits);
        format=FORMAT_RGB;
    } else if (strcmp(file_extension,".r4g4b4a4")==0) {
        if (w==0 || h==0)
            fatal_error("ERROR: width and height must be specified for RGB input files\n");
        alloc_planes(planes, 4, w, h, 0, 0);
        inputbits[0]=4, inputbits[1]=4, inputbits[2]=4, inputbits[3]=4;
        afbc_init_frame_info(&frame_info, w, h, 4, superblock_layout, !disable_yuv_transform, tiled, inputbits);
        format=FORMAT_RGB;
    } else if (strcmp(file_extension,".r8")==0) {
        if (w==0 || h==0)
            fatal_error("ERROR: width and height must be specified for RGB input files\n");
        alloc_planes(planes, 1, w, h, 0, 0);
        disable_yuv_transform = 1;
        inputbits[0]=8, inputbits[1]=0, inputbits[2]=0, inputbits[3]=0;
        afbc_init_frame_info(&frame_info, w, h, 1, superblock_layout, !disable_yuv_transform, tiled, inputbits);
        format=FORMAT_RGB;
    } else if (strcmp(file_extension,".r8g8")==0) {
        if (w==0 || h==0)
            fatal_error("ERROR: width and height must be specified for RGB input files\n");
        alloc_planes(planes, 2, w, h, 0, 0);
        disable_yuv_transform = 1;
        inputbits[0]=8, inputbits[1]=8, inputbits[2]=0, inputbits[3]=0;
        afbc_init_frame_info(&frame_info, w, h, 2, superblock_layout, 0, tiled, inputbits);
        format=FORMAT_RGB;
    } else if (strcmp(file_extension,".r16")==0) {
        if (w==0 || h==0)
            fatal_error("ERROR: width and height must be specified for RGB input files\n");
        alloc_planes(planes, 1, w, h, 0, 0);
        disable_yuv_transform = 1;
        inputbits[0]=16, inputbits[1]=0, inputbits[2]=0, inputbits[3]=0;
        afbc_init_frame_info(&frame_info, w, h, 1, superblock_layout, 0, tiled, inputbits);
        format=FORMAT_RGB;
    } else if (strcmp(file_extension,".r16g16")==0) {
        if (w==0 || h==0)
            fatal_error("ERROR: width and height must be specified for RGB input files\n");
        alloc_planes(planes, 2, w, h, 0, 0);
        disable_yuv_transform = 1;
        inputbits[0]=16, inputbits[1]=16, inputbits[2]=0, inputbits[3]=0;
        afbc_init_frame_info(&frame_info, w, h, 2, superblock_layout, 0, tiled, inputbits);
        format=FORMAT_RGB;
    } else if (strcmp(file_extension,".r16g16b16")==0) {
        if (w==0 || h==0)
            fatal_error("ERROR: width and height must be specified for RGB input files\n");
        alloc_planes(planes, 3, w, h, 0, 0);
        inputbits[0]=16, inputbits[1]=16, inputbits[2]=16, inputbits[3]=0;
        afbc_init_frame_info(&frame_info, w, h, 3, superblock_layout, !disable_yuv_transform, tiled, inputbits);
        format=FORMAT_RGB;
    } else if (strcmp(file_extension,".r16g16b16a16")==0) {
        if (w==0 || h==0)
            fatal_error("ERROR: width and height must be specified for RGB input files\n");
        alloc_planes(planes, 4, w, h, 0, 0);
        inputbits[0]=16, inputbits[1]=16, inputbits[2]=16, inputbits[3]=16;
        afbc_init_frame_info(&frame_info, w, h, 4, superblock_layout, !disable_yuv_transform, tiled, inputbits);
        format=FORMAT_RGB;
    } else {
        fatal_error("ERROR: unknown input file format %s\n", file_extension);
    }

    if (!disable_copies_crossing_8x8_if_yuv && (sl_subsampling[frame_info.superblock_layout]==AFBC_SUBSAMPLING_420 ||
                                                sl_subsampling[frame_info.superblock_layout]==AFBC_SUBSAMPLING_422)) {
        frame_info.disable_copies_crossing_8x8 = 0;
    }

    frame_info.file_message   = file_message;
    frame_info.block_split    = block_split;
    frame_info.disable_copy_rows = disable_copy_rows || block_split;
    frame_info.rtl_addressing = rtl_addressing;
    frame_info.allow_solid_color = allow_solid_color;
    frame_info.allow_all_copy_blocks = allow_all_copy_blocks;
    frame_info.only_uncompressed_blocks = only_uncompressed_blocks;

#ifdef GATHER_BIN_STATS
    bins_1B  = malloc(sizeof(bins_1B [0])*afbc_get_max_frame_size(&frame_info));
    bins_32B = malloc(sizeof(bins_32B[0])*afbc_get_max_frame_size(&frame_info));
    bins_64B = malloc(sizeof(bins_64B[0])*afbc_get_max_frame_size(&frame_info));
    {
        int i;
        for (i = 0; i < frame_info.frame_size; ++i) {
            bins_1B [i] = 0;
            bins_32B[i] = 0;
            bins_64B[i] = 0;
        }
    }
#endif

    // Loop over all frames
    frame_index=0;
    while(number_frames>0) {
        int res = 0;
        switch (format)
        {
          case FORMAT_PNG: res=1; break; // PNG frame is already read at this point
          case FORMAT_YUV420:
          {
             if (inputbits[0] >= 10) {
               // NOTE: this assumes that all components have the same size
               int shift_num = msb_align ? (((frame_info.inputbits[0]+7)/8)*8 - frame_info.inputbits[0]) : 0;
               res = read_yuv420(infile, w, h, planes, 2, shift_num);
             } else {
               res = read_yuv420(infile, w, h, planes, 1, 0);
             }
             break;
          }
          case FORMAT_YUV422:
          {
             if (inputbits[0] >= 10) {
               // NOTE: this assumes that all components have the same size
               int shift_num = msb_align ? (((frame_info.inputbits[0]+7)/8)*8 - frame_info.inputbits[0]) : 0;
               res = read_yuv422(infile, w, h, planes, 2, shift_num);
             } else {
               res = read_yuv422(infile, w, h, planes, 1, 0);
             }
             break;
          }
          case FORMAT_RGB:
              // when we read a volume we pretend we have a high frame
              res=read_rgb(infile, w, volume ? d*h : h, planes, inputbits);
              break;
          default: fatal_error("ERROR: unknown format"); break;
        }
            if (res==0)
                break; // end-of-file


        if (frame_index>=start_frame) {

            // Write raw uncompressed output file
            if (rawfile) {
                if (sl_subsampling[frame_info.superblock_layout]!=AFBC_SUBSAMPLING_NONE)
                    write_plane_separated_frame(&frame_info, planes, rawfile);
                else
                    write_packed_frame(&frame_info, planes, rawfile);
            }

            // Encode the frame and write to the outfile
            frame_size=encode_frame(&frame_info, planes, skip_fileheader, outfile);

            if (statsprint) {
                print_comp_info(&frame_info, frame_size, infile_name);
            } else if (print_extensive_info) {
                print_info(&frame_info, frame_size, frame_index);
            } else if (!quiet_mode) {
                print_info_single_line(&frame_info, frame_size, frame_index);
            }

            number_frames--;
        }
        frame_index++;
    }

#ifdef GATHER_BIN_STATS
    {
        long uncompressed_size = afbc_get_uncompressed_frame_size(f);
        long comp_size_1B;
        long comp_size_32B;
        long comp_size_64B;
        int i;
        const int bin_count_1B = afbc_get_max_frame_size(&frame_info);
        const int bin_count_32B = afbc_get_max_frame_size(&frame_info);
        const int bin_count_64B = afbc_get_max_frame_size(&frame_info);
        int bin_hits_1B = 0;
        int bin_hits_32B = 0;
        int bin_hits_64B = 0;
        for (i = 0; i < bin_count_1B; i++) if (bins_1B[i]) bin_hits_1B += 1;
        for (i = 0; i < bin_count_32B; i++) if (bins_32B[i]) bin_hits_32B += 1;
        for (i = 0; i < bin_count_64B; i++) if (bins_64B[i]) bin_hits_64B += 1;
        printf("bins 1B, 32B, 64B: %d, %d, %d\n", bin_hits_1B, bin_hits_32B, bin_hits_64B);
        comp_size_1B  = bin_hits_1B;
        comp_size_32B = 32*bin_hits_32B;
        comp_size_64B = 64*bin_hits_64B;
        printf("comp_size_1B, comp_size_32B, comp_size_64B, uncomp_size: %ld, %ld, %ld, %ld\n", comp_size_1B, comp_size_32B, comp_size_64B, uncompressed_size);
        printf("Compression ratio (1-comp/unc): %.1f, %.1f, %.1f\n",
               100.0*(uncompressed_size-comp_size_1B)/uncompressed_size,
               100.0*(uncompressed_size-comp_size_32B)/uncompressed_size,
               100.0*(uncompressed_size-comp_size_64B)/uncompressed_size);
    }
#endif

    return 0;
}
