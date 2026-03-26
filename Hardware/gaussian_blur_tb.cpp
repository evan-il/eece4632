#include <iostream>
#include <cstdint>
#include <cmath>
#include <cstring>
#include "gaussian_blur_hls.h"

// -----------------------------------------------------------------------
// Testbench for gaussian_blur()
//
// Uses a small image so C simulation runs quickly.
// Three test patterns are checked against a software reference:
//   1. Impulse    — single white pixel in the centre; output should look
//                   like the 5x5 kernel itself
//   2. Gradient   — horizontal ramp 0-255; blur should smooth it slightly
//   3. Checkerboard — high-frequency pattern; blur should reduce contrast
//
// Pass criterion: every output pixel is within ±1 of the reference
// (allows for integer rounding differences between HW and SW paths).
// -----------------------------------------------------------------------

// Use a smaller image for simulation speed
#define TB_HEIGHT 16
#define TB_WIDTH  16

// Tolerance for integer rounding
#define MAX_ERROR 1

// -----------------------------------------------------------------------
// Software reference: same separable 5x5 Gaussian with clamp-to-edge
// -----------------------------------------------------------------------
static const uint8_t KERNEL[KERNEL_SIZE] = {1, 4, 6, 4, 1};

static void ref_gaussian_blur(
    uint8_t input [TB_HEIGHT][TB_WIDTH],
    uint8_t output[TB_HEIGHT][TB_WIDTH])
{
    float tmp[TB_HEIGHT][TB_WIDTH];

    // Horizontal pass
    for (int r = 0; r < TB_HEIGHT; r++) {
        for (int c = 0; c < TB_WIDTH; c++) {
            float sum = 0;
            for (int k = 0; k < KERNEL_SIZE; k++) {
                int ci = c - KERNEL_HALF + k;
                if (ci < 0)          ci = 0;
                if (ci >= TB_WIDTH)  ci = TB_WIDTH - 1;
                sum += input[r][ci] * KERNEL[k];
            }
            tmp[r][c] = sum / KERNEL_SUM;
        }
    }

    // Vertical pass
    for (int r = 0; r < TB_HEIGHT; r++) {
        for (int c = 0; c < TB_WIDTH; c++) {
            float sum = 0;
            for (int k = 0; k < KERNEL_SIZE; k++) {
                int ri = r - KERNEL_HALF + k;
                if (ri < 0)           ri = 0;
                if (ri >= TB_HEIGHT)  ri = TB_HEIGHT - 1;
                sum += tmp[ri][c] * KERNEL[k];
            }
            output[r][c] = (uint8_t)(sum / KERNEL_SUM + 0.5f);  // round to nearest
        }
    }
}

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------
static void print_image(const char* label, uint8_t img[TB_HEIGHT][TB_WIDTH])
{
    std::cout << "\n--- " << label << " ---\n";
    for (int r = 0; r < TB_HEIGHT; r++) {
        for (int c = 0; c < TB_WIDTH; c++)
            std::cout << (int)img[r][c] << "\t";
        std::cout << "\n";
    }
}

// Copy uint8 array into pixel_t (ap_uint<8>) array expected by HLS function
static void fill_hls_input(
    uint8_t    src[TB_HEIGHT][TB_WIDTH],
    pixel_t    dst[IMG_HEIGHT][IMG_WIDTH])
{
    // Clamp-fill the full IMG_HEIGHT x IMG_WIDTH array so that HLS border
    // clamping (which uses IMG_HEIGHT/IMG_WIDTH) reads edge pixels, not zeros.
    for (int r = 0; r < IMG_HEIGHT; r++) {
        int sr = (r < TB_HEIGHT) ? r : TB_HEIGHT - 1;
        for (int c = 0; c < IMG_WIDTH; c++) {
            int sc = (c < TB_WIDTH) ? c : TB_WIDTH - 1;
            dst[r][c] = src[sr][sc];
        }
    }
}

// Read HLS output back into a plain uint8 array
static void read_hls_output(
    pixel_t    src[IMG_HEIGHT][IMG_WIDTH],
    uint8_t    dst[TB_HEIGHT][TB_WIDTH])
{
    for (int r = 0; r < TB_HEIGHT; r++)
        for (int c = 0; c < TB_WIDTH; c++)
            dst[r][c] = (uint8_t)src[r][c];
}

