/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear RWLock API Implementation                                  */
/*                                                                      */
/* WARNING!                                                             */
/* This library contains a synchronization technique protected by       */
/* the U.S. Parent 9,983,913.                                           */
/*                                                                      */
/* THIS IS A PRE-RELEASE LIBRARY SNAPSHOT FOR EVALUATION PURPOSES ONLY. */
/* AWAIT THE RELEASE AT https://mutexgear.com                           */
/*                                                                      */
/* Copyright (c) 2016-2019 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/


/**
 *	\file
 *	\brief MutexGear RWLock API Implementation
 *
 */


#include <mutexgear/rwlock.h>
#include "completion.h"
#include "utility.h"


 // A macro to derive mutexgear_rwlock_t::reader_push_locks array index from waiter pointer
#define _MUTEXGEAR_RWLOCK_MAKE_READER_PUSH_SELECTOR(Pointer) ((uintptr_t)(Pointer) / (size_t)_MUTEXGEAR_RWLOCK_READERPUSHSELECTOR_FACTOR)
#define _MUTEXGEAR_RWLOCK_ACCESS_READER_PUSH_LOCK(RWLock, Selector) ((RWLock)->reader_push_locks[(_MUTEXGEAR_RWLOCK_READERPUSHLOCK_MAXCOUNT - 1) - (Selector)])

 // Use a whole byte for the mask as that allows compiler to generate a byte memory access instead of masking
#define _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_MASK		0x00FF
#define _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_SHIFT	0

#define _MUTEXGEAR_RWLOCK_MODE_PSHARED					0x0100

#define _MUTEXGEAR_RWLOCK_MODE__ALLOWED_FLAGS			(_MUTEXGEAR_RWLOCK_MODE_PSHARED | ((_MUTEXGEAR_RWLOCK_READERPUSHLOCK_MAXCOUNT - 1) << _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_SHIFT))
MG_STATIC_ASSERT((((_MUTEXGEAR_RWLOCK_READERPUSHLOCK_MAXCOUNT - 1) | _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_MASK)) == _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_MASK);


#define _MUTEXGEAR_ERRNO__RWLOCK_ALLITEMSBUSY		EOK // A special status to indicate that all items are busy -- must not match any error codes that may appear naturally


//////////////////////////////////////////////////////////////////////////
// RWLock Attributes Implementation

 /*extern */
int mutexgear_rwlockattr_init(mutexgear_rwlockattr_t *__attr)
{
	__attr->mode_flags = 0;
	int ret = _mutexgear_lockattr_init(&__attr->lock_attr);
	return ret;
}

/*extern */
int mutexgear_rwlockattr_destroy(mutexgear_rwlockattr_t *__attr)
{
	int ret = _mutexgear_lockattr_destroy(&__attr->lock_attr);
	return ret;
}


/*extern */
int mutexgear_rwlockattr_getpshared(const mutexgear_rwlockattr_t *__attr, int *__out_pshared)
{
	int ret = _mutexgear_lockattr_getpshared(&__attr->lock_attr, __out_pshared);
	return ret;
}

/*extern */
int mutexgear_rwlockattr_setpshared(mutexgear_rwlockattr_t *__attr, int __pshared)
{
	int ret = _mutexgear_lockattr_setpshared(&__attr->lock_attr, __pshared);

	if (ret == EOK)
	{
		__attr->mode_flags = __pshared == MUTEXGEAR_PROCESS_SHARED ? __attr->mode_flags | _MUTEXGEAR_RWLOCK_MODE_PSHARED : __attr->mode_flags & ~_MUTEXGEAR_RWLOCK_MODE_PSHARED;
	}

	return ret;
}


/*extern */
int mutexgear_rwlockattr_getprioceiling(const mutexgear_rwlockattr_t *__attr, int *__out_prioceiling)
{
	int ret = _mutexgear_lockattr_getprioceiling(&__attr->lock_attr, __out_prioceiling);
	return ret;
}

/*extern */
int mutexgear_rwlockattr_getprotocol(const mutexgear_rwlockattr_t *__attr, int *__out_protocol)
{
	int ret = _mutexgear_lockattr_getprotocol(&__attr->lock_attr, __out_protocol);
	return ret;
}

/*extern */
int mutexgear_rwlockattr_setprioceiling(mutexgear_rwlockattr_t *__attr, int __prioceiling)
{
	int ret = _mutexgear_lockattr_setprioceiling(&__attr->lock_attr, __prioceiling);
	return ret;
}

/*extern */
int mutexgear_rwlockattr_setprotocol(mutexgear_rwlockattr_t *__attr, int __protocol)
{
	int ret = _mutexgear_lockattr_setprotocol(&__attr->lock_attr, __protocol);
	return ret;
}

