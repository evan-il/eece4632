#include <iostream>
#include <cstdint>
#include <cmath>
#include <cstring>
#include "blur_edge_combined.h"

#define TB_HEIGHT  32
#define TB_WIDTH   32
#define THRESHOLD  500   // 5x5 Gaussian-Sobel max |Gx|=12240, max mag=24480; tune as needed

// Software reference: 5x5 Gaussian-Sobel with clamp-to-edge borders.
// Matches the HLS kernel exactly (same SOBEL coefficients, L1 magnitude, threshold).
static const int SOBEL_REF[KERNEL_SIZE][KERNEL_SIZE] = {
    {-1,  -2, 0,  2, 1},
    {-4,  -8, 0,  8, 4},
    {-6, -12, 0, 12, 6},
    {-4,  -8, 0,  8, 4},
    {-1,  -2, 0,  2, 1}
};

static void ref_blur_edge(
    uint8_t input [TB_HEIGHT][TB_WIDTH],
    uint8_t output[TB_HEIGHT][TB_WIDTH],
    int threshold)
{
    for (int r = 0; r < TB_HEIGHT; r++) {
        for (int c = 0; c < TB_WIDTH; c++) {
            auto clamp_r = [](int i, int max) { return i < 0 ? 0 : i >= max ? max-1 : i; };
            auto clamp_c = [](int i, int max) { return i < 0 ? 0 : i >= max ? max-1 : i; };

            int Gx = 0, Gy = 0;
            for (int kr = 0; kr < KERNEL_SIZE; kr++) {
                for (int kc = 0; kc < KERNEL_SIZE; kc++) {
                    int ri = clamp_r(r - KERNEL_HALF + kr, TB_HEIGHT);
                    int ci = clamp_c(c - KERNEL_HALF + kc, TB_WIDTH);
                    int px = input[ri][ci];
                    Gx += SOBEL_REF[kr][kc] * px;
                    Gy += SOBEL_REF[kc][kr] * px;
                }
            }
            int mag = abs(Gx) + abs(Gy);
            output[r][c] = (mag >= threshold) ? 255 : 0;
        }
    }
}

static void fill_stream(hls::stream<axis_t> &src, uint8_t img[TB_HEIGHT][TB_WIDTH])
{
    for (int r = 0; r < TB_HEIGHT; r++) {
        for (int c = 0; c < TB_WIDTH; c++) {
            axis_t pix;
            pix.data = img[r][c];
            pix.keep = 0xFF;
            pix.strb = 0xFF;
            pix.last = (r == TB_HEIGHT-1 && c == TB_WIDTH-1) ? 1 : 0;
            src.write(pix);
        }
    }
}

static void drain_stream(hls::stream<axis_t> &dst, uint8_t out[TB_HEIGHT][TB_WIDTH])
{
    for (int r = 0; r < TB_HEIGHT; r++)
        for (int c = 0; c < TB_WIDTH; c++)
            out[r][c] = (uint8_t)dst.read().data;
}

static void print_binary(const char* label, uint8_t img[TB_HEIGHT][TB_WIDTH])
{
    std::cout << "\n--- " << label << " ---\n";
    for (int r = 0; r < TB_HEIGHT; r++) {
        for (int c = 0; c < TB_WIDTH; c++)
            std::cout << (img[r][c] ? '#' : '.');
        std::cout << "\n";
    }
}

// Compare interior region, accounting for KERNEL_HALF causal offset:
//   hls_out[r + KERNEL_HALF][c + KERNEL_HALF]  <-->  ref_out[r][c]
// Only checks r, c in [KERNEL_HALF, TB_HEIGHT/WIDTH - KERNEL_HALF - 1] to
// avoid border-handling differences between causal HLS and centered reference.
static int compare(uint8_t hls_out[TB_HEIGHT][TB_WIDTH], uint8_t ref_out[TB_HEIGHT][TB_WIDTH])
{
    int errors = 0;
    for (int r = KERNEL_HALF; r < TB_HEIGHT - KERNEL_HALF; r++) {
        for (int c = KERNEL_HALF; c < TB_WIDTH - KERNEL_HALF; c++) {
            uint8_t hls_val = hls_out[r + KERNEL_HALF][c + KERNEL_HALF];
            uint8_t ref_val = ref_out[r][c];
            if (hls_val != ref_val) {
                std::cout << "  MISMATCH at ref(" << r << "," << c << "): "
                          << "HLS=" << (int)hls_val
                          << "  REF=" << (int)ref_val << "\n";
                errors++;
            }
        }
    }
    return errors;
}

