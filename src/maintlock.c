/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear MaintLock API Implementation                               */
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
*	\brief MutexGear MaintLock API implementation
*
*/


#include <mutexgear/maintlock.h>

#include "completion.h"
#include "dlralist.h"
#include "utility.h"


#define _MUTEXGEAR_MAINTLOCK_LOCKED_FOR_UPDATE				0x0001
#define _MUTEXGEAR_MAINTLOCK_LOCKED_FOR_WAIT				0x0002

#define _MUTEXGEAR_MAINTLOCK_MODE_PSHARED					0x0100

#define _MUTEXGEAR_MAINTLOCK_LOCK__ALLOWED_FLAGS			(_MUTEXGEAR_MAINTLOCK_LOCKED_FOR_UPDATE | _MUTEXGEAR_MAINTLOCK_LOCKED_FOR_WAIT)
#define _MUTEXGEAR_MAINTLOCK_MODE__ALLOWED_FLAGS			(_MUTEXGEAR_MAINTLOCK_MODE_PSHARED)


//////////////////////////////////////////////////////////////////////////
// MaintLock Attributes Implementation

/*extern */
int mutexgear_maintlockattr_init(mutexgear_maintlockattr_t *__attr)
{
	__attr->mode_flags = 0;
	int ret = _mutexgear_lockattr_init(&__attr->lock_attr);
	return ret;
}

/*extern */
int mutexgear_maintlockattr_destroy(mutexgear_maintlockattr_t *__attr)
{
	int ret = _mutexgear_lockattr_destroy(&__attr->lock_attr);
	return ret;
}


/*extern */
int mutexgear_maintlockattr_getpshared(const mutexgear_maintlockattr_t *__attr, int *__out_pshared)
{
	int ret = _mutexgear_lockattr_getpshared(&__attr->lock_attr, __out_pshared);
	return ret;
}

/*extern */
int mutexgear_maintlockattr_setpshared(mutexgear_maintlockattr_t *__attr, int __pshared)
{
	int ret = _mutexgear_lockattr_setpshared(&__attr->lock_attr, __pshared);

	if (ret == EOK)
	{
		__attr->mode_flags = __pshared == MUTEXGEAR_PROCESS_SHARED ? __attr->mode_flags | _MUTEXGEAR_MAINTLOCK_MODE_PSHARED : __attr->mode_flags & ~_MUTEXGEAR_MAINTLOCK_MODE_PSHARED;
	}

	return ret;
}


/*extern */
int mutexgear_maintlockattr_getprioceiling(const mutexgear_maintlockattr_t *__attr, int *__out_prioceiling)
{
	int ret = _mutexgear_lockattr_getprioceiling(&__attr->lock_attr, __out_prioceiling);
	return ret;
}

/*extern */
int mutexgear_maintlockattr_getprotocol(const mutexgear_maintlockattr_t *__attr, int *__out_protocol)
{
	int ret = _mutexgear_lockattr_getprotocol(&__attr->lock_attr, __out_protocol);
	return ret;
}

/*extern */
int mutexgear_maintlockattr_setprioceiling(mutexgear_maintlockattr_t *__attr, int __prioceiling)
{
	int ret = _mutexgear_lockattr_setprioceiling(&__attr->lock_attr, __prioceiling);
	return ret;
}

/*extern */
int mutexgear_maintlockattr_setprotocol(mutexgear_maintlockattr_t *__attr, int __protocol)
{
	int ret = _mutexgear_lockattr_setprotocol(&__attr->lock_attr, __protocol);
	return ret;
}

