/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear RWLock API Implementation                                  */
/*                                                                      */
/* WARNING!                                                             */
/* This library contains a synchronization technique protected by       */
/* the U.S. Patent 9,983,913.                                           */
/*                                                                      */
/* THIS IS A PRE-RELEASE LIBRARY SNAPSHOT.                              */
/* AWAIT THE RELEASE AT https://mutexgear.com                           */
/*                                                                      */
/* Copyright (c) 2016-2022 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/


/**
 *	\file
 *	\brief MutexGear RWLock API implementation
 *
 */


#include <mutexgear/rwlock.h>

#if defined(MUTEXGEAR_USE_C11_GENERICS)

#undef mutexgear_rwlock_init
#undef mutexgear_rwlock_destroy
#undef mutexgear_rwlock_wrlock
#undef mutexgear_rwlock_trywrlock
#undef mutexgear_rwlock_wrunlock
#undef mutexgear_rwlock_rdlock
#undef mutexgear_rwlock_tryrdlock
#undef mutexgear_rwlock_rdunlock


#endif // #if defined(MUTEXGEAR_USE_C11_GENERICS)


#include "completion.h"
#include "dlralist.h"
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
	rdlock_itemtag_trylocked,

} rwlock_rdlock_item_tag_t;


_MUTEXGEAR_PURE_INLINE
mutexgear_completion_item_t *_mutexgear_rtdl_rwlock_getseparator(const mutexgear_trdl_rwlock_t *__rwlock_instance)
{
	const _mutexgear_completion_itemdata_t *separator_data = &__rwlock_instance->tryread_queue_separator;
	return _mutexgear_completion_item_getfromworkitem(&separator_data->work_item);
}


static int _mutexgear_rwlock_init(mutexgear_rwlock_t *__rwlock, mutexgear_rwlockattr_t *__attr/*=NULL*/);

/*extern */
int mutexgear_trdl_rwlock_init(mutexgear_trdl_rwlock_t *__rwlock, mutexgear_rwlockattr_t *__attr/*=NULL*/)
{
	bool success = false;
	int ret, mutex_destroy_status;

	bool queue_lock_allocated = false;

	do
	{
		if ((ret = _mutexgear_lock_init(&__rwlock->tryread_queue_lock, __attr != NULL ? &__attr->lock_attr : NULL)) != EOK)
		{
			break;
		}
		queue_lock_allocated = true;

		if ((ret = _mutexgear_rwlock_init(&__rwlock->basic_lock, __attr)) != EOK)
		{
			break;
		}

		_mg_atomic_construct_ptrdiff(_MG_PA_PTRDIFF(&__rwlock->wrlock_waits), 0);

		_mutexgear_completion_itemdata_init(&__rwlock->tryread_queue_separator);
		// Set "beingwaited" tag for the separator item so that it would not be selected by rwlock_wrlock_find_notmarked_reader_item()
		_mutexgear_completion_itemdata_setunsafetag(&__rwlock->tryread_queue_separator, rdlock_itemtag_beingwaited, true);
		// Immediately insert the separator item into the acquired reads queue
		mutexgear_completion_item_t *tryread_queue_separator = _mutexgear_rtdl_rwlock_getseparator(__rwlock);
		_mutexgear_completion_queue_unsafeenqueue_back(&__rwlock->basic_lock.acquired_reads, tryread_queue_separator);

		success = true;
	}
	while (false);

	if (!success)
	{
		if (queue_lock_allocated)
		{
			MG_CHECK(mutex_destroy_status, (mutex_destroy_status = _mutexgear_lock_destroy(&__rwlock->tryread_queue_lock)) == EOK); // This should succeed normally
		}
	}

	return success ? EOK : ret;
}

/*extern */
int mutexgear_rwlock_init(mutexgear_rwlock_t *__rwlock, mutexgear_rwlockattr_t *__attr/*=NULL*/)
{
	return _mutexgear_rwlock_init(__rwlock, __attr);
}


static 
int _mutexgear_rwlock_init(mutexgear_rwlock_t *__rwlock, mutexgear_rwlockattr_t *__attr/*=NULL*/)
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

		__rwlock->fl_un.mode_flags = mode_flags;

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

		mutexgear_dlraitem_t *const express_reads = _mutexgear_dlraitem_getfromprevious(&__rwlock->express_reads);
		_mutexgear_dlraitem_initprevious(express_reads, express_reads);

		_mg_atomic_construct_ptrdiff(_MG_PA_PTRDIFF(&__rwlock->express_commits), 0);

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

	return success ? EOK : ret;
}


static int _mutexgear_rwlock_destroy(mutexgear_rwlock_t *__rwlock);

/*extern */
int mutexgear_trdl_rwlock_destroy(mutexgear_trdl_rwlock_t *__rwlock)
{
	bool success = false;
	int ret, mutex_unlock_status, mutex_destroy_status;

	bool queue_lock_acquired = false, separator_dequeued = false;

	do
	{
		ptrdiff_t wrlock_waits = _mg_atomic_load_relaxed_ptrdiff(_MG_PCVA_PTRDIFF(&__rwlock->wrlock_waits));

		if (wrlock_waits != 0)
		{
			ret = wrlock_waits > 0 ? EBUSY : EINVAL;
			break;
		}

		mutexgear_completion_item_t *tryread_queue_separator = _mutexgear_rtdl_rwlock_getseparator(__rwlock);
		if (!_mutexgear_completion_itemdata_gettag(&tryread_queue_separator->data, rdlock_itemtag_beingwaited))
		{
			ret = EINVAL;
			break;
		}

		if ((ret = _mutexgear_lock_tryacquire(&__rwlock->tryread_queue_lock)) != EOK)
		{
			break;
		}
		queue_lock_acquired = true;

		mutexgear_completion_item_t *test_tail_item, *test_prev_item;
		if (_mutexgear_completion_queue_unsafegetunsafehead(&__rwlock->basic_lock.acquired_reads) != tryread_queue_separator
			|| _mutexgear_completion_queue_unsafegetunsafenext(tryread_queue_separator) != _mutexgear_completion_queue_getend(&__rwlock->basic_lock.acquired_reads)
			|| (_mutexgear_completion_queue_gettail(&test_tail_item, &__rwlock->basic_lock.acquired_reads), test_tail_item != tryread_queue_separator)
			|| (_mutexgear_completion_queue_getpreceding(&test_prev_item, &__rwlock->basic_lock.acquired_reads, tryread_queue_separator), test_prev_item != _mutexgear_completion_queue_getrend(&__rwlock->basic_lock.acquired_reads)))
		{
			ret = EINVAL;
			break;
		}

		_mutexgear_completion_queue_unsafedequeue(tryread_queue_separator);
		separator_dequeued = true;

		if ((ret = _mutexgear_rwlock_destroy(&__rwlock->basic_lock)) != EOK)
		{
			break;
		}

		MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__rwlock->tryread_queue_lock)) == EOK);
		MG_CHECK(mutex_destroy_status, (mutex_destroy_status = _mutexgear_lock_destroy(&__rwlock->tryread_queue_lock)) == EOK);

		_mg_atomic_destroy_ptrdiff(_MG_PA_PTRDIFF(&__rwlock->wrlock_waits));

		// Clear "beingwaited" tag from the separator item
		// _mutexgear_completion_itemdata_settag(&__rwlock->tryread_queue_separator, rdlock_itemtag_beingwaited, false); -- not needed
		_mutexgear_completion_itemdata_destroy(&__rwlock->tryread_queue_separator);

		success = true;
	}
	while (false);

	if (!success)
	{
		if (queue_lock_acquired)
		{
			if (separator_dequeued)
			{
				// Try to enqueue the separator back into the acquired_reads
				mutexgear_completion_item_t *tryread_queue_separator = _mutexgear_rtdl_rwlock_getseparator(__rwlock);
				_mutexgear_completion_queue_unsafeenqueue_back(&__rwlock->basic_lock.acquired_reads, tryread_queue_separator);
			}

			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__rwlock->tryread_queue_lock)) == EOK);
		}
	}

	return success ? EOK : ret;
}

