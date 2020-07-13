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
#include <iterator>
#include <functional>
#include <system_error>


_MUTEXGEAR_BEGIN_NAMESPACE()

_MUTEXGEAR_BEGIN_COMPLETION_NAMESPACE()


class worker:
	private mutexgear_completion_worker_t
{
public:
	typedef mutexgear_completion_worker_t *pointer;

	static worker &instance_from_pointer(pointer pPointerValue) noexcept { return *static_cast<worker *>(pPointerValue); }

	worker()
	{
		int iInitializationResult = mutexgear_completion_worker_init(this, nullptr);

		if (iInitializationResult != EOK)
		{
			throw std::system_error(std::error_code(iInitializationResult, std::system_category()));
		}
	}

	worker(const worker &wAnotherWorker) = delete;

	~worker() noexcept
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

	void unlock() noexcept
	{
		int iCompletionWaiterUnlockResult;
		MG_CHECK(iCompletionWaiterUnlockResult, (iCompletionWaiterUnlockResult = mutexgear_completion_worker_unlock(this)) == EOK);
	}

public:
	operator pointer() noexcept { return static_cast<pointer>(this); }
};

class worker_view
{
public:
	typedef worker::pointer pointer;

	worker_view() noexcept {}
	worker_view(worker::pointer pwpPointerInstance) noexcept : m_pwpWorkerPointer(pwpPointerInstance) {}
	worker_view(worker &wRefWorkerInstance) noexcept : m_pwpWorkerPointer(static_cast<worker::pointer>(wRefWorkerInstance)) {}
	worker_view(const worker_view &wvAnotherInstance) = default;

	worker_view &operator =(worker::pointer pwpPointerInstance) noexcept { m_pwpWorkerPointer = pwpPointerInstance; return *this; }
	worker_view &operator =(const worker_view &wvAnotherInstance) = default;

	void detach() noexcept { m_pwpWorkerPointer = nullptr; } // to be used for debug purposes

	void swap(worker_view &wvRefAnotherInstance) noexcept { std::swap(m_pwpWorkerPointer, wvRefAnotherInstance.m_pwpWorkerPointer); }

	bool operator ==(const worker_view &wvAnotherInstance) const noexcept { return m_pwpWorkerPointer == wvAnotherInstance.m_pwpWorkerPointer; }
	bool operator !=(const worker_view &wvAnotherInstance) const noexcept { return !operator ==(wvAnotherInstance); }

public:
	operator pointer() const noexcept { return m_pwpWorkerPointer; }

private:
	pointer				m_pwpWorkerPointer;
};


class waiter:
	private mutexgear_completion_waiter_t
{
public:
	typedef mutexgear_completion_waiter_t *pointer;

	static waiter &instance_from_pointer(pointer pPointerValue) noexcept { return *static_cast<waiter *>(pPointerValue); }

	waiter()
	{
		int iInitializationResult = mutexgear_completion_waiter_init(this, nullptr);

		if (iInitializationResult != EOK)
		{
			throw std::system_error(std::error_code(iInitializationResult, std::system_category()));
		}
	}
	
	waiter(const waiter &wAnotherWaiter) = delete; 

	~waiter() noexcept
	{
		int iCompletionWaiterDestructionResult;
		MG_CHECK(iCompletionWaiterDestructionResult, (iCompletionWaiterDestructionResult = mutexgear_completion_waiter_destroy(this)) == EOK);
	}

	waiter &operator =(const waiter &wAnotherWaiter) = delete;

	operator pointer() noexcept { return static_cast<pointer>(this); }
};


class item:
	private mutexgear_completion_item_t
{
public:
	typedef mutexgear_completion_item_t *pointer;

	static item &instance_from_pointer(pointer pPointerValue) noexcept { return *static_cast<item *>(pPointerValue); }

	item() noexcept { mutexgear_completion_item_init(this); }
	item(const item &iAnotherItem) = delete;

	~item() noexcept { mutexgear_completion_item_destroy(this); }

	item &operator =(const item &iAnotherItem) = delete;

public:
	void prestart(worker &wRefWorkerToBeEngaged) noexcept { mutexgear_completion_item_prestart(this, static_cast<worker::pointer>(wRefWorkerToBeEngaged)); }
	bool is_started() const noexcept { return mutexgear_completion_item_isstarted(this); }

	bool is_canceled(worker &wRefEngagedWorker) const noexcept { return mutexgear_completion_cancelablequeueditem_iscanceled(this, static_cast<worker::pointer>(wRefEngagedWorker)); }

public:
	operator pointer() noexcept { return static_cast<pointer>(this); }
};

