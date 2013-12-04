// Migration.cpp : ����Ӧ�ó������ڵ㡣
//

#include <Windows.h>
#include <CommCtrl.h>
#include <tlhelp32.h>
#include "resource.h"
#include "UMC.h"
#include "Migration.h"

wchar_t DNFMutantName[] = L"dbefeuate_ccen_khxfor_lcar_blr";
wchar_t DNFLauncherMutantName[] = L"NeopleLauncher";
int FoundCount = 0;
BOOL RunningOnX86 = TRUE;

HWND hMainWindow = NULL;

// ����Ƿ���64λ����ϵͳ��x64����1��x86����0�����򷵻�2
int Is64bitWindows()
{
	GNSITYPE GetNativeSystemInfo = (GNSITYPE)GetProcAddress(GetModuleHandle(L"kernel32.dll"),"GetNativeSystemInfo");
	if(!GetNativeSystemInfo) return 0;

	SYSTEM_INFO si;
	ZeroMemory(&si,sizeof(si));
	GetNativeSystemInfo(&si);
	if(si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
		return 0;
	else if(si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
		return 1;
	return 2;
}

BOOL AdjustPrivilege(BOOL bEnable)
{
	HANDLE hToken=0;
	TOKEN_PRIVILEGES tkp;

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = 0;
	if(bEnable) tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!LookupPrivilegeValue(NULL, L"SeDebugPrivilege", &tkp.Privileges[0].Luid)) return FALSE;
	if (OpenProcessToken((HANDLE)-1, TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &hToken))
	{
		if (AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, NULL))
		{
			CloseHandle(hToken);
			return TRUE;
		}
		CloseHandle(hToken);
	}
	return FALSE;
}

HOWTOCLOSE IdentifyDNFMutant(wchar_t* MutantName,ULONG NameLength)
{
	if(NameLength < 30)
		return DONT_CLOSE;

	wchar_t* RevName = new wchar_t[NameLength+1];
	wcsncpy(RevName,MutantName,NameLength);
	wcsrev(RevName);
	if(wcsncmp(RevName,DNFMutantName,30) == NULL)
	{
		delete[] RevName;
		FoundCount++;
		
		if(RunningOnX86)
			return CLOSE_INJECT;
		else
			return CLOSE_DIRECT;
	}
	else if(wcsncmp(RevName,DNFLauncherMutantName,14) == NULL)
	{
		delete[] RevName;
		FoundCount++;
		return CLOSE_DIRECT;
	}
	delete[] RevName;
	return DONT_CLOSE;
}

BOOL ShowHideAllDnf(BOOL bShow)
{
	HWND EvilParent = hMainWindow;
	HWND Migration = NULL;
	if(!bShow)
	{
		EvilParent = NULL;
		Migration = hMainWindow;
	}
	HWND hWnd = NULL;
	while(true)
	{
		hWnd = FindWindowEx(EvilParent,NULL,L"���³�����ʿ",L"���³�����ʿ");

		if(hWnd == NULL) break;
		if(bShow)
			SetParent(hWnd,Migration);
		FlashWindow(hWnd,TRUE);
		ShowWindow(hWnd,bShow ? SW_SHOW : SW_HIDE);
		FlashWindow(hWnd,TRUE);
		if(!bShow)
			SetParent(hWnd,Migration);
	}
	return TRUE;
}

void FuckJunkProcess()
{
	HANDLE hProcessSnap = NULL;
	PROCESSENTRY32 pe32 = {0};
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	ULONG_PTR PidCollect[16] = {0};
	ULONG_PTR Collected = 0;
	int DNFCount = 0;

	pe32.dwSize = sizeof(pe32);
	if(Process32First(hProcessSnap,&pe32))
	{
		do
		{
			if(wcsicmp(pe32.szExeFile,L"qqlogin.exe") == NULL || wcsicmp(pe32.szExeFile,L"QQDL.exe") == NULL || wcsicmp(pe32.szExeFile,L"TenSafe.exe") == NULL)
			{
				PidCollect[Collected++] = pe32.th32ProcessID;
			}
			else if(wcsicmp(pe32.szExeFile,L"dnf.exe") == NULL)
			{
				DNFCount++;
			}
		} while(Process32Next(hProcessSnap,&pe32));
	}

	CloseHandle(hProcessSnap);
	if(DNFCount >= 2)
	{
		for(ULONG_PTR i = 0;i < Collected;i++)
		{
			HANDLE hMLGB = OpenProcess(PROCESS_TERMINATE,FALSE,PidCollect[i]);
			TerminateProcess(hMLGB,0);
			CloseHandle(hMLGB);
		}
	}
}