/*extern */
int mutexgear_rwlock_destroy(mutexgear_rwlock_t *__rwlock)
{
	return _mutexgear_rwlock_destroy(__rwlock);
}

static 
int _mutexgear_rwlock_destroy(mutexgear_rwlock_t *__rwlock)
{
	bool success = false;
	int ret, mutex_unlock_status, mutex_destroy_status;

	unsigned int mutex_index, mutex_count;
	bool drain_prepared = false, waiting_reads_prepared = false, waiting_writes_prepared = false, acquired_reads_prepared = false, push_locks_acquired = false;

	do
	{
		const unsigned int mode_flags = __rwlock->fl_un.mode_flags;

		if ((mode_flags & ~_MUTEXGEAR_RWLOCK_MODE__ALLOWED_FLAGS) != 0)
		{
			ret = EINVAL;
			break;
		}

		mutexgear_dlraitem_t *const express_reads = _mutexgear_dlraitem_getfromprevious(&__rwlock->express_reads);

		if (mutexgear_dlraitem_getprevious(express_reads) != express_reads)
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
		_mutexgear_dlraitem_destroyprevious(express_reads);
		_mg_atomic_destroy_ptrdiff(_MG_PA_PTRDIFF(&__rwlock->express_commits));

		success = true;
	}
	while (false);

	if (!success)
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

	return success ? EOK : ret;
}


_MUTEXGEAR_PURE_INLINE
void rwlock_wrlock_increment_wrlock_waits(mutexgear_trdl_rwlock_t *__rwlock, bool *p_wait_required)
{
	ptrdiff_t wrlock_waits = _mg_atomic_fetch_add_relaxed_ptrdiff(_MG_PVA_PTRDIFF(&__rwlock->wrlock_waits), 2);
	MG_ASSERT(wrlock_waits >= 0); // A memory value corruption or an invalid memory modification otherwise

	*p_wait_required = (wrlock_waits & 1) == 0;
}

_MUTEXGEAR_PURE_INLINE
void rwlock_wrlock_unsafe_record_wrlock_waits_tryread_barrier_pass(mutexgear_trdl_rwlock_t *__rwlock)
{
	// NOTE: The function is implemented in an unsafe manner since all its invocations are serialized with a lock
	ptrdiff_t original_waits = _mg_atomic_load_relaxed_ptrdiff(_MG_PCVA_PTRDIFF(&__rwlock->wrlock_waits));
	ptrdiff_t wrlock_waits = _mg_atomic_fetch_add_relaxed_ptrdiff(_MG_PVA_PTRDIFF(&__rwlock->wrlock_waits), ~original_waits & 1);
	MG_VERIFY(wrlock_waits >= 2); // A memory value corruption or an invalid memory modification otherwise
}

_MUTEXGEAR_PURE_INLINE
void rwlock_wrlock_decrement_wrlock_waits(mutexgear_trdl_rwlock_t *__rwlock)
{
	ptrdiff_t wrlock_waits = _mg_atomic_load_relaxed_ptrdiff(_MG_PCVA_PTRDIFF(&__rwlock->wrlock_waits));
	MG_ASSERT(wrlock_waits >= 2); // A memory value corruption or an invalid memory modification otherwise

	while (!_mg_atomic_cas_relaxed_ptrdiff(_MG_PVA_PTRDIFF(&__rwlock->wrlock_waits), &wrlock_waits, (wrlock_waits <= 2 + 1 ? 0 : wrlock_waits - 2)))
	{
		MG_ASSERT(wrlock_waits >= 2);
	}
}

_MUTEXGEAR_PURE_INLINE
bool rwlock_wrlock_are_zero_wrlock_waits(mutexgear_trdl_rwlock_t *__rwlock)
{
	ptrdiff_t wrlock_waits_glance = _mg_atomic_load_relaxed_ptrdiff(_MG_PCVA_PTRDIFF(&__rwlock->wrlock_waits));
	MG_ASSERT(wrlock_waits_glance >= 0);

	return wrlock_waits_glance == 0;
}


static bool rwlock_wrlock_push_readers_waiting_to_acquire_access__single_channel(mutexgear_rwlock_t *__rwlock, mutexgear_completion_item_t *separator_item,
	mutexgear_completion_waiter_t *__waiter, int *__out_status);
static bool rwlock_wrlock_push_readers_waiting_to_acquire_access__multiple_channels(mutexgear_rwlock_t *__rwlock, mutexgear_completion_item_t *separator_item,
	mutexgear_completion_waiter_t *__waiter, int *__out_status);
static bool rwlock_wrlock_wait_all_reads_and_acquire_access(mutexgear_rwlock_t *__rwlock, mutexgear_completion_item_t *separator_item,
	mutexgear_completion_waiter_t *__waiter, mutexgear_completion_item_t *__last_reader_item, int *__out_status);
_MUTEXGEAR_PURE_INLINE bool rwlock_wrlock_find_notmarked_reader_item(mutexgear_completion_item_t **__out_reader_item,
	mutexgear_rwlock_t *__rwlock, mutexgear_completion_item_t *__reader_item);

