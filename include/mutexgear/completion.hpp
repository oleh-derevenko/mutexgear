#ifndef __MUTEXGEAR_COMPLETION_HPP_INCLUDED
#define __MUTEXGEAR_COMPLETION_HPP_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear Completion Class Definitions                               */
/*                                                                      */
/* WARNING!                                                             */
/* This library contains a synchronization technique protected by       */
/* the U.S. Patent 9,983,913.                                           */
/*                                                                      */
/* THIS IS A PRE-RELEASE LIBRARY SNAPSHOT FOR EVALUATION PURPOSES ONLY. */
/* AWAIT THE RELEASE AT https://mutexgear.com                           */
/*                                                                      */
/* Copyright (c) 2016-2020 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/

/**
 *	\file
 *	\brief MutexGear Completion class set definitions
 *
 *	The header defines a namespace with a set of classes being header-only wrappers
 *  for \c mutexgear_completion_*_t objects.
 *
 *	The \c waitable_queue wrapper class implements a multi-threaded server work queue 
 *	with ability to wait for work item completion. The \c cancelable_queue wrapper class
 *	additionally allows to request and conduct work item handling cancellation.
 *
 *	NOTE:
 *
 *	The \c mutexgear_completion_worker_t object (and hence, the \c completion::worker wrapper) 
 *	depends on a synchronization mechanism being a subject of the U.S. Patent No. 9983913. 
 *	Use USPTO Patent Full-Text and Image Database search (currently, 
 *	http://patft.uspto.gov/netahtml/PTO/search-adv.htm) to view the patent text.
 */


#include <mutexgear/completion.h>
#include <functional>
#include <system_error>


_MUTEXGEAR_BEGIN_NAMESPACE()

_MUTEXGEAR_BEGIN_COMPLETION_NAMESPACE()


class worker:
	private mutexgear_completion_worker_t
{
public:
	typedef mutexgear_completion_worker_t *pointer;

	static worker &instance_from_pointer(pointer pPointerValue) { return *static_cast<worker *>(pPointerValue); }

	worker()
	{
		int iInitializationResult = mutexgear_completion_worker_init(this, nullptr);

		if (iInitializationResult != EOK)
		{
			throw std::system_error(std::error_code(iInitializationResult, std::system_category()));
		}
	}

	worker(const worker &wAnotherWorker) = delete;

	~worker()
	{
		int iCompletionWaiterDestructionResult;
		MG_CHECK(iCompletionWaiterDestructionResult, (iCompletionWaiterDestructionResult = mutexgear_completion_worker_destroy(this)) == EOK);
	}

	worker &operator =(const worker &wAnotherWorker) = delete;

public:
	void lock()
	{
		int iLockResult = mutexgear_completion_worker_lock(this);

		if (iLockResult != EOK)
		{
			throw std::system_error(std::error_code(iLockResult, std::system_category()));
		}
	}

	void unlock()
	{
		int iCompletionWaiterUnlockResult;
		MG_CHECK(iCompletionWaiterUnlockResult, (iCompletionWaiterUnlockResult = mutexgear_completion_worker_unlock(this)) == EOK);
	}

public:
	operator pointer() { return static_cast<pointer>(this); }
};

class worker_view
{
public:
	typedef worker::pointer pointer;

	worker_view() {}
	worker_view(worker::pointer pwpPointerInstance) : m_pwpWorkerPointer(pwpPointerInstance) {}
	worker_view(worker &wRefWorkerInstance) : m_pwpWorkerPointer(static_cast<worker::pointer>(wRefWorkerInstance)) {}
	worker_view(const worker_view &wvAnotherInstance) = default;

	worker_view &operator =(worker::pointer pwpPointerInstance) { m_pwpWorkerPointer = pwpPointerInstance; return *this; }
	worker_view &operator =(const worker_view &wvAnotherInstance) = default;

	void detach() { m_pwpWorkerPointer = nullptr; } // to be used for debug purposes

	void swap(worker_view &wvRefAnotherInstance) { std::swap(m_pwpWorkerPointer, wvRefAnotherInstance.m_pwpWorkerPointer); }

	bool operator ==(const worker_view &wvAnotherInstance) const { return m_pwpWorkerPointer == wvAnotherInstance.m_pwpWorkerPointer; }
	bool operator !=(const worker_view &wvAnotherInstance) const { return !operator ==(wvAnotherInstance); }

public:
	operator pointer() const { return m_pwpWorkerPointer; }

private:
	pointer				m_pwpWorkerPointer;
};


