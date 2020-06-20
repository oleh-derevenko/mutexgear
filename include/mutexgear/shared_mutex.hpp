#ifndef __MUTEXGEAR_SHARED_MUTEX_HPP_INCLUDED
#define __MUTEXGEAR_SHARED_MUTEX_HPP_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear shared_mutex Class Definitions                             */
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
 *	\brief MutexGear \c shared_mutex class definitions
 *
 *	The header defines two \c shared_mutex classes (one in a nested namespace)
 *	which are header-only wrappers for \c mutexgear_rwlock_t and \c mutexgear_trdl_rwlock_t objects.
 *
 *	NOTE:
 *
 *	The \c mutexgear_rwlock_t and \c mutexgear_trdl_rwlock_t objects depend on a synchronization 
 *	mechanism being a subject of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and 
 *	Image Database search (currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm) 
 *	to view the patent text.
 */


#include <mutexgear/completion.hpp>
#include <mutexgear/rwlock.h>
#include <system_error>
#include <errno.h>


_MUTEXGEAR_BEGIN_NAMESPACE()

_MUTEXGEAR_BEGIN_SHMTX_HELPERS_NAMESPACE()

using _MUTEXGEAR_COMPLETION_NAMESPACE::worker;
using _MUTEXGEAR_COMPLETION_NAMESPACE::waiter;
using _MUTEXGEAR_COMPLETION_NAMESPACE::item;


_MUTEXGEAR_END_SHMTX_HELPERS_NAMESPACE();


class shared_mutex
{
public:
	typedef mutexgear_rwlock_t *native_handle_type;

	typedef _MUTEXGEAR_SHMTX_HELPERS_NAMESPACE::worker helper_worker_type;
	typedef _MUTEXGEAR_SHMTX_HELPERS_NAMESPACE::waiter helper_waiter_type;
	typedef _MUTEXGEAR_SHMTX_HELPERS_NAMESPACE::item helper_item_type;

	shared_mutex()
	{
		int iInitializationResult = mutexgear_rwlock_init(&m_wlRWLockInstance, NULL);

		if (iInitializationResult != EOK)
		{
			throw std::system_error(std::error_code(iInitializationResult, std::system_category()));
		}
	}

	shared_mutex(const shared_mutex &mtAnotherInstance) = delete;

	~shared_mutex() noexcept
	{
		int iSharedMutexDestructionResult;
		MG_CHECK(iSharedMutexDestructionResult, (iSharedMutexDestructionResult = mutexgear_rwlock_destroy(&m_wlRWLockInstance)) == EOK);
	}

	shared_mutex &operator =(const shared_mutex &mtAnotherInstance) = delete;

public:
	void lock(helper_worker_type &wRefWorkerInstance, helper_waiter_type &wRefWaiterInstance)
	{
		int iLockResult = mutexgear_rwlock_wrlock(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(wRefWorkerInstance), static_cast<helper_waiter_type::pointer>(wRefWaiterInstance), NULL);

		if (iLockResult != EOK)
		{
			throw std::system_error(std::error_code(iLockResult, std::system_category()));
		}
	}

	bool try_lock()
	{
		int iTryLockResult = mutexgear_rwlock_trywrlock(&m_wlRWLockInstance);
		return iTryLockResult == EOK || (iTryLockResult != EBUSY && (throw std::system_error(std::error_code(iTryLockResult, std::system_category())), false));
	}

	void unlock() noexcept
	{
		int iRWLockWrUnlockResult;
		MG_CHECK(iRWLockWrUnlockResult, (iRWLockWrUnlockResult = mutexgear_rwlock_wrunlock(&m_wlRWLockInstance)) == EOK);
	}

	void lock_shared(helper_worker_type &wRefWorkerInstance, helper_waiter_type &wRefWaiterInstance, helper_item_type &iRefItemInstance)
	{
		int iLockResult = mutexgear_rwlock_rdlock(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(wRefWorkerInstance), static_cast<helper_waiter_type::pointer>(wRefWaiterInstance), static_cast<helper_item_type::pointer>(iRefItemInstance));

		if (iLockResult != EOK)
		{
			throw std::system_error(std::error_code(iLockResult, std::system_category()));
		}
	}

	void unlock_shared(helper_worker_type &wRefWorkerInstance, helper_item_type &iRefItemInstance) noexcept
	{
		int iRWLockRdUnlockResult;
		MG_CHECK(iRWLockRdUnlockResult, (iRWLockRdUnlockResult = mutexgear_rwlock_rdunlock(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(wRefWorkerInstance), static_cast<helper_item_type::pointer>(iRefItemInstance))) == EOK);
	}

public:
	native_handle_type native_handle() const noexcept { return static_cast<native_handle_type>(const_cast<mutexgear_rwlock_t *>(&m_wlRWLockInstance)); }

private:
	mutexgear_rwlock_t		m_wlRWLockInstance;
};