/*extern */
int mutexgear_trdl_rwlock_wrlock(mutexgear_trdl_rwlock_t *__rwlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_waiter_t *__waiter, mutexgear_completion_item_t *__item/*=NULL*/)
{
	bool success = false;
	int ret, mutex_unlock_status, item_completion_status;

	bool item_initialized = false, wrwaits_incremented = false, wait_initialized = false, wait_inserted = false;

	mutexgear_completion_item_t wait_completion_item;
	mutexgear_completion_item_t *wait_completion_to_use = __item;

	do
	{
		if (__item == NULL)
		{
			MG_STATIC_ASSERT(MUTEXGEAR_PROCESS_SHARED != 0);

			if ((__rwlock->basic_lock.fl_un.mode_flags & MUTEXGEAR_PROCESS_SHARED) != 0) // In case of shared mode, item parameter must not be NULL
			{
				ret = EINVAL;
				break;
			}

			_mutexgear_completion_item_init(&wait_completion_item);
			wait_completion_to_use = &wait_completion_item;
			item_initialized = true;
		}

		bool tryreads_barrier_required;
		rwlock_wrlock_increment_wrlock_waits(__rwlock, &tryreads_barrier_required);
		wrwaits_incremented = true;

		if (tryreads_barrier_required)
		{
			if ((ret = _mutexgear_lock_acquire(&__rwlock->tryread_queue_lock)) != EOK)
			{
				break;
			}

			// It is more reasonable to signal the barrier within the lock, rather than after it,
			// as releasing the lock may wake up a higher priority thread and delay the flag appearance for upcoming writes.
			//
			// NOTE: // Also, since the operation is serialized with a lock, it can be performed in an unsafe manner for performance sake.
			rwlock_wrlock_unsafe_record_wrlock_waits_tryread_barrier_pass(__rwlock);

			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__rwlock->tryread_queue_lock)) == EOK); // Should succeed normally
		}

		bool access_acquired = false;

		mutexgear_completion_item_t *test_tail_item, *tryread_queue_separator = _mutexgear_rtdl_rwlock_getseparator(__rwlock);
		if ((_mutexgear_completion_queue_gettail(&test_tail_item, &__rwlock->basic_lock.acquired_reads), test_tail_item == tryread_queue_separator)
			&& _mutexgear_completion_queue_getunsafepreceding(tryread_queue_separator) == _mutexgear_completion_queue_getrend(&__rwlock->basic_lock.acquired_reads))
		{
			if ((ret = _mutexgear_completion_queue_lock(NULL, &__rwlock->basic_lock.acquired_reads)) != EOK)
			{
				break;
			}

			// Check via the "next" pointers as it is not atomic: atomic access is not needed while the queue is locked
			if (_mutexgear_completion_queue_unsafegetunsafehead(&__rwlock->basic_lock.acquired_reads) == tryread_queue_separator
				&& _mutexgear_completion_queue_unsafegetunsafenext(tryread_queue_separator) == _mutexgear_completion_queue_getend(&__rwlock->basic_lock.acquired_reads))
			{
				access_acquired = true;
			}
			else
			{
				// NOTE: Going for registering into waiting_writes with the acquired_reads unlocked
				// may result in a few extra reads passing into the object. But it is perfectly OK 
				// as those can be considered as if they would had gained the access even before the 
				// acquired_reads lock was acquired at all.
				MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__rwlock->basic_lock.acquired_reads)) == EOK); // Should succeed normally
			}
		}

		if (!access_acquired)
		{
			_mutexgear_completion_item_prestart(wait_completion_to_use, __worker);
			wait_initialized = true;

			if ((ret = _mutexgear_completion_queue_enqueue_back(&__rwlock->basic_lock.waiting_writes, wait_completion_to_use, NULL)) != EOK)
			{
				break;
			}
			wait_inserted = true;

			mutexgear_completion_item_t *tryread_queue_separator = _mutexgear_rtdl_rwlock_getseparator(__rwlock);
			// NOTE: The overhead in rwlock_wrlock_push_readers_waiting_to_acquire_access__multiple_channels() is actually
			// so minor that it is unclear if it is worth making the two separate implementations. Let it be though.
			if ((__rwlock->basic_lock.fl_un.mode_flags & _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_MASK) == 0
				? !rwlock_wrlock_push_readers_waiting_to_acquire_access__single_channel(&__rwlock->basic_lock, tryread_queue_separator, __waiter, &ret)
				: !rwlock_wrlock_push_readers_waiting_to_acquire_access__multiple_channels(&__rwlock->basic_lock, tryread_queue_separator, __waiter, &ret))
			{
				break;
			}

			MG_CHECK(item_completion_status, (item_completion_status = _mutexgear_completion_queueditem_safefinish(&__rwlock->basic_lock.waiting_writes, wait_completion_to_use, __worker)) == EOK); // Well, the item must be removed at any cost!
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
		if (wrwaits_incremented)
		{
			if (wait_initialized)
			{
				if (wait_inserted)
				{
					MG_CHECK(item_completion_status, (item_completion_status = _mutexgear_completion_queueditem_safefinish(&__rwlock->basic_lock.waiting_writes, wait_completion_to_use, __worker)) == EOK); // No way to handle -- must succeed
				}

				_mutexgear_completion_item_reinit(wait_completion_to_use);
			}

			rwlock_wrlock_decrement_wrlock_waits(__rwlock);
		}

		if (item_initialized)
		{
			_mutexgear_completion_item_destroy(&wait_completion_item);
		}

		MG_ASSERT(__item == NULL || _mutexgear_completion_item_isasinit(__item));
	}

	return success ? EOK : ret;
}

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

			if ((__rwlock->fl_un.mode_flags & MUTEXGEAR_PROCESS_SHARED) != 0) // In case of shared mode, item parameter must not be NULL
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

			if ((ret = _mutexgear_completion_queue_enqueue_back(&__rwlock->waiting_writes, wait_completion_to_use, NULL)) != EOK)
			{
				break;
			}
			wait_inserted = true;

			// NOTE: The overhead in rwlock_wrlock_push_readers_waiting_to_acquire_access__multiple_channels() is actually
			// so minor that it is unclear if it is worth making the two separate implementations. Let it be though.
			if ((__rwlock->fl_un.mode_flags & _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_MASK) == 0
				? !rwlock_wrlock_push_readers_waiting_to_acquire_access__single_channel(__rwlock, NULL, __waiter, &ret)
				: !rwlock_wrlock_push_readers_waiting_to_acquire_access__multiple_channels(__rwlock, NULL, __waiter, &ret))
			{
				break;
			}

			MG_CHECK(item_completion_status, (item_completion_status = _mutexgear_completion_queueditem_safefinish(&__rwlock->waiting_writes, wait_completion_to_use, __worker)) == EOK); // Well, the item must be removed at any cost!
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
				MG_CHECK(item_completion_status, (item_completion_status = _mutexgear_completion_queueditem_safefinish(&__rwlock->waiting_writes, wait_completion_to_use, __worker)) == EOK); // No way to handle -- must succeed
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
bool rwlock_wrlock_push_readers_waiting_to_acquire_access__single_channel(mutexgear_rwlock_t *__rwlock, mutexgear_completion_item_t *separator_item,
	mutexgear_completion_waiter_t *__waiter, int *__out_status)
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
		if (_mutexgear_completion_queue_gettail(&last_reader_item, &__rwlock->acquired_reads) // This is equivalent to emptiness check
			&& (last_reader_item != separator_item || _mutexgear_completion_queue_getpreceding(&last_reader_item, &__rwlock->acquired_reads, last_reader_item))
			&& !rwlock_wrlock_wait_all_reads_and_acquire_access(__rwlock, separator_item, __waiter, last_reader_item, &ret))
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
bool rwlock_wrlock_push_readers_waiting_to_acquire_access__multiple_channels(mutexgear_rwlock_t *__rwlock, mutexgear_completion_item_t *separator_item,
	mutexgear_completion_waiter_t *__waiter, int *__out_status)
{
	bool fault = false;
	int ret, mutex_unlock_status;

	const unsigned int lock_index_mask = (__rwlock->fl_un.mode_flags & _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_MASK) >> _MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_SHIFT;
	unsigned int starting_push_lock_index = (unsigned int)_MUTEXGEAR_RWLOCK_MAKE_READER_PUSH_SELECTOR(__waiter); // OK to truncate, the mask is smaller anyway
	MG_STATIC_ASSERT(_MUTEXGEAR_RWLOCK_MODE_READERPUSHLOCKS_MASK <= UINT_MAX);

	ptrdiff_t known_express_commits = 0;
	unsigned int push_lock_index = starting_push_lock_index;

	if ((ret = _mutexgear_lock_acquire(&_MUTEXGEAR_RWLOCK_ACCESS_READER_PUSH_LOCK(__rwlock, push_lock_index & lock_index_mask))) != EOK)
	{
		fault = true;
	}

	if (!fault)
	{
		++push_lock_index;

		known_express_commits = _mg_atomic_load_relaxed_ptrdiff(_MG_PCVA_PTRDIFF(&__rwlock->express_commits));
	}

	for (; !fault; fault = false)
	{
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
		if (!_mutexgear_completion_queue_gettail(&last_reader_item, &__rwlock->acquired_reads) // This is equivalent to emptiness check
			|| (last_reader_item == separator_item && !_mutexgear_completion_queue_getpreceding(&last_reader_item, &__rwlock->acquired_reads, last_reader_item)))
		{
			break;
		}

		if (rwlock_wrlock_wait_all_reads_and_acquire_access(__rwlock, separator_item, __waiter, last_reader_item, &ret))
		{
			break;
		}
		// The unlock for acquired_reads in the call above is not allowed to fail

		if (ret != _MUTEXGEAR_ERRNO__RWLOCK_ALLITEMSBUSY)
		{
			fault = true;
			break;
		}

		// Check if express_commits counter changed...
		ptrdiff_t new_express_commits = _mg_atomic_load_relaxed_ptrdiff(_MG_PCVA_PTRDIFF(&__rwlock->express_commits));

		// ...if not, proceed locking the next mutex
		if (known_express_commits == new_express_commits)
		{
			if ((ret = _mutexgear_lock_acquire(&_MUTEXGEAR_RWLOCK_ACCESS_READER_PUSH_LOCK(__rwlock, push_lock_index & lock_index_mask))) != EOK)
			{
				fault = true;
				break;
			}

			++push_lock_index;
		}
		// ...if changed, stay with the current locked mutex range and go for one more pass instead
		else
		{
			known_express_commits = new_express_commits;
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
bool rwlock_wrlock_wait_all_reads_and_acquire_access(mutexgear_rwlock_t *__rwlock, mutexgear_completion_item_t *separator_item,
	mutexgear_completion_waiter_t *__waiter, mutexgear_completion_item_t *__last_reader_item, int *__out_status)
{
	bool fault = false;
	int ret, mutex_unlock_status;

	mutexgear_completion_item_t	*reader_item = __last_reader_item;
	MG_ASSERT(reader_item != NULL);

	// bool access_locked = true;

	bool exit_loop;
	for (exit_loop = false; !exit_loop;
		// NOTE! It should be more favorable to start waiting from the tail of the queue where 
		// there are the most recent (and, hence, potentially last to release the lock) readers.
		// This way, the algorithm should exit with less number iterations.
		exit_loop = !_mutexgear_completion_queue_gettail(&reader_item, &__rwlock->acquired_reads)
		|| (reader_item == separator_item && !_mutexgear_completion_queue_getpreceding(&reader_item, &__rwlock->acquired_reads, reader_item)))
	{
		if (!_mutexgear_completion_itemdata_modifyunsafetag(&reader_item->data, rdlock_itemtag_beingwaited, true)
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

_MUTEXGEAR_PURE_INLINE
bool rwlock_wrlock_find_notmarked_reader_item(mutexgear_completion_item_t **__out_reader_item,
	mutexgear_rwlock_t *__rwlock, mutexgear_completion_item_t *__reader_item)
{
	bool success = false;

	mutexgear_completion_item_t *reader_item = __reader_item;
	while (_mutexgear_completion_queue_getpreceding(&reader_item, &__rwlock->acquired_reads, reader_item))
	{
		if (_mutexgear_completion_itemdata_modifyunsafetag(&reader_item->data, rdlock_itemtag_beingwaited, true))
		{
			success = true;
			break;
		}
	}

	return success && (*__out_reader_item = reader_item, true);
}


/*extern */
int mutexgear_trdl_rwlock_trywrlock(mutexgear_trdl_rwlock_t *__rwlock)
{
	bool success = false;
	int ret, mutex_unlock_status;

	bool wrwaits_incremented = false;

	do
	{
		mutexgear_completion_item_t *test_tail_item, *tryread_queue_separator = _mutexgear_rtdl_rwlock_getseparator(__rwlock);
		if ((_mutexgear_completion_queue_gettail(&test_tail_item, &__rwlock->basic_lock.acquired_reads), test_tail_item != tryread_queue_separator)
			|| _mutexgear_completion_queue_getunsafepreceding(tryread_queue_separator) != _mutexgear_completion_queue_getrend(&__rwlock->basic_lock.acquired_reads))
		{
			ret = EBUSY;
			break;
		}

		bool tryreads_barrier_required;
		rwlock_wrlock_increment_wrlock_waits(__rwlock, &tryreads_barrier_required);
		wrwaits_incremented = true;

		if (tryreads_barrier_required)
		{
			// NOTE: Try-locking here would create a possibility for both of concurrent 
			// tryrdlock and tryrwlock to return EBUSY on a free object. The tryrdlock 
			// would abort seeing wrlock_waits being not zero after it had acquired tryread_queue_lock.
			// The trywrlock would abort on the try-lock operation for the tryread_queue_lock would be here.
			if ((ret = _mutexgear_lock_acquire(&__rwlock->tryread_queue_lock)) != EOK)
			{
				break;
			}

			// It is more reasonable to signal the barrier within the lock, rather than after it,
			// as releasing the lock may wake up a higher priority thread and delay the flag appearance for upcoming writes.
			//
			// NOTE: // Also, since the operation is serialized with a lock, it can be performed in an unsafe manner for performance sake.
			rwlock_wrlock_unsafe_record_wrlock_waits_tryread_barrier_pass(__rwlock);

			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__rwlock->tryread_queue_lock)) == EOK); // Should succeed normally
		}

		if ((_mutexgear_completion_queue_gettail(&test_tail_item, &__rwlock->basic_lock.acquired_reads), test_tail_item != tryread_queue_separator)
			|| _mutexgear_completion_queue_getunsafepreceding(tryread_queue_separator) != _mutexgear_completion_queue_getrend(&__rwlock->basic_lock.acquired_reads))
		{
			ret = EBUSY;
			break;
		}

		// NOTE: All other cases when the acquired_reads mutex can be busy 
		// (namely, in mutexgear_rwlock_wrlock(), 
		// rwlock_wrlock_push_readers_waiting_to_acquire_access__single_channel(), 
		// rwlock_wrlock_push_readers_waiting_to_acquire_access__multiple_channels(), 
		// rwlock_wrlock_wait_all_reads_and_acquire_access(), mutexgear_rwlock_rdlock(),
		// rwlock_rdlock_wait_all_writes_and_acquire_access()) either indicate that there 
		// already are other read locks or the thread owning the acquired_reads 
		// is itself free to acquire the rwlock for read or write. The busy state while unlocking 
		// in mutexgear_rwlock_rdunlock() may be considered as if the rwlock is still read-locked.
		// So, EBUSY result here may be directly returned as the rwlock's try-lock status.
		if ((ret = _mutexgear_completion_queue_trylock(NULL, &__rwlock->basic_lock.acquired_reads)) != EOK)
		{
			break;
		}

		bool post_increment_lodempty_check_status = false;

		// Check via the "next" pointers as it is not atomic: atomic access is not needed while the queue is locked
		if (_mutexgear_completion_queue_unsafegetunsafehead(&__rwlock->basic_lock.acquired_reads) == tryread_queue_separator
			&& _mutexgear_completion_queue_unsafegetunsafenext(tryread_queue_separator) == _mutexgear_completion_queue_getend(&__rwlock->basic_lock.acquired_reads))
		{
			post_increment_lodempty_check_status = true;
		}
		else
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__rwlock->basic_lock.acquired_reads)) == EOK); // Should succeed normally
		}

		if (!post_increment_lodempty_check_status)
		{
			ret = EBUSY;
			break;
		}

		success = true;
	}
	while (false);

	if (!success)
	{
		if (wrwaits_incremented)
		{
			rwlock_wrlock_decrement_wrlock_waits(__rwlock);
		}
	}

	return success ? EOK : ret;
}

