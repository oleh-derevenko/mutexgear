#ifndef __MUTEXGEAR_MG_COMPLETION_H_INCLUDED
#define __MUTEXGEAR_MG_COMPLETION_H_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear Completion API Internal Definitions                        */
/*                                                                      */
/* WARNING!                                                             */
/* This library contains a synchronization technique protected by       */
/* the U.S. Patent 9,983,913.                                           */
/*                                                                      */
/* THIS IS A PRE-RELEASE LIBRARY SNAPSHOT.                              */
/* AWAIT THE RELEASE AT https://mutexgear.com                           */
/*                                                                      */
/* Copyright (c) 2016-2020 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/

/**
 *	\file
 *	\brief MutexGear Completion API internal definitions
 *
 *	The header defines a "completion" object.
 *	The completion is an object used to wait for an threaded operation end.
 */


#include <mutexgear/completion.h>
#include "utility.h"


//////////////////////////////////////////////////////////////////////////
// mutexgear_completion_genattr_t 

int _mutexgear_completion_genattr_init(mutexgear_completion_genattr_t *__attr);
int _mutexgear_completion_genattr_destroy(mutexgear_completion_genattr_t *__attr);
int _mutexgear_completion_genattr_getpshared(const mutexgear_completion_genattr_t *__attr, int *__pshared);
int _mutexgear_completion_genattr_setpshared(mutexgear_completion_genattr_t *__attr, int __pshared);
int _mutexgear_completion_genattr_getprioceiling(const mutexgear_completion_genattr_t *__attr, int *__prioceiling);
int _mutexgear_completion_genattr_getprotocol(const mutexgear_completion_genattr_t *__attr, int *__protocol);
int _mutexgear_completion_genattr_setprioceiling(mutexgear_completion_genattr_t *__attr, int __prioceiling);
int _mutexgear_completion_genattr_setprotocol(mutexgear_completion_genattr_t *__attr, int __protocol);
int _mutexgear_completion_genattr_setmutexattr(mutexgear_completion_genattr_t *__attr, const _MUTEXGEAR_LOCKATTR_T *__mutexattr);


//////////////////////////////////////////////////////////////////////////
// Completion Queue Types

#define MUTEXGEAR_COMPLETION_INVALID_DRAINIDX ((mutexgear_completion_drainidx_t)0)
#define MUTEXGEAR_COMPLETION_DRAINIDX_MIN ((mutexgear_completion_drainidx_t)1)

_MUTEXGEAR_PURE_INLINE
mutexgear_completion_drainidx_t _mutexgear_completion_drainidx_increment(mutexgear_completion_drainidx_t index)
{
	return index + 1 != MUTEXGEAR_COMPLETION_INVALID_DRAINIDX ? index + 1 : MUTEXGEAR_COMPLETION_DRAINIDX_MIN;
}

_MUTEXGEAR_PURE_INLINE
mutexgear_completion_drainidx_t _mutexgear_completion_drainidx_getmin()
{
	return MUTEXGEAR_COMPLETION_DRAINIDX_MIN;
}


#define MUTEXGEAR_COMPLETION_ACQUIRED_LOCKTOCKEN	((mutexgear_completion_locktoken_t)((char *)NULL + 1))

_MUTEXGEAR_PURE_INLINE
mutexgear_completion_locktoken_t _mutexgear_completion_queue_derivetoken(mutexgear_completion_queue_t *__queue_instance)
{
	return MUTEXGEAR_COMPLETION_ACQUIRED_LOCKTOCKEN;
}


//////////////////////////////////////////////////////////////////////////
// Regular queue APIs

int _mutexgear_completion_worker_init(mutexgear_completion_worker_t *__worker_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/);
int _mutexgear_completion_worker_destroy(mutexgear_completion_worker_t *__worker_instance);

int _mutexgear_completion_worker_lock(mutexgear_completion_worker_t *__worker_instance);
int _mutexgear_completion_worker_unlock(mutexgear_completion_worker_t *__worker_instance);

int _mutexgear_completion_waiter_init(mutexgear_completion_waiter_t *__waiter_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/);
int _mutexgear_completion_waiter_destroy(mutexgear_completion_waiter_t *__waiter_instance);