/*extern */
int mutexgear_maintlockattr_setmutexattr(mutexgear_maintlockattr_t *__attr, const _MUTEXGEAR_LOCKATTR_T *__mutexattr)
{
	int ret;

	do
	{
		int prioceiling_value, protocol_value, pshared_value = 0;

		bool pshared_missing = false;
		if ((ret = _mutexgear_lockattr_getpshared(__mutexattr, &pshared_value)) != EOK)
		{
			if (ret != _MUTEXGEAR_ERRNO__PSHARED_MISSING)
			{
				break;
			}

			pshared_missing = true;
		}

		bool prioceiling_missing = false;
		if ((ret = _mutexgear_lockattr_getprioceiling(__mutexattr, &prioceiling_value)) != EOK)
		{
			if (ret != _MUTEXGEAR_ERRNO__PRIOCEILING_MISSING)
			{
				break;
			}

			prioceiling_missing = true;
		}

		if ((ret = _mutexgear_lockattr_getprotocol(__mutexattr, &protocol_value)) != EOK)
		{
			break;
		}

		if (!pshared_missing && (ret = _mutexgear_lockattr_setpshared(&__attr->lock_attr, pshared_value)) != EOK)
		{
			break;
		}

		// The priority ceiling of 0 means that there is no priority ceiling defined (the default) and some targets fail with EINVAL when when attempting to assign the 0.
		if (!prioceiling_missing && prioceiling_value != 0 && (ret = _mutexgear_lockattr_setprioceiling(&__attr->lock_attr, prioceiling_value)) != EOK)
		{
			break;
		}

		if ((ret = _mutexgear_lockattr_setprotocol(&__attr->lock_attr, protocol_value)) != EOK)
		{
			break;
		}

		MG_ASSERT(ret == EOK);
	}
	while (false);

	return ret;
}


//////////////////////////////////////////////////////////////////////////
// MaintLock Implementation

static int _mutexgear_maintlock_init(mutexgear_maintlock_t *__maintlock, mutexgear_maintlockattr_t *__attr/*=NULL*/);

/*extern */
int mutexgear_maintlock_init(mutexgear_maintlock_t *__maintlock, mutexgear_maintlockattr_t *__attr/*=NULL*/)
{
	return _mutexgear_maintlock_init(__maintlock, __attr);
}


static
int _mutexgear_maintlock_init(mutexgear_maintlock_t *__maintlock, mutexgear_maintlockattr_t *__attr/*=NULL*/)
{
	bool success = false;
	int ret, genattr_destroy_status, queue_destroy_status;

	mutexgear_completion_genattr_t genattr;
	bool genattr_was_allocated = false, acquired_reads_were_allocated = false;

	do
	{
		const unsigned int mode_flags = __attr != NULL ? __attr->mode_flags : 0;

		if ((mode_flags & ~_MUTEXGEAR_MAINTLOCK_MODE__ALLOWED_FLAGS) != 0)
		{
			ret = EINVAL;
			break;
		}

		if (__attr != NULL)
		{
			if ((ret = _mutexgear_completion_genattr_init(&genattr)) != EOK)
			{
				break;
			}
		}
		genattr_was_allocated = true;

		if (__attr != NULL)
		{
			if ((ret = _mutexgear_completion_genattr_setmutexattr(&genattr, &__attr->lock_attr)) != EOK)
			{
				break;
			}
		}

		if ((ret = _mutexgear_completion_drainablequeue_init(&__maintlock->acquired_reads, __attr != NULL ? &genattr : NULL)) != EOK)
		{
			break;
		}
		acquired_reads_were_allocated = true;

		if ((ret = _mutexgear_completion_queue_init(&__maintlock->awaited_reads, __attr != NULL ? &genattr : NULL)) != EOK)
		{
			break;
		}

		if (__attr != NULL)
		{
			MG_CHECK(genattr_destroy_status, (genattr_destroy_status = _mutexgear_completion_genattr_destroy(&genattr)) == EOK); // This should succeed normally
		}

		_mg_atomic_construct_ptrdiff(_MG_PA_PTRDIFF(&__maintlock->fl_un.mode_and_lock_flags), mode_flags);

		success = true;
	}
	while (false);

	if (!success)
	{
		if (genattr_was_allocated)
		{
			if (acquired_reads_were_allocated)
			{
				MG_CHECK(queue_destroy_status, (queue_destroy_status = _mutexgear_completion_drainablequeue_destroy(&__maintlock->acquired_reads)) == EOK);
			}

			if (__attr != NULL)
			{
				MG_CHECK(genattr_destroy_status, (genattr_destroy_status = _mutexgear_completion_genattr_destroy(&genattr)) == EOK); // This should succeed normally
			}
		}
	}

	return success ? EOK : ret;
}


