/*
* Author:	Angelo Frasca Caccia ( lem0nSec_ )
* Title:	KBlast.exe ( client )
* Website:	https://github.com/lem0nSec/KBlast
*/


#include "KBlast.hpp"


/* todo
* Fix 'restore' in tokn
*/

RTL_OSVERSIONINFOW OSinfo = { 0 };

void KBlast_c_GetInfo(DWORD dwOption)
{
	SYSTEMTIME sTime = { 0 };
	DWORD dwBuild = 0;
	HMODULE ntdll = 0;
	PRTLGETVERSION RtlGetVersion = 0;

	if (OSinfo.dwOSVersionInfoSize == 0)
	{
		OSinfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);
		ntdll = GetModuleHandleW(L"ntdll.dll");
		if (ntdll != 0)
		{
			RtlGetVersion = (PRTLGETVERSION)GetProcAddress(ntdll, "RtlGetVersion");
			if (RtlGetVersion != 0)
			{
				RtlGetVersion(&OSinfo);
			}
		}
	}

	switch (dwOption)
	{
	case 0:
		GetSystemTime(&sTime);
		wprintf(
			L"    __ __ ____  __           __\n"
			L"   / //_// __ )/ /___ ______/ /_\t| KBlast client - OS Build #%d - Major version #%d\n"
			L"  / ,<  / __  / / __ `/ ___/ __/\t| Version : %s ( first release ) - Architecture : %s\n"
			L" / /| |/ /_/ / / /_/ (__  ) /_\t\t| Website : http://www.github.com/lem0nSec/KBlast\n"
			L"/_/ |_/_____/_/\\__,_/____/\\__/\t\t| Author  : lem0nSec_\n"
			L"------------------------------------------------------->>>\n", OSinfo.dwBuildNumber, OSinfo.dwMajorVersion, KBLAST_VERSION, KBLAST_ARCH
		);
		break;

	case 1:
		GetSystemTime(&sTime);
		wprintf(L"System time is : %d:%d:%d - %d/%d/%d\n", sTime.wHour, sTime.wMinute, sTime.wSecond, sTime.wMonth, sTime.wDay, sTime.wYear);
		break;

	case 2:
		wprintf(L"Architecture : %s\nBuild number : %d\nMajor version : %d\nMinor version : %d\nPlatform ID : %d\n", KBLAST_ARCH, OSinfo.dwBuildNumber, OSinfo.dwMajorVersion, OSinfo.dwMinorVersion, OSinfo.dwPlatformId);
		break;

	default:
		break;
	}

}

BOOL KBlast_c_CheckOSVersion()
{
	BOOL status = FALSE;
	HMODULE ntdll = 0;
	PRTLGETVERSION RtlGetVersion = 0;

	if (OSinfo.dwOSVersionInfoSize == 0)
	{
		OSinfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);
		ntdll = GetModuleHandleW(L"ntdll.dll");
		if (ntdll != 0)
		{
			RtlGetVersion = (PRTLGETVERSION)GetProcAddress(ntdll, "RtlGetVersion");
			if (RtlGetVersion != 0)
			{
				RtlGetVersion(&OSinfo);
			}
		}
	}

	if ((OSinfo.dwBuildNumber == 19045) && (OSinfo.dwMajorVersion == 10))
	{
		status = TRUE;
	}

	return status;

}

BOOL KBlast_c_init()
{
	BOOL initStatus = FALSE;
	DWORD szServiceInit = KBLAST_SD_FAILED;
	BOOL adminStatus = FALSE;

	adminStatus = KBlast_c_CheckTokenIntegrity();
	if (adminStatus == TRUE)
	{
		szServiceInit = KBlast_c_ServiceInitialize(SERVICE_CREATE_AND_LOAD);
		switch (szServiceInit)
		{
		case KBLAST_SD_SUCCESS:
			initStatus = TRUE;
			wprintf(L"[+] Driver up.\n");
			break;

		case KBLAST_SD_FAILED:
			wprintf(L"[-] Service registration failed.\n");
			break;

		case KBLAST_D_SUCCESS:
			initStatus = TRUE;
			wprintf(L"[+] Driver up.\n");
			break;

		case KBLAST_D_FAILED:
			wprintf(L"[-] Driver down.\n");
			break;

		case KBLAST_SD_EXIST:
			initStatus = TRUE;
			wprintf(L"[+] Driver up.\n");
			break;

		case KBLAST_BINARY_NOT_FOUND:
			wprintf(L"[-] %s not found.\n", KBLAST_DRV_BINARY);
			break;

		case KBLAST_BINARY_ERROR_GENERIC:
			wprintf(L"[-] %s error generic.\n", KBLAST_DRV_BINARY);
			break;

		default:
			break;
		}
	}
	else
	{
		wprintf(L"[-] Insufficient privileges. Quitting...\n");
	}

	return initStatus;

}


