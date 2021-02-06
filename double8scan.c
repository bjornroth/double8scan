/**
 *
 *  double8scan.c
 *
 *  (C) 2004-2021 Bjorn Roth, Infundo
 *
 *
 *
 *  JPEG library by Independent JPEG Group
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <getopt.h>
#else
#include <unistd.h>
#endif
#include <jpeglib.h>
#include <limits.h>
#include <math.h>


typedef enum { CHAN_R, CHAN_G, CHAN_B, CHAN_Y } CHAN_T;

typedef struct {
  int width;
  int height;
  int components;
  JSAMPARRAY scanline;
  int *scanstart;
  JSAMPLE *buffer;
} RAWBUF_T;

/* default defines */
#define WHITELEVEL 0xe0
#define BLACKLEVEL 0xc0

#define PERF_X_START 10
#define PERF_Y_START 40

#define MAX_PERFDIFF 20
#define MIN_PERF_HEIGHT_FAC 0.05
#define MAX_PERF_HEIGHT_FAC 0.4
#define MIN_FRAME_HEIGHT_FAC 0.3
#define MAX_FRAME_HEIGHT_FAC 0.8
#define FRAME_FRAC_WITH_PERF 0.5
#define PERF_OK_COUNT 5
#define MAX_FRAMEDIFF 30

#define FRAME_X_NEG_OFFS 20

#define JPEG_QUALITY 80

/* prototypes */
int decompress(char *filename, RAWBUF_T *imgbuf);
int find_perf(RAWBUF_T *imgbuf, int *offs, int verbose);
int find_xstart(RAWBUF_T *imgbuf, int ypos, int verbose);
int compress_frames(char *filename, RAWBUF_T *imgbuf, int imgheight,
                    int imgwidth, int quality, int verbose);
int rotate_strip(RAWBUF_T *imgbuf, int deg, int verbose);
void free_buf(RAWBUF_T *imgbuf);

/* globals */
int white_level = WHITELEVEL;
int black_level = BLACKLEVEL;

int min_perf_height;
int max_perf_height;
int min_frame_height;
int max_frame_height;

int perf_x_start = PERF_X_START;
int perf_y_start = PERF_Y_START;

int color_channel = CHAN_Y;

void usage(void) {
  printf("usage: double8scan [options] <infile>\n");
  printf("options:\n");
  printf("       -v             : verbose (-vv for more, etc)\n");
  printf("       -h <height>    : set frame height (needed for extraction)\n");
  printf("       -w <width>     : set frame width (needed for extraction)\n");
  printf("       -x <offs>      : x offset to start perf detection at\n");
  printf("       -y <offs>      : y offset to start perf detection at\n");
  printf("       -B <level>     : set black level 0-255 (for perf detection)\n");
  printf("       -W <level>     : set white level 0-255 (for perf detection)\n");
  printf("       -p <min>-<max> : set min/max values for perf height\n");
  printf("       -f <min>-<max> : set min/max values for frame height\n");
  printf("       -c <R|G|B|Y>   : color channel R, G, B or Y (default B)\n");
  printf("       -r <deg>       : rotate strip degrees (default 0, -90 if width > height)\n");
  printf("       -q <quality>   : JPEG output quality (0-100), default = %d\n", JPEG_QUALITY);
  exit(1);
}

