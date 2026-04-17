#include "blur_edge_combined.h"
#include <cstdint>

static const uint8_t KERNEL[KERNEL_SIZE] = {1, 4, 6, 4, 1};
static const uint8_t KERNEL[KERNEL_SIZE] = {1, 4, 6, 4, 1};

void blur_edge (
    hls::stream<axis_t> &src,
    hls::stream<axis_t> &dst,
    int height,
    int width,
    pixel_t threshold
) {
    #pragma HLS INTERFACE axis      port=src
    #pragma HLS INTERFACE axis      port=dst
    #pragma HLS INTERFACE s_axilite port=height
    #pragma HLS INTERFACE s_axilite port=width
    #pragma HLS INTERFACE s_axilite port=threshold
    #pragma HLS INTERFACE s_axilite port=return

    // changing to 5x5 kernel for edge detection - blurrier edges compared to 3x3
    // but this application doesnt require sensitive edge detection
    static pixel_t line_buf[KERNEL_SIZE][IMG_WIDTH];
    #pragma HLS ARRAY_PARTITION variable=line_buf complete dim=1
    pixel_t window[KERNEL_SIZE][KERNEL_SIZE];
    #pragma HLS ARRAY_PARTITION variable=window complete dim=0

    row_loop: for (int r = 0; r < height; r++) {
        #pragma HLS LOOP_TRIPCOUNT min=1 max=IMG_HEIGHT
        col_loop: for (int c = 0; c < width; c++) {
            #pragma HLS LOOP_TRIPCOUNT min=1 max=IMG_WIDTH
            #pragma HLS PIPELINE II=1

            // ---- 1. Read one pixel from the input stream ----
            axis_t pix_in = src.read();
            pixel_t px = pix_in.data;

            // ---- 2. Store in circular line buffer ----
            line_buf[r % KERNEL_SIZE][c] = px;

            // ---- 3. Shift window right (discard oldest column) ----
            for (int kr = 0; kr < KERNEL_SIZE; kr++) {
                #pragma HLS UNROLL
                for (int kc = 0; kc < KERNEL_SIZE - 1; kc++) {
                    #pragma HLS UNROLL
                    window[kr][kc] = window[kr][kc + 1];
                }
            }

            // ---- 4. Load new rightmost window column from line buffer ----
            // window[kr][KERNEL_SIZE-1] gets row index (r - KERNEL_SIZE+1 + kr).
            // When kr == KERNEL_SIZE-1 the index equals r (current row); use
            // the freshly read pixel directly to avoid a same-cycle RAW on
            // line_buf.  Clamp negative indices to row 0 (top border).
            for (int kr = 0; kr < KERNEL_SIZE; kr++) {
                #pragma HLS UNROLL
                int ri = r - KERNEL_SIZE + 1 + kr;
                if (ri < 0) ri = 0;
                window[kr][KERNEL_SIZE - 1] = (ri == r) ? px
                                                         : line_buf[ri % KERNEL_SIZE][c];
            }

            // ---- 5. Left-border clamping ----
            // For c < KERNEL_SIZE-1, window slots 0..(kc_start-1) were filled
            // with data from the end of the previous row (wrap-around garbage).
            // Redirect those positions to slot kc_start, which holds the first
            // real pixel of the current row (column 0 = left border replicate).
            int kc_start = (c < KERNEL_SIZE - 1) ? (KERNEL_SIZE - 1 - c) : 0;

            // ---- 6. 2D separable Gaussian convolution ----
            accum_t sum = 0;
            for (int kr = 0; kr < KERNEL_SIZE; kr++) {
                #pragma HLS UNROLL
                accum_t row_acc = 0;
                for (int kc = 0; kc < KERNEL_SIZE; kc++) {
                    #pragma HLS UNROLL
                    int kc_c = (kc < kc_start) ? kc_start : kc;
                    row_acc += (accum_t)window[kr][kc_c] * KERNEL[kc];
                }
                sum += row_acc * KERNEL[kr];
            }

            // calculate sobel gradients 5x5 kernel
            accum_t Gx = (accum_t)(-1)*win[0][0] + (accum_t)(+1)*win[0][2]
                       + (accum_t)(-2)*win[1][0] + (accum_t)(+2)*win[1][2]
                       + (accum_t)(-1)*win[2][0] + (accum_t)(+1)*win[2][2];

            accum_t Gy = (accum_t)(+1)*win[0][0] + (accum_t)(+2)*win[0][1] + (accum_t)(+1)*win[0][2]
                       + (accum_t)(-1)*win[2][0] + (accum_t)(-2)*win[2][1] + (accum_t)(-1)*win[2][2];

            // ---- 5. L1 magnitude approximation (no sqrt needed) ----
            // Cast resolves ap_int width ambiguity: negation promotes ap_int<16>
            // to ap_int<17>, making the ternary branches incompatible without it.
            accum_t mag = (accum_t)(Gx < 0 ? (ap_int<17>)(-Gx) : (ap_int<17>)(Gx)) + ((Gy < 0) ? (ap_int<17>)(-Gy) : (ap_int<17>)(Gy));


            // ---- 7. Scale and write output pixel ----
            // Total kernel weight = 16 * 16 = 256, so shift right by 8.
            axis_t pix_out;
            pix_out.data = (pixel_t)(sum >> 8);
            pix_out.keep = pix_in.keep;
            pix_out.strb = pix_in.strb;
            pix_out.last = pix_in.last;
            dst.write(pix_out);
        }
    }
}