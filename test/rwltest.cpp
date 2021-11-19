/************************************************************************/
/* The MutexGear Library                                                */
/* The Library RWLock Implementation Test File                          */
/*                                                                      */
/* Copyright (c) 2016-2021 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/

/**
 *	\file
 *	\brief RWLock Test
 *
 *	The file implements a test for both the C language \c mutexgear_rwlock_t and \c mutexgear_trdl_rwlock_t operations,
 *	as well as their C++ wrappers \c mg::shared_mutex and \c mg::trld::shared_mutex.
 *
 *	For the tests, numbers of threads are launched each having to perform \c MGTEST_RWLOCK_ITERATION_COUNT (a million by default)
 *	locks and unlocks. In between of each lock and unlock each thread performs a call to \c rand() function to simulate a small delay.
 *	Also, each thread records its number and operation type in a memory buffer so that it could be later possible to reconstruct
 *	the exact order of each lock and unlock operation with respect to all the other threads. At test levels of 'basic' and higher,
 *	this information is dumped into textual format files allowing both visual examination/comparison and/or conversion into bitmaps
 *	with operations and threads being color-coded in pixels. The same tests are run sequentially (and with a cool-down delay of 5 seconds) for
 *	the MutexGear implementation and the respective standard RWLock object available in the system thus allowing to compare the total consumed times
 *	and individual operation distributions and patterns. Also, the tests are instrumented with runtime checks to ensure lock consistency
 *	(like absence of simultaneous locks for write or write and read from multiple threads).
 *
 *	There are the following groups of tests:
 *	\li write locks for increasing numbers of threads;
 *	\li	read locks for increasing numbers of threads;
 *	\li mixed tests with each thread executing read locks interleaved with some write locks for increasing numbers of threads and increasing percentages of the write locks;
 *	\li mixed tests with a fraction of threads performing write locks and the remaining threads performing read locks for increasing numbers of threads and increasing fractions of the write locking threads.
 *
 *	From practical perspective, the last group seems to be somewhat artificial as it's hard to imagine real life cases
 *	when some threads would be constantly and repeatedly write-locking an object while other threads would be reading it.
 *
 *	Note that for better code coverage the threads first attempt their respective try-operation variants (the try-read or try-write locks)
 *	and perform the normal blocking locks only if the try-operations are unsuccessful. To disable this, redefine \c MGTEST_RWLOCK_TEST_TRYWRLOCK
 *	and \c MGTEST_RWLOCK_TEST_TRYRDLOCK to zeros in the header.
 *
 *	\warning
 *	WARNING!
 *	Also, keep in mind that since the tests make time measurements, they need to raise the threads to high priorities
 *	and this may require running with admin privileges on some systems (like Ubuntu Linux, for example).
 *	Also, running multiple high priority CPU consuming threads will likely suspend most of the normal activity in the system
 *	and can make it unavailable from network for the duration of the individual tests. So, it's a good idea to make sure
 *	the system is free and idle. Also, it's worth rebooting the system before the test run and stopping any extra
 *	software to clear the memory and caches and have minimum of interference from other system components.
 *
 *	\note
 *	The operation sequence dump file format is as follows.
 *	Each thread index is coded with a zero based "two-digit number", the higher digit being coded with a character.
 *	The read-locking threads (or operations) start with "a" and the write-locks start with 's'. With that,
 *	the lock operations are capitalized and the unlock operations use small letters. For example, a write lock in
 *	thread #0 is coded as 'S0'; a write-unlock of in thread #25 is coded as 'u5' ('u' = 's' + 2); a read lock in
 *	thread #11 is coded as 'B1' and the respective unlock is 'b1'. If a lock succeeds with its try-operation alone
 *	that is denoted with a dot after the thread number. All the lock operations that were accomplished with their normal
 *	[potentially] blocking calls, and all the unlocks, have commas after then.
 */

#include "pch.h"
#include "rwltest.h"
#include "mgtest_common.h"


#ifdef _MSC_VER
#pragma message("warning: The test takes quite some time - be patient")
#else
#warning The test may require admin rights for raising thread priorities
#warning Also, it takes quite some time - be patient
#endif


#if _MGTEST_HAVE_CXX11
#include <mutexgear/shared_mutex.hpp>
#else
#include <mutexgear/rwlock.h>
#endif 

#if _MGTEST_ANY_SHARED_MUTEX_AVAILABLE
#include <shared_mutex>
#endif


#if _MGTEST_HAVE_CXX11

#include <mutexgear/wheel.hpp>
#include <mutexgear/toggle.hpp>
#include <mutexgear/completion.hpp>


#endif // #if _MGTEST_HAVE_CXX11


#ifdef _WIN32
#include <process.h>
#endif

#include <atomic>
#include <utility>


#if _MGTEST_HAVE_STD__SHARED_MUTEX
#define SYSTEM_CPP_RWLOCK_VARIANT_T std::shared_mutex
#elif _MGTEST_HAVE_STD__SHARED_TIMED_MUTEX
#define SYSTEM_CPP_RWLOCK_VARIANT_T std::shared_timed_mutex
#endif
#define SYSTEM_C_RWLOCK_VARIANT_T _MG_RWLOCK_T


enum ERWLOCKLOCKTESTOBJECT
{
	LTO__MIN,

	LTO_SYSTEM = LTO__MIN,
	LTO_MUTEXGEAR,

	LTO__MAX,
};

enum ERWLOCKLOCKTESTLANGUAGE
{
	LTL__MIN,

	LTL_C = LTL__MIN,
	LTL_CPP,

	LTL__MAX,
};

enum ERWLOCKTESTTRYREADSUPPORT
{
	TRS__MIN,

	TRS_NO_TRYREAD_SUPPORT = TRS__MIN,
	TRS_WITH_TRYREAD_SUPPORT,

	TRS__MAX,
};


static const unsigned g_uiLockIterationCount = MGTEST_RWLOCK_ITERATION_COUNT;
static const unsigned g_uiLockOperationsPerLine = 100;
const unsigned int g_uiSettleDownSleepDelay = 5000;

static const char g_ascLockTestDetailsFileNameFormat[] = "LockTestDet%s_%u%s_%u_%s_%s.txt";

static const char g_ascPriorityAdjustmentErrorWarningFormat[] = "WARNING: Test thread priority adjustment failed with error code %d (insufficient privileges?). The test times may be inaccurate.\n";
static _mg_atomic_uint_t	g_uiPriorityAdjustmentErrorReported(0);

enum
{
	LOCK_THREAD_NAME_LENGTH = 2,
};

static const char g_cNormalVariantOperationDetailMapSeparatorChar = ',';
static const char g_cTryVariantOperationDetailMapSeparatorChar = '.';
static const char g_cWriterMapFirstChar = 'S';
static const char g_cReaderMapFirstChar = 'A';

#define ENCODE_SEQUENCE_WITH_TRYVARIANT(iiLockOperationSequence, bLockedWitTryVariant) (bLockedWitTryVariant ? (-1 - iiLockOperationSequence) : iiLockOperationSequence)
#define DECODE_ENCODED_SEQUENCE(SwTV) ((operationsidxint)(SwTV) < 0 ? (-1 - (SwTV)) : (SwTV))
#define DECODE_ENCODED_TRYVARIANT(SwTV) ((operationsidxint)(SwTV) < 0)

static const char *const g_aszTRDLSupportFileNameSuffixes[] =
{
	"", // TRS_NO_TRYREAD_SUPPORT,
	"_trdl", // TRS_WITH_TRYREAD_SUPPORT,
};
MG_STATIC_ASSERT(ARRAY_SIZE(g_aszTRDLSupportFileNameSuffixes) == TRS__MAX);

static const char *const g_aszTestedObjectKindFileNameSuffixes[] =
{
	"Sys", // LTO_SYSTEM,
	"MG", // LTO_MUTEXGEAR,
};
MG_STATIC_ASSERT(ARRAY_SIZE(g_aszTestedObjectKindFileNameSuffixes) == LTO__MAX);

static const char *const g_aszTestedObjectLanguageNames[] =
{
	"C", // LTL_C,
	"C++", // LTL_CPP,
};
MG_STATIC_ASSERT(ARRAY_SIZE(g_aszTestedObjectLanguageNames) == LTL__MAX);


class CThreadExecutionBarrier
{
public:
	typedef unsigned int size_type;

	CThreadExecutionBarrier(size_type uiCountValue)
	{
		_mg_atomic_init_uint(&m_uiCountRemaining, uiCountValue);

		int iEventInitResult;
		MG_CHECK(iEventInitResult, (iEventInitResult = _mutexgear_manualevent_init(&m_meEventInstance)) == EOK);
	}

	~CThreadExecutionBarrier()
	{
		int iEventDestroyResult;
		MG_CHECK(iEventDestroyResult, (iEventDestroyResult = _mutexgear_manualevent_destroy(&m_meEventInstance)) == EOK);
	}

	unsigned int RetrieveCounterValue() const
	{
		return _mg_atomic_load_relaxed_uint(&m_uiCountRemaining);
	}

	void DecreaseCounterValue(bool &bOutWasLastToDecrease)
	{
		bOutWasLastToDecrease = _mg_atomic_fetch_sub_relaxed_uint(&m_uiCountRemaining, 1) == 1;
	}

	void ResetInstance(size_type uiCountValue)
	{
		_mg_atomic_init_uint(&m_uiCountRemaining, uiCountValue);

		int iEventResetResult;
		MG_CHECK(iEventResetResult, (iEventResetResult = _mutexgear_manualevent_reset(&m_meEventInstance)) == EOK);
	}

	void SignalReleaseEvent()
	{
		int iEventSetResult;
		MG_CHECK(iEventSetResult, (iEventSetResult = _mutexgear_manualevent_set(&m_meEventInstance)) == EOK);
	}

	void WaitReleaseEvent()
	{
		int iEventWaitResult;
		MG_CHECK(iEventWaitResult, (iEventWaitResult = _mutexgear_manualevent_wait(&m_meEventInstance)) == EOK);
	}

private:
	volatile _mg_atomic_uint_t	m_uiCountRemaining;
	_MG_MANUALEVENT_T			m_meEventInstance;
};


template<ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
class CTryReadAdapter;

template<>
class CTryReadAdapter<TRS_NO_TRYREAD_SUPPORT, LTL_C>
{
public:
	typedef mutexgear_rwlock_t rwlock_type;

	static int TryReadLock(rwlock_type *pwlLockInstance, mutexgear_completion_worker_t *pcwLockWorker, mutexgear_completion_item_t *pciLockCompletionItem)
	{
		return EBUSY;
	}
};

template<>
class CTryReadAdapter<TRS_WITH_TRYREAD_SUPPORT, LTL_C>
{
public:
	typedef mutexgear_trdl_rwlock_t rwlock_type;

	static int TryReadLock(rwlock_type *pwlLockInstance, mutexgear_completion_worker_t *pcwLockWorker, mutexgear_completion_item_t *pciLockCompletionItem)
	{
		int iLockResult;
#if MGTEST_RWLOCK_TEST_TRYRDLOCK
		iLockResult = mutexgear_rwlock_tryrdlock(pwlLockInstance, pcwLockWorker, pciLockCompletionItem);
#else /// !MGTEST_RWLOCK_TEST_TRYRDLOCK
		iLockResult = EBUSY;
#endif // !MGTEST_RWLOCK_TEST_TRYRDLOCK
		return iLockResult;
	}
};


#if _MGTEST_HAVE_CXX11

template<>
class CTryReadAdapter<TRS_NO_TRYREAD_SUPPORT, LTL_CPP>
{
public:
	typedef mg::shared_mutex rwlock_type;

	static bool TryReadLock(rwlock_type &wlLockInstance, rwlock_type::helper_worker_type &wLockWorker, rwlock_type::helper_item_type &iLockCompletionItem)
	{
		return false;
	}
};

template<>
class CTryReadAdapter<TRS_WITH_TRYREAD_SUPPORT, LTL_CPP>
{
public:
	typedef mg::trdl::shared_mutex rwlock_type;

