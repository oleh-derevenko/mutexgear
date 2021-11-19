/************************************************************************/
/* The MutexGear Library                                                */
/* The Library Completion Queue Implementation Test File                */
/*                                                                      */
/* Copyright (c) 2016-2021 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/

/**
 *	\file
 *	\brief Completion Queue Test and Demonstration
 *
 *	The demo implements a producer-consumer queue where numbers of producer and
 *	consumer threads are started, let work for some time, and the shut down.
 *	The producers generate abstract "work items" identified with increasing natural numbers
 *	and the consumers calculate numeric sum of the processed items.
 *
 *	Meanwhile, the main thread, having let the threads to work for some time, shuts the producers down
 *	and starts discarding work items still in the queue while consumers continue to handle then on their own.
 *	Whenever the main item encounters a work item being busy being handled by a consumer, the thread uses
 *	the queue features to wait for the item handling to be completed by the consumer thread and that waiting
 *	is performed with muteces only. This demonstrates how work items can be dequeued prior to being handled or 
 *	be waited for the handling completion if they happen to be already started.
 *
 *	In the case of "cancelable queue", each fourth work item is, on generation, additionally linked 
 *	into an embedded list (this being a @c dlps_list usage demo) and the main thread, having stopped the producers,
 *	performs a pass through this list canceling the selected work items. If an item has not been started being handled 
 *	by a consumer thread yet the item is just dequeued and deleted. If the item handling is already in progress 
 *	the main thread uses the queue features to request the engaged consumer thread abort its work on the item 
 *	and to wait for the item handling to be either aborted or completed. The waiting for the abort or completion is 
 *	performed with muteces only and this demonstrates how work items can be dequeued prior to being handled or 
 *	be requested an abort and be waited for the abort or handling completion (on the consumer thread's discretion) 
 *	if they happen to be already started.
 *
 *	The equality of total of individual consumed, aborted, canceled and dropped sums with the total of generated 
 *	numbers is considered the test success criterion. The test statistics is output on levels of "basic" and higher.
 *
 *	Since it is impractical to implement such complex producer-consumer cooperation and work queue operations in C language 
 *	the demo was implemented via the library's C++ wrappers and requires C++ 11. If no C++ 11 support is detected 
 *	the completion queue tests are skipped.
 *
 *	\note Even though the thread counts and queue sizes can be easily customized by changing constants 
 *	the implementation was not tested to flawlessly handle all the corner cases. The main purpose of the test is 
 *	not to implement a perfect producer-consumer system but rather to demonstrate the completion queue features.
 */

#include "pch.h"
#include "cqtest.h"


#if _MGTEST_HAVE_CXX11

#include <mutexgear/completion.hpp>
#include <mutexgear/parent_wrapper.hpp>
#include <mutexgear/dlps_list.hpp>
#include <list>
#include <array>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <type_traits>


using mg::completion::worker;
using mg::completion::worker_view;
using mg::completion::waiter;
using mg::completion::item;
using mg::completion::item_view;
using mg::completion::waitable_queue;
using mg::completion::cancelable_queue;
using mg::completion::queue_lock_helper;
using mg::completion::acquire_tocken_t;
using mg::completion::queue_work_helper;
using mg::parent_wrapper;
using mg::dlps_info;
using mg::dlps_list;
using std::unique_ptr;
using std::list;
using std::array;
using std::thread;
using std::mutex;
using std::lock_guard;
using std::unique_lock;
using std::condition_variable;
using std::cv_status;
using std::atomic;
using std::atomic_bool;
using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::this_thread::sleep_for;

#endif // #if _MGTEST_HAVE_CXX11



//////////////////////////////////////////////////////////////////////////
// Worker Queues

enum EMGCOMPLETIONQUEUESFEATURE
{
	MGCQF__MIN,

	MGCQF_WAITABLE_QUEUE = MGCQF__MIN,
	MGCQF_CANCELABLE_QUEUE,

	MGCQF__MAX,

	MGCQF__TESTBEGIN = MGCQF__MIN,
	MGCQF__TESTEND = MGCQF__MAX,
	MGCQF__TESTCOUNT = MGCQF__TESTEND - MGCQF__TESTBEGIN,
};
MG_STATIC_ASSERT(MGCQF__TESTBEGIN <= MGCQF__TESTEND);


class CTesterBase
{
public:
	typedef size_t production_size_type;

	struct CTestStatistics
	{
		CTestStatistics()
		{
		}

		void AssignFiedls(production_size_type siProducedCount, production_size_type siConsumedSum, 
			production_size_type siAbortedSum, production_size_type siCanceledSum, 
			production_size_type siOnDiscardDroppedSum, production_size_type siOnDiscardWaitedCount)
		{
			m_siProducedCount = siProducedCount;
			m_siConsumedSum = siConsumedSum;
			m_siAbortedSum = siAbortedSum;
			m_siCanceledSum = siCanceledSum;
			m_siOnDiscardDroppedSum = siOnDiscardDroppedSum;
			m_siOnDiscardWaitedCount = siOnDiscardWaitedCount;
		}

		bool IsDataAcceptable(bool bZeroCanceledCountIsAllowed) const
		{
			bool bResult = (m_siConsumedSum != 0 || m_siAbortedSum != 0) 
				&& (m_siCanceledSum != 0 || bZeroCanceledCountIsAllowed)
				&& (m_siOnDiscardDroppedSum != 0 || m_siOnDiscardWaitedCount != 0)
				&& m_siProducedCount * (m_siProducedCount + 1) / 2 == m_siConsumedSum + m_siAbortedSum + m_siCanceledSum + m_siOnDiscardDroppedSum;
			return bResult;
		}

		void PrintContents()
		{
			if (!IsSkippedState())
			{
				printf(
					"\tProduced: Count - %zu, Sum - %zu\n"
					"\tSuccessfully Consumed: Sum - %zu\n"
					"\tStarted but Then Aborted: Sum - %zu\n"
					"\tCanceled From Queue Prior to Being Started: Sum - %zu\n"
					"\tDropped on Finalization: Sum - %zu\n"
					"\tWaited for Consumption Finish on Finalization: Count - %zu\n",
					m_siProducedCount, m_siProducedCount * (m_siProducedCount + 1) / 2,
					m_siConsumedSum, m_siAbortedSum,
					m_siCanceledSum,
					m_siOnDiscardDroppedSum, m_siOnDiscardWaitedCount);
			}
		}

		void AssignSkippedState() { m_siProducedCount = 0; }
		bool IsSkippedState() const { return m_siProducedCount == 0; }

