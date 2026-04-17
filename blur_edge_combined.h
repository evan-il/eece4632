#ifndef BLUR_EDGE_HLS_H
#define BLUR_EDGE_HLS_H

#define NDEBUG
#include "ap_int.h"
#include "hls_stream.h"
#include "ap_axi_sdata.h"

#define IMG_HEIGHT  480
#define IMG_WIDTH   640

#define KERNEL_SIZE 5

typedef ap_uint<8>   pixel_t;
// fits 255 * 16 * 16 = 65280 (need 17 bits; 20 for margin)
typedef ap_int<16>   accum_t;   // signed — Sobel produces negative gradients
typedef ap_axiu<8,1,1,1> axis_t;

// height/width allows the tb to use a small image without processing
// the full IMG_HEIGHT x IMG_WIDTH frame every call
void blur_edge(
    hls::stream<axis_t> &src,
    hls::stream<axis_t> &dst,
    int height,
    int width,
    pixel_t threshold
);

#endiF