	static bool TryReadLock(rwlock_type &wlLockInstance, rwlock_type::helper_worker_type &wLockWorker, rwlock_type::helper_item_type &iLockCompletionItem)
	{
		bool bLockResult;
#if MGTEST_RWLOCK_TEST_TRYRDLOCK
		bLockResult = wlLockInstance.try_lock_shared(wLockWorker, iLockCompletionItem);
#else /// !MGTEST_RWLOCK_TEST_TRYRDLOCK
		bLockResult = false;
#endif // !MGTEST_RWLOCK_TEST_TRYRDLOCK
		return bLockResult;
	}
};


#endif // #if _MGTEST_HAVE_CXX11


template<ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTOBJECT ttoTestedObjectKind, ERWLOCKLOCKTESTLANGUAGE ttlTestObjectLanguage>
class CRWLockImplementation;

enum
{
	LIOPT_MULTIPLE_WRITE_CHANNELS = 0x01,
};

template<ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport>
class CRWLockImplementation<trsTryReadSupport, LTO_SYSTEM, LTL_C>
{
public:
	CRWLockImplementation(unsigned /*uiImplementationOptions*/) { InitializeRWLockInstance(); }
	~CRWLockImplementation() { FinalizeRWLockInstance(); }

public:
	class CLockWriteExtraObjects
	{
	public:
	};

	class CLockReadExtraObjects :
		public CLockWriteExtraObjects
	{
	public:
	};

	void InitializeRWLockInstance()
	{
		int iInitResult;
		MG_CHECK(iInitResult, (iInitResult = _mutexgear_rwlock_init(&m_wlRWLock)) == EOK);
	}

	void FinalizeRWLockInstance()
	{
		int iDestroyResult;
		MG_CHECK(iDestroyResult, (iDestroyResult = _mutexgear_rwlock_destroy(&m_wlRWLock)) == EOK);
	}

	bool LockRWLockWrite(CLockWriteExtraObjects &eoRefExtraObjects)
	{
		bool bLockedWithTryVariant;

#if MGTEST_RWLOCK_TEST_TRYWRLOCK
		int iLockResult = _mutexgear_rwlock_trywrlock(&m_wlRWLock);
		bLockedWithTryVariant = iLockResult == EOK;
#else // !MGTEST_RWLOCK_TEST_TRYWRLOCK
		int iLockResult = EBUSY;
		bLockedWithTryVariant = false;
#endif // !MGTEST_RWLOCK_TEST_TRYWRLOCK

		if (!bLockedWithTryVariant)
		{
			MG_CHECK(iLockResult, iLockResult == EBUSY && (iLockResult = _mutexgear_rwlock_wrlock(&m_wlRWLock)) == EOK);
		}

		return bLockedWithTryVariant;
	}

	void UnlockRWLockWrite(CLockWriteExtraObjects &eoRefExtraObjects)
	{
		int iUnlockResult;
		MG_CHECK(iUnlockResult, (iUnlockResult = _mutexgear_rwlock_wrunlock(&m_wlRWLock)) == EOK);
	}

	bool LockRWLockRead(CLockReadExtraObjects &eoRefExtraObjects)
	{
		bool bLockedWithTryVariant;

#if MGTEST_RWLOCK_TEST_TRYRDLOCK
		int iLockResult = _mutexgear_rwlock_tryrdlock(&m_wlRWLock);
		bLockedWithTryVariant = iLockResult == EOK;
#else // !MGTEST_RWLOCK_TEST_TRYRDLOCK
		int iLockResult = EBUSY;
		bLockedWithTryVariant = false;
#endif // !MGTEST_RWLOCK_TEST_TRYRDLOCK

		if (!bLockedWithTryVariant)
		{
			MG_CHECK(iLockResult, iLockResult == EBUSY && (iLockResult = _mutexgear_rwlock_rdlock(&m_wlRWLock)) == EOK);
		}

		return bLockedWithTryVariant;
	}

	void UnlockRWLockRead(CLockReadExtraObjects &eoRefExtraObjects)
	{
		int iUnlockResult;
		MG_CHECK(iUnlockResult, (iUnlockResult = _mutexgear_rwlock_rdunlock(&m_wlRWLock)) == EOK);
	}

private:
	SYSTEM_C_RWLOCK_VARIANT_T	m_wlRWLock;
};

#if _MGTEST_ANY_SHARED_MUTEX_AVAILABLE

template<ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport>
class CRWLockImplementation<trsTryReadSupport, LTO_SYSTEM, LTL_CPP>
{
public:
	CRWLockImplementation(unsigned /*uiImplementationOptions*/) { InitializeRWLockInstance(); }
	~CRWLockImplementation() { FinalizeRWLockInstance(); }

public:
	class CLockWriteExtraObjects
	{
	public:
	};

	class CLockReadExtraObjects :
		public CLockWriteExtraObjects
	{
	public:
	};

	void InitializeRWLockInstance()
	{
	}

	void FinalizeRWLockInstance()
	{
	}

	bool LockRWLockWrite(CLockWriteExtraObjects &eoRefExtraObjects)
	{
		bool bLockedWithTryVariant;

#if MGTEST_RWLOCK_TEST_TRYWRLOCK
		bLockedWithTryVariant = m_wlRWLock.try_lock();
#else // !MGTEST_RWLOCK_TEST_TRYWRLOCK
		bLockedWithTryVariant = false;
#endif // !MGTEST_RWLOCK_TEST_TRYWRLOCK

		if (!bLockedWithTryVariant)
		{
			m_wlRWLock.lock();
		}

		return bLockedWithTryVariant;
	}

	void UnlockRWLockWrite(CLockWriteExtraObjects &eoRefExtraObjects)
	{
		m_wlRWLock.unlock();
	}

	bool LockRWLockRead(CLockReadExtraObjects &eoRefExtraObjects)
	{
		bool bLockedWithTryVariant;

#if MGTEST_RWLOCK_TEST_TRYRDLOCK
		bLockedWithTryVariant = m_wlRWLock.try_lock_shared();
#else // !MGTEST_RWLOCK_TEST_TRYRDLOCK
		bLockedWithTryVariant = false;
#endif // !MGTEST_RWLOCK_TEST_TRYRDLOCK

		if (!bLockedWithTryVariant)
		{
			m_wlRWLock.lock_shared();
		}

		return bLockedWithTryVariant;
	}

	void UnlockRWLockRead(CLockReadExtraObjects &eoRefExtraObjects)
	{
		m_wlRWLock.unlock_shared();
	}

private:
	SYSTEM_CPP_RWLOCK_VARIANT_T	m_wlRWLock;
};


#else  // #if !_MGTEST_ANY_SHARED_MUTEX_AVAILABLE

template<ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport>
class CRWLockImplementation<trsTryReadSupport, LTO_SYSTEM, LTL_CPP>
{
public:
	CRWLockImplementation(unsigned /*uiImplementationOptions*/) { }
	~CRWLockImplementation() { }

public:
	class CLockWriteExtraObjects
	{
	public:
	};

	class CLockReadExtraObjects :
		public CLockWriteExtraObjects
	{
	public:
	};

	bool LockRWLockWrite(CLockWriteExtraObjects &eoRefExtraObjects)
	{
		return true;
	}

	void UnlockRWLockWrite(CLockWriteExtraObjects &eoRefExtraObjects)
	{
	}

	bool LockRWLockRead(CLockReadExtraObjects &eoRefExtraObjects)
	{
		return true;
	}

	void UnlockRWLockRead(CLockReadExtraObjects &eoRefExtraObjects)
	{
	}
};


#endif // #if !_MGTEST_ANY_SHARED_MUTEX_AVAILABLE


template<ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport>
class CRWLockImplementation<trsTryReadSupport, LTO_MUTEXGEAR, LTL_C>
{
	typedef CTryReadAdapter<trsTryReadSupport, LTL_C> tryread_adapter_type;
	typedef typename tryread_adapter_type::rwlock_type rwlock_type;

public:
	CRWLockImplementation(unsigned uiImplementationOptions) { InitializeRWLockInstance(uiImplementationOptions); }
	~CRWLockImplementation() { FinalizeRWLockInstance(); }

public:
	class CLockWriteExtraObjects
	{
	public:
		CLockWriteExtraObjects()
		{
			int iWorkerInitResult;
			MG_CHECK(iWorkerInitResult, (iWorkerInitResult = mutexgear_completion_worker_init(&m_cwLockWorker, NULL)) == EOK);

			int iWorkerLockResult;
			MG_CHECK(iWorkerLockResult, (iWorkerLockResult = mutexgear_completion_worker_lock(&m_cwLockWorker)) == EOK);

			int iWaiterInitResult;
			MG_CHECK(iWaiterInitResult, (iWaiterInitResult = mutexgear_completion_waiter_init(&m_cwLockWaiter, NULL)) == EOK);
		}

		~CLockWriteExtraObjects()
		{
			int iWaiterDestroyResult;
			MG_CHECK(iWaiterDestroyResult, (iWaiterDestroyResult = mutexgear_completion_waiter_destroy(&m_cwLockWaiter)) == EOK);

			int iWorkerUnlockResult;
			MG_CHECK(iWorkerUnlockResult, (iWorkerUnlockResult = mutexgear_completion_worker_unlock(&m_cwLockWorker)) == EOK);

			int iWorkerDestroyResult;
			MG_CHECK(iWorkerDestroyResult, (iWorkerDestroyResult = mutexgear_completion_worker_destroy(&m_cwLockWorker)) == EOK);
		}

		mutexgear_completion_worker_t		m_cwLockWorker;
		mutexgear_completion_waiter_t		m_cwLockWaiter;
	};

	class CLockReadExtraObjects
	{
	public:
		CLockReadExtraObjects()
		{
			mutexgear_completion_item_init(&m_ciLockCompletionItem);
		}

		~CLockReadExtraObjects()
		{
			mutexgear_completion_item_destroy(&m_ciLockCompletionItem);
		}

		operator CLockWriteExtraObjects &() { return m_eoWriteObjects; }

		CLockWriteExtraObjects				m_eoWriteObjects;
		mutexgear_completion_item_t			m_ciLockCompletionItem;
	};

public:
	bool LockRWLockWrite(CLockWriteExtraObjects &eoRefExtraObjects)
	{
		bool bLockedWithTryVariant;
#if MGTEST_RWLOCK_TEST_TRYWRLOCK
		int iLockResult = mutexgear_rwlock_trywrlock(&m_wlRWLock);
		bLockedWithTryVariant = iLockResult == EOK;
#else // !MGTEST_RWLOCK_TEST_TRYWRLOCK
		int iLockResult = EBUSY;
		bLockedWithTryVariant = false;
#endif // !MGTEST_RWLOCK_TEST_TRYWRLOCK

		if (!bLockedWithTryVariant)
		{
			MG_CHECK(iLockResult, iLockResult == EBUSY && (iLockResult = mutexgear_rwlock_wrlock(&m_wlRWLock, &eoRefExtraObjects.m_cwLockWorker, &eoRefExtraObjects.m_cwLockWaiter, NULL)) == EOK);
		}

		return bLockedWithTryVariant;
	}

	void UnlockRWLockWrite(CLockWriteExtraObjects &eoRefExtraObjects)
	{
		int iUnlockResult;
		MG_CHECK(iUnlockResult, (iUnlockResult = mutexgear_rwlock_wrunlock(&m_wlRWLock)) == EOK);
	}

	bool LockRWLockRead(CLockReadExtraObjects &eoRefExtraObjects)
	{
		bool bLockedWithTryVariant;

		int iLockResult = tryread_adapter_type::TryReadLock(&m_wlRWLock, &eoRefExtraObjects.m_eoWriteObjects.m_cwLockWorker, &eoRefExtraObjects.m_ciLockCompletionItem);
		bLockedWithTryVariant = iLockResult == EOK;

		if (!bLockedWithTryVariant)
		{
			MG_CHECK(iLockResult, iLockResult == EBUSY && (iLockResult = mutexgear_rwlock_rdlock(&m_wlRWLock, &eoRefExtraObjects.m_eoWriteObjects.m_cwLockWorker, &eoRefExtraObjects.m_eoWriteObjects.m_cwLockWaiter, &eoRefExtraObjects.m_ciLockCompletionItem)) == EOK);
		}

		return bLockedWithTryVariant;
	}

