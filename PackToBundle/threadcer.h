#pragma once
#include <Windows.h>
class CriticalSection
{
	private:
	CRITICAL_SECTION m_csection;
	public:
	CriticalSection (DWORD spinCount = 4000) { InitializeCriticalSectionAndSpinCount (&m_csection, spinCount); }
	CriticalSection (const CriticalSection &) = delete;
	CriticalSection &operator = (const CriticalSection &) = delete;
	~CriticalSection () { DeleteCriticalSection (&m_csection); }
	void Lock () { EnterCriticalSection (&m_csection); }
	void Unlock () { LeaveCriticalSection (&m_csection); }
	bool TryLock () { return TryEnterCriticalSection (&m_csection) != 0; }
	class ScopedLock
	{
		public:
		explicit ScopedLock (CriticalSection &cs): m_cs (cs) { m_cs.Lock (); }
		~ScopedLock () { m_cs.Unlock (); }
		ScopedLock (const ScopedLock &) = delete;
		ScopedLock &operator = (const ScopedLock &) = delete;
		private:
		CriticalSection& m_cs;
	};
};

#define CreateScopedLock(_obj_cs_) CriticalSection::ScopedLock _obj_cs_ (_obj_cs_)

#ifdef __cplusplus_cli
ref struct TaskStructEvent
{
	typedef array <Object ^> args;
	typedef void (*eventfunc) (... args ^args);
	eventfunc post;
	args ^postargs;
	public:
	TaskStructEvent (
		eventfunc prefunc,
		args ^preargs,
		eventfunc postfunc,
		args ^postargs
	): post (postfunc), postargs (postargs)
	{
		if (prefunc == nullptr) {
		#pragma message("���棺Ԥ������ָ��Ϊ�գ�����������ʼ������")
		}
		if (prefunc)
		{
			if (preargs)
			{
			#pragma region ������֤ʾ��
				/*
				ʵ����Ŀ��Ӧ��Ӿ������ͼ�飬���磺
				ValidateArgsType<Button^>(preargs);
				*/
			#pragma endregion
				prefunc (preargs);
			}
			else prefunc (gcnew args {});
		}
	}
	~TaskStructEvent ()
	{
		if (post == nullptr)
		{
		#pragma message("���棺������ָ��Ϊ�գ���Դ�����޷���ȷ�ͷ�")
			return;
		}
		try
		{
			if (postargs) { post (postargs); }
			else { post (gcnew args {}); }
		}
		catch (Exception ^e)
		{
		#pragma message("ע�⣺�����е��쳣���ֶ�����")
		}
	}
};
#define CreateStructEvent(_varname_taskname_, _func_construct_, _args_construct_, _func_destruct_, _args_destruct_) \
TaskStructEvent _varname_taskname_ ( \
	_func_construct_, \
	_args_construct_, \
	_func_destruct_, \
	_args_destruct_ \
)
#endif

#ifdef __cplusplus
#include <functional>
#include <utility>
template <typename PreCallback, typename PostCallback> class ScopedEvent
{
	public:
	ScopedEvent (PreCallback &&pre, PostCallback &&post)
		: m_post (std::forward <PostCallback> (post))
	{
		static_assert (
			std::is_constructible <std::function <void ()>, PreCallback>::value,
			"Ԥ����ص������ת��Ϊ void () ����"
			);

		if (pre) { pre (); }
	}
	~ScopedEvent () noexcept
	{
		if (m_post) { m_post (); }
	}
	ScopedEvent (const ScopedEvent &) = delete;
	ScopedEvent &operator = (const ScopedEvent &) = delete;
	ScopedEvent (ScopedEvent &&) = default;
	ScopedEvent &operator =(ScopedEvent &&) = default;
	private:
	PostCallback m_post;
};
template <typename PreFunc, typename PostFunc> auto make_scoped_event (PreFunc &&pre, PostFunc &&post)
{
	return ScopedEvent <PreFunc, PostFunc> (
		std::forward <PreFunc> (pre),
		std::forward <PostFunc> (post)
		);
}
#endif