#ifndef EDGE_DETECT_HLS_H
#define EDGE_DETECT_HLS_H

#define NDEBUG
#include "ap_int.h"
#include "hls_stream.h"
#include "ap_axi_sdata.h"

#define IMG_HEIGHT  480
#define IMG_WIDTH   640

typedef ap_uint<8>   pixel_t;
typedef ap_int<16>   accum_t;   // signed — Sobel produces negative gradients
typedef ap_axiu<8,1,1,1> axis_t;

void edge_detect(
    hls::stream<axis_t> &src,
    hls::stream<axis_t> &dst,
    int height,
    int width,
    pixel_t threshold
);

#endif