int main(int argc, char *argv[]) {
  RAWBUF_T imgbuf;
  int verbose = 0;
  int height = 0;
  int width = 0;
  int rotate = 0;
  int quality = JPEG_QUALITY;
  int offs;
  char *file;

  while ((optopt = getopt(argc, argv, "vh:w:W:B:p:f:c:x:y:r:q:")) != EOF) {
    switch (optopt) {
    case 'v':
      verbose++;
      break;
    case 'h':
      sscanf(optarg, "%d", &height);
      break;
    case 'w':
      sscanf(optarg, "%d", &width);
      break;
    case 'W':
      sscanf(optarg, "%d", &white_level);
      break;
    case 'B':
      sscanf(optarg, "%d", &black_level);
      break;
    case 'p':
      sscanf(optarg, "%d-%d", &min_perf_height, &max_perf_height);
      break;
    case 'f':
      sscanf(optarg, "%d-%d", &min_frame_height, &max_frame_height);
      break;
    case 'c':
      if (strcmp(optarg, "R") == 0)
        color_channel = CHAN_R;
      else if (strcmp(optarg, "G") == 0)
        color_channel = CHAN_G;
      else if (strcmp(optarg, "B") == 0)
        color_channel = CHAN_B;
      else if (strcmp(optarg, "Y") == 0)
        color_channel = CHAN_Y;
      break;
    case 'x':
      sscanf(optarg, "%d", &perf_x_start);
      break;
    case 'y':
      sscanf(optarg, "%d", &perf_y_start);
      break;
    case 'r':
      sscanf(optarg, "%d", &rotate);
      break;
    case 'q':
      sscanf(optarg, "%d", &quality);
      if (quality < 0 || quality > 100)
        usage();
      break;
    case '?':
    default:
      usage();
      return -1;
    }
  }

  if (optind == argc) {
    usage();
    return -1;
  }

  file = argv[optind];

  printf("double8scan reading file %s\n", file);

  if (height == 0)
    printf("no frame height given, only probing\n");

  imgbuf.scanline = NULL;
  imgbuf.scanstart = NULL;
  imgbuf.buffer = NULL;

  decompress(file, &imgbuf);

  if (rotate == 0 && imgbuf.width > imgbuf.height)
    rotate = -90;

  if (rotate)
    rotate_strip(&imgbuf, rotate, verbose);

  printf("image height %d width %d components %d\n", imgbuf.height,
         imgbuf.width, imgbuf.components);
  printf("channel: ");
  if (color_channel == CHAN_Y)
    printf("Y");
  else if (color_channel == CHAN_R)
    printf("R");
  else if (color_channel == CHAN_G)
    printf("G");
  else if (color_channel == CHAN_B)
    printf("B");
  else
    printf("?");

  if (!min_perf_height) min_perf_height = imgbuf.width * MIN_PERF_HEIGHT_FAC;
  if (!max_perf_height) max_perf_height = imgbuf.width * MAX_PERF_HEIGHT_FAC;
  if (!min_frame_height) min_frame_height = imgbuf.width * MIN_FRAME_HEIGHT_FAC;
  if (!max_frame_height) max_frame_height = imgbuf.width * MAX_FRAME_HEIGHT_FAC;
  if (!width) width = imgbuf.width;

  printf(" black %d white %d perf %d-%d frame %d-%d\n", black_level,
         white_level, min_perf_height, max_perf_height, min_frame_height,
         max_frame_height);

  find_perf(&imgbuf, &offs, verbose);

  if (!height || !width)
    return 1;

  /* check if a whole frame is above first complete perf */
  if (offs > height)
    find_xstart(&imgbuf, offs - height, verbose);

  printf("using frame height %d width %d, offset %d\n", height, width, offs);

  compress_frames(file, &imgbuf, height, width, quality, verbose);

  free_buf(&imgbuf);

  return 0;
}