BOOL KBlast_c_cleanup()
{
	BOOL status = FALSE;
	DWORD szServiceStatus = KBLAST_SD_EXIST;
	BOOL adminStatus = FALSE;


	adminStatus = KBlast_c_CheckTokenIntegrity();
	if (adminStatus == TRUE)
	{
		szServiceStatus = KBlast_c_ServiceInitialize(SERVICE_UNLOAD_AND_DELETE);
		if (szServiceStatus == KBLAST_SD_SUCCESS)
		{
			wprintf(L"[+] Success.\n");
		}
		else
		{
			wprintf(L"[-] Failed.\n");
		}

	}

	return status;

}


void KBlast_c_ConsoleInit()
{
	COORD topLeft = { 0, 0 };
	HANDLE hConsole = 0;
	CONSOLE_SCREEN_BUFFER_INFO cInfo = { 0 };
	DWORD dwWritten;

	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hConsole, &cInfo);
	FillConsoleOutputCharacterW(hConsole, ' ', cInfo.dwSize.X, topLeft, &dwWritten);
	FillConsoleOutputCharacterW(hConsole, ' ', cInfo.dwSize.Y, topLeft, &dwWritten);
	SetConsoleCursorPosition(hConsole, topLeft);

	RtlZeroMemory(&cInfo, sizeof(CONSOLE_SCREEN_BUFFER_INFO));

	SetConsoleTitle(KBLAST_CLT_TITLE);
	KBlast_c_GetInfo(0);
}


BOOL KBlast_c_system(wchar_t* input)
{
	BOOL status = FALSE;
	char* systemInput = 0;

	systemInput = KBlast_c_utils_UnicodeStringToAnsiString(input);
	if (systemInput != 0)
	{
		system((char*)((DWORD_PTR)systemInput + 1));
		wprintf(L"\n");
		status = TRUE;
	}

	KBlast_c_utils_FreeAnsiString(systemInput);

	return status;

}


BOOL KBlast_c_ConsoleStart()
{
	BOOL status = FALSE;
	KBlast_c_ConsoleInit();

	if (KBlast_c_CheckOSVersion() == FALSE)
	{
		wprintf(L"[!] Warning : This OS version might not be fully supported. Critical issues may rise.\n");
	}

	wchar_t input[MAX_PATH];
	while (TRUE)
	{
		wprintf(L"KBlast > ");
		fgetws(input, ARRAYSIZE(input), stdin); fflush(stdin);
		if (wcscmp(input, L"help\n") == 0)
		{
			KBlast_c_module_help(GENERIC);
		}
		if (wcscmp(input, L"quit\n") == 0)
		{
			wprintf(L"bye!\n");
			break;
		}
		if (wcscmp(input, L"banner\n") == 0)
		{
			KBlast_c_GetInfo(0);
		}
		if (wcscmp(input, L"cls\n") == 0)
		{
			system("cls");
		}
		if (wcscmp(input, L"pid\n") == 0)
		{
			wprintf(L"PID : %d\n", GetCurrentProcessId());
		}
		if (wcsncmp(input, L"!", 1) == 0)
		{
			status = KBlast_c_system(input);
		}
		if (wcscmp(input, L"time\n") == 0)
		{
			KBlast_c_GetInfo(1);
		}
		if (wcscmp(input, L"version\n") == 0)
		{
			KBlast_c_GetInfo(2);
		}
		if (wcsncmp(input, KBLAST_MOD_MISC, 5) == 0)
		{
			KBlast_c_device_dispatch_misc((wchar_t*)((DWORD_PTR)input + 10));
		}
		if (wcsncmp(input, KBLAST_MOD_PROTECTION, 5) == 0)
		{
			KBlast_c_device_dispatch_protection((wchar_t*)((DWORD_PTR)input + 10));
		}
		if (wcsncmp(input, KBLAST_MOD_TOKEN, 5) == 0)
		{
			KBlast_c_device_dispatch_token((wchar_t*)((DWORD_PTR)input + 10));
		}
		if (wcsncmp(input, KBLAST_MOD_CALLBACK, 5) == 0)
		{
			KBlast_c_device_dispatch_callbacks((wchar_t*)((DWORD_PTR)input + 10));
		}
	}

	return status;

}


int wmain(int argc, wchar_t* argv[])
{
	BOOL status = FALSE;
	BOOL start = TRUE;

	if (argc < 2)
	{
		status = KBlast_c_init();
		if (status == TRUE)
		{
			wprintf(L"[+] Starting console...\n");
			KBlast_c_ConsoleStart();
			KBlast_c_cleanup();
		}
	}
	if (argc < 3)
	{
		start = FALSE;
		if (wcscmp(argv[1], L"/load") == 0) // load driver and exit
		{
			status = KBlast_c_init();
		}
		if (wcscmp(argv[1], L"/unload") == 0) // unload driver and exit
		{
			status = KBlast_c_cleanup();
		}
	}

	return status;

}