// Compare HLS output against software reference; returns number of mismatches
static int compare(
    uint8_t hls_out[TB_HEIGHT][TB_WIDTH],
    uint8_t ref_out[TB_HEIGHT][TB_WIDTH])
{
    int errors = 0;
    for (int r = 0; r < TB_HEIGHT; r++) {
        for (int c = 0; c < TB_WIDTH; c++) {
            int diff = abs((int)hls_out[r][c] - (int)ref_out[r][c]);
            if (diff > MAX_ERROR) {
                std::cout << "MISMATCH at (" << r << "," << c << "): "
                          << "HLS=" << (int)hls_out[r][c]
                          << "  REF=" << (int)ref_out[r][c]
                          << "  diff=" << diff << "\n";
                errors++;
            }
        }
    }
    return errors;
}

// -----------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------
int main()
{
    // Static so large arrays don't blow the stack (HLS sim requirement)
    static pixel_t hls_in [IMG_HEIGHT][IMG_WIDTH];
    static pixel_t hls_out[IMG_HEIGHT][IMG_WIDTH];

    static uint8_t in [TB_HEIGHT][TB_WIDTH];
    static uint8_t out_hls[TB_HEIGHT][TB_WIDTH];
    static uint8_t out_ref[TB_HEIGHT][TB_WIDTH];

    int total_errors = 0;

    // ----------------------------------------------------------------
    // Test 1: Impulse
    // Single white pixel at centre — output should match the 5x5 kernel
    // ----------------------------------------------------------------
    std::cout << "=== Test 1: Impulse ===\n";
    memset(in, 0, sizeof(in));
    in[TB_HEIGHT / 2][TB_WIDTH / 2] = 255;

    ref_gaussian_blur(in, out_ref);
    fill_hls_input(in, hls_in);
    gaussian_blur(hls_in, hls_out);
    read_hls_output(hls_out, out_hls);

    print_image("Input",     in);
    print_image("HLS output", out_hls);
    print_image("Reference",  out_ref);

    int e1 = compare(out_hls, out_ref);
    std::cout << "Test 1: " << (e1 == 0 ? "PASS" : "FAIL") << " (" << e1 << " mismatches)\n";
    total_errors += e1;

    // ----------------------------------------------------------------
    // Test 2: Horizontal gradient (ramp 0 → 255 across columns)
    // ----------------------------------------------------------------
    std::cout << "\n=== Test 2: Gradient ===\n";
    for (int r = 0; r < TB_HEIGHT; r++)
        for (int c = 0; c < TB_WIDTH; c++)
            in[r][c] = (uint8_t)((c * 255) / (TB_WIDTH - 1));

    ref_gaussian_blur(in, out_ref);
    fill_hls_input(in, hls_in);
    gaussian_blur(hls_in, hls_out);
    read_hls_output(hls_out, out_hls);

    int e2 = compare(out_hls, out_ref);
    std::cout << "Test 2: " << (e2 == 0 ? "PASS" : "FAIL") << " (" << e2 << " mismatches)\n";
    total_errors += e2;

    // ----------------------------------------------------------------
    // Test 3: Checkerboard (high-frequency pattern)
    // Blur should reduce contrast — no pixel should stay at 255 or 0
    // ----------------------------------------------------------------
    std::cout << "\n=== Test 3: Checkerboard ===\n";
    for (int r = 0; r < TB_HEIGHT; r++)
        for (int c = 0; c < TB_WIDTH; c++)
            in[r][c] = ((r + c) % 2 == 0) ? 255 : 0;

    ref_gaussian_blur(in, out_ref);
    fill_hls_input(in, hls_in);
    gaussian_blur(hls_in, hls_out);
    read_hls_output(hls_out, out_hls);

    int e3 = compare(out_hls, out_ref);
    std::cout << "Test 3: " << (e3 == 0 ? "PASS" : "FAIL") << " (" << e3 << " mismatches)\n";
    total_errors += e3;

    // ----------------------------------------------------------------
    // Final result — Vitis HLS checks the return value:
    //   0 → simulation passed
    //  >0 → simulation failed
    // ----------------------------------------------------------------
    std::cout << "\n=== Overall: " << (total_errors == 0 ? "PASS" : "FAIL")
              << " (" << total_errors << " total mismatches) ===\n";
    return total_errors;
}