int decompress(char *filename, RAWBUF_T *imgbuf) {
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  FILE *infile;
  int i;

  if ((infile = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    exit(1);
  }

  cinfo.err = jpeg_std_error(&jerr);

  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, infile);

  jpeg_read_header(&cinfo, TRUE);

  /* change to YCbCr if CHAN_Y */
  if (color_channel == CHAN_Y)
    cinfo.out_color_space = JCS_YCbCr;
  else
    cinfo.out_color_space = JCS_RGB;

  jpeg_start_decompress(&cinfo);

  imgbuf->width = cinfo.output_width;
  imgbuf->height = cinfo.output_height;
  imgbuf->components = cinfo.output_components;
  imgbuf->buffer = (JSAMPLE *)malloc(cinfo.output_height * cinfo.output_width *
                                     cinfo.output_components);
  imgbuf->scanline =
      (JSAMPARRAY)malloc(cinfo.output_height * sizeof(JSAMPLE *));
  imgbuf->scanstart = (int *)malloc(cinfo.output_height * sizeof(int));

  for (i = 0; i < cinfo.output_height; i++)
    imgbuf->scanline[i] =
        imgbuf->buffer + (i * cinfo.output_width * cinfo.output_components);

  while (cinfo.output_scanline < cinfo.output_height)
    jpeg_read_scanlines(&cinfo, &(imgbuf->scanline[cinfo.output_scanline]), 1);

  jpeg_finish_decompress(&cinfo);

  jpeg_destroy_decompress(&cinfo);

  fclose(infile);

  return 0;
}

int compress_frames(char *filename, RAWBUF_T *buf, int height, int width,
                    int quality, int verbose) {
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  FILE *outfile;
  int count = 0;
  int y;
  int last_frame_start = -1;
  int more_frames = 1;
  int scan_start;
  char chunkname[255];

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);

  cinfo.image_width = width;
  cinfo.image_height = height;
  cinfo.input_components = 3;

  /* change to YCbCr if CHAN_Y */
  if (color_channel == CHAN_Y)
    cinfo.in_color_space = JCS_YCbCr;
  else
    cinfo.in_color_space = JCS_RGB;

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, 0);

  while (1) {
    /* find next frame start */
    y = last_frame_start + 1;

    while (y < buf->height && buf->scanstart[y] == 0)
      y++;

    /* check that a whole frame exists after this FRAME_START */
    if ((y == buf->height) || (y + height >= buf->height)) {
      if (verbose)
        printf("y %d height %d tot height %d -> stopping", y, height, buf->height);
      break;
    }

    /* store last frame start */
    last_frame_start = y;

    /* store scanline start for frame */
    scan_start = buf->scanstart[y];

    if (verbose)
      printf("FRAME_START at %d, xoffs %d", y, scan_start);

    /* create filename */
    sprintf(chunkname, "%s.%03d.jpg", filename, count);

    if (verbose)
      printf(" => %s\n", chunkname);

    if ((outfile = fopen(chunkname, "wb")) == NULL) {
      fprintf(stderr, "can't open %s\n", chunkname);
      exit(1);
    }
    jpeg_stdio_dest(&cinfo, outfile);

    jpeg_start_compress(&cinfo, TRUE);

    while (cinfo.next_scanline < cinfo.image_height) {
      buf->scanline[y] += scan_start * buf->components;
      jpeg_write_scanlines(&cinfo, &(buf->scanline[y++]), 1);
    }

    jpeg_finish_compress(&cinfo);

    fclose(outfile);

    count++;
  }

  printf("\nwrote %d frames\n", count);

  jpeg_destroy_compress(&cinfo);

  return 0;
}

