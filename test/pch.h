#ifndef PCH_H
#define PCH_H

/************************************************************************/
/* The MutexGear Library                                                */
/* The Library Test Application Precompiled Header File                 */
/*                                                                      */
/* Copyright (c) 2016-2025 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/


#ifdef _WIN32
#define _WIN32_WINNT 0x0600
#include <Windows.h>
#endif

#include <mutexgear/utility.h>
#include <mutexgear/config.h>


#include <algorithm>
using std::min;
using std::max;

#include <stdio.h>
#include <string.h>


//////////////////////////////////////////////////////////////////////////
// Error codes

#include <errno.h>

#ifndef EOK
#define EOK 0
#endif // #ifndef EOK


//////////////////////////////////////////////////////////////////////////
// Additional atomics

#ifdef _MUTEXGEAR_HAVE_CXX11_ATOMICS

#include <atomic>


#define __MUTEXGEAR_ATOMIC_UINT_T std::atomic<unsigned int>

#define __MUTEGEAR_ATOMIC_INIT_UINT(destination, value) (*(destination) = (unsigned int)(value))
#define __MUTEGEAR_ATOMIC_LOAD_RELAXED_UINT(source) ((source)->load(std::memory_order_relaxed))
#define __MUTEGEAR_ATOMIC_FETCH_ADD_RELAXED_UINT(destination, value) ((destination)->fetch_add((unsigned int)(value), std::memory_order_relaxed))
#define __MUTEGEAR_ATOMIC_FETCH_SUB_RELAXED_UINT(destination, value) ((destination)->fetch_sub((unsigned int)(value), std::memory_order_relaxed))


#else // #ifndef _MUTEXGEAR_HAVE_CXX11_ATOMICS

#ifndef __MUTEXGEAR_ATOMIC_UINT_T
#error Please define __MUTEXGEAR_ATOMIC_UINT_T
#endif

#ifndef __MUTEGEAR_ATOMIC_INIT_UINT
#error Please define __MUTEGEAR_ATOMIC_INIT_UINT
#endif

#ifndef __MUTEGEAR_ATOMIC_LOAD_RELAXED_UINT
#error Please define __MUTEGEAR_ATOMIC_LOAD_RELAXED_UINT
#endif

#ifndef __MUTEGEAR_ATOMIC_FETCH_ADD_RELAXED_UINT
#error Please define __MUTEGEAR_ATOMIC_FETCH_ADD_RELAXED_UINT
#endif

#ifndef __MUTEGEAR_ATOMIC_FETCH_SUB_RELAXED_UINT
#error Please define __MUTEGEAR_ATOMIC_FETCH_SUB_RELAXED_UINT
#endif


#endif // #ifndef _MUTEXGEAR_HAVE_CXX11_ATOMICS


typedef __MUTEXGEAR_ATOMIC_UINT_T _mg_atomic_uint_t;

_MUTEXGEAR_PURE_INLINE
void _mg_atomic_init_uint(volatile _mg_atomic_uint_t *__destination, unsigned int __value1)
{
	__MUTEGEAR_ATOMIC_INIT_UINT(__destination, __value1);
}

_MUTEXGEAR_PURE_INLINE
unsigned int _mg_atomic_load_relaxed_uint(const volatile _mg_atomic_uint_t *__source)
{
	return __MUTEGEAR_ATOMIC_LOAD_RELAXED_UINT(__source);
}

_MUTEXGEAR_PURE_INLINE
unsigned int _mg_atomic_fetch_add_relaxed_uint(volatile _mg_atomic_uint_t *__destination, unsigned int __value1)
{
	return __MUTEGEAR_ATOMIC_FETCH_ADD_RELAXED_UINT(__destination, __value1);
}

_MUTEXGEAR_PURE_INLINE
unsigned int _mg_atomic_fetch_sub_relaxed_uint(volatile _mg_atomic_uint_t *__destination, unsigned int __value1)
{
	return __MUTEGEAR_ATOMIC_FETCH_SUB_RELAXED_UINT(__destination, __value1);
}


//////////////////////////////////////////////////////////////////////////
// System RWLock Implementation

#ifdef _WIN32
#define _MG_RWLOCK_T SRWLOCK
#else
#define _MG_RWLOCK_T pthread_rwlock_t
#endif


#ifdef _WIN32

_MUTEXGEAR_PURE_INLINE
int _mutexgear_rwlock_init(_MG_RWLOCK_T *__rwlock)
{
	InitializeSRWLock(__rwlock);
	return EOK;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_rwlock_destroy(_MG_RWLOCK_T *__rwlock)
{
	return EOK;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_rwlock_rdlock(_MG_RWLOCK_T *__rwlock)
{
	AcquireSRWLockShared(__rwlock);
	return EOK;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_rwlock_tryrdlock(_MG_RWLOCK_T *__rwlock)
{
	return TryAcquireSRWLockShared(__rwlock) ? EOK : EBUSY;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_rwlock_rdunlock(_MG_RWLOCK_T *__rwlock)
{
	ReleaseSRWLockShared(__rwlock);
	return EOK;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_rwlock_wrlock(_MG_RWLOCK_T *__rwlock)
{
	AcquireSRWLockExclusive(__rwlock);
	return EOK;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_rwlock_trywrlock(_MG_RWLOCK_T *__rwlock)
{
	return TryAcquireSRWLockExclusive(__rwlock) ? EOK : EBUSY;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_rwlock_wrunlock(_MG_RWLOCK_T *__rwlock)
{
	ReleaseSRWLockExclusive(__rwlock);
	return EOK;
}


#else // #ifndef _WIN32

#if defined(_MUTEXGEAR_HAVE_PTHREAD_H)
#include <pthread.h>
#endif // #if defined(_MUTEXGEAR_HAVE_PTHREAD_H)


_MUTEXGEAR_PURE_INLINE
int _mutexgear_rwlock_init(_MG_RWLOCK_T *__rwlock)
{
	return pthread_rwlock_init(__rwlock, NULL);
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_rwlock_destroy(_MG_RWLOCK_T *__rwlock)
{
	return pthread_rwlock_destroy(__rwlock);
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_rwlock_rdlock(_MG_RWLOCK_T *__rwlock)
{
	return pthread_rwlock_rdlock(__rwlock);
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_rwlock_tryrdlock(_MG_RWLOCK_T *__rwlock)
{
	return pthread_rwlock_tryrdlock(__rwlock);
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_rwlock_rdunlock(_MG_RWLOCK_T *__rwlock)
{
	return pthread_rwlock_unlock(__rwlock);
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_rwlock_wrlock(_MG_RWLOCK_T *__rwlock)
{
	return pthread_rwlock_wrlock(__rwlock);
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_rwlock_trywrlock(_MG_RWLOCK_T *__rwlock)
{
	return pthread_rwlock_trywrlock(__rwlock);
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_rwlock_wrunlock(_MG_RWLOCK_T *__rwlock)
{
	return pthread_rwlock_unlock(__rwlock);
}


#endif // #ifndef _WIN32


//////////////////////////////////////////////////////////////////////////
// System Manual Event Implementation

#ifdef _WIN32
#define _MG_MANUALEVENT_T HANDLE
#else
#define _MG_MANUALEVENT_T _mutexgear_manualevent_t
#endif


#ifdef _WIN32

_MUTEXGEAR_PURE_INLINE
int _mutexgear_manualevent_init(_MG_MANUALEVENT_T *__event_instance)
{
	*__event_instance = CreateEvent(NULL, true, false, NULL);
	return *__event_instance != INVALID_HANDLE_VALUE ? EOK : GetLastError();
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_manualevent_destroy(_MG_MANUALEVENT_T *__event_instance)
{
	BOOL bCloseResult = CloseHandle(*__event_instance);
	return bCloseResult != FALSE ? EOK : GetLastError();
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_manualevent_set(_MG_MANUALEVENT_T *__event_instance)
{
	BOOL bSetResult = SetEvent(*__event_instance);
	return bSetResult != FALSE ? EOK : GetLastError();
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_manualevent_reset(_MG_MANUALEVENT_T *__event_instance)
{
	BOOL bResetResult = ResetEvent(*__event_instance);
	return bResetResult != FALSE ? EOK : GetLastError();
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_manualevent_wait(_MG_MANUALEVENT_T *__event_instance)
{
	DWORD dwWaitResult = WaitForSingleObject(*__event_instance, INFINITE);
	return dwWaitResult == WAIT_OBJECT_0 ? EOK : GetLastError();
}


#else // #ifndef _WIN32

#if defined(_MUTEXGEAR_HAVE_PTHREAD_H)
#include <pthread.h>
#endif // #if defined(_MUTEXGEAR_HAVE_PTHREAD_H)


typedef struct __mutexgear_manualevent
{
	pthread_mutex_t				m_pmEventMutex;
	pthread_cond_t				m_pcEventCondition;
	bool						m_bEventState;

} _mutexgear_manualevent_t;


_MUTEXGEAR_PURE_INLINE
int _mutexgear_manualevent_init(_MG_MANUALEVENT_T *__event_instance)
{
	bool bResult = false;
	int iResult;

	bool bMutexInitialized = false;

	do
	{
		if ((iResult = pthread_mutex_init(&__event_instance->m_pmEventMutex, NULL)) != EOK)
		{
			break;
		}
		bMutexInitialized = true;
		
		if ((iResult = pthread_cond_init(&__event_instance->m_pcEventCondition, NULL)) != EOK)
		{
			break;
		}

		__event_instance->m_bEventState = false;

		bResult = true;
	}
	while (false);

	if (!bResult)
	{
		if (bMutexInitialized)
		{
			int iMutexDestroyResult;
			MG_CHECK(iMutexDestroyResult, (iMutexDestroyResult = pthread_mutex_destroy(&__event_instance->m_pmEventMutex)) == EOK);
		}
	}

	return iResult;
}


_MUTEXGEAR_PURE_INLINE
int _mutexgear_manualevent_destroy(_MG_MANUALEVENT_T *__event_instance)
{
	// bool bResult = false;
	int iResult;

	do
	{
		if ((iResult = pthread_cond_destroy(&__event_instance->m_pcEventCondition)) != EOK)
		{
			break;
		}
		
		int iDestroyResult;
		MG_CHECK(iDestroyResult, (iDestroyResult = pthread_mutex_destroy(&__event_instance->m_pmEventMutex)) == EOK);

		// bResult = true;
	}
	while (false);
	
	return iResult;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_manualevent_set(_MG_MANUALEVENT_T *__event_instance)
{
	bool bResult = false;
	int iResult;

	bool bMutexLocked = false;
	
	do
	{
		if ((iResult = pthread_mutex_lock(&__event_instance->m_pmEventMutex)) != EOK)
		{
			break;
		}
		bMutexLocked = true;

		__event_instance->m_bEventState = true;
		
		if ((iResult = pthread_cond_broadcast(&__event_instance->m_pcEventCondition)) != EOK)
		{
			break;
		}

		int iMutexUnlockResult;
		MG_CHECK(iMutexUnlockResult, (iMutexUnlockResult = pthread_mutex_unlock(&__event_instance->m_pmEventMutex)) == EOK);

		bResult = true;
	}
	while (false);

	if (!bResult)
	{
		if (bMutexLocked)
		{
			int iMutexUnlockResult;
			MG_CHECK(iMutexUnlockResult, (iMutexUnlockResult = pthread_mutex_unlock(&__event_instance->m_pmEventMutex)) == EOK);
		}
	}
	
	return iResult;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_manualevent_reset(_MG_MANUALEVENT_T *__event_instance)
{
	// bool bResult = false;
	int iResult;

	do
	{
		if ((iResult = pthread_mutex_lock(&__event_instance->m_pmEventMutex)) != EOK)
		{
			break;
		}

		__event_instance->m_bEventState = false;

		int iMutexUnlockResult;
		MG_CHECK(iMutexUnlockResult, (iMutexUnlockResult = pthread_mutex_unlock(&__event_instance->m_pmEventMutex)) == EOK);

		// bResult = true;
	}
	while (false);

	return iResult;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_manualevent_wait(_MG_MANUALEVENT_T *__event_instance)
{
	bool bResult = false;
	int iResult;

	bool bMutexLocked = false;

	do
	{
		if ((iResult = pthread_mutex_lock(&__event_instance->m_pmEventMutex)) != EOK)
		{
			break;
		}
		bMutexLocked = true;

		while (!__event_instance->m_bEventState)
		{
			if ((iResult = pthread_cond_wait(&__event_instance->m_pcEventCondition, &__event_instance->m_pmEventMutex)) == EOK)
			{
				break;
			}

			if (iResult != EINTR)
			{
				break;
			}
		}
		if (iResult != EOK)
		{
			break;
		}

		int iMutexUnlockResult;
		MG_CHECK(iMutexUnlockResult, (iMutexUnlockResult = pthread_mutex_unlock(&__event_instance->m_pmEventMutex)) == EOK);

		bResult = true;
	}
	while (false);

	if (!bResult)
	{
		if (bMutexLocked)
		{
			int iMutexUnlockResult;
			MG_CHECK(iMutexUnlockResult, (iMutexUnlockResult = pthread_mutex_unlock(&__event_instance->m_pmEventMutex)) == EOK);
		}
	}

	return iResult;
}


#endif // #ifndef _WIN32


//////////////////////////////////////////////////////////////////////////
// Time Functions

#ifdef _WIN32

_MUTEXGEAR_PURE_INLINE
int _mutexgear_sleep(uint64_t __timeout)
{
	Sleep((DWORD)(__timeout / 1000000));
	return 0;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_monotonic_clock_time(uint64_t *__out_timepoint)
{
	FILETIME sys_time;
	GetSystemTimeAsFileTime(&sys_time);
	
	*__out_timepoint = (sys_time.dwLowDateTime | ((uint64_t)sys_time.dwHighDateTime << 32)) * 100;
	return 0;
}

#else // #ifndef _WIN32

#include <time.h>

_MUTEXGEAR_PURE_INLINE
int _mutexgear_sleep(uint64_t __timeout)
{
	int ret;

	struct timespec sleep_request, sleep_remaining;
	sleep_request.tv_sec = __timeout / 1000000000;
	sleep_request.tv_nsec = __timeout % 1000000000;

	while ((ret = nanosleep(&sleep_request, &sleep_remaining)) != 0 && errno == EINTR)
	{
		sleep_request = sleep_remaining;
	}

	return ret;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_monotonic_clock_time(uint64_t *__out_timepoint)
{
#if defined(_MUTEXGEAR_HAVE_CLOCK_GETTIME)
	struct timespec clock_time;
	int ret = clock_gettime(CLOCK_MONOTONIC, &clock_time);
	return ret == 0 ? (*__out_timepoint = (uint64_t)clock_time.tv_sec * 1000000000 + clock_time.tv_nsec, 0) : -1;
#elif defined(_MUTEXGEAR_HAVE_GETTIMEOFDAY)
    struct timeval tv;
    return gettimeofday(&tv, NULL) == 0 ? (ts->tv_sec = tv.tv_sec, ts->tv_nsec = tv.tv_usec * 1000, 0) : (-1);
#else
#error Clock time retrieval function implementation is needed
#endif
}


#endif // #ifndef _WIN32


#ifndef _WIN32

#ifdef _MUTEXGEAR_HAVE_SCHED_SETSCHEDULER

#include <sched.h>
#define _MGTEST_POSIX_ADJUST_SCHED


#endif // #ifdef _MUTEXGEAR_HAVE_SCHED_SETSCHEDULER

#endif // #ifndef _WIN32


#ifdef _WIN32
	
_MUTEXGEAR_PURE_INLINE
int _mutexgear_set_current_thread_high()
{
	return SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL) ? 0 : GetLastError();
}


#elif defined(_MGTEST_POSIX_ADJUST_SCHED)

#include <string.h>

_MUTEXGEAR_PURE_INLINE
int _mutexgear_set_current_thread_high()
{
	int iResult;
	const int iSchedPolicy = SCHED_RR;

	sched_param spSchedParam;
	memset(&spSchedParam, 0, sizeof(spSchedParam));

	int iMinPriority, iMaxPriority;
	if ((iResult = iMinPriority = sched_get_priority_min(iSchedPolicy)) != -1 && (iResult = iMaxPriority = sched_get_priority_max(iSchedPolicy)) != -1)
	{
		spSchedParam.sched_priority = iMinPriority + (iMaxPriority - iMinPriority) * 3 / 4;

		iResult = sched_setscheduler(0, iSchedPolicy, &spSchedParam);
	}

	return iResult != -1 ? 0 : errno;
}


#else
	
_MUTEXGEAR_PURE_INLINE
int _mutexgear_set_current_thread_high()
{
	return ENOSYS;
}


#endif // #ifdef _MGTEST_POSIX_ADJUST_SCHED



//////////////////////////////////////////////////////////////////////////

#ifdef _MUTEXGEAR_HAVE_CXX11
#ifndef _MGTEST_HAVE_CXX11
#define _MGTEST_HAVE_CXX11 1
#endif

#ifdef _MUTEXGEAR_HAVE_STD__SHARED_MUTEX
#ifndef _MGTEST_HAVE_STD__SHARED_MUTEX
#define _MGTEST_HAVE_STD__SHARED_MUTEX 1
#endif
#endif

#ifdef _MUTEXGEAR_HAVE_STD__SHARED_TIMED_MUTEX
#ifndef _MGTEST_HAVE_STD__SHARED_TIMED_MUTEX
#define _MGTEST_HAVE_STD__SHARED_TIMED_MUTEX 1
#endif
#endif

#if _MGTEST_HAVE_STD__SHARED_MUTEX || _MGTEST_HAVE_STD__SHARED_TIMED_MUTEX
#define _MGTEST_ANY_SHARED_MUTEX_AVAILABLE 1
#endif

#endif // #ifdef _MUTEXGEAR_HAVE_CXX11


//////////////////////////////////////////////////////////////////////////

#define _MAKE_STRING_LITERAL_HELPER(s) #s
#define MAKE_STRING_LITERAL(s) _MAKE_STRING_LITERAL_HELPER(s)

#define IN_RANGE(Value, Min, Max) ((size_t)((size_t)(Value) - (size_t)(Min)) < (size_t)((size_t)(Max) - (size_t)(Min)))

#if !defined(__cplusplus)
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#else
template <typename TElementType, size_t tsiElementsCount>
TElementType(*__ARRAY_SIZE_HELPER(TElementType(&a)[tsiElementsCount]))[tsiElementsCount];
#define ARRAY_SIZE(a) (sizeof(*__ARRAY_SIZE_HELPER(a)) / sizeof((*__ARRAY_SIZE_HELPER(a))[0]))
#endif

#if defined(_MUTEXGEAR_HAVE_CXX11)
#include <type_traits>
#endif

template<typename EnumType>
inline
#if defined(_MUTEXGEAR_HAVE_CXX11)
typename std::enable_if<std::is_enum<EnumType>::value, EnumType>::type &operator ++(EnumType &Value)
#else
EnumType &operator ++(EnumType &Value)
#endif
{
	Value = (EnumType)(Value + 1);
	return Value;
}

template<typename ValueType>
inline
bool pred_not_zero(const ValueType &vtValue)
{
	return vtValue != (ValueType)0;
}



class CTimeUtils
{
public:
	typedef uint64_t timepoint;
	typedef int64_t timeduration;

	static timeduration GetMaxTimeduration() { return INT64_MAX; }
	static uint32_t MakeTimepointHash32(timepoint tpTimePoint)
	{
		uint32_t uiTimePointLow = (uint32_t)tpTimePoint, uiTimePointHigh = (uint32_t)(tpTimePoint >> 32);
		uint64_t uiHashValue =  (((uint64_t)uiTimePointLow + ((uiTimePointHigh >> 7) | (uiTimePointHigh << 25))) << 22)
			^ ((((uint64_t)((uiTimePointLow << 11) | (uiTimePointLow >> 21)) + ((uiTimePointHigh >> 25) | (uiTimePointHigh << 7))) << 11))
			^ ((((uint64_t)((uiTimePointLow << 22) | (uiTimePointLow >> 10)) + (uiTimePointHigh))/* << 0*/));
		return (uint32_t)uiHashValue ^ (uint32_t)(uiHashValue >> 32);
	}

	static void Sleep(unsigned int uiMillisecondCount)
	{
		int iSleepResult;
		MG_CHECK(iSleepResult, _mutexgear_sleep(uiMillisecondCount * (uint64_t)1000000) == 0 || (iSleepResult = errno, false));
	}

	static timepoint GetCurrentMonotonicTimeNano()
	{
		timepoint tpResult;

		int iClockResult;
		MG_CHECK(iClockResult, _mutexgear_monotonic_clock_time(&tpResult) == 0 || (iClockResult = errno, false));

		return tpResult;
	}
};


//////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#if _MSC_VER < 1900
#define snprintf _snprintf
#endif
#endif


#endif //PCH_H
