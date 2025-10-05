// main.cpp
#include <libraw/libraw.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h" // provide via Conan (stb/cci.20240531) or vendor the header

int main() {
    LibRaw p;

    // Open & unpack once
    if (p.open_file("test.dng") != LIBRAW_SUCCESS) return 1;
    if (p.unpack() != LIBRAW_SUCCESS) return 2;

    // ---- Pass 1: 16-bit TIFF via LibRaw writer ----
    p.imgdata.params.output_bps  = 16; // 16-bit
    p.imgdata.params.output_tiff = 1;  // write TIFF
    if (p.dcraw_process() != LIBRAW_SUCCESS) return 3;
    if (p.dcraw_ppm_tiff_writer("test_16bit.tiff") != LIBRAW_SUCCESS) return 4;

    // ---- Pass 2: 8-bit JPEG via stb_image_write (reuse same LibRaw object) ----
    p.imgdata.params.output_bps  = 8;  // 8-bit for JPEG
    p.imgdata.params.output_tiff = 0;  // request bitmap buffer
    if (p.dcraw_process() != LIBRAW_SUCCESS) return 5;

    libraw_processed_image_t* img = p.dcraw_make_mem_image();
    if (!img) return 6;
    if (img->type != LIBRAW_IMAGE_BITMAP || img->bits != 8 || img->colors < 3) {
        LibRaw::dcraw_clear_mem(img);
        return 7;
    }

    const int quality = 90;
    int ok = stbi_write_jpg("test.jpeg", img->width, img->height, img->colors, img->data, quality);
    LibRaw::dcraw_clear_mem(img);
    if (!ok) return 8;

    return 0;
}