static int _mutexgear_maintlock_destroy(mutexgear_maintlock_t *__maintlock);

/*extern */
int mutexgear_maintlock_destroy(mutexgear_maintlock_t *__maintlock)
{
	return _mutexgear_maintlock_destroy(__maintlock);
}

static
int _mutexgear_maintlock_destroy(mutexgear_maintlock_t *__maintlock)
{
	bool success = false;
	int ret;

	bool acquired_reads_prepared = false;

	do
	{
		const unsigned int mode_and_lock_flags = (unsigned int)_mg_atomic_load_relaxed_ptrdiff(_MG_PCVA_PTRDIFF(&__maintlock->fl_un.mode_and_lock_flags));

		if ((mode_and_lock_flags & ~(_MUTEXGEAR_MAINTLOCK_MODE__ALLOWED_FLAGS | _MUTEXGEAR_MAINTLOCK_LOCK__ALLOWED_FLAGS)) != 0)
		{
			ret = EINVAL;
			break;
		}

		if ((mode_and_lock_flags & (_MUTEXGEAR_MAINTLOCK_LOCKED_FOR_UPDATE | _MUTEXGEAR_MAINTLOCK_LOCKED_FOR_WAIT)) != 0)
		{
			ret = EBUSY;
			break;
		}

		// Try "preparing" each of the contained objects to verify its validity 
		// before starting to destroy anything.

		if ((ret = _mutexgear_completion_drainablequeue_preparedestroy(&__maintlock->acquired_reads)) != EOK)
		{
			break;
		}
		acquired_reads_prepared = true;

		if ((ret = _mutexgear_completion_queue_preparedestroy(&__maintlock->awaited_reads)) != EOK)
		{
			break;
		}

		// After all the members have been verified to be valid, it is OK to proceed with destructions 
		// and assume they must succeed.

		_mutexgear_completion_queue_completedestroy(&__maintlock->awaited_reads);
		_mutexgear_completion_drainablequeue_completedestroy(&__maintlock->acquired_reads);
		_mg_atomic_destroy_ptrdiff(_MG_PA_PTRDIFF(&__maintlock->fl_un.mode_and_lock_flags));

		success = true;
	}
	while (false);

	if (!success)
	{
		if (acquired_reads_prepared)
		{
			_mutexgear_completion_drainablequeue_unpreparedestroy(&__maintlock->acquired_reads);
		}
	}

	return success ? EOK : ret;
}


static int _mutexgear_maintlock_set_maintenance(mutexgear_maintlock_t *__maintlock);
static int _mutexgear_maintlock_clear_maintenance(mutexgear_maintlock_t *__maintlock);
static int _mutexgear_maintlock_test_maintenance(const mutexgear_maintlock_t *__maintlock);

/*extern */
int mutexgear_maintlock_set_maintenance(mutexgear_maintlock_t *__maintlock)
{
	return _mutexgear_maintlock_set_maintenance(__maintlock);
}

/*extern */
int mutexgear_maintlock_clear_maintenance(mutexgear_maintlock_t *__maintlock)
{
	return _mutexgear_maintlock_clear_maintenance(__maintlock);
}

/*extern */
int mutexgear_maintlock_test_maintenance(const mutexgear_maintlock_t *__maintlock)
{
	return _mutexgear_maintlock_test_maintenance(__maintlock);
}


