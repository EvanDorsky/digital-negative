// main.cpp — LibRaw decode, OpenCV process (16-bit), write TIFF before/after
#include <libraw/libraw.h>
#include <opencv2/opencv.hpp>
#include <cstdio>

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

    std::cout << "created rgb16\n";

    // img16: 16-bit RGB from LibRaw (CV_16UC3)
    cv::Mat img_f, blurred, bloom, result;

    // Convert to float for scaling
    rgb16.convertTo(img_f, CV_32F, 1.0/65535.0);

    std::cout << "converted to img_f\n";

    // Generate blurred version
    cv::GaussianBlur(img_f, blurred, cv::Size(0,0), 20.0); // sigma controls bloom radius
    
    std::cout << "make blurred\n";

    // Compute intensity mask
    cv::Mat gray;
    cv::cvtColor(img_f, gray, cv::COLOR_RGB2GRAY);

    std::cout << "make gray\n";

    // Strength proportional to brightness
    cv::Mat mask;
    cv::normalize(gray, mask, 0, 1, cv::NORM_MINMAX);
    cv::pow(mask, 2.0, mask); // emphasize highlights

    std::cout << "do brightness\n";

    // Combine original + bloom
    // bloom = img_f + blurred.mul(mask * 0.6f);
    // bloom = img_f;

    CV_Assert(img_f.size() == blurred.size());
    CV_Assert(img_f.type() == blurred.type());
    CV_Assert(mask.type() == CV_32FC1);

    cv::Mat scaledMask;
    cv::multiply(mask, cv::Scalar::all(0.8f), scaledMask); // elementwise scale

    // Expand mask to 3 channels
    cv::Mat mask3;
    cv::merge(std::vector<cv::Mat>{scaledMask, scaledMask, scaledMask}, mask3);

    // Multiply and add
    cv::Mat halo = blurred.mul(mask3);
    bloom = img_f + halo;

    // cv::normalize(bloom, bloom, 0, 1, cv::NORM_MINMAX);

    std::cout << "recombined and normalized\n";

    cv::Mat tmp16;
    bloom.convertTo(tmp16, CV_16U, 65535.0);
    // write directly into LibRaw’s RGBA buffer
    auto *dst = p.imgdata.image; // ushort[4] per pixel
    std::vector<cv::Mat> ch;
    cv::split(tmp16, ch);

    for (int y = 0; y < ih; ++y) {
        for (int x = 0; x < iw; ++x) {
            int i = y * iw + x;
            dst[i][0] = ch[0].at<uint16_t>(y, x);
            dst[i][1] = ch[1].at<uint16_t>(y, x);
            dst[i][2] = ch[2].at<uint16_t>(y, x);
            dst[i][3] = 65535; // opaque alpha
        }
    }

    // Back to 16-bit
    // bloom.convertTo(result, CV_16U, 65535.0);
    // cv::imwrite("halation.tiff", result);

    // Write processed 16-bit TIFF
    p.imgdata.params.output_tiff = 1;
    p.dcraw_ppm_tiff_writer("after.tiff");

    return 0;
}