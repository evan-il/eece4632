#include "edge_detection_hls.h"

void edge_detect(
    hls::stream<axis_t> &src,
    hls::stream<axis_t> &dst,
    int height,
    int width,
    pixel_t threshold
)
{
    #pragma HLS INTERFACE axis      port=src
    #pragma HLS INTERFACE axis      port=dst
    #pragma HLS INTERFACE s_axilite port=height
    #pragma HLS INTERFACE s_axilite port=width
    #pragma HLS INTERFACE s_axilite port=threshold
    #pragma HLS INTERFACE s_axilite port=return

    // Three-row circular line buffer — one bank per row for parallel access.
    static pixel_t line_buf[3][IMG_WIDTH];
    #pragma HLS ARRAY_PARTITION variable=line_buf complete dim=1

    // 3x3 sliding window — fully in registers.
    pixel_t win[3][3];
    #pragma HLS ARRAY_PARTITION variable=win complete dim=0

    row: for (int r = 0; r < height; r++) {
        #pragma HLS LOOP_TRIPCOUNT min=1 max=IMG_HEIGHT
        col: for (int c = 0; c < width; c++) {
            #pragma HLS LOOP_TRIPCOUNT min=1 max=IMG_WIDTH
            #pragma HLS PIPELINE II=1

            // ---- 1. Read one pixel from the input stream ----
            axis_t pix_in = src.read();
            pixel_t px = pix_in.data;

            // ---- 2. Shift line buffer up, write new row at bottom ----
            for (int i = 0; i < 2; i++) {
                #pragma HLS UNROLL
                line_buf[i][c] = line_buf[i + 1][c];
            }
            line_buf[2][c] = px;

            // ---- 3. Shift window left, load new rightmost column ----
            for (int i = 0; i < 3; i++) {
                #pragma HLS UNROLL
                win[i][0] = win[i][1];
                win[i][1] = win[i][2];
                win[i][2] = line_buf[i][c];
            }

            // ---- 4. Sobel gradients (3x3 kernel) ----
            accum_t Gx = (accum_t)(-1)*win[0][0] + (accum_t)(+1)*win[0][2]
                       + (accum_t)(-2)*win[1][0] + (accum_t)(+2)*win[1][2]
                       + (accum_t)(-1)*win[2][0] + (accum_t)(+1)*win[2][2];

            accum_t Gy = (accum_t)(+1)*win[0][0] + (accum_t)(+2)*win[0][1] + (accum_t)(+1)*win[0][2]
                       + (accum_t)(-1)*win[2][0] + (accum_t)(-2)*win[2][1] + (accum_t)(-1)*win[2][2];

            // ---- 5. L1 magnitude approximation (no sqrt needed) ----
            // Cast resolves ap_int width ambiguity: negation promotes ap_int<16>
            // to ap_int<17>, making the ternary branches incompatible without it.
            accum_t mag = (accum_t)(Gx < 0 ? (ap_int<17>)(-Gx) : (ap_int<17>)(Gx)) + ((Gy < 0) ? (ap_int<17>)(-Gy) : (ap_int<17>)(Gy));

            // ---- 6. Threshold; suppress the 2-pixel border ----
            pixel_t result;
            if (r < 2 || c < 2) {
                result = 0;
            } else {
                result = (mag >= (accum_t)threshold) ? (pixel_t)255 : (pixel_t)0;
            }
//hi
            // ---- 7. Write output pixel, preserving stream sideband signals ----
            axis_t pix_out;
            pix_out.data = result;
            pix_out.keep = pix_in.keep;
            pix_out.strb = pix_in.strb;
            pix_out.last = pix_in.last;
            dst.write(pix_out);
        }
    }
}