class item_view
{
public:
	typedef item::pointer pointer;

	item_view() noexcept {}
	item_view(item::pointer pipPointerInstance) noexcept : m_pipItemPointer(pipPointerInstance) {}
	item_view(item &iRefItemInstance) noexcept : m_pipItemPointer(static_cast<item::pointer>(iRefItemInstance)) {}
	item_view(const item_view &ivAnotherInstance) = default;

	bool is_null() const noexcept { return m_pipItemPointer == nullptr; }
	bool operator !() const noexcept { return is_null(); }

	void detach() noexcept { m_pipItemPointer = nullptr; } // to be used for debug purposes

	item_view &operator =(item::pointer pipPointerInstance) noexcept { m_pipItemPointer = pipPointerInstance; return *this; }
	item_view &operator =(item &iRefItemInstance) noexcept { m_pipItemPointer = static_cast<item::pointer>(iRefItemInstance); return *this; }
	item_view &operator =(const item_view &ivAnotherInstance) = default;
	void swap(item_view &ivRefAnotherInstance) noexcept { std::swap(m_pipItemPointer, ivRefAnotherInstance.m_pipItemPointer); }

	bool operator ==(const item_view &ivAnotherInstance) const noexcept { return m_pipItemPointer == ivAnotherInstance.m_pipItemPointer; }
	bool operator !=(const item_view &ivAnotherInstance) const noexcept { return !operator ==(ivAnotherInstance); }

public:
	bool is_canceled(worker &wRefEngagedWorker) const noexcept { return mutexgear_completion_cancelablequeueditem_iscanceled(m_pipItemPointer, static_cast<worker::pointer>(wRefEngagedWorker)); }

public:
	operator pointer() const noexcept { return m_pipItemPointer; }

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

	~waitable_queue() noexcept
	{
		int iCompletionQueueDestructionResult;
		MG_CHECK(iCompletionQueueDestructionResult, (iCompletionQueueDestructionResult = mutexgear_completion_queue_destroy(&m_cqQueueInstance)) == EOK);
	}

	waitable_queue &operator =(const waitable_queue &bqAnotherQueue) = delete;

public:
	class const_reverse_iterator;

	class const_iterator
	{
	public:
		typedef item_view value_type;
		typedef std::bidirectional_iterator_tag iterator_category;
		typedef ptrdiff_t difference_type;
		typedef const value_type &reference;
		typedef const value_type *pointer;

		const_iterator() noexcept {}
		const_iterator(const const_iterator &itOtherIterator) noexcept : m_ivIteratorInfo(itOtherIterator.m_ivIteratorInfo) {}
		inline const_iterator(const const_reverse_iterator &itOtherIterator) noexcept;

	protected:
		friend class waitable_queue;

		explicit const_iterator(mutexgear_completion_item_t *pciIteratorInfo) noexcept : m_ivIteratorInfo(pciIteratorInfo) {}

	public:
		const value_type &operator *() const noexcept { return m_ivIteratorInfo; }
		const value_type *operator ->() const noexcept { return &m_ivIteratorInfo; }

		bool operator ==(const const_iterator &itOtherIterator) const noexcept { return m_ivIteratorInfo == itOtherIterator.m_ivIteratorInfo; }
		bool operator !=(const const_iterator &itOtherIterator) const noexcept { return !operator ==(itOtherIterator); }

		const_iterator &operator ++() noexcept { m_ivIteratorInfo = mutexgear_completion_queue_unsafegetunsafenext(static_cast<item_view::pointer>(m_ivIteratorInfo)); return *this; }
		const_iterator operator ++(int) noexcept { item_view ivIteratorInfoCopy = m_ivIteratorInfo; m_ivIteratorInfo = mutexgear_completion_queue_unsafegetunsafenext(static_cast<item_view::pointer>(m_ivIteratorInfo)); return const_iterator(ivIteratorInfoCopy); }
		const_iterator &operator --() noexcept { m_ivIteratorInfo = mutexgear_completion_queue_getunsafepreceding(static_cast<item_view::pointer>(m_ivIteratorInfo)); return *this; }
		const_iterator operator --(int) noexcept { item_view ivIteratorInfoCopy = m_ivIteratorInfo; m_ivIteratorInfo = mutexgear_completion_queue_getunsafepreceding(static_cast<item_view::pointer>(m_ivIteratorInfo)); return const_iterator(ivIteratorInfoCopy); }

