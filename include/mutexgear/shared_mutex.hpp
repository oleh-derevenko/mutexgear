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
*	\brief MutexGear \c shared_mutex class definitions
*
*	The header defines two \c wp_shared_mutex<std::size_t tsiWriteChannels> class templateses 
*	(one of them in a nested namespace) which are header-only wrappers 
*	for \c mutexgear_rwlock_t and \c mutexgear_trdl_rwlock_t objects.
*
*	Also, the header defines \c shared_mutex as \c wp_shared_mutex<0> specializations.
*
*	NOTE:
*
*	The \c mutexgear_rwlock_t and \c mutexgear_trdl_rwlock_t objects depend on a synchronization
*	mechanism being a subject of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and
*	Image Database search (currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm)
*	to view the patent text.
*/


#include <mutexgear/_mtx_helpers.hpp>
#include <mutexgear/rwlock.h>
#include <algorithm>
#include <limits>
#include <system_error>
#include <errno.h>


_MUTEXGEAR_BEGIN_NAMESPACE()

_MUTEXGEAR_BEGIN_SHMTX_HELPERS_NAMESPACE()

using _MUTEXGEAR_MTX_HELPERS_NAMESPACE::worker;
using _MUTEXGEAR_MTX_HELPERS_NAMESPACE::waiter;
using _MUTEXGEAR_MTX_HELPERS_NAMESPACE::item;
using _MUTEXGEAR_MTX_HELPERS_NAMESPACE::bourgeois;


_MUTEXGEAR_END_SHMTX_HELPERS_NAMESPACE();


struct no_wp_t { explicit no_wp_t() noexcept = default; };


/**
*	\class wp_shared_mutex<std::size_t tsiWriteChannels>
*	\brief A wrapper for \c mutexgear_rwlock_t and its related functions.
*
*	The class implements a read-write lock without try-read lock support and with possibility to customize write channel count. 
*	If \p tsiWriteChannels is 0 the default initialization attributes are used.
*
*	The class method names are compatible with those of \c std::shared_mutex.
*
*	\see mutexgear_rwlock_t
*/
template<std::size_t tsiWriteChannels=0>
class wp_shared_mutex
{
public:
	typedef mutexgear_rwlock_t *native_handle_type;

	typedef _MUTEXGEAR_SHMTX_HELPERS_NAMESPACE::bourgeois helper_bourgeois_type;
	typedef _MUTEXGEAR_SHMTX_HELPERS_NAMESPACE::worker helper_worker_type;
	typedef _MUTEXGEAR_SHMTX_HELPERS_NAMESPACE::waiter helper_waiter_type;
	typedef _MUTEXGEAR_SHMTX_HELPERS_NAMESPACE::item helper_item_type;

	wp_shared_mutex()
	{
		int iInitializationResult;

		constexpr const bool bAttributesRequired = tsiWriteChannels != 0;
		bool bAttributesInitialized = false, bAttributesFailed = false;

		mutexgear_rwlockattr_t laLockAttributesStorage;

		if (bAttributesRequired)
		{
			if ((iInitializationResult = mutexgear_rwlockattr_init(&laLockAttributesStorage)) != EOK)
			{
				bAttributesFailed = true;
			}

			if (!bAttributesFailed)
			{
				bAttributesInitialized = true;

				// NOTE!
				// Define NOMINMAX if you have problems building this code on Windows.
				unsigned int uiLimitedChannelCount = static_cast<unsigned int>(std::min(tsiWriteChannels, static_cast<std::size_t>(std::numeric_limits<unsigned int>::max())));
				if ((iInitializationResult = mutexgear_rwlockattr_setwritechannels(&laLockAttributesStorage, uiLimitedChannelCount)) != EOK)
				{
					bAttributesFailed = true;
				}
			}
		}

		if (!bAttributesFailed)
		{
			iInitializationResult = mutexgear_rwlock_init(&m_wlRWLockInstance, bAttributesRequired ? &laLockAttributesStorage : nullptr);
		}

		if (bAttributesInitialized)
		{
			int iAttributesDestroyResult = mutexgear_rwlockattr_destroy(&laLockAttributesStorage);
			if (iAttributesDestroyResult != EOK)
			{
				iInitializationResult = iAttributesDestroyResult;
			}
		}

		if (iInitializationResult != EOK)
		{
			throw std::system_error(std::error_code(iInitializationResult, std::system_category()));
		}
	}

