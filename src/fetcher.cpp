#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <string>
using namespace std;

#pragma comment(lib, "wininet.lib")

string fetch_binance(const string& endpoint) {
    HINTERNET hInternet = InternetOpenA(
        "ArbitrageX",
        INTERNET_OPEN_TYPE_DIRECT,
        NULL, NULL, 0
    );
    if (!hInternet) {
        cerr << "[ERROR] InternetOpen failed\n";
        return "";
    }

    HINTERNET hConnect = InternetOpenUrlA(
        hInternet,
        endpoint.c_str(),
        NULL, 0,
        INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD,
        0
    );
    if (!hConnect) {
        cerr << "[ERROR] InternetOpenUrl failed: " << GetLastError() << "\n";
        InternetCloseHandle(hInternet);
        return "";
    }

    string response;
    char buffer[4096];
    DWORD bytesRead = 0;

    while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        response += buffer;
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return response;
}