static
int _mutexgear_maintlock_set_maintenance(mutexgear_maintlock_t *__maintlock)
{
	bool success = false;
	int ret;

	do 
	{
		const unsigned int preliminary_lock_flags = (unsigned int)_mg_atomic_load_relaxed_ptrdiff(_MG_PCVA_PTRDIFF(&__maintlock->fl_un.mode_and_lock_flags));

		if ((preliminary_lock_flags & ~(_MUTEXGEAR_MAINTLOCK_MODE__ALLOWED_FLAGS | _MUTEXGEAR_MAINTLOCK_LOCK__ALLOWED_FLAGS)) != 0)
		{
			ret = EINVAL;
			break;
		}

		if ((preliminary_lock_flags & _MUTEXGEAR_MAINTLOCK_LOCKED_FOR_UPDATE) != 0)
		{
			ret = EBUSY;
			break;
		}

		// WARNING! No protection here and the regular 'or' operation need to be used rather than the unsafe one!
		unsigned int original_lock_flags = (unsigned int)_mg_atomic_or_relaxed_ptrdiff(_MG_PVA_PTRDIFF(&__maintlock->fl_un.mode_and_lock_flags), _MUTEXGEAR_MAINTLOCK_LOCKED_FOR_UPDATE);
		if ((original_lock_flags & _MUTEXGEAR_MAINTLOCK_LOCKED_FOR_UPDATE) != 0)
		{
			ret = EBUSY;
			break;
		}

		success = true;
	}
	while (false);

	return success ? EOK : ret;
}

static
int _mutexgear_maintlock_clear_maintenance(mutexgear_maintlock_t *__maintlock)
{
	bool success = false;
	int ret;

	do
	{
		const unsigned int mode_and_lock_flags = (unsigned int)_mg_atomic_load_relaxed_ptrdiff(_MG_PCVA_PTRDIFF(&__maintlock->fl_un.mode_and_lock_flags));

		if ((mode_and_lock_flags & ~(_MUTEXGEAR_MAINTLOCK_MODE__ALLOWED_FLAGS | _MUTEXGEAR_MAINTLOCK_LOCK__ALLOWED_FLAGS)) != 0)
		{
			ret = EINVAL;
			break;
		}

		if ((mode_and_lock_flags & _MUTEXGEAR_MAINTLOCK_LOCKED_FOR_UPDATE) != 0)
		{
			// WARNING! No protection here and the regular 'and' neoperation eds to be used rather than the unsafe one!
			_mg_atomic_and_relaxed_ptrdiff(_MG_PVA_PTRDIFF(&__maintlock->fl_un.mode_and_lock_flags), ~(ptrdiff_t)_MUTEXGEAR_MAINTLOCK_LOCKED_FOR_UPDATE);
		}

		success = true;
	}
	while (false);

	return success ? EOK : ret;
}

static 
int _mutexgear_maintlock_test_maintenance(const mutexgear_maintlock_t *__maintlock)
{
	bool success = false, lock_status;
	int ret;

	do
	{
		const unsigned int mode_and_lock_flags = (unsigned int)_mg_atomic_load_relaxed_ptrdiff(_MG_PCVA_PTRDIFF(&__maintlock->fl_un.mode_and_lock_flags));

		if ((mode_and_lock_flags & ~(_MUTEXGEAR_MAINTLOCK_MODE__ALLOWED_FLAGS | _MUTEXGEAR_MAINTLOCK_LOCK__ALLOWED_FLAGS)) != 0)
		{
			ret = EINVAL;
			break;
		}

		lock_status = (mode_and_lock_flags & _MUTEXGEAR_MAINTLOCK_LOCKED_FOR_UPDATE) != 0;

		success = true;
	}
	while (false);

	return success ? lock_status : -ret;
}


static int _mutexgear_maintlock_tryrdlock(mutexgear_maintlock_t *__maintlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_item_t *__item, mutexgear_completion_drainidx_t *__out_queue_drain_index);

/*extern */
int mutexgear_maintlock_tryrdlock(mutexgear_maintlock_t *__maintlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_item_t *__item, mutexgear_maintlock_rdlock_token_t *__out_lock_token)
{
	mutexgear_completion_drainidx_t *pout_queue_drain_index = __out_lock_token;

	return _mutexgear_maintlock_tryrdlock(__maintlock, __worker, __item, pout_queue_drain_index);
}

