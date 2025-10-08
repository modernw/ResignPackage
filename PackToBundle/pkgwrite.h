#pragma once
#include <Windows.h>
#include <atlbase.h>
#include <Shlwapi.h>
#include <strsafe.h>
#include <string>
#include <vector>
#include <map>
#include <AppxPackaging.h>
#include <process.h>
#include <fmt/format.h>
#include "filedir.h"
#include "initread.h"
#include "threadcer.h"
#include "pkgread.h"
#include "version.h"
#include "cmdpipe.h"
#include "wndlibs.h"
#include "rctools.h"
#include "resource.h"

extern WInitFile g_config (EnsureTrailingSlash (GetProgramRootDirectoryW ()) + L"config.ini");
typedef struct IODirection
{
	typedef std::function <INT (LPCTSTR)> Input;
	typedef std::function <INT (LPCTSTR)> Output;
	typedef std::function <INT (LPCTSTR)> OutputLine;
	Input input = nullptr;
	Output output = nullptr;
	OutputLine outputLine = nullptr;
	IODirection (Input inputFunc = nullptr, Output outputFunc = nullptr, OutputLine outLineFunc = nullptr):
		input (inputFunc), output (outputFunc), outputLine (outLineFunc) {}
	IODirection (const IODirection &io): input (io.input), output (io.output), outputLine (io.outputLine) {}
	IODirection &operator = (const IODirection &io)
	{
		this->input = io.input;
		this->output = io.output;
		this->outputLine = io.outputLine;
	}
	INT safeInput (LPCTSTR str) { CreateScopedLock (m_cs); if (input && strvalid (str)) return input (str); else return 0; }
	INT safeOutput (LPCTSTR str) { CreateScopedLock (m_cs); if (output && strvalid (str)) return output (str); else return 0; }
	INT safeOutputLine (LPCTSTR str) {
		CreateScopedLock (m_cs);
		if (outputLine && strvalid (str)) outputLine (str);
		else if (output && strvalid (str)) output ((std::basic_string <_TCHAR> (str) + _T ("\n")).c_str ());
		else return 0;
	}
	INT safeInput (const std::basic_string <_TCHAR> &str) { return safeInput (str.c_str ()); }
	INT safeOutput (const std::basic_string <_TCHAR> &str) { return safeOutput (str.c_str ()); }
	INT safeOutputLine (const std::basic_string <_TCHAR> &str) { return safeOutputLine (str.c_str ()); }
	private:
	CriticalSection m_cs;
} IODIRECTION, *HIODIRECTION;

const std::wstring GetConfigFilePath ()
{
	return g_config.getFilePath ();
}
std::wstring GetKitDirectory ()
{
	std::wstring kitDir = g_config.readStringValue (
		L"Settings",
		L"KitDirectory",
		EnsureTrailingSlash (GetProgramRootDirectoryW ()) + L"kits"
	);
	kitDir = ProcessEnvVars (kitDir);
	return kitDir;
}
std::wstring GetKitToolFilePath (const std::wstring &filename)
{
	std::wstring kitdir = GetKitDirectory ();
	std::vector <WCHAR> buf (kitdir.capacity () + filename.capacity () + 4);
	PathCombineW ((LPWSTR)buf.data (), kitdir.c_str (), filename.c_str ());
	return buf.data ();
}
#define makecert_exe GetKitToolFilePath (L"makecert.exe")
#define pvk2pfx_exe GetKitToolFilePath (L"pvk2pfx.exe")
#define signtool_exe GetKitToolFilePath (L"signtool.exe")
bool CheckKits ()
{
	std::wstring kitDir = GetKitDirectory ();
	if (!IsDirectoryExists (kitDir)) return false;
	bool kitsOk =
		IsFileExists (makecert_exe) &&
		IsFileExists (pvk2pfx_exe) &&
		IsFileExists (signtool_exe);
	return kitsOk;
}