		production_size_type m_siProducedCount;
		production_size_type m_siConsumedSum;
		production_size_type m_siAbortedSum;
		production_size_type m_siCanceledSum;
		production_size_type m_siOnDiscardDroppedSum;
		production_size_type m_siOnDiscardWaitedCount;
	};

};


typedef void (*CCompletionQueueFeatureTestProcedure)(CTesterBase::CTestStatistics &tsOutTestStatistics);

static void PerformWaitableQueueTest(CTesterBase::CTestStatistics &tsOutTestStatistics);
static void PerformCancelableQueueTest(CTesterBase::CTestStatistics &tsOutTestStatistics);


static const CCompletionQueueFeatureTestProcedure g_afnWorkerQueueFeatureTestProcedures[MGCQF__MAX] =
{
	&PerformWaitableQueueTest, // MGCQF_WAITABLE_QUEUE,
	&PerformCancelableQueueTest, // MGCQF_CANCELABLE_QUEUE,
};

static const char *const g_aszWorkerQueueFeatureTestNames[MGCQF__MAX] =
{
	"Waitable Queue", // MGCQF_WAITABLE_QUEUE,
	"Cancelable Queue", // MGCQF_CANCELABLE_QUEUE,
};


/*static */EMGTESTFEATURELEVEL	CCompletionQueueTest::m_flSelectedFeatureTestLevel = MGTFL__DEFAULT;


/*static */
bool CCompletionQueueTest::RunTheTest(unsigned int &nOutSuccessCount, unsigned int &nOutTestCount)
{
	unsigned int nSuccessCount = 0;
	
	const EMGTESTFEATURELEVEL tlFeatureTestLevel = CCompletionQueueTest::RetrieveSelectedFeatureTestLevel();

	CTesterBase::CTestStatistics tsTestStatistics;

	for (EMGCOMPLETIONQUEUESFEATURE qfCompletionQueueFeature = MGCQF__TESTBEGIN; qfCompletionQueueFeature != MGCQF__TESTEND; ++qfCompletionQueueFeature)
	{
		const char *szFeatureName = g_aszWorkerQueueFeatureTestNames[qfCompletionQueueFeature];
		printf("Testing %29s: ", szFeatureName);

		CCompletionQueueFeatureTestProcedure fnTestProcedure = g_afnWorkerQueueFeatureTestProcedures[qfCompletionQueueFeature];
		fnTestProcedure(tsTestStatistics);

		bool bZeroCanceledCountIsAllowed = qfCompletionQueueFeature != MGCQF_CANCELABLE_QUEUE;
		bool bSkippedState = tsTestStatistics.IsSkippedState();
		bool bTestResult = bSkippedState || tsTestStatistics.IsDataAcceptable(bZeroCanceledCountIsAllowed);
		printf("%s\n", bSkippedState ? "skipped" : bTestResult ? "success" : "failure");

		if (IN_RANGE(tlFeatureTestLevel, MGTFL__DUMP_MIN, MGTFL__DUMP_MAX))
		{
			tsTestStatistics.PrintContents();
		}

		if (bTestResult)
		{
			nSuccessCount += 1;
		}
	}

	nOutSuccessCount = nSuccessCount;
	nOutTestCount = MGCQF__TESTCOUNT;
	return nSuccessCount == MGCQF__TESTCOUNT;
}


#if _MGTEST_HAVE_CXX11

