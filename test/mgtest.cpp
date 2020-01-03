#include "pch.h"
#include <mutexgear/rwlock.h>
#include <mutexgear/utility.h>

#ifdef _WIN32
#include <process.h>
#endif

#include <stdio.h>


#define _MGTEST_TEST_TRDL		1


#if _MGTEST_TEST_TRDL
#define MUTEXGEAR_RWLOCK_VARIANT_T mutexgear_trdl_rwlock_t
#else 
#define MUTEXGEAR_RWLOCK_VARIANT_T mutexgear_rwlock_t
#endif


//////////////////////////////////////////////////////////////////////////

typedef bool(*CFeatureTestProcedure)();


//////////////////////////////////////////////////////////////////////////

enum ERWLOCKLOCKTESTOBJECT
{
	LTO__MIN,

	LTO_SYSTEM = LTO__MIN,
	LTO_MUTEXGEAR,

	LTO__MAX,
};

static const unsigned g_uiLockIterationCount = 1000 * 1000;
static const unsigned g_uiLockOperationsPerLine = 100;
const unsigned int g_uiSettleDownSleepDelay = 5000;

static const char g_ascLockTestDetailsFileNameFormat[] = "LockTestDet_%zu%s_%zu_%s.txt";

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

static const char *const g_aszTestedObjectKindFileNameSuffixes[LTO__MAX] =
{
	"Sys", // LTO_SYSTEM,
	"MG", // LTO_MUTEXGEAR,
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


template<ERWLOCKLOCKTESTOBJECT ttoTestedObjectKind>
class CRWLockImplementation;

template<>
class CRWLockImplementation<LTO_SYSTEM>
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
		bool bLockedWithTryVariant = false;

		int iLockResult = _mutexgear_rwlock_trywrlock(&m_wlRWLock);
		MG_CHECK(iLockResult, (iLockResult == EOK && (bLockedWithTryVariant = true, true))
			|| (iLockResult == EBUSY && (iLockResult = _mutexgear_rwlock_wrlock(&m_wlRWLock)) == EOK));

		return bLockedWithTryVariant;
	}

	void UnlockRWLockWrite(CLockWriteExtraObjects &eoRefExtraObjects)
	{
		int iUnlockResult;
		MG_CHECK(iUnlockResult, (iUnlockResult = _mutexgear_rwlock_wrunlock(&m_wlRWLock)) == EOK);
	}

	bool LockRWLockRead(CLockReadExtraObjects &eoRefExtraObjects)
	{
		bool bLockedWithTryVariant = false;

		int iLockResult;
#if _MGTEST_TEST_TRDL
		iLockResult = _mutexgear_rwlock_tryrdlock(&m_wlRWLock);
#else
		iLockResult = EBUSY;
#endif
		MG_CHECK(iLockResult, (iLockResult == EOK && (bLockedWithTryVariant = true, true))
			|| (iLockResult == EBUSY && (iLockResult = _mutexgear_rwlock_rdlock(&m_wlRWLock)) == EOK));

		return bLockedWithTryVariant;
	}

	void UnlockRWLockRead(CLockReadExtraObjects &eoRefExtraObjects)
	{
		int iUnlockResult;
		MG_CHECK(iUnlockResult, (iUnlockResult = _mutexgear_rwlock_rdunlock(&m_wlRWLock)) == EOK);
	}

private:
	_MG_RWLOCK_T		m_wlRWLock;
};

template<>
class CRWLockImplementation<LTO_MUTEXGEAR>
{
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
		bool bLockedWithTryVariant = false;

		int iLockResult = mutexgear_rwlock_trywrlock(&m_wlRWLock);
		MG_CHECK(iLockResult, (iLockResult == EOK && (bLockedWithTryVariant = true, true))
			|| (iLockResult == EBUSY && (iLockResult = mutexgear_rwlock_wrlock(&m_wlRWLock, &eoRefExtraObjects.m_cwLockWorker, &eoRefExtraObjects.m_cwLockWaiter, NULL)) == EOK));

		return bLockedWithTryVariant;
	}

	void UnlockRWLockWrite(CLockWriteExtraObjects &eoRefExtraObjects)
	{
		int iUnlockResult;
		MG_CHECK(iUnlockResult, (iUnlockResult = mutexgear_rwlock_wrunlock(&m_wlRWLock)) == EOK);
	}

	bool LockRWLockRead(CLockReadExtraObjects &eoRefExtraObjects)
	{
		bool bLockedWithTryVariant = false;

		int iLockResult;
#if _MGTEST_TEST_TRDL
		iLockResult = mutexgear_rwlock_tryrdlock(&m_wlRWLock, &eoRefExtraObjects.m_eoWriteObjects.m_cwLockWorker, &eoRefExtraObjects.m_ciLockCompletionItem);
#else
		iLockResult = EBUSY;
#endif
		MG_CHECK(iLockResult, (iLockResult == EOK && (bLockedWithTryVariant = true, true))
			|| (iLockResult == EBUSY && (iLockResult = mutexgear_rwlock_rdlock(&m_wlRWLock, &eoRefExtraObjects.m_eoWriteObjects.m_cwLockWorker, &eoRefExtraObjects.m_eoWriteObjects.m_cwLockWaiter, &eoRefExtraObjects.m_ciLockCompletionItem)) == EOK));

		return bLockedWithTryVariant;
	}

	void UnlockRWLockRead(CLockReadExtraObjects &eoRefExtraObjects)
	{
		int iUnlockResult;
		MG_CHECK(iUnlockResult, (iUnlockResult = mutexgear_rwlock_rdunlock(&m_wlRWLock, &eoRefExtraObjects.m_eoWriteObjects.m_cwLockWorker, &eoRefExtraObjects.m_ciLockCompletionItem)) == EOK);
	}