static 
int _mutexgear_maintlock_tryrdlock(mutexgear_maintlock_t *__maintlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_item_t *__item, mutexgear_completion_drainidx_t *__out_queue_drain_index)
{
	bool success = false;
	int ret, mutex_unlock_status;

	bool item_prestarted = false, acquired_reads_locked = false;

	do
	{
		unsigned int preliminary_mode_and_lock_flags = (unsigned int)_mg_atomic_load_relaxed_ptrdiff(_MG_PCVA_PTRDIFF(&__maintlock->fl_un.mode_and_lock_flags));

		if ((preliminary_mode_and_lock_flags & _MUTEXGEAR_MAINTLOCK_LOCKED_FOR_UPDATE) != 0)
		{
			ret = EBUSY;
			break;
		}

		mutexgear_completion_item_prestart(__item, __worker);
		item_prestarted = true;

		mutexgear_completion_locktoken_t reads_lock_storage, *reads_lock_ptr = NULL;
		MG_ASSERT((reads_lock_ptr = &reads_lock_storage, true));

		if ((ret = _mutexgear_completion_drainablequeue_lock(reads_lock_ptr, &__maintlock->acquired_reads)) != EOK)
		{
			break;
		}
		acquired_reads_locked = true;

		unsigned int locked_mode_and_lock_flags = (unsigned int)_mg_atomic_load_relaxed_ptrdiff(_MG_PCVA_PTRDIFF(&__maintlock->fl_un.mode_and_lock_flags));

		if ((locked_mode_and_lock_flags & _MUTEXGEAR_MAINTLOCK_LOCKED_FOR_UPDATE) != 0)
		{
			ret = EBUSY;
			break;
		}

		_mutexgear_completion_drainablequeue_unsafeenqueue(&__maintlock->acquired_reads, __item, __out_queue_drain_index);
		MG_ASSERT(reads_lock_storage == MUTEXGEAR_COMPLETION_ACQUIRED_LOCKTOKEN);
		MG_DO_NOTHING(reads_lock_storage); // To suppress unused variable compiler warning

		MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_drainablequeue_plainunlock(&__maintlock->acquired_reads)) == EOK); // Should succeed normally

		success = true;
	}
	while (false);

	if (!success)
	{
		if (item_prestarted)
		{
			if (acquired_reads_locked)
			{
				MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_drainablequeue_plainunlock(&__maintlock->acquired_reads)) == EOK); // Should succeed normally
			}

			_mutexgear_completion_item_reinit(__item);
		}
	}

	return success ? EOK : ret;
}


static int _mutexgear_maintlock_rdunlock(mutexgear_maintlock_t *__maintlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_item_t *__item, mutexgear_completion_drainidx_t __original_drain_index);

/*extern */
int mutexgear_maintlock_rdunlock(mutexgear_maintlock_t *__maintlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_item_t *__item, const mutexgear_maintlock_rdlock_token_t *__lock_token)
{
	const mutexgear_completion_drainidx_t *p_queue_drain_index = __lock_token;

	return _mutexgear_maintlock_rdunlock(__maintlock, __worker, __item, *p_queue_drain_index);
}

