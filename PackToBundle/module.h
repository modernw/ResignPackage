#pragma once
#include <Windows.h>
#ifdef __cplusplus
#ifndef GetCurrentModule_bRefDefault 
// �� C++ �У�GetCurrentModule ��������Ĭ��ֵ���������֮ǰ�궨���Ĭ��ֵ�������ʱ�����˵Ⱥš�=��
// �÷��磺HMODULE GetCurrentModule (BOOL bRef GetCurrentModule_bRefDefault)
#define GetCurrentModule_bRefDefault = FALSE
#endif
#else
#define GetCurrentModule_bRefDefault
#endif
HMODULE GetCurrentModule (BOOL bRef GetCurrentModule_bRefDefault)
{
	HMODULE hModule = NULL;
	if (GetModuleHandleExW (bRef ? GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS : (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
		| GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT), (LPCWSTR)GetCurrentModule, &hModule))
	{
		return hModule;
	}
	return NULL;
}
HMODULE GetSelfModuleHandle ()
{
	MEMORY_BASIC_INFORMATION mbi;
	return ((::VirtualQuery (GetSelfModuleHandle, &mbi, sizeof (mbi)) != 0)
		? (HMODULE)mbi.AllocationBase : NULL);
}
#ifndef GetModuleHandleW_lpModuleNameDefault 
#define GetModuleHandleW_lpModuleNameDefault NULL
#endif
#ifndef DEFAULT_HMODULE
#ifdef HMODULE_MODE_EXE
#define DEFAULT_HMODULE NULL
#elif defined (HMODULE_MODE_DLL1)
#define DEFAULT_HMODULE GetCurrentModule ()
#elif defined (HMODULE_MODE_DLL2)
#define DEFAULT_HMODULE GetSelfModuleHandle ()
#else
#define DEFAULT_HMODULE GetModuleHandleW (GetModuleHandleW_lpModuleNameDefault)
#endif
#endif
#undef GetModuleHandleW_lpModuleNameDefault
#ifdef __cplusplus
#ifndef hModule_DefaultParam 
// �� C++ �У������ʹ�ô˺ꡰhModule_DefaultParam�������ڸ�һЩ�������βζ���Ĭ��ֵ���������֮ǰ�궨���Ĭ��ֵ�������ʱ�����˵Ⱥš�=��
// �÷��磺std::wstring GetRCStringSW (UINT resID, HMODULE hModule hModule_DefaultParam)��
#define hModule_DefaultParam = DEFAULT_HMODULE
#endif
#else
#define hModule_DefaultParam
#endif