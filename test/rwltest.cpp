/************************************************************************/
/* The MutexGear Library                                                */
/* The Library RWLock Implementation Test File                          */
/*                                                                      */
/* Copyright (c) 2016-2024 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/

/**
*	\file
*	\brief RWLock Test
*
*	The file implements a test for both the C language \c mutexgear_rwlock_t and \c mutexgear_trdl_rwlock_t operations
*	and their C++ wrappers \c mg::shared_mutex and \c mg::trdl::shared_mutex.
*
*	For the tests, numbers of threads are launched each having to perform \c MGTEST_RWLOCK_ITERATION_COUNT (a million by default)
*	locks and unlocks. In between of each lock and unlock each thread performs a call to \c rand() function to simulate a small delay.
*	Also, each thread records its number and operation type in a memory buffer so that it could be later possible to reconstruct
*	the exact order of each lock and unlock operation with respect to all the other threads. At test levels of 'basic' and higher,
*	this information is dumped into textual format files allowing both visual examination/comparison and/or conversion into bitmaps
*	with operations and threads color-coded in pixels. The same tests are run sequentially (and with a cooldown delay of 5 seconds) for
*	the MutexGear implementation and the respective standard RWLock object available in the system thus allowing to compare the total consumed times
*	and individual operation distributions and patterns. Also, the tests are instrumented with runtime checks to ensure lock consistency
*	(like absence of simultaneous locks for write or write and read from multiple threads).
*
*	There are the following test groups:
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
*	and, on some systems (like Ubuntu Linux), this may require running with admin privileges.
*	Also, running multiple high priority CPU consuming threads will likely suspend most of the normal activity in the system
*	and can make it unavailable from network for the duration of the individual tests. So, it's a good idea to make sure
*	the system is free and idle. Also, it's worth rebooting the system before the test run and stopping any extra
*	software to clear the memory and caches and have minimum of interference from other system components.
*
*	\note
*	The operation sequence dump file format is as follows.
*	Each thread index is coded with a zero based "two-digit hex number", the higher digit being coded with a character.
*	The read-locking threads (or operations) start with 'a' and the write-locks start with 's'. With that,
*	the lock operations have the first character capitalized and the unlock operations use small letters.
*	For example, a write lock in thread #0 is coded as 'S0'; a write unlock in thread #25 is coded as 't9' (25 = 0x19; 's' + 1 = 't');
*	a read lock in thread #11 is coded as 'Ab' and the respective unlock is 'ab'. If a lock succeeds with its try-operation alone
*	that is denoted with a dot after the thread number. All the lock operations that were accomplished with their normal
*	[potentially] blocking calls, and all the unlocks, have commas after them.
*/

#include "pch.h"
#include "rwltest.h"
#include "mgtest_common.h"


#ifdef _MSC_VER
#pragma message("!")
#pragma message("warning: warning: warning: The test takes quite some time - be patient")
#pragma message("warning: warning: warning: Also, high CPU usage at elevated priorities can cause driver malfunctions")
#pragma message("warning: warning: warning: Finally, at 'extra' level, lock sequence dump files require 55 GB of disk space to be saved")
#pragma message("warning: warning: warning: Tune MGTEST_RWLOCK_ITERATION_COUNT according to your hardware capabilities")
#pragma message("!")
#else
#warning The test may require admin rights for raising thread priorities
#warning Also, it takes quite some time - be patient
#warning Also, high CPU usage at elevated priorities can cause network disconnections and other driver malfunctions
#warning Finally, at 'extra' level, lock sequence dump files require 55 GB of disk space to be saved
#warning Tune MGTEST_RWLOCK_ITERATION_COUNT according to your hardware capabilities
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

enum ERWLOCKFINETEST
{
	LFT__MIN,

	LFT_SYSTEM = LFT__MIN,
	LFT_MUTEXGEAR,
	LFT_MUTEXGEAR_MINIMAL_TO_WP,
	LFT_MUTEXGEAR_AVERAGE_TO_WP,
	LFT_MUTEXGEAR_SUBSTANTIAL_TO_WP,
	LFT_MUTEXGEAR_INFINITE_TO_WP,

	LFT__MAX,
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
	"Sys", // 	LFT_SYSTEM = LFT__MIN,
	"MG", // LFT_MUTEXGEAR,
	"MG-" MAKE_STRING_LITERAL(MGTEST_RWLOCK_MINIMAL_READERS_TILL_WP) "WP", // LFT_MUTEXGEAR_MINIMAL_TO_WP,
	"MG-" MAKE_STRING_LITERAL(MGTEST_RWLOCK_AVERAGE_READERS_TILL_WP) "WP", // LFT_MUTEXGEAR_AVERAGE_TO_WP,
	"MG-" MAKE_STRING_LITERAL(MGTEST_RWLOCK_SUBSTANTIAL_READERS_TILL_WP) "WP", // LFT_MUTEXGEAR_SUBSTANTIAL_TO_WP,
	"MG-!WP", // LFT_MUTEXGEAR_INFINITE_TO_WP,
};
MG_STATIC_ASSERT(ARRAY_SIZE(g_aszTestedObjectKindFileNameSuffixes) == LFT__MAX);

static const char *const g_aszTestedObjectLanguageNames[] =
{
	"C", // LTL_C,
	"C++", // LTL_CPP,
};
MG_STATIC_ASSERT(ARRAY_SIZE(g_aszTestedObjectLanguageNames) == LTL__MAX);


class CRandomContextProvider
{
public:
	enum 
	{
		RANDOM_BYTE_COUNT = 32768,
		RANDOM_BLOCK_SIZE = 256,
		RANDOM_BLOCK_COUNT = RANDOM_BYTE_COUNT / RANDOM_BLOCK_SIZE,
	};

	typedef uint32_t randcontextint;
	typedef CTimeUtils::timepoint timepoint;

	static randcontextint ConvertTimeToRandcontext(timepoint tpTimePoint) { MG_STATIC_ASSERT(sizeof(randcontextint) == sizeof(uint32_t)); return CTimeUtils::MakeTimepointHash32(tpTimePoint); }
	static randcontextint AdjutsRandcontextForThreadIndex(randcontextint ciSourceRandomContext, unsigned uiThreadIndex) { return ciSourceRandomContext + uiThreadIndex / RANDOM_BLOCK_COUNT + (uiThreadIndex % RANDOM_BLOCK_COUNT) * RANDOM_BLOCK_SIZE;	}
	static randcontextint MixContextWithData(randcontextint ciRandomContext, unsigned uiRandomData) { randcontextint ciUpdatedContext = ciRandomContext ^ uiRandomData; return (ciUpdatedContext << 21) | (ciUpdatedContext >> 11); }

	static void RefreshRandomsCache()
	{
		const uint32_t *const puiDataEnd = reinterpret_cast<const uint32_t *>(m_auiRandomData + ARRAY_SIZE(m_auiRandomData));
		MG_VERIFY(std::find(reinterpret_cast<const uint32_t *>(m_auiRandomData), puiDataEnd, -1) == puiDataEnd); // Search through the data to touch it and move into the CPU cache
	}

	static unsigned ExtractRandomItem(randcontextint ciOperationRandomContext) { return m_auiRandomData[ciOperationRandomContext % RANDOM_BYTE_COUNT]; }

private:
	static const uint8_t m_auiRandomData[RANDOM_BYTE_COUNT];
};

/*static */const uint8_t CRandomContextProvider::m_auiRandomData[CRandomContextProvider::RANDOM_BYTE_COUNT] = 
{
#include "rwltest_randoms.h"
};


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


enum
{
	LIOPT_MULTIPLE_WRITE_CHANNELS		= 0x01,

	LIOPT__CUSTOM_WP_MASK				= 0xF0,
	LIOPT_IMMEDIATE_WP					= 0x00,
	LIOPT_MINIMAL_READERS_TILL_WP		= 0x10,
	LIOPT_AVERAGE_READERS_TILL_WP		= 0x20,
	LIOPT_SUBSTANTIAL_READERS_TILL_WP	= 0x30,
	LIOPT_NO_WP							= 0x40,
};

#define ENCODE_CUSTOM_WP_OPT(Value) (Value)
#define DECODE_CUSTOM_WP_OPT(Flags) ((Flags) & LIOPT__CUSTOM_WP_MASK)


template<ERWLOCKFINETEST tftFineTest>
class CRWLockFineTestTraits;

template<>
class CRWLockFineTestTraits<LFT_SYSTEM>
{
public:
	enum { test_object = LTO_SYSTEM, custom_wp_opt = LIOPT_IMMEDIATE_WP, };
};

template<>
class CRWLockFineTestTraits<LFT_MUTEXGEAR>
{
public:
	enum { test_object = LTO_MUTEXGEAR, custom_wp_opt = LIOPT_IMMEDIATE_WP, };
};

template<>
class CRWLockFineTestTraits<LFT_MUTEXGEAR_MINIMAL_TO_WP>
{
public:
	enum { test_object = LTO_MUTEXGEAR, custom_wp_opt = LIOPT_MINIMAL_READERS_TILL_WP, };
};

template<>
class CRWLockFineTestTraits<LFT_MUTEXGEAR_AVERAGE_TO_WP>
{
public:
	enum { test_object = LTO_MUTEXGEAR, custom_wp_opt = LIOPT_AVERAGE_READERS_TILL_WP, };
};

template<>
class CRWLockFineTestTraits<LFT_MUTEXGEAR_SUBSTANTIAL_TO_WP>
{
public:
	enum { test_object = LTO_MUTEXGEAR, custom_wp_opt = LIOPT_SUBSTANTIAL_READERS_TILL_WP, };
};

template<>
class CRWLockFineTestTraits<LFT_MUTEXGEAR_INFINITE_TO_WP>
{
public:
	enum { test_object = LTO_MUTEXGEAR, custom_wp_opt = LIOPT_NO_WP, };
};


template<unsigned int tuiImplementationOptions>
class CImplementationOptionsTraits;


template<>
class CImplementationOptionsTraits<0>
{
public:
	enum { write_channels = 0, readers_till_wp = 0, };
};

template<>
class CImplementationOptionsTraits<LIOPT_MULTIPLE_WRITE_CHANNELS>
{
public:
	enum { write_channels = MGTEST_RWLOCK_WRITE_CHANNELS, };
};

template<>
class CImplementationOptionsTraits<LIOPT_MINIMAL_READERS_TILL_WP>
{
public:
	enum { readers_till_wp = MGTEST_RWLOCK_MINIMAL_READERS_TILL_WP, };
};

template<>
class CImplementationOptionsTraits<LIOPT_AVERAGE_READERS_TILL_WP>
{
public:
	enum { readers_till_wp = MGTEST_RWLOCK_AVERAGE_READERS_TILL_WP, };
};

template<>
class CImplementationOptionsTraits<LIOPT_SUBSTANTIAL_READERS_TILL_WP>
{
public:
	enum { readers_till_wp = MGTEST_RWLOCK_SUBSTANTIAL_READERS_TILL_WP, };
};

template<>
class CImplementationOptionsTraits<LIOPT_NO_WP>
{
public:
	enum { readers_till_wp = MGTEST_RWLOCK_INFINITE_READERS_TILL_WP, };
};


template<unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
class CTryReadAdapter;

template<unsigned int tuiImplementationOptions>
class CTryReadAdapter<tuiImplementationOptions, TRS_NO_TRYREAD_SUPPORT, LTL_C>
{
public:
	typedef mutexgear_rwlock_t rwlock_type;

	static int TryReadLock(rwlock_type *pwlLockInstance, mutexgear_completion_worker_t *pcwLockWorker, mutexgear_completion_item_t *pciLockCompletionItem)
	{
		return EBUSY;
	}
};

template<unsigned int tuiImplementationOptions>
class CTryReadAdapter<tuiImplementationOptions, TRS_WITH_TRYREAD_SUPPORT, LTL_C>
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

template<unsigned int tuiImplementationOptions>
class CTryReadAdapter<tuiImplementationOptions, TRS_NO_TRYREAD_SUPPORT, LTL_CPP>
{
public:
	typedef mg::wp_shared_mutex<CImplementationOptionsTraits<tuiImplementationOptions & LIOPT_MULTIPLE_WRITE_CHANNELS>::write_channels> rwlock_type;

	static bool TryReadLock(rwlock_type &wlLockInstance, typename rwlock_type::helper_bourgeois_type &hbLockBourgeois)
	{
		return false;
	}
};

template<unsigned int tuiImplementationOptions>
class CTryReadAdapter<tuiImplementationOptions, TRS_WITH_TRYREAD_SUPPORT, LTL_CPP>
{
public:
	typedef mg::trdl::wp_shared_mutex<CImplementationOptionsTraits<tuiImplementationOptions & LIOPT_MULTIPLE_WRITE_CHANNELS>::write_channels> rwlock_type;

