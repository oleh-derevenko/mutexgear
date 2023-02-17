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
/* THIS IS A PRE-RELEASE LIBRARY SNAPSHOT.                              */
/* AWAIT THE RELEASE AT https://mutexgear.com                           */
/*                                                                      */
/* Copyright (c) 2016-2023 Oleh Derevenko. All rights are reserved.     */
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
#include <mutex>
#include <functional>
#include <system_error>
// #include <assert.h>


_MUTEXGEAR_BEGIN_NAMESPACE()

/**
 *	\namespace completion
 *	\brief A namespace to group completion queues and related classes.
 */
_MUTEXGEAR_BEGIN_COMPLETION_NAMESPACE()


/**
 *	\class worker
 *	\brief A wrapper for \c mutexgear_completion_worker_t and its related functions.
 *
 *	The class is a helper for active side handling items in a completion queue like \c waitable_queue or \c cancelable_queue.
 *
 *	\see mutexgear_completion_worker_t
 *	\see waitable_queue
 *	\see cancelable_queue
 */
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

/**
 *	\class worker_view
 *	\brief A wrapper for \c worker data pointer.
 *
 *	The class wraps a reference for accessing and handling a \worker data structure.
 *
 *	\see worker
 *	\see mutexgear_completion_worker_t
 */
class worker_view
{
public:
	typedef worker::pointer pointer;

	worker_view() noexcept {}
	worker_view(worker::pointer pwpPointerInstance) noexcept : m_pwpWorkerPointer(pwpPointerInstance) {}
	worker_view(worker &wRefWorkerInstance) noexcept : m_pwpWorkerPointer(static_cast<worker::pointer>(wRefWorkerInstance)) {}
	worker_view(const worker_view &wvAnotherInstance) = default;

	bool is_null() const noexcept { return m_pwpWorkerPointer == nullptr; }
	bool operator !() const noexcept { return is_null(); }

	void detach() noexcept { m_pwpWorkerPointer = nullptr; } // to be used for debug purposes

	worker_view &operator =(worker::pointer pwpPointerInstance) noexcept { m_pwpWorkerPointer = pwpPointerInstance; return *this; }
	worker_view &operator =(worker &wRefWorkerInstance) noexcept { m_pwpWorkerPointer = static_cast<worker::pointer>(wRefWorkerInstance); return *this; }
	worker_view &operator =(const worker_view &wvAnotherInstance) = default;
	void swap(worker_view &wvRefAnotherInstance) noexcept { std::swap(m_pwpWorkerPointer, wvRefAnotherInstance.m_pwpWorkerPointer); }

	bool operator ==(const worker_view &wvAnotherInstance) const noexcept { return m_pwpWorkerPointer == wvAnotherInstance.m_pwpWorkerPointer; }
	bool operator !=(const worker_view &wvAnotherInstance) const noexcept { return !operator ==(wvAnotherInstance); }

public:
	operator pointer() const noexcept { return m_pwpWorkerPointer; }

private:
	pointer				m_pwpWorkerPointer;
};


/**
 *	\class waiter
 *	\brief A wrapper for \c mutexgear_completion_waiter_t and its related functions.
 *
 *	The class is a helper for client side scheduling items into a completion queue, like \c waitable_queue or \c cancelable_queue, and waiting for them to be processed there.
 *
 *	\see mutexgear_completion_waiter_t
 *	\see waitable_queue
 *	\see cancelable_queue
 */
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


/**
 *	\class item
 *	\brief A wrapper for \c mutexgear_completion_item_t and its related functions.
 *
 *	The class is an item that can be inserted into a completion queue, like \c waitable_queue or \c cancelable_queue, 
 *	to act as a reference for accessing the item's host object for processing.
 *
 *	\see mutexgear_completion_item_t
 *	\see waitable_queue
 *	\see cancelable_queue
 */
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
	worker_view get_worker() const noexcept { return worker_view(mutexgear_completion_item_getworker(this)); }

	bool is_canceled(worker &wRefEngagedWorker) const noexcept { return mutexgear_completion_cancelablequeueditem_iscanceled(this, static_cast<worker::pointer>(wRefEngagedWorker)); }

public:
	operator pointer() noexcept { return static_cast<pointer>(this); }
};

