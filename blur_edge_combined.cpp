#include "blur_edge_combined.h"

// Gaussian-smoothed 5x5 Sobel kernel (separable: [1,4,6,4,1]^T x [-1,-2,0,2,1]).
// Applying this to raw pixels is mathematically equivalent to Gaussian blur then Sobel
// combined into a single pass. SOBEL[kr][kc] = Gx; SOBEL[kc][kr] = Gy (transpose).
static const int SOBEL[KERNEL_SIZE][KERNEL_SIZE] = {
    {-1,  -2, 0,  2, 1},
    {-4,  -8, 0,  8, 4},
    {-6, -12, 0, 12, 6},
    {-4,  -8, 0,  8, 4},
    {-1,  -2, 0,  2, 1}
};

void blur_edge(
    hls::stream<axis_t> &src,
    hls::stream<axis_t> &dst,
    int height,
    int width,
    int threshold
) {
    #pragma HLS INTERFACE axis      port=src
    #pragma HLS INTERFACE axis      port=dst
    #pragma HLS INTERFACE s_axilite port=height
    #pragma HLS INTERFACE s_axilite port=width
    #pragma HLS INTERFACE s_axilite port=threshold
    #pragma HLS INTERFACE s_axilite port=return

    static pixel_t line_buf[KERNEL_SIZE][IMG_WIDTH];
    #pragma HLS ARRAY_PARTITION variable=line_buf complete dim=1

    pixel_t window[KERNEL_SIZE][KERNEL_SIZE];
    #pragma HLS ARRAY_PARTITION variable=window complete dim=0

    row_loop: for (int r = 0; r < height; r++) {
        #pragma HLS LOOP_TRIPCOUNT min=1 max=IMG_HEIGHT
        col_loop: for (int c = 0; c < width; c++) {
            #pragma HLS LOOP_TRIPCOUNT min=1 max=IMG_WIDTH
            #pragma HLS PIPELINE II=1

            // 1. Read pixel from input stream
            axis_t pix_in = src.read();
            pixel_t px = pix_in.data;

            // 2. Store in circular line buffer
            line_buf[r % KERNEL_SIZE][c] = px;

            // 3. Shift window columns left
            for (int kr = 0; kr < KERNEL_SIZE; kr++) {
                #pragma HLS UNROLL
                for (int kc = 0; kc < KERNEL_SIZE - 1; kc++) {
                    #pragma HLS UNROLL
                    window[kr][kc] = window[kr][kc + 1];
                }
            }

            // 4. Load new rightmost column; use px directly for current row to avoid RAW
            for (int kr = 0; kr < KERNEL_SIZE; kr++) {
                #pragma HLS UNROLL
                int ri = r - KERNEL_SIZE + 1 + kr;
                if (ri < 0) ri = 0;
                window[kr][KERNEL_SIZE - 1] = (ri == r) ? px : line_buf[ri % KERNEL_SIZE][c];
            }

            // 5. Left-border clamping: redirect invalid columns to the first real column
            int kc_start = (c < KERNEL_SIZE - 1) ? (KERNEL_SIZE - 1 - c) : 0;

            // 6. Compute Gx and Gy using the fused Gaussian-Sobel kernel
            accum_t Gx = 0, Gy = 0;
            for (int kr = 0; kr < KERNEL_SIZE; kr++) {
                #pragma HLS UNROLL
                for (int kc = 0; kc < KERNEL_SIZE; kc++) {
                    #pragma HLS UNROLL
                    int kc_c = (kc < kc_start) ? kc_start : kc;
                    pixel_t pix = window[kr][kc_c];
                    Gx += (accum_t)SOBEL[kr][kc] * (accum_t)pix;
                    Gy += (accum_t)SOBEL[kc][kr] * (accum_t)pix;
                }
            }

            // 7. L1 magnitude — cast both ternary branches to ap_int<17> explicitly so HLS
            //    sees matching types (-Gx promotes ap_int<16> to ap_int<17>; Gx does not)
            accum_t mag = (accum_t)(Gx < 0 ? (ap_int<17>)(-Gx) : (ap_int<17>)(Gx))
                        + (accum_t)(Gy < 0 ? (ap_int<17>)(-Gy) : (ap_int<17>)(Gy));

            // 8. Suppress border pixels, apply threshold
            pixel_t result;
            if (r < KERNEL_HALF || c < KERNEL_HALF) {
                result = 0;
            } else {
                result = (mag >= (accum_t)threshold) ? (pixel_t)255 : (pixel_t)0;
            }

            // 9. Write output pixel
            axis_t pix_out;
            pix_out.data = result;
            pix_out.keep = pix_in.keep;
            pix_out.strb = pix_in.strb;
            pix_out.last = pix_in.last;
            dst.write(pix_out);
        }
    }
}