#define _MUTEXGEAR_COMPLETION_ITEM_EXTRA__CANCEL_REQUEST		(_MUTEXGEAR_COMPLETION_ITEM_EXTRA___HIGHEST_BIT >> 0)
#define _MUTEXGEAR_COMPLETION_ITEM_EXTRA__MARKED_STATUS			(_MUTEXGEAR_COMPLETION_ITEM_EXTRA___HIGHEST_BIT >> 1)


_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_item_setwow(mutexgear_completion_item_t *__item_instance, void *__worker_or_waiter)
{
	_mg_atomic_store_relaxed_ptrdiff(_MG_PVA_PTRDIFF(&__item_instance->p_worker_or_waiter), (uint8_t *)__worker_or_waiter - (uint8_t *)__item_instance);
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_item_barriersetwow(mutexgear_completion_item_t *__item_instance, void *__worker_or_waiter)
{
	_mg_atomic_store_release_ptrdiff(_MG_PVA_PTRDIFF(&__item_instance->p_worker_or_waiter), (uint8_t *)__worker_or_waiter - (uint8_t *)__item_instance);
}

_MUTEXGEAR_PURE_INLINE
void *_mutexgear_completion_item_barriergetwow(const mutexgear_completion_item_t *__item_instance)
{
	return (uint8_t *)__item_instance + _mg_atomic_load_acquire_ptrdiff(_MG_PCVA_PTRDIFF(&__item_instance->p_worker_or_waiter));
}


_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_item_init(mutexgear_completion_item_t *__item_instance)
{
	mutexgear_completion_item_init(__item_instance);
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_item_reinit(mutexgear_completion_item_t *__item_instance)
{
	mutexgear_completion_item_reinit(__item_instance);
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_item_isasinit(const mutexgear_completion_item_t *__item_instance)
{
	return mutexgear_completion_item_isasinit(__item_instance);
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_item_destroy(mutexgear_completion_item_t *__item_instance)
{
	mutexgear_completion_item_destroy(__item_instance);
}


_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_item_prestart(mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
{
	mutexgear_completion_item_prestart(__item_instance, __worker_instance);
}

_MUTEXGEAR_PURE_INLINE 
void _mutexgear_completion_item_settag(mutexgear_completion_item_t *__item_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value)
{
	mutexgear_completion_item_settag(__item_instance, __tag_index, __tag_value);
}

_MUTEXGEAR_PURE_INLINE 
bool _mutexgear_completion_item_unsafemodifytag(mutexgear_completion_item_t *__item_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value)
{
	return mutexgear_completion_item_unsafemodifytag(__item_instance, __tag_index, __tag_value);
}

_MUTEXGEAR_PURE_INLINE 
bool _mutexgear_completion_item_gettag(mutexgear_completion_item_t *__item_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/)
{
	return mutexgear_completion_item_gettag(__item_instance, __tag_index);
}


int _mutexgear_completion_queue_init(mutexgear_completion_queue_t *__queue_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/);
int _mutexgear_completion_queue_destroy(mutexgear_completion_queue_t *__queue_instance);

int _mutexgear_completion_queue_preparedestroy(mutexgear_completion_queue_t *__queue_instance);
void _mutexgear_completion_queue_completedestroy(mutexgear_completion_queue_t *__queue_instance);
void _mutexgear_completion_queue_unpreparedestroy(mutexgear_completion_queue_t *__queue_instance);


_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_queue_lock(mutexgear_completion_locktoken_t *__out_lock_acquired/*=NULL*/, mutexgear_completion_queue_t *__queue_instance)
{
	int ret = _mutexgear_lock_acquire(&__queue_instance->access_lock);
	return ret == EOK && (__out_lock_acquired == NULL || (*__out_lock_acquired = _mutexgear_completion_queue_derivetoken(__queue_instance), true)) ? EOK : ret;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_queue_trylock(mutexgear_completion_locktoken_t *__out_lock_acquired/*=NULL*/, mutexgear_completion_queue_t *__queue_instance)
{
	int ret = _mutexgear_lock_tryacquire(&__queue_instance->access_lock);
	return ret == EOK && (__out_lock_acquired == NULL || (*__out_lock_acquired = _mutexgear_completion_queue_derivetoken(__queue_instance), true)) ? EOK : ret;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_queue_plainunlock(mutexgear_completion_queue_t *__queue_instance)
{
	int ret = _mutexgear_lock_release(&__queue_instance->access_lock);
	return ret;
}

int _mutexgear_completion_queue_unlockandwait(mutexgear_completion_queue_t *__queue_instance,
	mutexgear_completion_item_t *__item_to_be_waited, mutexgear_completion_waiter_t *__waiter_instance);


_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_queue_lodisempty(const mutexgear_completion_queue_t *__queue_instance)
{
	return mutexgear_completion_queue_lodisempty(__queue_instance);
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_queue_gettail(mutexgear_completion_item_t **__out_tail_item,
	mutexgear_completion_queue_t *__queue_instance)
{
	return mutexgear_completion_queue_gettail(__out_tail_item, __queue_instance);
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_queue_getpreceding(mutexgear_completion_item_t **__out_preceding_item,
	mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance)
{
	return mutexgear_completion_queue_getpreceding(__out_preceding_item, __queue_instance, __item_instance);
}

_MUTEXGEAR_PURE_INLINE
mutexgear_completion_item_t *_mutexgear_completion_queue_getunsafepreceding(mutexgear_completion_item_t *__item_instance)
{
	return mutexgear_completion_queue_getunsafepreceding(__item_instance);
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_queue_unsafegethead(mutexgear_completion_item_t **__out_head_item,
	mutexgear_completion_queue_t *__queue_instance)
{
	return mutexgear_completion_queue_unsafegethead(__out_head_item, __queue_instance);
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_queue_unsafegetnext(mutexgear_completion_item_t **__out_next_item,
	mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance)
{
	return mutexgear_completion_queue_unsafegetnext(__out_next_item, __queue_instance, __item_instance);
}


_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_queue_enqueue(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
{
	bool success = false;
	int ret, mutex_unlock_status;

	// bool mutex_locked = false;

	do 
	{
		if (__lock_hint == NULL && (ret = _mutexgear_lock_acquire(&__queue_instance->access_lock)) != EOK)
		{
			break;
		}
		// mutex_locked = true; -- no breaks after this point at this time

		mutexgear_dlralist_linkback(&__queue_instance->work_list, &__item_instance->work_item);
		
		if (__lock_hint == NULL)
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__queue_instance->access_lock)) == EOK); // Should succeed normally
		}
		// mutex_locked = false;

		ret = EOK;
		success = true;
	}
	while (false);

	if (!success)
	{
		// if (mutex_locked)
		// {
		// 	if (__lock_hint == NULL)
		// 	{
		// 		MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__queue_instance->access_lock)) == EOK); // Should succeed normally
		// 	}
		// }
	}

	return ret;
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_queue_unsafedequeue(mutexgear_completion_item_t *__item_instance)
{
	mutexgear_dlralist_unlink(&__item_instance->work_item);
}

void _mutexgear_completion_queueditem_commcompletiontowaiter(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, 
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_waiter_t *__item_waiter);

_MUTEXGEAR_PURE_INLINE 
int _mutexgear_completion_queueditem_finish(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
{
	bool success = false;
	int ret, mutex_unlock_status;

	// bool mutex_locked = false;

	do
	{
		if (__lock_hint == NULL && (ret = _mutexgear_lock_acquire(&__queue_instance->access_lock)) != EOK)
		{
			break;
		}
		// mutex_locked = true; -- no breaks after this point at this time

		mutexgear_dlralist_unlink(&__item_instance->work_item);

		if (__lock_hint == NULL)
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__queue_instance->access_lock)) == EOK); // Should succeed normally
		}
		// mutex_locked = false;

		void *current_worker = _mutexgear_completion_item_getwow(__item_instance);
		
		if (current_worker != (void *)__worker_instance)
		{
			MG_ASSERT(current_worker != NULL);

			mutexgear_completion_waiter_t *item_waiter = (mutexgear_completion_waiter_t *)current_worker;
			_mutexgear_completion_queueditem_commcompletiontowaiter(__queue_instance, __item_instance, __worker_instance, item_waiter);

			// Make sure no reinitialization is necessary
			MG_ASSERT(_mutexgear_completion_item_isasinit(__item_instance));
		}
		else
		{
			// Reinitialize the item to have it prepared for next uses 
			// (it is more convenient and logical to reinitialize the item after use in the routine 
			// rather that requiring the caller to do it before each item reuse).
			_mutexgear_completion_item_reinit(__item_instance);
		}

		ret = EOK;
		success = true;
	}
	while (false);

	if (!success)
	{
		// if (mutex_locked)
		// {
		// 	if (__lock_hint == NULL)
		// 	{
		// 		MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__queue_instance->access_lock)) == EOK); // Should succeed normally
		// 	}
		// }
	}

	return ret;
}


//////////////////////////////////////////////////////////////////////////
// Completion DrainableQueue APIs

int _mutexgear_completion_drain_init(mutexgear_completion_drain_t *__drain_instance);
int _mutexgear_completion_drain_destroy(mutexgear_completion_drain_t *__drain_instance);

int _mutexgear_completion_drain_preparedestroy(mutexgear_completion_drain_t *__drain_instance);
void _mutexgear_completion_drain_completedestroy(mutexgear_completion_drain_t *__drain_instance);
void _mutexgear_completion_drain_unpreparedestroy(mutexgear_completion_drain_t *__drain_instance);

int _mutexgear_completion_drainablequeue_init(mutexgear_completion_drainablequeue_t *__queue_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/);
int _mutexgear_completion_drainablequeue_destroy(mutexgear_completion_drainablequeue_t *__queue_instance);

int _mutexgear_completion_drainablequeue_preparedestroy(mutexgear_completion_drainablequeue_t *__queue_instance);
void _mutexgear_completion_drainablequeue_completedestroy(mutexgear_completion_drainablequeue_t *__queue_instance);
void _mutexgear_completion_drainablequeue_unpreparedestroy(mutexgear_completion_drainablequeue_t *__queue_instance);


_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_drainablequeue_lock(mutexgear_completion_locktoken_t *__out_lock_acquired/*=NULL*/,
	mutexgear_completion_drainablequeue_t *__queue_instance)
{
	int ret = _mutexgear_completion_queue_lock(__out_lock_acquired, &__queue_instance->basic_queue);
	return ret;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_drainablequeue_getindex(mutexgear_completion_drainidx_t *__out_queue_drain_index,
	mutexgear_completion_drainablequeue_t *__queue_instance, mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
{
	MG_ASSERT(__out_queue_drain_index != NULL);

	bool success = false;
	int ret, mutex_unlock_status;

	// bool mutex_locked = false;

	do
	{
		if (__lock_hint == NULL && (ret = _mutexgear_completion_queue_lock(NULL, &__queue_instance->basic_queue)) != EOK)
		{
			break;
		}
		// mutex_locked = true; -- no breaks after this point at this time

		mutexgear_completion_drainidx_t queue_drain_index = __queue_instance->drain_index;

		if (__lock_hint == NULL)
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__queue_instance->basic_queue)) == EOK); // Should succeed normally
		}
		// mutex_locked = false;

		*__out_queue_drain_index = queue_drain_index;
		MG_ASSERT(queue_drain_index != MUTEXGEAR_COMPLETION_INVALID_DRAINIDX);

		ret = EOK;
		success = true;
	}
	while (false);

	if (!success)
	{
		// if (mutex_locked)
		// {
		// 	if (__lock_hint == NULL)
		// 	{
		// 		MG_CHECK(mutex_unlock_status, (mutex_unlock_status = mutexgear_completion_plain_unlock_queue(&__queue_instance->basic_queue)) == EOK); // Should succeed normally
		// 	}
		// }
	}

	return ret;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_drainablequeue_drain(mutexgear_completion_drainablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__drain_head_item, mutexgear_completion_drainidx_t __item_drain_index,
	mutexgear_completion_drain_t *__target_drain, mutexgear_completion_locktoken_t __lock_hint/*=NULL*/, bool *__out_drain_execution_status/*=NULL*/)
{
	MG_ASSERT(__drain_head_item != NULL);
	MG_ASSERT(__item_drain_index != MUTEXGEAR_COMPLETION_INVALID_DRAINIDX);

	bool success = false;
	int ret, mutex_unlock_status;

	// bool mutex_locked = false;

	do
	{
		bool drain_execution_status = false;

		if (__lock_hint == NULL && (ret = _mutexgear_completion_queue_lock(NULL, &__queue_instance->basic_queue)) != EOK)
		{
			break;
		}
		// mutex_locked = true; -- no breaks after this point at this time

		mutexgear_dlraitem_t *p_work_begin = mutexgear_dlralist_getbegin(&__queue_instance->basic_queue.work_list);
		mutexgear_dlraitem_t *p_item_work = mutexgear_completion_item_getworkitem(__drain_head_item);
		if (p_item_work == p_work_begin || __item_drain_index == __queue_instance->drain_index)
		{
			mutexgear_dlraitem_t *p_work_end = mutexgear_dlralist_getend(&__queue_instance->basic_queue.work_list);
			mutexgear_dlralist_spliceback(&__target_drain->drain_list, p_item_work, p_work_end);

			__queue_instance->drain_index = _mutexgear_completion_drainidx_increment(__queue_instance->drain_index);
			drain_execution_status = true;
		}
		else
		{
			// Ignore the drain request if the item is not at the head and provided index does not match that in the queue:
			// the item might have already been already drained and it is not possible to quickly determine whether it was or was not.
		}

		if (__lock_hint == NULL)
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__queue_instance->basic_queue)) == EOK); // Should succeed normally
		}
		// mutex_locked = false;

		if (__out_drain_execution_status != NULL)
		{
			*__out_drain_execution_status = drain_execution_status;
		}

		ret = EOK;
		success = true;
	}
	while (false);

	if (!success)
	{
		// if (mutex_locked)
		// {
		// 	if (__lock_hint == NULL)
		// 	{
		// 		MG_CHECK(mutex_unlock_status, (mutex_unlock_status = mutexgear_completion_plain_unlock_queue(&__queue_instance->basic_queue)) == EOK); // Should succeed normally
		// 	}
		// }
	}

	return ret;
}


_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_drainablequeue_plainunlock(mutexgear_completion_drainablequeue_t *__queue_instance)
{
	int ret = _mutexgear_completion_queue_plainunlock(&__queue_instance->basic_queue);
	return ret;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_drainablequeue_unlockandwait(mutexgear_completion_drainablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_to_be_waited, mutexgear_completion_waiter_t *__waiter_instance)
{
	int ret = _mutexgear_completion_queue_unlockandwait(&__queue_instance->basic_queue, __item_to_be_waited, __waiter_instance);
	return ret;
}


_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_drainablequeue_lodisempty(const mutexgear_completion_drainablequeue_t *__queue_instance)
{
	bool ret = mutexgear_completion_drainablequeue_lodisempty(__queue_instance);
	return ret;
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_drainablequeue_gettail(mutexgear_completion_item_t **__out_preceding_item,
	mutexgear_completion_drainablequeue_t *__queue_instance)
{
	return mutexgear_completion_drainablequeue_gettail(__out_preceding_item, __queue_instance);
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_drainablequeue_getpreceding(mutexgear_completion_item_t **__out_preceding_item,
	mutexgear_completion_drainablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance)
{
	return mutexgear_completion_drainablequeue_getpreceding(__out_preceding_item, __queue_instance, __item_instance);
}

_MUTEXGEAR_PURE_INLINE
mutexgear_completion_item_t *_mutexgear_completion_drainablequeue_unsafegetpreceding(mutexgear_completion_item_t *__item_instance)
{
	return mutexgear_completion_drainablequeue_getunsafepreceding(__item_instance);
}


_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_drainablequeue_enqueue(mutexgear_completion_drainablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/, mutexgear_completion_drainidx_t *__out_queue_drain_index/*=NULL*/)
{
	bool success = false;
	int ret, mutex_unlock_status;

	bool mutex_locked = false;

	do
	{
		mutexgear_completion_locktoken_t lock_hint_to_use;

		if (__lock_hint == NULL ? (ret = _mutexgear_completion_queue_lock(&lock_hint_to_use, &__queue_instance->basic_queue)) != EOK : (lock_hint_to_use = __lock_hint, false))
		{
			break;
		}
		mutex_locked = true;

		MG_ASSERT(lock_hint_to_use == _mutexgear_completion_queue_derivetoken(&__queue_instance->basic_queue));

		// Explicitly override the value to make the optimizer's life easier
		lock_hint_to_use = _mutexgear_completion_queue_derivetoken(&__queue_instance->basic_queue);

		if (__out_queue_drain_index != NULL
			&& (ret = _mutexgear_completion_drainablequeue_getindex(__out_queue_drain_index, __queue_instance, lock_hint_to_use)) != EOK)
		{
			break;
		}

		if ((ret = _mutexgear_completion_queue_enqueue(&__queue_instance->basic_queue, __item_instance, lock_hint_to_use)) != EOK)
		{
			break;
		}

		if (__lock_hint == NULL)
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__queue_instance->basic_queue)) == EOK); // Should succeed normally
		}
		// mutex_locked = false;

		ret = EOK;
		success = true;
	}
	while (false);

	if (!success)
	{
		if (mutex_locked)
		{
			if (__lock_hint == NULL)
			{
				MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__queue_instance->basic_queue)) == EOK); // Should succeed normally
			}
		}
	}

	return ret;
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_drainablequeue_unsafedequeue(mutexgear_completion_item_t *__item_instance)
{
	_mutexgear_completion_queue_unsafedequeue(__item_instance);
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_drainablequeueditem_finish(mutexgear_completion_drainablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance,
	mutexgear_completion_drainidx_t __item_drain_index/*=MUTEXGEAR_COMPLETION_INVALID_DRAINIDX*/, mutexgear_completion_drain_t *__target_drain/*=NULL*/,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
{
	MG_ASSERT((__target_drain == NULL) == (__item_drain_index == MUTEXGEAR_COMPLETION_INVALID_DRAINIDX));

	bool success = false;
	int ret, mutex_unlock_status;

	bool mutex_locked = false;

	do
	{
		mutexgear_completion_locktoken_t lock_hint_to_use;

		if (__lock_hint == NULL ? (ret = _mutexgear_completion_queue_lock(&lock_hint_to_use, &__queue_instance->basic_queue)) != EOK : (lock_hint_to_use = __lock_hint, false))
		{
			break;
		}
		mutex_locked = true;

		MG_ASSERT(lock_hint_to_use == _mutexgear_completion_queue_derivetoken(&__queue_instance->basic_queue));

		// Explicitly override the value to make the optimizer's life easier
		lock_hint_to_use = _mutexgear_completion_queue_derivetoken(&__queue_instance->basic_queue);

		if (__target_drain != NULL
			&& (ret = _mutexgear_completion_drainablequeue_drain(__queue_instance, __item_instance, __item_drain_index, __target_drain, lock_hint_to_use, NULL)) != EOK)
		{
			break;
		}

		if ((ret = _mutexgear_completion_queueditem_finish(&__queue_instance->basic_queue, __item_instance, __worker_instance, lock_hint_to_use)) != EOK)
		{
			break;
		}

		if (__lock_hint == NULL)
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__queue_instance->basic_queue)) == EOK); // Should succeed normally
		}
		// mutex_locked = false;

		ret = EOK;
		success = true;
	}
	while (false);

	if (!success)
	{
		if (mutex_locked)
		{
			if (__lock_hint == NULL)
			{
				MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__queue_instance->basic_queue)) == EOK); // Should succeed normally
			}
		}
	}

	return ret;
}


//////////////////////////////////////////////////////////////////////////
// Completion CancelableQueue APIs



#endif // #ifndef __MUTEXGEAR_MG_COMPLETION_H_INCLUDED
