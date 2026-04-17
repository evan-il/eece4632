#include "gaussian_blur_hls.h"
#include <cstdint>

static const uint8_t KERNEL[KERNEL_SIZE] = {1, 4, 6, 4, 1};

// AXI Stream Gaussian blur — single-pass 2D separable convolution.
//
// Interface:
//   src / dst  — AXI4-Stream (axis), one 8-bit pixel per beat, raster order
//   height / width — frame dimensions, written once via AXI4-Lite before start
//   return     — AXI4-Lite control / status
//
// Border handling:
//   Top    — replicated (line buffer clamps negative row indices to row 0)
//   Left   — replicated (kc_start clamps negative column indices to column 0)
//   Bottom / Right — causal: the filter uses only already-received pixels,
//                   so the last KERNEL_HALF rows / columns of the output
//                   are equivalent to border replication of the current edge.
//
// Output alignment:
//   The convolution window is causal (contains the current pixel and
//   KERNEL_SIZE-1 earlier pixels in each dimension).  Output pixel (r, c)
//   in the stream therefore represents the blur *centered* at image position
//   (r - KERNEL_HALF, c - KERNEL_HALF).  Consumers that need pixel-accurate
//   alignment should account for this KERNEL_HALF-row / KERNEL_HALF-column
//   offset, or simply ignore the first KERNEL_HALF rows and columns of output.

void gaussian_blur(
    hls::stream<axis_t> &src,
    hls::stream<axis_t> &dst,
    int height,
    int width
)
{
    #pragma HLS INTERFACE axis      port=src
    #pragma HLS INTERFACE axis      port=dst
    #pragma HLS INTERFACE s_axilite port=height
    #pragma HLS INTERFACE s_axilite port=width
    #pragma HLS INTERFACE s_axilite port=return

    // Circular line buffer — holds the last KERNEL_SIZE rows.
    // complete dim=1 gives each row its own BRAM/RAM bank so all five rows
    // can be read simultaneously in a single pipeline cycle.
    static pixel_t line_buf[KERNEL_SIZE][IMG_WIDTH];
    #pragma HLS ARRAY_PARTITION variable=line_buf complete dim=1

    // Column sliding window — fully in registers for parallel read/write.
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

            // ---- 3. Shift window left (discard oldest column) ----
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