int find_perf_with_range(RAWBUF_T *buf, int *offs, int *total_num_perf,
                         int *total_num_frames, int *x_for_max_frames,
                         int *median_frame_height, int from_x, int to_x,
                         int verbose) {
  int frame_height_hist[max_frame_height];

  float mean_img_height_sum = 0.0;
  float mean_img_max_height_sum = 0.0;
  float mean_img_min_height_sum = 0.0;
  float mean_offs_sum = 0.0;

  int median_frame_hist = 0;

  int okcount = 0;

  int y;
  int x = from_x * buf->components;
  int xoffs;
  int val;

  memset(frame_height_hist, 0, sizeof(frame_height_hist));

  while (x < to_x * buf->components) {
    int perfstart = 0;
    int perfend = 0;
    int firststart = 0;
    int firstend = 0;
    int firstframe = 0;
    int perfheight = 0;
    int num_perf = 0;
    int num_frames = 0;
    int perfheightsum = 0;
    int in_perf = 0;
    int perf_detected = 0;
    int maxperf = 0;
    int minperf = INT_MAX;
    int perfdiff = INT_MAX;
    int maxframe = 0;
    int minframe = INT_MAX;
    int framediff = INT_MAX;
    int imgheight = 0;
    int imgheightsum = 0;

    memset(buf->scanstart, 0, buf->height * sizeof(int));

    if (color_channel == CHAN_Y)
      xoffs = 0;
    else
      xoffs = color_channel;

    if (buf->scanline[perf_y_start][x + xoffs] > white_level)
      in_perf = 1;

    for (y = perf_y_start; y < buf->height; y++) {
      val = buf->scanline[y][x + xoffs];

      if (!in_perf && val > white_level) {
        in_perf = 1;
        perf_detected = 1;
        perfstart = y;

        if (!firststart)
          firststart = perfstart;
        if (firststart && firstend)
          firstframe = perfstart;

        if (verbose > 1) {
          if (num_perf == 0)
            printf("\n");
          printf("x = %d, perf %d start %d", x / buf->components, num_perf, y);
        }
      } else if (in_perf && val < black_level &&
                 ((y - perfstart) > min_perf_height)) {
        int lastend = perfend;

        in_perf = 0;
        perfend = y;

        if (perf_detected) {
          num_perf++;

          if (!firstend)
            firstend = perfend;

          perfheight = perfend - perfstart;
          imgheight = perfend - lastend;

          if (perfheight <= max_perf_height && imgheight > min_frame_height &&
              imgheight < max_frame_height) {
            perfheightsum += perfheight;

            if (perfheight > maxperf)
              maxperf = perfheight;
            if (perfheight < minperf)
              minperf = perfheight;

            /*
             * only do stats on frames when we have detected a
             * whole frame == two perfs
             */
            if (firstframe) {
              imgheightsum += imgheight;

              frame_height_hist[imgheight]++;

              if (imgheight > maxframe)
                maxframe = imgheight;
              if (imgheight < minframe)
                minframe = imgheight;

              num_frames++;
            }

            /* mark frame start in middle of perf */
            find_xstart(buf, y - perfheight / 2, verbose);
          } else if (verbose > 1) {
            printf(" [rej]");
          }

          if (verbose > 1)
            printf(" end %d height %d frame height %d\n", y, perfheight,
                   imgheight);
        }
      }
    }

    if (maxperf >= minperf)
      perfdiff = maxperf - minperf;
    if (maxframe >= minframe)
      framediff = maxframe - minframe;

    if (verbose > 1 && maxframe > 0) {
      printf("\nx = %d, perf: max %d min %d\n", x / buf->components, maxperf,
             minperf);
      printf("        frame: max %d min %d\n", maxframe, minframe);
    }

    /* check if perf (& frame) seems ok */
    if ((perfdiff <= MAX_PERFDIFF) && (framediff <= MAX_FRAMEDIFF)) {
      okcount++;
      perfheight = perfheightsum / num_perf;

      if (verbose == 1) {
        printf("\nx = %d, perf: max %d min %d\n", x / buf->components, maxperf,
               minperf);
        printf("        frame: max %d min %d\n", maxframe, minframe);
      }

      if (verbose)
        printf("OK count now %d\n", okcount);

      mean_img_height_sum += imgheightsum / (float)num_frames;
      mean_img_max_height_sum += maxframe;
      mean_img_min_height_sum += minframe;
      mean_offs_sum += firststart + perfheightsum / (2.0 * num_frames);

      if (num_perf > *total_num_perf) *total_num_perf = num_perf;
      if (num_frames > *total_num_frames) {
        *total_num_frames = num_frames;
        *x_for_max_frames = x/buf->components;
      }
    }

    x += buf->components;
  }

  *offs = roundf(mean_offs_sum / (float)okcount);

  for (int i = 0; i < max_frame_height; i++) {
    if (frame_height_hist[i] > median_frame_hist) {
      median_frame_hist = frame_height_hist[i];
      *median_frame_height = i;
    }
  }

  return 0;
}