/*extern */
int mutexgear_rwlock_trywrlock(mutexgear_rwlock_t *__rwlock)
{
	bool success = false;
	int ret, mutex_unlock_status;

	do
	{
		bool access_acquired = false;

		if (_mutexgear_completion_queue_lodisempty(&__rwlock->acquired_reads))
		{
			// NOTE: All other cases when the acquired_reads mutex can be busy 
			// (namely, in mutexgear_rwlock_wrlock(), 
			// rwlock_wrlock_push_readers_waiting_to_acquire_access__single_channel(), 
			// rwlock_wrlock_push_readers_waiting_to_acquire_access__multiple_channels(), 
			// rwlock_wrlock_wait_all_reads_and_acquire_access(), mutexgear_rwlock_rdlock(),
			// rwlock_rdlock_wait_all_writes_and_acquire_access()) either indicate that there 
			// already are other read locks or the thread owning the acquired_reads 
			// is itself free to acquire the rwlock for read or write. The busy state while unlocking 
			// in mutexgear_rwlock_rdunlock() may be considered as if the rwlock is still read-locked.
			// So, EBUSY result here may be directly returned as the rwlock's try-lock status.
			if ((ret = _mutexgear_completion_queue_trylock(NULL, &__rwlock->acquired_reads)) != EOK)
			{
				break;
			}

			if (_mutexgear_completion_queue_lodisempty(&__rwlock->acquired_reads))
			{
				access_acquired = true;
			}
			else
			{
				MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__rwlock->acquired_reads)) == EOK); // Should succeed normally
			}
		}

		if (!access_acquired)
		{
			ret = EBUSY;
			break;
		}

		success = true;
	}
	while (false);

	return success ? EOK : ret;
}