class waiter:
	private mutexgear_completion_waiter_t
{
public:
	typedef mutexgear_completion_waiter_t *pointer;

	static waiter &instance_from_pointer(pointer pPointerValue) { return *static_cast<waiter *>(pPointerValue); }

	waiter()
	{
		int iInitializationResult = mutexgear_completion_waiter_init(this, nullptr);

		if (iInitializationResult != EOK)
		{
			throw std::system_error(std::error_code(iInitializationResult, std::system_category()));
		}
	}
	
	waiter(const waiter &wAnotherWaiter) = delete; 

	~waiter()
	{
		int iCompletionWaiterDestructionResult;
		MG_CHECK(iCompletionWaiterDestructionResult, (iCompletionWaiterDestructionResult = mutexgear_completion_waiter_destroy(this)) == EOK);
	}

	waiter &operator =(const waiter &wAnotherWaiter) = delete;

	operator pointer() { return static_cast<pointer>(this); }
};


class item:
	private mutexgear_completion_item_t
{
public:
	typedef mutexgear_completion_item_t *pointer;

	static item &instance_from_pointer(pointer pPointerValue) { return *static_cast<item *>(pPointerValue); }

	item() { mutexgear_completion_item_init(this); }
	item(const item &iAnotherItem) = delete;

	~item() { mutexgear_completion_item_destroy(this); }

	item &operator =(const item &iAnotherItem) = delete;

public:
	void prestart(worker &wRefOwnerWorker) { mutexgear_completion_item_prestart(this, static_cast<worker::pointer>(wRefOwnerWorker)); }
	
	bool is_canceled(worker &wRefEngagedWorker) const { return mutexgear_completion_cancelablequeueditem_iscanceled(this, static_cast<worker::pointer>(wRefEngagedWorker)); }

public:
	operator pointer() { return static_cast<pointer>(this); }
};

class item_view
{
public:
	typedef item::pointer pointer;

	item_view() {}
	item_view(item::pointer pipPointerInstance): m_pipItemPointer(pipPointerInstance) {}
	item_view(item &iRefItemInstance): m_pipItemPointer(static_cast<item::pointer>(iRefItemInstance)) {}
	item_view(const item_view &ivAnotherInstance) = default;

	item_view &operator =(item::pointer pipPointerInstance) { m_pipItemPointer = pipPointerInstance; return *this; }
	item_view &operator =(const item_view &ivAnotherInstance) = default;

	bool is_null() const { return m_pipItemPointer == nullptr; }
	bool operator !() const { return is_null(); }

	void detach() { m_pipItemPointer = nullptr; } // to be used for debug purposes

	void swap(item_view &ivRefAnotherInstance) { std::swap(m_pipItemPointer, ivRefAnotherInstance.m_pipItemPointer); }

	bool operator ==(const item_view &ivAnotherInstance) const { return m_pipItemPointer == ivAnotherInstance.m_pipItemPointer; }
	bool operator !=(const item_view &ivAnotherInstance) const { return !operator ==(ivAnotherInstance); }

public:
	bool is_canceled(worker &wRefEngagedWorker) const { return mutexgear_completion_cancelablequeueditem_iscanceled(m_pipItemPointer, static_cast<worker::pointer>(wRefEngagedWorker)); }

public:
	operator pointer() const { return m_pipItemPointer; }

private:
	pointer			m_pipItemPointer;
};


class waitable_queue
{
public:
	waitable_queue()
	{
		int iInitializationResult = mutexgear_completion_queue_init(&m_cqQueueInstance, nullptr);

		if (iInitializationResult != EOK)
		{
			throw std::system_error(std::error_code(iInitializationResult, std::system_category()));
		}
	}

	waitable_queue(const waitable_queue &bqAnotherInstance) = delete;

	~waitable_queue()
	{
		int iCompletionQueueDestructionResult;
		MG_CHECK(iCompletionQueueDestructionResult, (iCompletionQueueDestructionResult = mutexgear_completion_queue_destroy(&m_cqQueueInstance)) == EOK);
	}

	waitable_queue &operator =(const waitable_queue &bqAnotherQueue) = delete;

public:
	typedef mutexgear_completion_locktoken_t lock_token_type;

	void lock(lock_token_type *pltOutLockToken=nullptr)
	{
		mutexgear_completion_locktoken_t *pclAcquiredLockTockenToUse = pltOutLockToken;
		int iLockResult = mutexgear_completion_queue_lock(pclAcquiredLockTockenToUse, &m_cqQueueInstance);

		if (iLockResult != EOK)
		{
			throw std::system_error(std::error_code(iLockResult, std::system_category()));
		}
	}

