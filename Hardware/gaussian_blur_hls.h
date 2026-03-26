#ifndef GAUSSIAN_BLUR_HLS_H
#define GAUSSIAN_BLUR_HLS_H

#define NDEBUG
#include "ap_int.h"

#define IMG_HEIGHT  480
#define IMG_WIDTH   640

#define KERNEL_SIZE 5
#define KERNEL_HALF 2
#define KERNEL_SUM  16

typedef ap_uint<8>  pixel_t;
typedef ap_uint<12> accum_t;

void gaussian_blur(
    pixel_t input [IMG_HEIGHT][IMG_WIDTH],
    pixel_t output[IMG_HEIGHT][IMG_WIDTH]
);

#endif