/*extern */
int mutexgear_trdl_rwlock_wrunlock(mutexgear_trdl_rwlock_t *__rwlock)
{
	bool success = false;
	int ret;

	do
	{
		// NOTE: This counter decrement before the unlock may corrupt unrelated memory 
		// if ___rwlock object is invalid. However, mutex unlock behavior if undefined for 
		// an invalid argument anyway.
		//
		// Checking for the counter to be positive before the decrement is no salvation.
		//
		// Moving the decrement after the mutex unlock would allow an unwanted behavior
		// when another thread at a higher priority would be unblocked due to the mutex 
		// becoming free, then it would acquire/release the rwlock and might call it again 
		// for tryrdlock while the object would have the wrlock_waits not decremented yet.
		rwlock_wrlock_decrement_wrlock_waits(__rwlock);

		if ((ret = _mutexgear_completion_queue_plainunlock(&__rwlock->basic_lock.acquired_reads)) != EOK)
		{
			break;
		}

		success = true;
	}
	while (false);

	return success ? EOK : ret;
}

/*extern */
int mutexgear_rwlock_wrunlock(mutexgear_rwlock_t *__rwlock)
{
	int ret = _mutexgear_completion_queue_plainunlock(&__rwlock->acquired_reads);
	return ret;
}


static int _mutexgear_rwlock_rdlock(mutexgear_completion_item_t *__end_item, mutexgear_rwlock_t *__rwlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_waiter_t *__waiter, mutexgear_completion_item_t *__item);
static bool rwlock_rdlock_wait_all_writes_and_acquire_access(mutexgear_rwlock_t *__rwlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_waiter_t *__waiter, mutexgear_completion_item_t *__item,
	mutexgear_completion_locktoken_t *__out_readers_lock/*=NULL*/, int *__out_status);
static bool rwlock_rdlock_wait_write_wait_emptiness(mutexgear_rwlock_t *__rwlock,
	mutexgear_completion_waiter_t *__waiter, mutexgear_completion_item_t *last_write_wait,
	/*bool *__out_waiting_writes_remain_locked, */int *__out_status);
_MUTEXGEAR_PURE_INLINE mutexgear_dlraitem_t *rwlock_rdlock_link_express_queue_till_rend(mutexgear_dlraitem_t *queue_tail, mutexgear_dlraitem_t *queue_rend);

/*extern */
int mutexgear_trdl_rwlock_rdlock(mutexgear_trdl_rwlock_t *__rwlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_waiter_t *__waiter, mutexgear_completion_item_t *__item)
{
	mutexgear_completion_item_t *tryread_queue_separator = _mutexgear_rtdl_rwlock_getseparator(__rwlock);
	return _mutexgear_rwlock_rdlock(tryread_queue_separator, &__rwlock->basic_lock, __worker, __waiter, __item);
}

/*extern */
int mutexgear_rwlock_rdlock(mutexgear_rwlock_t *__rwlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_waiter_t *__waiter, mutexgear_completion_item_t *__item)
{
	mutexgear_completion_item_t *end_item = _mutexgear_completion_queue_getend(&__rwlock->acquired_reads);
	return _mutexgear_rwlock_rdlock(end_item, __rwlock, __worker, __waiter, __item);
}

static
int _mutexgear_rwlock_rdlock(mutexgear_completion_item_t *__end_item, mutexgear_rwlock_t *__rwlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_waiter_t *__waiter, mutexgear_completion_item_t *__item)
{
	MG_ASSERT(!mutexgear_dlraitem_islinked(_mutexgear_completion_item_getworkitem(__item)));

	bool success = false;
	int ret, mutex_lock_status, mutex_unlock_status;

	bool read_work_started = false, access_locked = false;

	do
	{
		if (_mutexgear_completion_itemdata_getanytags(&__item->data))
		{
			ret = EINVAL;
			break;
		}

		bool queued_as_express = false;

		// Optimistically pre-start the item before entering locks
		_mutexgear_completion_item_prestart(__item, __worker);
		read_work_started = true;

		mutexgear_completion_locktoken_t readers_lock_storage, *readers_lock_ptr = NULL;
		MG_ASSERT((readers_lock_ptr = &readers_lock_storage, true));

		bool linked_into_express_queue = false;

		if (_mutexgear_completion_queue_lodisempty(&__rwlock->waiting_writes))
		{
			mutexgear_dlraitem_t *item_work_item = _mutexgear_completion_item_getworkitem(__item);

			// Set next self-linked to have an indicator that the item is in a single linked list yet
			_mutexgear_dlraitem_setnext(item_work_item, item_work_item);
			
			mutexgear_dlraitem_t *const express_reads = _mutexgear_dlraitem_getfromprevious(&__rwlock->express_reads);
			mutexgear_dlraitem_t *queued_express_last = mutexgear_dlraitem_getprevious(express_reads);
			
			linked_into_express_queue = true;
			if ((((_mutexgear_dlraitem_setunsafeprevious(item_work_item, queued_express_last), true) && _mutexgear_dlraitem_trysetprevious(express_reads, &queued_express_last, item_work_item))
				|| ((_mutexgear_dlraitem_setunsafeprevious(item_work_item, queued_express_last), true) && _mutexgear_dlraitem_trysetprevious(express_reads, &queued_express_last, item_work_item))
				|| ((_mutexgear_dlraitem_setunsafeprevious(item_work_item, queued_express_last), true) && _mutexgear_dlraitem_trysetprevious(express_reads, &queued_express_last, item_work_item))
				|| ((_mutexgear_dlraitem_setunsafeprevious(item_work_item, queued_express_last), true) && _mutexgear_dlraitem_trysetprevious(express_reads, &queued_express_last, item_work_item))
				|| ((_mutexgear_dlraitem_setunsafeprevious(item_work_item, queued_express_last), true) && _mutexgear_dlraitem_trysetprevious(express_reads, &queued_express_last, item_work_item))
				|| ((_mutexgear_dlraitem_setunsafeprevious(item_work_item, queued_express_last), true) && _mutexgear_dlraitem_trysetprevious(express_reads, &queued_express_last, item_work_item))
				|| ((_mutexgear_dlraitem_setunsafeprevious(item_work_item, queued_express_last), true) && _mutexgear_dlraitem_trysetprevious(express_reads, &queued_express_last, item_work_item))
				|| ((_mutexgear_dlraitem_setunsafeprevious(item_work_item, queued_express_last), true) && _mutexgear_dlraitem_trysetprevious(express_reads, &queued_express_last, item_work_item))
				|| (_mutexgear_dlraitem_unsaferesettounlinked(item_work_item), linked_into_express_queue = false, false))
				&& (_mutexgear_completion_queue_getunsafepreceding(__end_item) != _mutexgear_completion_queue_getrend(&__rwlock->acquired_reads)
					|| (/*__end_item != _mutexgear_completion_queue_getrend(&__rwlock->acquired_reads) && */_mutexgear_completion_queue_getunsafepreceding(_mutexgear_completion_queue_getrend(&__rwlock->acquired_reads)) != __end_item)))
			{
				queued_as_express = true;
			}
			else
			{
				// NOTE:
				// Execution must not bail out with an error if the work_item has been linked 
				// into the express queue -- there is no way to remove the item from there.
				// Even though the item might have not actually been linked (linking_into_express_queue_failed == true)
				// returning error in one case and aborting the program in the other case is impractical. 
				// Muteces do not fail on lock normally, provided they have been preallocated.
				MG_CHECK(mutex_lock_status, (mutex_lock_status = _mutexgear_completion_queue_lock(readers_lock_ptr, &__rwlock->acquired_reads)) == EOK);
				// access_locked = true;

				if (linked_into_express_queue || _mutexgear_completion_queue_lodisempty(&__rwlock->waiting_writes))
				{
					access_locked = true;
				}
				else
				{
					MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__rwlock->acquired_reads)) == EOK); // Should succeed normally
					// access_locked = false;
					MG_ASSERT((readers_lock_storage = NULL, true));
				}
			}
		}

		if (!queued_as_express)
		{
			if (!access_locked)
			{
				if (!rwlock_rdlock_wait_all_writes_and_acquire_access(__rwlock, __worker, __waiter, __item, readers_lock_ptr, &ret))
				{
					break;
				}

				// access_locked = true;
			}

			// Link own item first to give others a clue that the express queue can already be used...
			// (even though this can potentially waste two memory writes compared to linking the item 
			// together with the express queue in case if execution enters that branch)
			if (!linked_into_express_queue)
			{
				_mutexgear_completion_queue_unsafeenqueue_before(&__rwlock->acquired_reads, __end_item, __item);
				MG_ASSERT(readers_lock_storage == MUTEXGEAR_COMPLETION_ACQUIRED_LOCKTOCKEN);
				MG_DO_NOTHING(readers_lock_storage); // To suppress unused variable compiler warning
			}
			// ...then inspect the express_reads
			mutexgear_dlraitem_t *const express_reads = _mutexgear_dlraitem_getfromprevious(&__rwlock->express_reads);
			mutexgear_dlraitem_t  *express_tail_preview = mutexgear_dlraitem_getprevious(express_reads);

			if (express_tail_preview != express_reads)
			{
				// Increment the commit counter as soon as it is known that a commit will be performed 
				// to make the beset effort preventing writers from merging within a single reader push lock.
				_mg_atomic_store_relaxed_ptrdiff(_MG_PVA_PTRDIFF(&__rwlock->express_commits), _mg_atomic_load_relaxed_ptrdiff(_MG_PCVA_PTRDIFF(&__rwlock->express_commits)) + 1); // _mg_atomic_fetch_add_relaxed_ptrdiff(_MG_PVA_PTRDIFF(&__rwlock->express_commits), 1);

				mutexgear_dlraitem_t *last_express_read = _mutexgear_dlraitem_swapprevious(express_reads, express_reads);
				mutexgear_dlraitem_t *first_express_read = rwlock_rdlock_link_express_queue_till_rend(last_express_read, express_reads);
				MG_ASSERT((_mutexgear_dlraitem_setunsafeprevious(first_express_read, first_express_read), true)); // To suppress an assertion check in _mutexgear_completion_queue_unsafemultiqueue_before() on the first item to be not linked

				mutexgear_completion_item_t *last_item = _mutexgear_completion_item_getfromworkitem(last_express_read), *first_item = _mutexgear_completion_item_getfromworkitem(first_express_read);
				_mutexgear_completion_queue_unsafemultiqueue_before(&__rwlock->acquired_reads, __end_item, first_item, last_item);
				MG_ASSERT(readers_lock_storage == MUTEXGEAR_COMPLETION_ACQUIRED_LOCKTOCKEN);
				MG_DO_NOTHING(readers_lock_storage); // To suppress unused variable compiler warning
			}

			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__rwlock->acquired_reads)) == EOK); // Should succeed normally
			// access_locked = false;
		}

		success = true;
	}
	while (false);

	if (!success)
	{
		if (read_work_started)
		{
			MG_ASSERT(!access_locked);

			_mutexgear_completion_item_reinit(__item);
		}
	}

	return success ? EOK : ret;
}