/**
 *	\class item_view
 *	\brief A wrapper for \c item data pointer.
 *
 *	The class wraps a reference for accessing and handling an \c item data structure.
 *
 *	\see item
 *	\see mutexgear_completion_item_t
 */
class item_view
{
public:
	typedef item::pointer pointer;

	item_view() noexcept {}
	item_view(pointer pipPointerInstance) noexcept : m_pipItemPointer(pipPointerInstance) {}
	item_view(item &iRefItemInstance) noexcept : m_pipItemPointer(static_cast<item::pointer>(iRefItemInstance)) {}
	item_view(const item_view &ivAnotherInstance) = default;

	bool is_null() const noexcept { return m_pipItemPointer == nullptr; }
	bool operator !() const noexcept { return is_null(); }

	void detach() noexcept { m_pipItemPointer = nullptr; } // to be used for debug purposes

	item_view &operator =(pointer pipPointerInstance) noexcept { m_pipItemPointer = pipPointerInstance; return *this; }
	item_view &operator =(item &iRefItemInstance) noexcept { m_pipItemPointer = static_cast<item::pointer>(iRefItemInstance); return *this; }
	item_view &operator =(const item_view &ivAnotherInstance) = default;
	void swap(item_view &ivRefAnotherInstance) noexcept { std::swap(m_pipItemPointer, ivRefAnotherInstance.m_pipItemPointer); }

	bool operator ==(const item_view &ivAnotherInstance) const noexcept { return m_pipItemPointer == ivAnotherInstance.m_pipItemPointer; }
	bool operator !=(const item_view &ivAnotherInstance) const noexcept { return !operator ==(ivAnotherInstance); }

public:
	bool is_started() const noexcept { return mutexgear_completion_item_isstarted(m_pipItemPointer); }
	worker_view get_worker() const noexcept { return worker_view(mutexgear_completion_item_getworker(m_pipItemPointer)); }

	bool is_canceled(worker &wRefEngagedWorker) const noexcept { return mutexgear_completion_cancelablequeueditem_iscanceled(m_pipItemPointer, static_cast<worker::pointer>(wRefEngagedWorker)); }

public:
	operator pointer() const noexcept { return m_pipItemPointer; }

private:
	pointer			m_pipItemPointer;
};


