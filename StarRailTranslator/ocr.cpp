#include "ocr.h"

//TODO
//--YELLOWTEXT NOT PROPERLY REED

std::string performOCR(const cv::Mat& image, tesseract::TessBaseAPI* tessApi) {
    cv::Mat processedImage = preprocessImage(image);
    tessApi->SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
    tessApi->SetImage(processedImage.data, processedImage.cols, processedImage.rows, 1, processedImage.step);
    char* outText = tessApi->GetUTF8Text();
    std::string result(outText);

    //string cleanup --> To refine, based on generated artifacts
    auto first_letter_it = std::find_if(result.begin(), result.end(),
        [](unsigned char c) {
            return std::isalpha(c);
        });

    
    if (first_letter_it != result.end()) {
        result.erase(result.begin(), first_letter_it);
    }
    else {
        result.clear();
    }

    delete[] outText;
    return result;
}

cv::Mat preprocessImage(const cv::Mat& image) {
    //TODO
    // -- if not working trying processing separetly CharacterName and Dialogue
    // -- 
    cv::Mat gray, thresholded;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::threshold(gray, thresholded, 180, 255, cv::THRESH_BINARY_INV);
    //cv::imshow("processed image", thresholded);
    //cv::waitKey(100);

    return thresholded;
}

tesseract::TessBaseAPI* initTesseract() {
    tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();
    if (api->Init(NULL, "eng")) {
        delete api;
        std::cout <<  "OCR init failed";
        return nullptr;
    }
    return api;
}