template<class TQueueType, unsigned tuiProducerCount, unsigned tuiConsumerCount>
class CQueueTester:
	public CTesterBase
{
public:
	typedef TQueueType queue_type;
	typedef typename queue_type::const_iterator const_queue_iterator;

	CQueueTester();
	~CQueueTester();

public:
	using CTesterBase::production_size_type;
	using CTesterBase::CTestStatistics;
	typedef typename cancelable_queue::ownership_type ownership_type;

	void RunTheTest(CTestStatistics &tsOutTestStatistics);

private:
	void PrintTestStatisticsIfNecessary(production_size_type siProducedCount, production_size_type siConsumedCount, production_size_type siAbortedCount);

private:
	static constexpr unsigned m_uiTotalThreadCount = tuiProducerCount + tuiConsumerCount;
	static constexpr unsigned m_uiTestQueueSizeLimit = MGTEST_CQ_QUEUE_SIZE_LIMIT;
	static constexpr unsigned m_uiTestQueueSizeHighWatermark = NGTEST_CQ_QUEUE_SIZE_HIWATER;

	class CThreadBase;
	typedef array<unique_ptr<CThreadBase>, m_uiTotalThreadCount> CThreadStorageArray;
	typedef typename CThreadStorageArray::iterator thread_iterator;
	typedef typename CThreadStorageArray::size_type thread_size_type;

	static thread_size_type producer_count() noexcept { return tuiProducerCount; }
	static thread_size_type consumer_count() noexcept { return tuiConsumerCount; }

	class CWorkItem:
		public parent_wrapper<item>,
		private parent_wrapper<dlps_info, 0x21102803>
	{
		typedef parent_wrapper<item> CWorkItem_Parent;
		typedef parent_wrapper<dlps_info, 0x21102803> CWorkItem_CancelableLinkParent;

	public:
		static CWorkItem *GetInstanceFromItem(const item *psiItemInstance) noexcept { return const_cast<CWorkItem *>(static_cast<const CWorkItem *>(static_cast<const CWorkItem_Parent *>(psiItemInstance))); }

		explicit CWorkItem(production_size_type siItemIndex):
			CWorkItem_Parent(),
			CWorkItem_CancelableLinkParent(),
			m_siItemIndex(siItemIndex)
		{
		}

		static CWorkItem *GetInstanceFromCancelableLink(const dlps_info &dCancelableiLink) noexcept { return &static_cast<CWorkItem &>(static_cast<CWorkItem_CancelableLinkParent &>(const_cast<dlps_info &>(dCancelableiLink))); }
		void LinkIntoCancelableList(dlps_list &dlListInstance) noexcept { dlListInstance.link_back(static_cast<CWorkItem_CancelableLinkParent *>(this)); }
		void UnlinkFromCancelableListIfNecessary() noexcept { dlps_info *ppiThisCancelableLink = static_cast<CWorkItem_CancelableLinkParent *>(this); if (ppiThisCancelableLink->linked()) { dlps_list::unlink(dlps_list::make_iterator(ppiThisCancelableLink)); } }
		bool IsLinkedIntoCancelableList() const noexcept { return static_cast<const CWorkItem_CancelableLinkParent *>(this)->linked(); }

	public:
		production_size_type GetItemIndex() const noexcept { return m_siItemIndex; }

	private:
		production_size_type		m_siItemIndex;
	};


	template<class TAnotherQueueType, bool tsbEnabledMode=std::is_same<TAnotherQueueType, cancelable_queue>::value>
	class CItemLinkerTemplate;

	template<class TAnotherQueueType>
	class CItemLinkerTemplate<TAnotherQueueType, false>
	{
	public:
		void LinkItemInIfNecessary(CWorkItem &wiItemInstance) noexcept {}
		void UnlinkItemIfNecessary(CWorkItem &wiItemInstance) noexcept {}
		bool IsItemLinked(const CWorkItem &wiItemInstance) const noexcept { return false; }
		bool UnlinkFrontItem(CWorkItem *&pwiOutFrontItem) { return false; }
	};

	template<class TAnotherQueueType>
	class CItemLinkerTemplate<TAnotherQueueType, true>
	{
	public:
		void LinkItemInIfNecessary(CWorkItem &wiItemInstance)
		{
			// For the purpose of this example, link in every fourth item
			if (wiItemInstance.GetItemIndex() % 4 == 0)
			{
				lock_guard<mutex> lgLockInstance(GetCancelableLinkLock());

				dlps_list &dlCancelableListRef = GetCancelableList();
				wiItemInstance.LinkIntoCancelableList(dlCancelableListRef);
			}
		}

		void UnlinkItemIfNecessary(CWorkItem &wiItemInstance)
		{
			lock_guard<mutex> lgLockInstance(GetCancelableLinkLock());

			wiItemInstance.UnlinkFromCancelableListIfNecessary();
		}

		bool IsItemLinked(const CWorkItem &wiItemInstance) const
		{
			lock_guard<mutex> lgLockInstance(GetCancelableLinkLock());

			return wiItemInstance.IsLinkedIntoCancelableList();
		}

		bool UnlinkFrontItem(CWorkItem *&pwiOutFrontItem)
		{
			bool bResult = false;

			lock_guard<mutex> lgLockInstance(GetCancelableLinkLock());

			dlps_list &dlCancelableListRef = GetCancelableList();

			if (!dlCancelableListRef.empty())
			{
				pwiOutFrontItem = CWorkItem::GetInstanceFromCancelableLink(dlCancelableListRef.front());
				dlCancelableListRef.unlink_front();

				bResult = true;
			}

			return bResult;
		}

	private:
		dlps_list &GetCancelableList() const noexcept { return const_cast<dlps_list &>(m_dlCancelableList); }
		mutex &GetCancelableLinkLock() const noexcept { return const_cast<mutex &>(m_mCancelableLinkLock); }

	private:
		dlps_list	m_dlCancelableList;
		mutex		m_mCancelableLinkLock;
	};

	typedef CItemLinkerTemplate<queue_type> CWorkItemLinker;

	struct CWorkQueueObjects
	{
		typedef CQueueTester::CWorkItemLinker CWorkItemLinker;

		explicit CWorkQueueObjects(thread_size_type siActiveConsumerCount):
			m_qCompletionQueue(),
			m_itItemAwaitingStart(m_qCompletionQueue.end()),
			m_lsiQueueReserve(0),
			m_lsiQueueSize(0),
			m_siActiveConsumerCount(siActiveConsumerCount),
			m_mConsumptionWakeupLock(),
			m_cvConsumptionWakeupEvent(),
			m_abGenerationHighWatermarkReached(false),
			m_mProductionWakeupLock(),
			m_cvProductionWakeupEvent()
		{
		}

		~CWorkQueueObjects()
		{
			MG_ASSERT(m_qCompletionQueue.empty());
		}

		typedef size_t size_type;

		bool AllocateQueueQuota() noexcept
		{
			bool bResult = false;

			const size_type siSizeLimit = GetQueueSizeLimit();

			for (size_type siCurrentSize = m_lsiQueueReserve.load(std::memory_order_acquire); siCurrentSize < siSizeLimit; )
			{
				if (m_lsiQueueReserve.compare_exchange_strong(siCurrentSize, siCurrentSize + 1, std::memory_order_seq_cst, std::memory_order_acquire))
				{
					bResult = true;
					break;
				}

				MG_ASSERT(siCurrentSize <= siSizeLimit);
			}

			return bResult;
		}

		size_type IncrementQueueSize() noexcept
		{
			size_type siResult = m_lsiQueueSize.fetch_add(1, std::memory_order_seq_cst) + 1;
			MG_ASSERT(siResult != 0);

			return siResult;
		}

		void WaitQueueQuotaAvailability(const atomic_bool *pabAbortFlag/*=nullptr*/)
		{
			const size_type siSizeLimit = GetQueueSizeLimit();
			
			size_type siCurrentSize = m_lsiQueueReserve.load(std::memory_order_relaxed);
			MG_ASSERT(siCurrentSize <= siSizeLimit);

			if (siCurrentSize == siSizeLimit)
			{
				SetGenerationHighWatermarkReachedFlag();

				unique_lock<mutex> ulLockInstance(GetProductionWakeupLock());

				size_type siCurrentSizeRecheck = m_lsiQueueReserve.load(std::memory_order_acquire);
				MG_ASSERT(siCurrentSizeRecheck <= siSizeLimit);
				
				if (siCurrentSizeRecheck == siSizeLimit && (pabAbortFlag == nullptr || !pabAbortFlag->load(std::memory_order_relaxed)))
				{
					condition_variable &cvProductionWakeupEvent = GetProductionWakeupEvent();
					cvProductionWakeupEvent.wait(ulLockInstance);
				}
			}
		}

		void WakeupAConsumerIfNecessary(size_type siQueueSize)
		{
			const thread_size_type siTotalConsumerCount = consumer_count();

			if (siQueueSize <= siTotalConsumerCount)
			{
				thread_size_type siActiveConsumerCount;
				{
					lock_guard<mutex> lgLockInstance(GetConsumptionWakeupLock());

					siActiveConsumerCount = m_siActiveConsumerCount;
				}

				if (siActiveConsumerCount < siQueueSize)
				{
					condition_variable &cvConsumptionWakeupEvent = GetConsumptionWakeupEvent();
					cvConsumptionWakeupEvent.notify_one();
				}
			}
		}

		void WaitQueueItemAvailability(const atomic_bool *pabAbortFlag/*=nullptr*/)
		{
			if (GetConsumptionExhaustReachedFlag())
			{
				unique_lock<mutex> ulLockInstance(GetConsumptionWakeupLock());

				if (GetConsumptionExhaustReachedFlag() && (pabAbortFlag == nullptr || !pabAbortFlag->load(std::memory_order_relaxed)))
				{
					{
						MG_ASSERT(m_siActiveConsumerCount != 0);

						--m_siActiveConsumerCount;
					}

					m_cvConsumptionWakeupEvent.wait(ulLockInstance);

					{
						++m_siActiveConsumerCount;
						MG_ASSERT(m_siActiveConsumerCount != 0);
					}
				}
			}
		}

		void DecrementQueueSizeAndWakeupProducersIfNecessary()
		{
			size_type siOldQueueSize = m_lsiQueueSize.fetch_sub(1, std::memory_order_relaxed);
			MG_VERIFY(siOldQueueSize != 0);

			size_type siOldQueueReserve = m_lsiQueueReserve.fetch_sub(1, std::memory_order_seq_cst);

			if (siOldQueueReserve == GetQueueSizeHighWatermark() && ResetGenerationHighWatermarkReachedFlag())
			{
				lock_guard<mutex> lgLockInstance(GetProductionWakeupLock());

				condition_variable &cvProductionWakeupEvent = GetProductionWakeupEvent();
				cvProductionWakeupEvent.notify_all();
			}
		}

		void WakeupAllProducers()
		{
			lock_guard<mutex> lgLockInstance(GetProductionWakeupLock());

			condition_variable &cvProductionWakeupEvent = GetProductionWakeupEvent();
			cvProductionWakeupEvent.notify_all();
		}

		void UnblockAllConsumers()
		{
			lock_guard<mutex> lgLockInstance(GetConsumptionWakeupLock());

			condition_variable &cvConsumptionWakeupEvent = GetConsumptionWakeupEvent();
			cvConsumptionWakeupEvent.notify_all();
		}

		template<class TAnotherQueueType, bool tsbEnabledMode = std::is_same<TAnotherQueueType, cancelable_queue>::value>
		class CItemCancelProcessorTemplate;

		template<class TAnotherQueueType>
		class CItemCancelProcessorTemplate<TAnotherQueueType, false>
		{
		public:
			production_size_type operator ()(CWorkQueueObjects &qoRefQueueObjects, CWorkItemLinker &ilCancelableLinker, waiter &wRefWaiterToBeEngaged) noexcept { return 0; }
		};

		template<class TAnotherQueueType>
		class CItemCancelProcessorTemplate<TAnotherQueueType, true>
		{
		public:
			production_size_type operator ()(CWorkQueueObjects &qoRefQueueObjects, CWorkItemLinker &ilCancelableLinker, waiter &wRefWaiterToBeEngaged)
			{
				production_size_type nCanceledSum = 0;

				for (queue_lock_helper<queue_type> lhQueueLock(qoRefQueueObjects.m_qCompletionQueue); ; lhQueueLock.lock())
				{
					// Do not wrap the item into unique_ptr<> in advance as the function can bail out on exception 
					// yet before the item is canceled. 
					// Note that unlinking is not a problem in this case though and this canceling attempt was 
					// the only purpose for which the item was linked into the list.
					CWorkItem *pwiItemToBeCanceled;
					if (!ilCancelableLinker.UnlinkFrontItem(pwiItemToBeCanceled))
					{
						break;
					}

					MG_ASSERT(pwiItemToBeCanceled != nullptr);

					// If the item being canceled is the one pointed to by the iterator it means that
					// the item handling has not been started yet and the unlock_and_cancel() call below
					// is going to dequeue the item and assign this thread the owner status.
					// Therefore it is possible (and necessary) to adjust the iterator 
					// while the queue lock critical section has not been exited yet.
					if (!qoRefQueueObjects.IsItemsAwaitingStartEndReached())
					{
						qoRefQueueObjects.AdvanceItemAwaitingStartIfMatchesItem((item_view)*pwiItemToBeCanceled);
					}

					ownership_type oResultingItemOwnership;
					lhQueueLock.unlock_and_cancel(*pwiItemToBeCanceled, wRefWaiterToBeEngaged, oResultingItemOwnership, &CQueueTester::AbortWorkItem);

					// Now if the item was successfully canceled...
					// ...and if the ownership was given to this thread...
					if (oResultingItemOwnership == ownership_type::owner)
					{
						// ...destroy the canceled item.

						// Since unique_ptr allocation/destruction is used for items within this demo,
						// construct a unique_ptr with the item to have it deleted.
						unique_ptr<CWorkItem> uwiItemDestroyer(pwiItemToBeCanceled);

						nCanceledSum += uwiItemDestroyer->GetItemIndex();
					}
				}

				return nCanceledSum;
			}
		};

		typedef CItemCancelProcessorTemplate<queue_type> CWorkItemCancelProcessor;

		void CancelAllLinkedWorkItems(CWorkItemLinker &ilCancelableLinker, waiter &wRefWaiterToBeEngaged, 
			production_size_type &nOutCanceledSum)
		{
			// Normally, the code below would be placed inline.
			// Here, the cancel operation is executed via a template invocation 
			// to allow compiling the same example code both for the queues supporting and not supporting the operation.
			CWorkItemCancelProcessor cpCancelProcessor;
			nOutCanceledSum = cpCancelProcessor(*this, ilCancelableLinker, wRefWaiterToBeEngaged);
		}

		void DiscardOrWaitAllWorkItems(waiter &wRefWaiterToBeEngaged, 
			production_size_type &nOutDroppedSum, production_size_type &nOutWaitedCount)
		{
			production_size_type nDroppedSum = 0, nWaitedCount = 0;

			for (queue_lock_helper<queue_type> lhQueueLock(m_qCompletionQueue); !m_qCompletionQueue.empty(); lhQueueLock.lock())
			{
				// Discard items in direction opposite to consumption to avoid always falling onto the item being processed...
				const_queue_iterator itCurrentItem = m_qCompletionQueue.end(); 
				--itCurrentItem;

				const item_view &ivCurrentItem = *itCurrentItem;

				if (ivCurrentItem.is_started())
				{
					lhQueueLock.unlock_and_wait(ivCurrentItem, wRefWaiterToBeEngaged);

					++nWaitedCount;
				}
				else
				{
					unique_ptr<CWorkItem> uwiFrontItem(CWorkItem::GetInstanceFromItem(&item::instance_from_pointer(ivCurrentItem)));
					
					nDroppedSum += uwiFrontItem->GetItemIndex();

					AdvanceItemAwaitingStartIfMatchesItem(ivCurrentItem);

					m_qCompletionQueue.dequeue(ivCurrentItem);

					// Unlock the queue to have item destructor called outside of the critical section
					lhQueueLock.unlock();
				}
			}

			nOutDroppedSum = nDroppedSum;
			nOutWaitedCount = nWaitedCount;
		}

		void AdvanceItemAwaitingStartIfMatchesItem(const item_view &ivItemInstance) noexcept { if (*m_itItemAwaitingStart == ivItemInstance) { ++m_itItemAwaitingStart; } }
		void AdvanceToNextItemAwaitingStart() noexcept { ++m_itItemAwaitingStart; }
		bool SetLastQueuedItemAsTheItemAwaitingStartIfNecessary() noexcept { return m_itItemAwaitingStart == m_qCompletionQueue.end() && (--m_itItemAwaitingStart, true); }
		bool IsItemsAwaitingStartEndReached() const noexcept { return m_itItemAwaitingStart == m_qCompletionQueue.end(); }

		static size_type GetQueueSizeLimit() noexcept { return m_uiTestQueueSizeLimit; }
		static size_type GetQueueSizeHighWatermark() noexcept { return m_uiTestQueueSizeHighWatermark; }

		mutex &GetConsumptionWakeupLock() const noexcept { return const_cast<mutex &>(m_mConsumptionWakeupLock); }
		condition_variable &GetConsumptionWakeupEvent() const noexcept { return const_cast<condition_variable &>(m_cvConsumptionWakeupEvent); }

		void SetConsumptionExhaustReachedFlag() noexcept { m_abConsumptionExhaustReached.store(true, std::memory_order_relaxed); }
		void ResetConsumptionExhaustReachedFlag() noexcept { m_abConsumptionExhaustReached.store(false, std::memory_order_relaxed); }
		bool GetConsumptionExhaustReachedFlag() const noexcept { return m_abConsumptionExhaustReached.load(std::memory_order_relaxed); }

		void SetGenerationHighWatermarkReachedFlag() noexcept { m_abGenerationHighWatermarkReached.store(true, std::memory_order_release); }
		bool ResetGenerationHighWatermarkReachedFlag() noexcept { bool bExpectedValue = true; return m_abGenerationHighWatermarkReached.compare_exchange_strong(bExpectedValue, false, std::memory_order_seq_cst, std::memory_order_acquire); }

		mutex &GetProductionWakeupLock() const noexcept { return const_cast<mutex &>(m_mProductionWakeupLock); }
		condition_variable &GetProductionWakeupEvent() const noexcept { return const_cast<condition_variable &>(m_cvProductionWakeupEvent); }

		queue_type				m_qCompletionQueue;
		const_queue_iterator	m_itItemAwaitingStart;
		atomic<size_type>		m_lsiQueueReserve;
		atomic<size_type>		m_lsiQueueSize;
		thread_size_type		m_siActiveConsumerCount;

		mutex					m_mConsumptionWakeupLock;
		condition_variable		m_cvConsumptionWakeupEvent;
		atomic_bool				m_abConsumptionExhaustReached;

		atomic_bool				m_abGenerationHighWatermarkReached;
		mutex					m_mProductionWakeupLock;
		condition_variable		m_cvProductionWakeupEvent;
	};


	class CThreadBase:
		protected thread
	{
	public:
		typedef thread CThreadBase_ThreadParent;
		typedef CQueueTester::CWorkItemLinker CWorkItemLinker;

		explicit CThreadBase(CQueueTester *pqtHostTester, CWorkQueueObjects &qoRefQueueObjects, CWorkItemLinker &ilRefCancelableLinker):
			CThreadBase_ThreadParent(),
			m_abExitRequest(false),
			m_pqtHostTester(pqtHostTester),
			m_qoQueueObjectsRef(qoRefQueueObjects),
			m_clCancelableLinkerRef(ilRefCancelableLinker)
		{
		}

		virtual ~CThreadBase()
		{
			RequestExitAndJoinTheThreadIfNecessary();  // For the sake of abstraction ;)
		}

	protected:
		void RequestExitAndJoinTheThreadIfNecessary()
		{
			if (IsThreadJoinable())
			{
				RequestThreadExit();

				JoinOwnThread();
			}
		}

	public:
		virtual void RequestThreadExit()
		{
			SetExitRequestFlag();
		}

		typedef typename CWorkQueueObjects::size_type size_type;
		
	protected:
		CThreadBase &operator =(thread &&tThreadInstance) { CThreadBase_ThreadParent::operator =(std::move(tThreadInstance)); return *this; }

	protected:
		CQueueTester *GetHostTester() const noexcept { return m_pqtHostTester; }
		CWorkQueueObjects &GetQueueObjects() const noexcept { return const_cast<CWorkQueueObjects &>(m_qoQueueObjectsRef); }
		CWorkItemLinker &GetCancelableLinker() const noexcept { return const_cast<CWorkItemLinker &>(m_clCancelableLinkerRef); }

		void SetExitRequestFlag() noexcept { m_abExitRequest.store(true, std::memory_order_release); }
		bool GetExitRequestFlag() const noexcept { return m_abExitRequest.load(std::memory_order_acquire); }
		const atomic_bool &GetExitRequestStorage() const noexcept { return m_abExitRequest; }

	protected:
		bool IsThreadJoinable() const noexcept { return CThreadBase_ThreadParent::joinable(); }
		void JoinOwnThread() { CThreadBase_ThreadParent::join(); }

	private:
		atomic_bool			m_abExitRequest;
		CQueueTester		*m_pqtHostTester;
		CWorkQueueObjects	&m_qoQueueObjectsRef;
		CWorkItemLinker		&m_clCancelableLinkerRef;
	};

	class CProducerThread:
		public CThreadBase 
	{
	public:
		typedef CThreadBase CProducerThread_Parent;
		using typename CProducerThread_Parent::size_type;
		using typename CProducerThread_Parent::CWorkItemLinker;

		CProducerThread(CQueueTester *pqtHostTester, CWorkQueueObjects &qoRefQueueObjects, CWorkItemLinker &ilRefCancelableLinker):
			CProducerThread_Parent(pqtHostTester, qoRefQueueObjects, ilRefCancelableLinker)
		{
			CProducerThread_Parent::operator =(std::move(thread([&](){ ExecuteAsAProducer(); })));
		}

		~CProducerThread() override
		{
			RequestExitAndJoinTheThreadIfNecessary(); // The thread must be joined in the leaf class so that the fields did not start getting destroyed while the thread would be still running
		}

		void RequestThreadExit() override
		{
			CProducerThread_Parent::RequestThreadExit();

			UnblockThisProducer();
		}

	private:
		void ExecuteAsAProducer()
		{
			while (!GetExitRequestFlag())
			{
				if (AllocateQueueQuota())
				{
					size_type siQueueSize = InsertNewQueueItem();
					WakeupAConsumerIfNecessary(siQueueSize);
				}
				else
				{
					WaitQueueQuotaAvailability();
				}
			}
		}

		size_type InsertNewQueueItem()
		{
			size_type siResult;

			CQueueTester *pqtHostTester = GetHostTester();
			production_size_type siItemIndex = pqtHostTester->IncrementProducedCount();

			unique_ptr<CWorkItem> uwiWorkItem(new CWorkItem(siItemIndex));

			CWorkQueueObjects &qoQueueObjectsRef = GetQueueObjects();
			{
				queue_lock_helper<queue_type> lhQueueLockHelper(qoQueueObjectsRef.m_qCompletionQueue, acquire_tocken_t());

				CWorkItemLinker &ilCancelableLinkerRef = GetCancelableLinker();
				ilCancelableLinkerRef.LinkItemInIfNecessary(*uwiWorkItem);

				qoQueueObjectsRef.m_qCompletionQueue.enqueue(*uwiWorkItem, lhQueueLockHelper.token());
				uwiWorkItem.release();

				if (qoQueueObjectsRef.SetLastQueuedItemAsTheItemAwaitingStartIfNecessary())
				{
					qoQueueObjectsRef.ResetConsumptionExhaustReachedFlag(); // This can be done after leaving the lock but is kept here to not make the example overcomplicated
				}

				siResult = qoQueueObjectsRef.IncrementQueueSize(); // Increment the queue size within the lock, atomically with the insertion!!!
			}

			return siResult;
		}

		void WakeupAConsumerIfNecessary(size_type siQueueSize)
		{
			CWorkQueueObjects &qoQueueObjectsRef = GetQueueObjects();
			qoQueueObjectsRef.WakeupAConsumerIfNecessary(siQueueSize);
		}

		bool AllocateQueueQuota()
		{
			CWorkQueueObjects &qoQueueObjectsRef = GetQueueObjects();
			return qoQueueObjectsRef.AllocateQueueQuota();
		}

		void WaitQueueQuotaAvailability()
		{
			const atomic_bool &abExitRequestStorage = GetExitRequestStorage();

			CWorkQueueObjects &qoQueueObjectsRef = GetQueueObjects();
			return qoQueueObjectsRef.WaitQueueQuotaAvailability(&abExitRequestStorage);
		}

		void UnblockThisProducer()
		{
			CWorkQueueObjects &qoQueueObjectsRef = GetQueueObjects();
			qoQueueObjectsRef.WakeupAllProducers();
		}

	protected:
		using CProducerThread_Parent::RequestExitAndJoinTheThreadIfNecessary;
		using CProducerThread_Parent::IsThreadJoinable;
		using CProducerThread_Parent::GetExitRequestFlag;
		using CProducerThread_Parent::GetExitRequestStorage;
		using CProducerThread_Parent::GetHostTester;
		using CProducerThread_Parent::GetQueueObjects;
		using CProducerThread_Parent::GetCancelableLinker;
	};
	
	class CConsumerThread :
		public CThreadBase,
		private parent_wrapper<worker>
	{
	public:
		typedef CThreadBase CConsumerThread_Parent;
		typedef parent_wrapper<worker> CConsumerThread_WorkerParent;
		using typename CConsumerThread_Parent::CWorkItemLinker;

		static CConsumerThread *GetInstanceFromWorker(const worker_view &wvWorkerInstance) noexcept { return static_cast<CConsumerThread *>(static_cast<CConsumerThread_WorkerParent *>(&worker::instance_from_pointer(wvWorkerInstance))); }

		CConsumerThread(CQueueTester *pqtHostTester, CWorkQueueObjects &qoRefQueueObjects, CWorkItemLinker &ilRefCancelableLinker) :
			CConsumerThread_Parent(pqtHostTester, qoRefQueueObjects, ilRefCancelableLinker)
		{
			CConsumerThread_Parent::operator =(std::move(thread([&]() { ExecuteAsAConsumer(); })));
		}

		~CConsumerThread() override
		{
			RequestExitAndJoinTheThreadIfNecessary(); // The thread must be joined in the leaf class so that the fields did not start getting destroyed while the thread would be still running
		}

		void RequestThreadExit() override
		{
			CConsumerThread_Parent::RequestThreadExit();

			UnblockThisConsumer();
		}

	private:
		void ExecuteAsAConsumer()
		{
			worker &wConsumerWorker = GetWorkerInstance();
			lock_guard<worker> lgWorkerLockGuard(wConsumerWorker);

			for (bool bExitTheLoop = false; !bExitTheLoop; bExitTheLoop = GetExitRequestFlag())
			{
				// The consumers need to be declared "active" before start 
				// for the initial wait had a positive counter value for decrementing.
				// And since the consumers are declared "active" they need to initially go and check the work queue 
				// rather than calling the wait at once.
				bool bExitRequested;
				ProcessQueueItems(bExitRequested);

				if (bExitRequested)
				{
					break;
				}

				WaitWorkAvailability();
			}
		}

		void ProcessQueueItems(bool &bOutExitRequested)
		{
			bool bQueueEndReached = false;

			CWorkQueueObjects &qoQueueObjectsRef = GetQueueObjects();
			worker &wConsumerWorker = GetWorkerInstance();

			for (bool bExitTheLoop = false; !bExitTheLoop; bExitTheLoop = GetExitRequestFlag())
			{
				// Be sure to first declare the work item...
				unique_ptr<CWorkItem> uwiWorkItem;
				// ..and then the work helper after it.
				// This is necessary for the work helper to be destroyed first (e. g. in case of an exception)
				// and finish its job on the work item. Otherwise the work item will fail an assertion 
				// to be not locked at destruction.
				queue_work_helper<queue_type> whWorkHelper(qoQueueObjectsRef.m_qCompletionQueue);
				{
					queue_lock_helper<queue_type> lhQueueLockHelper(qoQueueObjectsRef.m_qCompletionQueue);

					if (qoQueueObjectsRef.IsItemsAwaitingStartEndReached())
					{
						qoQueueObjectsRef.SetConsumptionExhaustReachedFlag();

						bQueueEndReached = true;
						break;
					}

					const item_view &ivNextWorkItemView = *qoQueueObjectsRef.m_itItemAwaitingStart;
					uwiWorkItem.reset(CWorkItem::GetInstanceFromItem(&item::instance_from_pointer(ivNextWorkItemView)));

					whWorkHelper.start(*uwiWorkItem, wConsumerWorker);

					qoQueueObjectsRef.AdvanceToNextItemAwaitingStart();
				}

				DecrementQueueSizeAndWakeupProducersIfNecessary();
				HandleSingleWorkItem(*uwiWorkItem);

				whWorkHelper.safefinish();
			}

			bOutExitRequested = !bQueueEndReached;
		}

		void HandleSingleWorkItem(CWorkItem &wiWorkItem)
		{
			production_size_type siItemIndex = wiWorkItem.GetItemIndex();

			bool bWorkWasCanceled = false;

			// Sleep to simulate hard work
			auto tpWakeupTime = steady_clock::now() + (milliseconds)1;
			for (unique_lock<mutex> ulLockInstance(GetWorkSimulationLock()); ; )
			{
				condition_variable &cvWorkCancelationEvent = GetWorkCancelationEvent();
				if (cvWorkCancelationEvent.wait_until(ulLockInstance, tpWakeupTime) == cv_status::timeout)
				{
					CWorkItemLinker &ilCancelableLinkerRef = GetCancelableLinker();
					ilCancelableLinkerRef.UnlinkItemIfNecessary(wiWorkItem);
					break;
				}

				// If the condvar was signaled check whether the current item's work was requested to be canceled
				worker &wConsumerWorker = GetWorkerInstance();
				if (wiWorkItem.is_canceled(wConsumerWorker))
				{
					MG_ASSERT(!GetCancelableLinker().IsItemLinked(wiWorkItem)); // The item must have been unlinked for being canceled

					bWorkWasCanceled = true;
					break;
				}

				// If not, continue slee... that is, working
			}

			CQueueTester *pqtHostTester = GetHostTester();
			pqtHostTester->AccountConsumedIndex(siItemIndex, bWorkWasCanceled);
		}

		void WaitWorkAvailability()
		{
			const atomic_bool &abExitRequestStorage = GetExitRequestStorage();

			CWorkQueueObjects &qoQueueObjectsRef = GetQueueObjects();
			qoQueueObjectsRef.WaitQueueItemAvailability(&abExitRequestStorage);

		}

		void DecrementQueueSizeAndWakeupProducersIfNecessary()
		{
			CWorkQueueObjects &qoQueueObjectsRef = GetQueueObjects();
			qoQueueObjectsRef.DecrementQueueSizeAndWakeupProducersIfNecessary();
		}

		void UnblockThisConsumer()
		{
			CWorkQueueObjects &qoQueueObjectsRef = GetQueueObjects();
			qoQueueObjectsRef.UnblockAllConsumers();
		}


	public:
		void AbortWorkItemHandling()
		{
			// ... Since the worker simulates its business by waiting on an orphaned condvar
			// go on and signal the latter.
			lock_guard<mutex> lgLockInstance(GetWorkSimulationLock());

			condition_variable &cvWorkCancelationEvent = GetWorkCancelationEvent();
			cvWorkCancelationEvent.notify_all();
		}

	protected:
		using CConsumerThread_Parent::RequestExitAndJoinTheThreadIfNecessary;
		using CConsumerThread_Parent::IsThreadJoinable;
		using CConsumerThread_Parent::GetExitRequestFlag;
		using CConsumerThread_Parent::GetExitRequestStorage;
		using CConsumerThread_Parent::GetHostTester;
		using CConsumerThread_Parent::GetQueueObjects;
		using CConsumerThread_Parent::GetCancelableLinker;

	private:
		worker &GetWorkerInstance() const noexcept { return const_cast<worker &>(static_cast<const worker &>(static_cast<const CConsumerThread_WorkerParent &>(*this))); }

		mutex &GetWorkSimulationLock() const noexcept { return const_cast<mutex &>(m_mWorkSimulationLock); }
		condition_variable &GetWorkCancelationEvent() const { return const_cast<condition_variable &>(m_cvWorkCancelationEvent); }

	private:
		mutex				m_mWorkSimulationLock;
		condition_variable	m_cvWorkCancelationEvent;
	};

private:
	friend class CProducerThread;

	production_size_type IncrementProducedCount() noexcept
	{
		production_size_type siResult = m_lsiProducedCount.fetch_add(1, std::memory_order_seq_cst) + 1;
		return siResult;
	}

private:
	friend class CConsumerThread;

	void AccountConsumedIndex(production_size_type siItemIndex, bool bTheItemWasCanceled) noexcept
	{
		(!bTheItemWasCanceled ? m_lsiConsumedSum : m_lsiAbortedSum).fetch_add(siItemIndex, std::memory_order_relaxed);
	}

private:
	static void AbortWorkItem(queue_type &qQueueInstance, const worker_view &wvWorkerInstance, const item_view &ivItemInstance)
	{
		// Here you do whatever is necessary to unblock the worker from the blocked states 
		// it could be in... (see inside)
		CConsumerThread *pctConsumerInstance = CConsumerThread::GetInstanceFromWorker(wvWorkerInstance);
		pctConsumerInstance->AbortWorkItemHandling();
	}

private:
	void CreateAndStartAllConsumers()
	{
		CWorkQueueObjects &qoQueueObjectsRef = GetQueueObjects();
		CWorkItemLinker &ilCancelableLinkerRef = GetCancelableLinker();

		for (auto &utbCurrentThreadRef : CConsumerEnumerator(this))
		{
			utbCurrentThreadRef.reset(new CConsumerThread(this, qoQueueObjectsRef, ilCancelableLinkerRef));
		}
	}

	void CreateAndStartAllProducers()
	{
		CWorkQueueObjects &qoQueueObjectsRef = GetQueueObjects();
		CWorkItemLinker &ilCancelableLinkerRef = GetCancelableLinker();

		for (auto &utbCurrentThreadRef : CProducerEnumerator(this))
		{
			utbCurrentThreadRef.reset(new CProducerThread(this, qoQueueObjectsRef, ilCancelableLinkerRef));
		}
	}


	void CancelLinkedWorkItems(waiter &wRefWaiterToBeEngaged, production_size_type &nOutCanceledSum)
	{
		CWorkItemLinker &ilCancelableLinker = GetCancelableLinker();

		CWorkQueueObjects &qoQueueObjectsRef = GetQueueObjects();
		qoQueueObjectsRef.CancelAllLinkedWorkItems(ilCancelableLinker, wRefWaiterToBeEngaged, nOutCanceledSum);
	}


	void DiscardQueuedWorkItemsOrWaitThemToBeHandled(waiter &wRefWaiterToBeEngaged, 
		production_size_type &nOutDroppedSum, production_size_type &nOutWaitedCount)
	{
		CWorkQueueObjects &qoQueueObjectsRef = GetQueueObjects();
		qoQueueObjectsRef.DiscardOrWaitAllWorkItems(wRefWaiterToBeEngaged, nOutDroppedSum, nOutWaitedCount);
	}


	void CommandAllProducersStop()
	{
		for (const auto &utbCurrentThread : CProducerEnumerator(this))
		{
			CProducerThread *pptCurrentInstance = static_cast<CProducerThread *>(&*utbCurrentThread);
			pptCurrentInstance->RequestThreadExit();
		}
	}

	void CommandAllConsumersStop()
	{
		for (const auto &utbCurrentThread : CConsumerEnumerator(this))
		{
			CConsumerThread *pctCurrentInstance = static_cast<CConsumerThread *>(&*utbCurrentThread);
			pctCurrentInstance->RequestThreadExit();
		}
	}

	void WaitAndFreeAllProducers()
	{
		for (auto &utbCurrentThreadRef : CProducerEnumerator(this))
		{
			utbCurrentThreadRef.reset();
		}
	}

	void WaitAndFreeAllConsumers()
	{
		for (auto &utbCurrentThreadRef : CConsumerEnumerator(this))
		{
			utbCurrentThreadRef.reset();
		}
	}


private:
	friend struct CProducerEnumerator;
	friend struct CConsumerEnumerator;

	struct CProducerEnumerator
	{
		CProducerEnumerator(CQueueTester *pqtHostQueueTester) noexcept : m_pqtHostQueueTester(pqtHostQueueTester) {}

		thread_iterator begin() noexcept { return m_pqtHostQueueTester->m_saThreadStorage.begin(); }
		thread_iterator end() noexcept { return m_pqtHostQueueTester->m_saThreadStorage.begin() + m_pqtHostQueueTester->producer_count(); }

		CQueueTester	*m_pqtHostQueueTester;
	};

	struct CConsumerEnumerator
	{
		CConsumerEnumerator(CQueueTester *pqtHostQueueTester) noexcept: m_pqtHostQueueTester(pqtHostQueueTester) {}

		thread_iterator begin() noexcept { return m_pqtHostQueueTester->m_saThreadStorage.begin() + m_pqtHostQueueTester->producer_count(); }
		thread_iterator end() noexcept { return m_pqtHostQueueTester->m_saThreadStorage.end(); }

		CQueueTester	*m_pqtHostQueueTester;
	};

	CWorkQueueObjects &GetQueueObjects() const noexcept { return const_cast<CWorkQueueObjects &>(m_qoQueueObjects); }
	CWorkItemLinker &GetCancelableLinker() const noexcept { return const_cast<CWorkItemLinker &>(m_ilCancelableLinker); }

private:
	CThreadStorageArray		m_saThreadStorage;
	CWorkQueueObjects		m_qoQueueObjects;
	CWorkItemLinker			m_ilCancelableLinker;
	atomic<production_size_type> m_lsiProducedCount;
	atomic<production_size_type> m_lsiConsumedSum;
	atomic<production_size_type> m_lsiAbortedSum;
};


