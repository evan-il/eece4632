#include <iostream>
#include <cstdint>
#include <cmath>
#include <cstring>
#include "edge_detection_hls.h"

// -----------------------------------------------------------------------
// Testbench for edge_detect() — AXI Stream version
//
// Four test patterns are checked against a software reference (centred Sobel):
//   1. Uniform black   — no edges anywhere, all output must be 0
//   2. Vertical edge   — step 0->255 at midcolumn, strong Gx response
//   3. Horizontal edge — step 0->255 at midrow,    strong Gy response
//   4. Checkerboard    — high-frequency pattern, edges everywhere
//
// Causal-delay accounting:
//   The 3x3 Sobel window is causal: output pixel at stream position (r, c)
//   represents the edge centred at input position (r-1, c-1).
//   Comparison therefore checks hls_out[r+1][c+1] against ref_out[r][c]
//   for r in [1, TB_HEIGHT-2], c in [1, TB_WIDTH-2].
//   HLS positions r < 2 || c < 2 are always 0 (border suppression).
// -----------------------------------------------------------------------

#define TB_HEIGHT  32
#define TB_WIDTH   32
#define THRESHOLD  30       // tune to match application; lower after Gaussian pre-blur

// -----------------------------------------------------------------------
// Software reference: centred 3x3 Sobel with clamp-to-edge borders
// Matches the HLS kernel exactly (L1 magnitude, same threshold comparison).
// -----------------------------------------------------------------------
static void ref_edge_detect(
    uint8_t input [TB_HEIGHT][TB_WIDTH],
    uint8_t output[TB_HEIGHT][TB_WIDTH],
    uint8_t threshold)
{
    for (int r = 0; r < TB_HEIGHT; r++) {
        for (int c = 0; c < TB_WIDTH; c++) {
            // Clamp indices to image bounds (matches HLS border replication)
            auto cr = [](int i, int max) { return i < 0 ? 0 : i >= max ? max-1 : i; };

            int p00 = input[cr(r-1,TB_HEIGHT)][cr(c-1,TB_WIDTH)];
            int p01 = input[cr(r-1,TB_HEIGHT)][cr(c  ,TB_WIDTH)];
            int p02 = input[cr(r-1,TB_HEIGHT)][cr(c+1,TB_WIDTH)];
            int p10 = input[cr(r  ,TB_HEIGHT)][cr(c-1,TB_WIDTH)];
            int p12 = input[cr(r  ,TB_HEIGHT)][cr(c+1,TB_WIDTH)];
            int p20 = input[cr(r+1,TB_HEIGHT)][cr(c-1,TB_WIDTH)];
            int p21 = input[cr(r+1,TB_HEIGHT)][cr(c  ,TB_WIDTH)];
            int p22 = input[cr(r+1,TB_HEIGHT)][cr(c+1,TB_WIDTH)];

            int Gx = -p00 + p02 - 2*p10 + 2*p12 - p20 + p22;
            int Gy =  p00 + 2*p01 + p02 - p20 - 2*p21 - p22;
            int mag = abs(Gx) + abs(Gy);

            output[r][c] = (mag >= (int)threshold) ? 255 : 0;
        }
    }
}

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------
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

// Print binary edge map using '#' for edge pixels, '.' for background.
static void print_binary(const char* label, uint8_t img[TB_HEIGHT][TB_WIDTH])
{
    std::cout << "\n--- " << label << " ---\n";
    for (int r = 0; r < TB_HEIGHT; r++) {
        for (int c = 0; c < TB_WIDTH; c++)
            std::cout << (img[r][c] ? '#' : '.');
        std::cout << "\n";
    }
}