		const_iterator &operator =(const const_iterator &itOtherIterator) noexcept { m_ivIteratorInfo = itOtherIterator.m_ivIteratorInfo; return *this; }
		inline const_iterator &operator =(const const_reverse_iterator &itOtherIterator) noexcept;

	private:
		item_view				m_ivIteratorInfo;
	};

	typedef std::reverse_iterator<const_iterator> _const_reverse_iterator_parent;
	class const_reverse_iterator:
		public _const_reverse_iterator_parent
	{
	public:
		const_reverse_iterator() noexcept {}
		const_reverse_iterator(const const_reverse_iterator &itOtherIterator) noexcept : _const_reverse_iterator_parent(itOtherIterator) {}
		const_reverse_iterator(const const_iterator &itOtherIterator) noexcept : _const_reverse_iterator_parent(itOtherIterator) {}
	};

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

	void unlock() noexcept
	{
		int iCompletionQueueUnlockResult;
		MG_CHECK(iCompletionQueueUnlockResult, (iCompletionQueueUnlockResult = mutexgear_completion_queue_plainunlock(&m_cqQueueInstance)) == EOK);
	}

	bool empty() const noexcept { return mutexgear_completion_queue_lodisempty(&m_cqQueueInstance); }

	const_iterator begin() const noexcept
	{
		mutexgear_completion_item_t *pciHeadItem;
		mutexgear_completion_queue_unsafegethead(&pciHeadItem, const_cast<mutexgear_completion_queue_t *>(&m_cqQueueInstance)); // Ignore the result

		return const_iterator(pciHeadItem);
	}

	const_iterator end() const noexcept
	{
		mutexgear_completion_item_t *pciEndItem = mutexgear_completion_queue_getend(const_cast<mutexgear_completion_queue_t *>(&m_cqQueueInstance));
		return const_iterator(pciEndItem);
	}

	const_reverse_iterator rbegin() const noexcept
	{
		mutexgear_completion_item_t *pciEndItem = mutexgear_completion_queue_getend(const_cast<mutexgear_completion_queue_t *>(&m_cqQueueInstance));
		return const_reverse_iterator(const_iterator(pciEndItem));
	}

	const_reverse_iterator rend() const noexcept
	{
		mutexgear_completion_item_t *pciHeadItem;
		mutexgear_completion_queue_unsafegethead(&pciHeadItem, const_cast<mutexgear_completion_queue_t *>(&m_cqQueueInstance)); // Ignore the result

		return const_reverse_iterator(const_iterator(pciHeadItem));
	}

	const item_view &front() const noexcept { return *begin(); }
	const item_view &back() const noexcept { return *rbegin(); }

	void unlock_and_wait(const item_view &ivRefItemToBeWaited, waiter &wRefWaiterToBeEngaged)
	{
		// NOTE: The unlock portion of the call always succeeds
		int iWaitResult = mutexgear_completion_queue_unlockandwait(&m_cqQueueInstance, static_cast<item_view::pointer>(ivRefItemToBeWaited), static_cast<waiter::pointer>(wRefWaiterToBeEngaged));

		if (iWaitResult != EOK)
		{
			throw std::system_error(std::error_code(iWaitResult, std::system_category()));
		}
	}

	void enqueue(item &iRefItemInstance, lock_token_type ltLockToken) noexcept
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
	void dequeue(item &iRefItemInstance) noexcept
	{
		mutexgear_completion_queue_unsafedequeue(static_cast<item::pointer>(iRefItemInstance));
	}

	void start(item &iRefItemInstance, worker &wRefWorkerToBeEngaged) noexcept
	{
		mutexgear_completion_queueditem_start(static_cast<item::pointer>(iRefItemInstance), static_cast<worker::pointer>(wRefWorkerToBeEngaged));
	}

