#include "gaussian_blur_hls.h"
#include <cstdint>

static const uint8_t KERNEL[KERNEL_SIZE] = {1, 4, 6, 4, 1};

void gaussian_blur(
    pixel_t input [IMG_HEIGHT][IMG_WIDTH],
    pixel_t output[IMG_HEIGHT][IMG_WIDTH]
)
{
    #pragma HLS INTERFACE m_axi     port=input  offset=slave bundle=gmem0 depth=IMG_HEIGHT*IMG_WIDTH
    #pragma HLS INTERFACE m_axi     port=output offset=slave bundle=gmem1 depth=IMG_HEIGHT*IMG_WIDTH
    #pragma HLS INTERFACE s_axilite port=return

    static pixel_t line_buf[IMG_HEIGHT][IMG_WIDTH];
    #pragma HLS ARRAY_PARTITION variable=line_buf cyclic factor=KERNEL_SIZE dim=1

    // Horizontal pass
    h_row: for (int r = 0; r < IMG_HEIGHT; r++) {
        #pragma HLS LOOP_TRIPCOUNT min=IMG_HEIGHT max=IMG_HEIGHT
        h_col: for (int c = 0; c < IMG_WIDTH; c++) {
            #pragma HLS PIPELINE II=1
            accum_t sum = 0;
            h_k: for (int k = 0; k < KERNEL_SIZE; k++) {
                #pragma HLS UNROLL
                int ci = c - KERNEL_HALF + k;
                if (ci < 0)          ci = 0;
                if (ci >= IMG_WIDTH) ci = IMG_WIDTH - 1;
                sum += (accum_t)(input[r][ci]) * KERNEL[k];
            }
            line_buf[r][c] = (pixel_t)(sum >> 4);
        }
    }

    // Vertical pass
    v_row: for (int r = 0; r < IMG_HEIGHT; r++) {
        #pragma HLS LOOP_TRIPCOUNT min=IMG_HEIGHT max=IMG_HEIGHT
        v_col: for (int c = 0; c < IMG_WIDTH; c++) {
            #pragma HLS PIPELINE II=1
            accum_t sum = 0;
            v_k: for (int k = 0; k < KERNEL_SIZE; k++) {
                #pragma HLS UNROLL
                int ri = r - KERNEL_HALF + k;
                if (ri < 0)           ri = 0;
                if (ri >= IMG_HEIGHT) ri = IMG_HEIGHT - 1;
                sum += (accum_t)(line_buf[ri][c]) * KERNEL[k];
            }
            output[r][c] = (pixel_t)(sum >> 4);
        }
    }
}