	wp_shared_mutex(const wp_shared_mutex &mtAnotherInstance) = delete;

	~wp_shared_mutex() noexcept
	{
		int iSharedMutexDestructionResult;
		MG_CHECK(iSharedMutexDestructionResult, (iSharedMutexDestructionResult = mutexgear_rwlock_destroy(&m_wlRWLockInstance)) == EOK);
	}

	wp_shared_mutex &operator =(const wp_shared_mutex &mtAnotherInstance) = delete;

public:
	void lock(helper_bourgeois_type &bRefBourgeoisInstance, helper_waiter_type &wRefWaiterInstance)
	{
		int iLockResult = mutexgear_rwlock_wrlock(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(bRefBourgeoisInstance), static_cast<helper_waiter_type::pointer>(wRefWaiterInstance), static_cast<helper_item_type::pointer>(bRefBourgeoisInstance));

		if (iLockResult != EOK)
		{
			throw std::system_error(std::error_code(iLockResult, std::system_category()));
		}
	}

	void lock(helper_bourgeois_type &bRefBourgeoisInstance, helper_waiter_type &wRefWaiterInstance, std::size_t siReadersTillWP)
	{
		// NOTE!
		// Define NOMINMAX if you have problems building this code on Windows.
		int iLimitedReadersTillWP = static_cast<int>(std::min(siReadersTillWP, static_cast<std::size_t>(std::numeric_limits<int>::max())));
		int iLockResult = mutexgear_rwlock_wrlock_cwp(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(bRefBourgeoisInstance), static_cast<helper_waiter_type::pointer>(wRefWaiterInstance), static_cast<helper_item_type::pointer>(bRefBourgeoisInstance), iLimitedReadersTillWP);

		if (iLockResult != EOK)
		{
			throw std::system_error(std::error_code(iLockResult, std::system_category()));
		}
	}

