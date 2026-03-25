#include "gaussian_blur_hls.h"

// 1D Gaussian kernel coefficients — applied horizontally then vertically
// Full kernel is the outer product: [1,4,6,4,1]^T * [1,4,6,4,1] / 256
// Using separable form cuts multiplies from k^2 to 2k per pixel
static const uint8_t KERNEL[KERNEL_SIZE] = {1, 4, 6, 4, 1};

// -----------------------------------------------------------------------
// gaussian_blur
//
// Applies a separable 5x5 Gaussian blur to a single-channel (grayscale)
// image using two 1D convolution passes:
//   Pass 1 — horizontal: reads input,  writes line_buf
//   Pass 2 — vertical:   reads line_buf, writes output
//
// Border pixels are handled with clamp-to-edge padding.
//
// AXI4 master ports let the PS DMA data to/from DRAM.
// AXI4-Lite slave port exposes the start/done control registers.
// -----------------------------------------------------------------------
void gaussian_blur(
    pixel_t input [IMG_HEIGHT][IMG_WIDTH],
    pixel_t output[IMG_HEIGHT][IMG_WIDTH]
)
{
    // --- Interface pragmas ---
    // m_axi: AXI4 master — block reads/writes DRAM directly
    // s_axilite: AXI4-Lite slave — PS starts the block and polls done
    #pragma HLS INTERFACE m_axi     port=input  offset=slave bundle=gmem0 depth=IMG_HEIGHT*IMG_WIDTH
    #pragma HLS INTERFACE m_axi     port=output offset=slave bundle=gmem1 depth=IMG_HEIGHT*IMG_WIDTH
    #pragma HLS INTERFACE s_axilite port=return

    // Intermediate buffer holding the horizontally blurred image
    // Partitioned along columns by factor 5 so the vertical pass
    // can read 5 rows simultaneously without memory conflicts
    static pixel_t line_buf[IMG_HEIGHT][IMG_WIDTH];
    #pragma HLS ARRAY_PARTITION variable=line_buf cyclic factor=KERNEL_SIZE dim=1

    // ----------------------------------------------------------------
    // Pass 1 — horizontal convolution
    // For each pixel (r, c): convolve row r with KERNEL across columns
    // ----------------------------------------------------------------
    h_row: for (int r = 0; r < IMG_HEIGHT; r++) {
        #pragma HLS LOOP_TRIPCOUNT min=IMG_HEIGHT max=IMG_HEIGHT
        h_col: for (int c = 0; c < IMG_WIDTH; c++) {
            #pragma HLS PIPELINE II=1   // one output pixel per clock cycle

            accum_t sum = 0;
            h_k: for (int k = 0; k < KERNEL_SIZE; k++) {
                #pragma HLS UNROLL      // unroll inner loop — all taps in parallel

                // Clamp column index to image border
                int ci = c - KERNEL_HALF + k;
                if (ci < 0)          ci = 0;
                if (ci >= IMG_WIDTH) ci = IMG_WIDTH - 1;

                sum += (accum_t)(input[r][ci]) * KERNEL[k];
            }

            // Divide by kernel sum (16) — synthesises as a 4-bit right shift
            line_buf[r][c] = (pixel_t)(sum >> 4);
        }
    }

    // ----------------------------------------------------------------
    // Pass 2 — vertical convolution
    // For each pixel (r, c): convolve column c with KERNEL across rows
    // ----------------------------------------------------------------
    v_row: for (int r = 0; r < IMG_HEIGHT; r++) {
        #pragma HLS LOOP_TRIPCOUNT min=IMG_HEIGHT max=IMG_HEIGHT
        v_col: for (int c = 0; c < IMG_WIDTH; c++) {
            #pragma HLS PIPELINE II=1

            accum_t sum = 0;
            v_k: for (int k = 0; k < KERNEL_SIZE; k++) {
                #pragma HLS UNROLL

                // Clamp row index to image border
                int ri = r - KERNEL_HALF + k;
                if (ri < 0)           ri = 0;
                if (ri >= IMG_HEIGHT) ri = IMG_HEIGHT - 1;

                sum += (accum_t)(line_buf[ri][c]) * KERNEL[k];
            }

            output[r][c] = (pixel_t)(sum >> 4);
        }
    }
}
