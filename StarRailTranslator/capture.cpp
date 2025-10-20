#include "capture.h"

cv::Mat captureGame() {

    //capturing full screen
    HDC hScreen = GetDC(NULL);
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HDC hDCMem = CreateCompatibleDC(hScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, width, height);
    SelectObject(hDCMem, hBitmap);

    BitBlt(hDCMem, 0, 0, width, height, hScreen, 0, 0, SRCCOPY);

    cv::Mat img(height, width, CV_8UC4);
    GetBitmapBits(hBitmap, height * width * 4, img.data);

    DeleteObject(hBitmap);
    DeleteDC(hDCMem);
    ReleaseDC(NULL, hScreen);

    cv::cvtColor(img, img, cv::COLOR_BGRA2BGR);

    // TODO based on screen coordinates, to change for different screen sizes
    // Cropping dialogue section
    cv::Rect area(5, 1140, 2535, 190);
    area = area & cv::Rect(0, 0, img.cols, img.rows);
    cv::Mat cropped = img(area);


    return cropped;
}

