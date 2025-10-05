#include <libraw/libraw.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"   // place stb_image_write.h in your include path

int main() {
    // 1) Write 16-bit TIFF via LibRaw
    {
        LibRaw p;
        if (p.open_file("test.dng") != LIBRAW_SUCCESS) return 1;
        if (p.unpack() != LIBRAW_SUCCESS) return 2;
        p.imgdata.params.output_bps = 16;   // 16-bit output
        p.imgdata.params.output_tiff = 1;   // write TIFF (not PPM)
        if (p.dcraw_process() != LIBRAW_SUCCESS) return 3;
        if (p.dcraw_ppm_tiff_writer("test_16bit.tiff") != LIBRAW_SUCCESS) return 4;
    }

    // 2) Write JPEG using processed 8-bit RGB buffer + stb_image_write
    {
        LibRaw p;
        if (p.open_file("test.dng") != LIBRAW_SUCCESS) return 5;
        if (p.unpack() != LIBRAW_SUCCESS) return 6;
        p.imgdata.params.output_bps = 8;    // 8-bit for JPEG
        p.imgdata.params.output_tiff = 0;   // bitmap buffer
        if (p.dcraw_process() != LIBRAW_SUCCESS) return 7;

        libraw_processed_image_t* img = p.dcraw_make_mem_image();
        if (!img) return 8;
        if (img->type != LIBRAW_IMAGE_BITMAP || img->bits != 8 || img->colors < 3) {
            LibRaw::dcraw_clear_mem(img);
            return 9;
        }
        int ok = stbi_write_jpg("test.jpeg", img->width, img->height, img->colors, img->data, 90);
        LibRaw::dcraw_clear_mem(img);
        if (!ok) return 10;
    }

    return 0;
}