static
bool rwlock_rdlock_wait_all_writes_and_acquire_access(mutexgear_rwlock_t *__rwlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_waiter_t *__waiter, mutexgear_completion_item_t *__item,
	mutexgear_completion_locktoken_t *__out_readers_lock/*=NULL*/, int *__out_status)
{
	bool fault = false;
	int ret, mutex_unlock_status, item_completion_status;

	mutexgear_completion_drainidx_t read_wait_drain_index;

	bool /*access_locked, *//*waiting_reads_locked, */wait_inserted, waiting_writes_locked;

	for (; ; )
	{
		/*access_locked = false, *//*waiting_reads_locked = false, */wait_inserted = false, waiting_writes_locked = false;

		mutexgear_completion_locktoken_t waiting_reads_lock_storage, *waiting_reads_lock_ptr = NULL;
		MG_ASSERT((waiting_reads_lock_ptr = &waiting_reads_lock_storage, true));

		if ((ret = _mutexgear_completion_drainablequeue_lock(waiting_reads_lock_ptr, &__rwlock->waiting_reads)) != EOK)
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
		//
		// _mutexgear_completion_item_prestart(__item, __worker); -- the item was alreaddy pre-started in the caller or at the end of the previous loop pass
		//

		_mutexgear_completion_drainablequeue_unsafeenqueue(&__rwlock->waiting_reads, __item, &read_wait_drain_index);
		MG_ASSERT(waiting_reads_lock_storage == MUTEXGEAR_COMPLETION_ACQUIRED_LOCKTOCKEN);
		MG_DO_NOTHING(waiting_reads_lock_storage); // To suppress unused variable compiler warning
		wait_inserted = true;

		if (read_wait_queue_was_empty)
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_drainablequeue_plainunlock(&__rwlock->waiting_reads)) == EOK); // Should succeed normally
			// waiting_reads_locked = false;
			MG_ASSERT((waiting_reads_lock_storage = NULL, true));

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
			MG_CHECK(item_completion_status, (item_completion_status = _mutexgear_completion_drainablequeueditem_safefinish(&__rwlock->waiting_reads, __item, __worker, read_wait_drain_index, &__rwlock->read_wait_drain)) == EOK); // No way to handle -- must succeed
			// wait_inserted = false;
		}
		else
		{
			mutexgear_completion_item_t *previous_wait = _mutexgear_completion_drainablequeue_unsafegetpreceding(__item);
			MG_ASSERT(previous_wait != NULL);

			ret = _mutexgear_completion_drainablequeue_unlockandwait(&__rwlock->waiting_reads, previous_wait, __waiter);
			// waiting_reads_locked = false; // The unlock in the call above is not allowed to fail
			MG_ASSERT((waiting_reads_lock_storage = NULL, true));

			if (ret != EOK)
			{
				fault = true;
				break;
			}

			MG_CHECK(item_completion_status, (item_completion_status = _mutexgear_completion_drainablequeueditem_safefinish(&__rwlock->waiting_reads, __item, __worker, MUTEXGEAR_COMPLETION_INVALID_DRAINIDX, NULL)) == EOK);
			// wait_inserted = false;
		}

		/*access_locked = false, *//*waiting_reads_locked = false, */wait_inserted = false, waiting_writes_locked = false;

		// Optimistically pre-start the item before entering locked sections to be ready for return into the caller, or for the next loop pass
		_mutexgear_completion_item_prestart(__item, __worker);

		if ((ret = _mutexgear_completion_queue_lock(__out_readers_lock, &__rwlock->acquired_reads)) != EOK)
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
		MG_ASSERT(__out_readers_lock == NULL || (*__out_readers_lock = NULL, true));
	}

	if (fault)
	{
		if (wait_inserted)
		{
			if (waiting_writes_locked)
			{
				MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__rwlock->waiting_writes)) == EOK); // Should succeed normally
			}

			MG_CHECK(item_completion_status, (item_completion_status = _mutexgear_completion_drainablequeueditem_safefinish(&__rwlock->waiting_reads, __item, __worker, read_wait_drain_index, &__rwlock->read_wait_drain)) == EOK); // No way to handle -- must succeed
			// _mutexgear_completion_item_prestart(__item, __worker); -- no need to keep the item pre-started with a failed exit
		}
	}

	return !fault || (*__out_status = ret, false);
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

