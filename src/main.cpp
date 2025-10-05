// main.cpp — write 16-bit TIFF before and after your own manipulation
#include <libraw/libraw.h>
#include <cstdint>
#include <algorithm>

static inline unsigned short clamp16(uint32_t v) {
    return (v > 65535u) ? 65535u : static_cast<unsigned short>(v);
}

int main() {
    LibRaw p;

    // Open & unpack once
    if (p.open_file("test.dng") != LIBRAW_SUCCESS) return 1;
    if (p.unpack() != LIBRAW_SUCCESS) return 2;

    // Produce processed 16-bit RGB in p.imgdata.image
    p.imgdata.params.output_bps  = 16;
    p.imgdata.params.output_tiff = 0;     // we'll write after edits
    if (p.dcraw_process() != LIBRAW_SUCCESS) return 3;

    // 1) Write baseline (unmodified) 16-bit TIFF
    p.imgdata.params.output_tiff = 1;
    if (p.dcraw_ppm_tiff_writer("test_before.tiff") != LIBRAW_SUCCESS) return 4;

    // 2) Manipulate the in-memory processed buffer
    auto iw  = p.imgdata.sizes.iwidth;
    auto ih  = p.imgdata.sizes.iheight;
    auto *px = p.imgdata.image;           // ushort (*image)[4], channels: R,G,B,(A)
    if (!px) return 5;

    // Example: simple gain + clamp on RGB
    const float gain = 1.2f;              // tweak as needed
    for (uint32_t y = 0; y < ih; ++y) {
        for (uint32_t x = 0; x < iw; ++x) {
            uint32_t i = y * iw + x;
            for (int c = 0; c < 3; ++c) {
                uint32_t v = static_cast<uint32_t>(px[i][c] * gain + 0.5f);
                px[i][c] = clamp16(v);
            }
        }
    }

    // 3) Write edited 16-bit TIFF
    // (buffer is already modified; just call the writer again)
    if (p.dcraw_ppm_tiff_writer("test_after.tiff") != LIBRAW_SUCCESS) return 6;

    return 0;
}