_MUTEXGEAR_BEGIN_TRDL_NAMESPACE()

class shared_mutex
{
public:
	typedef mutexgear_trdl_rwlock_t *native_handle_type;

	typedef _MUTEXGEAR_SHMTX_HELPERS_NAMESPACE::worker helper_worker_type;
	typedef _MUTEXGEAR_SHMTX_HELPERS_NAMESPACE::waiter helper_waiter_type;
	typedef _MUTEXGEAR_SHMTX_HELPERS_NAMESPACE::item helper_item_type;

	shared_mutex()
	{
		int iInitializationResult = mutexgear_trdl_rwlock_init(&m_wlRWLockInstance, NULL);

		if (iInitializationResult != EOK)
		{
			throw std::system_error(std::error_code(iInitializationResult, std::system_category()));
		}
	}

	shared_mutex(const shared_mutex &mtAnotherInstance) = delete;

	~shared_mutex() noexcept
	{
		int iSharedMutexDestructionResult;
		MG_CHECK(iSharedMutexDestructionResult, (iSharedMutexDestructionResult = mutexgear_trdl_rwlock_destroy(&m_wlRWLockInstance)) == EOK);
	}

	shared_mutex &operator =(const shared_mutex &mtAnotherInstance) = delete;

public:
	void lock(helper_worker_type &wRefWorkerInstance, helper_waiter_type &wRefWaiterInstance)
	{
		int iLockResult = mutexgear_trdl_rwlock_wrlock(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(wRefWorkerInstance), static_cast<helper_waiter_type::pointer>(wRefWaiterInstance), NULL);

		if (iLockResult != EOK)
		{
			throw std::system_error(std::error_code(iLockResult, std::system_category()));
		}
	}

	bool try_lock()
	{
		int iTryLockResult = mutexgear_trdl_rwlock_trywrlock(&m_wlRWLockInstance);
		return iTryLockResult == EOK || (iTryLockResult != EBUSY && (throw std::system_error(std::error_code(iTryLockResult, std::system_category())), false));
	}

	void unlock() noexcept
	{
		int iRWLockWrUnlockResult;
		MG_CHECK(iRWLockWrUnlockResult, (iRWLockWrUnlockResult = mutexgear_trdl_rwlock_wrunlock(&m_wlRWLockInstance)) == EOK);
	}

	void lock_shared(helper_worker_type &wRefWorkerInstance, helper_waiter_type &wRefWaiterInstance, helper_item_type &iRefItemInstance)
	{
		int iLockResult = mutexgear_trdl_rwlock_rdlock(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(wRefWorkerInstance), static_cast<helper_waiter_type::pointer>(wRefWaiterInstance), static_cast<helper_item_type::pointer>(iRefItemInstance));

		if (iLockResult != EOK)
		{
			throw std::system_error(std::error_code(iLockResult, std::system_category()));
		}
	}

	bool try_lock_shared(helper_worker_type &wRefWorkerInstance, helper_item_type &iRefItemInstance)
	{
		int iTryLockResult = mutexgear_trdl_rwlock_tryrdlock(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(wRefWorkerInstance), static_cast<helper_item_type::pointer>(iRefItemInstance));
		return iTryLockResult == EOK || (iTryLockResult != EBUSY && (throw std::system_error(std::error_code(iTryLockResult, std::system_category())), false));
	}

	void unlock_shared(helper_worker_type &wRefWorkerInstance, helper_item_type &iRefItemInstance) noexcept
	{
		int iRWLockRdUnlockResult;
		MG_CHECK(iRWLockRdUnlockResult, (iRWLockRdUnlockResult = mutexgear_trdl_rwlock_rdunlock(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(wRefWorkerInstance), static_cast<helper_item_type::pointer>(iRefItemInstance))) == EOK);
	}

public:
	native_handle_type native_handle() const noexcept { return static_cast<native_handle_type>(const_cast<mutexgear_trdl_rwlock_t *>(&m_wlRWLockInstance)); }

private:
	mutexgear_trdl_rwlock_t	m_wlRWLockInstance;
};


_MUTEXGEAR_END_TRDL_NAMESPACE();


_MUTEXGEAR_END_NAMESPACE();


#endif // #ifndef __MUTEXGEAR_SHARED_MUTEX_HPP_INCLUDED