	static 
	void unsafefinish__locked(item &iRefItemInstance) noexcept
	{
		mutexgear_completion_queueditem_unsafefinish__locked(static_cast<item::pointer>(iRefItemInstance));
	}

	void unsafefinish__unlocked(item &iRefItemInstance, worker &wRefEngagedWorker) noexcept
	{
		mutexgear_completion_queueditem_unsafefinish__unlocked(&m_cqQueueInstance, static_cast<item::pointer>(iRefItemInstance), static_cast<worker::pointer>(wRefEngagedWorker));
	}

	void safefinish(item &iRefItemInstance, worker &wRefEngagedWorker)
	{
		int iFinishResult = mutexgear_completion_queueditem_safefinish(&m_cqQueueInstance, static_cast<item::pointer>(iRefItemInstance), static_cast<worker::pointer>(wRefEngagedWorker));

		if (iFinishResult != EOK)
		{
			throw std::system_error(std::error_code(iFinishResult, std::system_category()));
		}
	}

private:
	mutexgear_completion_queue_t	m_cqQueueInstance;
};

/*inline */
waitable_queue::const_iterator::const_iterator(const waitable_queue::const_reverse_iterator &itOtherIterator) noexcept :
	m_ivIteratorInfo(*itOtherIterator)
{
}

/*inline */
waitable_queue::const_iterator &waitable_queue::const_iterator::operator =(const waitable_queue::const_reverse_iterator &itOtherIterator) noexcept
{
	m_ivIteratorInfo = *itOtherIterator;
	return *this;
}


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

	~cancelable_queue() noexcept
	{
		int iCompletionQueueDestructionResult;
		MG_CHECK(iCompletionQueueDestructionResult, (iCompletionQueueDestructionResult = mutexgear_completion_cancelablequeue_destroy(&m_cqQueueInstance)) == EOK);
	}

	cancelable_queue &operator =(const cancelable_queue &cqAnotherQueue) = delete;

