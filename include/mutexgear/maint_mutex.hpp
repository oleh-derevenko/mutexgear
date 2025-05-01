#ifndef __MUTEXGEAR_MAINT_MUTEX_HPP_INCLUDED
#define __MUTEXGEAR_MAINT_MUTEX_HPP_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear maint_mutex Class Definitions                              */
/*                                                                      */
/* WARNING!                                                             */
/* This library contains a synchronization technique protected by       */
/* the U.S. Patent 9,983,913.                                           */
/*                                                                      */
/* THIS IS A PRE-RELEASE LIBRARY SNAPSHOT.                              */
/* AWAIT THE RELEASE AT https://mutexgear.com                           */
/*                                                                      */
/* Copyright (c) 2016-2025 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/

/**
*	\file
*	\brief MutexGear \c maint_mutex class definitions
*
*	The header defines \c maint_mutex class which is a header-only wrapper
*	for \c mutexgear_maintlock_t object.
*
*	NOTE:
*
*	The \c mutexgear_maintlock_t object depends on a synchronization
*	mechanism being a subject of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and
*	Image Database search (currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm)
*	to view the patent text.
*/


#include <mutexgear/_mtx_helpers.hpp>
#include <mutexgear/maintlock.h>
#include <system_error>
#include <errno.h>


_MUTEXGEAR_BEGIN_NAMESPACE()

_MUTEXGEAR_BEGIN_MNTMTX_HELPERS_NAMESPACE()

using _MUTEXGEAR_MTX_HELPERS_NAMESPACE::worker;
using _MUTEXGEAR_MTX_HELPERS_NAMESPACE::waiter;
using _MUTEXGEAR_MTX_HELPERS_NAMESPACE::item;
using _MUTEXGEAR_MTX_HELPERS_NAMESPACE::bourgeois;


class shared_lock_token
{
public:
	typedef mutexgear_maintlock_rdlock_token_t *pointer;
	typedef const mutexgear_maintlock_rdlock_token_t *const_pointer;

public:
	operator pointer() noexcept { return &m_ltTokenValue; }
	operator const_pointer() const noexcept { return &m_ltTokenValue; }

private:
	mutexgear_maintlock_rdlock_token_t		m_ltTokenValue;
};


_MUTEXGEAR_END_MNTMTX_HELPERS_NAMESPACE();


/**
*	\class maint_mutex
*	\brief A wrapper for \c mutexgear_maintlock_t and its related functions.
*
*	The class implements a maintenance lock. 
*
*	\see mutexgear_maintlock_t
*/
class maint_mutex
{
public:
	typedef mutexgear_maintlock_t *native_handle_type;

	typedef _MUTEXGEAR_MNTMTX_HELPERS_NAMESPACE::bourgeois helper_bourgeois_type;
	typedef _MUTEXGEAR_MNTMTX_HELPERS_NAMESPACE::worker helper_worker_type;
	typedef _MUTEXGEAR_MNTMTX_HELPERS_NAMESPACE::waiter helper_waiter_type;
	typedef _MUTEXGEAR_MNTMTX_HELPERS_NAMESPACE::item helper_item_type;
	typedef _MUTEXGEAR_MNTMTX_HELPERS_NAMESPACE::shared_lock_token lock_token_type;

	maint_mutex()
	{
		int iInitializationResult = mutexgear_maintlock_init(&m_mlMaintLockInstance, nullptr);

		if (iInitializationResult != EOK)
		{
			throw std::system_error(std::error_code(iInitializationResult, std::system_category()));
		}
	}

	maint_mutex(const maint_mutex &mtAnotherInstance) = delete;

	~maint_mutex() noexcept
	{
		int iMaintMutexDestructionResult;
		MG_CHECK(iMaintMutexDestructionResult, (iMaintMutexDestructionResult = mutexgear_maintlock_destroy(&m_mlMaintLockInstance)) == EOK);
	}

	maint_mutex &operator =(const maint_mutex &mtAnotherInstance) = delete;

public:
	void set_maintenance()
	{
		int iOperationResult = mutexgear_maintlock_set_maintenance(&m_mlMaintLockInstance);

		if (iOperationResult != EOK)
		{
			throw std::system_error(std::error_code(iOperationResult, std::system_category()));
		}
	}

