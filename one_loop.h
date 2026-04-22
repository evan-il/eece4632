#ifndef ONELOOP_HLS_H
#define ONELOOP_HLS_H

//#define NDEBUG
#include <cstdint>
#include "ap_int.h"
#include "hls_stream.h"
#include "ap_axi_sdata.h"

#define IMG_HEIGHT  480
#define IMG_WIDTH   640

#define KERNEL_SIZE 5

typedef ap_uint<8> pixel_t;
// fits 255 * 16 * 16 = 65280 (need 17 bits; 20 for margin)
typedef ap_axiu<8,1,1,1> axis_t;
typedef ap_int<24>  acc_t;
typedef ap_uint<24> mag_t;

struct axi_pixel_t {
    pixel_t    data;
    ap_uint<1> last;
};

// height/width allows the tb to use a small image without processing
// the full IMG_HEIGHT x IMG_WIDTH frame every call
void one_loop(
    hls::stream<axis_t> &src,
    hls::stream<axis_t> &dst,
    int rows,
    int col,
);

#endif