// main.cpp — LibRaw decode, OpenCV process (16-bit), write TIFF before/after (red-biased halation)
#include <libraw/libraw.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
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

    // Reference write (before)
    p.imgdata.params.output_tiff = 1;
    p.dcraw_ppm_tiff_writer("before.tiff");

    // ---- Convert LibRaw RGBA16 -> RGB16 -> float ----
    cv::Mat rgba16(ih, iw, CV_16UC4, (void*)p.imgdata.image);
    cv::Mat rgb16(ih, iw, CV_16UC3);
    int from_to[] = {0,0, 1,1, 2,2}; // R,G,B from RGBA
    cv::mixChannels(&rgba16, 1, &rgb16, 1, from_to, 3);

    std::cout << "created rgb16\n";

    cv::Mat img_f, blurred, bloom;
    rgb16.convertTo(img_f, CV_32F, 1.0/65535.0);
    std::cout << "converted to img_f\n";

    // ---- Blur (spread for halation) ----
    cv::GaussianBlur(img_f, blurred, cv::Size(0,0), 20.0);
    std::cout << "make blurred\n";

    // ---- Intensity mask (limit effect to highlights) ----
    cv::Mat gray;
    cv::cvtColor(img_f, gray, cv::COLOR_RGB2GRAY);
    std::cout << "make gray\n";

    cv::Mat mask;
    cv::normalize(gray, mask, 0, 1, cv::NORM_MINMAX);
    cv::pow(mask, 1.4, mask); // emphasize highlights
    std::cout << "do brightness\n";

    // ---- Combine (with RED bias) ----
    CV_Assert(img_f.size() == blurred.size());
    CV_Assert(img_f.type() == blurred.type());
    CV_Assert(mask.type() == CV_32FC1);

    cv::Mat scaledMask;
    cv::multiply(mask, cv::Scalar::all(2.5f), scaledMask); // global bloom strength

    // Broadcast mask to 3 channels
    cv::Mat mask3;
    cv::merge(std::vector<cv::Mat>{scaledMask, scaledMask, scaledMask}, mask3);

    // Halo = blurred * mask
    cv::Mat halo = blurred.mul(mask3);

    // RED bias: boost red channel of the halo (R=channel 2 in RGB order here)
    {
        std::vector<cv::Mat> hch;
        cv::split(halo, hch);
        hch[0] *= 1.5f;             // increase to taste (e.g., 1.2–2.0)
        // optional gentle cross-talk for warmer glow:
        // hch[1] += 0.15f * hch[2]; // G pickup
        // hch[0] += 0.08f * hch[2]; // B pickup
        cv::merge(hch, halo);
    }

    bloom = img_f + halo;
    std::cout << "recombined (red-biased)\n";

    // ---- Write back into LibRaw RGBA16 buffer ----
    cv::Mat tmp16;
    bloom.convertTo(tmp16, CV_16U, 65535.0);

    auto *dst = p.imgdata.image; // ushort[4] per pixel (R,G, B, A)
    std::vector<cv::Mat> ch;
    cv::split(tmp16, ch);

    for (int y = 0; y < ih; ++y) {
        for (int x = 0; x < iw; ++x) {
            int i = y * iw + x;
            dst[i][0] = ch[0].at<uint16_t>(y, x); // R
            dst[i][1] = ch[1].at<uint16_t>(y, x); // G
            dst[i][2] = ch[2].at<uint16_t>(y, x); // B
            dst[i][3] = 65535;                    // A
        }
    }

    // Final write (after)
    p.imgdata.params.output_tiff = 1;
    p.dcraw_ppm_tiff_writer("after.tiff");

    return 0;
}