/*extern */
int mutexgear_rwlockattr_setmutexattr(mutexgear_rwlockattr_t *__attr, const _MUTEXGEAR_LOCKATTR_T *__mutexattr)
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

		if ((ret = _mutexgear_lockattr_getprioceiling(__mutexattr, &prioceiling_value)) != EOK)
		{
			break;
		}

		if ((ret = _mutexgear_lockattr_getprotocol(__mutexattr, &protocol_value)) != EOK)
		{
			break;
		}

		if (!pshared_missing && (ret = _mutexgear_lockattr_setpshared(&__attr->lock_attr, pshared_value)) != EOK)
		{
			break;
		}

		if ((ret = _mutexgear_lockattr_setprioceiling(&__attr->lock_attr, prioceiling_value)) != EOK)
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


/*extern */
int mutexgear_rwlockattr_setwritechannels(mutexgear_rwlockattr_t *__attr, unsigned int __channel_count)
{
	unsigned int channel_count = __channel_count <= 1 ? 1
		: __channel_count <= 2 ? 2
		: 4;
	MG_STATIC_ASSERT(_MUTEXGEAR_RWLOCK_READERPUSHLOCK_MAXCOUNT == 4);

	__attr->mode_flags = (__attr->mode_flags & ~(_MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_MASK << _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_SHIFT))
		| ((channel_count - 1) << _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_SHIFT);
	MG_ASSERT(((channel_count - 1) | _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_MASK) == _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_MASK);

	return EOK;
}

/*extern */
int mutexgear_rwlockattr_getwritechannels(mutexgear_rwlockattr_t *__attr, unsigned int *__out_channel_count)
{
	unsigned channnel_mask = (__attr->mode_flags >> _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_SHIFT) & _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_MASK;
	*__out_channel_count = channnel_mask + 1;
	return EOK;
}


//////////////////////////////////////////////////////////////////////////
// RWLock Implementation

typedef enum _rwlock_rdlock_item_tag
{
	rdlock_itemtag_beingwaited,

} rwlock_rdlock_item_tag_t;


