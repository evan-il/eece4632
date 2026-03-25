#ifndef GAUSSIAN_BLUR_HLS_H
#define GAUSSIAN_BLUR_HLS_H

#include "ap_int.h"

// --- Image dimensions — change to match your input matrices ---
#define IMG_HEIGHT  480
#define IMG_WIDTH   640

// --- Kernel: 5x5 separable Gaussian [1, 4, 6, 4, 1] / 16 ---
// Approximates sigma ~1.0. Clean power-of-2 sum (16) means
// the divide can be replaced by a 4-bit right shift in hardware.
#define KERNEL_SIZE 5
#define KERNEL_HALF 2       // KERNEL_SIZE / 2
#define KERNEL_SUM  16      // sum of 1D kernel coefficients

typedef ap_uint<8>  pixel_t;   // single channel, 0-255
typedef ap_uint<12> accum_t;   // 8-bit pixel * max coeff (6) * 5 taps = max 7650, fits in 13 bits

// Top-level function — this becomes the HLS IP block
void gaussian_blur(
    pixel_t input [IMG_HEIGHT][IMG_WIDTH],
    pixel_t output[IMG_HEIGHT][IMG_WIDTH]
);

#endif
