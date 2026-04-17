#include <iostream>
#include <cstdint>
#include <cmath>
#include <cstring>
#include "gaussian_blur_hls.h"

// -----------------------------------------------------------------------
// Testbench for gaussian_blur() — AXI Stream version
//
// Uses a small image so C simulation runs quickly.
// Three test patterns are checked against a software reference:
//   1. Impulse    — single white pixel in the centre
//   2. Gradient   — horizontal ramp 0-255; blur should smooth it slightly
//   3. Checkerboard — high-frequency pattern; blur should reduce contrast
//
// Output alignment note:
//   The streaming filter is causal, so output pixel (r, c) in the output
//   stream corresponds to the blur *centered* at input position
//   (r - KERNEL_HALF, c - KERNEL_HALF).  The comparison therefore checks
//   hls_out[r + KERNEL_HALF][c + KERNEL_HALF] against ref_out[r][c] for
//   the interior region where both are fully defined:
//     r  in [0,          TB_HEIGHT - 1 - KERNEL_HALF]
//     c  in [0,          TB_WIDTH  - 1 - KERNEL_HALF]
//
// Pass criterion: every compared pixel is within ±1 of the reference
// (allows for integer rounding differences between HW and SW paths).
// -----------------------------------------------------------------------

#define TB_HEIGHT 16
#define TB_WIDTH  16

#define MAX_ERROR 1

// -----------------------------------------------------------------------
// Software reference: centred 5x5 Gaussian with clamp-to-edge borders
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
            output[r][c] = (uint8_t)(sum / KERNEL_SUM + 0.5f);
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

// Push a TB_HEIGHT x TB_WIDTH image into an AXI Stream, one pixel per beat.
static void fill_stream(
    hls::stream<axis_t> &src,
    uint8_t img[TB_HEIGHT][TB_WIDTH])
{
    for (int r = 0; r < TB_HEIGHT; r++) {
        for (int c = 0; c < TB_WIDTH; c++) {
            axis_t pix;
            pix.data = img[r][c];
            pix.keep = 0xFF;
            pix.strb = 0xFF;
            pix.last = (r == TB_HEIGHT - 1 && c == TB_WIDTH - 1) ? 1 : 0;
            src.write(pix);
        }
    }
}

// Drain a TB_HEIGHT x TB_WIDTH output stream into a plain array.
static void drain_stream(
    hls::stream<axis_t> &dst,
    uint8_t out[TB_HEIGHT][TB_WIDTH])
{
    for (int r = 0; r < TB_HEIGHT; r++)
        for (int c = 0; c < TB_WIDTH; c++)
            out[r][c] = (uint8_t)dst.read().data;
}

// Compare HLS output against software reference.
// Accounts for the KERNEL_HALF-row / KERNEL_HALF-column causal delay:
//   hls_out[r + KERNEL_HALF][c + KERNEL_HALF] should match ref_out[r][c].
// Only the interior region where both values are fully defined is checked.
static int compare(
    uint8_t hls_out[TB_HEIGHT][TB_WIDTH],
    uint8_t ref_out[TB_HEIGHT][TB_WIDTH])
{
    int errors = 0;
    for (int r = 0; r < TB_HEIGHT - KERNEL_HALF; r++) {
        for (int c = 0; c < TB_WIDTH - KERNEL_HALF; c++) {
            int diff = abs((int)hls_out[r + KERNEL_HALF][c + KERNEL_HALF]
                         - (int)ref_out[r][c]);
            if (diff > MAX_ERROR) {
                std::cout << "MISMATCH at ref(" << r << "," << c << "): "
                          << "HLS=" << (int)hls_out[r + KERNEL_HALF][c + KERNEL_HALF]
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
    static uint8_t in      [TB_HEIGHT][TB_WIDTH];
    static uint8_t out_hls [TB_HEIGHT][TB_WIDTH];
    static uint8_t out_ref [TB_HEIGHT][TB_WIDTH];

    hls::stream<axis_t> src("src");
    hls::stream<axis_t> dst("dst");

    int total_errors = 0;

    // ----------------------------------------------------------------
    // Test 1: Impulse
    // Single white pixel at centre — output should look like the kernel
    // ----------------------------------------------------------------
    std::cout << "=== Test 1: Impulse ===\n";
    memset(in, 0, sizeof(in));
    in[TB_HEIGHT / 2][TB_WIDTH / 2] = 255;

    ref_gaussian_blur(in, out_ref);
    fill_stream(src, in);
    gaussian_blur(src, dst, TB_HEIGHT, TB_WIDTH);
    drain_stream(dst, out_hls);

    print_image("Input",      in);
    print_image("HLS output", out_hls);
    print_image("Reference",  out_ref);

    int e1 = compare(out_hls, out_ref);
    std::cout << "Test 1: " << (e1 == 0 ? "PASS" : "FAIL")
              << " (" << e1 << " mismatches)\n";
    total_errors += e1;

    // ----------------------------------------------------------------
    // Test 2: Horizontal gradient (ramp 0 → 255 across columns)
    // ----------------------------------------------------------------
    std::cout << "\n=== Test 2: Gradient ===\n";
    for (int r = 0; r < TB_HEIGHT; r++)
        for (int c = 0; c < TB_WIDTH; c++)
            in[r][c] = (uint8_t)((c * 255) / (TB_WIDTH - 1));

    ref_gaussian_blur(in, out_ref);
    fill_stream(src, in);
    gaussian_blur(src, dst, TB_HEIGHT, TB_WIDTH);
    drain_stream(dst, out_hls);

    int e2 = compare(out_hls, out_ref);
    std::cout << "Test 2: " << (e2 == 0 ? "PASS" : "FAIL")
              << " (" << e2 << " mismatches)\n";
    total_errors += e2;

    // ----------------------------------------------------------------
    // Test 3: Checkerboard (high-frequency pattern)
    // Blur should reduce contrast — no interior pixel should stay at 255 or 0
    // ----------------------------------------------------------------
    std::cout << "\n=== Test 3: Checkerboard ===\n";
    for (int r = 0; r < TB_HEIGHT; r++)
        for (int c = 0; c < TB_WIDTH; c++)
            in[r][c] = ((r + c) % 2 == 0) ? 255 : 0;

    ref_gaussian_blur(in, out_ref);
    fill_stream(src, in);
    gaussian_blur(src, dst, TB_HEIGHT, TB_WIDTH);
    drain_stream(dst, out_hls);

    int e3 = compare(out_hls, out_ref);
    std::cout << "Test 3: " << (e3 == 0 ? "PASS" : "FAIL")
              << " (" << e3 << " mismatches)\n";
    total_errors += e3;

    // ----------------------------------------------------------------
    // Vitis HLS checks the return value: 0 = pass, >0 = fail
    // ----------------------------------------------------------------
    std::cout << "\n=== Overall: " << (total_errors == 0 ? "PASS" : "FAIL")
              << " (" << total_errors << " total mismatches) ===\n";
    return total_errors;
}
