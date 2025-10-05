#include <libraw/libraw.h>

int main() {
    LibRaw processor;
    if (processor.open_file("test.dng") != LIBRAW_SUCCESS) return 1;
    if (processor.unpack() != LIBRAW_SUCCESS) return 1;
    if (processor.dcraw_process() != LIBRAW_SUCCESS) return 1;
    libraw_processed_image_t* img = processor.dcraw_make_mem_image();
    if (!img) return 1;
    processor.dcraw_ppm_tiff_writer("test.jpeg");
    LibRaw::dcraw_clear_mem(img);
    return 0;
}