bool MakeCert (const std::wstring &swIdentityPublisher, const std::wstring &swOutputDir, const std::wstring &swOutputFileName, std::wstring &outCerFilePath, std::wstring &outPvkFilePath, IODirection &outputConsole = IODirection (), HWND owner = NULL)
{
	ConsolePipe console;
	std::wstring m_kitdir = GetKitDirectory (), cmdline (L"");
	std::wstring m_pvkfile = EnsureTrailingSlash (ProcessEnvVars (swOutputDir)) + swOutputFileName + L".pvk";
	std::wstring m_cerfile = EnsureTrailingSlash (ProcessEnvVars (swOutputDir)) + swOutputFileName + L".cer";
	cmdline += L"\"" + EnsureTrailingSlash (m_kitdir) + L"makecert.exe" + L"\"";
	cmdline += L" -n \"" + swIdentityPublisher + L"\" -r -a sha256 -len 2048 -cy end -h 0 -eku 1.3.6.1.5.5.7.3.3 -b 01/01/2000 -sv " +
		L"\"" + m_pvkfile + L"\" " +
		L"\"" + m_cerfile + L"\"";
	console.Execute (cmdline.c_str (), owner);
	bool setSuccess = false;
	while (console.IsProcessRunning ())
	{
		if (&outputConsole && outputConsole.output)
		{
			std::wstring buf = console.GetOutputTextW (240);
			if (buf.empty ()) Sleep (50);
			outputConsole.safeOutput (buf.c_str ());
		}
		{
			//Sleep (100);
			//HWND hrandom = console.GetRandomHWndFromCurrentProcess ();
			//if (IsWindowOwner (hrandom, owner) && IsWindowActived (hrandom)) continue;
			//if (!IsWindowOwner (hrandom, owner)) SetWindowOwner (hrandom, owner);
			//// SetWindowOwner (hrandom, owner);
			//if (IsIconic (hrandom)) ShowWindow (hrandom, SW_RESTORE);
			//if (IsWindowActived (hrandom)) SetWindowActived (hrandom);
		}
	}
	WaitForSingleObject (console.GetThreadHandle (), INFINITE);
	if (&outputConsole && outputConsole.output)
	{
		int cnt = 0;
		std::wstring buf = L"";
		do {
			buf = console.GetOutputTextW (240);
			if (buf.empty ()) Sleep (50);
			outputConsole.safeOutput (buf.c_str ());
			if (buf.empty () || buf.length () <= 0)
			{
				cnt ++;
			}
		} while (!buf.empty () && buf.length () > 0 && cnt > 2);
	}
	Sleep (100);
	std::wstring allout = console.GetAllOutputW ();
	bool res =
		(StrInclude (allout, L"Success", true) || StrInclude (allout, L"Succeed", true) || StrInclude (allout, L"Succeeded", true)) &&
		(!StrInclude (allout, L"Error", true) && !StrInclude (allout, L"Failed", true)) &&
		IsFileExists (m_cerfile) &&
		IsFileExists (m_pvkfile);
	if (&outCerFilePath) outCerFilePath = m_cerfile;
	if (&outPvkFilePath) outPvkFilePath = m_pvkfile;
	return res;
}
bool Pvk2Pfx (const std::wstring &swInputCer, const std::wstring &swInputPvk, const std::wstring &swOutputDir, const std::wstring &swOutputFileName, std::wstring &outPfxFileName, IODirection &outputConsole = IODirection (), HWND owner = NULL)
{
	ConsolePipe console;
	std::wstring m_kitdir = GetKitDirectory (), cmdline (L"");
	std::wstring m_pfxfile = EnsureTrailingSlash (ProcessEnvVars (swOutputDir)) + swOutputFileName + L".pfx";
	cmdline += L"\"" + EnsureTrailingSlash (m_kitdir) + L"pvk2pfx.exe" + L"\"";
	cmdline += std::wstring (L" -pvk ") + L"\"" + swInputPvk + L"\"" +
		L" -spc " + L"\"" + swInputCer + L"\"" +
		L" -pfx " + L"\"" + m_pfxfile + L"\"";
	console.Execute (cmdline.c_str (), owner);
	bool setSuccess = false;
	while (console.IsProcessRunning ())
	{
		if (&outputConsole && outputConsole.output)
		{
			std::wstring buf = console.GetOutputTextW (240);
			if (buf.empty ()) Sleep (50);
			outputConsole.safeOutput (buf.c_str ());
		}
		{
			Sleep (100);
			if (IsWindowOwner (console.GetRandomHWndFromCurrentProcess (), owner)) break;
			setSuccess = console.SetCurrentProgressWndOwner (owner);
			SetWindowState (console.GetRandomHWndFromCurrentProcess ());
			if (!setSuccess)
			{
				if (!setSuccess)
				{
					std::vector <HWND> &hres = FindWindowsByTitleBlurW (L"Enter Private Key Password");
					setSuccess = false;
					for (auto it : hres)
					{
						if (GetProcessHandleFromHwnd (it) == console.GetProcessHandle ())
						{
							if (IsWindowOwner (it, owner)) break;
							setSuccess = SetWindowOwner (it, owner);
							SetWindowState (it);
							break;
						}
					}
				}
			}
		}
	}
	WaitForSingleObject (console.GetThreadHandle (), INFINITE);
	if (&outputConsole && outputConsole.output)
	{
		std::wstring buf = L"";
		do {
			buf = console.GetOutputTextW (240);
			if (buf.empty ()) Sleep (50);
			outputConsole.safeOutput (buf.c_str ());
		} while (!buf.empty () && buf.length () > 0);
	}
	std::wstring allout = console.GetAllOutputW ();
	bool res = (LabelEqual (allout, L"") || !StrInclude (allout, L"ERROR", true) && !StrInclude (allout, L"Failed", true)) && IsFileExists (m_pfxfile);
	if (&outPfxFileName) outPfxFileName = m_pfxfile;
	return res;
}
bool SignTool (const std::wstring &swInputPkg, const std::wstring &swInputPfx, IODirection &outputConsole = IODirection ())
{
	ConsolePipe console;
	std::wstring m_kitdir = GetKitDirectory (), cmdline (L"");
	cmdline += L"\"" + EnsureTrailingSlash (m_kitdir) + L"signtool.exe" + L"\"";
	cmdline += std::wstring (L" sign -fd SHA256 -a -f ") + L"\"" + swInputPfx + L"\"" +
		L" " + L"\"" + swInputPkg + L"\"";
	console.Execute (cmdline.c_str ());
	while (console.IsProcessRunning ())
	{
		if (&outputConsole && outputConsole.output)
		{
			std::wstring buf = console.GetOutputTextW (240);
			if (buf.empty ()) Sleep (50);
			outputConsole.safeOutput (buf.c_str ());
		}
	}
	WaitForSingleObject (console.GetThreadHandle (), INFINITE);
	if (&outputConsole && outputConsole.output)
	{
		std::wstring buf = L"";
		do {
			buf = console.GetOutputTextW (240);
			if (buf.empty ()) Sleep (50);
			outputConsole.safeOutput (buf.c_str ());
		} while (!buf.empty () && buf.length () > 0);
	}
	std::wstring allout = console.GetAllOutputW ();
	bool res = StrInclude (allout, L"Successfully signed:", true);
	return res;
}

SYSTEMTIME GetSystemCurrentTime ()
{
	SYSTEMTIME st;
	GetLocalTime (&st);
	return st;
}
std::wstring GetTimestampForFileName (const std::wstring &format = L"%d%02d%02d%02d%02d%02d", const SYSTEMTIME &current = GetSystemCurrentTime ())
{
	WCHAR buf [128] = {0};
	swprintf (buf, format.c_str (),
		current.wYear, current.wMonth, current.wDay,
		current.wHour, current.wMinute, current.wSecond,
		current.wMilliseconds
	);
	return buf;
}