	void UnlockRWLockRead(CLockReadExtraObjects &eoRefExtraObjects)
	{
		int iUnlockResult;
		MG_CHECK(iUnlockResult, (iUnlockResult = mutexgear_rwlock_rdunlock(&m_wlRWLock, &eoRefExtraObjects.m_eoWriteObjects.m_cwLockWorker, &eoRefExtraObjects.m_ciLockCompletionItem)) == EOK);
	}

private:
	void InitializeRWLockInstance(unsigned uiImplementationOptions)
	{
		int iInitResult;
		mutexgear_rwlockattr_t attr;

		MG_CHECK(iInitResult, (iInitResult = mutexgear_rwlockattr_init(&attr)) == EOK);

		if ((uiImplementationOptions & LIOPT_MULTIPLE_WRITE_CHANNELS) != 0)
		{
			MG_CHECK(iInitResult, (iInitResult = mutexgear_rwlockattr_setwritechannels(&attr, MGTEST_RWLOCK_WRITE_CHANNELS)) == EOK);
		}

		MG_CHECK(iInitResult, (iInitResult = mutexgear_rwlock_init(&m_wlRWLock, &attr)) == EOK);
		MG_CHECK(iInitResult, (iInitResult = mutexgear_rwlockattr_destroy(&attr)) == EOK);
	}

	void FinalizeRWLockInstance()
	{
		int iDestroyResult;
		MG_CHECK(iDestroyResult, (iDestroyResult = mutexgear_rwlock_destroy(&m_wlRWLock)) == EOK);
	}

private:
	rwlock_type				m_wlRWLock;
};


#if _MGTEST_HAVE_CXX11

template<ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport>
class CRWLockImplementation<trsTryReadSupport, LTO_MUTEXGEAR, LTL_CPP>
{
	typedef CTryReadAdapter<trsTryReadSupport, LTL_CPP> tryread_adapter_type;
	typedef typename tryread_adapter_type::rwlock_type rwlock_type;

public:
	CRWLockImplementation(unsigned /*uiImplementationOptions*/) { InitializeRWLockInstance(); }
	~CRWLockImplementation() { FinalizeRWLockInstance(); }

public:
	class CLockWriteExtraObjects
	{
	public:
		CLockWriteExtraObjects()
		{
			m_cwLockWorker.lock();
		}

		~CLockWriteExtraObjects()
		{
			m_cwLockWorker.unlock();
		}

		typename rwlock_type::helper_worker_type m_cwLockWorker;
		typename rwlock_type::helper_waiter_type m_cwLockWaiter;
	};

	class CLockReadExtraObjects
	{
	public:
		CLockReadExtraObjects()
		{
		}

		~CLockReadExtraObjects()
		{
		}

		operator CLockWriteExtraObjects &() { return m_eoWriteObjects; }

		CLockWriteExtraObjects				m_eoWriteObjects;
		typename rwlock_type::helper_item_type m_ciLockCompletionItem;
	};

public:
	bool LockRWLockWrite(CLockWriteExtraObjects &eoRefExtraObjects)
	{
		bool bLockedWithTryVariant;
#if MGTEST_RWLOCK_TEST_TRYWRLOCK
		bLockedWithTryVariant = m_wlRWLock.try_lock();
#else // !MGTEST_RWLOCK_TEST_TRYWRLOCK
		bLockedWithTryVariant = false;
#endif // !MGTEST_RWLOCK_TEST_TRYWRLOCK

		if (!bLockedWithTryVariant)
		{
			m_wlRWLock.lock(eoRefExtraObjects.m_cwLockWorker, eoRefExtraObjects.m_cwLockWaiter);
		}

		return bLockedWithTryVariant;
	}

	void UnlockRWLockWrite(CLockWriteExtraObjects &eoRefExtraObjects)
	{
		m_wlRWLock.unlock();
	}

	bool LockRWLockRead(CLockReadExtraObjects &eoRefExtraObjects)
	{
		bool bLockedWithTryVariant;

		bLockedWithTryVariant = tryread_adapter_type::TryReadLock(m_wlRWLock, eoRefExtraObjects.m_eoWriteObjects.m_cwLockWorker, eoRefExtraObjects.m_ciLockCompletionItem);

		if (!bLockedWithTryVariant)
		{
			m_wlRWLock.lock_shared(eoRefExtraObjects.m_eoWriteObjects.m_cwLockWorker, eoRefExtraObjects.m_eoWriteObjects.m_cwLockWaiter, eoRefExtraObjects.m_ciLockCompletionItem);
		}

		return bLockedWithTryVariant;
	}

	void UnlockRWLockRead(CLockReadExtraObjects &eoRefExtraObjects)
	{
		m_wlRWLock.unlock_shared(eoRefExtraObjects.m_eoWriteObjects.m_cwLockWorker, eoRefExtraObjects.m_ciLockCompletionItem);
	}

private:
	void InitializeRWLockInstance()
	{
	}

	void FinalizeRWLockInstance()
	{
	}

private:
	rwlock_type				m_wlRWLock;
};


#else // #if !_MGTEST_HAVE_CXX11

template<ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport>
class CRWLockImplementation<trsTryReadSupport, LTO_MUTEXGEAR, LTL_CPP>
{
public:
	CRWLockImplementation() { }
	~CRWLockImplementation() { }

public:
	class CLockWriteExtraObjects
	{
	public:
		CLockWriteExtraObjects()
		{
		}

		~CLockWriteExtraObjects()
		{
		}
	};

	class CLockReadExtraObjects
	{
	public:
		CLockReadExtraObjects()
		{
		}

		~CLockReadExtraObjects()
		{
		}

		operator CLockWriteExtraObjects &() { return m_eoWriteObjects; }

		CLockWriteExtraObjects				m_eoWriteObjects;
	};

public:
	bool LockRWLockWrite(CLockWriteExtraObjects &eoRefExtraObjects)
	{
		return true;
	}

	void UnlockRWLockWrite(CLockWriteExtraObjects &eoRefExtraObjects)
	{
	}

	bool LockRWLockRead(CLockReadExtraObjects &eoRefExtraObjects)
	{
		return true;
	}

	void UnlockRWLockRead(CLockReadExtraObjects &eoRefExtraObjects)
	{
	}
};


#endif // #if !_MGTEST_HAVE_CXX11


class CRWLockLockTestProgress;

class IRWLockLockTestThread
{
public:
	virtual void ReleaseInstance() = 0;
};


class CRWLockLockTestBase
{
public:
	typedef unsigned int iterationcntint;
	typedef unsigned int operationidxint; typedef signed int operationsidxint; typedef _mg_atomic_uint_t atomic_operationidxint;
	typedef unsigned int threadcntint;

	static inline void atomic_init_operationidxint(volatile atomic_operationidxint *piiDestination, operationidxint iiValue)
	{
		_mg_atomic_init_uint(piiDestination, iiValue);
	}

	static inline operationidxint atomic_fetch_add_relaxed_operationidxint(volatile atomic_operationidxint *piiDestination, operationidxint iiValue)
	{
		return _mg_atomic_fetch_add_relaxed_uint(piiDestination, iiValue);
	}
};

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
class CRWLockLockTestExecutor :
	public CRWLockLockTestBase
{
public:
	explicit CRWLockLockTestExecutor(iterationcntint uiIterationCount, unsigned uiImplementationOptions) :
		m_uiIterationCount(uiIterationCount),
		m_uiImplementationOptions(uiImplementationOptions),
		m_pscDetailsMergeBuffer(NULL),
		m_iiThreadOperationCount(0)
	{
		std::fill(m_aittTestThreads, m_aittTestThreads + ARRAY_SIZE(m_aittTestThreads), (IRWLockLockTestThread *)NULL);
		std::fill(m_apiiThreadOperationIndices, m_apiiThreadOperationIndices + ARRAY_SIZE(m_apiiThreadOperationIndices), (operationidxint *)NULL);
		std::fill(m_apsfOperationDetailFiles, m_apsfOperationDetailFiles + ARRAY_SIZE(m_apsfOperationDetailFiles), (FILE *)NULL);
	}

	~CRWLockLockTestExecutor();

public:
	bool RunTheTask();

private:
	void AllocateThreadOperationBuffers(threadcntint ciTotalThreadCount);
	void FreeThreadOperationBuffers(threadcntint ciTotalThreadCount);
	operationidxint GetMaximalThreadOperationCount() const;

	template<ERWLOCKLOCKTESTOBJECT ttoTestedObjectKind>
	void AllocateTestThreads(CRWLockImplementation<trsTryReadSupport, ttoTestedObjectKind, ttlTestLanguage> &liRefRWLock,
		threadcntint ciWriterCount, threadcntint ciReraderCount,
		CThreadExecutionBarrier &sbRefStartBarrier, CThreadExecutionBarrier &sbRefFinishBarrier, CThreadExecutionBarrier &sbRefExitBarrier,
		CRWLockLockTestProgress &tpRefProgressInstance);
	void FreeTestThreads(CThreadExecutionBarrier &sbRefExitBarrier, threadcntint ciTotalThreadCount);

	void WaitTestThreadsReady(CThreadExecutionBarrier &sbStartBarrier);
	void LaunchTheTest(CThreadExecutionBarrier &sbStartBarrier);
	void WaitTheTestEnd(CThreadExecutionBarrier &sbFinishBarrier);

	void InitializeTestResults(threadcntint ciWriterCount, threadcntint ciReraderCount, EMGTESTFEATURELEVEL flTestLevel);
	void PublishTestResults(ERWLOCKLOCKTESTOBJECT toTestedObjectKind, threadcntint ciWriterCount, threadcntint ciReraderCount, EMGTESTFEATURELEVEL flTestLevel,
		CTimeUtils::timeduration tdTestDuration);
	void BuildMergedThreadOperationMap(char ascMergeBuffer[], operationidxint iiLastSavedOperationIndex, operationidxint iiThreadOperationCount,
		operationidxint aiiThreadLastOperationIndices[], threadcntint ciWriterCount, threadcntint ciReaderCount);
	operationidxint FillThreadOperationMap(char ascMergeBuffer[], operationidxint iiLastSavedOperationIndex, const operationidxint iiThreadOperationCount,
		const operationidxint *piiThreadOperationIndices, operationidxint iiThreadLastOperationIndex, const char ascThreadMapChars[LOCK_THREAD_NAME_LENGTH]);
	void InitializeThreadName(char ascThreadMapChars[LOCK_THREAD_NAME_LENGTH + 1], char cMapFirstChar);
	void IncrementThreadName(char ascThreadMapChars[LOCK_THREAD_NAME_LENGTH + 1], threadcntint ciThreadIndex);
	void SaveThreadOperationMapToDetails(ERWLOCKLOCKTESTOBJECT toTestedObjectKind, const char ascMergeBuffer[], operationidxint iiThreadOperationCount);
	void FinalizeTestResults(EMGTESTFEATURELEVEL flTestLevel);

private:
	enum
	{
		LOCKTEST_WRITER_COUNT = tuiWriterCount,
		LOCKTEST_READER_COUNT = tuiReaderCount,
		LOCKTEST_THREAD_COUNT = LOCKTEST_WRITER_COUNT + LOCKTEST_READER_COUNT,
	};

private:
	void SetDetailsMergeBuffer(char *pscValue) { m_pscDetailsMergeBuffer = pscValue; }
	char *GetDetailsMergeBuffer() const { return m_pscDetailsMergeBuffer; }

	void SetThreadOperationCount(operationidxint iiValue) { m_iiThreadOperationCount = iiValue; }
	operationidxint GetThreadOperationCount() const { return m_iiThreadOperationCount; }

private:
	iterationcntint		m_uiIterationCount;
	unsigned int		m_uiImplementationOptions;
	IRWLockLockTestThread *m_aittTestThreads[LOCKTEST_THREAD_COUNT];
	operationidxint		*m_apiiThreadOperationIndices[LOCKTEST_THREAD_COUNT];
	char				*m_pscDetailsMergeBuffer;
	operationidxint		m_iiThreadOperationCount;
	FILE				*m_apsfOperationDetailFiles[LTO__MAX];
};