private:
	void InitializeRWLockInstance()
	{
		int iInitResult;
		mutexgear_rwlockattr_t attr;

		MG_CHECK(iInitResult, (iInitResult = mutexgear_rwlockattr_init(&attr)) == EOK);
		// MG_CHECK(iInitResult, (iInitResult = mutexgear_rwlockattr_setwritechannels(&attr, 2)) == EOK);
		MG_CHECK(iInitResult, (iInitResult = mutexgear_rwlock_init(&m_wlRWLock, &attr)) == EOK);
		MG_CHECK(iInitResult, (iInitResult = mutexgear_rwlockattr_destroy(&attr)) == EOK);
	}

	void FinalizeRWLockInstance()
	{
		int iDestroyResult;
		MG_CHECK(iDestroyResult, (iDestroyResult = mutexgear_rwlock_destroy(&m_wlRWLock)) == EOK);
	}

private:
	MUTEXGEAR_RWLOCK_VARIANT_T		m_wlRWLock;
};


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

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor>
class CRWLockLockTestExecutor:
	public CRWLockLockTestBase
{
public:
	explicit CRWLockLockTestExecutor(iterationcntint uiIterationCount) :
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
	void AllocateThreadOperationBuffers(threadcntint ciTotalThreadCount);
	void FreeThreadOperationBuffers(threadcntint ciTotalThreadCount);
	operationidxint GetMaximalThreadOperationCount() const;

	template<ERWLOCKLOCKTESTOBJECT ttoTestedObjectKind>
	void AllocateTestThreads(CRWLockImplementation<ttoTestedObjectKind> &liRefRWLock, 
		threadcntint ciWriterCount, threadcntint ciReraderCount, 
		CThreadExecutionBarrier &sbRefStartBarrier, CThreadExecutionBarrier &sbRefFinishBarrier, CThreadExecutionBarrier &sbRefExitBarrier, 
		CRWLockLockTestProgress &tpRefProgressInstance);
	void FreeTestThreads(CThreadExecutionBarrier &sbRefExitBarrier, threadcntint ciTotalThreadCount);

	void WaitTestThreadsReady(CThreadExecutionBarrier &sbStartBarrier);
	void LaunchTheTest(CThreadExecutionBarrier &sbStartBarrier);
	void WaitTheTestEnd(CThreadExecutionBarrier &sbFinishBarrier);

	void InitializeTestResults(threadcntint ciWriterCount, threadcntint ciReraderCount);
	void PublishTestResults(ERWLOCKLOCKTESTOBJECT toTestedObjectKind, threadcntint ciWriterCount, threadcntint ciReraderCount, CTimeUtils::timeduration tdTestDuration);
	void BuildMergedThreadOperationMap(char ascMergeBuffer[], operationidxint iiLastSavedOperationIndex, operationidxint iiThreadOperationCount,
		operationidxint aiiThreadLastOperationIndices[], threadcntint ciWriterCount, threadcntint ciReaderCount);
	operationidxint FillThreadOperationMap(char ascMergeBuffer[], operationidxint iiLastSavedOperationIndex, const operationidxint iiThreadOperationCount,
		const operationidxint *piiThreadOperationIndices, operationidxint iiThreadLastOperationIndex, const char ascThreadMapChars[LOCK_THREAD_NAME_LENGTH]);
	void InitializeThreadName(char ascThreadMapChars[LOCK_THREAD_NAME_LENGTH + 1], char cMapFirstChar);
	void IncrementThreadName(char ascThreadMapChars[LOCK_THREAD_NAME_LENGTH + 1], threadcntint ciThreadIndex);
	void SaveThreadOperationMapToDetails(ERWLOCKLOCKTESTOBJECT toTestedObjectKind, const char ascMergeBuffer[], operationidxint iiThreadOperationCount);
	void FinalizeTestResults();

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
class CRWLockLockTestThread:
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

class CRWLockLockExecutorLogic
{
public:
	typedef CRWLockLockTestBase::operationidxint operationidxint;

	static unsigned GetOperationKindCount() { return LOK__MAX; }
	static EEXECUTORLOCKOPERATIONKIND GetOperationIndexOperationKind(operationidxint iiOperationIndex) { return (EEXECUTORLOCKOPERATIONKIND)(iiOperationIndex % LOK__MAX); }
};

template<ERWLOCKLOCKTESTOBJECT ttoTestedObjectKind>
class CRWLockWriteLockExecutor
{
public:
	struct CConstructionParameter
	{
		CConstructionParameter(CRWLockImplementation<ttoTestedObjectKind> &liRWLockInstance) : m_liRWLockInstance(liRWLockInstance) {}

		CRWLockImplementation<ttoTestedObjectKind>	&m_liRWLockInstance;
	};

	CRWLockWriteLockExecutor(const CConstructionParameter &cpConstructionParameter) : m_liRWLockInstance(cpConstructionParameter.m_liRWLockInstance) {}

	typedef typename CRWLockImplementation<ttoTestedObjectKind>::CLockWriteExtraObjects COperationExtraObjects;
	typedef CRWLockLockTestBase::iterationcntint iterationcntint;
	typedef CRWLockLockTestBase::operationidxint operationidxint;

	static operationidxint GetThreadOperationCount(iterationcntint uiIterationCount) { return (operationidxint)uiIterationCount * LOK__MAX; }

	bool ApplyLock(COperationExtraObjects &eoRefExtraObjects)
	{
		return m_liRWLockInstance.LockRWLockWrite(eoRefExtraObjects);
	}

	void ReleaseLock(COperationExtraObjects &eoRefExtraObjects)
	{
		m_liRWLockInstance.UnlockRWLockWrite(eoRefExtraObjects);
	}

private:
	CRWLockImplementation<ttoTestedObjectKind>	&m_liRWLockInstance;
};

template<ERWLOCKLOCKTESTOBJECT ttoTestedObjectKind, unsigned int tuiReaderWriteDivisor>
class CRWLockReadLockExecutor
{
public:
	struct CConstructionParameter
	{
		CConstructionParameter(CRWLockImplementation<ttoTestedObjectKind> &liRWLockInstance) : m_liRWLockInstance(liRWLockInstance) {}

		CRWLockImplementation<ttoTestedObjectKind>	&m_liRWLockInstance;
	};

	CRWLockReadLockExecutor(const CConstructionParameter &cpConstructionParameter) : m_liRWLockInstance(cpConstructionParameter.m_liRWLockInstance), m_ciLockIndex(0) {}

	typedef typename CRWLockImplementation<ttoTestedObjectKind>::CLockReadExtraObjects COperationExtraObjects;
	typedef CRWLockLockTestBase::iterationcntint iterationcntint;
	typedef CRWLockLockTestBase::operationidxint operationidxint;

	static operationidxint GetThreadOperationCount(iterationcntint uiIterationCount) { return (operationidxint)uiIterationCount * LOK__MAX; }

	bool ApplyLock(COperationExtraObjects &eoRefExtraObjects)
	{
		bool bLockedWithTryVariant;

		if (tuiReaderWriteDivisor == 0 || ++m_ciLockIndex % tuiReaderWriteDivisor != 0)
		{
			bLockedWithTryVariant = m_liRWLockInstance.LockRWLockRead(eoRefExtraObjects);
		}
		else
		{
			bLockedWithTryVariant = m_liRWLockInstance.LockRWLockWrite(eoRefExtraObjects);
		}

		return bLockedWithTryVariant;
	}

	void ReleaseLock(COperationExtraObjects &eoRefExtraObjects)
	{
		if (tuiReaderWriteDivisor == 0 || m_ciLockIndex % tuiReaderWriteDivisor != 0)
		{
			m_liRWLockInstance.UnlockRWLockRead(eoRefExtraObjects);
		}
		else
		{
			m_liRWLockInstance.UnlockRWLockWrite(eoRefExtraObjects);
		}
	}

private:
	CRWLockImplementation<ttoTestedObjectKind>	&m_liRWLockInstance;
	iterationcntint				m_ciLockIndex;
};


//////////////////////////////////////////////////////////////////////////

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor>
CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::~CRWLockLockTestExecutor()
{
	MG_ASSERT(std::find_if(m_aittTestThreads, m_aittTestThreads + ARRAY_SIZE(m_aittTestThreads), pred_not_zero<IRWLockLockTestThread *>) == m_aittTestThreads + ARRAY_SIZE(m_aittTestThreads));
	MG_ASSERT(std::find_if(m_apiiThreadOperationIndices, m_apiiThreadOperationIndices + ARRAY_SIZE(m_apiiThreadOperationIndices), pred_not_zero<operationidxint *>) == m_apiiThreadOperationIndices + ARRAY_SIZE(m_apiiThreadOperationIndices));
	MG_ASSERT(m_pscDetailsMergeBuffer == NULL);
	MG_ASSERT(std::find_if(m_apsfOperationDetailFiles, m_apsfOperationDetailFiles + ARRAY_SIZE(m_apsfOperationDetailFiles), pred_not_zero<FILE *>) == m_apsfOperationDetailFiles + ARRAY_SIZE(m_apsfOperationDetailFiles));
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor>
bool CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::RunTheTask()
{
	InitializeTestResults(LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT);
	AllocateThreadOperationBuffers(LOCKTEST_THREAD_COUNT);

	CThreadExecutionBarrier sbStartBarrier(LOCKTEST_THREAD_COUNT), sbFinishBarrier(LOCKTEST_THREAD_COUNT), sbExitBarrier(0);
	CRWLockLockTestProgress tpProgressInstance;
	CTimeUtils::timeduration atdObjectTestDurations[LTO__MAX];

	{
		const ERWLOCKLOCKTESTOBJECT toTestedObjectKind = LTO_SYSTEM;

		CRWLockImplementation<toTestedObjectKind> liRWLock;
		AllocateTestThreads<toTestedObjectKind>(liRWLock, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, sbStartBarrier, sbFinishBarrier, sbExitBarrier, tpProgressInstance);

		WaitTestThreadsReady(sbStartBarrier);

		CTimeUtils::timepoint tpTestStartTime = CTimeUtils::GetCurrentMonotonicTimeNano();

		LaunchTheTest(sbStartBarrier);
		WaitTheTestEnd(sbFinishBarrier);

		CTimeUtils::timepoint tpTestEndTime = CTimeUtils::GetCurrentMonotonicTimeNano();

		CTimeUtils::timeduration tdTestDuration = atdObjectTestDurations[toTestedObjectKind] = tpTestEndTime - tpTestStartTime;
		PublishTestResults(toTestedObjectKind, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, tdTestDuration);

		FreeTestThreads(sbExitBarrier, LOCKTEST_THREAD_COUNT);

		sbStartBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
		sbFinishBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
		sbExitBarrier.ResetInstance(0);
		tpProgressInstance.ResetInstance();
	}

	{
		const ERWLOCKLOCKTESTOBJECT toTestedObjectKind = LTO_MUTEXGEAR;

		CRWLockImplementation<toTestedObjectKind> liRWLock;
		AllocateTestThreads<toTestedObjectKind>(liRWLock, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, sbStartBarrier, sbFinishBarrier, sbExitBarrier, tpProgressInstance);

		WaitTestThreadsReady(sbStartBarrier);

		CTimeUtils::timepoint tpTestStartTime = CTimeUtils::GetCurrentMonotonicTimeNano();

		LaunchTheTest(sbStartBarrier);
		WaitTheTestEnd(sbFinishBarrier);

		CTimeUtils::timepoint tpTestEndTime = CTimeUtils::GetCurrentMonotonicTimeNano();

		CTimeUtils::timeduration tdTestDuration = atdObjectTestDurations[toTestedObjectKind] = tpTestEndTime - tpTestStartTime;
		PublishTestResults(toTestedObjectKind, LOCKTEST_WRITER_COUNT, LOCKTEST_READER_COUNT, tdTestDuration);

		FreeTestThreads(sbExitBarrier, LOCKTEST_THREAD_COUNT);

		// sbStartBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
		// sbFinishBarrier.ResetInstance(LOCKTEST_THREAD_COUNT);
		// sbExitBarrier.ResetInstance(0);
		// tpProgressInstance.ResetInstance();
	}
	MG_STATIC_ASSERT(LTO__MAX == 2);

	printf("(Sys/MG=%2lu.%.03lu/%2lu.%.03lu): ", 
		(unsigned long)(atdObjectTestDurations[LTO_SYSTEM] / 1000000000), (unsigned long)(atdObjectTestDurations[LTO_SYSTEM] % 1000000000) / 1000000,
		(unsigned long)(atdObjectTestDurations[LTO_MUTEXGEAR] / 1000000000), (unsigned long)(atdObjectTestDurations[LTO_MUTEXGEAR] % 1000000000) / 1000000);
	MG_STATIC_ASSERT(LTO__MAX == 2);

	
	FreeThreadOperationBuffers(LOCKTEST_THREAD_COUNT);
	FinalizeTestResults();

	return true;
}


template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::AllocateThreadOperationBuffers(threadcntint ciTotalThreadCount)
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

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::FreeThreadOperationBuffers(threadcntint ciTotalThreadCount)
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

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor>
typename CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::operationidxint CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::GetMaximalThreadOperationCount() const
{
	operationidxint nThreadOperationCount;

	const iterationcntint uiIterationCount = m_uiIterationCount;
	const operationidxint iiWriteOperationCount = CRWLockWriteLockExecutor<LTO__MIN>::GetThreadOperationCount(uiIterationCount), iiReadOperationCount = CRWLockReadLockExecutor<LTO__MIN, 0>::GetThreadOperationCount(uiIterationCount);
	nThreadOperationCount = max(iiWriteOperationCount, iiReadOperationCount);

	MG_STATIC_ASSERT(LOCKTEST_THREAD_COUNT == (threadcntint)(LOCKTEST_WRITER_COUNT + LOCKTEST_READER_COUNT));

	return nThreadOperationCount;
}


template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor>
template<ERWLOCKLOCKTESTOBJECT ttoTestedObjectKind>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::AllocateTestThreads(CRWLockImplementation<ttoTestedObjectKind> &liRefRWLock,
	threadcntint ciWriterCount, threadcntint ciReraderCount,
	CThreadExecutionBarrier &sbRefStartBarrier, CThreadExecutionBarrier &sbRefFinishBarrier, CThreadExecutionBarrier &sbRefExitBarrier, 
	CRWLockLockTestProgress &tpRefProgressInstance)
{
	const iterationcntint uiIterationCount = m_uiIterationCount;

	threadcntint uiThreadIndex = 0;

	typename CRWLockWriteLockExecutor<ttoTestedObjectKind>::CConstructionParameter cpWriteExecutorParameter(liRefRWLock);
	operationidxint nThreadOperationCount = (operationidxint)uiIterationCount * LOK__MAX;

	const threadcntint ciWritersEndIndex = uiThreadIndex + ciWriterCount;
	for (; uiThreadIndex != ciWritersEndIndex; ++uiThreadIndex)
	{
		MG_ASSERT(m_aittTestThreads[uiThreadIndex] == NULL);

		typedef CRWLockLockTestThread<CRWLockWriteLockExecutor<ttoTestedObjectKind> > CUsedRWLockLockTestThread;
		typedef typename CUsedRWLockLockTestThread::CThreadParameterInfo CUsedThreadParameterInfo;
		CUsedRWLockLockTestThread *pttThreadInstance = new CUsedRWLockLockTestThread(CUsedThreadParameterInfo(sbRefStartBarrier, sbRefFinishBarrier, sbRefExitBarrier, tpRefProgressInstance), uiIterationCount, cpWriteExecutorParameter, m_apiiThreadOperationIndices[uiThreadIndex]);
		m_aittTestThreads[uiThreadIndex] = pttThreadInstance;
	}

	typename CRWLockReadLockExecutor<ttoTestedObjectKind, tuiReaderWriteDivisor>::CConstructionParameter cpReadExecutorParameter(liRefRWLock);

	const threadcntint ciReadersEndIndex = uiThreadIndex + ciReraderCount;
	for (; uiThreadIndex != ciReadersEndIndex; ++uiThreadIndex)
	{
		MG_ASSERT(m_aittTestThreads[uiThreadIndex] == NULL);

		typedef CRWLockLockTestThread<CRWLockReadLockExecutor<ttoTestedObjectKind, tuiReaderWriteDivisor> > CUsedRWLockLockTestThread;
		typedef typename CUsedRWLockLockTestThread::CThreadParameterInfo CUsedThreadParameterInfo;
		CUsedRWLockLockTestThread *pttThreadInstance = new CUsedRWLockLockTestThread(CUsedThreadParameterInfo(sbRefStartBarrier, sbRefFinishBarrier, sbRefExitBarrier, tpRefProgressInstance), uiIterationCount, cpReadExecutorParameter, m_apiiThreadOperationIndices[uiThreadIndex]);
		m_aittTestThreads[uiThreadIndex] = pttThreadInstance;
	}
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::FreeTestThreads(CThreadExecutionBarrier &sbRefExitBarrier, threadcntint ciTotalThreadCount)
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


template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::WaitTestThreadsReady(CThreadExecutionBarrier &sbStartBarrier)
{
	while (sbStartBarrier.RetrieveCounterValue() != 0)
	{
		CTimeUtils::Sleep(1);
	}

	// Sleep a longer period for all the threads to enter blocked state 
	// and the system to finish any disk cache flush activity from previous tests
	CTimeUtils::Sleep(g_uiSettleDownSleepDelay);
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::LaunchTheTest(CThreadExecutionBarrier &sbStartBarrier)
{
	sbStartBarrier.SignalReleaseEvent();
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::WaitTheTestEnd(CThreadExecutionBarrier &sbFinishBarrier)
{
	sbFinishBarrier.WaitReleaseEvent();
}


template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::InitializeTestResults(threadcntint ciWriterCount, threadcntint ciReraderCount)
{
	int iFileOpenStatus;
	char ascFileNameFormatBuffer[256];

	const unsigned uiReaderWriteDivisor = tuiReaderWriteDivisor;
	const unsigned uiWritesPercent = uiReaderWriteDivisor != 0 ? 100 / uiReaderWriteDivisor : 0;

	for (ERWLOCKLOCKTESTOBJECT toTestedObjectKind = LTO__MIN; toTestedObjectKind != LTO__MAX; ++toTestedObjectKind)
	{
		MG_ASSERT(m_apsfOperationDetailFiles[toTestedObjectKind] == NULL);

		const char *szObjectKindFileNameSuffix = g_aszTestedObjectKindFileNameSuffixes[toTestedObjectKind];
		int iPrintResult = snprintf(ascFileNameFormatBuffer, sizeof(ascFileNameFormatBuffer), g_ascLockTestDetailsFileNameFormat, 
			tuiReaderWriteDivisor == 0 ? (size_t)ciWriterCount : (size_t)uiWritesPercent, tuiReaderWriteDivisor == 0 ? "" : "%", (size_t)ciReraderCount, szObjectKindFileNameSuffix);
		MG_CHECK(iPrintResult, IN_RANGE(iPrintResult - 1, 0, ARRAY_SIZE(ascFileNameFormatBuffer) - 1) || (iPrintResult < 0 && (iPrintResult = -errno, false)));

		FILE *psfObjectKindDetailsFile = fopen(ascFileNameFormatBuffer, "w+");
		MG_CHECK(iFileOpenStatus, psfObjectKindDetailsFile != NULL || (iFileOpenStatus = errno, false));

		m_apsfOperationDetailFiles[toTestedObjectKind] = psfObjectKindDetailsFile;
	}
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::PublishTestResults(ERWLOCKLOCKTESTOBJECT toTestedObjectKind,
	threadcntint ciWriterCount, threadcntint ciReaderCount, CTimeUtils::timeduration tdTestDuration)
{
	MG_ASSERT(IN_RANGE(toTestedObjectKind, LTO__MIN, LTO__MAX));

	const unsigned uiReaderWriteDivisor = tuiReaderWriteDivisor;
	const unsigned uiWritesPercent = uiReaderWriteDivisor != 0 ? 100 / uiReaderWriteDivisor : 0;

	FILE *psfDetailsFile = m_apsfOperationDetailFiles[toTestedObjectKind];
	int iPrintResult = fprintf(psfDetailsFile, "Lock-unlock test with %zu%s write%s%s, %zu %s%s\nTime taken: %lu sec %lu nsec\n\nOperation sequence map:\n",
		tuiReaderWriteDivisor == 0 ? (size_t)ciWriterCount : (size_t)uiWritesPercent, tuiReaderWriteDivisor == 0 ? "" : "%",
		tuiReaderWriteDivisor == 0 ? "r" : "s", tuiReaderWriteDivisor == 0 && ciWriterCount != 1 ? "s" : "", (size_t)ciReaderCount, 
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

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::BuildMergedThreadOperationMap(char ascMergeBuffer[], operationidxint iiLastSavedOperationIndex, operationidxint iiThreadOperationCount,
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

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor>
typename CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::operationidxint CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::FillThreadOperationMap(
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

		const unsigned uiReaderWriteDivisor = tuiReaderWriteDivisor;
		if (uiReaderWriteDivisor != 0 && ascThreadMapChars[0] >= g_cReaderMapFirstChar)
		{
			operationidxint iiThreadReducedOperationIndex = iiThreadCurrentOperationIndex / uiPerIndexOperationKindCount;
			
			if ((iiThreadReducedOperationIndex + 1) % uiReaderWriteDivisor == 0)
			{
				ascMergeBuffer[siNameInsertOffset] -= g_cReaderMapFirstChar - g_cWriterMapFirstChar;
			}
		}

		ascMergeBuffer[siNameInsertOffset] += g_ascLockOperationKindThreadNameFirstCharModifiers[okOperationKind];
	}

	return iiThreadCurrentOperationIndex;
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::InitializeThreadName(char ascThreadMapChars[LOCK_THREAD_NAME_LENGTH + 1], char cMapFirstChar)
{
	ascThreadMapChars[0] = cMapFirstChar;
	int iPrintResult = sprintf(ascThreadMapChars + 1, "%0*zx", LOCK_THREAD_NAME_LENGTH - 1, (size_t)0);
	MG_CHECK(iPrintResult, iPrintResult == LOCK_THREAD_NAME_LENGTH - 1 || (iPrintResult < 0 && (iPrintResult = -errno, false)));
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::IncrementThreadName(char ascThreadMapChars[LOCK_THREAD_NAME_LENGTH + 1], threadcntint ciThreadIndex)
{
	threadcntint ciThreadIndexNumericPart = ciThreadIndex % ((threadcntint)1 << (LOCK_THREAD_NAME_LENGTH - 1) * 4); // 4 bits per a hexadecimal digit
	if (ciThreadIndexNumericPart == 0)
	{
		++ascThreadMapChars[0];
	}

	int iPrintResult = sprintf(ascThreadMapChars + 1, "%0*zx", LOCK_THREAD_NAME_LENGTH - 1, (size_t)ciThreadIndexNumericPart);
	MG_CHECK(iPrintResult, iPrintResult == LOCK_THREAD_NAME_LENGTH - 1 || (iPrintResult < 0 && (iPrintResult = -errno, false)));
}

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::SaveThreadOperationMapToDetails(ERWLOCKLOCKTESTOBJECT toTestedObjectKind,
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

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount, unsigned int tuiReaderWriteDivisor>
void CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, tuiReaderWriteDivisor>::FinalizeTestResults()
{
	for (ERWLOCKLOCKTESTOBJECT toTestedObjectKind = LTO__MIN; toTestedObjectKind != LTO__MAX; ++toTestedObjectKind)
	{
		FILE *psfObjectKindDetailsFile = m_apsfOperationDetailFiles[toTestedObjectKind];

		if (psfObjectKindDetailsFile != NULL)
		{
			m_apsfOperationDetailFiles[toTestedObjectKind] = NULL;

			fclose(psfObjectKindDetailsFile);
		}
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
	pttThreadInstance->ExecuteThread();

	return 0;
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
		MG_ASSERT(LOK__MAX == 2);
	}
}


template<class TOperationExecutor>
/*virtual */
void CRWLockLockTestThread<TOperationExecutor>::ReleaseInstance()
{
	delete this;
}


//////////////////////////////////////////////////////////////////////////
// The Tests

template<unsigned int tuiWriterCount, unsigned int tuiReaderCount>
bool TestRWLockLocks()
{
	CRWLockLockTestExecutor<tuiWriterCount, tuiReaderCount, 0> ltTestInstance(g_uiLockIterationCount);
	return ltTestInstance.RunTheTask();
}

template<unsigned int tuiThreadCount, unsigned int tuiWriteDivivisor>
bool TestRWLockMixed()
{
	CRWLockLockTestExecutor<0, tuiThreadCount, tuiWriteDivivisor> ltTestInstance(g_uiLockIterationCount);
	return ltTestInstance.RunTheTask();
}


enum EMGRWLOCKFEATURE
{
	MGWLF__MIN,

	MGWLF_1TW_ND = MGWLF__MIN,
	MGWLF_4TW_ND,
	MGWLF_8TW_ND,
	MGWLF_16TW_ND,

	MGWLF_1TR_ND,
	MGWLF_4TR_ND,
	MGWLF_8TR_ND,
	MGWLF_16TR_ND,

	MGWLF_8T_12PW_ND,
	MGWLF_16T_12PW_ND,
	MGWLF_32T_12PW_ND,
	MGWLF_64T_12PW_ND,

	MGWLF_8T_25PW_ND,
	MGWLF_16T_25PW_ND,
	MGWLF_32T_25PW_ND,
	MGWLF_64T_25PW_ND,

	MGWLF_8T_50PW_ND,
	MGWLF_16T_50PW_ND,
	MGWLF_32T_50PW_ND,
	MGWLF_64T_50PW_ND,

	MGWLF_1TW_1TR_ND,
	MGWLF_1TW_4TR_ND,
	MGWLF_1TW_8TR_ND,
	MGWLF_1TW_16TR_ND,

	MGWLF_4TW_4TR_ND,
	MGWLF_4TW_8TR_ND,
	MGWLF_4TW_16TR_ND,
	MGWLF_4TW_32TR_ND,

	MGWLF_8TW_8TR_ND,
	MGWLF_8TW_16TR_ND,
	MGWLF_8TW_32TR_ND,
	MGWLF_8TW_64TR_ND,

	MGWLF_16TW_16TR_ND,
	MGWLF_16TW_32TR_ND,
	MGWLF_16TW_64TR_ND,
	MGWLF_16TW_128TR_ND,

	MGWLF__MAX,

	MGWLF__TESTBEGIN = MGWLF__MIN,
	MGWLF__TESTEND = MGWLF__MAX,
	MGWLF__TESTCOUNT = MGWLF__TESTEND - MGWLF__TESTBEGIN,
};
MG_STATIC_ASSERT(MGWLF__TESTBEGIN <= MGWLF__TESTEND);

static const CFeatureTestProcedure g_afnRWLockFeatureTestProcedures[MGWLF__MAX] =
{
	&TestRWLockLocks<1, 0>, // MGWLF_1TW_ND,
	&TestRWLockLocks<4, 0>, // MGWLF_4TW_ND,
	&TestRWLockLocks<8, 0>, // MGWLF_8TW_ND,
	&TestRWLockLocks<16, 0>, // MGWLF_16TW_ND,

	&TestRWLockLocks<0, 1>, // MGWLF_1TR_ND,
	&TestRWLockLocks<0, 4>, // MGWLF_4TR_ND,
	&TestRWLockLocks<0, 8>, // MGWLF_8TR_ND,
	&TestRWLockLocks<0, 16>, // MGWLF_16TR_ND,

	&TestRWLockMixed<8, 8>, // MGWLF_8T_12PW_ND,
	&TestRWLockMixed<16, 8>, // MGWLF_16T_12PW_ND,
	&TestRWLockMixed<32, 8>, // MGWLF_32T_12PW_ND,
	&TestRWLockMixed<64, 8>, // MGWLF_64T_12PW_ND,

	&TestRWLockMixed<8, 4>, // MGWLF_8T_25PW_ND,
	&TestRWLockMixed<16, 4>, // MGWLF_16T_25PW_ND,
	&TestRWLockMixed<32, 4>, // MGWLF_32T_25PW_ND,
	&TestRWLockMixed<64, 4>, // MGWLF_64T_25PW_ND,

	&TestRWLockMixed<8, 2>, // MGWLF_8T_50PW_ND,
	&TestRWLockMixed<16, 2>, // MGWLF_16T_50PW_ND,
	&TestRWLockMixed<32, 2>, // MGWLF_32T_50PW_ND,
	&TestRWLockMixed<64, 2>, // MGWLF_64T_50PW_ND,

	&TestRWLockLocks<1, 1>, // MGWLF_1TW_1TR_ND,
	&TestRWLockLocks<1, 4>, // MGWLF_1TW_4TR_ND,
	&TestRWLockLocks<1, 8>, // MGWLF_1TW_8TR_ND,
	&TestRWLockLocks<1, 16>, // MGWLF_1TW_16TR_ND,

	&TestRWLockLocks<4, 4>, // MGWLF_4TW_4TR_ND,
	&TestRWLockLocks<4, 8>, // MGWLF_4TW_8TR_ND,
	&TestRWLockLocks<4, 16>, // MGWLF_4TW_16TR_ND,
	&TestRWLockLocks<4, 32>, // MGWLF_4TW_32TR_ND,

	&TestRWLockLocks<8, 8>, // MGWLF_8TW_8TR_ND,
	&TestRWLockLocks<8, 16>, // MGWLF_8TW_16TR_ND,
	&TestRWLockLocks<8, 32>, // MGWLF_8TW_32TR_ND,
	&TestRWLockLocks<8, 64>, // MGWLF_8TW_64TR_ND,

	&TestRWLockLocks<16, 16>, // MGWLF_16TW_16TR_ND,
	&TestRWLockLocks<16, 32>, // MGWLF_16TW_32TR_ND,
	&TestRWLockLocks<16, 64>, // MGWLF_16TW_64TR_ND,
	&TestRWLockLocks<16, 128>, // MGWLF_16TW_128TR_ND,
};

static const char *const g_aszRWLockFeatureTestNames[MGWLF__MAX] =
{
	"1 Writer", // MGWLF_1TW_ND,
	"4 Writers", // MGWLF_4TW_ND,
	"8 Writers", // MGWLF_8TW_ND,
	"16 Writers", // MGWLF_16TW_ND,

	"1 Reader", // MGWLF_1TR_ND,
	"4 Readers", // MGWLF_4TR_ND,
	"8 Readers", // MGWLF_8TR_ND,
	"16 Readers", // MGWLF_16TR_ND,

	"12% writes, 8 threads", // MGWLF_8T_12PW_ND,
	"12% writes, 16 threads", // MGWLF_16T_12PW_ND,
	"12% writes, 32 threads", // MGWLF_32T_12PW_ND,
	"12% writes, 64 threads", // MGWLF_64T_12PW_ND,

	"25% writes, 8 threads", // MGWLF_8T_25PW_ND,
	"25% writes, 16 threads", // MGWLF_16T_25PW_ND,
	"25% writes, 32 threads", // MGWLF_32T_25PW_ND,
	"25% writes, 64 threads", // MGWLF_64T_25PW_ND,

	"50% writes, 8 threads",  // MGWLF_8T_50PW_ND,
	"50% writes, 16 threads", // MGWLF_16T_50PW_ND,
	"50% writes, 32 threads", // MGWLF_32T_50PW_ND,
	"50% writes, 64 threads", // MGWLF_64T_50PW_ND,

	"1 Writer, 1 Reader", // MGWLF_1TW_1TR_ND,
	"1 Writer, 4 Readers", // MGWLF_1TW_4TR_ND,
	"1 Writer, 8 Readers", // MGWLF_1TW_8TR_ND,
	"1 Writer, 16 Readers", // MGWLF_1TW_16TR_ND,

	"4 Writers, 4 Readers", // MGWLF_4TW_4TR_ND,
	"4 Writers, 8 Readers", // MGWLF_4TW_8TR_ND,
	"4 Writers, 16 Readers", // MGWLF_4TW_16TR_ND,
	"4 Writers, 32 Readers", // MGWLF_4TW_32TR_ND,

	"8 Writers, 8 Readers", // MGWLF_8TW_8TR_ND,
	"8 Writers, 16 Readers", // MGWLF_8TW_16TR_ND,
	"8 Writers, 32 Readers", // MGWLF_8TW_32TR_ND,
	"8 Writers, 64 Readers", // MGWLF_8TW_64TR_ND,

	"16 Writers, 16 Readers", // MGWLF_16TW_16TR_ND,
	"16 Writers, 32 Readers", // MGWLF_16TW_32TR_ND,
	"16 Writers, 64 Readers", // MGWLF_16TW_64TR_ND,
	"16 Writers, 128 Readers", // MGWLF_16TW_128TR_ND,
};


static 
bool TestRWLock(unsigned int &nOutSuccessCount, unsigned int &nOutTestCount)
{
	unsigned int nSuccessCount = 0;

	for (EMGRWLOCKFEATURE lfRWLockFeature = MGWLF__TESTBEGIN; lfRWLockFeature != MGWLF__TESTEND; ++lfRWLockFeature)
	{
		const char *szFeatureName = g_aszRWLockFeatureTestNames[lfRWLockFeature];
		printf("Testing %29s: ", szFeatureName);

		CFeatureTestProcedure fnTestProcedure = g_afnRWLockFeatureTestProcedures[lfRWLockFeature];
		bool bTestResult = fnTestProcedure();
		printf("%s\n", bTestResult ? "success" : "failure");

		if (bTestResult)
		{
			nSuccessCount += 1;
		}
	}

	nOutSuccessCount = nSuccessCount;
	nOutTestCount = MGWLF__TESTCOUNT;
	return nSuccessCount == MGWLF__TESTCOUNT;
}



enum EMUTEXGEARSUBSYSTEMTEST
{
	MGST__MIN,

	MGST_RWLOCK = MGST__MIN,

	MGST__MAX,
};

typedef bool (*CMGSubsystemTestProcedure)(unsigned int &nOutSuccessCount, unsigned int &nOutTestCount);


static const CMGSubsystemTestProcedure g_afnMGSubsystemTestProcedures[MGST__MAX] =
{
	&TestRWLock, // MGST_RWLOCK,
};

static const char *const g_aszMGSubsystemNames[MGST__MAX] =
{
	"RWLock", // MGST_RWLOCK,
};

static 
bool PerformMGCoverageTests(unsigned int &nOutFailureCount)
{
	unsigned int nSuccessCount = 0;

	for (EMUTEXGEARSUBSYSTEMTEST stSubsystemTest = MGST__MIN; stSubsystemTest != MGST__MAX; ++stSubsystemTest)
	{
		const char *szSubsystemName = g_aszMGSubsystemNames[stSubsystemTest];
		printf("\nTesting subsystem \"%s\"\n", szSubsystemName);
		printf("----------------------------------------------\n");

		unsigned int nSubsysytemSuccessCount = 0, nSubsystemTestCount = 1;

		CMGSubsystemTestProcedure fnTestProcedure = g_afnMGSubsystemTestProcedures[stSubsystemTest];
		if (fnTestProcedure(nSubsysytemSuccessCount, nSubsystemTestCount) && nSubsysytemSuccessCount == nSubsystemTestCount)
		{
			nSuccessCount += 1;
		}

		unsigned int nSubsysytemFailureCount = nSubsystemTestCount - nSubsysytemSuccessCount;
		printf("----------------------------------------------\n");
		printf("Feature tests failed:           %3u out of %3u\n", nSubsysytemFailureCount, nSubsystemTestCount);
	}

	unsigned int nFailureCount = MGST__MAX - nSuccessCount;

	printf("\n==============================================\n");
	printf("Subsystem tests failed:         %3u out of %3u\n", nFailureCount, (unsigned int)MGST__MAX);

	nOutFailureCount = nFailureCount;
	return nSuccessCount == MGST__MAX;
}




int main()
{
	unsigned int nFailureCount;
	PerformMGCoverageTests(nFailureCount);

	return nFailureCount;
}