/**
 *	\class waitable_queue
 *	\brief A wrapper for \c mutexgear_completion_queue_t and its related functions.
 *
 *	The class implements a threading-aware queue that can be used for scheduling work
 *	items and letting them to be handled asynchronously with threads while allowing the clients 
 *	to request waits for particular item handling completions.
 *
 *	\see mutexgear_completion_queue_t
 */
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

	item_view front() const noexcept { return *begin(); }
	item_view back() const noexcept { return *rbegin(); }

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
	void dequeue(const item_view &ivItemInstance) noexcept
	{
		mutexgear_completion_queue_unsafedequeue(static_cast<item_view::pointer>(ivItemInstance));
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


/**
 *	\class cancelable_queue
 *	\brief A wrapper for \c mutexgear_completion_cancelablequeue_t and its related functions.
 *
 *	The class implements a threading-aware queue that can be used for scheduling work
 *	items and letting them to be handled asynchronously with threads while allowing the clients
 *	to request waits for particular item handling completions and request canceling and safe removal
 *	of the items that no longer need the processing.
 *
 *	\see mutexgear_completion_queue_t
 */
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

	item_view front() const noexcept { return *begin(); }
	item_view back() const noexcept { return *rbegin(); }

	void unlock_and_wait(const item_view &ivRefItemToBeWaited, waiter &wRefWaiterToBeEngaged)
	{
		// NOTE: The unlock portion of the call always succeeds
		int iWaitResult = mutexgear_completion_cancelablequeue_unlockandwait(&m_cqQueueInstance, static_cast<item_view::pointer>(ivRefItemToBeWaited), static_cast<waiter::pointer>(wRefWaiterToBeEngaged));

		if (iWaitResult != EOK)
		{
			throw std::system_error(std::error_code(iWaitResult, std::system_category()));
		}
	}

	enum class ownership_type
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
	void dequeue(const item_view &ivItemInstance) noexcept
	{
		mutexgear_completion_cancelablequeue_unsafedequeue(static_cast<item_view::pointer>(ivItemInstance));
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
		MG_STATIC_ASSERT(mg_completion_ownership__max == static_cast<int>(ownership_type::ownership__max));
		MG_STATIC_ASSERT(mg_completion_ownership__max == 2);
		MG_STATIC_ASSERT(mg_completion_not_owner == static_cast<int>(ownership_type::not_owner));
		MG_STATIC_ASSERT(mg_completion_owner == static_cast<int>(ownership_type::owner));

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


struct acquire_tocken_t { explicit acquire_tocken_t() noexcept = default; };

/**
 *	\template queue_lock_helper
 *	\brief A template class to aid safe locking of completion queues
 *
 *	\see waitable_queue
 *	\see cancelable_queue
 */
template<class TQueueType, bool tsbCancelAvailability=std::is_same<TQueueType, cancelable_queue>::value>
class queue_lock_helper;

template<class TQueueType>
class queue_lock_helper<TQueueType, false>
{
public:
	typedef TQueueType queue_type;
	typedef typename queue_type::lock_token_type lock_token_type;
	
	explicit queue_lock_helper(queue_type &qQueueInstance):
		m_psqQueueInstance(&qQueueInstance),
		m_bQueueIsLocked(true),
		m_bTokenIsAvailable(false),
		m_ltLockToken()
	{
		qQueueInstance.lock();
	}

	explicit queue_lock_helper(queue_type &qQueueInstance, acquire_tocken_t):
		m_psqQueueInstance(&qQueueInstance),
		m_bQueueIsLocked(true), 
		m_bTokenIsAvailable(true)
	{
		qQueueInstance.lock(&m_ltLockToken);
	}

	queue_lock_helper(queue_type &qQueueInstance, std::defer_lock_t) noexcept:
		m_psqQueueInstance(&qQueueInstance),
		m_bQueueIsLocked(false),
		m_bTokenIsAvailable(false),
		m_ltLockToken()
	{
		// The queue is assumed to be already locked
	}


	queue_lock_helper(queue_type &qQueueInstance, std::adopt_lock_t) noexcept: 
		m_psqQueueInstance(&qQueueInstance),
		m_bQueueIsLocked(true),
		m_bTokenIsAvailable(false),
		m_ltLockToken()
	{
		// The queue is assumed to be already locked
	}

	queue_lock_helper(queue_type &qQueueInstance, std::adopt_lock_t, lock_token_type ltTokenInstance) noexcept:
		m_psqQueueInstance(&qQueueInstance),
		m_bQueueIsLocked(true),
		m_bTokenIsAvailable(true),
		m_ltLockToken(ltTokenInstance)
	{
		// The queue is assumed to be already locked
	}

	~queue_lock_helper() noexcept
	{
		_finalize();
	}

	queue_lock_helper(const queue_lock_helper &) = delete;
	queue_lock_helper &operator=(const queue_lock_helper &) = delete;

	queue_lock_helper(queue_lock_helper &&lhAnotherInstance) noexcept:
		m_psqQueueInstance(lhAnotherInstance.m_psqQueueInstance),
		m_bQueueIsLocked(lhAnotherInstance.m_bQueueIsLocked),
		m_bTokenIsAvailable(lhAnotherInstance.m_bTokenIsAvailable),
		m_ltLockToken(std::move(lhAnotherInstance.m_ltLockToken))
	{
		lhAnotherInstance.m_psqQueueInstance = nullptr;
		lhAnotherInstance.m_bQueueIsLocked = false;
	}

	queue_lock_helper &operator =(queue_lock_helper &&lhAnotherInstance)
	{
		_finalize();

		queue_lock_helper(std::move(lhAnotherInstance)).swap(*this);
		lhAnotherInstance.m_psqQueueInstance = nullptr;
		lhAnotherInstance.m_bQueueIsLocked = false;

		return *this;
	}

	void lock()
	{
		if (!m_psqQueueInstance)
		{
			throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
		}
		else if (m_bQueueIsLocked)
		{
			throw std::system_error(std::make_error_code(std::errc::resource_deadlock_would_occur));
		}
		else
		{
			m_psqQueueInstance->lock();
			m_bQueueIsLocked = true;
			// assert(!m_bTokenIsAvailable);
		}
	}

	void lock(acquire_tocken_t)
	{
		if (!m_psqQueueInstance)
		{
			throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
		}
		else if (m_bQueueIsLocked)
		{
			throw std::system_error(std::make_error_code(std::errc::resource_deadlock_would_occur));
		}
		else
		{
			m_psqQueueInstance->lock(&m_ltLockToken);
			m_bQueueIsLocked = true;
			m_bTokenIsAvailable = true;
		}
	}

	void unlock()
	{
		if (!m_bQueueIsLocked)
		{
			throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
		}
		else
		{
			m_psqQueueInstance->unlock();
			_set_unlocked_status();
		}
	}

	void unlock_and_wait(const item_view &ivRefItemToBeWaited, waiter &wRefWaiterToBeEngaged)
	{
		if (!m_bQueueIsLocked)
		{
			throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
		}
		else
		{
			m_psqQueueInstance->unlock_and_wait(ivRefItemToBeWaited, wRefWaiterToBeEngaged);
			_set_unlocked_status();
		}
	}

	void swap(queue_lock_helper &lhAnotherInstance) noexcept
	{
		std::swap(m_psqQueueInstance, lhAnotherInstance.m_psqQueueInstance);
		std::swap(m_bQueueIsLocked, lhAnotherInstance.m_bQueueIsLocked);
		std::swap(m_bTokenIsAvailable, lhAnotherInstance.m_bTokenIsAvailable);
		std::swap(m_ltLockToken, lhAnotherInstance.m_ltLockToken);
	}

	queue_type *release(lock_token_type *pltOutLockToken=nullptr)
	{
		if (pltOutLockToken != nullptr && !m_bTokenIsAvailable)
		{
			throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
		}

		queue_type *psqQueueInstance = m_psqQueueInstance;
		
		if (pltOutLockToken != nullptr)
		{
			*pltOutLockToken = std::move(m_ltLockToken);
		}

		m_psqQueueInstance = nullptr;
		m_bQueueIsLocked = false;
		m_bTokenIsAvailable = false;
		
		return psqQueueInstance;
	}

	bool owns_lock() const noexcept
	{
		return m_bQueueIsLocked;
	}

	bool has_token() const noexcept
	{
		return m_bTokenIsAvailable;
	}

	queue_type *queue() const noexcept
	{
		return m_psqQueueInstance;
	}

	lock_token_type token() const
	{
		if (!m_bTokenIsAvailable)
		{
			throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
		}

		return m_ltLockToken;
	}

protected:
	void _set_unlocked_status() noexcept
	{
		m_bQueueIsLocked = false;
		m_bTokenIsAvailable = false;
	}

private:
	void _finalize() noexcept
	{
		if (m_bQueueIsLocked)
		{
			m_psqQueueInstance->unlock();
		}
	}

private:
	queue_type		*m_psqQueueInstance;
	bool			m_bQueueIsLocked;
	bool			m_bTokenIsAvailable;
	lock_token_type	m_ltLockToken;
};

template<class TQueueType>
class queue_lock_helper<TQueueType, true> :
	public queue_lock_helper<TQueueType, false>
{
public:
	using parent_type = queue_lock_helper<TQueueType, false>;
	using typename parent_type::queue_type;
	using typename parent_type::lock_token_type;

	explicit queue_lock_helper(queue_type &qQueueInstance) :
		parent_type(qQueueInstance)
	{
	}

	explicit queue_lock_helper(queue_type &qQueueInstance, acquire_tocken_t atAcquireTokenRequest) :
		parent_type(qQueueInstance, atAcquireTokenRequest)
	{
	}

	queue_lock_helper(queue_type &qQueueInstance, std::defer_lock_t dlDeferLockRequest) noexcept :
		parent_type(qQueueInstance, dlDeferLockRequest)
	{
	}


	queue_lock_helper(queue_type &qQueueInstance, std::adopt_lock_t alAdoptLockRequest) noexcept :
		parent_type(qQueueInstance, alAdoptLockRequest)
	{
	}

	queue_lock_helper(queue_type &qQueueInstance, std::adopt_lock_t alAdoptLockRequest, lock_token_type ltTokenInstance) noexcept :
		parent_type(qQueueInstance, alAdoptLockRequest, ltTokenInstance)
	{
	}

	~queue_lock_helper() noexcept = default;

	queue_lock_helper(queue_lock_helper &&lhAnotherInstance) noexcept :
		parent_type(std::move(static_cast<parent_type &>(lhAnotherInstance)))
	{
	}

	queue_lock_helper &operator =(queue_lock_helper &&lhAnotherInstance)
	{
		parent_type::operator =(std::move(static_cast<parent_type &>(lhAnotherInstance)));
		return *this;
	}

	using parent_type::lock;
	using parent_type::unlock;
	using parent_type::unlock_and_wait;

	void unlock_and_cancel(const item_view &ivRefItemToBeWaited, waiter &wRefWaiterToBeEngaged, typename queue_type::ownership_type &oOutResultingItemOwnership,
		const typename queue_type::cancel_callback_type &fnCancelCallback = nullptr) noexcept(false)
	{
		if (!owns_lock())
		{
			throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
		}
		else
		{
			try
			{
				queue()->unlock_and_cancel(ivRefItemToBeWaited, wRefWaiterToBeEngaged, oOutResultingItemOwnership, fnCancelCallback);
				_set_unlocked_status();
			}
			catch (...)
			{
				_set_unlocked_status();
				throw;
			}
		}
	}

	using parent_type::swap;
	using parent_type::release;

	using parent_type::owns_lock;
	using parent_type::has_token;
	using parent_type::queue;
	using parent_type::token;

protected:
	using parent_type::_set_unlocked_status;
};



struct adopt_work_t { explicit adopt_work_t() noexcept = default; };


/**
 *	\template queue_work_helper
 *	\brief A template class to aid safe starting and finishing work on queue items
 *
 *	\see waitable_queue
 *	\see cancelable_queue
 */
template<class TQueueType>
class queue_work_helper
{
public:
	typedef TQueueType queue_type;

	explicit queue_work_helper(queue_type &qQueueInstance) noexcept :
		m_psqQueueInstance(&qQueueInstance),
		m_psiItemInstance(nullptr)
	{
	}

	queue_work_helper(queue_type &qQueueInstance, adopt_work_t, item &iWorkStartedItem) noexcept :
		m_psqQueueInstance(&qQueueInstance),
		m_psiItemInstance(m_psiItemInstance = &iWorkStartedItem),
		m_pswEngagedWorker(&worker::instance_from_pointer(iWorkStartedItem.get_worker())),
		m_bLockedFinishPartExecuted(false)
	{
	}

	~queue_work_helper()
	{
		_finalize();
	}

	queue_work_helper(const queue_work_helper &) = delete;
	queue_work_helper &operator =(const queue_work_helper &) = delete;

	queue_work_helper(queue_work_helper &&whAnotherInstance) noexcept :
		m_psqQueueInstance(whAnotherInstance.m_psqQueueInstance),
		m_psiItemInstance(whAnotherInstance.m_psiItemInstance)
	{
		if (whAnotherInstance.m_psiItemInstance != nullptr)
		{
			whAnotherInstance.m_psiItemInstance == nullptr;
			m_pswEngagedWorker = whAnotherInstance.m_pswEngagedWorker;
			m_bLockedFinishPartExecuted = whAnotherInstance.m_bLockedFinishPartExecuted;
		}
	}
	
	queue_work_helper &operator =(queue_work_helper &&whAnotherInstance)
	{
		_finalize();

		m_psqQueueInstance = whAnotherInstance.m_psqQueueInstance;
		m_psiItemInstance = whAnotherInstance.m_psiItemInstance;

		if (whAnotherInstance.m_psiItemInstance != nullptr)
		{
			whAnotherInstance.m_psiItemInstance == nullptr;
			m_pswEngagedWorker = whAnotherInstance.m_pswEngagedWorker;
			m_bLockedFinishPartExecuted = whAnotherInstance.m_bLockedFinishPartExecuted;
		}

		return *this;
	}

	void start(item &iRefItemInstance, worker &wRefWorkerToBeEngaged) noexcept
	{
		m_psqQueueInstance->start(iRefItemInstance, wRefWorkerToBeEngaged);

		// Assign after calling the method to let it bail out on exceptions
		m_psiItemInstance = &iRefItemInstance;
		m_pswEngagedWorker = &wRefWorkerToBeEngaged;
		m_bLockedFinishPartExecuted = false;
	}

	void unsafefinish__locked()
	{
		if (m_psiItemInstance == nullptr || m_bLockedFinishPartExecuted)
		{
			throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
		}
		else
		{
			m_psqQueueInstance->unsafefinish__locked(*m_psiItemInstance);
			m_bLockedFinishPartExecuted = true;
		}
	}

	void unsafefinish__unlocked()
	{
		if (m_psiItemInstance == nullptr || !m_bLockedFinishPartExecuted)
		{
			throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
		}
		else
		{
			m_psqQueueInstance->unsafefinish__unlocked(*m_psiItemInstance, *m_pswEngagedWorker);
			m_psiItemInstance = nullptr;
		}
	}

	void safefinish()
	{
		if (m_psiItemInstance == nullptr || m_bLockedFinishPartExecuted)
		{
			throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
		}
		else
		{
			m_psqQueueInstance->safefinish(*m_psiItemInstance, *m_pswEngagedWorker);
			m_psiItemInstance = nullptr;
		}
	}

	void swap(queue_work_helper &whAnotherInstance) noexcept
	{
		std::swap(m_psqQueueInstance, whAnotherInstance.m_psqQueueInstance);

		if (m_psiItemInstance != nullptr)
		{
			if (whAnotherInstance.m_psiItemInstance != nullptr)
			{
				std::swap(m_psiItemInstance, whAnotherInstance.m_psiItemInstance);
				std::swap(m_pswEngagedWorker, whAnotherInstance.m_pswEngagedWorker);
				std::swap(m_bLockedFinishPartExecuted, whAnotherInstance.m_bLockedFinishPartExecuted);
			}
			else
			{
				whAnotherInstance.m_psiItemInstance = m_psiItemInstance;
				m_psiItemInstance = nullptr;

				whAnotherInstance.m_pswEngagedWorker = m_pswEngagedWorker;
				whAnotherInstance.m_bLockedFinishPartExecuted = m_bLockedFinishPartExecuted;
			}
		}
		else if (whAnotherInstance.m_psiItemInstance != nullptr)
		{
			m_psiItemInstance = whAnotherInstance.m_psiItemInstance;
			whAnotherInstance.m_psiItemInstance = nullptr;

			m_pswEngagedWorker = whAnotherInstance.m_pswEngagedWorker;
			m_bLockedFinishPartExecuted = whAnotherInstance.m_bLockedFinishPartExecuted;
		}
	}

	queue_type *release(item_view &ivOutStartedItem, worker_view &wvOutEngagedWorker, bool &bOutLockedFinishWasExecuted) noexcept
	{
		queue_type *psqQueueInstance = m_psqQueueInstance;

		ivOutStartedItem = m_psiItemInstance;

		if (m_psiItemInstance != nullptr)
		{
			m_psiItemInstance = nullptr;

			wvOutEngagedWorker = *m_pswEngagedWorker;
			bOutLockedFinishWasExecuted = m_bLockedFinishPartExecuted;
		}
		else
		{
			wvOutEngagedWorker = (worker::pointer)nullptr;
			bOutLockedFinishWasExecuted = false;
		}

		return psqQueueInstance;
	}

	bool is_started() const noexcept
	{
		return m_psiItemInstance != nullptr;
	}

	bool is_finished__locked() const noexcept
	{
		return m_psiItemInstance != nullptr && m_bLockedFinishPartExecuted;
	}

	queue_type *queue() const noexcept
	{
		return m_psqQueueInstance;
	}

private:
	void _finalize()
	{
		if (m_psiItemInstance != nullptr)
		{
			if (m_bLockedFinishPartExecuted)
			{
				m_psqQueueInstance->unsafefinish__unlocked(*m_psiItemInstance, *m_pswEngagedWorker);
			}
			else
			{
				m_psqQueueInstance->safefinish(*m_psiItemInstance, *m_pswEngagedWorker);
			}
		}
	}

private:
	queue_type		*m_psqQueueInstance;
	item			*m_psiItemInstance;
	worker			*m_pswEngagedWorker;
	bool			m_bLockedFinishPartExecuted;
};


_MUTEXGEAR_END_COMPLETION_NAMESPACE();

_MUTEXGEAR_END_NAMESPACE();


#endif // #ifndef __MUTEXGEAR_COMPLETION_HPP_INCLUDED