class CRWLockLockTestProgress
{
public:
	CRWLockLockTestProgress()
	{
		ResetInstance();
	}

public:
	typedef CRWLockLockTestBase::operationidxint operationidxint;
	typedef CRWLockLockTestBase::atomic_operationidxint atomic_operationidxint;

	void ResetInstance()
	{
		CRWLockLockTestBase::atomic_init_operationidxint(&m_iiOperationIndexStorage, 0);
	}

	operationidxint GenerateOperationIndex()
	{
		return CRWLockLockTestBase::atomic_fetch_add_relaxed_operationidxint(&m_iiOperationIndexStorage, 1);
	}

private:
	volatile atomic_operationidxint	m_iiOperationIndexStorage;
};


template<class TOperationExecutor>
class CRWLockLockTestThread :
	public IRWLockLockTestThread
{
public:
	typedef CRWLockLockTestBase::iterationcntint iterationcntint;
	typedef CRWLockLockTestBase::operationidxint operationidxint;

	struct CThreadParameterInfo
	{
	public:
		explicit CThreadParameterInfo(CThreadExecutionBarrier &sbRefStartBarrier, CThreadExecutionBarrier &sbRefFinishBarrier, CThreadExecutionBarrier &sbRefExitBarrier,
			CRWLockLockTestProgress &tpRefProgressInstance) :
			m_psbStartBarrier(&sbRefStartBarrier),
			m_psbFinishBarrier(&sbRefFinishBarrier),
			m_psbExitBarrier(&sbRefExitBarrier),
			m_ptpProgressInstance(&tpRefProgressInstance)
		{
		}

	public:
		CThreadExecutionBarrier			*m_psbStartBarrier;
		CThreadExecutionBarrier			*m_psbFinishBarrier;
		CThreadExecutionBarrier			*m_psbExitBarrier;
		CRWLockLockTestProgress			*m_ptpProgressInstance;
	};

public:
	CRWLockLockTestThread(const CThreadParameterInfo &piThreadParameters, iterationcntint uiIterationCount, const typename TOperationExecutor::CConstructionParameter &cpExecutorParameter, operationidxint aiiOperationIndexBuffer[]) :
		m_piThreadParameters(piThreadParameters),
		m_uiIterationCount(uiIterationCount),
		m_piiOperationIndexBuffer(aiiOperationIndexBuffer),
		m_oeOperationExecutor(cpExecutorParameter)
	{
		StartRunningThread();
	}

protected: // Call IRWLockLockTestThread::ReleaseInstance()
	virtual ~CRWLockLockTestThread();

private:
	void StartRunningThread();
	void StopRunningThread();

private:
#ifdef _WIN32
	static unsigned __stdcall StaticExecuteThread(void *psvThreadParameter);
#else // #ifndef _WIN32
	static void *StaticExecuteThread(void *psvThreadParameter);
#endif  // #ifndef _WIN32

private:
	typedef typename TOperationExecutor::COperationExtraObjects CExecutorExtraObjects;

	void AdjustThreadPriority();
	void ExecuteThread();
	void DoTheDirtyJob(CExecutorExtraObjects &eoRefExecutorExtraObjects, CRWLockLockTestProgress *ptpProgressInstance);

private: // IRWLockLockTestThread
	virtual void ReleaseInstance();

private:
	CThreadParameterInfo m_piThreadParameters;
	iterationcntint		m_uiIterationCount;
	operationidxint		*m_piiOperationIndexBuffer;
	TOperationExecutor	m_oeOperationExecutor;
#ifdef _WIN32
	uintptr_t			m_hThreadHandle;
#else // #ifndef _WIN32
	pthread_t			m_hThreadHandle;
#endif  // #ifndef _WIN32
};


enum EEXECUTORLOCKOPERATIONKIND
{
	LOK__MIN,

	LOK__ENCODED_MIN = LOK__MIN,

	LOK_ACUIORE_LOCK = LOK__ENCODED_MIN,

	LOK__ENCODED_MAX,

	LOK_RELEASE_LOCK = LOK__ENCODED_MAX,

	LOK__MAX,
};

static const char g_ascLockOperationKindThreadNameFirstCharModifiers[LOK__MAX] =
{
	'\0', //  LOK_ACUIORE_LOCK,
	'a' - 'A', // LOK_RELEASE_LOCK,
};

class CRWLockValidator
{
public:
	typedef unsigned int storage_type;

	static void IncrementReads() { storage_type uiPrevLockStatus = m_uiLockStatus.fetch_add(2, std::memory_order_relaxed); MG_CHECK(uiPrevLockStatus, (uiPrevLockStatus & 1) == 0 && uiPrevLockStatus != (storage_type)0 - 2); }
	static void DecrementReads() { storage_type uiPrevLockStatus = m_uiLockStatus.fetch_sub(2, std::memory_order_relaxed); MG_CHECK(uiPrevLockStatus, (uiPrevLockStatus & 1) == 0 && uiPrevLockStatus != (storage_type)0); }

	static void IncrementWrites() { storage_type uiPrevLockStatus = m_uiLockStatus.fetch_or(1, std::memory_order_relaxed); MG_CHECK(uiPrevLockStatus, uiPrevLockStatus == 0); }
	static void DecrementWrites() { storage_type uiPrevLockStatus = m_uiLockStatus.fetch_xor(1, std::memory_order_relaxed); MG_CHECK(uiPrevLockStatus, uiPrevLockStatus == 1); }

private:
	static std::atomic<storage_type>			m_uiLockStatus;
};

/*static */std::atomic<CRWLockValidator::storage_type>		CRWLockValidator::m_uiLockStatus(0);


class CRWLockLockExecutorLogic
{
public:
	typedef CRWLockLockTestBase::operationidxint operationidxint;

	static unsigned GetOperationKindCount() { return LOK__MAX; }
	static EEXECUTORLOCKOPERATIONKIND GetOperationIndexOperationKind(operationidxint iiOperationIndex) { return (EEXECUTORLOCKOPERATIONKIND)(iiOperationIndex % LOK__MAX); }
};

template<ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTOBJECT ttoTestedObjectKind, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
class CRWLockWriteLockExecutor
{
	typedef CRWLockImplementation<trsTryReadSupport, ttoTestedObjectKind, ttlTestLanguage> implementation_type;

public:
	struct CConstructionParameter
	{
		CConstructionParameter(implementation_type &liRWLockInstance) : m_liRWLockInstance(liRWLockInstance) {}

		implementation_type			&m_liRWLockInstance;
	};

	CRWLockWriteLockExecutor(const CConstructionParameter &cpConstructionParameter) : m_liRWLockInstance(cpConstructionParameter.m_liRWLockInstance) {}

	typedef typename implementation_type::CLockWriteExtraObjects COperationExtraObjects;
	typedef CRWLockLockTestBase::iterationcntint iterationcntint;
	typedef CRWLockLockTestBase::operationidxint operationidxint;

	static operationidxint GetThreadOperationCount(iterationcntint uiIterationCount) { return (operationidxint)uiIterationCount * LOK__MAX; }

	bool ApplyLock(COperationExtraObjects &eoRefExtraObjects)
	{
		bool bLockedWithTryVariant = m_liRWLockInstance.LockRWLockWrite(eoRefExtraObjects);
		CRWLockValidator::IncrementWrites();

		return bLockedWithTryVariant;
	}

	void ReleaseLock(COperationExtraObjects &eoRefExtraObjects)
	{
		CRWLockValidator::DecrementWrites();
		m_liRWLockInstance.UnlockRWLockWrite(eoRefExtraObjects);
	}

private:
	implementation_type			&m_liRWLockInstance;
};

template<ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTOBJECT ttoTestedObjectKind, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage, unsigned int tuiReaderWriteDivisor>
class CRWLockReadLockExecutor
{
public:
	typedef CRWLockImplementation<trsTryReadSupport, ttoTestedObjectKind, ttlTestLanguage> implementation_type;

	struct CConstructionParameter
	{
		CConstructionParameter(implementation_type &liRWLockInstance) : m_liRWLockInstance(liRWLockInstance) {}

		implementation_type			&m_liRWLockInstance;
	};

	CRWLockReadLockExecutor(const CConstructionParameter &cpConstructionParameter) : m_liRWLockInstance(cpConstructionParameter.m_liRWLockInstance), m_ciLockIndex(0) {}

	typedef typename implementation_type::CLockReadExtraObjects COperationExtraObjects;
	typedef CRWLockLockTestBase::iterationcntint iterationcntint;
	typedef CRWLockLockTestBase::operationidxint operationidxint;

	static operationidxint GetThreadOperationCount(iterationcntint uiIterationCount) { return (operationidxint)uiIterationCount * LOK__MAX; }

	bool ApplyLock(COperationExtraObjects &eoRefExtraObjects)
	{
		bool bLockedWithTryVariant;

		volatile unsigned uiReaderWriteDivisor; // The volatile is necessary to avoid compiler warnings with some compilers
		if (tuiReaderWriteDivisor == 0 || (uiReaderWriteDivisor = tuiReaderWriteDivisor, ++m_ciLockIndex % uiReaderWriteDivisor != 0))
		{
			bLockedWithTryVariant = m_liRWLockInstance.LockRWLockRead(eoRefExtraObjects);
			CRWLockValidator::IncrementReads();
		}
		else
		{
			bLockedWithTryVariant = m_liRWLockInstance.LockRWLockWrite(eoRefExtraObjects);
			CRWLockValidator::IncrementWrites();
		}

		return bLockedWithTryVariant;
	}

	void ReleaseLock(COperationExtraObjects &eoRefExtraObjects)
	{
		volatile unsigned uiReaderWriteDivisor; // The volatile is necessary to avoid compiler warnings with some compilers
		if (tuiReaderWriteDivisor == 0 || (uiReaderWriteDivisor = tuiReaderWriteDivisor, m_ciLockIndex % uiReaderWriteDivisor != 0))
		{
			CRWLockValidator::DecrementReads();
			m_liRWLockInstance.UnlockRWLockRead(eoRefExtraObjects);
		}
		else
		{
			CRWLockValidator::DecrementWrites();
			m_liRWLockInstance.UnlockRWLockWrite(eoRefExtraObjects);
		}
	}

private:
	implementation_type			&m_liRWLockInstance;
	iterationcntint				m_ciLockIndex;
};