/*extern */
int mutexgear_rwlock_init(mutexgear_rwlock_t *__rwlock, mutexgear_rwlockattr_t *__attr/*=NULL*/)
{
	bool success = false;
	int ret, mutex_destroy_status, genattr_destroy_status, drainablequeue_destroy_status, queue_destroy_status;

	mutexgear_completion_genattr_t genattr;
	unsigned int mutex_index = 0, mutex_count = 0;
	bool readers_push_was_allocated = false, genattr_was_allocated = false, acquired_reads_were_allocated = false;
	bool waiting_writes_were_allocated = false, waiting_reads_were_allocated = false;

	do
	{
		const unsigned int mode_flags = __attr != NULL ? __attr->mode_flags : 0;

		if ((mode_flags & ~_MUTEXGEAR_RWLOCK_MODE__ALLOWED_FLAGS) != 0)
		{
			ret = EINVAL;
			break;
		}

		__rwlock->mode_flags = mode_flags;

		mutex_count = ((mode_flags >> _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_SHIFT) & _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_MASK) + 1;
		MG_ASSERT(!readers_push_was_allocated);
		MG_ASSERT(mutex_index == 0);

		for (/*readers_push_was_allocated = false*/; !readers_push_was_allocated; readers_push_was_allocated = ++mutex_index == mutex_count)
		{
			if ((ret = _mutexgear_lock_init(&_MUTEXGEAR_RWLOCK_ACCESS_READER_PUSH_LOCK(__rwlock, mutex_index), __attr != NULL ? &__attr->lock_attr : NULL)) != EOK)
			{
				break;
			}
		}
		if (!readers_push_was_allocated)
		{
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

		if ((ret = _mutexgear_completion_queue_init(&__rwlock->acquired_reads, __attr != NULL ? &genattr : NULL)) != EOK)
		{
			break;
		}
		acquired_reads_were_allocated = true;

		if ((ret = _mutexgear_completion_queue_init(&__rwlock->waiting_writes, __attr != NULL ? &genattr : NULL)) != EOK)
		{
			break;
		}
		waiting_writes_were_allocated = true;

		if ((ret = _mutexgear_completion_drainablequeue_init(&__rwlock->waiting_reads, __attr != NULL ? &genattr : NULL)) != EOK)
		{
			break;
		}
		waiting_reads_were_allocated = true;

		if ((ret = _mutexgear_completion_drain_init(&__rwlock->read_wait_drain)) != EOK)
		{
			break;
		}

		if (__attr != NULL)
		{
			MG_CHECK(genattr_destroy_status, (genattr_destroy_status = _mutexgear_completion_genattr_destroy(&genattr)) == EOK); // This should succeed normally
		}
		// wheelattr_was_allocated = false;

		ret = EOK;

		success = true;
	}
	while (false);

	if (!success)
	{
		if (readers_push_was_allocated)
		{
			if (genattr_was_allocated)
			{
				if (acquired_reads_were_allocated)
				{
					if (waiting_writes_were_allocated)
					{
						if (waiting_reads_were_allocated)
						{
							MG_CHECK(drainablequeue_destroy_status, (drainablequeue_destroy_status = _mutexgear_completion_drainablequeue_destroy(&__rwlock->waiting_reads)) == EOK);
						}

						MG_CHECK(queue_destroy_status, (queue_destroy_status = _mutexgear_completion_queue_destroy(&__rwlock->waiting_writes)) == EOK);
					}

					MG_CHECK(queue_destroy_status, (queue_destroy_status = _mutexgear_completion_queue_destroy(&__rwlock->acquired_reads)) == EOK);
				}

				if (__attr != NULL)
				{
					MG_CHECK(genattr_destroy_status, (genattr_destroy_status = _mutexgear_completion_genattr_destroy(&genattr)) == EOK); // This should succeed normally
				}
			}

			MG_ASSERT(mutex_index != 0);

			--mutex_index;
			MG_CHECK(mutex_destroy_status, (mutex_destroy_status = _mutexgear_lock_destroy(&_MUTEXGEAR_RWLOCK_ACCESS_READER_PUSH_LOCK(__rwlock, mutex_index))) == EOK); // This should succeed normally
		}

		while (mutex_index != 0)
		{
			--mutex_index;
			MG_CHECK(mutex_destroy_status, (mutex_destroy_status = _mutexgear_lock_destroy(&_MUTEXGEAR_RWLOCK_ACCESS_READER_PUSH_LOCK(__rwlock, mutex_index))) == EOK); // This should succeed normally
		}
	}

	return ret;
}


/*extern */
int mutexgear_rwlock_destroy(mutexgear_rwlock_t *__rwlock)
{
	int ret, mutex_unlock_status, mutex_destroy_status;

	unsigned int mutex_index, mutex_count;
	bool drain_prepared = false, waiting_reads_prepared = false, waiting_writes_prepared = false, acquired_reads_prepared = false, push_locks_acquired = false;

	do 
	{
		const unsigned int mode_flags = __rwlock->mode_flags;

		if ((mode_flags & ~_MUTEXGEAR_RWLOCK_MODE__ALLOWED_FLAGS) != 0)
		{
			ret = EINVAL;
			break;
		}

		// Try "preparing" each of the contained objects to verify its validity 
		// before starting to destroy anything.

		if ((ret = _mutexgear_completion_drain_preparedestroy(&__rwlock->read_wait_drain)) != EOK)
		{
			break;
		}
		drain_prepared = true;

		if ((ret = _mutexgear_completion_drainablequeue_preparedestroy(&__rwlock->waiting_reads)) != EOK)
		{
			break;
		}
		waiting_reads_prepared = true;

		if ((ret = _mutexgear_completion_queue_preparedestroy(&__rwlock->waiting_writes)) != EOK)
		{
			break;
		}
		waiting_writes_prepared = true;

		if ((ret = _mutexgear_completion_queue_preparedestroy(&__rwlock->acquired_reads)) != EOK)
		{
			break;
		}
		acquired_reads_prepared = true;

		mutex_index = 0, mutex_count = ((mode_flags >> _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_SHIFT) & _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_MASK) + 1;
		MG_ASSERT(!push_locks_acquired);

		for (/*push_locks_acquired = false*/; !push_locks_acquired; push_locks_acquired = ++mutex_index == mutex_count)
		{
			if ((ret = _mutexgear_lock_tryacquire(&_MUTEXGEAR_RWLOCK_ACCESS_READER_PUSH_LOCK(__rwlock, mutex_index))) != EOK)
			{
				break;
			}
		}
		if (!push_locks_acquired)
		{
			break;
		}

		// After all the members have been verified to be valid, it is OK to proceed with destructions 
		// and assume they must succeed.

		bool push_locks_destroyed;
		for (push_locks_destroyed = false; !push_locks_destroyed; push_locks_destroyed = --mutex_index == 0)
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&_MUTEXGEAR_RWLOCK_ACCESS_READER_PUSH_LOCK(__rwlock, mutex_index - 1))) == EOK);
			MG_CHECK(mutex_destroy_status, (mutex_destroy_status = _mutexgear_lock_destroy(&_MUTEXGEAR_RWLOCK_ACCESS_READER_PUSH_LOCK(__rwlock, mutex_index - 1))) == EOK);
		}

		_mutexgear_completion_queue_completedestroy(&__rwlock->acquired_reads);
		_mutexgear_completion_queue_completedestroy(&__rwlock->waiting_writes);
		_mutexgear_completion_drainablequeue_completedestroy(&__rwlock->waiting_reads);
		_mutexgear_completion_drain_completedestroy(&__rwlock->read_wait_drain);

		ret = EOK;
	}
	while (false);

	if (ret != EOK)
	{
		if (drain_prepared)
		{
			if (waiting_reads_prepared)
			{
				if (waiting_writes_prepared)
				{
					if (acquired_reads_prepared)
					{
						MG_ASSERT(!push_locks_acquired);

						while (mutex_index != 0)
						{
							--mutex_index;
							MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&_MUTEXGEAR_RWLOCK_ACCESS_READER_PUSH_LOCK(__rwlock, mutex_index))) == EOK);
						}

						_mutexgear_completion_queue_unpreparedestroy(&__rwlock->acquired_reads);
					}

					_mutexgear_completion_queue_unpreparedestroy(&__rwlock->waiting_writes);
				}

				_mutexgear_completion_drainablequeue_unpreparedestroy(&__rwlock->waiting_reads);
			}

			_mutexgear_completion_drain_unpreparedestroy(&__rwlock->read_wait_drain);
		}
	}

	return ret;
}


