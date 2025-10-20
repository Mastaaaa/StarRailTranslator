#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <thread>
#pragma comment(lib, "ws2_32.lib")

bool startPythonServer(const std::wstring& pythonExe, const std::wstring& scriptPath);

SOCKET connectToServer(const char* ip, int port);

bool sendMessage(SOCKET clientSocket, std::string message);

std::string receiveMessage(SOCKET clientSocket);
