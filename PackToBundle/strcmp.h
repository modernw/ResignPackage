#pragma once
#include <Shlwapi.h>
#include "norstr.h"
typedef char CHAR;
typedef wchar_t WCHAR;
typedef CHAR *LPSTR;
typedef WCHAR *LPWSTR;
typedef const CHAR *LPCSTR;
typedef const WCHAR *LPCWSTR;
#ifdef __cplusplus
#define ptrnull(ptr) (!(ptr))
#else
#define ptrnull(ptr) ((ptr) == NULL)
#endif
#define ptrvalid(ptr) (!ptrnull (ptr))
// ���� char * �� WCHAR * �ַ�������βΪ NULL�����ж��Ƿ�Ϊ�ǿ��ַ�����ָ����Ч�ҳ��ȴ��� 0��ǧ������Ұָ�룬����һ���������
#define strvalid(strptr) (ptrvalid (strptr) && *(strptr))
// ���� char * �� WCHAR * �ַ�������βΪ NULL�����ж��Ƿ�Ϊ���ַ�����ָ��Ϊ NULL �򳤶�Ϊ 0��ǧ������Ұָ�룬����һ���������
#define strnull(strptr) (ptrnull (strptr) || !*(strptr))
typedef std::wnstring strlabel, StringLabel;
std::wstring StringTrim (const std::wstring &str) { return std::wnstring::trim (str); }
std::string StringTrim (const std::string &str) { return std::nstring::trim (str); }
#define StringToUpper l0km::toupper
#define StringToLower l0km::tolower
int LabelCompare (const std::wstring &l1, const std::wstring &l2)
{
	return std::wnstring::compare (l1, l2);
}
int LabelCompare (const std::string &l1, const std::string &l2)
{
	return std::nstring::compare (l1, l2);
}
bool LabelEqual (const std::wstring &l1, const std::wstring &l2)
{
	return std::wnstring::equals (l1, l2);
}
bool LabelEqual (const std::string &l1, const std::string &l2)
{
	return std::wnstring::equals (l1, l2);
}
bool LabelEmpty (const std::wstring &str) { return std::wnstring::empty (str); }
bool LabelEmpty (const std::string &str) { return std::nstring::empty (str); }
#define LabelNoEmpty(_str_) (!LabelEmpty (_str_))
int InStr (const std::string &text, const std::string &keyword, bool ignoreCase = false)
{
	std::string s1, s2;
	if (ignoreCase)
	{
		s1 = StringToUpper (text);
		s2 = StringToUpper (keyword);
	}
	else
	{
		s1 = text;
		s2 = keyword;
	}
	const char *found = StrStrIA (s1.c_str (), s2.c_str ());
	if (!found)
	{
		return -1;
	}
	return found - text.c_str ();
}
int InStr (const std::wstring &text, const std::wstring &keyword, bool ignoreCase = false)
{
	std::wstring s1, s2;
	if (ignoreCase)
	{
		s1 = StringToUpper (text);
		s2 = StringToUpper (keyword);
	}
	else
	{
		s1 = text;
		s2 = keyword;
	}
	const WCHAR *found = StrStrIW (s1.c_str (), s2.c_str ());
	if (!found)
	{
		return -1;
	}
	return found - text.c_str ();
}
bool StrInclude (const std::string &text, const std::string &keyword, bool ignoreCase = false)
{
	std::string s1, s2;
	if (ignoreCase)
	{
		s1 = StringToUpper (text);
		s2 = StringToUpper (keyword);
	}
	else
	{
		s1 = text;
		s2 = keyword;
	}
	const char *found = StrStrIA (s1.c_str (), s2.c_str ());
	if (!found) return false;
	return true;
}
bool StrInclude (const std::wstring &text, const std::wstring &keyword, bool ignoreCase = false)
{
	std::wstring s1, s2;
	if (ignoreCase)
	{
		s1 = StringToUpper (text);
		s2 = StringToUpper (keyword);
	}
	else
	{
		s1 = text;
		s2 = keyword;
	}
	const WCHAR *found = StrStrIW (s1.c_str (), s2.c_str ());
	if (!found) return false;
	return true;
}
// �ú����������� "<str1>\0<str2>\0" �����ַ���������ͨ�öԻ����е��ļ���
LPCWSTR strcpynull (LPWSTR dest, LPCWSTR endwith, size_t bufsize)
{
	if (!dest || !endwith || bufsize == 0)
		return dest;
	if (dest [0] == L'\0' && bufsize > 1)
	{
		dest [1] = L'\0';
	}
	size_t pos = 0;
	while (pos < bufsize - 1)
	{
		if (dest [pos] == L'\0' && dest [pos + 1] == L'\0')
		{
			if (dest [0]) pos ++;
			break;
		}
		pos ++;
	}
	size_t i = 0;
	while (pos < bufsize - 1 && endwith [i] != L'\0')
	{
		dest [pos ++] = endwith [i ++];
	}
	if (pos < bufsize)
	{
		dest [pos] = L'\0';
	}
	return dest;
}