static 
int _mutexgear_maintlock_rdunlock(mutexgear_maintlock_t *__maintlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_item_t *__item, mutexgear_completion_drainidx_t __original_drain_index)
{
	bool success = false;
	int ret, mutex_unlock_status;

	do
	{
		bool drain_index_matched = false;

		mutexgear_completion_drainidx_t preview_drain_index;
		_mutexgear_completion_drainablequeue_atomicgetindex(&preview_drain_index, &__maintlock->acquired_reads);

		if (preview_drain_index == __original_drain_index)
		{
			mutexgear_completion_locktoken_t reads_lock_storage, *reads_lock_ptr = NULL;
			MG_ASSERT((reads_lock_ptr = &reads_lock_storage, true));

			if ((ret = _mutexgear_completion_drainablequeue_lock(reads_lock_ptr, &__maintlock->acquired_reads)) != EOK)
			{
				break;
			}

			mutexgear_completion_drainidx_t current_drain_index;
			_mutexgear_completion_drainablequeue_unsafegetindex(&current_drain_index, &__maintlock->acquired_reads);

			if (current_drain_index == __original_drain_index)
			{
				_mutexgear_completion_drainablequeueditem_unsafefinish__locked(&__maintlock->acquired_reads, __item, MUTEXGEAR_COMPLETION_INVALID_DRAINIDX, NULL);
				MG_ASSERT(reads_lock_storage == MUTEXGEAR_COMPLETION_ACQUIRED_LOCKTOKEN);
				MG_DO_NOTHING(reads_lock_storage); // To suppress unused variable compiler warning

				MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_drainablequeue_plainunlock(&__maintlock->acquired_reads)) == EOK); // Should succeed normally

				_mutexgear_completion_drainablequeueditem_unsafefinish__unlocked(&__maintlock->acquired_reads, __item, __worker);

				drain_index_matched = true;
			}
		}

		if (!drain_index_matched)
		{
			mutexgear_completion_locktoken_t awaiteds_lock_storage, *awaiteds_lock_ptr = NULL;
			MG_ASSERT((awaiteds_lock_ptr = &awaiteds_lock_storage, true));

			if ((ret = _mutexgear_completion_queue_lock(awaiteds_lock_ptr, &__maintlock->awaited_reads)) != EOK)
			{
				break;
			}

			_mutexgear_completion_queueditem_unsafefinish__locked(__item);
			MG_ASSERT(awaiteds_lock_storage == MUTEXGEAR_COMPLETION_ACQUIRED_LOCKTOKEN);
			MG_DO_NOTHING(awaiteds_lock_storage); // To suppress unused variable compiler warning

			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__maintlock->awaited_reads)) == EOK); // Should succeed normally

			_mutexgear_completion_queueditem_unsafefinish__unlocked(&__maintlock->awaited_reads, __item, __worker);
		}

		success = true;
	}
	while (false);

	return success ? EOK : ret;
}


static int _mutexgear_maintlock_wait_rdunlock(mutexgear_maintlock_t *__maintlock,
	mutexgear_completion_waiter_t *__waiter);

/*extern */
int mutexgear_maintlock_wait_rdunlock(mutexgear_maintlock_t *__maintlock,
	mutexgear_completion_waiter_t *__waiter)
{
	return _mutexgear_maintlock_wait_rdunlock(__maintlock, __waiter);
}


static bool maintlock_process_and_unlock_awaited_reads_queue(mutexgear_maintlock_t *__maintlock,
	mutexgear_completion_waiter_t *__waiter, int *__out_status);