	void unlock()
	{
		int iCompletionQueueUnlockResult;
		MG_CHECK(iCompletionQueueUnlockResult, (iCompletionQueueUnlockResult = mutexgear_completion_queue_plainunlock(&m_cqQueueInstance)) == EOK);
	}

	bool empty() const { return mutexgear_completion_queue_lodisempty(&m_cqQueueInstance); }

	item_view front()
	{
		mutexgear_completion_item_t *pciHeadItem = nullptr;
		bool bRetrievalResult = mutexgear_completion_queue_unsafegethead(&pciHeadItem, &m_cqQueueInstance);
		MG_VERIFY(bRetrievalResult);

		return item_view(pciHeadItem);
	}

	void unlock_and_wait(const item_view &ivRefItemToBeWaited, waiter &wRefWaiterToBeEngaged)
	{
		// NOTE: The unlock portion of the call always succeeds
		int iWaitResult = mutexgear_completion_queue_unlockandwait(&m_cqQueueInstance, static_cast<item_view::pointer>(ivRefItemToBeWaited), static_cast<waiter::pointer>(wRefWaiterToBeEngaged));

		if (iWaitResult != EOK)
		{
			throw std::system_error(std::error_code(iWaitResult, std::system_category()));
		}
	}

	void enqueue(item &iRefItemInstance, lock_token_type ltLockToken)
	{
		MG_ASSERT(ltLockToken != nullptr);

		const mutexgear_completion_locktoken_t clQueueLock = ltLockToken;
		int iEnqueueResult = mutexgear_completion_queue_enqueue(&m_cqQueueInstance, static_cast<item::pointer>(iRefItemInstance), clQueueLock);
		MG_VERIFY(iEnqueueResult == EOK);
	}

	void enqueue_with_locking(item &iRefItemInstance)
	{
		const mutexgear_completion_locktoken_t clQueueLock = nullptr;
		int iEnqueueResult = mutexgear_completion_queue_enqueue(&m_cqQueueInstance, static_cast<item::pointer>(iRefItemInstance), clQueueLock);

		if (iEnqueueResult != EOK)
		{
			throw std::system_error(std::error_code(iEnqueueResult, std::system_category()));
		}
	}

	static 
	void dequeue(item &iRefItemInstance)
	{
		mutexgear_completion_queue_unsafedequeue(static_cast<item::pointer>(iRefItemInstance));
	}

	void finish(item &iRefItemInstance, worker &wRefEngagedWorker, lock_token_type ltLockToken)
	{
		MG_ASSERT(ltLockToken != nullptr);

		const mutexgear_completion_locktoken_t clQueueLock = ltLockToken;
		int iFinishResult = mutexgear_completion_queueditem_finish(&m_cqQueueInstance, static_cast<item::pointer>(iRefItemInstance), static_cast<worker::pointer>(wRefEngagedWorker), clQueueLock);
		MG_VERIFY(iFinishResult == EOK);
	}

	void finish_with_locking(item &iRefItemInstance, worker &wRefEngagedWorker)
	{
		const mutexgear_completion_locktoken_t clQueueLock = nullptr;
		int iFinishResult = mutexgear_completion_queueditem_finish(&m_cqQueueInstance, static_cast<item::pointer>(iRefItemInstance), static_cast<worker::pointer>(wRefEngagedWorker), clQueueLock);

		if (iFinishResult != EOK)
		{
			throw std::system_error(std::error_code(iFinishResult, std::system_category()));
		}
	}

private:
	mutexgear_completion_queue_t	m_cqQueueInstance;
};


class cancelable_queue
{
public:
	cancelable_queue()
	{
		int iInitializationResult = mutexgear_completion_cancelablequeue_init(&m_cqQueueInstance, nullptr);

		if (iInitializationResult != EOK)
		{
			throw std::system_error(std::error_code(iInitializationResult, std::system_category()));
		}
	}

	cancelable_queue(const cancelable_queue &cqAnotherInstance) = delete;

	~cancelable_queue()
	{
		int iCompletionQueueDestructionResult;
		MG_CHECK(iCompletionQueueDestructionResult, (iCompletionQueueDestructionResult = mutexgear_completion_cancelablequeue_destroy(&m_cqQueueInstance)) == EOK);
	}

