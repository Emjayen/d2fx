#include <Windows.h>
#include "dx.h"
#include "d2fx.h"
#include "d2def.h"

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


u32 HPCMs()
{
    u64 t;
    QueryPerformanceCounter((LARGE_INTEGER*) &t);
    t *= 1000;
    t /= QpcHz;

    return (u32) t;
}



u32* D2LastUpdateTick = (u32*) 0x7A0490;

u32* D2Unknown1 = (u32*) 0x7A04BC;


DWORD WINAPI PE_GetTickCount()
{
    auto t = HPCMs();

    if(_ReturnAddress() == (void*) 0x44F088)
    {
        *D2Unknown1 = 0;

        static u32 last;
        
        LOG("Game update: %ums delta", t - last);
        last = t;

        return (*D2LastUpdateTick) + 40;
    }

    return t;
}


DWORD WINAPI PE_timeGetTime()
{
    static bool bTest;

    if(bTest)
        return 1;

    return HPCMs();
}


extern HANDLE hVBI;


BOOL WINAPI PE_GetMessageA
(
    MSG* pMsg,
    HWND hwnd,
    UINT FilterMin,
    UINT FilterMax
)
{
    if(_ReturnAddress() != (void*) 0x451BE6)
        return GetMessageA(pMsg, hwnd, FilterMin, FilterMax);

    return FALSE;
}



bool bShouldBeRendering = true;


u32 saved_PrecisionX;
u32 saved_PrecisionY;


void SavePlayer(D2Unit* pUnit)
{
    saved_PrecisionX = pUnit->pStaticPath->PrecisionX;
    saved_PrecisionY = pUnit->pStaticPath->PrecisionY;
}

void RestorePlayer(D2Unit* pUnit)
{
    pUnit->pStaticPath->PrecisionX = saved_PrecisionX;
    pUnit->pStaticPath->PrecisionY = saved_PrecisionY;
}

void StepPlayer(D2Unit* pUnit, u64 hpcdt)
{
    //D2Unit* pUnit = *(D2Unit**) (((uiptr) GetModuleHandle(NULL)) + 0x3A6A70);


    pUnit->pStaticPath->PrecisionX = 0;
    pUnit->pStaticPath->PrecisionY = 0;
}


BOOL WINAPI PE_PeekMessageA
(
    MSG* pMsg,
    HWND hwnd,
    UINT FilterMin,
    UINT FilterMax,
    UINT RemoveMsg
)
{
    if(_ReturnAddress() != (void*) 0x451BD2 || *((u32*) 0x7A061C) == 0)
        return PeekMessageA(pMsg, hwnd, FilterMin, FilterMax, RemoveMsg);

    static u64 t_accum;
    static u64 t_last;

    LOG("Enter wait");

    t_last = HPC();

    auto hpc_step = QpcHz / 25;

    D2Unit* pPlayer = *(D2Unit**) (((uiptr) GetModuleHandle(NULL)) + 0x3A6A70);

    SavePlayer(pPlayer);

    for(;;)
    {
        if(MsgWaitForMultipleObjectsEx(1, &hVBI, INFINITE, QS_ALLINPUT, 0) == WAIT_OBJECT_0)
        {
            auto t = HPC();
            auto dt = t - t_last;
            t_last = t;
            t_accum += dt;

            auto step_count = t_accum / hpc_step;
            t_accum -= step_count * hpc_step;

            auto const do_game_step = step_count > 0;

            LOG("Step for %I64u steps", step_count);

           
            //if(do_game_step)
               // RestorePlayer(pPlayer);

            while(step_count--)
                ((void(__stdcall*)(void*)) 0x44EFA0)(0);

            if(do_game_step)
                SavePlayer(pPlayer);

           // StepPlayer(pPlayer, t_accum);

            if(*((u32*) 0x7A061C))
            {
                void** addr = ((void**) 0x7A0484);
                bShouldBeRendering = true;
                ((void(__thiscall*)(void*)) (*addr))(0);
                bShouldBeRendering = false;
            }
         
        }

        else
        {
            HACCEL* phAccel = (HACCEL*) 0x7A0708;

            MSG msg;

            while(PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if(msg.message == WM_QUIT)
                    return TRUE;

                TranslateAccelerator(msg.hwnd, *phAccel, &msg);
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }
    }
}



VOID WINAPI PE_Sleep(DWORD dwMilliseconds)
{
    if(dwMilliseconds == 250)
        return;

    Sleep(dwMilliseconds);
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

        ASSERT(PsWriteImport(GetModuleHandle(NULL), NULL, "Sleep", (uiptr) &PE_Sleep) != 0);
        ASSERT(PsWriteImport(GetModuleHandle(NULL), NULL, "PeekMessageA", (uiptr) &PE_PeekMessageA) != 0);
        //ASSERT(PsWriteImport(GetModuleHandle(NULL), NULL, "GetMessageA", (uiptr) &PE_GetMessageA) != 0);

        ASSERT(PsWriteImport(GetModuleHandle(NULL), NULL, "GetTickCount", (uiptr) &PE_GetTickCount) != 0);
        ASSERT(PsWriteImport(GetModuleHandle(NULL), NULL, "timeGetTime", (uiptr) &PE_timeGetTime) != 0);

        DWORD Old;
        VirtualProtect((LPVOID) 0x44F28B, 8, PAGE_READWRITE, &Old);
        memset((void*) 0x44F28B, 0x90, 6);
        VirtualProtect((LPVOID) 0x44F28B, 8, Old, &Old);
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