static 
int _mutexgear_maintlock_wait_rdunlock(mutexgear_maintlock_t *__maintlock,
	mutexgear_completion_waiter_t *__waiter)
{
	bool success = false;
	int ret, mutex_unlock_status;

	bool awaiteds_locked = false, business_recorded = false;

	do 
	{
		bool any_reads = false;

		mutexgear_completion_locktoken_t awaiteds_lock_storage, *awaiteds_lock_ptr = NULL;
		MG_ASSERT((awaiteds_lock_ptr = &awaiteds_lock_storage, true));

		// Lock the awaited reads first to minimize time the acquired reads lock will be held...
		if ((ret = _mutexgear_completion_queue_lock(awaiteds_lock_ptr, &__maintlock->awaited_reads)) != EOK)
		{
			break;
		}
		awaiteds_locked = true;
		// ... then set the business bit and check for invalid state...
		const unsigned int original_lock_flags = (unsigned int)_mg_atomic_unsafeor_relaxed_ptrdiff(_MG_PVA_PTRDIFF(&__maintlock->fl_un.mode_and_lock_flags), _MUTEXGEAR_MAINTLOCK_LOCKED_FOR_WAIT);
		if ((original_lock_flags & _MUTEXGEAR_MAINTLOCK_LOCKED_FOR_WAIT) != 0)
		{
			ret = EBUSY;
			break;
		}
		business_recorded = true;
		// ... and then...
		mutexgear_completion_locktoken_t reads_lock_storage, *reads_lock_ptr = NULL;
		MG_ASSERT((reads_lock_ptr = &reads_lock_storage, true));
		// ... lock the acquired reads
		if ((ret = _mutexgear_completion_drainablequeue_lock(reads_lock_ptr, &__maintlock->acquired_reads)) != EOK)
		{
			break;
		}

		if (!_mutexgear_completion_drainablequeue_lodisempty(&__maintlock->acquired_reads))
		{
			_mutexgear_completion_drainablequeue_unsafedsplice__locked(&__maintlock->acquired_reads, &__maintlock->awaited_reads);
			MG_ASSERT(reads_lock_storage == MUTEXGEAR_COMPLETION_ACQUIRED_LOCKTOKEN);
			MG_DO_NOTHING(reads_lock_storage); // To suppress unused variable compiler warning
			MG_ASSERT(awaiteds_lock_storage == MUTEXGEAR_COMPLETION_ACQUIRED_LOCKTOKEN);
			MG_DO_NOTHING(awaiteds_lock_storage); // To suppress unused variable compiler warning

			any_reads = true;
		}

		MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_drainablequeue_plainunlock(&__maintlock->acquired_reads)) == EOK); // Should succeed normally

		if (any_reads)
		{
			if (!maintlock_process_and_unlock_awaited_reads_queue(__maintlock, __waiter, &ret))
			{
				awaiteds_locked = false; // by protocol, the queue is to be unlocked even in case of of a failure
				break;
			}

			awaiteds_locked = false;
		}

		if (awaiteds_locked)
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__maintlock->awaited_reads)) == EOK); // Should succeed normally
		}

		// NOTE! 
		// It may look as a race to to be perforing unsafe 'and' here, after the lock is released...
		// But actually, the unsafe 'or' and the function entry, can't perform its addition 
		// until it would see the this 'and' operation to commit its result.
		// 
		// Also see the commentary in the error hander below.
		_mg_atomic_unsafeand_relaxed_ptrdiff(_MG_PVA_PTRDIFF(&__maintlock->fl_un.mode_and_lock_flags), ~(ptrdiff_t)_MUTEXGEAR_MAINTLOCK_LOCKED_FOR_WAIT);

		success = true;
	}
	while (false);

	if (!success)
	{
		if (awaiteds_locked)
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__maintlock->awaited_reads)) == EOK); // Should succeed normally
		}
		// WARNING! The states can be cleared independently and the handlers must not be nested
		if (business_recorded)
		{
			// NOTE! 
			// See the commentary above regarding the unsafe 'and' operation being performed out of lock
			_mg_atomic_unsafeand_relaxed_ptrdiff(_MG_PVA_PTRDIFF(&__maintlock->fl_un.mode_and_lock_flags), ~(ptrdiff_t)_MUTEXGEAR_MAINTLOCK_LOCKED_FOR_WAIT);
		}
	}

	return success ? EOK : ret;
}


static 
bool maintlock_process_and_unlock_awaited_reads_queue(mutexgear_maintlock_t *__maintlock, 
	mutexgear_completion_waiter_t *__waiter, int *__out_status)
{
	MG_ASSERT(!_mutexgear_completion_queue_lodisempty(&__maintlock->awaited_reads));

	bool fault = false;
	int ret, mutex_unlock_status;

	mutexgear_completion_item_t *head_item = _mutexgear_completion_queue_unsafegetunsafehead(&__maintlock->awaited_reads);
	for (; ; )
	{
		if ((ret = _mutexgear_completion_queue_unlockandwait(&__maintlock->awaited_reads, head_item, __waiter)) != EOK)
		{
			fault = true;
			break;
		}

		if (_mutexgear_completion_queue_lodisempty(&__maintlock->awaited_reads))
		{
			break;
		}

		if ((ret = _mutexgear_completion_queue_lock(NULL, &__maintlock->awaited_reads)) != EOK)
		{
			fault = true;
			break;
		}

		if (!_mutexgear_completion_queue_unsafegethead(&head_item, &__maintlock->awaited_reads))
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__maintlock->awaited_reads)) == EOK); // Should succeed normally
			break;
		}
	}

	MG_ASSERT(fault || _mutexgear_completion_queue_lodisempty(&__maintlock->awaited_reads));

	return !fault || (*__out_status = ret, false);
}


//////////////////////////////////////////////////////////////////////////
