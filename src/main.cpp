// main.cpp — LibRaw decode, OpenCV process (16-bit), write TIFF before/after
#include <libraw/libraw.h>
#include <opencv2/opencv.hpp>

int main() {
    LibRaw p;

    // Open & unpack RAW
    if (p.open_file("test.dng") != LIBRAW_SUCCESS) return 1;
    if (p.unpack() != LIBRAW_SUCCESS) return 2;

    // Produce processed 16-bit RGB in p.imgdata.image (no file write yet)
    p.imgdata.params.output_bps  = 16;
    p.imgdata.params.output_tiff = 0;
    if (p.dcraw_process() != LIBRAW_SUCCESS) return 3;

    // Wrap LibRaw buffer (ushort (*image)[4]) as OpenCV Mat
    const int iw = p.imgdata.sizes.iwidth;
    const int ih = p.imgdata.sizes.iheight;
    if (!p.imgdata.image) return 4;

    p.imgdata.params.output_tiff = 1;
    p.dcraw_ppm_tiff_writer("before.tiff");

    // conversion

    cv::Mat rgba16(ih, iw, CV_16UC4, (void*)p.imgdata.image);
    cv::Mat rgb16;
    rgb16.create(ih, iw, CV_16UC3);

    // extract R,G,B (0,1,2) from RGBA
    int from_to[] = {0,0, 1,1, 2,2};
    cv::mixChannels(&rgba16, 1, &rgb16, 1, from_to, 3);

    // convert to BGR for OpenCV writer
    cv::Mat bgr16;
    cv::cvtColor(rgb16, bgr16, cv::COLOR_RGB2BGR);

    // --- OpenCV processing (example: 3x3 sharpening kernel on 16-bit data) ---
    cv::Mat kernel = (cv::Mat_<float>(3,3) <<
         1,  1,  1,
         1,  1,  1,
         1,  1,  1);
    kernel /= cv::sum(kernel)[0];

    // cv::Mat rgb16_after;
    cv::filter2D(rgba16, rgba16, -1, kernel, cv::Point(-1,-1), 0, cv::BORDER_REPLICATE);

    // Write processed 16-bit TIFF
    p.imgdata.params.output_tiff = 1;
    p.dcraw_ppm_tiff_writer("after.tiff");

    return 0;
}