static bool rwlock_wrlock_push_readers_waiting_to_acquire_access__single_channel(mutexgear_rwlock_t *__rwlock, mutexgear_completion_waiter_t *__waiter,
	int *__out_status);
static bool rwlock_wrlock_push_readers_waiting_to_acquire_access__many_channels(mutexgear_rwlock_t *__rwlock, mutexgear_completion_waiter_t *__waiter,
	int *__out_status);
static bool rwlock_wrlock_wait_all_reads_and_acquire_access(mutexgear_rwlock_t *__rwlock, mutexgear_completion_waiter_t *__waiter,
	mutexgear_completion_item_t	*__last_reader_item, int *__out_status);
static bool rwlock_wrlock_find_notmarked_reader_item(mutexgear_completion_item_t **__out_reader_item, 
	mutexgear_rwlock_t *__rwlock, mutexgear_completion_item_t *__reader_item);

/*extern */
int mutexgear_rwlock_wrlock(mutexgear_rwlock_t *__rwlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_waiter_t *__waiter, mutexgear_completion_item_t *__item/*=NULL*/)
{
	bool success = false;
	int ret, mutex_unlock_status, item_completion_status;

	bool item_initialized = false, wait_initialized = false, wait_inserted = false;

	mutexgear_completion_item_t wait_completion_item;
	mutexgear_completion_item_t *wait_completion_to_use = __item;

	do 
	{
		if (__item == NULL)
		{
			MG_STATIC_ASSERT(MUTEXGEAR_PROCESS_SHARED != 0);

			if ((__rwlock->mode_flags & MUTEXGEAR_PROCESS_SHARED) != 0) // In case of shared mode, item parameter must not be NULL
			{
				ret = EINVAL;
				break;
			}

			_mutexgear_completion_item_init(&wait_completion_item);
			wait_completion_to_use = &wait_completion_item;
			item_initialized = true;
		}

		bool access_acquired = false;

		if (_mutexgear_completion_queue_lodisempty(&__rwlock->acquired_reads))
		{
			if ((ret = _mutexgear_completion_queue_lock(NULL, &__rwlock->acquired_reads)) != EOK)
			{
				break;
			}

			if (_mutexgear_completion_queue_lodisempty(&__rwlock->acquired_reads))
			{
				access_acquired = true;
			}
			else
			{
				// NOTE: Going for registering into waiting_writes with the acquired_reads unlocked
				// may result in a few extra reads passing into the object. But it is perfectly OK 
				// as those can be considered as if they would had gained the access even before the 
				// acquired_reads lock was acquired at all.
				MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__rwlock->acquired_reads)) == EOK); // Should succeed normally
			}
		}

		if (!access_acquired)
		{
			_mutexgear_completion_item_prestart(wait_completion_to_use, __worker);
			wait_initialized = true;

			if ((ret = _mutexgear_completion_queue_enqueue(&__rwlock->waiting_writes, wait_completion_to_use, NULL)) != EOK)
			{
				break;
			}
			wait_inserted = true;

			// NOTE: The overhead in rwlock_wrlock_push_readers_waiting_to_acquire_access__many_channels() is actually
			// so minor that it is unclear if it is worth making the two separate implementations. Let it be though.
			if ((__rwlock->mode_flags & _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_MASK) == 0
				? !rwlock_wrlock_push_readers_waiting_to_acquire_access__single_channel(__rwlock, __waiter, &ret)
				: !rwlock_wrlock_push_readers_waiting_to_acquire_access__many_channels(__rwlock, __waiter, &ret))
			{
				break;
			}

			MG_CHECK(item_completion_status, (item_completion_status = _mutexgear_completion_queueditem_finish(&__rwlock->waiting_writes, wait_completion_to_use, __worker, NULL)) == EOK); // Well, the item must be removed at any cost!
			// wait_inserted = false;
		}

		if (__item == NULL)
		{
			_mutexgear_completion_item_destroy(&wait_completion_item);
		}

		MG_ASSERT(__item == NULL || _mutexgear_completion_item_isasinit(__item));

		success = true;
	}
	while (false);

	if (!success)
	{
		if (wait_initialized)
		{
			if (wait_inserted)
			{
				MG_CHECK(item_completion_status, (item_completion_status = _mutexgear_completion_queueditem_finish(&__rwlock->waiting_writes, wait_completion_to_use, __worker, NULL)) == EOK); // No way to handle -- must succeed
			}

			_mutexgear_completion_item_reinit(wait_completion_to_use);
		}

		if (item_initialized)
		{
			_mutexgear_completion_item_destroy(&wait_completion_item);
		}

		MG_ASSERT(__item == NULL || _mutexgear_completion_item_isasinit(__item));
	}

	return success ? EOK : ret;
}

