#include "connection.h"
#include "ocr.h"
#include "capture.h"
#include "overlay.h"

int main() {
    tesseract::TessBaseAPI* tessApi = initTesseract();
    std::string lastText;
    int stableCounter = 0;


    const std::wstring pyhtonExePath = L"C:/Users/Masta/AppData/Local/Programs/Python/Python313/python.exe";
    const std::wstring pyProgramPath = L"server.py";
    const char* ip = "127.0.0.1";
    int port = 65432;

    if (!startPythonServer(pyhtonExePath, pyProgramPath)) {
        std::cout << "Error starting pyhton server";
        return -1;
    }
    //TODO waits to avoid errors, maybe instead of sleep try to reconnect
    Sleep(100);
    //creating socket for server communication

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup fallito con errore: " << result << std::endl;
        return 1;
    }
    SOCKET socketToPython = connectToServer(ip, port);
    HWND hOverlay = createOverlay();
    //Init timer overaly for msg callback function
    SetTimer(hOverlay, 1, 50, NULL);

    //TODO - Instead of infinite loop here
    //----> Instead sends messageses for the overlay handler in overlay.cpp

    InitOverlayLogic(hOverlay, tessApi, socketToPython);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    //cleanup tess;
    closesocket(socketToPython);
    WSACleanup();
    tessApi->End();
    delete tessApi;
    return 0;
}
