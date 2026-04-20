#ifndef BLUR_EDGE_HLS_H
#define BLUR_EDGE_HLS_H

#define NDEBUG
#include "ap_int.h"
#include "hls_stream.h"
#include "ap_axi_sdata.h"

#define IMG_HEIGHT  480
#define IMG_WIDTH   640

#define KERNEL_SIZE 5
#define KERNEL_HALF 2

typedef ap_uint<8>   pixel_t;
typedef ap_int<16>   accum_t;   // signed — Sobel produces negative gradients; max |Gx|=12240 fits
typedef ap_axiu<8,1,1,1> axis_t;

void blur_edge(
    hls::stream<axis_t> &src,
    hls::stream<axis_t> &dst,
    int height,
    int width,
    int threshold
);

#endif
