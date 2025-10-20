#include "connection.h"

bool startPythonServer(const std::wstring& pythonExe, const std::wstring& scriptPath) {
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    std::wstring cmd = L"\"" + pythonExe + L"\" \"" + scriptPath + L"\"";

    if (!CreateProcessW(
        NULL,           
        &cmd[0],    
        NULL, NULL, FALSE, 0,
        NULL, NULL, &si, &pi))
    {
        std::wcerr << L"Error starting python: " << GetLastError() << std::endl;
        return false;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return true;
}

SOCKET connectToServer(const char* ip, int port) {
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Creating socket failed with error: " << WSAGetLastError() << std::endl;
        return INVALID_SOCKET;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed with error: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        return INVALID_SOCKET;
    }

    std::cout << "Successfull connected to server" << std::endl;
    return clientSocket;
}

bool sendMessage(SOCKET clientSocket, std::string message) {
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Invalid socket" << std::endl;
        return false;
    }

    int result = send(clientSocket, message.c_str(), (int)message.length(), 0);
    if (result == SOCKET_ERROR) {
        std::cerr << "Sending failed with error: " << WSAGetLastError() << std::endl;
        return false;
    }
    //std::cout << "Sent message: '" << message << "'" << std::endl;
    return true;
}

std::string receiveMessage(SOCKET clientSocket) {
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived == 0) {
        std::cout << "Connection to server closed" << std::endl;
        return "";
    }
    buffer[bytesReceived] = '\0';
    return std::string(buffer);
}