_MUTEXGEAR_PURE_INLINE
mutexgear_dlraitem_t *rwlock_rdlock_link_express_queue_till_rend(mutexgear_dlraitem_t *queue_tail, mutexgear_dlraitem_t *queue_rend)
{
	MG_ASSERT(queue_tail != queue_rend);

	mutexgear_dlraitem_t *first_element;

	for (mutexgear_dlraitem_t *previous_element, *current_element = queue_tail; ; current_element = previous_element)
	{
		previous_element = mutexgear_dlraitem_getprevious(current_element);

		if (previous_element == queue_rend)
		{
			first_element = current_element;
			break;
		}

		_mutexgear_dlraitem_setnext(previous_element, current_element);
	}

	return first_element;
}


/*extern */
int mutexgear_trdl_rwlock_tryrdlock(mutexgear_trdl_rwlock_t *__rwlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_item_t *__item)
{
	bool success = false;
	int ret, mutex_unlock_status;

	bool access_locked = false, read_work_started = false/*, tryreads_locked = false*/;

	do
	{
		if (_mutexgear_completion_itemdata_getanytags(&__item->data))
		{
			ret = EINVAL;
			break;
		}

		if (!rwlock_wrlock_are_zero_wrlock_waits(__rwlock))
		{
			ret = EBUSY;
			break;
		}

		// Optimistically pre-tag and pre-start the item before entering locks
		_mutexgear_completion_itemdata_setunsafetag(&__item->data, rdlock_itemtag_trylocked, true);
		_mutexgear_completion_item_prestart(__item, __worker);
		read_work_started = true;

		if ((ret = _mutexgear_lock_acquire(&__rwlock->tryread_queue_lock)) != EOK)
		{
			break;
		}
		// tryreads_locked = true;

		if (rwlock_wrlock_are_zero_wrlock_waits(__rwlock))
		{
			// Enqueue the item back, after the tryread_queue_separator, as this is a "try-read" section protected with tryread_queue_lock mutex
			_mutexgear_completion_queue_unsafeenqueue_back(&__rwlock->basic_lock.acquired_reads, __item);

			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__rwlock->tryread_queue_lock)) == EOK); // Should succeed normally
			// tryreads_locked = false;

			access_locked = true;
		}
		else
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__rwlock->tryread_queue_lock)) == EOK); // Should succeed normally
			// tryreads_locked = false;
		}

		if (!access_locked)
		{
			ret = EBUSY;
			break;
		}

		success = true;
	}
	while (false);

	if (!success)
	{
		if (read_work_started)
		{
			// if (tryreads_locked)
			// {
			// 	MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__rwlock->tryread_queue_lock)) == EOK); // Should succeed normally
			// }

			_mutexgear_completion_item_reinit(__item);
			_mutexgear_completion_itemdata_setunsafetag(&__item->data, rdlock_itemtag_trylocked, false);
		}

		MG_ASSERT(!access_locked);
	}

	return success ? EOK : ret;
}


_MUTEXGEAR_PURE_INLINE void rwlock_rdunlock_committed_commit_express_reads(mutexgear_rwlock_t *__rwlock, mutexgear_completion_item_t *__end_item);
_MUTEXGEAR_PURE_INLINE void rwlock_rdunlock_queued_commit_express_reads(mutexgear_rwlock_t *__rwlock, mutexgear_completion_item_t *__end_item, 
	mutexgear_dlraitem_t *item_work_item);

/*extern */
int mutexgear_trdl_rwlock_rdunlock(mutexgear_trdl_rwlock_t *__rwlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_item_t *__item)
{
	bool success = false;
	int ret, mutex_unlock_status;

	bool access_locked = false;

	do
	{
		mutexgear_completion_locktoken_t reads_lock_storage, *reads_lock_ptr = NULL;
		MG_ASSERT((reads_lock_ptr = &reads_lock_storage, true));

		if ((ret = _mutexgear_completion_queue_lock(reads_lock_ptr, &__rwlock->basic_lock.acquired_reads)) != EOK)
		{
			break;
		}
		access_locked = true;

		mutexgear_dlraitem_t *item_work_item = _mutexgear_completion_item_getworkitem(__item);

		// Check if the item has already been linked into the acquired_reads
		if (mutexgear_dlraitem_getnext(item_work_item) != item_work_item)
		{
			bool tryreads_locked = false;

			if (_mutexgear_completion_itemdata_gettag(&__item->data, rdlock_itemtag_trylocked)
				&& ((ret = _mutexgear_lock_acquire(&__rwlock->tryread_queue_lock)) != EOK || (tryreads_locked = true, false)))
			{
				break;
			}

			// The item must be finished before retrieving the express_reads 
			// to not create a window when new express_reads items could be added 
			// with acquired_reads looking not empty for them...
			_mutexgear_completion_queueditem_unsafefinish__locked(__item);
			MG_ASSERT(reads_lock_storage == MUTEXGEAR_COMPLETION_ACQUIRED_LOCKTOCKEN);
			MG_DO_NOTHING(reads_lock_storage); // To suppress unused variable compiler warning

			if (tryreads_locked)
			{
				MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__rwlock->tryread_queue_lock)) == EOK); // Should succeed normally
			}

			// ...and then it is safe to extract the queued express_reads 
			mutexgear_completion_item_t *tryread_queue_separator = _mutexgear_rtdl_rwlock_getseparator(__rwlock);
			rwlock_rdunlock_committed_commit_express_reads(&__rwlock->basic_lock, tryread_queue_separator);

			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__rwlock->basic_lock.acquired_reads)) == EOK); // Should succeed normally

			_mutexgear_completion_queueditem_unsafefinish__unlocked(&__rwlock->basic_lock.acquired_reads, __item, __worker);

			// Clear rdlock_itemtag_beingwaited tag that might have been set by waiting writes, as well as clear possible rdlock_itemtag_trylocked flag
			_mutexgear_completion_itemdata_setallunsafetags(&__item->data, false);
		}
		else
		{
			// ...and then it is safe to extract the queued express_reads 
			mutexgear_completion_item_t *tryread_queue_separator = _mutexgear_rtdl_rwlock_getseparator(__rwlock);
			rwlock_rdunlock_queued_commit_express_reads(&__rwlock->basic_lock, tryread_queue_separator, item_work_item);

			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__rwlock->basic_lock.acquired_reads)) == EOK); // Should succeed normally

			mutexgear_dlraitem_t *item_work_item = _mutexgear_completion_item_getworkitem(__item);
			_mutexgear_dlraitem_unsaferesettounlinked(item_work_item);
			// Just reinit the item since it did not get into a queue yet
			_mutexgear_completion_item_reinit(__item);

			// Just check the tag since it could not have been set by writes yet
			MG_ASSERT(!_mutexgear_completion_itemdata_gettag(&__item->data, rdlock_itemtag_beingwaited));
		}

		success = true;
	}
	while (false);

	if (!success)
	{
		if (access_locked)
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__rwlock->basic_lock.acquired_reads)) == EOK); // Should succeed normally
		}
	}

	return success ? EOK : ret;
}