	static bool TryReadLock(rwlock_type &wlLockInstance, typename rwlock_type::helper_bourgeois_type &hbLockBourgeois)
	{
		bool bLockResult;
#if MGTEST_RWLOCK_TEST_TRYRDLOCK
		bLockResult = wlLockInstance.try_lock_shared(hbLockBourgeois);
#else /// !MGTEST_RWLOCK_TEST_TRYRDLOCK
		bLockResult = false;
#endif // !MGTEST_RWLOCK_TEST_TRYRDLOCK
		return bLockResult;
	}
};


#endif // #if _MGTEST_HAVE_CXX11


template<ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, unsigned int tuiImplementationOptions, ERWLOCKLOCKTESTOBJECT ttoTestedObjectKind, ERWLOCKLOCKTESTLANGUAGE ttlTestObjectLanguage>
class CRWLockImplementation;

template<ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, unsigned int tuiImplementationOptions>
class CRWLockImplementation<trsTryReadSupport, tuiImplementationOptions, LTO_SYSTEM, LTL_C>
{
public:
	CRWLockImplementation() { InitializeRWLockInstance(); }
	~CRWLockImplementation() { FinalizeRWLockInstance(); }

public:
	class CLockWriteExtraObjects
	{
	public:
	};

	class CLockReadExtraObjects:
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

template<ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, unsigned int tuiImplementationOptions>
class CRWLockImplementation<trsTryReadSupport, tuiImplementationOptions, LTO_SYSTEM, LTL_CPP>
{
public:
	CRWLockImplementation() { InitializeRWLockInstance(); }
	~CRWLockImplementation() { FinalizeRWLockInstance(); }

public:
	class CLockWriteExtraObjects
	{
	public:
	};

	class CLockReadExtraObjects:
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

template<ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, unsigned int tuiImplementationOptions>
class CRWLockImplementation<trsTryReadSupport, tuiImplementationOptions, LTO_SYSTEM, LTL_CPP>
{
public:
	CRWLockImplementation() { }
	~CRWLockImplementation() { }

public:
	class CLockWriteExtraObjects
	{
	public:
	};

	class CLockReadExtraObjects:
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


template<ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, unsigned int tuiImplementationOptions>
class CRWLockImplementation<trsTryReadSupport, tuiImplementationOptions, LTO_MUTEXGEAR, LTL_C>
{
	typedef CTryReadAdapter<tuiImplementationOptions, trsTryReadSupport, LTL_C> tryread_adapter_type;
	typedef typename tryread_adapter_type::rwlock_type rwlock_type;

public:
	CRWLockImplementation() { InitializeRWLockInstance(); }
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

			mutexgear_completion_item_init(&m_ciLockCompletionItem);
		}

		~CLockWriteExtraObjects()
		{
			mutexgear_completion_item_destroy(&m_ciLockCompletionItem);

			int iWaiterDestroyResult;
			MG_CHECK(iWaiterDestroyResult, (iWaiterDestroyResult = mutexgear_completion_waiter_destroy(&m_cwLockWaiter)) == EOK);

			int iWorkerUnlockResult;
			MG_CHECK(iWorkerUnlockResult, (iWorkerUnlockResult = mutexgear_completion_worker_unlock(&m_cwLockWorker)) == EOK);

			int iWorkerDestroyResult;
			MG_CHECK(iWorkerDestroyResult, (iWorkerDestroyResult = mutexgear_completion_worker_destroy(&m_cwLockWorker)) == EOK);
		}

		mutexgear_completion_worker_t		m_cwLockWorker;
		mutexgear_completion_waiter_t		m_cwLockWaiter;
		mutexgear_completion_item_t			m_ciLockCompletionItem;
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
			const int iReadersTillWP = CImplementationOptionsTraits<DECODE_CUSTOM_WP_OPT(tuiImplementationOptions)>::readers_till_wp;

			if (iReadersTillWP == 0)
			{
				MG_CHECK(iLockResult, iLockResult == EBUSY && (iLockResult = mutexgear_rwlock_wrlock(&m_wlRWLock, &eoRefExtraObjects.m_cwLockWorker, &eoRefExtraObjects.m_cwLockWaiter, &eoRefExtraObjects.m_ciLockCompletionItem)) == EOK);
			}
			else
			{
				MG_CHECK(iLockResult, iLockResult == EBUSY && (iLockResult = mutexgear_rwlock_wrlock_cwp(&m_wlRWLock, &eoRefExtraObjects.m_cwLockWorker, &eoRefExtraObjects.m_cwLockWaiter, &eoRefExtraObjects.m_ciLockCompletionItem, iReadersTillWP)) == EOK);
			}
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

		int iLockResult = tryread_adapter_type::TryReadLock(&m_wlRWLock, &eoRefExtraObjects.m_eoWriteObjects.m_cwLockWorker, &eoRefExtraObjects.m_eoWriteObjects.m_ciLockCompletionItem);
		bLockedWithTryVariant = iLockResult == EOK;

		if (!bLockedWithTryVariant)
		{
			MG_CHECK(iLockResult, iLockResult == EBUSY && (iLockResult = mutexgear_rwlock_rdlock(&m_wlRWLock, &eoRefExtraObjects.m_eoWriteObjects.m_cwLockWorker, &eoRefExtraObjects.m_eoWriteObjects.m_cwLockWaiter, &eoRefExtraObjects.m_eoWriteObjects.m_ciLockCompletionItem)) == EOK);
		}

		return bLockedWithTryVariant;
	}