//////////////////////////////////////////////////////////////////////////

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::~CRWLockLockTestExecutor()
{
	MG_ASSERT(std::find_if(m_aittTestThreads, m_aittTestThreads + ARRAY_SIZE(m_aittTestThreads), pred_not_zero<IRWLockLockTestThread *>) == m_aittTestThreads + ARRAY_SIZE(m_aittTestThreads));
	MG_ASSERT(std::find_if(m_apiiThreadOperationIndices, m_apiiThreadOperationIndices + ARRAY_SIZE(m_apiiThreadOperationIndices), pred_not_zero<operationidxint *>) == m_apiiThreadOperationIndices + ARRAY_SIZE(m_apiiThreadOperationIndices));
	MG_ASSERT(m_pscDetailsMergeBuffer == NULL);
	MG_ASSERT(std::find_if(m_apsfOperationDetailFiles, m_apsfOperationDetailFiles + ARRAY_SIZE(m_apsfOperationDetailFiles), pred_not_zero<FILE *>) == m_apsfOperationDetailFiles + ARRAY_SIZE(m_apsfOperationDetailFiles));
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
bool CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::RunTheTask()
{
	EMGTESTFEATURELEVEL flLevelToTest = CRWLockTest::RetrieveSelectedFeatureTestLevel();

	InitializeTestResults(LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, flLevelToTest);
	AllocateThreadOperationBuffers(LOCKTEST_THREAD_COUNT);

	CThreadExecutionBarrier sbStartBarrier(LOCKTEST_THREAD_COUNT), sbFinishBarrier(LOCKTEST_THREAD_COUNT), sbExitBarrier(0);
	CRWLockLockTestProgress tpProgressInstance;
	CTimeUtils::timeduration atdObjectTestDurations[LTO__MAX];

	{
		const ERWLOCKLOCKTESTOBJECT toTestedObjectKind = LTO_SYSTEM;

#if  !_MGTEST_ANY_SHARED_MUTEX_AVAILABLE
		if (ttlTestLanguage == LTL_CPP)
		{
			atdObjectTestDurations[toTestedObjectKind] = 0;
		}
		else
#endif
		{
			CRWLockImplementation<trsTryReadSupport, toTestedObjectKind, ttlTestLanguage> liRWLock(m_uiImplementationOptions);
			AllocateTestThreads<toTestedObjectKind>(liRWLock, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, sbStartBarrier, sbFinishBarrier, sbExitBarrier, tpProgressInstance);

			WaitTestThreadsReady(sbStartBarrier);

			CTimeUtils::timepoint tpTestStartTime = CTimeUtils::GetCurrentMonotonicTimeNano();

			LaunchTheTest(sbStartBarrier);
			WaitTheTestEnd(sbFinishBarrier);

			CTimeUtils::timepoint tpTestEndTime = CTimeUtils::GetCurrentMonotonicTimeNano();

			CTimeUtils::timeduration tdTestDuration = atdObjectTestDurations[toTestedObjectKind] = tpTestEndTime - tpTestStartTime;
			PublishTestResults(toTestedObjectKind, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, flLevelToTest, tdTestDuration);

			FreeTestThreads(sbExitBarrier, LOCKTEST_THREAD_COUNT);

			sbStartBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
			sbFinishBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
			sbExitBarrier.ResetInstance(0);
			tpProgressInstance.ResetInstance();
		}
	}

	{
		const ERWLOCKLOCKTESTOBJECT toTestedObjectKind = LTO_MUTEXGEAR;

#if !_MGTEST_HAVE_CXX11
		if (ttlTestLanguage == LTL_CPP)
		{
			atdObjectTestDurations[toTestedObjectKind] = 0;
		}
		else
#endif
		{
			CRWLockImplementation<trsTryReadSupport, toTestedObjectKind, ttlTestLanguage> liRWLock(m_uiImplementationOptions);
			AllocateTestThreads<toTestedObjectKind>(liRWLock, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, sbStartBarrier, sbFinishBarrier, sbExitBarrier, tpProgressInstance);

			WaitTestThreadsReady(sbStartBarrier);

			CTimeUtils::timepoint tpTestStartTime = CTimeUtils::GetCurrentMonotonicTimeNano();

			LaunchTheTest(sbStartBarrier);
			WaitTheTestEnd(sbFinishBarrier);

			CTimeUtils::timepoint tpTestEndTime = CTimeUtils::GetCurrentMonotonicTimeNano();

			CTimeUtils::timeduration tdTestDuration = atdObjectTestDurations[toTestedObjectKind] = tpTestEndTime - tpTestStartTime;
			PublishTestResults(toTestedObjectKind, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, flLevelToTest, tdTestDuration);

			FreeTestThreads(sbExitBarrier, LOCKTEST_THREAD_COUNT);

			// sbStartBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
			// sbFinishBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
			// sbExitBarrier.ResetInstance(0);
			// tpProgressInstance.ResetInstance();
		}
	}
	MG_STATIC_ASSERT(LTO__MAX == 2);

	printf("(Sys/MG=%2lu.%.03lu/%2lu.%.03lu): ",
		(unsigned long)(atdObjectTestDurations[LTO_SYSTEM] / 1000000000), (unsigned long)(atdObjectTestDurations[LTO_SYSTEM] % 1000000000) / 1000000,
		(unsigned long)(atdObjectTestDurations[LTO_MUTEXGEAR] / 1000000000), (unsigned long)(atdObjectTestDurations[LTO_MUTEXGEAR] % 1000000000) / 1000000);
	MG_STATIC_ASSERT(LTO__MAX == 2);


	FreeThreadOperationBuffers(LOCKTEST_THREAD_COUNT);
	FinalizeTestResults(flLevelToTest);

	return true;
}


template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::AllocateThreadOperationBuffers(threadcntint ciTotalThreadCount)
{
	const operationidxint nThreadOperationCount = GetMaximalThreadOperationCount();
	SetThreadOperationCount(nThreadOperationCount);

	{
		MG_ASSERT(GetDetailsMergeBuffer() == NULL);

		char *pscDetailsMergeBuffer = new char[(size_t)nThreadOperationCount * (LOCK_THREAD_NAME_LENGTH + 1)];
		SetDetailsMergeBuffer(pscDetailsMergeBuffer);
	}

	for (threadcntint uiThreadIndex = 0; uiThreadIndex != ciTotalThreadCount; ++uiThreadIndex)
	{
		MG_ASSERT(m_apiiThreadOperationIndices[uiThreadIndex] == NULL);

		operationidxint *piiThreadOperationIndexBuffer = new operationidxint[nThreadOperationCount];
		m_apiiThreadOperationIndices[uiThreadIndex] = piiThreadOperationIndexBuffer;
	}
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::FreeThreadOperationBuffers(threadcntint ciTotalThreadCount)
{
	for (threadcntint uiThreadIndex = 0; uiThreadIndex != ciTotalThreadCount; ++uiThreadIndex)
	{
		operationidxint *piiThreadOperationIndexBuffer = m_apiiThreadOperationIndices[uiThreadIndex];

		if (piiThreadOperationIndexBuffer != NULL)
		{
			m_apiiThreadOperationIndices[uiThreadIndex] = NULL;

			delete[] piiThreadOperationIndexBuffer;
		}
	}

	char *pscDetailsMergeBuffer = GetDetailsMergeBuffer();
	if (pscDetailsMergeBuffer != NULL)
	{
		SetDetailsMergeBuffer(NULL);

		delete[] pscDetailsMergeBuffer;
	}
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
typename CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::operationidxint CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::GetMaximalThreadOperationCount() const
{
	operationidxint nThreadOperationCount;

	const iterationcntint uiIterationCount = m_uiIterationCount;
	const operationidxint iiWriteOperationCount = CRWLockWriteLockExecutor<trsTryReadSupport, LTO__MIN, ttlTestLanguage>::GetThreadOperationCount(uiIterationCount), iiReadOperationCount = CRWLockReadLockExecutor<trsTryReadSupport, LTO__MIN, ttlTestLanguage, 0>::GetThreadOperationCount(uiIterationCount);
	nThreadOperationCount = max(iiWriteOperationCount, iiReadOperationCount);

	MG_STATIC_ASSERT(LOCKTEST_THREAD_COUNT == (threadcntint)(LOCKTEST_WRITER_COUNT + LOCKTEST_READER_COUNT));

	return nThreadOperationCount;
}


template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
template<ERWLOCKLOCKTESTOBJECT ttoTestedObjectKind>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::AllocateTestThreads(CRWLockImplementation<trsTryReadSupport, ttoTestedObjectKind, ttlTestLanguage> &liRefRWLock,
	threadcntint ciWriterCount, threadcntint ciReraderCount,
	CThreadExecutionBarrier &sbRefStartBarrier, CThreadExecutionBarrier &sbRefFinishBarrier, CThreadExecutionBarrier &sbRefExitBarrier,
	CRWLockLockTestProgress &tpRefProgressInstance)
{
	const iterationcntint uiIterationCount = m_uiIterationCount;

	threadcntint uiThreadIndex = 0;

	typename CRWLockWriteLockExecutor<trsTryReadSupport, ttoTestedObjectKind, ttlTestLanguage>::CConstructionParameter cpWriteExecutorParameter(liRefRWLock);

	const threadcntint ciWritersEndIndex = uiThreadIndex + ciWriterCount;
	for (; uiThreadIndex != ciWritersEndIndex; ++uiThreadIndex)
	{
		MG_ASSERT(m_aittTestThreads[uiThreadIndex] == NULL);

		typedef CRWLockLockTestThread<CRWLockWriteLockExecutor<trsTryReadSupport, ttoTestedObjectKind, ttlTestLanguage> > CUsedRWLockLockTestThread;
		typedef typename CUsedRWLockLockTestThread::CThreadParameterInfo CUsedThreadParameterInfo;
		CUsedRWLockLockTestThread *pttThreadInstance = new CUsedRWLockLockTestThread(CUsedThreadParameterInfo(sbRefStartBarrier, sbRefFinishBarrier, sbRefExitBarrier, tpRefProgressInstance), uiIterationCount, cpWriteExecutorParameter, m_apiiThreadOperationIndices[uiThreadIndex]);
		m_aittTestThreads[uiThreadIndex] = pttThreadInstance;
	}

	typename CRWLockReadLockExecutor<trsTryReadSupport, ttoTestedObjectKind, ttlTestLanguage, tuiReaderWriteDivisor>::CConstructionParameter cpReadExecutorParameter(liRefRWLock);

	const threadcntint ciReadersEndIndex = uiThreadIndex + ciReraderCount;
	for (; uiThreadIndex != ciReadersEndIndex; ++uiThreadIndex)
	{
		MG_ASSERT(m_aittTestThreads[uiThreadIndex] == NULL);

		typedef CRWLockLockTestThread<CRWLockReadLockExecutor<trsTryReadSupport, ttoTestedObjectKind, ttlTestLanguage, tuiReaderWriteDivisor> > CUsedRWLockLockTestThread;
		typedef typename CUsedRWLockLockTestThread::CThreadParameterInfo CUsedThreadParameterInfo;
		CUsedRWLockLockTestThread *pttThreadInstance = new CUsedRWLockLockTestThread(CUsedThreadParameterInfo(sbRefStartBarrier, sbRefFinishBarrier, sbRefExitBarrier, tpRefProgressInstance), uiIterationCount, cpReadExecutorParameter, m_apiiThreadOperationIndices[uiThreadIndex]);
		m_aittTestThreads[uiThreadIndex] = pttThreadInstance;
	}
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::FreeTestThreads(CThreadExecutionBarrier &sbRefExitBarrier, threadcntint ciTotalThreadCount)
{
	sbRefExitBarrier.SignalReleaseEvent();

	for (threadcntint uiThreadIndex = 0; uiThreadIndex != ciTotalThreadCount; ++uiThreadIndex)
	{
		IRWLockLockTestThread *ittCurrentTestThread = m_aittTestThreads[uiThreadIndex];

		if (ittCurrentTestThread != NULL)
		{
			m_aittTestThreads[uiThreadIndex] = NULL;

			ittCurrentTestThread->ReleaseInstance();
		}
	}
}


template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::WaitTestThreadsReady(CThreadExecutionBarrier &sbStartBarrier)
{
	while (sbStartBarrier.RetrieveCounterValue() != 0)
	{
		CTimeUtils::Sleep(1);
	}

	// Sleep a longer period for all the threads to enter blocked state 
	// and the system to finish any disk cache flush activity from previous tests
	CTimeUtils::Sleep(g_uiSettleDownSleepDelay);
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::LaunchTheTest(CThreadExecutionBarrier &sbStartBarrier)
{
	sbStartBarrier.SignalReleaseEvent();
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::WaitTheTestEnd(CThreadExecutionBarrier &sbFinishBarrier)
{
	sbFinishBarrier.WaitReleaseEvent();
}


template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::InitializeTestResults(threadcntint ciWriterCount, threadcntint ciReraderCount,
	EMGTESTFEATURELEVEL flTestLevel)
{
	int iFileOpenStatus;
	char ascFileNameFormatBuffer[256];

	volatile unsigned uiReaderWriteDivisor; // The volatile is necessary to avoid a compile error in VS2013
	const unsigned uiWritesPercent = tuiReaderWriteDivisor != 0 ? (uiReaderWriteDivisor = tuiReaderWriteDivisor, 100 / uiReaderWriteDivisor) : 0;

	for (ERWLOCKLOCKTESTOBJECT toTestedObjectKind = LTO__MIN; toTestedObjectKind != LTO__MAX; ++toTestedObjectKind)
	{
		MG_ASSERT(m_apsfOperationDetailFiles[toTestedObjectKind] == NULL);

		if (IN_RANGE(flTestLevel, MGTFL__DUMP_MIN, MGTFL__DUMP_MAX))
		{
			const char *szTRDLSupportFileNameSuffix = g_aszTRDLSupportFileNameSuffixes[trsTryReadSupport];
			const char *szObjectKindFileNameSuffix = g_aszTestedObjectKindFileNameSuffixes[toTestedObjectKind];
			const char *szLanguageNameSuffix = g_aszTestedObjectLanguageNames[ttlTestLanguage];
			int iPrintResult = snprintf(ascFileNameFormatBuffer, sizeof(ascFileNameFormatBuffer), g_ascLockTestDetailsFileNameFormat,
				szTRDLSupportFileNameSuffix, tuiReaderWriteDivisor == 0 ? (unsigned int)ciWriterCount : (unsigned int)uiWritesPercent, tuiReaderWriteDivisor == 0 ? "" : "%", (unsigned int)ciReraderCount,
				szObjectKindFileNameSuffix, szLanguageNameSuffix);
			MG_CHECK(iPrintResult, IN_RANGE(iPrintResult - 1, 0, ARRAY_SIZE(ascFileNameFormatBuffer) - 1) || (iPrintResult < 0 && (iPrintResult = -errno, false)));

			FILE *psfObjectKindDetailsFile = fopen(ascFileNameFormatBuffer, "w+");
			MG_CHECK(iFileOpenStatus, psfObjectKindDetailsFile != NULL || (iFileOpenStatus = errno, false));

			m_apsfOperationDetailFiles[toTestedObjectKind] = psfObjectKindDetailsFile;
		}
	}
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::PublishTestResults(ERWLOCKLOCKTESTOBJECT toTestedObjectKind,
	threadcntint ciWriterCount, threadcntint ciReaderCount, EMGTESTFEATURELEVEL flTestLevel, CTimeUtils::timeduration tdTestDuration)
{
	MG_ASSERT(IN_RANGE(toTestedObjectKind, LTO__MIN, LTO__MAX));

	if (IN_RANGE(flTestLevel, MGTFL__DUMP_MIN, MGTFL__DUMP_MAX))
	{
		volatile unsigned uiReaderWriteDivisor; // The volatile is necessary to avoid a compile error in VS2013
		const unsigned uiWritesPercent = tuiReaderWriteDivisor != 0 ? (uiReaderWriteDivisor = tuiReaderWriteDivisor, 100 / uiReaderWriteDivisor) : 0;

		FILE *psfDetailsFile = m_apsfOperationDetailFiles[toTestedObjectKind];
		int iPrintResult = fprintf(psfDetailsFile, "Lock-unlock test for %s with %u%s write%s%s, %u %s%s\nTime taken: %lu sec %lu nsec\n\nOperation sequence map:\n",
			g_aszTestedObjectLanguageNames[ttlTestLanguage],
			tuiReaderWriteDivisor == 0 ? (unsigned int)ciWriterCount : (unsigned int)uiWritesPercent, tuiReaderWriteDivisor == 0 ? "" : "%",
			tuiReaderWriteDivisor == 0 ? "r" : "s", tuiReaderWriteDivisor == 0 && ciWriterCount != 1 ? "s" : "", (unsigned int)ciReaderCount,
			tuiReaderWriteDivisor == 0 ? "reader" : "thread", ciReaderCount != 1 ? "s" : "",
			(unsigned long)(tdTestDuration / 1000000000), (unsigned long)(tdTestDuration % 1000000000));
		MG_CHECK(iPrintResult, iPrintResult > 0 || (iPrintResult = errno, false));

		const threadcntint ciTotalThreadCount = ciWriterCount + ciReaderCount;
		operationidxint *piiThreadLastOperationIndices = new operationidxint[ciTotalThreadCount];
		std::fill(piiThreadLastOperationIndices, piiThreadLastOperationIndices + ciTotalThreadCount, 0);

		const operationidxint nThreadOperationCount = GetThreadOperationCount();
		operationidxint iiLastSavedOperationIndex = 0;
		char *pscMergeBuffer = GetDetailsMergeBuffer();
		for (threadcntint ciCurrentBufferIndex = 0; ciCurrentBufferIndex != ciTotalThreadCount; ++ciCurrentBufferIndex)
		{
			BuildMergedThreadOperationMap(pscMergeBuffer, iiLastSavedOperationIndex, nThreadOperationCount, piiThreadLastOperationIndices, ciWriterCount, ciReaderCount);
			SaveThreadOperationMapToDetails(toTestedObjectKind, pscMergeBuffer, nThreadOperationCount);
			iiLastSavedOperationIndex += nThreadOperationCount;
		}

		delete[] piiThreadLastOperationIndices;
	}
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::BuildMergedThreadOperationMap(char ascMergeBuffer[], operationidxint iiLastSavedOperationIndex, operationidxint iiThreadOperationCount,
	operationidxint aiiThreadLastOperationIndices[], threadcntint ciWriterCount, threadcntint ciReaderCount)
{
	char ascCurrentThreadMapChars[LOCK_THREAD_NAME_LENGTH + 1]; // +1 for terminal zero
	threadcntint ciCurrentThreadIndex = 0;

	InitializeThreadName(ascCurrentThreadMapChars, g_cWriterMapFirstChar);
	const threadcntint ciWriterThreadsBegin = ciCurrentThreadIndex, ciWriterThreadsEnd = ciCurrentThreadIndex + ciWriterCount;
	for (; ciCurrentThreadIndex != ciWriterThreadsEnd; )
	{
		const operationidxint *piiThreadOperationIndices = m_apiiThreadOperationIndices[ciCurrentThreadIndex];
		aiiThreadLastOperationIndices[ciCurrentThreadIndex] = FillThreadOperationMap(ascMergeBuffer, iiLastSavedOperationIndex, iiThreadOperationCount, piiThreadOperationIndices, aiiThreadLastOperationIndices[ciCurrentThreadIndex], ascCurrentThreadMapChars);

		++ciCurrentThreadIndex;
		IncrementThreadName(ascCurrentThreadMapChars, ciCurrentThreadIndex - ciWriterThreadsBegin);
	}

	InitializeThreadName(ascCurrentThreadMapChars, g_cReaderMapFirstChar);
	const threadcntint ciReaderThreadsBegin = ciCurrentThreadIndex, ciReaderThreadsEnd = ciCurrentThreadIndex + ciReaderCount;
	for (; ciCurrentThreadIndex != ciReaderThreadsEnd; )
	{
		const operationidxint *piiThreadOperationIndices = m_apiiThreadOperationIndices[ciCurrentThreadIndex];
		aiiThreadLastOperationIndices[ciCurrentThreadIndex] = FillThreadOperationMap(ascMergeBuffer, iiLastSavedOperationIndex, iiThreadOperationCount, piiThreadOperationIndices, aiiThreadLastOperationIndices[ciCurrentThreadIndex], ascCurrentThreadMapChars);

		++ciCurrentThreadIndex;
		IncrementThreadName(ascCurrentThreadMapChars, ciCurrentThreadIndex - ciReaderThreadsBegin);
	}
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
typename CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::operationidxint CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::FillThreadOperationMap(
	char ascMergeBuffer[], operationidxint iiLastSavedOperationIndex, const operationidxint iiThreadOperationCount,
	const operationidxint *piiThreadOperationIndices, operationidxint iiThreadLastOperationIndex, const char ascThreadMapChars[LOCK_THREAD_NAME_LENGTH])
{
	const unsigned uiPerIndexOperationKindCount = CRWLockLockExecutorLogic::GetOperationKindCount();

	const operationidxint iiAcceptableOperationSequencesEnd = iiLastSavedOperationIndex + iiThreadOperationCount;
	operationidxint iiThreadCurrentOperationIndex = iiThreadLastOperationIndex;
	for (; iiThreadCurrentOperationIndex != iiThreadOperationCount; ++iiThreadCurrentOperationIndex)
	{
		operationidxint iiRawThreadOperationSequence = piiThreadOperationIndices[iiThreadCurrentOperationIndex];
		EEXECUTORLOCKOPERATIONKIND okOperationKind = CRWLockLockExecutorLogic::GetOperationIndexOperationKind(iiThreadCurrentOperationIndex);

		bool bTryVariantWasUsed = false;
		operationidxint iiThreadOperationSequence = iiRawThreadOperationSequence;
		if (IN_RANGE(okOperationKind, LOK__ENCODED_MIN, LOK__ENCODED_MAX))
		{
			bTryVariantWasUsed = DECODE_ENCODED_TRYVARIANT(iiRawThreadOperationSequence);
			iiThreadOperationSequence = DECODE_ENCODED_SEQUENCE(iiRawThreadOperationSequence);
		}

		if (iiThreadOperationSequence >= iiAcceptableOperationSequencesEnd)
		{
			break;
		}

		size_t siNameInsertOffset = (size_t)(iiThreadOperationSequence - iiLastSavedOperationIndex) * (LOCK_THREAD_NAME_LENGTH + 1);
		ascMergeBuffer[siNameInsertOffset + LOCK_THREAD_NAME_LENGTH] = !bTryVariantWasUsed ? g_cNormalVariantOperationDetailMapSeparatorChar : g_cTryVariantOperationDetailMapSeparatorChar;

		std::copy(ascThreadMapChars, ascThreadMapChars + LOCK_THREAD_NAME_LENGTH, ascMergeBuffer + siNameInsertOffset);

		if (tuiReaderWriteDivisor != 0 && ascThreadMapChars[0] >= g_cReaderMapFirstChar)
		{
			operationidxint iiThreadReducedOperationIndex = iiThreadCurrentOperationIndex / uiPerIndexOperationKindCount;

			volatile unsigned uiReaderWriteDivisor = tuiReaderWriteDivisor; // The volatile is necessary to avoid compiler warnings with some compilers
			if ((iiThreadReducedOperationIndex + 1) % uiReaderWriteDivisor == 0)
			{
				ascMergeBuffer[siNameInsertOffset] -= g_cReaderMapFirstChar - g_cWriterMapFirstChar;
			}
		}

		ascMergeBuffer[siNameInsertOffset] += g_ascLockOperationKindThreadNameFirstCharModifiers[okOperationKind];
	}

	return iiThreadCurrentOperationIndex;
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::InitializeThreadName(char ascThreadMapChars[LOCK_THREAD_NAME_LENGTH + 1], char cMapFirstChar)
{
	ascThreadMapChars[0] = cMapFirstChar;
	int iPrintResult = sprintf(ascThreadMapChars + 1, "%0*x", LOCK_THREAD_NAME_LENGTH - 1, (unsigned int)0);
	MG_CHECK(iPrintResult, iPrintResult == LOCK_THREAD_NAME_LENGTH - 1 || (iPrintResult < 0 && (iPrintResult = -errno, false)));
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::IncrementThreadName(char ascThreadMapChars[LOCK_THREAD_NAME_LENGTH + 1], threadcntint ciThreadIndex)
{
	threadcntint ciThreadIndexNumericPart = ciThreadIndex % ((threadcntint)1 << (LOCK_THREAD_NAME_LENGTH - 1) * 4); // 4 bits per a hexadecimal digit
	if (ciThreadIndexNumericPart == 0)
	{
		++ascThreadMapChars[0];
	}

	int iPrintResult = sprintf(ascThreadMapChars + 1, "%0*x", LOCK_THREAD_NAME_LENGTH - 1, (unsigned int)ciThreadIndexNumericPart);
	MG_CHECK(iPrintResult, iPrintResult == LOCK_THREAD_NAME_LENGTH - 1 || (iPrintResult < 0 && (iPrintResult = -errno, false)));
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::SaveThreadOperationMapToDetails(ERWLOCKLOCKTESTOBJECT toTestedObjectKind,
	const char ascMergeBuffer[], operationidxint iiThreadOperationCount)
{
	MG_ASSERT(iiThreadOperationCount % g_uiLockOperationsPerLine == 0);

	FILE *psfDetailsFile = m_apsfOperationDetailFiles[toTestedObjectKind];

	const size_t siBufferEnndOffset = (size_t)iiThreadOperationCount * (LOCK_THREAD_NAME_LENGTH + 1);
	for (size_t siCurrentOffset = 0; siCurrentOffset != siBufferEnndOffset; siCurrentOffset += g_uiLockOperationsPerLine * (LOCK_THREAD_NAME_LENGTH + 1))
	{
		int iPrintResult = fprintf(psfDetailsFile, "%.*s\n", (int)(g_uiLockOperationsPerLine * (LOCK_THREAD_NAME_LENGTH + 1)), ascMergeBuffer + siCurrentOffset);
		MG_CHECK(iPrintResult, iPrintResult > 0 || (iPrintResult = -errno, false));
	}
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, trsTryReadSupport, ttlTestLanguage>::FinalizeTestResults(EMGTESTFEATURELEVEL flTestLevel)
{
	for (ERWLOCKLOCKTESTOBJECT toTestedObjectKind = LTO__MIN; toTestedObjectKind != LTO__MAX; ++toTestedObjectKind)
	{
		if (IN_RANGE(flTestLevel, MGTFL__DUMP_MIN, MGTFL__DUMP_MAX))
		{
			FILE *psfObjectKindDetailsFile = m_apsfOperationDetailFiles[toTestedObjectKind];

			if (psfObjectKindDetailsFile != NULL)
			{
				m_apsfOperationDetailFiles[toTestedObjectKind] = NULL;

				fclose(psfObjectKindDetailsFile);
			}
		}

		MG_ASSERT(m_apsfOperationDetailFiles[toTestedObjectKind] == NULL);
	}
}

//////////////////////////////////////////////////////////////////////////
// CRWLockLockTestThread

template<class TOperationExecutor>
/*virtual */
CRWLockLockTestThread<TOperationExecutor>::~CRWLockLockTestThread()
{
	StopRunningThread();
}


template<class TOperationExecutor>
void CRWLockLockTestThread<TOperationExecutor>::StartRunningThread()
{
#ifdef _WIN32

	int iThreadStartResult;
	uintptr_t uiThreadHandle = _beginthreadex(NULL, 0, &StaticExecuteThread, this, 0, NULL);
	MG_CHECK(iThreadStartResult, uiThreadHandle != 0 || (iThreadStartResult = errno, false));

	m_hThreadHandle = uiThreadHandle;


#else // #ifndef _WIN32

	int iThreadStartResult;
	MG_CHECK(iThreadStartResult, (iThreadStartResult = pthread_create(&m_hThreadHandle, NULL, &StaticExecuteThread, this)) == EOK);

#endif  // #ifndef _WIN32
}

template<class TOperationExecutor>
void CRWLockLockTestThread<TOperationExecutor>::StopRunningThread()
{
#ifdef _WIN32

	int iThreadWaitResult;
	MG_CHECK(iThreadWaitResult, (iThreadWaitResult = WaitForSingleObject((HANDLE)m_hThreadHandle, INFINITE)) == WAIT_OBJECT_0);

	CloseHandle((HANDLE)m_hThreadHandle);


#else // #ifndef _WIN32

	int iThreadWaitResult;
	MG_CHECK(iThreadWaitResult, (iThreadWaitResult = pthread_join(m_hThreadHandle, NULL)) == 0);

#endif  // #ifndef _WIN32
}


template<class TOperationExecutor>
/*static */
#ifdef _WIN32
unsigned __stdcall CRWLockLockTestThread<TOperationExecutor>::StaticExecuteThread(void *psvThreadParameter)
#else // #ifndef _WIN32
void *CRWLockLockTestThread<TOperationExecutor>::StaticExecuteThread(void *psvThreadParameter)
#endif  // #ifndef _WIN32
{
	CRWLockLockTestThread *pttThreadInstance = (CRWLockLockTestThread *)psvThreadParameter;
	pttThreadInstance->AdjustThreadPriority();
	pttThreadInstance->ExecuteThread();

	return 0;
}


template<class TOperationExecutor>
void CRWLockLockTestThread<TOperationExecutor>::AdjustThreadPriority()
{
	int iPriorityIncreaseResult = _mutexgear_set_current_thread_high();

	if (iPriorityIncreaseResult != 0)
	{
		if (_mg_atomic_fetch_add_relaxed_uint(&g_uiPriorityAdjustmentErrorReported, iPriorityIncreaseResult) != 0)
		{
			_mg_atomic_fetch_sub_relaxed_uint(&g_uiPriorityAdjustmentErrorReported, iPriorityIncreaseResult);
		}
	}
}

template<class TOperationExecutor>
void CRWLockLockTestThread<TOperationExecutor>::ExecuteThread()
{
	// Execute initialization code in the constructor...
	CExecutorExtraObjects eoExecutorExtraObjects;

	bool bWasLastToStart;
	m_piThreadParameters.m_psbStartBarrier->DecreaseCounterValue(bWasLastToStart);
	// Ignore the bWasLastToStart -- the main thread polls the counter

	m_piThreadParameters.m_psbStartBarrier->WaitReleaseEvent();

	DoTheDirtyJob(eoExecutorExtraObjects, m_piThreadParameters.m_ptpProgressInstance);

	bool bWasLastToFinish;
	m_piThreadParameters.m_psbFinishBarrier->DecreaseCounterValue(bWasLastToFinish);

	if (bWasLastToFinish)
	{
		m_piThreadParameters.m_psbFinishBarrier->SignalReleaseEvent();
	}

	m_piThreadParameters.m_psbExitBarrier->WaitReleaseEvent();
}

template<class TOperationExecutor>
void CRWLockLockTestThread<TOperationExecutor>::DoTheDirtyJob(CExecutorExtraObjects &eoRefExecutorExtraObjects, CRWLockLockTestProgress *ptpProgressInstance)
{
	volatile int iRandomNumber;
	operationidxint *piiOperationIndexBuffer = m_piiOperationIndexBuffer;

	const iterationcntint uiIterationCount = m_uiIterationCount;
	for (iterationcntint uiIterationIndex = 0; uiIterationIndex != uiIterationCount; ++uiIterationIndex)
	{
		bool bLockedWitTryVariant = m_oeOperationExecutor.ApplyLock(eoRefExecutorExtraObjects);

		operationidxint iiLockOperationSequence = ptpProgressInstance->GenerateOperationIndex();
		*piiOperationIndexBuffer++ = ENCODE_SEQUENCE_WITH_TRYVARIANT(iiLockOperationSequence, bLockedWitTryVariant);

		iRandomNumber = rand(); // Generate a random number to simulate a small delay

		operationidxint iiUnlockOperationSequence = ptpProgressInstance->GenerateOperationIndex();
		*piiOperationIndexBuffer++ = iiUnlockOperationSequence;

		m_oeOperationExecutor.ReleaseLock(eoRefExecutorExtraObjects);
		MG_STATIC_ASSERT(LOK__MAX == 2);
	}
}


template<class TOperationExecutor>
/*virtual */
void CRWLockLockTestThread<TOperationExecutor>::ReleaseInstance()
{
	delete this;
}


//////////////////////////////////////////////////////////////////////////
// RWLock

enum EMGRWLOCKFEATURE
{
	MGWLF__MIN,

	MGWLF_1TW_C = MGWLF__MIN,
	MGWLF_4TW_C,
	MGWLF_8TW_C,
	MGWLF_16TW_C,

	MGWLF_1TR_C,
	MGWLF_4TR_C,
	MGWLF_16TR_C,
	MGWLF_64TR_C,

	MGWLF_8T_12PW_C,
	MGWLF_16T_12PW_C,
	MGWLF_32T_12PW_C,
	MGWLF_64T_12PW_C,

	MGWLF_8T_25PW_C,
	MGWLF_16T_25PW_C,
	MGWLF_32T_25PW_C,
	MGWLF_64T_25PW_C,

#if _MGTEST_HAVE_CXX11
	MGWLF_16T_25PW_CPP,
#endif // #if _MGTEST_HAVE_CXX11

	MGWLF_8T_50PW_C,
	MGWLF_16T_50PW_C,
	MGWLF_32T_50PW_C,
	MGWLF_64T_50PW_C,

	MGWLF_1TW_1TR_C,
	MGWLF_1TW_4TR_C,
	MGWLF_1TW_8TR_C,
	MGWLF_1TW_16TR_C,

	MGWLF_4TW_4TR_C,
	MGWLF_4TW_8TR_C,
	MGWLF_4TW_16TR_C,
	MGWLF_4TW_32TR_C,

	MGWLF_8TW_16TR_C,
	MGWLF_8TW_32TR_C,
	MGWLF_8TW_64TR_C,
	MGWLF_8TW_128TR_C,

	MGWLF__MAX,

	MGWLF__TESTBEGIN = MGWLF__MIN,
	MGWLF__TESTEND = MGWLF__MAX,
	MGWLF__TESTCOUNT = MGWLF__TESTEND - MGWLF__TESTBEGIN,
};
MG_STATIC_ASSERT(MGWLF__TESTBEGIN <= MGWLF__TESTEND);


typedef bool(*CRWLockFeatureTestProcedure)();

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
bool TestRWLockLocks()
{
	CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, 0, trsTryReadSupport, ttlTestLanguage> ltTestInstance(g_uiLockIterationCount, 0);
	return ltTestInstance.RunTheTask();
}

template<unsigned int tuiThreadCount, unsigned int tuiWriteDivivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
bool TestRWLockMixed()
{
	CRWLockLockTestExecutor<0, tuiThreadCount, tuiWriteDivivisor, trsTryReadSupport, ttlTestLanguage> ltTestInstance(g_uiLockIterationCount, tuiImplementationOptions);
	return ltTestInstance.RunTheTask();
}


static const EMGTESTFEATURELEVEL g_aflRWLockFeatureTestLevels[] =
{
	MGTFL_BASIC, // MGWLF_1TW_C,
	MGTFL_BASIC, // MGWLF_4TW_C,
	MGTFL_QUICK, // MGWLF_8TW_C,
	MGTFL_BASIC, // MGWLF_16TW_C,

	MGTFL_BASIC, // MGWLF_1TR_C,
	MGTFL_BASIC, // MGWLF_4TR_C,
	MGTFL_QUICK, // MGWLF_16TR_C,
	MGTFL_BASIC, // MGWLF_64TR_C,

	MGTFL_BASIC, // MGWLF_8T_12PW_C,
	MGTFL_BASIC, // MGWLF_16T_12PW_C,
	MGTFL_BASIC, // MGWLF_32T_12PW_C,
	MGTFL_BASIC, // MGWLF_64T_12PW_C,

	MGTFL_BASIC, // MGWLF_8T_25PW_C,
#if _MGTEST_HAVE_CXX11
	MGTFL_BASIC, // MGWLF_16T_25PW_C,
#else
	MGTFL_QUICK, // MGWLF_16T_25PW_C,
#endif // #if _MGTEST_HAVE_CXX11
	MGTFL_BASIC, // MGWLF_32T_25PW_C,
	MGTFL_QUICK, // MGWLF_64T_25PW_C, // This is to include LIOPT_MULTIPLE_WRITE_CHANNELS into the quick test

#if _MGTEST_HAVE_CXX11
	MGTFL_QUICK, // MGWLF_16T_25PW_CPP,
#endif // #if _MGTEST_HAVE_CXX11

	MGTFL_EXTRA, // MGWLF_8T_50PW_C,
	MGTFL_EXTRA, // MGWLF_16T_50PW_C,
	MGTFL_EXTRA, // MGWLF_32T_50PW_C,
	MGTFL_EXTRA, // MGWLF_64T_50PW_C,

	MGTFL_BASIC, // MGWLF_1TW_1TR_C,
	MGTFL_BASIC, // MGWLF_1TW_4TR_C,
	MGTFL_BASIC, // MGWLF_1TW_8TR_C,
	MGTFL_BASIC, // MGWLF_1TW_16TR_C,

	MGTFL_BASIC, // MGWLF_4TW_4TR_C,
	MGTFL_BASIC, // MGWLF_4TW_8TR_C,
	MGTFL_BASIC, // MGWLF_4TW_16TR_C,
	MGTFL_BASIC, // MGWLF_4TW_32TR_C,

	MGTFL_EXTRA, // MGWLF_8TW_16TR_C,
	MGTFL_EXTRA, // MGWLF_8TW_32TR_C,
	MGTFL_EXTRA, // MGWLF_8TW_64TR_C,
	MGTFL_EXTRA, // MGWLF_8TW_128TR_C,
};
MG_STATIC_ASSERT(ARRAY_SIZE(g_aflRWLockFeatureTestLevels) == MGWLF__MAX);

static const CRWLockFeatureTestProcedure g_afnRWLockFeatureTestProcedures[] =
{
	&TestRWLockLocks<1, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_C,
	&TestRWLockLocks<4, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_C,
	&TestRWLockLocks<8, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_C,
	&TestRWLockLocks<16, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16TW_C,

	&TestRWLockLocks<0, 1, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TR_C,
	&TestRWLockLocks<0, 4, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TR_C,
	&TestRWLockLocks<0, 16, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16TR_C,
	&TestRWLockLocks<0, 64, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_64TR_C,

	&TestRWLockMixed<8, 8, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8T_12PW_C,
	&TestRWLockMixed<16, 8, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16T_12PW_C,
	&TestRWLockMixed<32, 8, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_32T_12PW_C,
	&TestRWLockMixed<64, 8, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_64T_12PW_C,

	&TestRWLockMixed<8, 4, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8T_25PW_C,
	&TestRWLockMixed<16, 4, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16T_25PW_C,
	&TestRWLockMixed<32, 4, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_32T_25PW_C,
	&TestRWLockMixed<64, 4, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_64T_25PW_C,

#if _MGTEST_HAVE_CXX11
	&TestRWLockMixed<16, 4, 0, TRS_NO_TRYREAD_SUPPORT, LTL_CPP>, // MGWLF_16T_25PW_CPP,
#endif // #if _MGTEST_HAVE_CXX11

	&TestRWLockMixed<8, 2, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8T_50PW_C,
	&TestRWLockMixed<16, 2, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16T_50PW_C,
	&TestRWLockMixed<32, 2, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_32T_50PW_C,
	&TestRWLockMixed<64, 2, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_64T_50PW_C,

	&TestRWLockLocks<1, 1, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_1TR_C,
	&TestRWLockLocks<1, 4, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_4TR_C,
	&TestRWLockLocks<1, 8, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_8TR_C,
	&TestRWLockLocks<1, 16, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_16TR_C,

	&TestRWLockLocks<4, 4, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_4TR_C,
	&TestRWLockLocks<4, 8, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_8TR_C,
	&TestRWLockLocks<4, 16, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_16TR_C,
	&TestRWLockLocks<4, 32, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_32TR_C,

	&TestRWLockLocks<8, 16, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_16TR_C,
	&TestRWLockLocks<8, 32, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_32TR_C,
	&TestRWLockLocks<8, 64, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_64TR_C,
	&TestRWLockLocks<8, 128, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_128TR_C,
};
MG_STATIC_ASSERT(ARRAY_SIZE(g_afnRWLockFeatureTestProcedures) == MGWLF__MAX);

static const char *const g_aszRWLockFeatureTestNames[] =
{
	"1 Writer, C", // MGWLF_1TW_C,
	"4 Writers, C", // MGWLF_4TW_C,
	"8 Writers, C", // MGWLF_8TW_C,
	"16 Writers, C", // MGWLF_16TW_C,

	"1 Reader, C", // MGWLF_1TR_C,
	"4 Readers, C", // MGWLF_4TR_C,
	"16 Readers, C", // MGWLF_16TR_C,
	"64 Readers, C", // MGWLF_64TR_C,

	"12% writes, 8 threads, C", // MGWLF_8T_12PW_C,
	"12% writes, 16 threads, C", // MGWLF_16T_12PW_C,
	"12% writes, 32 threads, C", // MGWLF_32T_12PW_C,
	"12% writes @" MAKE_STRING_LITERAL(MGTEST_RWLOCK_WRITE_CHANNELS) "cnl, 64 threads, C", // MGWLF_64T_12PW_C,

	"25% writes, 8 threads, C", // MGWLF_8T_25PW_C,
	"25% writes, 16 threads, C", // MGWLF_16T_25PW_C,
	"25% writes, 32 threads, C", // MGWLF_32T_25PW_C,
	"25% writes @" MAKE_STRING_LITERAL(MGTEST_RWLOCK_WRITE_CHANNELS) "cnl, 64 threads, C", // MGWLF_64T_25PW_C,

#if _MGTEST_HAVE_CXX11
	"25% writes, 16 threads, C++", // MGWLF_16T_25PW_CPP,
#endif // #if _MGTEST_HAVE_CXX11

	"50% writes, 8 threads, C",  // MGWLF_8T_50PW_C,
	"50% writes, 16 threads, C", // MGWLF_16T_50PW_C,
	"50% writes, 32 threads, C", // MGWLF_32T_50PW_C,
	"50% writes @" MAKE_STRING_LITERAL(MGTEST_RWLOCK_WRITE_CHANNELS) "cnl, 64 threads, C", // MGWLF_64T_50PW_C,

	"1 Writer, 1 Reader, C", // MGWLF_1TW_1TR_C,
	"1 Writer, 4 Readers, C", // MGWLF_1TW_4TR_C,
	"1 Writer, 8 Readers, C", // MGWLF_1TW_8TR_C,
	"1 Writer, 16 Readers, C", // MGWLF_1TW_16TR_C,

	"4 Writers, 4 Readers, C", // MGWLF_4TW_4TR_C,
	"4 Writers, 8 Readers, C", // MGWLF_4TW_8TR_C,
	"4 Writers, 16 Readers, C", // MGWLF_4TW_16TR_C,
	"4 Writers, 32 Readers, C", // MGWLF_4TW_32TR_C,

	"8 Writers, 16 Readers, C", // MGWLF_8TW_16TR_C,
	"8 Writers, 32 Readers, C", // MGWLF_8TW_32TR_C,
	"8 Writers, 64 Readers, C", // MGWLF_8TW_64TR_C,
	"8 Writers, 128 Readers, C", // MGWLF_8TW_128TR_C,
};
MG_STATIC_ASSERT(ARRAY_SIZE(g_aszRWLockFeatureTestNames) == MGWLF__MAX);


/*static */EMGTESTFEATURELEVEL	CRWLockTest::m_flSelectedFeatureTestLevel = MGTFL__DEFAULT;


/*static */
bool CRWLockTest::RunBasicImplementationTest(unsigned int &nOutSuccessCount, unsigned int &nOutTestCount)
{
	unsigned int nSuccessCount = 0, nTestCount = 0;

	_mg_atomic_init_uint(&g_uiPriorityAdjustmentErrorReported, 0);

	EMGTESTFEATURELEVEL flLevelToTest = CRWLockTest::RetrieveSelectedFeatureTestLevel();

	for (EMGRWLOCKFEATURE lfRWLockFeature = MGWLF__TESTBEGIN; lfRWLockFeature != MGWLF__TESTEND; ++lfRWLockFeature)
	{
		if (g_aflRWLockFeatureTestLevels[lfRWLockFeature] <= flLevelToTest)
		{
			++nTestCount;

			const char *szFeatureName = g_aszRWLockFeatureTestNames[lfRWLockFeature];
			printf("Testing %32s: ", szFeatureName);

			CRWLockFeatureTestProcedure fnTestProcedure = g_afnRWLockFeatureTestProcedures[lfRWLockFeature];
			bool bTestResult = fnTestProcedure();
			printf("%s\n", bTestResult ? "success" : "failure");

			if (bTestResult)
			{
				nSuccessCount += 1;
			}
		}
	}

	int iPriorityIncreaseResult;
	if ((iPriorityIncreaseResult = _mg_atomic_load_relaxed_uint(&g_uiPriorityAdjustmentErrorReported)) != 0)
	{
		printf(g_ascPriorityAdjustmentErrorWarningFormat, iPriorityIncreaseResult);
	}

	nOutSuccessCount = nSuccessCount;
	nOutTestCount = nTestCount;
	MG_ASSERT(nTestCount <= MGWLF__TESTCOUNT);

	return nSuccessCount == nTestCount;
}


#define g_aflTRDLRWLockFeatureTestLevels g_aflRWLockFeatureTestLevels
#define g_aszTRDLRWLockFeatureTestNames g_aszRWLockFeatureTestNames

static const CRWLockFeatureTestProcedure g_afnTRDLRWLockFeatureTestProcedures[] =
{
	&TestRWLockLocks<1, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_C,
	&TestRWLockLocks<4, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_C,
	&TestRWLockLocks<8, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_C,
	&TestRWLockLocks<16, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16TW_C,

	&TestRWLockLocks<0, 1, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TR_C,
	&TestRWLockLocks<0, 4, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TR_C,
	&TestRWLockLocks<0, 16, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16TR_C,
	&TestRWLockLocks<0, 64, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_64TR_C,

	&TestRWLockMixed<8, 8, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8T_12PW_C,
	&TestRWLockMixed<16, 8, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16T_12PW_C,
	&TestRWLockMixed<32, 8, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_32T_12PW_C,
	&TestRWLockMixed<64, 8, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_64T_12PW_C,

	&TestRWLockMixed<8, 4, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8T_25PW_C,
	&TestRWLockMixed<16, 4, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16T_25PW_C,
	&TestRWLockMixed<32, 4, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_32T_25PW_C,
	&TestRWLockMixed<64, 4, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_64T_25PW_C,

#if _MGTEST_HAVE_CXX11
	&TestRWLockMixed<16, 4, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_CPP>, // MGWLF_16T_25PW_CPP,
#endif // #if _MGTEST_HAVE_CXX11

	&TestRWLockMixed<8, 2, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8T_50PW_C,
	&TestRWLockMixed<16, 2, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16T_50PW_C,
	&TestRWLockMixed<32, 2, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_32T_50PW_C,
	&TestRWLockMixed<64, 2, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_64T_50PW_C,

	&TestRWLockLocks<1, 1, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_1TR_C,
	&TestRWLockLocks<1, 4, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_4TR_C,
	&TestRWLockLocks<1, 8, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_8TR_C,
	&TestRWLockLocks<1, 16, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_16TR_C,

	&TestRWLockLocks<4, 4, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_4TR_C,
	&TestRWLockLocks<4, 8, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_8TR_C,
	&TestRWLockLocks<4, 16, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_16TR_C,
	&TestRWLockLocks<4, 32, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_32TR_C,

	&TestRWLockLocks<8, 16, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_16TR_C,
	&TestRWLockLocks<8, 32, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_32TR_C,
	&TestRWLockLocks<8, 64, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_64TR_C,
	&TestRWLockLocks<8, 128, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_128TR_C,
};
MG_STATIC_ASSERT(ARRAY_SIZE(g_afnTRDLRWLockFeatureTestProcedures) == MGWLF__MAX);

/*static */
bool CRWLockTest::RunTRDLImplementationTest(unsigned int &nOutSuccessCount, unsigned int &nOutTestCount)
{
	unsigned int nSuccessCount = 0, nTestCount = 0;

	_mg_atomic_init_uint(&g_uiPriorityAdjustmentErrorReported, 0);

	EMGTESTFEATURELEVEL flLevelToTest = CRWLockTest::RetrieveSelectedFeatureTestLevel();

	for (EMGRWLOCKFEATURE lfRWLockFeature = MGWLF__TESTBEGIN; lfRWLockFeature != MGWLF__TESTEND; ++lfRWLockFeature)
	{
		if (g_aflTRDLRWLockFeatureTestLevels[lfRWLockFeature] <= flLevelToTest)
		{
			++nTestCount;

			const char *szFeatureName = g_aszTRDLRWLockFeatureTestNames[lfRWLockFeature];
			printf("Testing %32s: ", szFeatureName);

			CRWLockFeatureTestProcedure fnTestProcedure = g_afnTRDLRWLockFeatureTestProcedures[lfRWLockFeature];
			bool bTestResult = fnTestProcedure();
			printf("%s\n", bTestResult ? "success" : "failure");

			if (bTestResult)
			{
				nSuccessCount += 1;
			}
		}
	}

	int iPriorityIncreaseResult;
	if ((iPriorityIncreaseResult = _mg_atomic_load_relaxed_uint(&g_uiPriorityAdjustmentErrorReported)) != 0)
	{
		printf(g_ascPriorityAdjustmentErrorWarningFormat, iPriorityIncreaseResult);
	}

	nOutSuccessCount = nSuccessCount;
	nOutTestCount = nTestCount;
	MG_ASSERT(nTestCount <= MGWLF__TESTCOUNT);

	return nSuccessCount == nTestCount;
}

