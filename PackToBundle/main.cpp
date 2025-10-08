#include <Windows.h>
#include <CommCtrl.h>
#include <vector>
#include <string>
#include <algorithm>
#include <type_traits>
#include <iterator>
#include <WinUser.h>
#include <type_traits>
#include <memory>
#include <pugiconfig.hpp>
#include <pugixml.hpp>
#include <ShlObj.h>
#include <thread>
#include <wincrypt.h>
#include <fmt/format.h>
#include "module.h"
#include "pkgread.h"
#include "pkgwrite.h"
#include "rctools.h"
#include "resource.h"
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
int GetDPI ()
{
	HDC hDC = GetDC (NULL);
	int DPI_A = (int)(((double)GetDeviceCaps (hDC, 118) / (double)GetDeviceCaps (hDC, 8)) * 100);
	int DPI_B = (int)(((double)GetDeviceCaps (hDC, 88) / 96) * 100);
	ReleaseDC (NULL, hDC);
	if (DPI_A == 100) return DPI_B;
	else if (DPI_B == 100) return DPI_A;
	else if (DPI_A == DPI_B) return DPI_A;
	else return 0;
}
template <typename t> t max (const t v1, const t v2) { if (v1 > v2) return v1; else return v2; }
template <typename t> t min (const t v1, const t v2) { if (v1 < v2) return v1; else return v2; }
class AutoFont
{
	public:
	static AutoFont Create (
		const wchar_t *faceName,
		int height,
		int weight = FW_NORMAL,
		bool italic = false,
		bool underline = false,
		bool strikeOut = false,
		DWORD charSet = DEFAULT_CHARSET
	)
	{
		LOGFONT lf = {0};
		const float scaling = GetDPI () * 0.01f;
		lf.lfHeight = -static_cast <LONG> (std::abs (height) * scaling);
		lf.lfWeight = weight;
		lf.lfItalic = italic;
		lf.lfUnderline = underline;
		lf.lfStrikeOut = strikeOut;
		lf.lfCharSet = charSet;
		wcscpy_s (lf.lfFaceName, faceName);
		HFONT hFont = ::CreateFontIndirect (&lf);
		if (!hFont)
		{
			throw std::system_error (
				std::error_code (::GetLastError (), std::system_category ()),
				"CreateFontIndirect failed"
			);
		}
		return AutoFont (hFont);
	}
	static AutoFont FromHandle (HFONT hFont, bool takeOwnership = false)
	{
		if (!hFont) return AutoFont (nullptr);
		if (takeOwnership) return AutoFont (hFont);
		else
		{
			LOGFONT lf;
			if (0 == ::GetObjectW (hFont, sizeof (LOGFONT), &lf))
			{
				throw std::system_error (
					std::error_code (::GetLastError (), std::system_category ()),
					"GetObject failed"
				);
			}
			HFONT newFont = ::CreateFontIndirect (&lf);
			if (!newFont)
			{
				throw std::system_error
				(
					std::error_code (::GetLastError (), std::system_category ()),
					"CreateFontIndirect failed"
				);
			}
			return AutoFont (newFont);
		}
	}
	explicit AutoFont (HFONT hFont = nullptr): m_hFont (hFont) {}
	~AutoFont () { Release (); }
	AutoFont (const AutoFont &) = delete;
	AutoFont &operator=(const AutoFont &) = delete;
	AutoFont (AutoFont &&other) noexcept : m_hFont (other.m_hFont)
	{
		other.m_hFont = nullptr;
	}
	AutoFont &operator = (AutoFont &&other) noexcept
	{
		if (this != &other)
		{
			Release ();
			m_hFont = other.m_hFont;
			other.m_hFont = nullptr;
		}
		return *this;
	}
	HFONT Get () const noexcept { return m_hFont; }
	void Release () noexcept
	{
		if (m_hFont)
		{
			::DeleteObject (m_hFont);
			m_hFont = nullptr;
		}
	}
	void Destroy () noexcept { Release (); }
	explicit operator bool () const noexcept { return m_hFont != nullptr; }
	private:
	HFONT m_hFont = nullptr;
};
void SetControlFont (AutoFont &font, HWND hWnd) { SendMessageW (hWnd, WM_SETFONT, (WPARAM)font.Get (), TRUE); }
#undef max
#undef min
static std::wstring lastfile = L"";
size_t ExploreFile (HWND hParent, std::vector <std::wstring> &results, LPWSTR lpFilter = L"Windows Store App Package (*.appx)\0*.appx", DWORD dwFlags = OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST, const std::wstring &swWndTitle = std::wstring (L"Please select the file(-s): "), const std::wstring &swInitDir = GetFileDirectoryW (lastfile))
{
	results.clear ();
	const DWORD BUFFER_SIZE = 65536; // 64KB
	std::vector <WCHAR> buffer (BUFFER_SIZE, 0);
	OPENFILENAME ofn;
	ZeroMemory (&ofn, sizeof (ofn));
	ofn.hwndOwner = hParent;
	ofn.lpstrFile = (LPWSTR)buffer.data ();
	ofn.nMaxFile = BUFFER_SIZE;
	ofn.lpstrFilter = lpFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrTitle = swWndTitle.c_str ();
	ofn.Flags = dwFlags;
	ofn.lpstrInitialDir = swInitDir.c_str ();
	ofn.lStructSize = sizeof (ofn);
	if (GetOpenFileNameW (&ofn))
	{
		LPCWSTR p = buffer.data ();
		std::wstring dir = p;
		p += dir.length () + 1;
		if (*p == 0) results.push_back (dir);
		else
		{
			while (*p)
			{
				std::wstring fullPath = dir + L"\\" + p;
				results.push_back (fullPath);
				p += wcslen (p) + 1;
			}
		}
		if (!results.empty ()) lastfile = results.back ();
	}
	return results.size ();
}
int CALLBACK BrowseCallbackProc (HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if (uMsg == BFFM_INITIALIZED)
	{
		SendMessageW (hwnd, BFFM_SETSELECTION, TRUE, lpData);
	}
	return 0;
}
static std::wstring lastdir = L"";
std::wstring ExploreDirectory (HWND hParent, UINT uFlag = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE, LPCWSTR lpWndTitle = L"Please select a folder: ")
{
	WCHAR path [MAX_PATH] = {0};
	BROWSEINFO bi = {0};
	bi.hwndOwner = hParent;
	bi.lpszTitle = lpWndTitle;
	bi.ulFlags = uFlag;
	bi.lpfn = BrowseCallbackProc;
	bi.lParam = (LPARAM)lastdir.c_str ();
	LPITEMIDLIST pidl = SHBrowseForFolderW (&bi);
	if (pidl)
	{
		SHGetPathFromIDListW (pidl, path);
		std::wstring res (L"");
		if (path) res += path;
		lastdir = res;
		CoTaskMemFree (pidl);
		return res;
	}
	return L"";
}
static std::wstring g_msgText;
static std::wstring g_msgTitle;
class ScopedHICON
{
	HICON hIcon = nullptr;
	public:
	ScopedHICON () = default;
	explicit ScopedHICON (HICON icon): hIcon (icon) {}
	ScopedHICON (const ScopedHICON&) = delete;
	ScopedHICON &operator = (const ScopedHICON&) = delete;
	ScopedHICON (ScopedHICON &&other) noexcept : hIcon (other.hIcon) { other.hIcon = nullptr; }
	ScopedHICON &operator = (ScopedHICON&& other) noexcept
	{
		if (this != &other)
		{
			reset ();
			hIcon = other.hIcon;
			other.hIcon = nullptr;
		}
		return *this;
	}
	~ScopedHICON () { reset (); }
	void reset (HICON newIcon = nullptr)
	{
		if (hIcon)
		{
			DestroyIcon (hIcon);
			hIcon = nullptr;
		}
		hIcon = newIcon;
	}
	HICON get () const { return hIcon; }
	operator HICON() const { return hIcon; }
	bool valid () const { return hIcon != nullptr; }
};
ScopedHICON g_hIcon;
INT_PTR CALLBACK MsgWndProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG: {
			if (g_hIcon.valid ())
			{
				SendMessageW (hDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hIcon.get ());
				SendMessageW (hDlg, WM_SETICON, ICON_SMALL, (LPARAM)g_hIcon.get ());
			}
			if (!g_msgTitle.empty ()) SetWindowTextW (hDlg, g_msgTitle.c_str ());
			HWND hEdit = GetDlgItem (hDlg, IDC_EDITDISPLAY);
			if (hEdit)
			{
				SendMessageW (hEdit, WM_SETTEXT, 0, (LPARAM)g_msgText.c_str ());
				SendMessageW (hEdit, EM_SETSEL, 0, 0);
				SendMessageW (hEdit, EM_SCROLLCARET, 0, 0);
			}
			return TRUE;
		} break;
		case WM_COMMAND: {
			switch (LOWORD (wParam))
			{
				case IDOK: {
					return EndDialog (hDlg, IDCANCEL);
				} break;
				case IDCANCEL: {
					return EndDialog (hDlg, IDCANCEL);
				}
			}
		} break;
	}
	return FALSE;
}
void MessageBoxLongStringW (HWND hDlg, const std::wstring &lpText = std::wstring (L""), const std::wstring &lpTitle = std::wstring (L""))
{
	g_msgTitle = lpTitle;
	g_msgText = lpText;
	DialogBoxW (NULL, MAKEINTRESOURCEW (IDD_DIALOGMSG), hDlg, MsgWndProc);
}
INT_PTR CALLBACK CfmWndProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG: {
			if (g_hIcon.valid ())
			{
				SendMessageW (hDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hIcon.get ());
				SendMessageW (hDlg, WM_SETICON, ICON_SMALL, (LPARAM)g_hIcon.get ());
			}
			if (!g_msgTitle.empty ()) SetWindowTextW (hDlg, g_msgTitle.c_str ());
			HWND hEdit = GetDlgItem (hDlg, IDC_EDITDISPLAY);
			if (hEdit)
			{
				SendMessageW (hEdit, WM_SETTEXT, 0, (LPARAM)g_msgText.c_str ());
				SendMessageW (hEdit, EM_SETSEL, 0, 0);
				SendMessageW (hEdit, EM_SCROLLCARET, 0, 0);
			}
			return TRUE;
		} break;
		case WM_COMMAND: {
			switch (LOWORD (wParam))
			{
				case IDOK: {
					return EndDialog (hDlg, true);
				} break;
				case IDCLOSE:
				case IDCANCEL: {
					return EndDialog (hDlg, false);
				} break;
			}
		} break;
	}
	return FALSE;
}
bool ConfirmBoxLongStringW (HWND hDlg, const std::wstring &lpText = std::wstring (L""), const std::wstring &lpTitle = std::wstring (L""))
{
	g_msgTitle = lpTitle;
	g_msgText = lpText;
	return DialogBoxW (NULL, MAKEINTRESOURCEW (IDD_DIALOGCONFIRM), hDlg, CfmWndProc);
}
BOOL IsCheckboxChecked (HWND hCheckbox)
{
	return (SendMessage (hCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED);
}
void SetCheckboxState (HWND hCheckbox, BOOL checked)
{
	SendMessage (hCheckbox, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}
std::wstring GetWindowTextW (HWND hWnd)
{
	int len = GetWindowTextLengthW (hWnd);
	if (len > 0)
	{
		auto buf = std::make_unique <WCHAR []> (len + 1);
		ZeroMemory (buf.get (), sizeof (WCHAR) * (len + 1));
		GetWindowTextW (hWnd, buf.get (), len + 1);
		return std::wstring () + buf.get ();
	}
	return L"";
}
ITaskbarList3 *g_pTaskbarList = NULL;
void InvokeSetWndEnable (HWND hParent, HWND hWnd, bool bEnable);
std::wstring InvokeGetWndText (HWND hParent, HWND hWnd);
void AppendTextToEdit (HWND hEdit, const std::wstring& text)
{
	// 获取当前文本长度
	LRESULT length = GetWindowTextLengthW (hEdit);
	if (length < 0) length = 0;

	// 获取编辑框的最大文本长度
	// EM_GETLIMITTEXT 返回控件允许的最大字符数
	LRESULT maxLen = SendMessageW (hEdit, EM_GETLIMITTEXT, 0, 0);
	if (maxLen <= 0) maxLen = 65535; // 默认上限（安全兜底）

									 // 如果超过上限，则删除前半部分旧内容
	if (length + (LRESULT)text.size () > maxLen)
	{
		// 删除一半旧文本（可调整策略）
		int removeCount = static_cast<int>(length / 2);
		SendMessageW (hEdit, EM_SETSEL, 0, removeCount);
		SendMessageW (hEdit, EM_REPLACESEL, FALSE, (LPARAM)L"");
		length = GetWindowTextLengthW (hEdit); // 更新长度
	}

	// 追加文本到末尾
	SendMessageW (hEdit, EM_SETSEL, (WPARAM)length, (LPARAM)length);
	SendMessageW (hEdit, EM_REPLACESEL, FALSE, (LPARAM)text.c_str ());
	SendMessageW (hEdit, EM_SCROLLCARET, 0, 0);
}
bool CheckTestPositiveInteger (const std::wstring &t)
{
	std::wstring trimed = std::wnstring::trim (t);
	for (auto &ch : trimed)
	{
		if (ch >= '0' && ch <= '9') continue;
		else return false;
	}
	return true;
}
std::wstring FormatTime (const std::wstring &fmt = L"HH:mm:ss", const SYSTEMTIME &st = GetSystemCurrentTime ())
{
	size_t size = GetTimeFormatW (LOCALE_USER_DEFAULT, 0, &st, NULL, NULL, 0);
	auto buf = std::make_unique <WCHAR []> (size + 1);
	GetTimeFormatW (LOCALE_USER_DEFAULT, 0, &st, NULL, buf.get (), size);
	return std::wstring () + buf.get ();
}
std::wstring FormatDate (const std::wstring &fmt = L"yyyy-MM-dd", const SYSTEMTIME &st = GetSystemCurrentTime ())
{
	size_t size = GetDateFormatW (LOCALE_USER_DEFAULT, 0, &st, NULL, NULL, 0);
	auto buf = std::make_unique <WCHAR []> (size + 1);
	GetDateFormatW (LOCALE_USER_DEFAULT, 0, &st, NULL, buf.get (), size);
	return std::wstring () + buf.get ();
}
#define WM_ENABLE_CONTROL (WM_APP + 1)
#define WM_GET_TEXT (WM_APP + 2)
#define WM_SET_TEXT (WM_APP + 3)
#define IDT_DEBOUNCE 1001
#define DEBOUNCE_DELAY 500  // 毫秒
void InvokeSetWndEnable (HWND hParent, HWND hWnd, bool bEnable)
{
	PostMessageW (hParent, WM_ENABLE_CONTROL, (WPARAM)hWnd, (LPARAM)bEnable);
}
std::wstring InvokeGetWndText (HWND hParent, HWND hWnd)
{
	LPWSTR wsptr = nullptr;
	raii endt ([&wsptr] () {
		if (wsptr) free (wsptr);
		wsptr = nullptr;
	});
	SendMessageW (hParent, WM_GET_TEXT, (WPARAM)hWnd, (LPARAM)&wsptr);
	return std::wstring (wsptr);
}
std::wstring Base64Encode (const std::wstring &input)  
{
	DWORD outputLength = 0;
	if (!CryptBinaryToStringW (
		(const BYTE*)input.data (),
		input.size (),
		CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
		NULL,
		&outputLength)) {
		return L"";
	}
	std::wstring output (outputLength, '\0');
	if (!CryptBinaryToStringW (
		(const BYTE*)input.data (),
		input.size (),
		CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
		&output [0],
		&outputLength)) {
		return L"";
	}
	if (!output.empty () && output.back () == '\0') output.pop_back ();
	return output;
}
std::wstring GetCurrentUserRemark ()
{
	std::wstring text = ProcessEnvVars (L"%UserProfile%");
	return Base64Encode (text);
}
static WInitFile g_inputsave (EnsureTrailingSlash (GetProgramRootDirectoryW ()) + L"inputsave.ini");
BOOL SetWindowTextW (HWND hWnd, const std::wstring &wstr)
{
	return SetWindowTextW (hWnd, wstr.c_str ());
}
std::wstring GetWindowTextWString (HWND hWnd)
{
	int len = GetWindowTextLengthW (hWnd);
	std::wstring text (len, L'\0');
	GetWindowTextW (hWnd, &text [0], len + 1);
	return text;
}
struct CopyProgressInfo
{
	LARGE_INTEGER TotalFileSize;
	LARGE_INTEGER TotalBytesTransferred;
	LARGE_INTEGER StreamSize;
	LARGE_INTEGER StreamBytesTransferred;
	DWORD dwStreamNumber;
	DWORD dwCallbackReason;
	HANDLE hSourceFile;
	HANDLE hDestinationFile;
};
using CopyProgressCallback = std::function<DWORD (const CopyProgressInfo &)>;
static DWORD CALLBACK CopyProgressBridge (LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred, LARGE_INTEGER StreamSize, LARGE_INTEGER StreamBytesTransferred, DWORD dwStreamNumber, DWORD dwCallbackReason, HANDLE hSourceFile, HANDLE hDestinationFile, LPVOID lpData)
{
	if (!lpData) return PROGRESS_CONTINUE;
	CopyProgressCallback *cb = reinterpret_cast <CopyProgressCallback *> (lpData);
	CopyProgressInfo info;
	info.TotalFileSize = TotalFileSize;
	info.TotalBytesTransferred = TotalBytesTransferred;
	info.StreamSize = StreamSize;
	info.StreamBytesTransferred = StreamBytesTransferred;
	info.dwStreamNumber = dwStreamNumber;
	info.dwCallbackReason = dwCallbackReason;
	info.hSourceFile = hSourceFile;
	info.hDestinationFile = hDestinationFile;
	try { return (*cb)(info); }
	catch (...) { return PROGRESS_CANCEL; }
}
bool CopyFileWithCallbackW (const std::wstring &src, const std::wstring &dst, const CopyProgressCallback &callback = nullptr, DWORD dwCopyFlags = 0)
{
	CopyProgressCallback cbCopy = callback;
	LPVOID lpData = callback ? (LPVOID)&cbCopy : nullptr;
	BOOL result = ::CopyFileExW (
		src.c_str (),
		dst.c_str (),
		callback ? CopyProgressBridge : nullptr,
		lpData,
		nullptr,
		dwCopyFlags
	);
	return result != FALSE;
}
CriticalSection g_cs;
void RunTask (HWND hDlg)
{
	CreateScopedLock (g_cs);
	HWND hWnds [] = {
		GetDlgItem (hDlg, IDC_INPUTPKG),
		GetDlgItem (hDlg, IDC_INPUTPKG_BROWSE),
		GetDlgItem (hDlg, IDC_OUTDIR_SRC),
		GetDlgItem (hDlg, IDC_OUTDIR_CUST),
		GetDlgItem (hDlg, IDC_CUSTOM_OUTDIR),
		GetDlgItem (hDlg, IDC_OUTDIR_BROWSE),
		GetDlgItem (hDlg, IDC_NAME_PFN),
		GetDlgItem (hDlg, IDC_NAME_CUSTOM),
		GetDlgItem (hDlg, IDC_NAME_SOURCE),
		GetDlgItem (hDlg, IDC_CUSTOM_NAME),
		GetDlgItem (hDlg, IDC_RUN)
	};
	for (auto &it : hWnds) EnableWindow (it, false);
	raii endt ([&hWnds] () {
		for (auto &it : hWnds) EnableWindow (it, true);
		EnableWindow (hWnds [4], IsCheckboxChecked (hWnds [3]));
		EnableWindow (hWnds [5], IsCheckboxChecked (hWnds [3]));
		EnableWindow (hWnds [9], IsCheckboxChecked (hWnds [7]));
	});
	HWND hConsole = GetDlgItem (hDlg, IDC_OUTPUT),
		hProgress = GetDlgItem (hDlg, IDC_PROGRESSBAR),
		hPercent = GetDlgItem (hDlg, IDC_PROGRESSDISPLAY),
		hStatus = GetDlgItem (hDlg, IDC_STATUSDISPLAY);
	IODirection io (
		nullptr,
		[&hConsole] (LPCWSTR c) -> int {
		AppendTextToEdit (hConsole, c);
		return 0;
	}, [&hConsole, &hStatus] (LPCWSTR c) -> int {
		AppendTextToEdit (hConsole, std::wstring (c) + L"\r\n");
		{
			std::wstring format = GetRCStringSW (IDS_STATUS);
			static HANDLE hHeap = HeapCreate (0, 0, 0);
			if (!hHeap) return -1;
			size_t len = format.length () + lstrlenW (c) + 4;
			WCHAR *buf = (WCHAR *)HeapAlloc (hHeap, HEAP_ZERO_MEMORY, sizeof (WCHAR) * len);
			raii freeBuf ([buf] () {
				if (buf) HeapFree (hHeap, 0, buf);
			});
			if (buf)
			{
				swprintf (buf, len, format.c_str (), std::wnstring (c).trim ().c_str ());
				SetWindowTextW (hStatus, buf);
			}
		}
		return 0;
	});
	auto progresscb = [&hDlg, &hProgress, &hPercent] (int progress) {
		g_pTaskbarList->SetProgressState (hDlg, TBPF_NORMAL);
		SendMessageW (hProgress, PBM_SETSTATE, PBST_NORMAL, 0);
		SendMessageW (hProgress, PBM_SETPOS, progress, 0);
		PBRANGE range = {0};
		SendMessage (hProgress, PBM_GETRANGE, TRUE, (LPARAM)&range);
		WCHAR buf [256] = {0};
		swprintf (buf, GetRCStringSW (IDS_PROGRESS).c_str (), (int)(progress / (double)range.iHigh * 100));
		g_pTaskbarList->SetProgressValue (hDlg, progress, range.iHigh);
		SetWindowTextW (hPercent, buf);
	};
	raii endtask ([&hDlg, &hWnds, &hConsole, &io] () {
		io.safeOutputLine (fmt::format (
			GetRCStringSW (IDS_TASK_FORMAT).c_str (),
			FormatDate (GetRCStringSW (IDS_DATE_FORMAT)).c_str (),
			FormatTime (GetRCStringSW (IDS_TIME_FORMAT)).c_str (),
			GetRCStringSW (IDS_TASK_END).c_str ()
		) + L"\r\n");
		g_pTaskbarList->SetProgressState (hDlg, TBPF_NOPROGRESS);
	});
	SetWindowLongPtr (hProgress, GWL_STYLE, (GetWindowLongPtr (hProgress, GWL_STYLE) & ~PBS_MARQUEE));
	g_pTaskbarList->SetProgressState (hDlg, TBPF_NORMAL);
	SendMessageW (hProgress, PBM_SETMARQUEE, FALSE, 0);
	SendMessageW (hProgress, PBM_SETSTATE, PBST_NORMAL, 0);
	SendMessageW (hProgress, PBM_SETRANGE, 0, MAKELPARAM (0, 103));
	SendMessageW (hProgress, PBM_SETPOS, 0, 0);
	io.safeOutputLine (fmt::format (
		GetRCStringSW (IDS_TASK_FORMAT).c_str (),
		FormatDate (GetRCStringSW (IDS_DATE_FORMAT)).c_str (),
		FormatTime (GetRCStringSW (IDS_TIME_FORMAT)).c_str (),
		GetRCStringSW (IDS_TASK_START).c_str ()
	));
	std::wstring filepath = GetWindowTextW (hWnds [0]);
	std::wstring publisher = L"";
	std::wstring pfn = L"";
	{
		package_info inf (filepath);
		if (std::wnstring::empty (filepath) || !IsFileExists (filepath) || !inf.is_valid)
		{
			std::wstring msg = GetRCStringSW (IDS_ERROR_PKG);
			io.safeOutputLine (msg);
			MessageBoxW (hDlg, msg.c_str (), GetRCStringSW (IDS_TITLE_ERROR).c_str (), MB_ICONERROR);
			return;
		}
		publisher = inf.identity.publisher;
		pfn = inf.identity.package_full_name;
	}
	enum class OutDirType { src, cust } outtype;
	enum class NameType { pfn, cust, src } nametype;
	std::wstring outdir = L"";
	std::wstring outname = L"";
	{
		if (!(IsCheckboxChecked (hWnds [2]) ^ IsCheckboxChecked (hWnds [3])))
		{
			std::wstring msg = GetRCStringSW (IDS_ERROR_OUTDIRTYPE);
			io.safeOutputLine (msg);
			MessageBoxW (hDlg, msg.c_str (), GetRCStringSW (IDS_TITLE_ERROR).c_str (), MB_ICONERROR);
			return;
		}
		else if (IsCheckboxChecked (hWnds [2]))
		{
			outtype = OutDirType::src;
			outdir = GetFileDirectoryW (filepath);
		}
		else if (IsCheckboxChecked (hWnds [3]))
		{
			outdir = ProcessEnvVars (GetWindowTextW (hWnds [4]));
		}
		if (!(IsCheckboxChecked (hWnds [6]) + IsCheckboxChecked (hWnds [7]) + IsCheckboxChecked (hWnds [8]) == 1))
		{
			std::wstring msg = GetRCStringSW (IDS_ERROR_NAMENAMED);
			io.safeOutputLine (msg);
			MessageBoxW (hDlg, msg.c_str (), GetRCStringSW (IDS_TITLE_ERROR).c_str (), MB_ICONERROR);
			return;
		}
		else if (IsCheckboxChecked (hWnds [6]))
		{
			nametype = NameType::pfn;
			outname = pfn;
		}
		else if (IsCheckboxChecked (hWnds [7]))
		{
			nametype = NameType::cust;
			outname = GetWindowTextW (hWnds [9]);
		}
		else if (IsCheckboxChecked (hWnds [8]))
		{
			nametype = NameType::src;
			LPWSTR lpFileName = PathFindFileNameW (filepath.c_str ());
			std::vector <WCHAR> buf (lstrlenW (lpFileName) + 1);
			lstrcpyW ((LPWSTR)buf.data (), lpFileName);
			PathRemoveExtensionW ((LPWSTR)buf.data ());
			outname = buf.data ();
		}
	}
	if (std::wnstring::empty (outdir) || !IsDirectoryExists (outdir) && !CreateDirectoryW (outdir.c_str (), 0))
	{
		std::wstring msg = GetRCStringSW (IDS_ERROR_DIRINVALID);
		io.safeOutputLine (msg);
		MessageBoxW (hDlg, msg.c_str (), GetRCStringSW (IDS_TITLE_ERROR).c_str (), MB_ICONERROR);
		return;
	}
	if (std::wnstring::empty (outname) || !IsValidWindowsName (outname))
	{
		std::wstring msg = GetRCStringSW (IDS_ERROR_NAMEINVALID);
		io.safeOutputLine (msg);
		MessageBoxW (hDlg, msg.c_str (), GetRCStringSW (IDS_TITLE_ERROR).c_str (), MB_ICONERROR);
		return;
	}
	std::wstring newdir = PathCombine (outdir, outname);
	if (IsDirectoryExists (newdir)) newdir = PathCombine (outdir, GetTimestampForFileName () + L"_" + outname);
	if (std::wnstring::empty (newdir) || !IsDirectoryExists (newdir) && !CreateDirectoryW (newdir.c_str (), 0))
	{
		std::wstring msg = GetRCStringSW (IDS_ERROR_DIRINVALID);
		io.safeOutputLine (msg);
		MessageBoxW (hDlg, msg.c_str (), GetRCStringSW (IDS_TITLE_ERROR).c_str (), MB_ICONERROR);
		return;
	}
	std::wstring outcer, outpvk, outpfx, outpkg;
	io.safeOutputLine (GetRCStringSW (IDS_PROGRESS_MAKECERT));
	bool res = MakeCert (publisher, newdir, outname, outcer, outpvk, io, hDlg);
	if (!res)
	{
		std::wstring msg = GetRCStringSW (IDS_ERROR_CERT);
		io.safeOutputLine (msg);
		MessageBoxW (hDlg, msg.c_str (), GetRCStringSW (IDS_TITLE_ERROR).c_str (), MB_ICONERROR);
		return;
	}
	progresscb (1);
	io.safeOutputLine (GetRCStringSW (IDS_PROGRESS_PVK2PFX));
	res = Pvk2Pfx (outcer, outpvk, newdir, outname, outpfx, io, hDlg);
	if (!res)
	{
		std::wstring msg = GetRCStringSW (IDS_ERROR_PFX);
		io.safeOutputLine (msg);
		MessageBoxW (hDlg, msg.c_str (), GetRCStringSW (IDS_TITLE_ERROR).c_str (), MB_ICONERROR);
		return;
	}
	progresscb (2);
	outpkg = PathCombineW (newdir, outname + PathFindExtensionW (filepath.c_str ()));
	io.safeOutputLine (GetRCStringSW (IDS_PROGRESS_COPY));
	res = CopyFileWithCallbackW (filepath, outpkg, [&progresscb] (const CopyProgressInfo &cpi) -> DWORD {
		double percentCompleted = (static_cast <double> (cpi.TotalBytesTransferred.QuadPart) / static_cast <double> (cpi.TotalFileSize.QuadPart)) * 100;
		progresscb ((int)(2 + percentCompleted));
		return PROGRESS_CONTINUE;
	});
	if (!res || !IsFileExists (outpkg))
	{
		std::wstring msg = GetRCStringSW (IDS_ERROR_COPY);
		io.safeOutputLine (msg);
		MessageBoxW (hDlg, msg.c_str (), GetRCStringSW (IDS_TITLE_ERROR).c_str (), MB_ICONERROR);
		return;
	}
	progresscb (102);
	io.safeOutputLine (GetRCStringSW (IDS_PROGRESS_SIGN));
	res = SignTool (outpkg, outpfx, io);
	if (!res)
	{
		std::wstring msg = GetRCStringSW (IDS_ERROR_SIGN);
		io.safeOutputLine (msg);
		MessageBoxW (hDlg, msg.c_str (), GetRCStringSW (IDS_TITLE_ERROR).c_str (), MB_ICONERROR);
		return;
	}
	progresscb (103);
	{
		std::wstring format = GetRCStringSW (IDS_SUCCESS);
		static HANDLE hHeap = HeapCreate (0, 0, 0);
		if (!hHeap)
		{
			io.outputLine (L"HeapCreate failed");
			return;
		}
		size_t bufLen = format.length () + outpkg.length () + 4;
		WCHAR *buf = (WCHAR *)HeapAlloc (hHeap, HEAP_ZERO_MEMORY, sizeof (WCHAR) * bufLen);
		raii freeBuf ([buf] () {
			if (buf) HeapFree (hHeap, 0, buf);
		});
		swprintf (buf, bufLen, format.c_str (), outpkg.c_str ());
		io.outputLine (buf);
	}
}
std::wnstring g_lastinputfile = L"";
INT_PTR CALLBACK WndProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_ENABLE_CONTROL: {
			HWND hCtrl = (HWND)wParam;
			BOOL enable = (BOOL)lParam;
			EnableWindow (hCtrl, enable);
			return 0;
		} break;
		case WM_GET_TEXT: {
			*(LPWSTR *)lParam = _wcsdup (GetWindowTextW ((HWND)wParam).c_str ());
			return 0;
		} break;
		case WM_INITDIALOG: {
			if (g_hIcon.valid ())
			{
				SendMessageW (hDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hIcon.get ());
				SendMessageW (hDlg, WM_SETICON, ICON_SMALL, (LPARAM)g_hIcon.get ());
			}
			{
				HWND hWnds [] = {
					GetDlgItem (hDlg, IDC_INPUTPKG),
					GetDlgItem (hDlg, IDC_INPUTPKG_BROWSE),
					GetDlgItem (hDlg, IDC_OUTDIR_SRC),
					GetDlgItem (hDlg, IDC_OUTDIR_CUST),
					GetDlgItem (hDlg, IDC_CUSTOM_OUTDIR),
					GetDlgItem (hDlg, IDC_OUTDIR_BROWSE),
					GetDlgItem (hDlg, IDC_NAME_PFN),
					GetDlgItem (hDlg, IDC_NAME_CUSTOM),
					GetDlgItem (hDlg, IDC_NAME_SOURCE),
					GetDlgItem (hDlg, IDC_CUSTOM_NAME),
					GetDlgItem (hDlg, IDC_RUN)
				};
				SetWindowTextW (hWnds [0], g_inputsave.readStringValue (GetCurrentUserRemark (), L"PackageWillResign"));
				unsigned dirmethod = g_inputsave.readUIntValue (GetCurrentUserRemark (), L"PackageOutputMethod");
				SetCheckboxState (hWnds [2], !dirmethod);
				SetCheckboxState (hWnds [3], dirmethod);
				SetWindowTextW (hWnds [4], g_inputsave.readStringValue (GetCurrentUserRemark (), L"CustomOutputDirPath"));
				unsigned namemethod = g_inputsave.readUIntValue (GetCurrentUserRemark (), L"PackageNameMethod");
				SetCheckboxState (hWnds [6], namemethod == 0);
				SetCheckboxState (hWnds [7], namemethod == 1);
				SetCheckboxState (hWnds [8], namemethod >= 2);
				SetWindowTextW (hWnds [9], g_inputsave.readStringValue (GetCurrentUserRemark (), L"CustomOutputName"));
			}
			HWND hEnableCustomName = GetDlgItem (hDlg, IDC_NAME_CUSTOM);
			HWND hCustomNameInput = GetDlgItem (hDlg, IDC_CUSTOM_NAME);
			EnableWindow (hCustomNameInput, IsCheckboxChecked (hEnableCustomName));
			HWND hProgress = GetDlgItem (hDlg, IDC_PROGRESSBAR);
			LONG_PTR style = GetWindowLongPtr (hProgress, GWL_STYLE);
			SetWindowLongPtr (hProgress, GWL_STYLE, style | PBS_MARQUEE);
			SendMessageW (hProgress, PBM_SETMARQUEE, TRUE, 50);
			HWND hEdit = GetDlgItem (hDlg, IDC_OUTPUT);
			SendMessageW (hEdit, EM_LIMITTEXT, (WPARAM)-1, 0);
			HWND hEnableCustomDir = GetDlgItem (hDlg, IDC_OUTDIR_CUST);
			HWND hCustomOutDir = GetDlgItem (hDlg, IDC_CUSTOM_OUTDIR);
			HWND hOutDirBrowse = GetDlgItem (hDlg, IDC_OUTDIR_BROWSE);
			EnableWindow (hCustomOutDir, IsCheckboxChecked (hEnableCustomDir));
			EnableWindow (hOutDirBrowse, IsCheckboxChecked (hEnableCustomDir));
			HWND hKitVersion = GetDlgItem (hDlg, IDC_VERSIONCHECK);
			std::wstringstream ss;
			ss << L"Check Kits: " << (CheckKits () ? L"true" : L"false") << std::endl
				<< L"MakeCert.exe: " << GetExeFileVersion (makecert_exe).stringifyw () << std::endl
				<< L"Pvk2Pfx.exe: " << GetExeFileVersion (pvk2pfx_exe).stringifyw () << std::endl
				<< L"SignTool.exe: " << GetExeFileVersion (signtool_exe).stringifyw () << std::endl;
			SetWindowTextW (hKitVersion, ss.str ().c_str ());
			return TRUE;
		} break;
		case WM_COMMAND: {
			switch (LOWORD (wParam))
			{
				case IDCANCEL: {
					return EndDialog (hDlg, IDCANCEL);
				} break;
				case IDC_NAME_SOURCE:
				case IDC_NAME_PFN:
				case IDC_NAME_CUSTOM: {
					HWND hEnableCustomName = GetDlgItem (hDlg, IDC_NAME_CUSTOM);
					HWND hCustomNameInput = GetDlgItem (hDlg, IDC_CUSTOM_NAME);
					EnableWindow (hCustomNameInput, IsCheckboxChecked (hEnableCustomName));
					return TRUE;
				} break;
				case IDC_OUTDIR_SRC:
				case IDC_OUTDIR_CUST: {
					HWND hEnableCustomDir = GetDlgItem (hDlg, IDC_OUTDIR_CUST);
					HWND hCustomOutDir = GetDlgItem (hDlg, IDC_CUSTOM_OUTDIR);
					HWND hOutDirBrowse = GetDlgItem (hDlg, IDC_OUTDIR_BROWSE);
					EnableWindow (hCustomOutDir, IsCheckboxChecked (hEnableCustomDir));
					EnableWindow (hOutDirBrowse, IsCheckboxChecked (hEnableCustomDir));
					return TRUE;
				} break;
				case IDC_OUTDIR_BROWSE: {
					std::wstring dir = ExploreDirectory (hDlg);
					if (!std::wnstring::empty (dir) && IsDirectoryExists (dir))
					{
						HWND hCustomOutDir = GetDlgItem (hDlg, IDC_CUSTOM_OUTDIR);
						SetWindowTextW (hCustomOutDir, dir);
					}
					return TRUE;
				} break;
				case IDC_INPUTPKG_BROWSE: {
					HWND hEdit = GetDlgItem (hDlg, IDC_INPUTPKG);
					std::vector <std::wstring> files;
					std::wstring filterdisplay = GetRCStringSW (IDS_DIALOG_PKGDESP);
					std::wstring filtertypes = L"*.appx;*.appxbundle;*.msix;*.msixbundle";
					std::vector <WCHAR> filter (filterdisplay.capacity () + filtertypes.capacity () + 4);
					lstrcpyW ((LPWSTR)filter.data (), filterdisplay.c_str ());
					strcpynull ((LPWSTR)filter.data (), filtertypes.c_str (), filter.size ());
					if (ExploreFile (hDlg, files, filter.data (), OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST, GetRCStringSW (IDS_DIALOG_SELECTPKG)))
					{
						if (!std::wnstring::empty (files [0]) && IsFileExists (files [0]))
						{
							SetWindowTextW (hEdit, files [0]);
						}
					}
					return TRUE;
				} break;
				case IDC_INPUTPKG: {
					switch (HIWORD (wParam))
					{
						case EN_CHANGE: {
							KillTimer (hDlg, IDT_DEBOUNCE);
							SetTimer (hDlg, IDT_DEBOUNCE, DEBOUNCE_DELAY, NULL);
							return TRUE;
						} break;
					}
					return FALSE;
				} break;
				case IDC_RUN: {
					std::thread thread (RunTask, hDlg);
					thread.detach ();
					return TRUE;
				} break;
			}
		} break;
		case WM_DESTROY: {
			{
				HWND hWnds [] = {
					GetDlgItem (hDlg, IDC_INPUTPKG),
					GetDlgItem (hDlg, IDC_INPUTPKG_BROWSE),
					GetDlgItem (hDlg, IDC_OUTDIR_SRC),
					GetDlgItem (hDlg, IDC_OUTDIR_CUST),
					GetDlgItem (hDlg, IDC_CUSTOM_OUTDIR),
					GetDlgItem (hDlg, IDC_OUTDIR_BROWSE),
					GetDlgItem (hDlg, IDC_NAME_PFN),
					GetDlgItem (hDlg, IDC_NAME_CUSTOM),
					GetDlgItem (hDlg, IDC_NAME_SOURCE),
					GetDlgItem (hDlg, IDC_CUSTOM_NAME),
					GetDlgItem (hDlg, IDC_RUN)
				};
				g_inputsave.writeStringValue (GetCurrentUserRemark (), L"PackageWillResign", GetWindowTextW (hWnds [0]));
				unsigned dirmethod = IsCheckboxChecked (hWnds [3]) ? 1 : 0;
				g_inputsave.writeUIntValue (GetCurrentUserRemark (), L"PackageOutputMethod", dirmethod);
				g_inputsave.writeStringValue (GetCurrentUserRemark (), L"CustomOutputDirPath", GetWindowTextW (hWnds [4]));
				unsigned namemethod = 0;
				if (IsCheckboxChecked (hWnds [6])) namemethod = 0;
				else if (IsCheckboxChecked (hWnds [7])) namemethod = 1;
				else if (IsCheckboxChecked (hWnds [8])) namemethod = 2;
				g_inputsave.writeUIntValue (GetCurrentUserRemark (), L"PackageNameMethod", namemethod);
				g_inputsave.writeStringValue (GetCurrentUserRemark (), L"CustomOutputName", GetWindowTextW (hWnds [9]));
			}
			return TRUE;
		} break;
		case WM_TIMER:
		{
			if (wParam == IDT_DEBOUNCE)
			{
				KillTimer (hDlg, IDT_DEBOUNCE);

				HWND hEdit = GetDlgItem (hDlg, IDC_INPUTPKG);
				std::wstring filepath = GetWindowTextWString (hEdit);
				if (g_lastinputfile == filepath) return 0;
				else g_lastinputfile = filepath;
				HWND hIdentity [] = {
					GetDlgItem (hDlg, IDC_PKGIDNAME),
					GetDlgItem (hDlg, IDC_PKGIDPUBLISHER),
					GetDlgItem (hDlg, IDC_PKGIDVERSION),
					GetDlgItem (hDlg, IDC_PKGIDARCHI)
				};
				for (auto &h : hIdentity)
					SetWindowTextW (h, L"");

				if (!std::nstring::empty (filepath) && IsFileExists (filepath))
				{
					package_info inf (filepath);
					SetWindowTextW (hIdentity [0], inf.identity.name);
					SetWindowTextW (hIdentity [1], inf.identity.publisher);
					SetWindowTextW (hIdentity [2], inf.identity.version.stringifyw ());

					std::wstring archi;
					switch (inf.identity.architecture)
					{
						case APPX_PACKAGE_ARCHITECTURE_ARM: archi = L"Arm"; break;
						case APPX_PACKAGE_ARCHITECTURE_NEUTRAL: archi = L"Neutral"; break;
						case APPX_PACKAGE_ARCHITECTURE_X64: archi = L"x64"; break;
						case APPX_PACKAGE_ARCHITECTURE_X86: archi = L"x86"; break;
						case (APPX_PACKAGE_ARCHITECTURE)12: archi = L"Arm64"; break;
						default: archi = L"";
					}
					SetWindowTextW (hIdentity [3], archi);
				}
			}
			return 0;
		}
	}
	return FALSE;
}
int APIENTRY wWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	raii relp ([] () {
		if (g_pTaskbarList)
		{
			g_pTaskbarList->Release ();
			g_pTaskbarList = NULL;
		}
	});
	SetupInstanceEnvironment ();
	g_hIcon = ScopedHICON (LoadIconW (hInstance, MAKEINTRESOURCE (IDI_ICONMAIN)));
	CoInitializeEx (NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	OleInitialize (NULL);
	CoCreateInstance (CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS (&g_pTaskbarList));
	DialogBoxW (hInstance, MAKEINTRESOURCEW (IDD_DIALOGMAIN), NULL, WndProc);
	if (g_pTaskbarList)
	{
		g_pTaskbarList->Release ();
		g_pTaskbarList = NULL;
	}
	CoUninitialize ();
	return 0;
}