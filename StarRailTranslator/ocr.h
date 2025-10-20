#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

std::string performOCR(const cv::Mat& image, tesseract::TessBaseAPI* tessApi);

cv::Mat preprocessImage(const cv::Mat& image);

tesseract::TessBaseAPI* initTesseract();