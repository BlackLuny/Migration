﻿// Thunderneko.cpp : 定义 DLL 应用程序的导出函数。
//

#include <stdio.h>
#include <Windows.h>
#include "..\Share\UMC.h"

HMODULE hMyModule = NULL;
WCHAR DNFMutantName [] = L"dbefeuate_ccen_khxfor_lcar_blr";
WCHAR DNFLauncherMutantName [] = L"NeopleLauncher";

int FoundCount = 0;

HOWTOCLOSE IdentifyDNFMutant(LPCWSTR MutantName, ULONG NameLength)
{
    ULONG nDNFMutantName = sizeof(DNFMutantName) / sizeof(DNFMutantName[0]) - 1;
    ULONG nDNFLauncherMutantName = sizeof(DNFLauncherMutantName) / sizeof(DNFLauncherMutantName[0]) - 1;
    ULONG nMaxLength = (sizeof(DNFMutantName) > sizeof(DNFLauncherMutantName)) ? nDNFMutantName : nDNFLauncherMutantName;

    if (NameLength < nMaxLength)
    {
        return DONT_CLOSE;
    }

    if (!wcsncmp(MutantName, DNFMutantName, nDNFMutantName) || !wcsncmp(MutantName, DNFLauncherMutantName, nDNFLauncherMutantName))
    {
        FoundCount++;
        return CLOSE_DIRECT;
    }
    return DONT_CLOSE;
}

int __stdcall MoeMoeSorayuki(LPVOID Useless)
{
    FoundCount = 0;
    if (!EnumerateAndCloseMutant(IdentifyDNFMutant))
    {
        FoundCount = -1;
    }
    return FoundCount;
}

VOID __stdcall MoeMoeAndExit(LPVOID Useless)
{
    FreeLibraryAndExitThread(hMyModule, MoeMoeSorayuki(NULL));
}

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved
    )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        hMyModule = hModule;
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}