	void UnlockRWLockRead(CLockReadExtraObjects &eoRefExtraObjects)
	{
		int iUnlockResult;
		MG_CHECK(iUnlockResult, (iUnlockResult = mutexgear_rwlock_rdunlock(&m_wlRWLock, &eoRefExtraObjects.m_eoWriteObjects.m_cwLockWorker, &eoRefExtraObjects.m_eoWriteObjects.m_ciLockCompletionItem)) == EOK);
	}

private:
	void InitializeRWLockInstance()
	{
		int iInitResult;
		mutexgear_rwlockattr_t attr;

		MG_CHECK(iInitResult, (iInitResult = mutexgear_rwlockattr_init(&attr)) == EOK);

		if ((tuiImplementationOptions & LIOPT_MULTIPLE_WRITE_CHANNELS) != 0)
		{
			MG_CHECK(iInitResult, (iInitResult = mutexgear_rwlockattr_setwritechannels(&attr, CImplementationOptionsTraits<LIOPT_MULTIPLE_WRITE_CHANNELS>::write_channels)) == EOK);
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

template<ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, unsigned int tuiImplementationOptions>
class CRWLockImplementation<trsTryReadSupport, tuiImplementationOptions, LTO_MUTEXGEAR, LTL_CPP>
{
	typedef CTryReadAdapter<tuiImplementationOptions, trsTryReadSupport, LTL_CPP> tryread_adapter_type;
	typedef typename tryread_adapter_type::rwlock_type rwlock_type;

public:
	CRWLockImplementation() { InitializeRWLockInstance(); }
	~CRWLockImplementation() { FinalizeRWLockInstance(); }

public:
	class CLockWriteExtraObjects
	{
	public:
		CLockWriteExtraObjects()
		{
			m_hbLockBourgeois.lock();
		}

		~CLockWriteExtraObjects()
		{
			m_hbLockBourgeois.unlock();
		}

		typename rwlock_type::helper_bourgeois_type m_hbLockBourgeois;
		typename rwlock_type::helper_waiter_type m_hwLockWaiter;
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
		bool bLockedWithTryVariant;
#if MGTEST_RWLOCK_TEST_TRYWRLOCK
		bLockedWithTryVariant = m_wlRWLock.try_lock();
#else // !MGTEST_RWLOCK_TEST_TRYWRLOCK
		bLockedWithTryVariant = false;
#endif // !MGTEST_RWLOCK_TEST_TRYWRLOCK

		if (!bLockedWithTryVariant)
		{
			const int iReadersTillWP = CImplementationOptionsTraits<DECODE_CUSTOM_WP_OPT(tuiImplementationOptions)>::readers_till_wp;

			if (iReadersTillWP == 0)
			{
				m_wlRWLock.lock(eoRefExtraObjects.m_hbLockBourgeois, eoRefExtraObjects.m_hwLockWaiter);
			}
			else if (iReadersTillWP == MGTEST_RWLOCK_INFINITE_READERS_TILL_WP)
			{
				m_wlRWLock.lock(eoRefExtraObjects.m_hbLockBourgeois, eoRefExtraObjects.m_hwLockWaiter, mg::no_wp_t());
			}
			else
			{
				m_wlRWLock.lock(eoRefExtraObjects.m_hbLockBourgeois, eoRefExtraObjects.m_hwLockWaiter, iReadersTillWP);
			}
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

		bLockedWithTryVariant = tryread_adapter_type::TryReadLock(m_wlRWLock, eoRefExtraObjects.m_eoWriteObjects.m_hbLockBourgeois);

		if (!bLockedWithTryVariant)
		{
			m_wlRWLock.lock_shared(eoRefExtraObjects.m_eoWriteObjects.m_hbLockBourgeois, eoRefExtraObjects.m_eoWriteObjects.m_hwLockWaiter);
		}

		return bLockedWithTryVariant;
	}

	void UnlockRWLockRead(CLockReadExtraObjects &eoRefExtraObjects)
	{
		m_wlRWLock.unlock_shared(eoRefExtraObjects.m_eoWriteObjects.m_hbLockBourgeois);
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

template<ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, unsigned int tuiImplementationOptions>
class CRWLockImplementation<trsTryReadSupport, tuiImplementationOptions, LTO_MUTEXGEAR, LTL_CPP>
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


class CLockDurationUtils
{
public:
	typedef CTimeUtils::timeduration timeduration;

	static bool IsDurationSmaller(timeduration tdTestDuration, timeduration tdThanDuration) { return tdTestDuration * 3 / 2 <= tdThanDuration; }
};


class IRWLockLockTestThread
{
public:
	typedef CTimeUtils::timeduration timeduration;

	virtual void ReleaseInstance(timeduration &tdOutMinimumLockDuration, timeduration &tdOutMinDurationUnavailabilityTime, timeduration &tdOutHigherDurationUnavailabilityTime,
		timeduration &tdOutMaxUnavailabilityTime) = 0;
};


class CRWLockLockTestBase
{
public:
	typedef unsigned int iterationcntint;
	typedef unsigned int operationidxint; typedef signed int operationsidxint; typedef _mg_atomic_uint_t atomic_operationidxint;
	typedef unsigned int threadcntint;
	typedef CRandomContextProvider::randcontextint randcontextint;

	static inline void atomic_init_operationidxint(volatile atomic_operationidxint *piiDestination, operationidxint iiValue)
	{
		_mg_atomic_init_uint(piiDestination, iiValue);
	}

	static inline operationidxint atomic_fetch_add_relaxed_operationidxint(volatile atomic_operationidxint *piiDestination, operationidxint iiValue)
	{
		return _mg_atomic_fetch_add_relaxed_uint(piiDestination, iiValue);
	}
};

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
class CRWLockLockTestExecutor:
	public CRWLockLockTestBase
{
public:
	explicit CRWLockLockTestExecutor(iterationcntint uiIterationCount):
		m_uiIterationCount(uiIterationCount),
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
	typedef CTimeUtils::timeduration timeduration;
	typedef CTimeUtils::timepoint timepoint;

	void AllocateThreadOperationBuffers(threadcntint ciTotalThreadCount);
	void FreeThreadOperationBuffers(threadcntint ciTotalThreadCount);
	operationidxint GetMaximalThreadOperationCount() const;

	template<unsigned tuiCustomizedImplementationOptions, ERWLOCKLOCKTESTOBJECT ttoTestedObjectKind>
	void AllocateTestThreads(
		CRWLockImplementation<trsTryReadSupport, tuiCustomizedImplementationOptions, ttoTestedObjectKind, ttlTestLanguage> &liRefRWLock,
		threadcntint ciWriterCount, threadcntint ciReraderCount, timepoint tpRunStartTime, 
		CThreadExecutionBarrier &sbRefStartBarrier, CThreadExecutionBarrier &sbRefFinishBarrier, CThreadExecutionBarrier &sbRefExitBarrier,
		CRWLockLockTestProgress &tpRefProgressInstance);
	void FreeTestThreads(CThreadExecutionBarrier &sbRefExitBarrier, threadcntint ciTotalThreadCount, 
		timeduration &tdOutTotalUnavailabilityTime, timeduration &tdOutMaxUnavailabilityTime);

	void WaitTestThreadsReady(CThreadExecutionBarrier &sbStartBarrier);
	void LaunchTheTest(CThreadExecutionBarrier &sbStartBarrier);
	void WaitTheTestEnd(CThreadExecutionBarrier &sbFinishBarrier);

	void InitializeTestResults(threadcntint ciWriterCount, threadcntint ciReraderCount, EMGTESTFEATURELEVEL flTestLevel);
	void PublishTestResults(ERWLOCKFINETEST ftTestKind, threadcntint ciWriterCount, threadcntint ciReraderCount, timepoint tpRunStartTime, 
		EMGTESTFEATURELEVEL flTestLevel, timeduration tdTestDuration, timeduration tdTotalUnavailabilityTime, timeduration tdMaxUnavailabilityTime);
	void FillInitialThreadRandomContexts(randcontextint aciThreadRandomContexts[], threadcntint ciTotalThreadCount, randcontextint ciRunRandomContext);
	void BuildMergedThreadOperationMap(char ascMergeBuffer[], operationidxint iiLastSavedOperationIndex, operationidxint iiThreadOperationCount,
		operationidxint aiiThreadLastOperationIndices[], threadcntint ciWriterCount, threadcntint ciReaderCount, randcontextint aciThreadRandomContexts[], unsigned puiThreadRandomData[]);
	operationidxint FillThreadOperationMap(char ascMergeBuffer[], operationidxint iiLastSavedOperationIndex, const operationidxint iiThreadOperationCount,
		const operationidxint *piiThreadOperationIndices, operationidxint iiThreadLastOperationIndex, randcontextint &ciVarThreadRandomContext, unsigned &uiVarThreadRandomData, 
		const char ascThreadMapChars[LOCK_THREAD_NAME_LENGTH]);
	void InitializeThreadName(char ascThreadMapChars[LOCK_THREAD_NAME_LENGTH + 1], char cMapFirstChar);
	void IncrementThreadName(char ascThreadMapChars[LOCK_THREAD_NAME_LENGTH + 1], threadcntint ciThreadIndex);
	void SaveThreadOperationMapToDetails(ERWLOCKFINETEST ftTestKind, const char ascMergeBuffer[], operationidxint iiThreadOperationCount);
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
	IRWLockLockTestThread *m_aittTestThreads[LOCKTEST_THREAD_COUNT];
	operationidxint *m_apiiThreadOperationIndices[LOCKTEST_THREAD_COUNT];
	char *m_pscDetailsMergeBuffer;
	operationidxint		m_iiThreadOperationCount;
	FILE *m_apsfOperationDetailFiles[LFT__MAX];
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
class CRWLockLockTestThread:
	public IRWLockLockTestThread
{
public:
	typedef CRWLockLockTestBase::iterationcntint iterationcntint;
	typedef CRWLockLockTestBase::operationidxint operationidxint;
	typedef CRandomContextProvider::randcontextint randcontextint;
	typedef CTimeUtils::timepoint timepoint;
	using IRWLockLockTestThread::timeduration;

	struct CThreadParameterInfo
	{
	public:
		explicit CThreadParameterInfo(CThreadExecutionBarrier &sbRefStartBarrier, CThreadExecutionBarrier &sbRefFinishBarrier, CThreadExecutionBarrier &sbRefExitBarrier,
			CRWLockLockTestProgress &tpRefProgressInstance):
			m_psbStartBarrier(&sbRefStartBarrier),
			m_psbFinishBarrier(&sbRefFinishBarrier),
			m_psbExitBarrier(&sbRefExitBarrier),
			m_ptpProgressInstance(&tpRefProgressInstance)
		{
		}

	public:
		CThreadExecutionBarrier *m_psbStartBarrier;
		CThreadExecutionBarrier *m_psbFinishBarrier;
		CThreadExecutionBarrier *m_psbExitBarrier;
		CRWLockLockTestProgress *m_ptpProgressInstance;
	};

public:
	CRWLockLockTestThread(const CThreadParameterInfo &piThreadParameters, randcontextint ciThreadRandomContext, iterationcntint uiIterationCount, const typename TOperationExecutor::CConstructionParameter &cpExecutorParameter, operationidxint aiiOperationIndexBuffer[]):
		m_piThreadParameters(piThreadParameters),
		m_ciThreadRandomContext(ciThreadRandomContext),
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
	virtual void ReleaseInstance(timeduration &tdOutMinimumLockDuration, timeduration &tdOutMinDurationUnavailabilityTime, timeduration &tdOutHigherDurationUnavailabilityTime, 
		timeduration &tdOutMaxUnavailabilityTime);

private:
	void StoreUnavailabilityTimes(timeduration tdMinimumLockDuration, timeduration tdMinDurationUnavailabilityTime, timeduration tdHigherDurationUnavailabilityTime,
		timeduration tdMaxUnavailabilityTime)
	{
		m_tdMinimumLockDuration = tdMinimumLockDuration;
		m_tdMinDurationUnavailabilityTime = tdMinDurationUnavailabilityTime;
		m_tdHigherDurationUnavailabilityTime = tdHigherDurationUnavailabilityTime;
		m_tdMaxUnavailabilityTime = tdMaxUnavailabilityTime;
	}

	void RetrieveUnavailabilityTimes(timeduration &tdOutMinimumLockDuration, timeduration &tdOutMinDurationUnavailabilityTime, timeduration &tdOutHigherDurationUnavailabilityTime,
		timeduration &tdOutMaxUnavailabilityTime) const
	{
		tdOutMinimumLockDuration = m_tdMinimumLockDuration;
		tdOutMinDurationUnavailabilityTime = m_tdMinDurationUnavailabilityTime;
		tdOutHigherDurationUnavailabilityTime = m_tdHigherDurationUnavailabilityTime;
		tdOutMaxUnavailabilityTime = m_tdMaxUnavailabilityTime;
	}

private:
	CThreadParameterInfo m_piThreadParameters;
	randcontextint		m_ciThreadRandomContext;
	iterationcntint		m_uiIterationCount;
	operationidxint		*m_piiOperationIndexBuffer;
	timeduration		m_tdMinimumLockDuration;
	timeduration		m_tdMinDurationUnavailabilityTime;
	timeduration		m_tdHigherDurationUnavailabilityTime;
	timeduration		m_tdMaxUnavailabilityTime;
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
	LOK__RANDCONTEXT_UPDATE_MIN = LOK__ENCODED_MIN,

	LOK_ACUIORE_LOCK = LOK__RANDCONTEXT_UPDATE_MIN,

	LOK__RANDCONTEXT_UPDATE_MAX,
	LOK__ENCODED_MAX = LOK__RANDCONTEXT_UPDATE_MAX,

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

template<ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, unsigned int tuiImplementationOptions, ERWLOCKLOCKTESTOBJECT ttoTestedObjectKind, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
class CRWLockWriteLockExecutor
{
	typedef CRWLockImplementation<trsTryReadSupport, tuiImplementationOptions, ttoTestedObjectKind, ttlTestLanguage> implementation_type;

public:
	struct CConstructionParameter
	{
		CConstructionParameter(implementation_type &liRWLockInstance): m_liRWLockInstance(liRWLockInstance) {}

		implementation_type &m_liRWLockInstance;
	};

	CRWLockWriteLockExecutor(const CConstructionParameter &cpConstructionParameter): m_liRWLockInstance(cpConstructionParameter.m_liRWLockInstance) {}

	typedef typename implementation_type::CLockWriteExtraObjects COperationExtraObjects;
	typedef CRWLockLockTestBase::iterationcntint iterationcntint;
	typedef CRWLockLockTestBase::operationidxint operationidxint;

	static operationidxint GetThreadOperationCount(iterationcntint uiIterationCount) { return (operationidxint)uiIterationCount * LOK__MAX; }

	bool ApplyLock(COperationExtraObjects &eoRefExtraObjects, unsigned uiRandomData)
	{
		bool bLockedWithTryVariant = m_liRWLockInstance.LockRWLockWrite(eoRefExtraObjects);
		CRWLockValidator::IncrementWrites();

		return bLockedWithTryVariant;
	}

	void ReleaseLock(COperationExtraObjects &eoRefExtraObjects, unsigned uiRandomData)
	{
		CRWLockValidator::DecrementWrites();
		m_liRWLockInstance.UnlockRWLockWrite(eoRefExtraObjects);
	}

private:
	implementation_type &m_liRWLockInstance;
};

template<ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, unsigned int tuiImplementationOptions, ERWLOCKLOCKTESTOBJECT ttoTestedObjectKind, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage, unsigned int tuiReaderWriteDivisor>
class CRWLockReadLockExecutor
{
public:
	typedef CRWLockImplementation<trsTryReadSupport, tuiImplementationOptions, ttoTestedObjectKind, ttlTestLanguage> implementation_type;

	struct CConstructionParameter
	{
		CConstructionParameter(implementation_type &liRWLockInstance): m_liRWLockInstance(liRWLockInstance) {}

		implementation_type &m_liRWLockInstance;
	};

	CRWLockReadLockExecutor(const CConstructionParameter &cpConstructionParameter): m_liRWLockInstance(cpConstructionParameter.m_liRWLockInstance) {}

	typedef typename implementation_type::CLockReadExtraObjects COperationExtraObjects;
	typedef CRWLockLockTestBase::iterationcntint iterationcntint;
	typedef CRWLockLockTestBase::operationidxint operationidxint;

	static operationidxint GetThreadOperationCount(iterationcntint uiIterationCount) { return (operationidxint)uiIterationCount * LOK__MAX; }

	bool ApplyLock(COperationExtraObjects &eoRefExtraObjects, unsigned uiRandomData)
	{
		bool bLockedWithTryVariant;

		MG_STATIC_ASSERT((tuiReaderWriteDivisor & (tuiReaderWriteDivisor - 1)) == 0); // For the test below...
		// ...
		if (tuiReaderWriteDivisor == 0 || (uiRandomData & (tuiReaderWriteDivisor - 1)) != 0)
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

	void ReleaseLock(COperationExtraObjects &eoRefExtraObjects, unsigned uiRandomData)
	{
		MG_STATIC_ASSERT((tuiReaderWriteDivisor & (tuiReaderWriteDivisor - 1)) == 0); // For the test below...
		// ...
		if (tuiReaderWriteDivisor == 0 || (uiRandomData & (tuiReaderWriteDivisor - 1)) != 0)
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
};


//////////////////////////////////////////////////////////////////////////

static const char g_ascLockTestLegend[] = "Total = Consumed clock time for the test\nSBID = sum of lock times bigger than 1 quantum\nMBID = the maximal lock time of those bigger than 1 quantum";

static const int g_iTestFeatureNameWidth = 33;
static const char g_ascFeatureTestingText[] = "Testing ";
static const char g_ascPostFeatureSuffix[] = ": ";

static const char g_ascLockTestTableCaption[] = "          Sys         /          MG         /        MG-" MAKE_STRING_LITERAL(MGTEST_RWLOCK_MINIMAL_READERS_TILL_WP) "WP       /        MG-" MAKE_STRING_LITERAL(MGTEST_RWLOCK_AVERAGE_READERS_TILL_WP) "WP       /        MG-" MAKE_STRING_LITERAL(MGTEST_RWLOCK_SUBSTANTIAL_READERS_TILL_WP) "WP       /        MG-!WP       ";
static const char g_ascLockTestTableSubCapn[] = "  Total,  SBID , MBID / Total,  SBID , MBID / Total,  SBID , MBID "                                                           "/ Total,  SBID , MBID "                                                           "/ Total,  SBID , MBID "                                                               "/ Total,  SBID , MBID ";

static const char g_ascLockTestTimesFormat[] = "(%2lu.%.03lu,%3lu.%.03lu,%2lu.%.03lu/%2lu.%.03lu,%3lu.%.03lu,%2lu.%.03lu/%2lu.%.03lu,%3lu.%.03lu,%2lu.%.03lu/%2lu.%.03lu,%3lu.%.03lu,%2lu.%.03lu/%2lu.%.03lu,%3lu.%.03lu,%2lu.%.03lu/%2lu.%.03lu,%3lu.%.03lu,%2lu.%.03lu): ";


template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::~CRWLockLockTestExecutor()
{
	MG_ASSERT(std::find_if(m_aittTestThreads, m_aittTestThreads + ARRAY_SIZE(m_aittTestThreads), pred_not_zero<IRWLockLockTestThread *>) == m_aittTestThreads + ARRAY_SIZE(m_aittTestThreads));
	MG_ASSERT(std::find_if(m_apiiThreadOperationIndices, m_apiiThreadOperationIndices + ARRAY_SIZE(m_apiiThreadOperationIndices), pred_not_zero<operationidxint *>) == m_apiiThreadOperationIndices + ARRAY_SIZE(m_apiiThreadOperationIndices));
	MG_ASSERT(m_pscDetailsMergeBuffer == NULL);
	MG_ASSERT(std::find_if(m_apsfOperationDetailFiles, m_apsfOperationDetailFiles + ARRAY_SIZE(m_apsfOperationDetailFiles), pred_not_zero<FILE *>) == m_apsfOperationDetailFiles + ARRAY_SIZE(m_apsfOperationDetailFiles));
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
bool CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::RunTheTask()
{
	const bool bSingleOperationTest = (tuiWriterCount == 0 && tuiReaderWriteDivisor == 0) || tuiReaderCount == 0;

	EMGTESTFEATURELEVEL flLevelToTest = CRWLockTest::RetrieveSelectedFeatureTestLevel();

	InitializeTestResults(LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, flLevelToTest);
	AllocateThreadOperationBuffers(LOCKTEST_THREAD_COUNT);

	CThreadExecutionBarrier sbStartBarrier(LOCKTEST_THREAD_COUNT), sbFinishBarrier(LOCKTEST_THREAD_COUNT), sbExitBarrier(0);
	CRWLockLockTestProgress tpProgressInstance;
	timeduration atdObjectTestDurations[LFT__MAX] = { 0, }, atdObjectTestTotalUnavailabilityTimes[LFT__MAX] = { 0, }, atdObjectTestMaxUnavailabilityTimes[LFT__MAX] = { 0, };

	timepoint tpRunStartTime = CTimeUtils::GetCurrentMonotonicTimeNano();

	{
		const ERWLOCKFINETEST ftTestKind = LFT_SYSTEM;
		const ERWLOCKLOCKTESTOBJECT toTestedObjectKind = (ERWLOCKLOCKTESTOBJECT)CRWLockFineTestTraits<ftTestKind>::test_object;
		const unsigned uiCustomWPOption = CRWLockFineTestTraits<ftTestKind>::custom_wp_opt;

#if !_MGTEST_ANY_SHARED_MUTEX_AVAILABLE
		if (ttlTestLanguage == LTL_CPP)
		{
			// Do nothing
		}
		else
#endif
		{
			const unsigned uiCustomizedImplementationOptions = tuiImplementationOptions | ENCODE_CUSTOM_WP_OPT(uiCustomWPOption);
			CRWLockImplementation<trsTryReadSupport, uiCustomizedImplementationOptions, toTestedObjectKind, ttlTestLanguage> liRWLock;
			AllocateTestThreads<uiCustomizedImplementationOptions, toTestedObjectKind>(liRWLock, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, tpRunStartTime, sbStartBarrier, sbFinishBarrier, sbExitBarrier, tpProgressInstance);

			WaitTestThreadsReady(sbStartBarrier);
			CRandomContextProvider::RefreshRandomsCache();

			timepoint tpTestStartTime = CTimeUtils::GetCurrentMonotonicTimeNano();

			LaunchTheTest(sbStartBarrier);
			WaitTheTestEnd(sbFinishBarrier);

			timepoint tpTestEndTime = CTimeUtils::GetCurrentMonotonicTimeNano();

			timeduration tdTestDuration = atdObjectTestDurations[ftTestKind] = tpTestEndTime - tpTestStartTime;

			timeduration tdTotalUnavailabilityTime, tdMaxUnavailabilityTime;
			FreeTestThreads(sbExitBarrier, LOCKTEST_THREAD_COUNT, tdTotalUnavailabilityTime, tdMaxUnavailabilityTime);
			atdObjectTestTotalUnavailabilityTimes[ftTestKind] = tdTotalUnavailabilityTime;
			atdObjectTestMaxUnavailabilityTimes[ftTestKind] = tdMaxUnavailabilityTime;

			PublishTestResults(ftTestKind, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, tpRunStartTime, flLevelToTest, tdTestDuration, tdTotalUnavailabilityTime, tdMaxUnavailabilityTime);

			sbStartBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
			sbFinishBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
			sbExitBarrier.ResetInstance(0);
			tpProgressInstance.ResetInstance();
		}
	}

	{
		const ERWLOCKFINETEST ftTestKind = LFT_MUTEXGEAR;
		const ERWLOCKLOCKTESTOBJECT toTestedObjectKind = (ERWLOCKLOCKTESTOBJECT)CRWLockFineTestTraits<ftTestKind>::test_object;
		const unsigned uiCustomWPOption = CRWLockFineTestTraits<ftTestKind>::custom_wp_opt;

		if (
#if !_MGTEST_HAVE_CXX11
			ttlTestLanguage == LTL_CPP ||
#endif
			(uiCustomWPOption != 0 && bSingleOperationTest)
			)
		{
			// Do nothing
		}
		else
		{
			const unsigned uiCustomizedImplementationOptions = tuiImplementationOptions | ENCODE_CUSTOM_WP_OPT(uiCustomWPOption);
			CRWLockImplementation<trsTryReadSupport, uiCustomizedImplementationOptions, toTestedObjectKind, ttlTestLanguage> liRWLock;
			AllocateTestThreads<uiCustomizedImplementationOptions, toTestedObjectKind>(liRWLock, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, tpRunStartTime, sbStartBarrier, sbFinishBarrier, sbExitBarrier, tpProgressInstance);

			WaitTestThreadsReady(sbStartBarrier);
			CRandomContextProvider::RefreshRandomsCache();

			timepoint tpTestStartTime = CTimeUtils::GetCurrentMonotonicTimeNano();

			LaunchTheTest(sbStartBarrier);
			WaitTheTestEnd(sbFinishBarrier);

			timepoint tpTestEndTime = CTimeUtils::GetCurrentMonotonicTimeNano();

			timeduration tdTestDuration = atdObjectTestDurations[ftTestKind] = tpTestEndTime - tpTestStartTime;

			timeduration tdTotalUnavailabilityTime, tdMaxUnavailabilityTime;
			FreeTestThreads(sbExitBarrier, LOCKTEST_THREAD_COUNT, tdTotalUnavailabilityTime, tdMaxUnavailabilityTime);
			atdObjectTestTotalUnavailabilityTimes[ftTestKind] = tdTotalUnavailabilityTime;
			atdObjectTestMaxUnavailabilityTimes[ftTestKind] = tdMaxUnavailabilityTime;

			PublishTestResults(ftTestKind, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, tpRunStartTime, flLevelToTest, tdTestDuration, tdTotalUnavailabilityTime, tdMaxUnavailabilityTime);

			sbStartBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
			sbFinishBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
			sbExitBarrier.ResetInstance(0);
			tpProgressInstance.ResetInstance();
		}
	}

	{
		const ERWLOCKFINETEST ftTestKind = LFT_MUTEXGEAR_MINIMAL_TO_WP;
		const ERWLOCKLOCKTESTOBJECT toTestedObjectKind = (ERWLOCKLOCKTESTOBJECT)CRWLockFineTestTraits<ftTestKind>::test_object;
		const unsigned uiCustomWPOption = CRWLockFineTestTraits<ftTestKind>::custom_wp_opt;

		if (
#if !_MGTEST_HAVE_CXX11
			ttlTestLanguage == LTL_CPP ||
#endif
			(uiCustomWPOption != 0 && bSingleOperationTest)
			)
		{
			// Do nothing
		}
		else
		{
			const unsigned uiCustomizedImplementationOptions = tuiImplementationOptions | ENCODE_CUSTOM_WP_OPT(uiCustomWPOption);
			CRWLockImplementation<trsTryReadSupport, uiCustomizedImplementationOptions, toTestedObjectKind, ttlTestLanguage> liRWLock;
			AllocateTestThreads<uiCustomizedImplementationOptions, toTestedObjectKind>(liRWLock, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, tpRunStartTime, sbStartBarrier, sbFinishBarrier, sbExitBarrier, tpProgressInstance);

			WaitTestThreadsReady(sbStartBarrier);
			CRandomContextProvider::RefreshRandomsCache();

			timepoint tpTestStartTime = CTimeUtils::GetCurrentMonotonicTimeNano();

			LaunchTheTest(sbStartBarrier);
			WaitTheTestEnd(sbFinishBarrier);

			timepoint tpTestEndTime = CTimeUtils::GetCurrentMonotonicTimeNano();

			timeduration tdTestDuration = atdObjectTestDurations[ftTestKind] = tpTestEndTime - tpTestStartTime;

			timeduration tdTotalUnavailabilityTime, tdMaxUnavailabilityTime;
			FreeTestThreads(sbExitBarrier, LOCKTEST_THREAD_COUNT, tdTotalUnavailabilityTime, tdMaxUnavailabilityTime);
			atdObjectTestTotalUnavailabilityTimes[ftTestKind] = tdTotalUnavailabilityTime;
			atdObjectTestMaxUnavailabilityTimes[ftTestKind] = tdMaxUnavailabilityTime;

			PublishTestResults(ftTestKind, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, tpRunStartTime, flLevelToTest, tdTestDuration, tdTotalUnavailabilityTime, tdMaxUnavailabilityTime);

			sbStartBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
			sbFinishBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
			sbExitBarrier.ResetInstance(0);
			tpProgressInstance.ResetInstance();
		}
	}

	{
		const ERWLOCKFINETEST ftTestKind = LFT_MUTEXGEAR_AVERAGE_TO_WP;
		const ERWLOCKLOCKTESTOBJECT toTestedObjectKind = (ERWLOCKLOCKTESTOBJECT)CRWLockFineTestTraits<ftTestKind>::test_object;
		const unsigned uiCustomWPOption = CRWLockFineTestTraits<ftTestKind>::custom_wp_opt;

		if (
#if !_MGTEST_HAVE_CXX11
			ttlTestLanguage == LTL_CPP ||
#endif
			(uiCustomWPOption != 0 && bSingleOperationTest)
			)
		{
			// Do nothing
		}
		else
		{
			const unsigned uiCustomizedImplementationOptions = tuiImplementationOptions | ENCODE_CUSTOM_WP_OPT(uiCustomWPOption);
			CRWLockImplementation<trsTryReadSupport, uiCustomizedImplementationOptions, toTestedObjectKind, ttlTestLanguage> liRWLock;
			AllocateTestThreads<uiCustomizedImplementationOptions, toTestedObjectKind>(liRWLock, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, tpRunStartTime, sbStartBarrier, sbFinishBarrier, sbExitBarrier, tpProgressInstance);

			WaitTestThreadsReady(sbStartBarrier);
			CRandomContextProvider::RefreshRandomsCache();

			timepoint tpTestStartTime = CTimeUtils::GetCurrentMonotonicTimeNano();

			LaunchTheTest(sbStartBarrier);
			WaitTheTestEnd(sbFinishBarrier);

			timepoint tpTestEndTime = CTimeUtils::GetCurrentMonotonicTimeNano();

			timeduration tdTestDuration = atdObjectTestDurations[ftTestKind] = tpTestEndTime - tpTestStartTime;

			timeduration tdTotalUnavailabilityTime, tdMaxUnavailabilityTime;
			FreeTestThreads(sbExitBarrier, LOCKTEST_THREAD_COUNT, tdTotalUnavailabilityTime, tdMaxUnavailabilityTime);
			atdObjectTestTotalUnavailabilityTimes[ftTestKind] = tdTotalUnavailabilityTime;
			atdObjectTestMaxUnavailabilityTimes[ftTestKind] = tdMaxUnavailabilityTime;

			PublishTestResults(ftTestKind, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, tpRunStartTime, flLevelToTest, tdTestDuration, tdTotalUnavailabilityTime, tdMaxUnavailabilityTime);

			sbStartBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
			sbFinishBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
			sbExitBarrier.ResetInstance(0);
			tpProgressInstance.ResetInstance();
		}
	}

	{
		const ERWLOCKFINETEST ftTestKind = LFT_MUTEXGEAR_SUBSTANTIAL_TO_WP;
		const ERWLOCKLOCKTESTOBJECT toTestedObjectKind = (ERWLOCKLOCKTESTOBJECT)CRWLockFineTestTraits<ftTestKind>::test_object;
		const unsigned uiCustomWPOption = CRWLockFineTestTraits<ftTestKind>::custom_wp_opt;

		if (
#if !_MGTEST_HAVE_CXX11
			ttlTestLanguage == LTL_CPP ||
#endif
			(uiCustomWPOption != 0 && bSingleOperationTest)
			)
		{
			// Do nothing
		}
		else
		{
			const unsigned uiCustomizedImplementationOptions = tuiImplementationOptions | ENCODE_CUSTOM_WP_OPT(uiCustomWPOption);
			CRWLockImplementation<trsTryReadSupport, uiCustomizedImplementationOptions, toTestedObjectKind, ttlTestLanguage> liRWLock;
			AllocateTestThreads<uiCustomizedImplementationOptions, toTestedObjectKind>(liRWLock, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, tpRunStartTime, sbStartBarrier, sbFinishBarrier, sbExitBarrier, tpProgressInstance);

			WaitTestThreadsReady(sbStartBarrier);
			CRandomContextProvider::RefreshRandomsCache();

			timepoint tpTestStartTime = CTimeUtils::GetCurrentMonotonicTimeNano();

			LaunchTheTest(sbStartBarrier);
			WaitTheTestEnd(sbFinishBarrier);

			timepoint tpTestEndTime = CTimeUtils::GetCurrentMonotonicTimeNano();

			timeduration tdTestDuration = atdObjectTestDurations[ftTestKind] = tpTestEndTime - tpTestStartTime;

			timeduration tdTotalUnavailabilityTime, tdMaxUnavailabilityTime;
			FreeTestThreads(sbExitBarrier, LOCKTEST_THREAD_COUNT, tdTotalUnavailabilityTime, tdMaxUnavailabilityTime);
			atdObjectTestTotalUnavailabilityTimes[ftTestKind] = tdTotalUnavailabilityTime;
			atdObjectTestMaxUnavailabilityTimes[ftTestKind] = tdMaxUnavailabilityTime;

			PublishTestResults(ftTestKind, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, tpRunStartTime, flLevelToTest, tdTestDuration, tdTotalUnavailabilityTime, tdMaxUnavailabilityTime);

			sbStartBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
			sbFinishBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
			sbExitBarrier.ResetInstance(0);
			tpProgressInstance.ResetInstance();
		}
	}

	{
		const ERWLOCKFINETEST ftTestKind = LFT_MUTEXGEAR_INFINITE_TO_WP;
		const ERWLOCKLOCKTESTOBJECT toTestedObjectKind = (ERWLOCKLOCKTESTOBJECT)CRWLockFineTestTraits<ftTestKind>::test_object;
		const unsigned uiCustomWPOption = CRWLockFineTestTraits<ftTestKind>::custom_wp_opt;

		if (
#if !_MGTEST_HAVE_CXX11
			ttlTestLanguage == LTL_CPP ||
#endif
			(uiCustomWPOption != 0 && bSingleOperationTest)
			)
		{
			// Do nothing
		}
		else
		{
			const unsigned uiCustomizedImplementationOptions = tuiImplementationOptions | ENCODE_CUSTOM_WP_OPT(uiCustomWPOption);
			CRWLockImplementation<trsTryReadSupport, uiCustomizedImplementationOptions, toTestedObjectKind, ttlTestLanguage> liRWLock;
			AllocateTestThreads<uiCustomizedImplementationOptions, toTestedObjectKind>(liRWLock, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, tpRunStartTime, sbStartBarrier, sbFinishBarrier, sbExitBarrier, tpProgressInstance);

			WaitTestThreadsReady(sbStartBarrier);
			CRandomContextProvider::RefreshRandomsCache();

			timepoint tpTestStartTime = CTimeUtils::GetCurrentMonotonicTimeNano();

			LaunchTheTest(sbStartBarrier);
			WaitTheTestEnd(sbFinishBarrier);

			timepoint tpTestEndTime = CTimeUtils::GetCurrentMonotonicTimeNano();

			timeduration tdTestDuration = atdObjectTestDurations[ftTestKind] = tpTestEndTime - tpTestStartTime;

			timeduration tdTotalUnavailabilityTime, tdMaxUnavailabilityTime;
			FreeTestThreads(sbExitBarrier, LOCKTEST_THREAD_COUNT, tdTotalUnavailabilityTime, tdMaxUnavailabilityTime);
			atdObjectTestTotalUnavailabilityTimes[ftTestKind] = tdTotalUnavailabilityTime;
			atdObjectTestMaxUnavailabilityTimes[ftTestKind] = tdMaxUnavailabilityTime;

			PublishTestResults(ftTestKind, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, tpRunStartTime, flLevelToTest, tdTestDuration, tdTotalUnavailabilityTime, tdMaxUnavailabilityTime);

			// sbStartBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
			// sbFinishBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
			// sbExitBarrier.ResetInstance(0);
			// tpProgressInstance.ResetInstance();
		}
	}
	MG_STATIC_ASSERT(LFT__MAX == 6);

	printf(g_ascLockTestTimesFormat,
		(unsigned long)(atdObjectTestDurations[LFT_SYSTEM] / 1000000000), (unsigned long)(atdObjectTestDurations[LFT_SYSTEM] % 1000000000) / 1000000,
		(unsigned long)(atdObjectTestTotalUnavailabilityTimes[LFT_SYSTEM] / 1000000000), (unsigned long)(atdObjectTestTotalUnavailabilityTimes[LFT_SYSTEM] % 1000000000) / 1000000,
		(unsigned long)(atdObjectTestMaxUnavailabilityTimes[LFT_SYSTEM] / 1000000000), (unsigned long)(atdObjectTestMaxUnavailabilityTimes[LFT_SYSTEM] % 1000000000) / 1000000,
		(unsigned long)(atdObjectTestDurations[LFT_MUTEXGEAR] / 1000000000), (unsigned long)(atdObjectTestDurations[LFT_MUTEXGEAR] % 1000000000) / 1000000,
		(unsigned long)(atdObjectTestTotalUnavailabilityTimes[LFT_MUTEXGEAR] / 1000000000), (unsigned long)(atdObjectTestTotalUnavailabilityTimes[LFT_MUTEXGEAR] % 1000000000) / 1000000,
		(unsigned long)(atdObjectTestMaxUnavailabilityTimes[LFT_MUTEXGEAR] / 1000000000), (unsigned long)(atdObjectTestMaxUnavailabilityTimes[LFT_MUTEXGEAR] % 1000000000) / 1000000,
		(unsigned long)(atdObjectTestDurations[LFT_MUTEXGEAR_MINIMAL_TO_WP] / 1000000000), (unsigned long)(atdObjectTestDurations[LFT_MUTEXGEAR_MINIMAL_TO_WP] % 1000000000) / 1000000,
		(unsigned long)(atdObjectTestTotalUnavailabilityTimes[LFT_MUTEXGEAR_MINIMAL_TO_WP] / 1000000000), (unsigned long)(atdObjectTestTotalUnavailabilityTimes[LFT_MUTEXGEAR_MINIMAL_TO_WP] % 1000000000) / 1000000,
		(unsigned long)(atdObjectTestMaxUnavailabilityTimes[LFT_MUTEXGEAR_MINIMAL_TO_WP] / 1000000000), (unsigned long)(atdObjectTestMaxUnavailabilityTimes[LFT_MUTEXGEAR_MINIMAL_TO_WP] % 1000000000) / 1000000,
		(unsigned long)(atdObjectTestDurations[LFT_MUTEXGEAR_AVERAGE_TO_WP] / 1000000000), (unsigned long)(atdObjectTestDurations[LFT_MUTEXGEAR_AVERAGE_TO_WP] % 1000000000) / 1000000,
		(unsigned long)(atdObjectTestTotalUnavailabilityTimes[LFT_MUTEXGEAR_AVERAGE_TO_WP] / 1000000000), (unsigned long)(atdObjectTestTotalUnavailabilityTimes[LFT_MUTEXGEAR_AVERAGE_TO_WP] % 1000000000) / 1000000,
		(unsigned long)(atdObjectTestMaxUnavailabilityTimes[LFT_MUTEXGEAR_AVERAGE_TO_WP] / 1000000000), (unsigned long)(atdObjectTestMaxUnavailabilityTimes[LFT_MUTEXGEAR_AVERAGE_TO_WP] % 1000000000) / 1000000,
		(unsigned long)(atdObjectTestDurations[LFT_MUTEXGEAR_SUBSTANTIAL_TO_WP] / 1000000000), (unsigned long)(atdObjectTestDurations[LFT_MUTEXGEAR_SUBSTANTIAL_TO_WP] % 1000000000) / 1000000,
		(unsigned long)(atdObjectTestTotalUnavailabilityTimes[LFT_MUTEXGEAR_SUBSTANTIAL_TO_WP] / 1000000000), (unsigned long)(atdObjectTestTotalUnavailabilityTimes[LFT_MUTEXGEAR_SUBSTANTIAL_TO_WP] % 1000000000) / 1000000,
		(unsigned long)(atdObjectTestMaxUnavailabilityTimes[LFT_MUTEXGEAR_SUBSTANTIAL_TO_WP] / 1000000000), (unsigned long)(atdObjectTestMaxUnavailabilityTimes[LFT_MUTEXGEAR_SUBSTANTIAL_TO_WP] % 1000000000) / 1000000,
		(unsigned long)(atdObjectTestDurations[LFT_MUTEXGEAR_INFINITE_TO_WP] / 1000000000), (unsigned long)(atdObjectTestDurations[LFT_MUTEXGEAR_INFINITE_TO_WP] % 1000000000) / 1000000,
		(unsigned long)(atdObjectTestTotalUnavailabilityTimes[LFT_MUTEXGEAR_INFINITE_TO_WP] / 1000000000), (unsigned long)(atdObjectTestTotalUnavailabilityTimes[LFT_MUTEXGEAR_INFINITE_TO_WP] % 1000000000) / 1000000,
		(unsigned long)(atdObjectTestMaxUnavailabilityTimes[LFT_MUTEXGEAR_INFINITE_TO_WP] / 1000000000), (unsigned long)(atdObjectTestMaxUnavailabilityTimes[LFT_MUTEXGEAR_INFINITE_TO_WP] % 1000000000) / 1000000);
		MG_STATIC_ASSERT(LFT__MAX == 6);

	FreeThreadOperationBuffers(LOCKTEST_THREAD_COUNT);
	FinalizeTestResults(flLevelToTest);

	return true;
}


template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::AllocateThreadOperationBuffers(threadcntint ciTotalThreadCount)
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

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::FreeThreadOperationBuffers(threadcntint ciTotalThreadCount)
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

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
typename CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::operationidxint CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::GetMaximalThreadOperationCount() const
{
	operationidxint nThreadOperationCount;

	const iterationcntint uiIterationCount = m_uiIterationCount;
	const operationidxint iiWriteOperationCount = CRWLockWriteLockExecutor<trsTryReadSupport, tuiImplementationOptions, LTO__MIN, ttlTestLanguage>::GetThreadOperationCount(uiIterationCount), iiReadOperationCount = CRWLockReadLockExecutor<trsTryReadSupport, tuiImplementationOptions, LTO__MIN, ttlTestLanguage, 0>::GetThreadOperationCount(uiIterationCount);
	nThreadOperationCount = max(iiWriteOperationCount, iiReadOperationCount);

	MG_STATIC_ASSERT(LOCKTEST_THREAD_COUNT == (threadcntint)(LOCKTEST_WRITER_COUNT + LOCKTEST_READER_COUNT));

	return nThreadOperationCount;
}


template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
template<unsigned tuiCustomizedImplementationOptions, ERWLOCKLOCKTESTOBJECT ttoTestedObjectKind>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::AllocateTestThreads(
	CRWLockImplementation<trsTryReadSupport, tuiCustomizedImplementationOptions, ttoTestedObjectKind, ttlTestLanguage> &liRefRWLock,
	threadcntint ciWriterCount, threadcntint ciReraderCount, timepoint tpRunStartTime, 
	CThreadExecutionBarrier &sbRefStartBarrier, CThreadExecutionBarrier &sbRefFinishBarrier, CThreadExecutionBarrier &sbRefExitBarrier,
	CRWLockLockTestProgress &tpRefProgressInstance)
{
	const iterationcntint uiIterationCount = m_uiIterationCount;
	randcontextint ciRunRandomContext = CRandomContextProvider::ConvertTimeToRandcontext(tpRunStartTime);

	threadcntint uiThreadIndex = 0;

	typename CRWLockWriteLockExecutor<trsTryReadSupport, tuiCustomizedImplementationOptions, ttoTestedObjectKind, ttlTestLanguage>::CConstructionParameter cpWriteExecutorParameter(liRefRWLock);

	const threadcntint ciWritersEndIndex = uiThreadIndex + ciWriterCount;
	for (; uiThreadIndex != ciWritersEndIndex; ++uiThreadIndex)
	{
		MG_ASSERT(m_aittTestThreads[uiThreadIndex] == NULL);

		typedef CRWLockLockTestThread<CRWLockWriteLockExecutor<trsTryReadSupport, tuiCustomizedImplementationOptions, ttoTestedObjectKind, ttlTestLanguage> > CUsedRWLockLockTestThread;
		typedef typename CUsedRWLockLockTestThread::CThreadParameterInfo CUsedThreadParameterInfo;
		randcontextint ciThreadRandomContext = CRandomContextProvider::AdjutsRandcontextForThreadIndex(ciRunRandomContext, uiThreadIndex);
		CUsedRWLockLockTestThread *pttThreadInstance = new CUsedRWLockLockTestThread(CUsedThreadParameterInfo(sbRefStartBarrier, sbRefFinishBarrier, sbRefExitBarrier, tpRefProgressInstance), ciThreadRandomContext, uiIterationCount, cpWriteExecutorParameter, m_apiiThreadOperationIndices[uiThreadIndex]);
		m_aittTestThreads[uiThreadIndex] = pttThreadInstance;
	}

	typename CRWLockReadLockExecutor<trsTryReadSupport, tuiCustomizedImplementationOptions, ttoTestedObjectKind, ttlTestLanguage, tuiReaderWriteDivisor>::CConstructionParameter cpReadExecutorParameter(liRefRWLock);

	const threadcntint ciReadersEndIndex = uiThreadIndex + ciReraderCount;
	for (; uiThreadIndex != ciReadersEndIndex; ++uiThreadIndex)
	{
		MG_ASSERT(m_aittTestThreads[uiThreadIndex] == NULL);

		typedef CRWLockLockTestThread<CRWLockReadLockExecutor<trsTryReadSupport, tuiCustomizedImplementationOptions, ttoTestedObjectKind, ttlTestLanguage, tuiReaderWriteDivisor> > CUsedRWLockLockTestThread;
		typedef typename CUsedRWLockLockTestThread::CThreadParameterInfo CUsedThreadParameterInfo;
		randcontextint ciThreadRandomContext = CRandomContextProvider::AdjutsRandcontextForThreadIndex(ciRunRandomContext, uiThreadIndex);
		CUsedRWLockLockTestThread *pttThreadInstance = new CUsedRWLockLockTestThread(CUsedThreadParameterInfo(sbRefStartBarrier, sbRefFinishBarrier, sbRefExitBarrier, tpRefProgressInstance), ciThreadRandomContext, uiIterationCount, cpReadExecutorParameter, m_apiiThreadOperationIndices[uiThreadIndex]);
		m_aittTestThreads[uiThreadIndex] = pttThreadInstance;
	}
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::FreeTestThreads(CThreadExecutionBarrier &sbRefExitBarrier, threadcntint ciTotalThreadCount,
	timeduration &tdOutTotalUnavailabilityTime, timeduration &tdOutMaxUnavailabilityTime)
{
	sbRefExitBarrier.SignalReleaseEvent();

	timeduration tdGlobalMinimumLockDuration = CTimeUtils::GetMaxTimeduration();
	timeduration tdTotalMinDurationUnavailabilityTime = 0, tdTotalHigherDurationUnavailabilityTime = 0;
	timeduration tdGlobalMaxUnavailabilityTime = 0;

	for (threadcntint uiThreadIndex = 0; uiThreadIndex != ciTotalThreadCount; ++uiThreadIndex)
	{
		IRWLockLockTestThread *ittCurrentTestThread = m_aittTestThreads[uiThreadIndex];

		if (ittCurrentTestThread != NULL)
		{
			m_aittTestThreads[uiThreadIndex] = NULL;

			timeduration tdThreadMinimumLockDuration, tdThreadMinDurationUnavailabilityTime, tdThreadHigherDurationUnavailabilityTime;
			timeduration tdThreadMaxUnavailabilityTime;
			ittCurrentTestThread->ReleaseInstance(tdThreadMinimumLockDuration, tdThreadMinDurationUnavailabilityTime, tdThreadHigherDurationUnavailabilityTime, tdThreadMaxUnavailabilityTime);

			if (CLockDurationUtils::IsDurationSmaller(tdThreadMinimumLockDuration, tdGlobalMinimumLockDuration))
			{
				tdTotalHigherDurationUnavailabilityTime += tdTotalMinDurationUnavailabilityTime;
				tdTotalMinDurationUnavailabilityTime = tdThreadMinDurationUnavailabilityTime;
				tdGlobalMinimumLockDuration = tdThreadMinimumLockDuration;
			}
			else if (CLockDurationUtils::IsDurationSmaller(tdGlobalMinimumLockDuration, tdThreadMinimumLockDuration))
			{
				tdTotalHigherDurationUnavailabilityTime += tdThreadMinDurationUnavailabilityTime;
			}
			else
			{
				tdTotalMinDurationUnavailabilityTime += tdThreadMinDurationUnavailabilityTime;
			}

			tdTotalHigherDurationUnavailabilityTime += tdThreadHigherDurationUnavailabilityTime;

			tdGlobalMaxUnavailabilityTime = std::max(tdGlobalMaxUnavailabilityTime, tdThreadMaxUnavailabilityTime);
		}
	}

	tdOutTotalUnavailabilityTime = tdTotalHigherDurationUnavailabilityTime;
	tdOutMaxUnavailabilityTime = tdGlobalMaxUnavailabilityTime;
}


template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::WaitTestThreadsReady(CThreadExecutionBarrier &sbStartBarrier)
{
	while (sbStartBarrier.RetrieveCounterValue() != 0)
	{
		CTimeUtils::Sleep(1);
	}

	// Sleep a longer period for all the threads to enter blocked state 
	// and the system to finish any disk cache flush activity from previous tests
	CTimeUtils::Sleep(g_uiSettleDownSleepDelay);
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::LaunchTheTest(CThreadExecutionBarrier &sbStartBarrier)
{
	sbStartBarrier.SignalReleaseEvent();
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::WaitTheTestEnd(CThreadExecutionBarrier &sbFinishBarrier)
{
	sbFinishBarrier.WaitReleaseEvent();
}


template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::InitializeTestResults(threadcntint ciWriterCount, threadcntint ciReraderCount,
	EMGTESTFEATURELEVEL flTestLevel)
{
	int iFileOpenStatus;
	char ascFileNameFormatBuffer[256];

	volatile unsigned uiReaderWriteDivisor; // The volatile is necessary to avoid a compile error in VS2013
	const unsigned uiWritesPercent = tuiReaderWriteDivisor != 0 ? (uiReaderWriteDivisor = tuiReaderWriteDivisor, 100 / uiReaderWriteDivisor) : 0;

	for (ERWLOCKFINETEST ftTestKind = LFT__MIN; ftTestKind != LFT__MAX; ++ftTestKind)
	{
		MG_ASSERT(m_apsfOperationDetailFiles[ftTestKind] == NULL);

		if (IN_RANGE(flTestLevel, MGTFL__DUMP_MIN, MGTFL__DUMP_MAX))
		{
			const char *szTRDLSupportFileNameSuffix = g_aszTRDLSupportFileNameSuffixes[trsTryReadSupport];
			const char *szObjectKindFileNameSuffix = g_aszTestedObjectKindFileNameSuffixes[ftTestKind];
			const char *szLanguageNameSuffix = g_aszTestedObjectLanguageNames[ttlTestLanguage];
			int iPrintResult = snprintf(ascFileNameFormatBuffer, sizeof(ascFileNameFormatBuffer), g_ascLockTestDetailsFileNameFormat,
				szTRDLSupportFileNameSuffix, tuiReaderWriteDivisor == 0 ? (unsigned int)ciWriterCount : (unsigned int)uiWritesPercent, tuiReaderWriteDivisor == 0 ? "" : "%", (unsigned int)ciReraderCount,
				szObjectKindFileNameSuffix, szLanguageNameSuffix);
			MG_CHECK(iPrintResult, IN_RANGE(iPrintResult - 1, 0, ARRAY_SIZE(ascFileNameFormatBuffer) - 1) || (iPrintResult < 0 && (iPrintResult = -errno, false)));

			FILE *psfObjectKindDetailsFile = fopen(ascFileNameFormatBuffer, "w+");
			MG_CHECK(iFileOpenStatus, psfObjectKindDetailsFile != NULL || (iFileOpenStatus = errno, false));

			m_apsfOperationDetailFiles[ftTestKind] = psfObjectKindDetailsFile;
		}
	}
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::PublishTestResults(ERWLOCKFINETEST ftTestKind,
	threadcntint ciWriterCount, threadcntint ciReaderCount, timepoint tpRunStartTime, EMGTESTFEATURELEVEL flTestLevel, timeduration tdTestDuration, timeduration tdTotalUnavailabilityTime, timeduration tdMaxUnavailabilityTime)
{
	MG_ASSERT(IN_RANGE(ftTestKind, LFT__MIN, LFT__MAX));

	if (IN_RANGE(flTestLevel, MGTFL__DUMP_MIN, MGTFL__DUMP_MAX))
	{
		volatile unsigned uiReaderWriteDivisor; // The volatile is necessary to avoid a compile error in VS2013
		const unsigned uiWritesPercent = tuiReaderWriteDivisor != 0 ? (uiReaderWriteDivisor = tuiReaderWriteDivisor, 100 / uiReaderWriteDivisor) : 0;

		FILE *psfDetailsFile = m_apsfOperationDetailFiles[ftTestKind];
		int iPrintResult = fprintf(psfDetailsFile, "Lock-unlock test for %s with %u%s write%s%s, %u %s%s\n"
			"Consumed Time (less is better): %lu sec %lu nsec\nSignificant Blocking Incurred Delays (less is better): %lu sec %lu nsec\nMaximal Blocking Incurred Delay (less is better): %lu sec %lu nsec\n"
			"Legend:\n  a0+ = reads, s0+ = writes\n  upper case = lock, lower case = unlock\n  trailing comma = locked with blocking variant, trailing dot = locked with try- variant\n"
			"\nOperation sequence map:\n",
			g_aszTestedObjectLanguageNames[ttlTestLanguage],
			tuiReaderWriteDivisor == 0 ? (unsigned int)ciWriterCount : (unsigned int)uiWritesPercent, tuiReaderWriteDivisor == 0 ? "" : "%",
			tuiReaderWriteDivisor == 0 ? "r" : "s", tuiReaderWriteDivisor == 0 && ciWriterCount != 1 ? "s" : "", (unsigned int)ciReaderCount,
			tuiReaderWriteDivisor == 0 ? "reader" : "thread", ciReaderCount != 1 ? "s" : "",
			(unsigned long)(tdTestDuration / 1000000000), (unsigned long)(tdTestDuration % 1000000000),
			(unsigned long)(tdTotalUnavailabilityTime / 1000000000), (unsigned long)(tdTotalUnavailabilityTime % 1000000000),
			(unsigned long)(tdMaxUnavailabilityTime / 1000000000), (unsigned long)(tdMaxUnavailabilityTime % 1000000000));
		MG_CHECK(iPrintResult, iPrintResult > 0 || (iPrintResult = errno, false));

		const threadcntint ciTotalThreadCount = ciWriterCount + ciReaderCount;
		operationidxint *piiThreadLastOperationIndices = new operationidxint[ciTotalThreadCount];
		std::fill(piiThreadLastOperationIndices, piiThreadLastOperationIndices + ciTotalThreadCount, 0);

		randcontextint ciRunRandomContext = CRandomContextProvider::ConvertTimeToRandcontext(tpRunStartTime);
		randcontextint *pciThreadRandomContexts = new randcontextint[ciTotalThreadCount];
		FillInitialThreadRandomContexts(pciThreadRandomContexts, ciTotalThreadCount, ciRunRandomContext);

		unsigned *puiThreadRandomData = new unsigned[ciTotalThreadCount];
		std::fill(puiThreadRandomData, puiThreadRandomData + ciTotalThreadCount, 0);

		operationidxint iiLastSavedOperationIndex = 0;
		char *pscMergeBuffer = GetDetailsMergeBuffer();
		const operationidxint nThreadOperationCount = GetThreadOperationCount();
		for (threadcntint ciCurrentBufferIndex = 0; ciCurrentBufferIndex != ciTotalThreadCount; ++ciCurrentBufferIndex)
		{
			BuildMergedThreadOperationMap(pscMergeBuffer, iiLastSavedOperationIndex, nThreadOperationCount, piiThreadLastOperationIndices, ciWriterCount, ciReaderCount, pciThreadRandomContexts, puiThreadRandomData);
			SaveThreadOperationMapToDetails(ftTestKind, pscMergeBuffer, nThreadOperationCount);
			iiLastSavedOperationIndex += nThreadOperationCount;
		}

		delete[] puiThreadRandomData;
		delete[] pciThreadRandomContexts;
		delete[] piiThreadLastOperationIndices;
	}
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::FillInitialThreadRandomContexts(
	randcontextint aciThreadRandomContexts[], threadcntint ciTotalThreadCount, randcontextint ciRunRandomContext)
{
	for (threadcntint ciCurrentThreadIndex = 0; ciCurrentThreadIndex != ciTotalThreadCount; ++ciCurrentThreadIndex)
	{
		aciThreadRandomContexts[ciCurrentThreadIndex] = CRandomContextProvider::AdjutsRandcontextForThreadIndex(ciRunRandomContext, ciCurrentThreadIndex);
	}
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::BuildMergedThreadOperationMap(char ascMergeBuffer[], operationidxint iiLastSavedOperationIndex, operationidxint iiThreadOperationCount,
	operationidxint aiiThreadLastOperationIndices[], threadcntint ciWriterCount, threadcntint ciReaderCount, randcontextint aciThreadRandomContexts[], unsigned puiThreadRandomData[])
{
	char ascCurrentThreadMapChars[LOCK_THREAD_NAME_LENGTH + 1]; // +1 for terminal zero
	threadcntint ciCurrentThreadIndex = 0;

	InitializeThreadName(ascCurrentThreadMapChars, g_cWriterMapFirstChar);
	const threadcntint ciWriterThreadsBegin = ciCurrentThreadIndex, ciWriterThreadsEnd = ciCurrentThreadIndex + ciWriterCount;
	for (; ciCurrentThreadIndex != ciWriterThreadsEnd; )
	{
		const operationidxint *piiThreadOperationIndices = m_apiiThreadOperationIndices[ciCurrentThreadIndex];
		aiiThreadLastOperationIndices[ciCurrentThreadIndex] = FillThreadOperationMap(ascMergeBuffer, iiLastSavedOperationIndex, iiThreadOperationCount, piiThreadOperationIndices, aiiThreadLastOperationIndices[ciCurrentThreadIndex], aciThreadRandomContexts[ciCurrentThreadIndex], puiThreadRandomData[ciCurrentThreadIndex], ascCurrentThreadMapChars);

		++ciCurrentThreadIndex;
		IncrementThreadName(ascCurrentThreadMapChars, ciCurrentThreadIndex - ciWriterThreadsBegin);
	}

	InitializeThreadName(ascCurrentThreadMapChars, g_cReaderMapFirstChar);
	const threadcntint ciReaderThreadsBegin = ciCurrentThreadIndex, ciReaderThreadsEnd = ciCurrentThreadIndex + ciReaderCount;
	for (; ciCurrentThreadIndex != ciReaderThreadsEnd; )
	{
		const operationidxint *piiThreadOperationIndices = m_apiiThreadOperationIndices[ciCurrentThreadIndex];
		aiiThreadLastOperationIndices[ciCurrentThreadIndex] = FillThreadOperationMap(ascMergeBuffer, iiLastSavedOperationIndex, iiThreadOperationCount, piiThreadOperationIndices, aiiThreadLastOperationIndices[ciCurrentThreadIndex], aciThreadRandomContexts[ciCurrentThreadIndex], puiThreadRandomData[ciCurrentThreadIndex], ascCurrentThreadMapChars);

		++ciCurrentThreadIndex;
		IncrementThreadName(ascCurrentThreadMapChars, ciCurrentThreadIndex - ciReaderThreadsBegin);
	}
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
typename CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::operationidxint CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::FillThreadOperationMap(
	char ascMergeBuffer[], operationidxint iiLastSavedOperationIndex, const operationidxint iiThreadOperationCount, 
	const operationidxint *piiThreadOperationIndices, operationidxint iiThreadLastOperationIndex, randcontextint &ciVarThreadRandomContext, unsigned &uiVarThreadRandomData, 
	const char ascThreadMapChars[LOCK_THREAD_NAME_LENGTH])
{
	randcontextint ciOperationRandomContext = ciVarThreadRandomContext;
	unsigned uiRandomData = uiVarThreadRandomData;

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

		if (tuiReaderWriteDivisor != 0)
		{
			MG_ASSERT(IN_RANGE(ascThreadMapChars[0], g_cReaderMapFirstChar, g_cWriterMapFirstChar));

			if (IN_RANGE(okOperationKind, LOK__RANDCONTEXT_UPDATE_MIN, LOK__RANDCONTEXT_UPDATE_MAX))
			{
				uiRandomData = CRandomContextProvider::ExtractRandomItem(ciOperationRandomContext);
				ciOperationRandomContext = CRandomContextProvider::MixContextWithData(ciOperationRandomContext, uiRandomData);
			}

			MG_STATIC_ASSERT((tuiReaderWriteDivisor & (tuiReaderWriteDivisor - 1)) == 0); // For the test below...

			if ((uiRandomData & (tuiReaderWriteDivisor - 1)) == 0)
			{
				ascMergeBuffer[siNameInsertOffset] += g_cWriterMapFirstChar - g_cReaderMapFirstChar;
			}
		}

		ascMergeBuffer[siNameInsertOffset] += g_ascLockOperationKindThreadNameFirstCharModifiers[okOperationKind];
	}

	uiVarThreadRandomData = uiRandomData;
	ciVarThreadRandomContext = ciOperationRandomContext;
	return iiThreadCurrentOperationIndex;
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::InitializeThreadName(char ascThreadMapChars[LOCK_THREAD_NAME_LENGTH + 1], char cMapFirstChar)
{
	ascThreadMapChars[0] = cMapFirstChar;
	int iPrintResult = snprintf(ascThreadMapChars + 1, LOCK_THREAD_NAME_LENGTH, "%0*x", LOCK_THREAD_NAME_LENGTH - 1, (unsigned int)0);
	MG_CHECK(iPrintResult, iPrintResult == LOCK_THREAD_NAME_LENGTH - 1 || (iPrintResult < 0 && (iPrintResult = -errno, false)));
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::IncrementThreadName(char ascThreadMapChars[LOCK_THREAD_NAME_LENGTH + 1], threadcntint ciThreadIndex)
{
	threadcntint ciThreadIndexNumericPart = ciThreadIndex % ((threadcntint)1 << (LOCK_THREAD_NAME_LENGTH - 1) * 4); // 4 bits per a hexadecimal digit
	if (ciThreadIndexNumericPart == 0)
	{
		++ascThreadMapChars[0];
	}

	int iPrintResult = snprintf(ascThreadMapChars + 1, LOCK_THREAD_NAME_LENGTH, "%0*x", LOCK_THREAD_NAME_LENGTH - 1, (unsigned int)ciThreadIndexNumericPart);
	MG_CHECK(iPrintResult, iPrintResult == LOCK_THREAD_NAME_LENGTH - 1 || (iPrintResult < 0 && (iPrintResult = -errno, false)));
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::SaveThreadOperationMapToDetails(ERWLOCKFINETEST ftTestKind, 
	const char ascMergeBuffer[], operationidxint iiThreadOperationCount)
{
	MG_ASSERT(iiThreadOperationCount % g_uiLockOperationsPerLine == 0);

	FILE *psfDetailsFile = m_apsfOperationDetailFiles[ftTestKind];

	const size_t siBufferEnndOffset = (size_t)iiThreadOperationCount * (LOCK_THREAD_NAME_LENGTH + 1);
	for (size_t siCurrentOffset = 0; siCurrentOffset != siBufferEnndOffset; siCurrentOffset += g_uiLockOperationsPerLine * (LOCK_THREAD_NAME_LENGTH + 1))
	{
		int iPrintResult = fprintf(psfDetailsFile, "%.*s\n", (int)(g_uiLockOperationsPerLine * (LOCK_THREAD_NAME_LENGTH + 1)), ascMergeBuffer + siCurrentOffset);
		MG_CHECK(iPrintResult, iPrintResult > 0 || (iPrintResult = -errno, false));
	}
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage>::FinalizeTestResults(EMGTESTFEATURELEVEL flTestLevel)
{
	for (ERWLOCKFINETEST ftTestKind = LFT__MIN; ftTestKind != LFT__MAX; ++ftTestKind)
	{
		if (IN_RANGE(flTestLevel, MGTFL__DUMP_MIN, MGTFL__DUMP_MAX))
		{
			FILE *psfObjectKindDetailsFile = m_apsfOperationDetailFiles[ftTestKind];

			if (psfObjectKindDetailsFile != NULL)
			{
				m_apsfOperationDetailFiles[ftTestKind] = NULL;

				fclose(psfObjectKindDetailsFile);
			}
		}

		MG_ASSERT(m_apsfOperationDetailFiles[ftTestKind] == NULL);
	}
}

//////////////////////////////////////////////////////////////////////////
// CRWLockLockTestThread

template<class TOperationExecutor>
/*virtual */
CRWLockLockTestThread<TOperationExecutor>::~CRWLockLockTestThread()
{
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
	randcontextint ciOperationRandomContext = m_ciThreadRandomContext;

	timeduration tdMinimumLockDuration = CTimeUtils::GetMaxTimeduration();
	timeduration tdMinDurationUnavailabilityTime = 0, tdHigherDurationUnavailabilityTime = 0;
	timeduration tdMaxUnavailabilityTime = 0;

	const iterationcntint uiIterationCount = m_uiIterationCount;
	for (iterationcntint uiIterationIndex = 0; uiIterationIndex != uiIterationCount; ++uiIterationIndex)
	{
		unsigned uiRandomData = CRandomContextProvider::ExtractRandomItem(ciOperationRandomContext);
		ciOperationRandomContext = CRandomContextProvider::MixContextWithData(ciOperationRandomContext, uiRandomData);

		timepoint tpPreLockTime = CTimeUtils::GetCurrentMonotonicTimeNano();
		bool bLockedWitTryVariant = m_oeOperationExecutor.ApplyLock(eoRefExecutorExtraObjects, uiRandomData);
		operationidxint iiLockOperationSequence = ptpProgressInstance->GenerateOperationIndex();
		volatile timepoint tpPostLockTime = CTimeUtils::GetCurrentMonotonicTimeNano();

		*piiOperationIndexBuffer++ = ENCODE_SEQUENCE_WITH_TRYVARIANT(iiLockOperationSequence, bLockedWitTryVariant);

		iRandomNumber = rand(); // Generate a random number to simulate a small delay

		operationidxint iiUnlockOperationSequence = ptpProgressInstance->GenerateOperationIndex();
		*piiOperationIndexBuffer++ = iiUnlockOperationSequence;

		m_oeOperationExecutor.ReleaseLock(eoRefExecutorExtraObjects, uiRandomData);
		MG_STATIC_ASSERT(LOK__MAX == 2);

		if (!bLockedWitTryVariant)
		{
			CTimeUtils::timeduration tdLockDuration = tpPostLockTime - tpPreLockTime;

			if (tdLockDuration != 0)
			{
				if (CLockDurationUtils::IsDurationSmaller(tdLockDuration, tdMinimumLockDuration))
				{
					tdHigherDurationUnavailabilityTime += tdMinDurationUnavailabilityTime;
					tdMinDurationUnavailabilityTime = tdLockDuration;
					tdMinimumLockDuration = tdLockDuration;
					
					// The comparison needs to be done here to ensure initialization for the case when all durations are minimal
					tdMaxUnavailabilityTime = std::max(tdMaxUnavailabilityTime, tdLockDuration);
				}
				else if (CLockDurationUtils::IsDurationSmaller(tdMinimumLockDuration, tdLockDuration))
				{
					tdHigherDurationUnavailabilityTime += tdLockDuration;

					tdMaxUnavailabilityTime = std::max(tdMaxUnavailabilityTime, tdLockDuration);
				}
				else
				{
					tdMinDurationUnavailabilityTime += tdLockDuration;
				}
			}
		}
	}

	StoreUnavailabilityTimes(tdMinimumLockDuration, tdMinDurationUnavailabilityTime, tdHigherDurationUnavailabilityTime, tdMaxUnavailabilityTime);
}

template<class TOperationExecutor>
/*virtual */
void CRWLockLockTestThread<TOperationExecutor>::ReleaseInstance(timeduration &tdOutMinimumLockDuration, timeduration &tdOutMinDurationUnavailabilityTime, timeduration &tdOutHigherDurationUnavailabilityTime,
	timeduration &tdOutMaxUnavailabilityTime)
{
	StopRunningThread();
	RetrieveUnavailabilityTimes(tdOutMinimumLockDuration, tdOutMinDurationUnavailabilityTime, tdOutHigherDurationUnavailabilityTime, tdOutMaxUnavailabilityTime);
	
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
	MGWLF_16T_25PW_CPP,
	MGWLF_32T_25PW_CPP,
	MGWLF_64T_25PW_C,

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

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
bool TestRWLockLocks()
{
	CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, 0, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage> ltTestInstance(g_uiLockIterationCount);
	return ltTestInstance.RunTheTask();
}

template<unsigned int tuiThreadCount, unsigned int tuiWriteDivivisor, unsigned int tuiImplementationOptions, ERWLOCKTESTTRYREADSUPPORT trsTryReadSupport, ERWLOCKLOCKTESTLANGUAGE ttlTestLanguage>
bool TestRWLockMixed()
{
	CRWLockLockTestExecutor<0, tuiThreadCount, tuiWriteDivivisor, tuiImplementationOptions, trsTryReadSupport, ttlTestLanguage> ltTestInstance(g_uiLockIterationCount);
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
	MGTFL_QUICK, // MGWLF_16T_25PW_CPP,
	MGTFL_QUICK, // MGWLF_32T_25PW_CPP,
	MGTFL_BASIC, // MGWLF_64T_25PW_C,

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
	&TestRWLockLocks<1, 0, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_C,
	&TestRWLockLocks<4, 0, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_C,
	&TestRWLockLocks<8, 0, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_C,
	&TestRWLockLocks<16, 0, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16TW_C,

	&TestRWLockLocks<0, 1, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TR_C,
	&TestRWLockLocks<0, 4, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TR_C,
	&TestRWLockLocks<0, 16, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16TR_C,
	&TestRWLockLocks<0, 64, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_64TR_C,

	&TestRWLockMixed<8, 8, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8T_12PW_C,
	&TestRWLockMixed<16, 8, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16T_12PW_C,
	&TestRWLockMixed<32, 8, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_32T_12PW_C,
	&TestRWLockMixed<64, 8, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_64T_12PW_C,

	&TestRWLockMixed<8, 4, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8T_25PW_C,
#if _MGTEST_HAVE_CXX11
	&TestRWLockMixed<16, 4, 0, TRS_NO_TRYREAD_SUPPORT, LTL_CPP>, // MGWLF_16T_25PW_CPP,
	&TestRWLockMixed<32, 4, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_NO_TRYREAD_SUPPORT, LTL_CPP>, // MGWLF_32T_25PW_CPP,
#else // #if !_MGTEST_HAVE_CXX11
	&TestRWLockMixed<16, 4, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16T_25PW_CPP,
	&TestRWLockMixed<32, 4, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_32T_25PW_CPP,
#endif  // #if !_MGTEST_HAVE_CXX11
	&TestRWLockMixed<64, 4, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_64T_25PW_C,

	&TestRWLockMixed<8, 2, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8T_50PW_C,
	&TestRWLockMixed<16, 2, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16T_50PW_C,
	&TestRWLockMixed<32, 2, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_32T_50PW_C,
	&TestRWLockMixed<64, 2, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_64T_50PW_C,

	&TestRWLockLocks<1, 1, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_1TR_C,
	&TestRWLockLocks<1, 4, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_4TR_C,
	&TestRWLockLocks<1, 8, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_8TR_C,
	&TestRWLockLocks<1, 16, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_16TR_C,

	&TestRWLockLocks<4, 4, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_4TR_C,
	&TestRWLockLocks<4, 8, 0, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_8TR_C,
	&TestRWLockLocks<4, 16, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_16TR_C,
	&TestRWLockLocks<4, 32, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_32TR_C,

	&TestRWLockLocks<8, 16, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_16TR_C,
	&TestRWLockLocks<8, 32, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_32TR_C,
	&TestRWLockLocks<8, 64, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_64TR_C,
	&TestRWLockLocks<8, 128, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_NO_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_128TR_C,
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
#if _MGTEST_HAVE_CXX11
	"25% writes, 16 threads, C++", // MGWLF_16T_25PW_CPP,
	"25% writes @" MAKE_STRING_LITERAL(MGTEST_RWLOCK_WRITE_CHANNELS) "cnl, 32 threads, C++", // MGWLF_32T_25PW_CPP,
#else // #if !_MGTEST_HAVE_CXX11
	"25% writes, 16 threads, C", // MGWLF_16T_25PW_CPP,
	"25% writes @" MAKE_STRING_LITERAL(MGTEST_RWLOCK_WRITE_CHANNELS) "cnl, 32 threads, C", // MGWLF_32T_25PW_CPP,
#endif // #if !_MGTEST_HAVE_CXX11
	"25% writes @" MAKE_STRING_LITERAL(MGTEST_RWLOCK_WRITE_CHANNELS) "cnl, 64 threads, C", // MGWLF_64T_25PW_C,

	"50% writes, 8 threads, C",  // MGWLF_8T_50PW_C,
	"50% writes @" MAKE_STRING_LITERAL(MGTEST_RWLOCK_WRITE_CHANNELS) "cnl, 16 threads, C", // MGWLF_16T_50PW_C,
	"50% writes @" MAKE_STRING_LITERAL(MGTEST_RWLOCK_WRITE_CHANNELS) "cnl, 32 threads, C", // MGWLF_32T_50PW_C,
	"50% writes @" MAKE_STRING_LITERAL(MGTEST_RWLOCK_WRITE_CHANNELS) "cnl, 64 threads, C", // MGWLF_64T_50PW_C,

	"1 Writer, 1 Reader, C", // MGWLF_1TW_1TR_C,
	"1 Writer, 4 Readers, C", // MGWLF_1TW_4TR_C,
	"1 Writer, 8 Readers, C", // MGWLF_1TW_8TR_C,
	"1 Writer, 16 Readers, C", // MGWLF_1TW_16TR_C,

	"4 Writers, 4 Readers, C", // MGWLF_4TW_4TR_C,
	"4 Writers, 8 Readers, C", // MGWLF_4TW_8TR_C,
	"4 Writers @" MAKE_STRING_LITERAL(MGTEST_RWLOCK_WRITE_CHANNELS) "cnl, 16 Readers, C", // MGWLF_4TW_16TR_C,
	"4 Writers @" MAKE_STRING_LITERAL(MGTEST_RWLOCK_WRITE_CHANNELS) "cnl, 32 Readers, C", // MGWLF_4TW_32TR_C,

	"8 Writers @" MAKE_STRING_LITERAL(MGTEST_RWLOCK_WRITE_CHANNELS) "cnl, 16 Readers, C", // MGWLF_8TW_16TR_C,
	"8 Writers @" MAKE_STRING_LITERAL(MGTEST_RWLOCK_WRITE_CHANNELS) "cnl, 32 Readers, C", // MGWLF_8TW_32TR_C,
	"8 Writers @" MAKE_STRING_LITERAL(MGTEST_RWLOCK_WRITE_CHANNELS) "cnl, 64 Readers, C", // MGWLF_8TW_64TR_C,
	"8 Writers @" MAKE_STRING_LITERAL(MGTEST_RWLOCK_WRITE_CHANNELS) "cnl, 128 Readers, C", // MGWLF_8TW_128TR_C,
};
MG_STATIC_ASSERT(ARRAY_SIZE(g_aszRWLockFeatureTestNames) == MGWLF__MAX);


/*static */EMGTESTFEATURELEVEL	CRWLockTest::m_flSelectedFeatureTestLevel = MGTFL__DEFAULT;


/*static */
bool CRWLockTest::RunBasicImplementationTest(unsigned int &nOutSuccessCount, unsigned int &nOutTestCount)
{
	unsigned int nSuccessCount = 0, nTestCount = 0;

	_mg_atomic_init_uint(&g_uiPriorityAdjustmentErrorReported, 0);

	EMGTESTFEATURELEVEL flLevelToTest = CRWLockTest::RetrieveSelectedFeatureTestLevel();

	printf("%s\n", g_ascLockTestLegend);

	int iCaptionWidth = ARRAY_SIZE(g_ascFeatureTestingText) - 1 + g_iTestFeatureNameWidth + ARRAY_SIZE(g_ascPostFeatureSuffix) - 1 + ARRAY_SIZE(g_ascLockTestTableCaption) - 1;
	printf("%*s\n%*s\n", iCaptionWidth, g_ascLockTestTableCaption, iCaptionWidth, g_ascLockTestTableSubCapn);

	for (EMGRWLOCKFEATURE lfRWLockFeature = MGWLF__TESTBEGIN; lfRWLockFeature != MGWLF__TESTEND; ++lfRWLockFeature)
	{
		if (g_aflRWLockFeatureTestLevels[lfRWLockFeature] <= flLevelToTest)
		{
			++nTestCount;

			const char *szFeatureName = g_aszRWLockFeatureTestNames[lfRWLockFeature];
			printf("%s%*s%s", g_ascFeatureTestingText, g_iTestFeatureNameWidth, szFeatureName, g_ascPostFeatureSuffix);

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
	&TestRWLockLocks<1, 0, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_C,
	&TestRWLockLocks<4, 0, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_C,
	&TestRWLockLocks<8, 0, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_C,
	&TestRWLockLocks<16, 0, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16TW_C,

	&TestRWLockLocks<0, 1, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TR_C,
	&TestRWLockLocks<0, 4, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TR_C,
	&TestRWLockLocks<0, 16, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16TR_C,
	&TestRWLockLocks<0, 64, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_64TR_C,

	&TestRWLockMixed<8, 8, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8T_12PW_C,
	&TestRWLockMixed<16, 8, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16T_12PW_C,
	&TestRWLockMixed<32, 8, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_32T_12PW_C,
	&TestRWLockMixed<64, 8, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_64T_12PW_C,

	&TestRWLockMixed<8, 4, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8T_25PW_C,
#if _MGTEST_HAVE_CXX11
	&TestRWLockMixed<16, 4, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_CPP>, // MGWLF_16T_25PW_CPP,
	&TestRWLockMixed<32, 4, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_WITH_TRYREAD_SUPPORT, LTL_CPP>, // MGWLF_32T_25PW_CPP,
#else  // #if !_MGTEST_HAVE_CXX11
	&TestRWLockMixed<16, 4, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16T_25PW_CPP,
	&TestRWLockMixed<32, 4, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_32T_25PW_CPP,
#endif // #if !_MGTEST_HAVE_CXX11
	&TestRWLockMixed<64, 4, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_64T_25PW_C,

	&TestRWLockMixed<8, 2, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8T_50PW_C,
	&TestRWLockMixed<16, 2, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_16T_50PW_C,
	&TestRWLockMixed<32, 2, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_32T_50PW_C,
	&TestRWLockMixed<64, 2, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_64T_50PW_C,

	&TestRWLockLocks<1, 1, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_1TR_C,
	&TestRWLockLocks<1, 4, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_4TR_C,
	&TestRWLockLocks<1, 8, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_8TR_C,
	&TestRWLockLocks<1, 16, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_1TW_16TR_C,

	&TestRWLockLocks<4, 4, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_4TR_C,
	&TestRWLockLocks<4, 8, 0, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_8TR_C,
	&TestRWLockLocks<4, 16, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_16TR_C,
	&TestRWLockLocks<4, 32, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_4TW_32TR_C,

	&TestRWLockLocks<8, 16, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_16TR_C,
	&TestRWLockLocks<8, 32, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_32TR_C,
	&TestRWLockLocks<8, 64, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_64TR_C,
	&TestRWLockLocks<8, 128, LIOPT_MULTIPLE_WRITE_CHANNELS, TRS_WITH_TRYREAD_SUPPORT, LTL_C>, // MGWLF_8TW_128TR_C,
};
MG_STATIC_ASSERT(ARRAY_SIZE(g_afnTRDLRWLockFeatureTestProcedures) == MGWLF__MAX);

/*static */
bool CRWLockTest::RunTRDLImplementationTest(unsigned int &nOutSuccessCount, unsigned int &nOutTestCount)
{
	unsigned int nSuccessCount = 0, nTestCount = 0;

	_mg_atomic_init_uint(&g_uiPriorityAdjustmentErrorReported, 0);

	EMGTESTFEATURELEVEL flLevelToTest = CRWLockTest::RetrieveSelectedFeatureTestLevel();

	int iCaptionWidth = ARRAY_SIZE(g_ascFeatureTestingText) - 1 + g_iTestFeatureNameWidth + ARRAY_SIZE(g_ascPostFeatureSuffix) - 1 + ARRAY_SIZE(g_ascLockTestTableCaption) - 1;
	printf("%*s\n%*s\n", iCaptionWidth, g_ascLockTestTableCaption, iCaptionWidth, g_ascLockTestTableSubCapn);

	for (EMGRWLOCKFEATURE lfRWLockFeature = MGWLF__TESTBEGIN; lfRWLockFeature != MGWLF__TESTEND; ++lfRWLockFeature)
	{
		if (g_aflTRDLRWLockFeatureTestLevels[lfRWLockFeature] <= flLevelToTest)
		{
			++nTestCount;

			const char *szFeatureName = g_aszTRDLRWLockFeatureTestNames[lfRWLockFeature];
			printf("%s%*s%s", g_ascFeatureTestingText, g_iTestFeatureNameWidth, szFeatureName, g_ascPostFeatureSuffix);

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