static
bool rwlock_wrlock_push_readers_waiting_to_acquire_access__single_channel(mutexgear_rwlock_t *__rwlock, mutexgear_completion_waiter_t *__waiter,
	int *__out_status)
{
	bool success = false;
	int ret, mutex_unlock_status;

	bool push_locked = false;

	do 
	{
		if ((ret = _mutexgear_lock_acquire(&_MUTEXGEAR_RWLOCK_ACCESS_READER_PUSH_LOCK(__rwlock, 0))) != EOK)
		{
			break;
		}
		push_locked = true;

		// The readers push lock is a likely blocking lock, so there is a reason to re-check the acquired reads.
		// This could, actually, be done within the rwlock_wrlock_wait_all_reads_and_acquire_access() 
		// but extra checking here unrolls the first pass of the loop without entering the nested function.
		// 
		// The code is going to lock acquired_reads anyway, so there is no reason to check for the queue to be empty unlocked.
		if ((ret = _mutexgear_completion_queue_lock(NULL, &__rwlock->acquired_reads)) != EOK)
		{
			break;
		}

		mutexgear_completion_item_t	*last_reader_item;
		if (_mutexgear_completion_queue_gettail(&last_reader_item, &__rwlock->acquired_reads) // This is equivalent for the emptiness check but the code will make use of the item without extra atomic access
			&& !rwlock_wrlock_wait_all_reads_and_acquire_access(__rwlock, __waiter, last_reader_item, &ret))
		{
			// The unlock for acquired_reads in the call above is not allowed to fail

			// Also, _MUTEXGEAR_RWLOCK_EALLITEMSBUSY cannot be returned
			MG_ASSERT(ret != _MUTEXGEAR_ERRNO__RWLOCK_ALLITEMSBUSY);

			break;
		}

		MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&_MUTEXGEAR_RWLOCK_ACCESS_READER_PUSH_LOCK(__rwlock, 0)) == EOK)); // Should succeed normally
		// push_locked = false;

		success = true;
	}
	while (false);

	if (!success)
	{
		if (push_locked)
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&_MUTEXGEAR_RWLOCK_ACCESS_READER_PUSH_LOCK(__rwlock, 0))) == EOK); // Should succeed normally
		}
	}

	return success || (*__out_status = ret, false);
}

static 
bool rwlock_wrlock_push_readers_waiting_to_acquire_access__many_channels(mutexgear_rwlock_t *__rwlock, mutexgear_completion_waiter_t *__waiter,
	int *__out_status)
{
	bool fault = false;
	int ret, mutex_unlock_status;

	const unsigned int lock_index_mask = (__rwlock->mode_flags & _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_MASK) >> _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_SHIFT;
	unsigned int starting_push_lock_index = _MUTEXGEAR_RWLOCK_MAKE_READER_PUSH_SELECTOR(__waiter);

	unsigned int push_lock_index;
	for (push_lock_index = starting_push_lock_index; ; )
	{
		if ((ret = _mutexgear_lock_acquire(&_MUTEXGEAR_RWLOCK_ACCESS_READER_PUSH_LOCK(__rwlock, push_lock_index & lock_index_mask))) != EOK)
		{
			fault = true;
			break;
		}
		++push_lock_index;

		// The readers push lock is a likely blocking lock, so there is a reason to re-check the acquired reads.
		// This could, actually, be done within the rwlock_wrlock_wait_all_reads_and_acquire_access() 
		// but extra checking here unrolls the first pass of the loop without entering the nested function.
		// 
		// The code is going to lock acquired_reads anyway, so there is no reason to check for the queue to be empty unlocked.
		if ((ret = _mutexgear_completion_queue_lock(NULL, &__rwlock->acquired_reads)) != EOK)
		{
			fault = true;
			break;
		}

		mutexgear_completion_item_t	*last_reader_item;
		if (!_mutexgear_completion_queue_gettail(&last_reader_item, &__rwlock->acquired_reads)) // This is equivalent for the emptiness check but the code will make use of the item without extra atomic access
		{
			break;
		}

		if (rwlock_wrlock_wait_all_reads_and_acquire_access(__rwlock, __waiter, last_reader_item, &ret))
		{
			break;
		}
		// The unlock for acquired_reads in the call above is not allowed to fail

		if (ret != _MUTEXGEAR_ERRNO__RWLOCK_ALLITEMSBUSY)
		{
			fault = true;
			break;
		}
	}

	if (!fault)
	{
		bool exit_loop;
		for (exit_loop = false; !exit_loop; exit_loop = push_lock_index == starting_push_lock_index)
		{
			--push_lock_index;

			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&_MUTEXGEAR_RWLOCK_ACCESS_READER_PUSH_LOCK(__rwlock, push_lock_index & lock_index_mask))) == EOK); // Should succeed normally
		}
	}

	if (fault)
	{
		while (push_lock_index != starting_push_lock_index)
		{
			--push_lock_index;

			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&_MUTEXGEAR_RWLOCK_ACCESS_READER_PUSH_LOCK(__rwlock, push_lock_index & lock_index_mask))) == EOK); // Should succeed normally
		}
	}

	return !fault || (*__out_status = ret, false);
}