	cancelable_queue &operator =(const cancelable_queue &cqAnotherQueue) = delete;

public:
	typedef mutexgear_completion_locktoken_t lock_token_type;

	void lock(lock_token_type *pltOutLockToken=nullptr)
	{
		mutexgear_completion_locktoken_t *pclAcquiredLockTockenToUse = pltOutLockToken;
		int iLockResult = mutexgear_completion_cancelablequeue_lock(pclAcquiredLockTockenToUse, &m_cqQueueInstance);

		if (iLockResult != EOK)
		{
			throw std::system_error(std::error_code(iLockResult, std::system_category()));
		}
	}

	void unlock()
	{
		int iCompletionQueueUnlockResult;
		MG_CHECK(iCompletionQueueUnlockResult, (iCompletionQueueUnlockResult = mutexgear_completion_cancelablequeue_plainunlock(&m_cqQueueInstance)) == EOK);
	}

	bool empty() const { return mutexgear_completion_cancelablequeue_lodisempty(&m_cqQueueInstance); }

	item_view front()
	{
		mutexgear_completion_item_t *pciHeadItem = nullptr;
		bool bRetrievalResult = mutexgear_completion_cancelablequeue_unsafegethead(&pciHeadItem, &m_cqQueueInstance);
		MG_VERIFY(bRetrievalResult);

		return item_view(pciHeadItem);
	}

	void unlock_and_wait(const item_view &ivRefItemToBeWaited, waiter &wRefWaiterToBeEngaged)
	{
		// NOTE: The unlock portion of the call always succeeds
		int iWaitResult = mutexgear_completion_cancelablequeue_unlockandwait(&m_cqQueueInstance, static_cast<item_view::pointer>(ivRefItemToBeWaited), static_cast<waiter::pointer>(wRefWaiterToBeEngaged));

		if (iWaitResult != EOK)
		{
			throw std::system_error(std::error_code(iWaitResult, std::system_category()));
		}
	}

	enum ownership_type
	{
		ownership__min,

		not_owner = ownership__min,
		owner,

		ownership__max,
	};

	typedef std::function<void(cancelable_queue &, const worker_view &, const item_view &)> cancel_callback_type;

	void unlock_and_cancel(const item_view &ivRefItemToBeWaited, waiter &wRefWaiterToBeEngaged, ownership_type &oOutResultingItemOwnership,
		const cancel_callback_type &fnCancelCallback=nullptr)
	{
		cancel_context_type ccCancelContext;
		mutexgear_completion_cancel_fn_t fnItemCancelFunctionToUse = NULL;

		mutexgear_completion_ownership_t coItemOwnership;
		
		if (fnCancelCallback)
		{
			ccCancelContext.assign(this, fnCancelCallback);
			fnItemCancelFunctionToUse = &HandleItemCancel;
		}

		int iWaitResult = mutexgear_completion_cancelablequeue_unlockandcancel(&m_cqQueueInstance,
			static_cast<item_view::pointer>(ivRefItemToBeWaited), static_cast<waiter::pointer>(wRefWaiterToBeEngaged),
			fnItemCancelFunctionToUse, reinterpret_cast<void *>(&ccCancelContext), &coItemOwnership);

		if (iWaitResult != EOK)
		{
			throw std::system_error(std::error_code(iWaitResult, std::system_category()));
		}

		oOutResultingItemOwnership = ConvertMutexgearCompletionOwnershipToOwnershipType(coItemOwnership);
	}

	void enqueue(item &iRefItemInstance, lock_token_type ltLockToken)
	{
		MG_ASSERT(ltLockToken != nullptr);

		const mutexgear_completion_locktoken_t clQueueLock = ltLockToken;
		int iEnqueueResult = mutexgear_completion_cancelablequeue_enqueue(&m_cqQueueInstance, static_cast<item::pointer>(iRefItemInstance), clQueueLock);
		MG_VERIFY(iEnqueueResult == EOK);
	}

	void enqueue_with_locking(item &iRefItemInstance)
	{
		const mutexgear_completion_locktoken_t clQueueLock = nullptr;
		int iEnqueueResult = mutexgear_completion_cancelablequeue_enqueue(&m_cqQueueInstance, static_cast<item::pointer>(iRefItemInstance), clQueueLock);

		if (iEnqueueResult != EOK)
		{
			throw std::system_error(std::error_code(iEnqueueResult, std::system_category()));
		}
	}

	static
	void dequeue(item &iRefItemInstance)
	{
		mutexgear_completion_cancelablequeue_unsafedequeue(static_cast<item::pointer>(iRefItemInstance));
	}