int InjectDllAndRunFunc(wchar_t* pszDllFile, DWORD dwProcessId,DWORD FuncOffset)
{
	HANDLE hProcess  = NULL;
	HANDLE hThread   = NULL;
	DWORD dwSize   = 0;
	CHAR* pszRemoteBuf = NULL;
	LPTHREAD_START_ROUTINE lpThreadFun = NULL;

	hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, dwProcessId);
	if (NULL == hProcess)
	{
		return -2;
	}

	dwSize = (DWORD)((wcslen(pszDllFile) + 1)*2);
	pszRemoteBuf = (CHAR*)VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
	if (NULL == pszRemoteBuf)
	{
		CloseHandle(hProcess);
		return -2;
	}

	if (FALSE == WriteProcessMemory(hProcess, pszRemoteBuf, (LPVOID)pszDllFile, dwSize, NULL))
	{
		VirtualFreeEx(hProcess, pszRemoteBuf, dwSize, MEM_DECOMMIT);
		CloseHandle(hProcess);
		return -2;
	}

	lpThreadFun = (LPTHREAD_START_ROUTINE)LoadLibraryW;

	if (NULL == lpThreadFun)
	{
		VirtualFreeEx(hProcess, pszRemoteBuf, dwSize, MEM_DECOMMIT);
		CloseHandle(hProcess);
		return -2;
	}

	hThread = CreateRemoteThread(hProcess, NULL, 0, lpThreadFun, pszRemoteBuf, 0, NULL);
	if(NULL == hThread) hThread = OsCreateRemoteThread2(hProcess, NULL, 0, lpThreadFun, pszRemoteBuf, 0, NULL);
	if (NULL == hThread)
	{
		VirtualFreeEx(hProcess, pszRemoteBuf, dwSize, MEM_DECOMMIT);
		CloseHandle(hProcess);
		return -2;
	}

	HMODULE hDLLModule = NULL;
	WaitForSingleObject(hThread, INFINITE);
	GetExitCodeThread(hThread,(LPDWORD)&hDLLModule);

	VirtualFreeEx(hProcess, pszRemoteBuf, dwSize, MEM_DECOMMIT);
	CloseHandle(hThread);

	lpThreadFun = (LPTHREAD_START_ROUTINE)((BYTE*)hDLLModule+FuncOffset);

	hThread = CreateRemoteThread(hProcess, NULL, 0, lpThreadFun, pszRemoteBuf, 0, NULL);
	if(NULL == hThread) hThread = OsCreateRemoteThread2(hProcess, NULL, 0, lpThreadFun, pszRemoteBuf, 0, NULL);

	if(NULL == hThread)
	{
		CloseHandle(hProcess);
		return -2;
	}

	WaitForSingleObject(hThread, INFINITE);
	int Result = -2;
	GetExitCodeThread(hThread,(LPDWORD)&Result);
	CloseHandle(hThread);
	CloseHandle(hProcess);

	return Result;
}

int InjectEnumerateAndCloseMutant()
{
	HMODULE hThunderNeko = LoadLibrary(L"ThunderNeko.dll");
	if(hThunderNeko == NULL)
		return -1;
	
	void* FuncAddress = GetProcAddress(hThunderNeko,"MoeMoeAndExit");
	if(FuncAddress == NULL)
		return -1;

	wchar_t FullPath[MAX_PATH*2];
	DWORD FuncOffset = (BYTE*)FuncAddress - (BYTE*)hThunderNeko;
	GetModuleFileName(hThunderNeko,FullPath,MAX_PATH*2);
	FreeLibrary(hThunderNeko);

	DWORD MyTar = 0;
	HANDLE hProcessSnap = NULL;
	PROCESSENTRY32 pe32 = {0};
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

	pe32.dwSize = sizeof(pe32);
	if(Process32First(hProcessSnap,&pe32))
	{
		do
		{
			if(wcsicmp(pe32.szExeFile,L"svchost.exe") == NULL)
			{
				MyTar = pe32.th32ProcessID;
				break;
			}
		} while(Process32Next(hProcessSnap,&pe32));
	}
	CloseHandle(hProcessSnap);

	return InjectDllAndRunFunc(FullPath,MyTar,FuncOffset);
}