	void lock(helper_bourgeois_type &bRefBourgeoisInstance, helper_waiter_type &wRefWaiterInstance, no_wp_t)
	{
		int iLockResult = mutexgear_rwlock_wrlock_cwp(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(bRefBourgeoisInstance), static_cast<helper_waiter_type::pointer>(wRefWaiterInstance), static_cast<helper_item_type::pointer>(bRefBourgeoisInstance), -1);

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

	void lock_shared(helper_bourgeois_type &bRefBourgeoisInstance, helper_waiter_type &wRefWaiterInstance)
	{
		int iLockResult = mutexgear_rwlock_rdlock(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(bRefBourgeoisInstance), static_cast<helper_waiter_type::pointer>(wRefWaiterInstance), static_cast<helper_item_type::pointer>(bRefBourgeoisInstance));

		if (iLockResult != EOK)
		{
			throw std::system_error(std::error_code(iLockResult, std::system_category()));
		}
	}

	void unlock_shared(helper_bourgeois_type &bRefBourgeoisInstance) noexcept
	{
		int iRWLockRdUnlockResult;
		MG_CHECK(iRWLockRdUnlockResult, (iRWLockRdUnlockResult = mutexgear_rwlock_rdunlock(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(bRefBourgeoisInstance), static_cast<helper_item_type::pointer>(bRefBourgeoisInstance))) == EOK);
	}

public: // Less convenient overloads
	void lock(helper_worker_type &wRefWorkerInstance, helper_waiter_type &wRefWaiterInstance)
	{
		int iLockResult = mutexgear_rwlock_wrlock(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(wRefWorkerInstance), static_cast<helper_waiter_type::pointer>(wRefWaiterInstance), nullptr);

		if (iLockResult != EOK)
		{
			throw std::system_error(std::error_code(iLockResult, std::system_category()));
		}
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

using shared_mutex = wp_shared_mutex<0>;


/**
*	\namespace trdl
*	\brief A namespace for the try-read lock enabled \c trdl::shared_mutex variant
*/
_MUTEXGEAR_BEGIN_TRDL_NAMESPACE()

/**
*	\class trdl::wp_shared_mutex<std::size_t tsiWriteChannels>
*	\brief A wrapper for \c mutexgear_trdl_rwlock_t and its related functions.
*
*	The class implements a read-write lock with try-read lock support and with possibility to customize write channel count.
*	If \p tsiWriteChannels is 0 the default initialization attributes are used.
*
*	The class method names are compatible with those of \c std::shared_mutex.
*
*	\see mutexgear_trdl_rwlock_t
*/
template<std::size_t tsiWriteChannels=0>
class wp_shared_mutex
{
public:
	typedef mutexgear_trdl_rwlock_t *native_handle_type;

	typedef _MUTEXGEAR_SHMTX_HELPERS_NAMESPACE::bourgeois helper_bourgeois_type;
	typedef _MUTEXGEAR_SHMTX_HELPERS_NAMESPACE::worker helper_worker_type;
	typedef _MUTEXGEAR_SHMTX_HELPERS_NAMESPACE::waiter helper_waiter_type;
	typedef _MUTEXGEAR_SHMTX_HELPERS_NAMESPACE::item helper_item_type;

	wp_shared_mutex()
	{
		int iInitializationResult;

		constexpr const bool bAttributesRequired = tsiWriteChannels != 0;
		bool bAttributesInitialized = false, bAttributesFailed = false;

		mutexgear_rwlockattr_t laLockAttributesStorage;

		if (bAttributesRequired)
		{
			if ((iInitializationResult = mutexgear_rwlockattr_init(&laLockAttributesStorage)) != EOK)
			{
				bAttributesFailed = true;
			}

			if (!bAttributesFailed)
			{
				bAttributesInitialized = true;

				// NOTE!
				// Define NOMINMAX if you have problems building this code on Windows.
				unsigned int uiLimitedChannelCount = static_cast<unsigned int>(std::min(tsiWriteChannels, static_cast<std::size_t>(std::numeric_limits<unsigned int>::max())));
				if ((iInitializationResult = mutexgear_rwlockattr_setwritechannels(&laLockAttributesStorage, uiLimitedChannelCount)) != EOK)
				{
					bAttributesFailed = true;
				}
			}
		}

		if (!bAttributesFailed)
		{
			iInitializationResult = mutexgear_trdl_rwlock_init(&m_wlRWLockInstance, bAttributesRequired ? &laLockAttributesStorage : nullptr);
		}

		if (bAttributesInitialized)
		{
			int iAttributesDestroyResult = mutexgear_rwlockattr_destroy(&laLockAttributesStorage);
			if (iAttributesDestroyResult != EOK)
			{
				iInitializationResult = iAttributesDestroyResult;
			}
		}

		if (iInitializationResult != EOK)
		{
			throw std::system_error(std::error_code(iInitializationResult, std::system_category()));
		}
	}

	wp_shared_mutex(const wp_shared_mutex &mtAnotherInstance) = delete;

	~wp_shared_mutex() noexcept
	{
		int iSharedMutexDestructionResult;
		MG_CHECK(iSharedMutexDestructionResult, (iSharedMutexDestructionResult = mutexgear_trdl_rwlock_destroy(&m_wlRWLockInstance)) == EOK);
	}

	wp_shared_mutex &operator =(const wp_shared_mutex &mtAnotherInstance) = delete;

public:
	void lock(helper_bourgeois_type &bRefBourgeoisInstance, helper_waiter_type &wRefWaiterInstance)
	{
		int iLockResult = mutexgear_trdl_rwlock_wrlock(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(bRefBourgeoisInstance), static_cast<helper_waiter_type::pointer>(wRefWaiterInstance), static_cast<helper_item_type::pointer>(bRefBourgeoisInstance));

		if (iLockResult != EOK)
		{
			throw std::system_error(std::error_code(iLockResult, std::system_category()));
		}
	}

	void lock(helper_bourgeois_type &bRefBourgeoisInstance, helper_waiter_type &wRefWaiterInstance, std::size_t siReadersTillWP)
	{
		// NOTE!
		// Define NOMINMAX if you have problems building this code on Windows.
		int iLimitedReadersTillWP = static_cast<int>(std::min(siReadersTillWP, static_cast<std::size_t>(std::numeric_limits<int>::max())));
		int iLockResult = mutexgear_trdl_rwlock_wrlock_cwp(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(bRefBourgeoisInstance), static_cast<helper_waiter_type::pointer>(wRefWaiterInstance), static_cast<helper_item_type::pointer>(bRefBourgeoisInstance), iLimitedReadersTillWP);

		if (iLockResult != EOK)
		{
			throw std::system_error(std::error_code(iLockResult, std::system_category()));
		}
	}

	void lock(helper_bourgeois_type &bRefBourgeoisInstance, helper_waiter_type &wRefWaiterInstance, no_wp_t)
	{
		int iLockResult = mutexgear_trdl_rwlock_wrlock_cwp(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(bRefBourgeoisInstance), static_cast<helper_waiter_type::pointer>(wRefWaiterInstance), static_cast<helper_item_type::pointer>(bRefBourgeoisInstance), -1);

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

	void lock_shared(helper_bourgeois_type &bRefBourgeoisInstance, helper_waiter_type &wRefWaiterInstance)
	{
		int iLockResult = mutexgear_trdl_rwlock_rdlock(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(bRefBourgeoisInstance), static_cast<helper_waiter_type::pointer>(wRefWaiterInstance), static_cast<helper_item_type::pointer>(bRefBourgeoisInstance));

		if (iLockResult != EOK)
		{
			throw std::system_error(std::error_code(iLockResult, std::system_category()));
		}
	}

	bool try_lock_shared(helper_bourgeois_type &bRefBourgeoisInstance)
	{
		int iTryLockResult = mutexgear_trdl_rwlock_tryrdlock(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(bRefBourgeoisInstance), static_cast<helper_item_type::pointer>(bRefBourgeoisInstance));
		return iTryLockResult == EOK || (iTryLockResult != EBUSY && (throw std::system_error(std::error_code(iTryLockResult, std::system_category())), false));
	}

	void unlock_shared(helper_bourgeois_type &bRefBourgeoisInstance) noexcept
	{
		int iRWLockRdUnlockResult;
		MG_CHECK(iRWLockRdUnlockResult, (iRWLockRdUnlockResult = mutexgear_trdl_rwlock_rdunlock(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(bRefBourgeoisInstance), static_cast<helper_item_type::pointer>(bRefBourgeoisInstance))) == EOK);
	}

public: // Less convenient overloads
	void lock(helper_worker_type &wRefWorkerInstance, helper_waiter_type &wRefWaiterInstance)
	{
		int iLockResult = mutexgear_trdl_rwlock_wrlock(&m_wlRWLockInstance, static_cast<helper_worker_type::pointer>(wRefWorkerInstance), static_cast<helper_waiter_type::pointer>(wRefWaiterInstance), nullptr);

		if (iLockResult != EOK)
		{
			throw std::system_error(std::error_code(iLockResult, std::system_category()));
		}
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

using shared_mutex = wp_shared_mutex<0>;


_MUTEXGEAR_END_TRDL_NAMESPACE();


_MUTEXGEAR_END_NAMESPACE();


#endif // #ifndef __MUTEXGEAR_SHARED_MUTEX_HPP_INCLUDED