public:
	class const_reverse_iterator;

	class const_iterator
	{
	public:
		typedef item_view value_type;
		typedef std::bidirectional_iterator_tag iterator_category;
		typedef ptrdiff_t difference_type;
		typedef const value_type &reference;
		typedef const value_type *pointer;

		const_iterator() noexcept {}
		const_iterator(const const_iterator &itOtherIterator) noexcept : m_ivIteratorInfo(itOtherIterator.m_ivIteratorInfo) {}
		inline const_iterator(const const_reverse_iterator &itOtherIterator) noexcept;

	protected:
		friend class cancelable_queue;

		explicit const_iterator(mutexgear_completion_item_t *pliIteratorInfo) noexcept : m_ivIteratorInfo(pliIteratorInfo) {}

	public:
		const value_type &operator *() const noexcept { return m_ivIteratorInfo; }
		const value_type *operator ->() const noexcept { return &m_ivIteratorInfo; }

		bool operator ==(const const_iterator &itOtherIterator) const noexcept { return m_ivIteratorInfo == itOtherIterator.m_ivIteratorInfo; }
		bool operator !=(const const_iterator &itOtherIterator) const noexcept { return !operator ==(itOtherIterator); }

		const_iterator &operator ++() noexcept { m_ivIteratorInfo = mutexgear_completion_queue_unsafegetunsafenext(static_cast<item_view::pointer>(m_ivIteratorInfo)); return *this; }
		const_iterator operator ++(int) noexcept { item_view ivIteratorInfoCopy = m_ivIteratorInfo; m_ivIteratorInfo = mutexgear_completion_queue_unsafegetunsafenext(static_cast<item_view::pointer>(m_ivIteratorInfo)); return const_iterator(ivIteratorInfoCopy); }
		const_iterator &operator --() noexcept { m_ivIteratorInfo = mutexgear_completion_queue_getunsafepreceding(static_cast<item_view::pointer>(m_ivIteratorInfo)); return *this; }
		const_iterator operator --(int) noexcept { item_view ivIteratorInfoCopy = m_ivIteratorInfo; m_ivIteratorInfo = mutexgear_completion_queue_getunsafepreceding(static_cast<item_view::pointer>(m_ivIteratorInfo)); return const_iterator(ivIteratorInfoCopy); }

		const_iterator &operator =(const const_iterator &itOtherIterator) noexcept { m_ivIteratorInfo = itOtherIterator.m_ivIteratorInfo; return *this; }
		inline const_iterator &operator =(const const_reverse_iterator &itOtherIterator) noexcept;

	private:
		item_view				m_ivIteratorInfo;
	};

	typedef std::reverse_iterator<const_iterator> _const_reverse_iterator_parent;
	class const_reverse_iterator :
		public _const_reverse_iterator_parent
	{
	public:
		const_reverse_iterator() noexcept {}
		const_reverse_iterator(const const_reverse_iterator &itOtherIterator) noexcept : _const_reverse_iterator_parent(itOtherIterator) {}
		const_reverse_iterator(const const_iterator &itOtherIterator) noexcept : _const_reverse_iterator_parent(itOtherIterator) {}
	};

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

	void unlock() noexcept
	{
		int iCompletionQueueUnlockResult;
		MG_CHECK(iCompletionQueueUnlockResult, (iCompletionQueueUnlockResult = mutexgear_completion_cancelablequeue_plainunlock(&m_cqQueueInstance)) == EOK);
	}

	bool empty() const noexcept { return mutexgear_completion_cancelablequeue_lodisempty(&m_cqQueueInstance); }

	const_iterator begin() const noexcept
	{
		mutexgear_completion_item_t *pciHeadItem;
		mutexgear_completion_cancelablequeue_unsafegethead(&pciHeadItem, const_cast<mutexgear_completion_cancelablequeue_t *>(&m_cqQueueInstance)); // Ignore the result

		return const_iterator(pciHeadItem);
	}

	const_iterator end() const noexcept
	{
		mutexgear_completion_item_t *pciEndItem = mutexgear_completion_cancelablequeue_getend(const_cast<mutexgear_completion_cancelablequeue_t *>(&m_cqQueueInstance));
		return const_iterator(pciEndItem);
	}

	const_reverse_iterator rbegin() const noexcept
	{
		mutexgear_completion_item_t *pciEndItem = mutexgear_completion_cancelablequeue_getend(const_cast<mutexgear_completion_cancelablequeue_t *>(&m_cqQueueInstance));
		return const_reverse_iterator(const_iterator(pciEndItem));
	}

	const_reverse_iterator rend() const noexcept
	{
		mutexgear_completion_item_t *pciHeadItem;
		mutexgear_completion_cancelablequeue_unsafegethead(&pciHeadItem, const_cast<mutexgear_completion_cancelablequeue_t *>(&m_cqQueueInstance)); // Ignore the result

		return const_reverse_iterator(const_iterator(pciHeadItem));
	}

	const item_view &front() const noexcept { return *begin(); }
	const item_view &back() const noexcept { return *rbegin(); }

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
		const cancel_callback_type &fnCancelCallback=nullptr) noexcept(false)
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

	void enqueue(item &iRefItemInstance, lock_token_type ltLockToken) noexcept
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
	void dequeue(item &iRefItemInstance) noexcept
	{
		mutexgear_completion_cancelablequeue_unsafedequeue(static_cast<item::pointer>(iRefItemInstance));
	}

