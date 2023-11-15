#include <Windows.h>
#include "dx.h"

#include <stdio.h>

#include "helper.h"


LRESULT WINAPI OnWindowMsg(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_CREATE:
        return 0;
    }

    return DefWindowProc(hwnd, Msg, wParam, lParam);
}



HANDLE hLog;
u64 QpcHz;



u64 HPC()
{
    u64 t;
    QueryPerformanceCounter((LARGE_INTEGER*) &t);
    return t;
}


DWORD WINAPI PE_GetTickCount()
{
    u64 t = HPC();
    t *= 1000;
    t /= QpcHz;

    return t;
}



LRESULT WINAPI PE_PeekMessageA
(
    MSG* pMsg,
    HWND hwnd,
    UINT FilterMin,
    UINT FilterMax,
    UINT RemoveMsg
)
{
    return 0;
}


BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpvReserved)  // reserved
{
    if(fdwReason == DLL_PROCESS_ATTACH)
    {
        hLog = CreateFile("d2fx.log", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, 0, 0);


        QueryPerformanceFrequency((LARGE_INTEGER*) &QpcHz);

        ASSERT(PsWriteImport(GetModuleHandle(NULL), NULL, "GetTickCount", (uiptr) &PE_GetTickCount) != 0);
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
    //OutputDebugString(tmp);
}