int find_perf(RAWBUF_T *buf, int *offs, int verbose) {
  int total_num_perf = 0;
  int total_num_frames = 0;
  int x_for_max_frames = 0;
  int median_frame_height = 0;

  /* First evalutate perfs for the whole range of x columns */
  find_perf_with_range(buf, offs, &total_num_perf, &total_num_frames,
                       &x_for_max_frames, &median_frame_height, 0,
                       FRAME_FRAC_WITH_PERF * buf->width, verbose);

  printf("global: num perfs: %d num frames: %d (at x = %d), offs to first: %d, median height: %d\n",
         total_num_perf, total_num_frames, x_for_max_frames, *offs,
         median_frame_height);

  /* Use the optimal X to index the frame starts */
  find_perf_with_range(buf, offs, &total_num_perf, &total_num_frames,
                       &x_for_max_frames, &median_frame_height, x_for_max_frames,
                       x_for_max_frames + 1, verbose);

  printf("x = %d: num perfs: %d num frames: %d , offs to first: %d, median height: %d\n",
         x_for_max_frames, total_num_perf, total_num_frames, *offs, median_frame_height);

  return 0;
}

int find_xstart(RAWBUF_T *buf, int ypos, int verbose) {
  int xpos = perf_x_start;
  int xoffs;
  int found_perf_start = 0;
  int found_perf_end = 0;
  int val;

  if (color_channel == CHAN_Y)
    xoffs = 0;
  else
    xoffs = color_channel;

  while (!found_perf_end && xpos < buf->width / 2) {
    val = buf->scanline[ypos][xpos * buf->components + xoffs];

    if (!found_perf_start && val > white_level) {
      found_perf_start = xpos;
      if (verbose > 2)
        printf(" (perf x start %d", xpos);
    } else if (found_perf_start && val < black_level) {
      found_perf_end = xpos;
      if (verbose > 2)
        printf(" end %d)", xpos);
    }

    if (!found_perf_end)
      xpos++;
  }

  if (found_perf_end > FRAME_X_NEG_OFFS)
    buf->scanstart[ypos] = found_perf_end - FRAME_X_NEG_OFFS;
  else
    buf->scanstart[ypos] = 1;

  return found_perf_end;
}

int rotate_strip(RAWBUF_T *buf, int deg, int verbose) {
  JSAMPLE *newbuf;
  JSAMPARRAY newscanline;
  int *newscanstart;
  int newwidth;
  int newheight;
  int x, y, i;

  if (deg == -90) {
    newwidth = buf->height;
    newheight = buf->width;

    if (verbose)
      printf("rotating -90 deg...\n");

    newbuf = (JSAMPLE *)malloc(newwidth * newheight * buf->components);
    newscanline = (JSAMPARRAY)malloc(newheight * sizeof(JSAMPLE *));
    newscanstart = (int *)malloc(newheight * sizeof(int));

    /* set up new scanlines */
    for (y = 0; y < newheight; y++)
      newscanline[y] = newbuf + (y * newwidth * buf->components);

    /* rotate into new buffer */
    for (y = 0; y < buf->height; y++) {
      for (x = 0; x < buf->width; x++) {
        for (i = 0; i < buf->components; i++) {
          newscanline[x][y * buf->components + i] =
              buf->scanline[y][(buf->width - (x + 1)) * buf->components + i];
        }
      }
    }

    free_buf(buf);

    buf->height = newheight;
    buf->width = newwidth;
    buf->buffer = newbuf;
    buf->scanline = newscanline;
    buf->scanstart = newscanstart;
  } else {
    return -1;
  }

  return 0;
}

void free_buf(RAWBUF_T *buf) {
  if (buf->scanline)
    free(buf->scanline);
  if (buf->scanstart)
    free(buf->scanstart);
  if (buf->buffer)
    free(buf->buffer);
}