template<class TQueueType, unsigned tuiProducerCount, unsigned tuiConsumerCount>
CQueueTester<TQueueType, tuiProducerCount, tuiConsumerCount>::CQueueTester():
	m_saThreadStorage(),
	m_qoQueueObjects(consumer_count()),
	m_lsiProducedCount(0),
	m_lsiConsumedSum(0),
	m_lsiAbortedSum(0)
{
}

template<class TQueueType, unsigned tuiProducerCount, unsigned tuiConsumerCount>
CQueueTester<TQueueType, tuiProducerCount, tuiConsumerCount>::~CQueueTester()
{
}


template<class TQueueType, unsigned tuiProducerCount, unsigned tuiConsumerCount>
void CQueueTester<TQueueType, tuiProducerCount, tuiConsumerCount>::RunTheTest(CTestStatistics &tsOutTestStatistics)
{
	production_size_type nCanceledSum, nOnDiscardDroppedItemSum, nOnDiscardWaitedItemCount;

	CreateAndStartAllConsumers();
	
	try 
	{
		CreateAndStartAllProducers();

		try
		{
			try
			{
				sleep_for((milliseconds)5000);

				CommandAllProducersStop();
				WaitAndFreeAllProducers();
			}
			catch(...)
			{
				CommandAllProducersStop();
				WaitAndFreeAllProducers();
				throw;
			}

			waiter wQueueFlushWaiter;
			CancelLinkedWorkItems(wQueueFlushWaiter, nCanceledSum);
			DiscardQueuedWorkItemsOrWaitThemToBeHandled(wQueueFlushWaiter, nOnDiscardDroppedItemSum, nOnDiscardWaitedItemCount);
		}
		catch (...)
		{
			waiter wQueueFlushWaiter;
			DiscardQueuedWorkItemsOrWaitThemToBeHandled(wQueueFlushWaiter, nOnDiscardDroppedItemSum, nOnDiscardWaitedItemCount); // The counts are ignored in case of an exception
			throw;
		}

		CommandAllConsumersStop();
		WaitAndFreeAllConsumers();
	}
	catch(...)
	{
		CommandAllConsumersStop();
		WaitAndFreeAllConsumers();
		throw;
	}

	production_size_type siProducedCount = m_lsiProducedCount.load(std::memory_order_relaxed);
	production_size_type siConsumedSum = m_lsiConsumedSum.load(std::memory_order_relaxed);
	production_size_type siAbortedSum = m_lsiAbortedSum.load(std::memory_order_relaxed);

	tsOutTestStatistics.AssignFiedls(siProducedCount, siConsumedSum, siAbortedSum, nCanceledSum, nOnDiscardDroppedItemSum, nOnDiscardWaitedItemCount);
}