// Compare HLS output against software reference, accounting for (1,1) causal offset:
//   hls_out[r+1][c+1]  <-->  ref_out[r][c]
// Checks the interior region where both are fully defined:
//   r in [1, TB_HEIGHT-2],  c in [1, TB_WIDTH-2]
static int compare(uint8_t hls_out[TB_HEIGHT][TB_WIDTH], uint8_t ref_out[TB_HEIGHT][TB_WIDTH])
{
    int errors = 0;
    for (int r = 1; r < TB_HEIGHT - 1; r++) {
        for (int c = 1; c < TB_WIDTH - 1; c++) {
            uint8_t hls_val = hls_out[r + 1][c + 1];
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
    // Test 1: Uniform black — no edges anywhere
    // All Gx and Gy are 0; every output pixel must be 0.
    // ----------------------------------------------------------------
    std::cout << "=== Test 1: Uniform black ===\n";
    memset(in, 0, sizeof(in));

    ref_edge_detect(in, out_ref, THRESHOLD);
    fill_stream(src, in);
    edge_detect(src, dst, TB_HEIGHT, TB_WIDTH, THRESHOLD);
    drain_stream(dst, out_hls);

    int e1 = compare(out_hls, out_ref);
    std::cout << "Test 1: " << (e1 == 0 ? "PASS" : "FAIL")
              << " (" << e1 << " mismatches)\n";
    total_errors += e1;

    // ----------------------------------------------------------------
    // Test 2: Vertical step edge — left half 0, right half 255
    // Gx is large at the step column; Gy is 0 everywhere.
    // Expect a single vertical line of edge pixels at midcolumn.
    // ----------------------------------------------------------------
    std::cout << "\n=== Test 2: Vertical step edge ===\n";
    for (int r = 0; r < TB_HEIGHT; r++)
        for (int c = 0; c < TB_WIDTH; c++)
            in[r][c] = (c >= TB_WIDTH / 2) ? 255 : 0;

    ref_edge_detect(in, out_ref, THRESHOLD);
    fill_stream(src, in);
    edge_detect(src, dst, TB_HEIGHT, TB_WIDTH, THRESHOLD);
    drain_stream(dst, out_hls);

    print_binary("HLS output", out_hls);
    print_binary("Reference",  out_ref);

    int e2 = compare(out_hls, out_ref);
    std::cout << "Test 2: " << (e2 == 0 ? "PASS" : "FAIL")
              << " (" << e2 << " mismatches)\n";
    total_errors += e2;

    // ----------------------------------------------------------------
    // Test 3: Horizontal step edge — top half 0, bottom half 255
    // Gy is large at the step row; Gx is 0 everywhere.
    // Expect a single horizontal line of edge pixels at midrow.
    // ----------------------------------------------------------------
    std::cout << "\n=== Test 3: Horizontal step edge ===\n";
    for (int r = 0; r < TB_HEIGHT; r++)
        for (int c = 0; c < TB_WIDTH; c++)
            in[r][c] = (r >= TB_HEIGHT / 2) ? 255 : 0;

    ref_edge_detect(in, out_ref, THRESHOLD);
    fill_stream(src, in);
    edge_detect(src, dst, TB_HEIGHT, TB_WIDTH, THRESHOLD);
    drain_stream(dst, out_hls);

    print_binary("HLS output", out_hls);
    print_binary("Reference",  out_ref);

    int e3 = compare(out_hls, out_ref);
    std::cout << "Test 3: " << (e3 == 0 ? "PASS" : "FAIL")
              << " (" << e3 << " mismatches)\n";
    total_errors += e3;

    // ----------------------------------------------------------------
    // Test 4: Checkerboard — high-frequency pattern
    // Every pixel neighbours a pixel of opposite polarity.
    // Almost every interior pixel should be detected as an edge.
    // ----------------------------------------------------------------
    std::cout << "\n=== Test 4: Checkerboard ===\n";
    for (int r = 0; r < TB_HEIGHT; r++)
        for (int c = 0; c < TB_WIDTH; c++)
            in[r][c] = ((r + c) % 2 == 0) ? 255 : 0;

    ref_edge_detect(in, out_ref, THRESHOLD);
    fill_stream(src, in);
    edge_detect(src, dst, TB_HEIGHT, TB_WIDTH, THRESHOLD);
    drain_stream(dst, out_hls);

    int e4 = compare(out_hls, out_ref);
    std::cout << "Test 4: " << (e4 == 0 ? "PASS" : "FAIL")
              << " (" << e4 << " mismatches)\n";
    total_errors += e4;

    // ----------------------------------------------------------------
    // Vitis HLS interprets return value: 0 = PASS, non-zero = FAIL
    // ----------------------------------------------------------------
    std::cout << "\n=== Overall: " << (total_errors == 0 ? "PASS" : "FAIL")
              << " (" << total_errors << " total mismatches) ===\n";
    return total_errors;
}
