#include <windows.h>
#include <tchar.h>
#include <locale.h>
#include <malloc.h>
#include "TxtMiruCli.h"

#define TXTMIRUEXE        _T("TxtMiru2.exe")
#define TXTMIRU_PROP_NAME _T("TxtMiru2.0-Server")

struct TxtMiruData
{
	HWND hWnd;
};
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	if(!GetProp(hwnd, TXTMIRU_PROP_NAME)){
		return TRUE;
	}
	auto *lpData = reinterpret_cast<TxtMiruData*>(lParam);
	lpData->hWnd = hwnd;
	return FALSE;
}

/////////////////////////////////
#define MSGFLT_ADD        1
#define WM_COPYGLOBALDATA 0x0049

using FuncChangeWindowMessageFilter = BOOL (__stdcall *)(UINT message, DWORD dwFlag);
using FuncChangeWindowMessageFilterEx = BOOL (__stdcall *)(HWND hWnd, UINT message, DWORD action, PCHANGEFILTERSTRUCT pChangeFilterStruct);

int SendCopyData(HWND hWnd, COPYDATASTRUCT &cpydata)
{
	auto hLib = LoadLibrary(_T("user32.dll"));
	if (hLib) {
		auto ChangeWindowMessageFilterEx = reinterpret_cast<FuncChangeWindowMessageFilterEx>(GetProcAddress(hLib, "ChangeWindowMessageFilterEx"));
		if (ChangeWindowMessageFilterEx) {
			ChangeWindowMessageFilterEx(hWnd, WM_COPYDATA, MSGFLT_ALLOW, NULL);
			ChangeWindowMessageFilterEx(hWnd, WM_COPYGLOBALDATA, MSGFLT_ALLOW, NULL);
		} else {
			auto ChangeWindowMessageFilter = reinterpret_cast<FuncChangeWindowMessageFilter>(GetProcAddress(hLib, "ChangeWindowMessageFilter"));
			if (ChangeWindowMessageFilter) {
				ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
				ChangeWindowMessageFilter(WM_COPYGLOBALDATA, MSGFLT_ADD);
			}
		}
		FreeLibrary(hLib);
	}
	SendMessage(hWnd, WM_COPYDATA, NULL/*hwnd*/, reinterpret_cast<LPARAM>(&cpydata));
	return 0;
}

int WINAPI _tWinMain(_In_ HINSTANCE hCurInst, _In_opt_ HINSTANCE hPrevInst, _In_ LPTSTR lpsCmdLine, _In_ int nCmdShow)
{
	if (!lpsCmdLine) {
		return 0;
	}
	TCHAR szLanguageName[100];
	::GetLocaleInfo(GetSystemDefaultLCID(), LOCALE_SENGLANGUAGE, szLanguageName, _countof(szLanguageName));
	_tsetlocale(LC_ALL, szLanguageName);

	TxtMiruData data = {};
	EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
	if (data.hWnd) {
		COPYDATASTRUCT cpydata = {};
		cpydata.cbData = lstrlen(lpsCmdLine) * sizeof(TCHAR) + 1;
		cpydata.lpData = reinterpret_cast<void*>(lpsCmdLine);

		SendMessage(data.hWnd, WM_COPYDATA, NULL/*hwnd*/, reinterpret_cast<LPARAM>(&cpydata));
		auto dwForceThread = GetWindowThreadProcessId(data.hWnd, nullptr);
		auto dwCurrentThread = GetCurrentThreadId();
		if (dwForceThread == dwCurrentThread) {
			BringWindowToTop(data.hWnd);
		} else {
			AttachThreadInput(dwForceThread, dwCurrentThread, true);
			BringWindowToTop(data.hWnd);
			AttachThreadInput(dwForceThread, dwCurrentThread, false);
		}
	} else {
		auto len = _countof(TXTMIRUEXE) + 1 + lstrlen(lpsCmdLine) + 1;
		auto lpTxtMiruCmdLine = static_cast<LPTSTR>(_malloca((len + 1) * sizeof(TCHAR)));
		if (lpTxtMiruCmdLine) {
			PROCESS_INFORMATION pi = {};
			STARTUPINFO si = { sizeof(STARTUPINFO) };
			_stprintf_s(lpTxtMiruCmdLine, len/* * sizeof(TCHAR)*/, _T("%s %s"), TXTMIRUEXE, lpsCmdLine);
			CreateProcess(nullptr, lpTxtMiruCmdLine, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);
			if (pi.hThread) {
				CloseHandle(pi.hThread);
			}
			if (pi.hProcess) {
				CloseHandle(pi.hProcess);
			}
		}
	}
	return 0;
}