template<class TQueueType, unsigned tuiProducerCount, unsigned tuiConsumerCount>
void CQueueTester<TQueueType, tuiProducerCount, tuiConsumerCount>::PrintTestStatisticsIfNecessary(
	production_size_type siProducedCount, production_size_type siConsumedCount, production_size_type siAbortedCount)
{
}

#endif // #if _MGTEST_HAVE_CXX11


//////////////////////////////////////////////////////////////////////////
// The Test Routines

static 
void PerformWaitableQueueTest(CTesterBase::CTestStatistics &tsOutTestStatistics/*=nullptr*/)
{
#if _MGTEST_HAVE_CXX11
	CQueueTester<waitable_queue, NGTEST_CQ_PRODUCER_COUNT, NGTEST_CQ_CONSUMER_COUNT> qtTestInstance;
	qtTestInstance.RunTheTest(tsOutTestStatistics);
#else // #if !_MGTEST_HAVE_CXX11
	tsOutTestStatistics.AssignSkippedState();
#endif // #if !_MGTEST_HAVE_CXX11
}

static 
void PerformCancelableQueueTest(CTesterBase::CTestStatistics &tsOutTestStatistics/*=nullptr*/)
{
#if _MGTEST_HAVE_CXX11
	CQueueTester<cancelable_queue, NGTEST_CQ_PRODUCER_COUNT, NGTEST_CQ_CONSUMER_COUNT> qtTestInstance;
	qtTestInstance.RunTheTest(tsOutTestStatistics);
#else // #if !_MGTEST_HAVE_CXX11
	tsOutTestStatistics.AssignSkippedState();
#endif // #if !_MGTEST_HAVE_CXX11
}

