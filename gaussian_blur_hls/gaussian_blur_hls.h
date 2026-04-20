#ifndef GAUSSIAN_BLUR_HLS_H
#define GAUSSIAN_BLUR_HLS_H

#define NDEBUG
#include "ap_int.h"
#include "hls_stream.h"
#include "ap_axi_sdata.h"

#define IMG_HEIGHT  480
#define IMG_WIDTH   640

#define KERNEL_SIZE 5
#define KERNEL_HALF 2
#define KERNEL_SUM  16

typedef ap_uint<8>  pixel_t;
typedef ap_uint<20> accum_t;  // fits 255 * 16 * 16 = 65280 (need 17 bits; 20 for margin)
typedef ap_axiu<8,1,1,1> axis_t;

// height/width allow the testbench to use a small image without processing
// the full IMG_HEIGHT x IMG_WIDTH frame on every C-simulation call.
void gaussian_blur(
    hls::stream<axis_t> &src,
    hls::stream<axis_t> &dst,
    int height,
    int width
);

#endif