int main()
{
    static uint8_t in      [TB_HEIGHT][TB_WIDTH];
    static uint8_t out_hls [TB_HEIGHT][TB_WIDTH];
    static uint8_t out_ref [TB_HEIGHT][TB_WIDTH];

    hls::stream<axis_t> src("src");
    hls::stream<axis_t> dst("dst");

    int total_errors = 0;

    // Test 1: Uniform black — no edges anywhere
    std::cout << "=== Test 1: Uniform black ===\n";
    memset(in, 0, sizeof(in));

    ref_blur_edge(in, out_ref, THRESHOLD);
    fill_stream(src, in);
    blur_edge(src, dst, TB_HEIGHT, TB_WIDTH, THRESHOLD);
    drain_stream(dst, out_hls);

    int e1 = compare(out_hls, out_ref);
    std::cout << "Test 1: " << (e1 == 0 ? "PASS" : "FAIL")
              << " (" << e1 << " mismatches)\n";
    total_errors += e1;

    // Test 2: Vertical step edge — left half 0, right half 255
    std::cout << "\n=== Test 2: Vertical step edge ===\n";
    for (int r = 0; r < TB_HEIGHT; r++)
        for (int c = 0; c < TB_WIDTH; c++)
            in[r][c] = (c >= TB_WIDTH / 2) ? 255 : 0;

    ref_blur_edge(in, out_ref, THRESHOLD);
    fill_stream(src, in);
    blur_edge(src, dst, TB_HEIGHT, TB_WIDTH, THRESHOLD);
    drain_stream(dst, out_hls);

    print_binary("HLS output", out_hls);
    print_binary("Reference",  out_ref);

    int e2 = compare(out_hls, out_ref);
    std::cout << "Test 2: " << (e2 == 0 ? "PASS" : "FAIL")
              << " (" << e2 << " mismatches)\n";
    total_errors += e2;

    // Test 3: Horizontal step edge — top half 0, bottom half 255
    std::cout << "\n=== Test 3: Horizontal step edge ===\n";
    for (int r = 0; r < TB_HEIGHT; r++)
        for (int c = 0; c < TB_WIDTH; c++)
            in[r][c] = (r >= TB_HEIGHT / 2) ? 255 : 0;

    ref_blur_edge(in, out_ref, THRESHOLD);
    fill_stream(src, in);
    blur_edge(src, dst, TB_HEIGHT, TB_WIDTH, THRESHOLD);
    drain_stream(dst, out_hls);

    print_binary("HLS output", out_hls);
    print_binary("Reference",  out_ref);

    int e3 = compare(out_hls, out_ref);
    std::cout << "Test 3: " << (e3 == 0 ? "PASS" : "FAIL")
              << " (" << e3 << " mismatches)\n";
    total_errors += e3;

    // Test 4: Checkerboard — high-frequency pattern; edges everywhere in interior
    std::cout << "\n=== Test 4: Checkerboard ===\n";
    for (int r = 0; r < TB_HEIGHT; r++)
        for (int c = 0; c < TB_WIDTH; c++)
            in[r][c] = ((r + c) % 2 == 0) ? 255 : 0;

    ref_blur_edge(in, out_ref, THRESHOLD);
    fill_stream(src, in);
    blur_edge(src, dst, TB_HEIGHT, TB_WIDTH, THRESHOLD);
    drain_stream(dst, out_hls);

    int e4 = compare(out_hls, out_ref);
    std::cout << "Test 4: " << (e4 == 0 ? "PASS" : "FAIL")
              << " (" << e4 << " mismatches)\n";
    total_errors += e4;

    std::cout << "\n=== Overall: " << (total_errors == 0 ? "PASS" : "FAIL")
              << " (" << total_errors << " total mismatches) ===\n";
    return total_errors;
}
