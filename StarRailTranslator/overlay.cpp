#include "connection.h"
#include "ocr.h"
#include "overlay.h"
#include "capture.h"


HWND hGameWindow = NULL;
HWND hOverlay = NULL;
static tesseract::TessBaseAPI* g_tessApi = nullptr;
static SOCKET g_socketToPython = INVALID_SOCKET;
static std::string g_lastText = "";
static std::string g_lastTranslatedText = "";
static int g_stableCounter = 0;
static bool g_isOverlayVisible = true;

void InitOverlayLogic(HWND hOverlay, tesseract::TessBaseAPI* api, SOCKET sock) {
    g_tessApi = api;
    g_socketToPython = sock;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            int width = clientRect.right - clientRect.left;
            int height = clientRect.bottom - clientRect.top;

            //checking if window is visible
            if (width <= 0 || height <= 0) {
                return 0;
            }

            HDC hdcScreen = GetDC(NULL);
            HDC hdcWindow = GetDC(hwnd);
            HDC hdcMem = CreateCompatibleDC(hdcScreen);

            BITMAPINFO bmi;
            ZeroMemory(&bmi, sizeof(BITMAPINFO));
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = width;
            bmi.bmiHeader.biHeight = -height; // Top-down bitmap
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            void* pBitmapData;
            HBITMAP hBitmap = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBitmapData, NULL, 0);

            if (hBitmap == NULL) {
                DeleteDC(hdcMem);
                ReleaseDC(NULL, hdcScreen);
                ReleaseDC(hwnd, hdcWindow);
                return 0;
            }

            HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

            DWORD alpha = 0xAA;
            DWORD grayColor = (50 << 16) | (50 << 8) | 50;
            DWORD semiTransGray = (alpha << 24) | grayColor;

            DWORD* pData = (DWORD*)pBitmapData;
            for (int i = 0; i < width * height; ++i) {
                pData[i] = semiTransGray;
            }

            SetBkMode(hdcMem, TRANSPARENT);
            SetTextColor(hdcMem, RGB(255, 255, 255));

            HFONT hFont = CreateFontW(
                36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial"
            );
            HFONT hOldFont = (HFONT)SelectObject(hdcMem, hFont);

            // Converting to safeString
            std::wstring wideText;
            if (!g_lastTranslatedText.empty()) {
                int requiredSize = MultiByteToWideChar(CP_UTF8, 0, g_lastTranslatedText.c_str(), (int)g_lastTranslatedText.size(), NULL, 0);
                if (requiredSize > 0) {
                    wideText.resize(requiredSize);
                    MultiByteToWideChar(CP_UTF8, 0, g_lastTranslatedText.c_str(), (int)g_lastTranslatedText.size(), &wideText[0], requiredSize);
                }
            }

            // Drawing to backbuffer
            DrawTextW(hdcMem, wideText.c_str(), -1, &clientRect, DT_CENTER | DT_WORDBREAK | DT_VCENTER);

            SelectObject(hdcMem, hOldFont);
            DeleteObject(hFont);

            //Draw
            BLENDFUNCTION blend = { 0 };
            blend.BlendOp = AC_SRC_OVER;
            blend.BlendFlags = 0;
            blend.SourceConstantAlpha = 255;
            blend.AlphaFormat = AC_SRC_ALPHA;

            POINT ptSrc = { 0, 0 };
            SIZE size = { width, height };
            POINT ptDst = { clientRect.left, clientRect.top };

            UpdateLayeredWindow(hwnd, hdcWindow, &ptDst, &size, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

            //Cleanup
            SelectObject(hdcMem, hOldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(hdcMem);
            ReleaseDC(NULL, hdcScreen);
            ReleaseDC(hwnd, hdcWindow);

            ValidateRect(hwnd, NULL);

            return 0;
        }

        case WM_TIMER:
            if (wParam == 1) { 

                //checking to hide window
                HWND hCurrentActiveWindow = GetForegroundWindow();
                if (hCurrentActiveWindow != hGameWindow) {
                    if (g_isOverlayVisible) {
                        ShowWindow(hOverlay, SW_HIDE);
                        g_isOverlayVisible = false;
                    }
                    return 0;
                }
                if (!g_isOverlayVisible) {
                    ShowWindow(hOverlay, SW_SHOW);
                    g_isOverlayVisible = true;
                }
                cv::Mat game_image = captureGame();
                std::string ocrText = performOCR(game_image, g_tessApi);
                if (ocrText == g_lastText) {
                    g_stableCounter++;
                }
                else {
                    // Testo scomparso?
                    if (ocrText.empty()) {
                        if (!g_lastTranslatedText.empty()) {
                            g_lastTranslatedText = ""; // Svuota
                            InvalidateRect(hOverlay, NULL, TRUE); // Ridisegna (per pulire)
                        }
                    }
                    // Testo cambiato
                    g_stableCounter = 0;
                    g_lastText = ocrText;
                }
                if (g_stableCounter > 2 && g_stableCounter < 4 && !ocrText.empty()) {
                    //sending text to translate here
                    sendMessage(g_socketToPython, ocrText);
                    std::string translatedText = receiveMessage(g_socketToPython);
                    std::cout << "Translated Text: " << translatedText << std::endl;
                    g_lastTranslatedText = translatedText;
                    InvalidateRect(hOverlay, NULL, TRUE);
                }
            }
            return 0;
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
                     
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

HWND createOverlay() {
    hGameWindow = FindWindow(L"UnityWndClass", L"Honkai: Star Rail");
    if (hGameWindow == NULL) {
        std::cerr << "Game window not found!" << std::endl;
        return NULL;
    }

    RECT gameRect;
    GetWindowRect(hGameWindow, &gameRect);
    int gameX = gameRect.left + 5;
    int gameY = gameRect.top + 1400;
    int gameWidth = 2550;
    int gameHeight = 243;

    const wchar_t CLASS_NAME[] = L"OverlayWindowClass";

    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = NULL;

    if (!RegisterClassEx(&wc)) {
        std::cerr << "Class registration failed!" << std::endl;
        return NULL;
    }

    //Creating overlay window
    DWORD extendedStyle = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;

    hOverlay = CreateWindowEx(
        extendedStyle,                 
        CLASS_NAME,                     
        L"Game Translator Overlay",     
        WS_POPUP,                       
        gameX, gameY,                   
        gameWidth, gameHeight,         
        NULL, NULL, hInstance, NULL
    );

    if (hOverlay == NULL) {
        std::cerr << "Overlay creation failed!" << std::endl;
        return NULL;
    }

    //SetLayeredWindowAttributes(hOverlay, 0, 200, LWA_COLORKEY);

    ShowWindow(hOverlay, SW_SHOW);
    UpdateWindow(hOverlay);

    std::cout << "Overlay Created!" << std::endl;
    return hOverlay;
}