#pragma once
#include <locale>
#include <codecvt>
#include <string>
#include <iostream>
#include <windows.h>

HWND createOverlay();

void InitOverlayLogic(HWND hOverlay, tesseract::TessBaseAPI* api, SOCKET sock);