static 
bool rwlock_wrlock_wait_all_reads_and_acquire_access(mutexgear_rwlock_t *__rwlock, mutexgear_completion_waiter_t *__waiter, 
	mutexgear_completion_item_t *__last_reader_item, int *__out_status)
{
	bool fault = false;
	int ret, mutex_unlock_status;

	mutexgear_completion_item_t	*reader_item = __last_reader_item;
	MG_ASSERT(reader_item != NULL);

	// bool access_locked = true;

	bool exit_loop;
	for (exit_loop = false; !exit_loop; exit_loop = !_mutexgear_completion_queue_gettail(&reader_item, &__rwlock->acquired_reads))
	{
		if (!_mutexgear_completion_item_unsafemodifytag(reader_item, rdlock_itemtag_beingwaited, true)
			&& !rwlock_wrlock_find_notmarked_reader_item(&reader_item, __rwlock, reader_item))
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__rwlock->acquired_reads)) == EOK);

			ret = _MUTEXGEAR_ERRNO__RWLOCK_ALLITEMSBUSY;
			fault = true;
			break;
		}

		ret = _mutexgear_completion_queue_unlockandwait(&__rwlock->acquired_reads, reader_item, __waiter);
		// access_locked = false; // The unlock in the call above is not allowed to fail

		if (ret != EOK)
		{
			fault = true;
			break;
		}

		if ((ret = _mutexgear_completion_queue_lock(NULL, &__rwlock->acquired_reads)) != EOK)
		{
			fault = true;
			break;
		}
		// access_locked = true; -- no failure exits while the mutex is locked
	}

	return !fault || (*__out_status = ret, false);
}

static 
bool rwlock_wrlock_find_notmarked_reader_item(mutexgear_completion_item_t **__out_reader_item, 
	mutexgear_rwlock_t *__rwlock, mutexgear_completion_item_t *__reader_item)
{
	bool success = false;

	mutexgear_completion_item_t *reader_item = __reader_item;
	while (_mutexgear_completion_queue_getpreceding(&reader_item, &__rwlock->acquired_reads, reader_item))
	{
		if (_mutexgear_completion_item_unsafemodifytag(reader_item, rdlock_itemtag_beingwaited, true))
		{
			success = true;
			break;
		}
	}

	return success && (*__out_reader_item = reader_item, true);
}

/*extern */
int mutexgear_rwlock_wrunlock(mutexgear_rwlock_t *__rwlock)
{
	int ret = _mutexgear_completion_queue_plainunlock(&__rwlock->acquired_reads);
	return ret;
}


static bool rwlock_rdlock_wait_all_writes_and_acquire_access(mutexgear_rwlock_t *__rwlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_waiter_t *__waiter, mutexgear_completion_drainableitem_t *__item,
	mutexgear_completion_locktoken_t *__out_readers_lock, int *__out_status);
static bool rwlock_rdlock_wait_write_wait_emptiness(mutexgear_rwlock_t *__rwlock, 
	mutexgear_completion_waiter_t *__waiter, mutexgear_completion_item_t *last_write_wait,
	/*bool *__out_waiting_writes_remain_locked, */int *__out_status);

/*extern */
int mutexgear_rwlock_rdlock(mutexgear_rwlock_t *__rwlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_waiter_t *__waiter, mutexgear_completion_drainableitem_t *__item)
{
	bool success = false;
	int ret, mutex_unlock_status;

	bool access_locked = false, read_work_started = false;

	do 
	{
		if (_mutexgear_completion_item_gettag(&__item->basic_item, rdlock_itemtag_beingwaited))
		{
			ret = EINVAL;
			break;
		}

		mutexgear_completion_locktoken_t readers_lock;

		if (_mutexgear_completion_queue_lodisempty(&__rwlock->waiting_writes))
		{
			if ((ret = _mutexgear_completion_queue_lock(&readers_lock, &__rwlock->acquired_reads)) != EOK)
			{
				break;
			}
			// access_locked = true;

			if (_mutexgear_completion_queue_lodisempty(&__rwlock->waiting_writes))
			{
				access_locked = true;
			}
			else
			{
				MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__rwlock->acquired_reads)) == EOK); // Should succeed normally
				// access_locked = false;
				MG_ASSERT((readers_lock = NULL, true));
			}
		}

		if (!access_locked)
		{
			if (!rwlock_rdlock_wait_all_writes_and_acquire_access(__rwlock, __worker, __waiter, __item, &readers_lock, &ret))
			{
				break;
			}
			access_locked = true;
		}

		_mutexgear_completion_item_prestart(&__item->basic_item, __worker);
		read_work_started = true;

		// Use a replacement variable to help the optimizer
		const mutexgear_completion_locktoken_t _readers_lock = MUTEXGEAR_COMPLETION_ACQUIRED_LOCKTOCKEN;
		MG_ASSERT(readers_lock == _readers_lock);

		if ((ret = _mutexgear_completion_queue_enqueue(&__rwlock->acquired_reads, &__item->basic_item, _readers_lock)) != EOK)
		{
			break;
		}

		MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__rwlock->acquired_reads)) == EOK); // Should succeed normally
		// access_locked = false;

		success = true;
	}
	while (false);

	if (!success)
	{
		if (access_locked)
		{
			if (read_work_started)
			{
				_mutexgear_completion_item_reinit(&__item->basic_item);
			}

			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__rwlock->acquired_reads)) == EOK); // Should succeed normally
		}

	}

	return success ? EOK : ret;
}