/*extern */
int mutexgear_rwlock_rdunlock(mutexgear_rwlock_t *__rwlock,
	mutexgear_completion_worker_t *__worker, mutexgear_completion_item_t *__item)
{
	bool success = false;
	int ret, mutex_unlock_status;

	do
	{
		mutexgear_completion_locktoken_t reads_lock_storage, *reads_lock_ptr = NULL;
		MG_ASSERT((reads_lock_ptr = &reads_lock_storage, true));

		if ((ret = _mutexgear_completion_queue_lock(reads_lock_ptr, &__rwlock->acquired_reads)) != EOK)
		{
			break;
		}

		mutexgear_dlraitem_t *item_work_item = _mutexgear_completion_item_getworkitem(__item);

		// Check if the item has already been linked into the acquired_reads
		if (mutexgear_dlraitem_getnext(item_work_item) != item_work_item)
		{
			// The item must be finished before retrieving the express_reads 
			// to not create a window when new express_reads items could be added 
			// with acquired_reads looking not empty for them...
			_mutexgear_completion_queueditem_unsafefinish__locked(__item);
			MG_ASSERT(reads_lock_storage == MUTEXGEAR_COMPLETION_ACQUIRED_LOCKTOCKEN);
			MG_DO_NOTHING(reads_lock_storage); // To suppress unused variable compiler warning

			// ...and then it is safe to extract the queued express_reads 
			mutexgear_completion_item_t *end_item = _mutexgear_completion_queue_getend(&__rwlock->acquired_reads);
			rwlock_rdunlock_committed_commit_express_reads(__rwlock, end_item);

			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__rwlock->acquired_reads)) == EOK); // Should succeed normally

			_mutexgear_completion_queueditem_unsafefinish__unlocked(&__rwlock->acquired_reads, __item, __worker);

			// Clear the tag that might have been set by waiting writes
			_mutexgear_completion_itemdata_setunsafetag(&__item->data, rdlock_itemtag_beingwaited, false);
		}
		else
		{
			// ...and then it is safe to extract the queued express_reads 
			mutexgear_completion_item_t *end_item = _mutexgear_completion_queue_getend(&__rwlock->acquired_reads);
			rwlock_rdunlock_queued_commit_express_reads(__rwlock, end_item, item_work_item);

			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__rwlock->acquired_reads)) == EOK); // Should succeed normally

			mutexgear_dlraitem_t *item_work_item = _mutexgear_completion_item_getworkitem(__item);
			_mutexgear_dlraitem_unsaferesettounlinked(item_work_item);
			// Just reinit the item since it did not get into a queue yet
			_mutexgear_completion_item_reinit(__item);

			// Just check the tag since it could not have been set by writes yet
			MG_ASSERT(!_mutexgear_completion_itemdata_gettag(&__item->data, rdlock_itemtag_beingwaited));
		}

		success = true;
	}
	while (false);

	return success ? EOK : ret;
}

_MUTEXGEAR_PURE_INLINE 
void rwlock_rdunlock_committed_commit_express_reads(mutexgear_rwlock_t *__rwlock, mutexgear_completion_item_t *__end_item)
{
	mutexgear_dlraitem_t *const express_reads = _mutexgear_dlraitem_getfromprevious(&__rwlock->express_reads);
	// WARNING!
	// No relaxed previewing can be performed here as it has no protection against reordering 
	// before the __item removal from the queue above. And that creates a data race with the express_reads scheduling 
	// and its following check that the acquired_reads is not empty
	// mutexgear_dlraitem_t  *express_tail_preview = mutexgear_dlraitem_getprevious(express_reads); -- WARNING!!!
	mutexgear_dlraitem_t *last_express_read = _mutexgear_dlraitem_swapprevious(express_reads, express_reads);

	if (last_express_read != express_reads)
	{
		// Increment the commit counter as soon as it is known that a commit will be performed 
		// to make the beset effort preventing writers from merging within a single reader push lock.
		_mg_atomic_store_relaxed_ptrdiff(_MG_PVA_PTRDIFF(&__rwlock->express_commits), _mg_atomic_load_relaxed_ptrdiff(_MG_PCVA_PTRDIFF(&__rwlock->express_commits)) + 1); // _mg_atomic_fetch_add_relaxed_ptrdiff(_MG_PVA_PTRDIFF(&__rwlock->express_commits), 1);

		mutexgear_dlraitem_t *first_express_read = rwlock_rdlock_link_express_queue_till_rend(last_express_read, express_reads);
		MG_ASSERT((_mutexgear_dlraitem_setunsafeprevious(first_express_read, first_express_read), true)); // To suppress an assertion check in _mutexgear_completion_queue_unsafemultiqueue_before() on the first item to be not linked

		mutexgear_completion_item_t *last_item = _mutexgear_completion_item_getfromworkitem(last_express_read), *first_item = _mutexgear_completion_item_getfromworkitem(first_express_read);
		_mutexgear_completion_queue_unsafemultiqueue_before(&__rwlock->acquired_reads, __end_item, first_item, last_item);
	}
}

_MUTEXGEAR_PURE_INLINE
void rwlock_rdunlock_queued_commit_express_reads(mutexgear_rwlock_t *__rwlock, mutexgear_completion_item_t *__end_item, 
	mutexgear_dlraitem_t *item_work_item)
{
	bool got_express_reads = false;

	mutexgear_dlraitem_t *const express_reads = _mutexgear_dlraitem_getfromprevious(&__rwlock->express_reads);
	mutexgear_dlraitem_t *first_express_read, *last_express_read = _mutexgear_dlraitem_swapprevious(express_reads, express_reads);
	MG_FAKE_INITIALIZE(first_express_read, 0);

	// __item has not been removed from express_reads yet, so the list cannot be empty
	MG_ASSERT(last_express_read != express_reads);

	if (last_express_read != item_work_item)
	{
		// Increment the commit counter as soon as it is known that a commit will be performed 
		// to make the beset effort preventing writers from merging within a single reader push lock.
		_mg_atomic_store_relaxed_ptrdiff(_MG_PVA_PTRDIFF(&__rwlock->express_commits), _mg_atomic_load_relaxed_ptrdiff(_MG_PCVA_PTRDIFF(&__rwlock->express_commits)) + 1); // _mg_atomic_fetch_add_relaxed_ptrdiff(_MG_PVA_PTRDIFF(&__rwlock->express_commits), 1);

		mutexgear_dlraitem_t *first_preitem_express_read = rwlock_rdlock_link_express_queue_till_rend(last_express_read, item_work_item);
		mutexgear_dlraitem_t *first_postitem_express_read = mutexgear_dlraitem_getprevious(item_work_item);

		if (first_postitem_express_read != express_reads)
		{
			_mutexgear_dlraitem_setunsafeprevious(first_preitem_express_read, first_postitem_express_read);
			_mutexgear_dlraitem_setnext(first_postitem_express_read, first_preitem_express_read);

			first_express_read = rwlock_rdlock_link_express_queue_till_rend(first_postitem_express_read, express_reads);
		}
		else
		{
			first_express_read = first_preitem_express_read;
		}

		got_express_reads = true;
	}
	else
	{
		mutexgear_dlraitem_t *first_postitem_express_read = mutexgear_dlraitem_getprevious(item_work_item);

		if (first_postitem_express_read != express_reads)
		{
			// Increment the commit counter as soon as it is known that a commit will be performed 
			// to make the beset effort preventing writers from merging within a single reader push lock.
			_mg_atomic_store_relaxed_ptrdiff(_MG_PVA_PTRDIFF(&__rwlock->express_commits), _mg_atomic_load_relaxed_ptrdiff(_MG_PCVA_PTRDIFF(&__rwlock->express_commits)) + 1); // _mg_atomic_fetch_add_relaxed_ptrdiff(_MG_PVA_PTRDIFF(&__rwlock->express_commits), 1);

			last_express_read = first_postitem_express_read;
			first_express_read = rwlock_rdlock_link_express_queue_till_rend(first_postitem_express_read, express_reads);

			got_express_reads = true;
		}
	}

	if (got_express_reads)
	{
		MG_ASSERT((_mutexgear_dlraitem_setunsafeprevious(first_express_read, first_express_read), true)); // To suppress an assertion check in _mutexgear_completion_queue_unsafemultiqueue_before() on the first item to be not linked

		mutexgear_completion_item_t *last_item = _mutexgear_completion_item_getfromworkitem(last_express_read), *first_item = _mutexgear_completion_item_getfromworkitem(first_express_read);
		_mutexgear_completion_queue_unsafemultiqueue_before(&__rwlock->acquired_reads, __end_item, first_item, last_item);
	}
}

//////////////////////////////////////////////////////////////////////////