INT_PTR CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_INITDIALOG:
		hMainWindow = hWnd;
#ifdef _WIN64
		{
		wchar_t WindowTitle[200] = {0};
		GetWindowText(hWnd,WindowTitle,200);
		wcscat_s(WindowTitle,L" (x64)");
		SetWindowText(hWnd,WindowTitle);
		}
#endif
		return TRUE;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
			case IDCANCEL:
				EndDialog(hWnd,NULL);
				break;
			case IDC_CLOSEHANDLE:
			{
#ifndef _WIN64
				DWORD dwMajorVersion = (DWORD)(LOBYTE(LOWORD(GetVersion())));
				if(dwMajorVersion < 6 && RunningOnX86)
				{
					// ����ͷ��XP��û��Ȩ
					FoundCount = InjectEnumerateAndCloseMutant();
					if(FoundCount == -1)
					{
						MessageBox(hWnd,L"������ʹ�õ�Windows����ϵͳ�汾С��Windows Vista����˳�����ҪThunderNeko.dll�����ڱ�����Ŀ¼�²�������������\n\n��������ThunderNeko.dll�ܿ����𻵻򲻴��ڡ�",L"����ͷ��XP��û��Ȩ",MB_ICONINFORMATION|MB_OK);
					}
					if(FoundCount == -2)
					{
						MessageBox(hWnd,L"����ʧ��...",L"...",MB_ICONSTOP|MB_OK);
					}
				}
				else
#endif
				{
					FoundCount = 0;
					if(!EnumerateAndCloseMutant(IdentifyDNFMutant))
					{
						MessageBox(hWnd,L"�������ö���г�����һЩ���⣬��Ȩ�޲�����\n\n�����ʹ�õĲ���ϵͳ��Windows XP/2003 x64 Edition�����������x64�汾ʹ�ã����ܿ��Խ��������⡣",L"����",MB_ICONSTOP|MB_OK);
						break;
					}
				}
				if(FoundCount == 0)
				{
					MessageBox(hWnd,L"�۸�����û�ҵ���...",L"����",MB_OK|MB_ICONSTOP);
				}
				else if(FoundCount > 1)
				{
					MessageBox(hWnd,L"˵��ģ����������P��..",L"�ɹ�",MB_OK|MB_ICONINFORMATION);
				}
				else if(FoundCount == 1)
				{
					MessageBox(hWnd,L"������һ������һ����",L"���",MB_OK|MB_ICONINFORMATION);
				}
				break;
			}
			case IDC_HIDEWINDOW:
				ShowHideAllDnf(FALSE);
				break;
			case IDC_SHOWWINDOW:
				ShowHideAllDnf(TRUE);
				FuckJunkProcess();
				break;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPWSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	if(!AdjustPrivilege(TRUE))
	{
		MessageBox(NULL,L"�޷�ȡ�õ���Ȩ�ޡ�\n\n��ʹ�ù���ԱȨ�����б�����",L"����",MB_ICONSTOP|MB_OK);
		return 0;
	}

	InitCommonControls();

	if(Is64bitWindows() != 0)
	{
		RunningOnX86 = FALSE;
#ifndef _WIN64
		MessageBox(NULL,L"����ǰ����һ��64λ�汾Windows��ʹ�ñ������32λ�汾�����п�����ɱ����������������뾡����ȡһ��Migration_x64.exe��ʹ�á�",L"��ʾ",MB_OK|MB_ICONINFORMATION);
#endif
	}

	wcsrev(DNFMutantName);
	wcsrev(DNFLauncherMutantName);

	return DialogBox(hInstance,MAKEINTRESOURCE(IDD_MAINDLG),NULL,WndProc);
}