	item_view start(worker &wRefWorkerToBeEngaged, lock_token_type ltLockToken)
	{
		MG_ASSERT(ltLockToken != nullptr);

		mutexgear_completion_item_t *pciAcquiredItem;
		const mutexgear_completion_locktoken_t clQueueLock = ltLockToken;
		int iStartResult = mutexgear_completion_cancelablequeueditem_start(&pciAcquiredItem, &m_cqQueueInstance, static_cast<worker::pointer>(wRefWorkerToBeEngaged), clQueueLock);
		MG_VERIFY(iStartResult == EOK);

		return item_view(pciAcquiredItem); // The item may be a nullptr
	}

	item_view start_with_locking(worker &wRefWorkerToBeEngaged)
	{
		mutexgear_completion_item_t *pciAcquiredItem;
		const mutexgear_completion_locktoken_t clQueueLock = nullptr;
		int iStartResult = mutexgear_completion_cancelablequeueditem_start(&pciAcquiredItem, &m_cqQueueInstance, static_cast<worker::pointer>(wRefWorkerToBeEngaged), clQueueLock);
		
		if (iStartResult != EOK)
		{
			throw std::system_error(std::error_code(iStartResult, std::system_category()));
		}

		return item_view(pciAcquiredItem); // The item may be a nullptr
	}

	void finish(item &iRefItemInstance, worker &wRefEngagedWorker, lock_token_type ltLockToken)
	{
		MG_ASSERT(ltLockToken != nullptr);

		const mutexgear_completion_locktoken_t clQueueLock = ltLockToken;
		int iFinishResult = mutexgear_completion_cancelablequeueditem_finish(&m_cqQueueInstance, static_cast<item::pointer>(iRefItemInstance), static_cast<worker::pointer>(wRefEngagedWorker), clQueueLock);
		MG_VERIFY(iFinishResult == EOK);
	}

	void finish_with_locking(item &iRefItemInstance, worker &wRefEngagedWorker)
	{
		const mutexgear_completion_locktoken_t clQueueLock = nullptr;
		int iFinishResult = mutexgear_completion_cancelablequeueditem_finish(&m_cqQueueInstance, static_cast<item::pointer>(iRefItemInstance), static_cast<worker::pointer>(wRefEngagedWorker), clQueueLock);

		if (iFinishResult != EOK)
		{
			throw std::system_error(std::error_code(iFinishResult, std::system_category()));
		}
	}

private:
	static ownership_type ConvertMutexgearCompletionOwnershipToOwnershipType(mutexgear_completion_ownership_t coItemOwnership)
	{
		MG_STATIC_ASSERT(mg_completion_ownership__max + 0 == ownership__max);
		MG_STATIC_ASSERT(mg_completion_ownership__max == 2);
		MG_STATIC_ASSERT(mg_completion_not_owner + 0 == not_owner);
		MG_STATIC_ASSERT(mg_completion_owner + 0 == owner);

		return static_cast<ownership_type>(coItemOwnership);
	}

	struct cancel_context_type
	{
		void assign(cancelable_queue *pcqQueueInstance, const cancel_callback_type &fnCancelCallback)
		{
			m_pcqQueueInstance = pcqQueueInstance;
			m_pfnCancelCallback = &fnCancelCallback;
		}

		cancelable_queue		*m_pcqQueueInstance;
		const cancel_callback_type *m_pfnCancelCallback;
	};

	static void HandleItemCancel(void *psvCancelContext, mutexgear_completion_cancelablequeue_t *pcqQueueInstance, 
		mutexgear_completion_worker_t *pscWorkerInstance, mutexgear_completion_item_t *pciItemInstance)
	{
		const cancel_context_type *pccCancelContext = reinterpret_cast<cancel_context_type *>(psvCancelContext);
		MG_ASSERT(&pccCancelContext->m_pcqQueueInstance->m_cqQueueInstance == pcqQueueInstance);

		(*pccCancelContext->m_pfnCancelCallback)(*pccCancelContext->m_pcqQueueInstance, pscWorkerInstance, pciItemInstance);
	}

private:
	mutexgear_completion_cancelablequeue_t	m_cqQueueInstance;
};


_MUTEXGEAR_END_COMPLETION_NAMESPACE();

_MUTEXGEAR_END_NAMESPACE();


#endif // #ifndef __MUTEXGEAR_COMPLETION_HPP_INCLUDED
