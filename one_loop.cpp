#include "one_loop.h"

//  GX_GAUSS = SOBEL_X ⋆ GAUSS
//  GY_GAUSS = SOBEL_Y ⋆ GAUSS
static const int GX_GAUSS[9][9] = {
    {  -1,   -6,  -14,  -14,    0,   14,   14,    6,   1 },
    {  -8,  -48, -112, -112,    0,  112,  112,   48,   8 },
    { -28, -168, -392, -392,    0,  392,  392,  168,  28 },
    { -56, -336, -784, -784,    0,  784,  784,  336,  56 },
    { -70, -420, -980, -980,    0,  980,  980,  420,  70 },
    { -56, -336, -784, -784,    0,  784,  784,  336,  56 },
    { -28, -168, -392, -392,    0,  392,  392,  168,  28 },
    {  -8,  -48, -112, -112,    0,  112,  112,   48,   8 },
    {  -1,   -6,  -14,  -14,    0,   14,   14,    6,   1 },
};
static const int GY_COMBINED[9][9] = {
    {  -1,   -8,  -28,  -56,  -70,  -56,  -28,   -8,  -1 },
    {  -6,  -48, -168, -336, -420, -336, -168,  -48,  -6 },
    { -14, -112, -392, -784, -980, -784, -392, -112, -14 },
    { -14, -112, -392, -784, -980, -784, -392, -112, -14 },
    {   0,    0,    0,    0,    0,    0,    0,    0,   0 },
    {  14,  112,  392,  784,  980,  784,  392,  112,  14 },
    {  14,  112,  392,  784,  980,  784,  392,  112,  14 },
    {   6,   48,  168,  336,  420,  336,  168,   48,   6 },
    {   1,    8,   28,   56,   70,   56,   28,    8,   1 },
};

static inline pixel_t clamp_u8(mag_t v) {
    #pragma HLS INLINE
    return (v > 255) ? (pixel_t)255 : (pixel_t)v;
}

void one_loop(
    hls::stream<axi_pixel_t>& src,
    hls::stream<axi_pixel_t>& dst,
    int rows,
    int cols) {
 
    #pragma HLS INTERFACE axis      port=src
    #pragma HLS INTERFACE axis      port=dst
    #pragma HLS INTERFACE s_axilite port=rows   bundle=ctrl
    #pragma HLS INTERFACE s_axilite port=cols   bundle=ctrl
    #pragma HLS INTERFACE s_axilite port=return bundle=ctrl

    pixel_t line_buf[8][IMG_WIDTH];
    #pragma HLS ARRAY_PARTITION variable=line_buf complete dim=1

    pixel_t window[9][9];
    #pragma HLS ARRAY_PARTITION variable=window complete dim=0

    ROW_LOOP: for (int r = 0; r < rows; r++) {
        COL_LOOP: for (int c = 0; c < cols; c++) {
            #pragma HLS PIPELINE II=1

            axi_pixel_t in_px = src.read();
            pixel_t in_val = in_px.data;
            
            // move window
            SHIFT_R: for (int i = 0; i < 9; i++) {
                #pragma HLS UNROLL
                SHIFT_C: for (int j = 0; j < 8; j++) {
                    #pragma HLS UNROLL
                    window[i][j] = window[i][j+1];
                }
            }

            FILL_COL: for (int i = 0; i < 8; i++) {
                #pragma HLS UNROLL
                window[i][8] = line_buf[i][c];
            }
            window[8][8] = in_val;

            
            BUF: for (int i = 0; i < 7; i++) {
                #pragma HLS UNROLL
                line_buf[i][c] = line_buf[i+1][c];
            }
            line_buf[7][c] = in_val;

            acc_t gx = 0, gy = 0;
            CALC_R: for (int i = 0; i < 9; i++) {
                #pragma HLS UNROLL
                CALC_C: for (int j = 0; j < 9; j++) {
                    #pragma HLS UNROLL
                    acc_t p = (acc_t)window[i][j];
                    gx += p * GX_GAUSS[i][j];
                    gy += p * GY_GAUSS[i][j];
                }
            }

            mag_t abs_gx = (gx < 0) ? (mag_t)(-gx) : (mag_t)gx;
            mag_t abs_gy = (gy < 0) ? (mag_t)(-gy) : (mag_t)gy;
            mag_t mag = (abs_gx + abs_gy) >> 13;

            axi_pixel_t out_px;
            out_px.data = (r >= 8 && c >= 8) ? clamp_u8(mag) : (pixel_t)0;
            out_px.last = (r == rows-1 && c == cols-1) ? 1 : 0;
            dst.write(out_px);
        }
    }
    }