// 	item_view start_any(worker &wRefWorkerToBeEngaged, lock_token_type ltLockToken) noexcept
// 	{
// 		MG_ASSERT(ltLockToken != nullptr);
// 
// 		mutexgear_completion_item_t *pciAcquiredItem;
// 		const mutexgear_completion_locktoken_t clQueueLock = ltLockToken;
// 		int iStartResult = mutexgear_completion_cancelablequeue_locateandstart(&pciAcquiredItem, &m_cqQueueInstance, static_cast<worker::pointer>(wRefWorkerToBeEngaged), clQueueLock);
// 		MG_VERIFY(iStartResult == EOK);
// 
// 		return item_view(pciAcquiredItem); // The item may be a nullptr
// 	}
// 
// 	item_view start_any_with_locking(worker &wRefWorkerToBeEngaged)
// 	{
// 		mutexgear_completion_item_t *pciAcquiredItem;
// 		const mutexgear_completion_locktoken_t clQueueLock = nullptr;
// 		int iStartResult = mutexgear_completion_cancelablequeue_locateandstart(&pciAcquiredItem, &m_cqQueueInstance, static_cast<worker::pointer>(wRefWorkerToBeEngaged), clQueueLock);
// 		
// 		if (iStartResult != EOK)
// 		{
// 			throw std::system_error(std::error_code(iStartResult, std::system_category()));
// 		}
// 
// 		return item_view(pciAcquiredItem); // The item may be a nullptr
// 	}

	void start(item &iRefItemInstance, worker &wRefWorkerToBeEngaged) noexcept
	{
		mutexgear_completion_cancelablequeueditem_start(static_cast<item::pointer>(iRefItemInstance), static_cast<worker::pointer>(wRefWorkerToBeEngaged));
	}

	static 
	void unsafefinish__locked(item &iRefItemInstance) noexcept
	{
		mutexgear_completion_cancelablequeueditem_unsafefinish__locked(static_cast<item::pointer>(iRefItemInstance));
	}

	void unsafefinish__unlocked(item &iRefItemInstance, worker &wRefEngagedWorker) noexcept
	{
		mutexgear_completion_cancelablequeueditem_unsafefinish__unlocked(&m_cqQueueInstance, static_cast<item::pointer>(iRefItemInstance), static_cast<worker::pointer>(wRefEngagedWorker));
	}

	void safefinish(item &iRefItemInstance, worker &wRefEngagedWorker)
	{
		int iFinishResult = mutexgear_completion_cancelablequeueditem_safefinish(&m_cqQueueInstance, static_cast<item::pointer>(iRefItemInstance), static_cast<worker::pointer>(wRefEngagedWorker));

		if (iFinishResult != EOK)
		{
			throw std::system_error(std::error_code(iFinishResult, std::system_category()));
		}
	}

private:
	static ownership_type ConvertMutexgearCompletionOwnershipToOwnershipType(mutexgear_completion_ownership_t coItemOwnership) noexcept
	{
		MG_STATIC_ASSERT(mg_completion_ownership__max + 0 == ownership__max);
		MG_STATIC_ASSERT(mg_completion_ownership__max == 2);
		MG_STATIC_ASSERT(mg_completion_not_owner + 0 == not_owner);
		MG_STATIC_ASSERT(mg_completion_owner + 0 == owner);

		return static_cast<ownership_type>(coItemOwnership);
	}

	struct cancel_context_type
	{
		void assign(cancelable_queue *pcqQueueInstance, const cancel_callback_type &fnCancelCallback) noexcept
		{
			m_pcqQueueInstance = pcqQueueInstance;
			m_pfnCancelCallback = &fnCancelCallback;
		}

		cancelable_queue		*m_pcqQueueInstance;
		const cancel_callback_type *m_pfnCancelCallback;
	};

	static void HandleItemCancel(void *psvCancelContext, mutexgear_completion_cancelablequeue_t *pcqQueueInstance, 
		mutexgear_completion_worker_t *pscWorkerInstance, mutexgear_completion_item_t *pciItemInstance)/* noexcept(false)*/
	{
		const cancel_context_type *pccCancelContext = reinterpret_cast<cancel_context_type *>(psvCancelContext);
		MG_ASSERT(&pccCancelContext->m_pcqQueueInstance->m_cqQueueInstance == pcqQueueInstance);

		(*pccCancelContext->m_pfnCancelCallback)(*pccCancelContext->m_pcqQueueInstance, pscWorkerInstance, pciItemInstance);
	}

private:
	mutexgear_completion_cancelablequeue_t	m_cqQueueInstance;
};

/*inline */
cancelable_queue::const_iterator::const_iterator(const cancelable_queue::const_reverse_iterator &itOtherIterator) noexcept :
	m_ivIteratorInfo(*itOtherIterator)
{
}

/*inline */
cancelable_queue::const_iterator &cancelable_queue::const_iterator::operator =(const cancelable_queue::const_reverse_iterator &itOtherIterator) noexcept
{
	m_ivIteratorInfo = *itOtherIterator;
	return *this;
}


_MUTEXGEAR_END_COMPLETION_NAMESPACE();

_MUTEXGEAR_END_NAMESPACE();


#endif // #ifndef __MUTEXGEAR_COMPLETION_HPP_INCLUDED