static 
bool rwlock_rdlock_wait_all_writes_and_acquire_access(mutexgear_rwlock_t *__rwlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_waiter_t *__waiter, mutexgear_completion_drainableitem_t *__item,
	mutexgear_completion_locktoken_t *__out_readers_lock, int *__out_status)
{
	bool fault = false;
	int ret, mutex_unlock_status, read_wait_enqueue_status, item_completion_status;
	
	mutexgear_completion_locktoken_t readers_lock;
	mutexgear_completion_drainidx_t read_wait_drain_index;

	bool /*access_locked, *//*waiting_reads_locked, */wait_inserted, waiting_writes_locked;

	for (; ; )
	{
		/*access_locked = false, *//*waiting_reads_locked = false, */wait_inserted = false, waiting_writes_locked = false;

		mutexgear_completion_locktoken_t waiting_reads_lock;
		if ((ret = _mutexgear_completion_drainablequeue_lock(&waiting_reads_lock, &__rwlock->waiting_reads)) != EOK)
		{
			fault = true;
			break;
		}
		// waiting_reads_locked = true;

		// Get the status in advance as an element is going to be inserted into the queue
		bool read_wait_queue_was_empty = _mutexgear_completion_drainablequeue_lodisempty(&__rwlock->waiting_reads);

		// NOTE: Here the wait item is speculatively inserted into the waiting_reads list 
		// even though it may not be necessary there if the waiting_writes list appears to be empty after the lock below.
		// This is done to separate locked sections of waiting_reads and waiting_writes and remove dependency between them.
		// On the other hand, consider the situation that the next conditional block is entered. There was already a check above 
		// that the waiting_writes list was not empty. Since waiting_reads appears in the current function only 
		// there are no interdependencies with other RWLock operations, and since the read_wait queue appeared empty
		// (the conditional block is entered by assumption) the current thread was the first to acquire the waiting_reads lock above 
		// and there was no contest for it. So the lock above was quick and the waiting_writes list can be assumed to remain not empty.
		// The speculative insertion looks to be reasonable and is very likely to be confirmed to be necessary.
		_mutexgear_completion_drainableitem_prestart(__item, __worker);

		const mutexgear_completion_locktoken_t _waiting_reads_lock = MUTEXGEAR_COMPLETION_ACQUIRED_LOCKTOCKEN;
		MG_ASSERT(waiting_reads_lock == _waiting_reads_lock);

		MG_CHECK(read_wait_enqueue_status, (read_wait_enqueue_status = _mutexgear_completion_drainablequeue_enqueue(&__rwlock->waiting_reads, __item, _waiting_reads_lock, &read_wait_drain_index)) == EOK); // The object has already been verified in the preceding calls
		wait_inserted = true;

		if (read_wait_queue_was_empty)
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_drainablequeue_plainunlock(&__rwlock->waiting_reads)) == EOK); // Should succeed normally
			// waiting_reads_locked = false;
			MG_ASSERT((waiting_reads_lock = NULL, true));

			if ((ret = _mutexgear_completion_queue_lock(NULL, &__rwlock->waiting_writes)) != EOK)
			{
				fault = true;
				break;
			}
			waiting_writes_locked = true;

			mutexgear_completion_item_t	*last_write_wait;
			if (_mutexgear_completion_queue_gettail(&last_write_wait, &__rwlock->waiting_writes)) // This is equivalent for the emptiness check but the code will make use of the item later
			{
				// bool waiting_writes_remain_locked;
				if (!rwlock_rdlock_wait_write_wait_emptiness(__rwlock, __waiter, last_write_wait, /*&waiting_writes_remain_locked, */&ret))
				{
					// if (waiting_writes_remain_locked)
					// {
					// 	MG_CHECK(mutex_unlock_status, (mutex_unlock_status = mutexgear_completion_plain_unlock_queue(&__rwlock->waiting_writes)) == EOK); // Should succeed normally
					// }

					// waiting_writes_locked = false;
					fault = true;
					break;
				}

			}
			else
			{
				// Getting here means that the waiting writes have been all gone until the lock has been acquired. 
				// Just release the locks and try again from the start.
			}

			// Release the write before unlocking the readers so that all the readers had equal chances and, also, there were no readers hitting the locked write waiter queue mutex
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__rwlock->waiting_writes)) == EOK); // Should succeed normally
			// waiting_writes_locked = false;
			MG_CHECK(item_completion_status, (item_completion_status = _mutexgear_completion_drainablequeueditem_finish(&__rwlock->waiting_reads, __item, __worker, read_wait_drain_index, &__rwlock->read_wait_drain, NULL)) == EOK); // No way to handle -- must succeed
			// wait_inserted = false;
		}
		else
		{
			mutexgear_completion_drainableitem_t *previous_wait = _mutexgear_completion_drainablequeue_unsafegetpreceding(__item);
			MG_ASSERT(previous_wait != NULL);

			ret = _mutexgear_completion_drainablequeue_unlockandwait(&__rwlock->waiting_reads, previous_wait, __waiter);
			// waiting_reads_locked = false; // The unlock in the call above is not allowed to fail
			MG_ASSERT((waiting_reads_lock = NULL, true));

			if (ret != EOK)
			{
				fault = true;
				break;
			}

			MG_CHECK(item_completion_status, (item_completion_status = _mutexgear_completion_drainablequeueditem_finish(&__rwlock->waiting_reads, __item, __worker, MUTEXGEAR_COMPLETION_INVALID_DRAINIDX, NULL, NULL)) == EOK);
			// wait_inserted = false;
		}
	
		/*access_locked = false, *//*waiting_reads_locked = false, */wait_inserted = false, waiting_writes_locked = false;

		if ((ret = _mutexgear_completion_queue_lock(&readers_lock, &__rwlock->acquired_reads)) != EOK)
		{
			fault = true;
			break;
		}
		// access_locked = true;

		if (_mutexgear_completion_queue_lodisempty(&__rwlock->waiting_writes))
		{
			break;
		}

		MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__rwlock->acquired_reads)) == EOK); // Should succeed normally
		// access_locked = false;
		MG_ASSERT((readers_lock = NULL, true));
	}

	if (fault)
	{
		if (wait_inserted)
		{
			if (waiting_writes_locked)
			{
				MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__rwlock->waiting_writes)) == EOK); // Should succeed normally
			}

			MG_CHECK(item_completion_status, (item_completion_status = _mutexgear_completion_drainablequeueditem_finish(&__rwlock->waiting_reads, __item, __worker, read_wait_drain_index, &__rwlock->read_wait_drain, NULL)) == EOK); // No way to handle -- must succeed
		}
	}

	return (!fault && (*__out_readers_lock = readers_lock, true)) || (*__out_status = ret, false);
}