	void clear_maintenance()
	{
		int iOperationResult = mutexgear_maintlock_clear_maintenance(&m_mlMaintLockInstance);

		if (iOperationResult != EOK)
		{
			throw std::system_error(std::error_code(iOperationResult, std::system_category()));
		}
	}

	bool is_maintenance() const
	{
		int iOperationResult = mutexgear_maintlock_test_maintenance(&m_mlMaintLockInstance);
		return iOperationResult > 0 || (iOperationResult < 0 && (throw std::system_error(std::error_code(-iOperationResult, std::system_category())), false));
	}

	bool try_lock_shared(helper_bourgeois_type &bRefBourgeoisInstance, lock_token_type &ltOutLockToken)
	{
		int iTryLockResult = mutexgear_maintlock_tryrdlock(&m_mlMaintLockInstance, static_cast<helper_worker_type::pointer>(bRefBourgeoisInstance), static_cast<helper_item_type::pointer>(bRefBourgeoisInstance), static_cast<lock_token_type::pointer>(ltOutLockToken));
		return iTryLockResult == EOK || (iTryLockResult != EBUSY && (throw std::system_error(std::error_code(iTryLockResult, std::system_category())), false));
	}

	void unlock_shared(helper_bourgeois_type &bRefBourgeoisInstance, const lock_token_type &ltLockToken) noexcept
	{
		int iMaintLockRdUnlockResult;
		MG_CHECK(iMaintLockRdUnlockResult, (iMaintLockRdUnlockResult = mutexgear_maintlock_rdunlock(&m_mlMaintLockInstance, static_cast<helper_worker_type::pointer>(bRefBourgeoisInstance), static_cast<helper_item_type::pointer>(bRefBourgeoisInstance), static_cast<lock_token_type::const_pointer>(ltLockToken))) == EOK);
	}

	void wait_shared_unlock(helper_waiter_type &wRefWaiterInstance)
	{
		int iWaitResult = mutexgear_maintlock_wait_rdunlock(&m_mlMaintLockInstance, static_cast<helper_waiter_type::pointer>(wRefWaiterInstance));

		if (iWaitResult != EOK)
		{
			throw std::system_error(std::error_code(iWaitResult, std::system_category()));
		}
	}

public: // Less convenient overloads
	bool try_lock_shared(helper_worker_type &wRefWorkerInstance, helper_item_type &iRefItemInstance, lock_token_type &ltOutLockToken)
	{
		int iTryLockResult = mutexgear_maintlock_tryrdlock(&m_mlMaintLockInstance, static_cast<helper_worker_type::pointer>(wRefWorkerInstance), static_cast<helper_item_type::pointer>(iRefItemInstance), static_cast<lock_token_type::pointer>(ltOutLockToken));
		return iTryLockResult == EOK || (iTryLockResult != EBUSY && (throw std::system_error(std::error_code(iTryLockResult, std::system_category())), false));
	}

	void unlock_shared(helper_worker_type &wRefWorkerInstance, helper_item_type &iRefItemInstance, const lock_token_type &ltLockToken) noexcept
	{
		int iMaintLockRdUnlockResult;
		MG_CHECK(iMaintLockRdUnlockResult, (iMaintLockRdUnlockResult = mutexgear_maintlock_rdunlock(&m_mlMaintLockInstance, static_cast<helper_worker_type::pointer>(wRefWorkerInstance), static_cast<helper_item_type::pointer>(iRefItemInstance), static_cast<lock_token_type::const_pointer>(ltLockToken))) == EOK);
	}

public:
	native_handle_type native_handle() const noexcept { return static_cast<native_handle_type>(const_cast<mutexgear_maintlock_t *>(&m_mlMaintLockInstance)); }

private:
	mutexgear_maintlock_t		m_mlMaintLockInstance;
};


_MUTEXGEAR_END_NAMESPACE();


#endif // #ifndef __MUTEXGEAR_MAINT_MUTEX_HPP_INCLUDED
