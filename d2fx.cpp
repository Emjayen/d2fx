#include <Windows.h>
#include "dx.h"

#include <stdio.h>


LRESULT WINAPI OnWindowMsg(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_CREATE:
        return 0;
    }

    return DefWindowProc(hwnd, Msg, wParam, lParam);
}


void main()
{
    HWND hwnd;



    float height = 600;

    float y = 600;

    float r = -((y / height) * 2 - 1);



    SetProcessDPIAware();

    WNDCLASSEX wcx = {};
    wcx.cbSize = sizeof(wcx);
    wcx.hInstance = GetModuleHandle(NULL);
    wcx.lpszClassName = "d2fx";
    wcx.lpfnWndProc = &OnWindowMsg;

    RegisterClassEx(&wcx);

    if (!(hwnd = CreateWindowEx(WS_EX_APPWINDOW, wcx.lpszClassName, "d2fx", WS_VISIBLE | WS_OVERLAPPED, 0, 0, 800, 600, NULL, NULL, wcx.hInstance, NULL)))
        return;

    DxInitialize(hwnd);

    MSG msg;

    for (;;)
    {
        GetMessage(&msg, NULL, 0, 0);
        DispatchMessage(&msg);
    }
}


HANDLE hLog;

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpvReserved)  // reserved
{
    if(fdwReason == DLL_PROCESS_ATTACH)
    {
        hLog = CreateFile("d2fx.log", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, 0, 0);
    }

    return TRUE;
}


void Log(const char* pFormat, ...)
{
    char tmp[512];

    va_list va;
    va_start(va, pFormat);
    auto len = vsprintf(tmp, pFormat, va);
    va_end(va);

    DWORD Result;
    WriteFile(hLog, tmp, len, &Result, NULL);
    OutputDebugString(tmp);
}