static 
bool rwlock_rdlock_wait_write_wait_emptiness(mutexgear_rwlock_t *__rwlock, 
	mutexgear_completion_waiter_t *__waiter, mutexgear_completion_item_t *last_write_wait,
	/*bool *__out_waiting_writes_remain_locked, */int *__out_status)
{
	bool fault = false;
	int ret;

	// bool waiting_writes_unlocked = false;

	mutexgear_completion_item_t	*write_wait = last_write_wait;
	MG_ASSERT(write_wait != NULL);

	bool exit_loop;
	for (exit_loop = false; !exit_loop; exit_loop = !_mutexgear_completion_queue_gettail(&write_wait, &__rwlock->waiting_writes))
	{
		ret = _mutexgear_completion_queue_unlockandwait(&__rwlock->waiting_writes, write_wait, __waiter);
		// waiting_writes_unlocked = true; // The unlock in the call above is not allowed to fail

		if (ret != EOK)
		{
			fault = true;
			break;
		}

		if ((ret = _mutexgear_completion_queue_lock(NULL, &__rwlock->waiting_writes)) != EOK)
		{
			fault = true;
			break;
		}
		// waiting_writes_unlocked = false;
	}

	return !fault || (/**__out_waiting_writes_remain_locked = !waiting_writes_unlocked, */*__out_status = ret, false);
}


/*extern */
int mutexgear_rwlock_rdunlock(mutexgear_rwlock_t *__rwlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_drainableitem_t *__item)
{
	int ret;

	do 
	{
		if ((ret = _mutexgear_completion_queueditem_finish(&__rwlock->acquired_reads, &__item->basic_item, __worker, NULL)) != EOK)
		{
			break;
		}

		// Clear the tag that might have been set by waiting writes
		_mutexgear_completion_item_settag(&__item->basic_item, rdlock_itemtag_beingwaited, false);

		MG_ASSERT(ret == EOK);
	}
	while (false);
	
	return ret;
}


 //////////////////////////////////////////////////////////////////////////
