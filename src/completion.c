/************************************************************************/
/* The MutexGear Library                                                */
/* Mutex Completion API Implementation                                  */
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
 *	\brief MutexGear Completion API implementation
 *
 */


#include "completion.h"


//////////////////////////////////////////////////////////////////////////
 // Completion CancelableQueue Implementation

_MUTEXGEAR_PURE_INLINE void _mutexgear_completion_cancelablequeue_unsafedequeue(mutexgear_completion_item_t *__item_instance);

_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_cancelablequeue_init(mutexgear_completion_cancelablequeue_t *__queue_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/)
{
	int ret;

	do
	{
		if ((ret = _mutexgear_completion_queue_init(&__queue_instance->basic_queue, __attr)) != EOK)
		{
			break;
		}

		MG_ASSERT(ret == EOK);
	}
	while (false);

	return ret;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_cancelablequeue_destroy(mutexgear_completion_cancelablequeue_t *__queue_instance)
{
	int ret;

	do
	{
		if ((ret = _mutexgear_completion_queue_destroy(&__queue_instance->basic_queue)) != EOK)
		{
			break;
		}

		MG_ASSERT(ret == EOK);
	}
	while (false);

	return ret;
}


_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_cancelablequeue_lock(mutexgear_completion_locktoken_t *__out_acquired_lock/*=NULL*/,
	mutexgear_completion_cancelablequeue_t *__queue_instance)
{
	int ret = _mutexgear_completion_queue_lock(__out_acquired_lock, &__queue_instance->basic_queue);
	return ret;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_cancelablequeue_plainunlock(mutexgear_completion_cancelablequeue_t *__queue_instance)
{
	int ret = _mutexgear_completion_queue_plainunlock(&__queue_instance->basic_queue);
	return ret;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_cancelablequeue_unlockandwait(mutexgear_completion_cancelablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_to_be_waited, mutexgear_completion_waiter_t *__waiter_instance)
{
	int ret = _mutexgear_completion_queue_unlockandwait(&__queue_instance->basic_queue, __item_to_be_waited, __waiter_instance);
	return ret;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_cancelablequeue_unlockandcancel(mutexgear_completion_cancelablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_to_be_canceled, mutexgear_completion_waiter_t *__waiter_instance,
	void(*__item_cancel_fn/*=NULL*/)(void *__cancel_context, mutexgear_completion_cancelablequeue_t *__queue, mutexgear_completion_worker_t *__worker, mutexgear_completion_item_t *__item), void *__cancel_context/*=NULL*/,
	mutexgear_completion_ownership_t *__out_item_resulting_ownership)
{
	int ret, wait_detach_lock_status, mutex_unlock_status;
	mutexgear_completion_ownership_t item_resulting_ownership;

	do
	{
		mutexgear_completion_worker_t *worker_instance = (mutexgear_completion_worker_t *)_mutexgear_completion_item_getwow(__item_to_be_canceled);

		if ((void *)worker_instance != (void *)__item_to_be_canceled) // != NULL
		{
			wait_detach_lock_status = _mutexgear_lock_acquire(&__waiter_instance->wait_detach_lock);

			if (wait_detach_lock_status == EOK)
			{
				_mutexgear_completion_itemdata_settag(&__item_to_be_canceled->data, mutexgear_completion_cancelablequeue_itemtag_cancelrequested, true);

				_mutexgear_completion_item_barriersetwow(__item_to_be_canceled, __waiter_instance);
			}
		}
		else
		{
			_mutexgear_completion_cancelablequeue_unsafedequeue(__item_to_be_canceled);
		}

		// Due to the function contract, the mutex must be unlocked regardless of the return status
		MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__queue_instance->basic_queue)) == EOK);

		if ((void *)worker_instance != (void *)__item_to_be_canceled) // != NULL
		{
			if (wait_detach_lock_status != EOK)
			{
				ret = wait_detach_lock_status;
				break;
			}

			if (__item_cancel_fn != NULL)
			{
				__item_cancel_fn(__cancel_context, __queue_instance, worker_instance, __item_to_be_canceled);
			}

			// After the wait_info has been made available to the worker there is no way to cancel the waiting -- it must be executed in full
			_mutexgear_completion_wait_item_completion_and_detach(&__queue_instance->basic_queue, __item_to_be_canceled, __waiter_instance, worker_instance);
			item_resulting_ownership = mg_completion_not_owner;
		}
		else
		{
			item_resulting_ownership = mg_completion_owner;
		}

		*__out_item_resulting_ownership = item_resulting_ownership;
		ret = EOK;
	}
	while (false);

	return ret;
}


_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_cancelablequeue_enqueue(mutexgear_completion_cancelablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
{
	int ret = _mutexgear_completion_queue_enqueue(&__queue_instance->basic_queue, __item_instance, __lock_hint);
	return ret;
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_cancelablequeue_unsafedequeue(mutexgear_completion_item_t *__item_instance)
{
	_mutexgear_completion_queue_unsafedequeue(__item_instance);
}

// _MUTEXGEAR_PURE_INLINE
// int _mutexgear_completion_cancelablequeue_locateandstart(mutexgear_completion_item_t **__out_acquired_item,
// 	mutexgear_completion_cancelablequeue_t *__queue_instance, mutexgear_completion_worker_t *__worker_instance,
// 	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
// {
// 	MG_ASSERT(__worker_instance != NULL);
// 
// 	bool success = false;
// 	mutexgear_completion_item_t *acquired_item = NULL;
// 	int ret, mutex_unlock_status;
// 
// 	// bool mutex_locked = false;
// 
// 	do
// 	{
// 		if (__lock_hint == NULL && (ret = _mutexgear_completion_queue_lock(NULL, &__queue_instance->basic_queue)) != EOK)
// 		{
// 			break;
// 		}
// 		// mutex_locked = true; -- no breaks after this point at this time
// 
// 		mutexgear_completion_item_t *current_item;
// 		bool continue_loop = _mutexgear_completion_queue_unsafegethead(&current_item, &__queue_instance->basic_queue);
// 		for (; continue_loop; continue_loop = _mutexgear_completion_queue_unsafegetnext(&current_item, &__queue_instance->basic_queue, current_item))
// 		{
// 			if (!_mutexgear_completion_item_isstarted(current_item))
// 			{
// 				_mutexgear_completion_cancelablequeueditem_start(current_item, __worker_instance);
// 				acquired_item = current_item;
// 				break;
// 			}
// 		}
// 
// 		if (__lock_hint == NULL)
// 		{
// 			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__queue_instance->basic_queue)) == EOK); // Should succeed normally
// 		}
// 		// mutex_locked = false;
// 
// 		*__out_acquired_item = acquired_item != NULL ? acquired_item : NULL;
// 		ret = EOK;
// 		success = true;
// 	}
// 	while (false);
// 
// 	if (!success)
// 	{
// 		// if (mutex_locked)
// 		// {
// 		// 	if (__lock_hint == NULL)
// 		// 	{
// 		// 		MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__queue_instance->basic_queue.access_lock)) == EOK); // Should succeed normally
// 		// 	}
// 		// }
// 	}
// 
// 	return ret;
// }

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_cancelablequeueditem_iscanceled(const mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
{
	MG_ASSERT(__worker_instance != NULL);

	bool ret = false;

	// First, execute a more relaxed access as a waiter is not likely to be assigned 
	void *current_worker = _mutexgear_completion_item_getwow(__item_instance);

	if (current_worker != (void *)__worker_instance)
	{
		// Then, execute the more strict version to ensure cancel request write visibility
		void *current_worker_recheck = _mutexgear_completion_item_barriergetwow(__item_instance);
		// The same value should be returned again...
		MG_VERIFY(current_worker_recheck == current_worker);
		// ...and it is not to be NULL
		MG_ASSERT(current_worker != (void *)__item_instance);

		// Finally, check if the cancel flag is set
		ret = _mutexgear_completion_itemdata_gettag(&__item_instance->data, mutexgear_completion_cancelablequeue_itemtag_cancelrequested);
	}

	return ret;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_cancelablequeueditem_safefinish(mutexgear_completion_cancelablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
{
	int ret;

	do
	{
		if ((ret = _mutexgear_completion_queueditem_safefinish(&__queue_instance->basic_queue, __item_instance, __worker_instance)) != EOK)
		{
			break;
		}

		// Clear the cancel request tag that might have been set for the item
		_mutexgear_completion_itemdata_settag(&__item_instance->data, mutexgear_completion_cancelablequeue_itemtag_cancelrequested, false);

		MG_ASSERT(ret == EOK);
	}
	while (false);

	return ret;
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_cancelablequeueditem_unsafefinish__locked(mutexgear_completion_item_t *__item_instance)
{
	_mutexgear_completion_queueditem_unsafefinish__locked(__item_instance);
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_cancelablequeueditem_unsafefinish__unlocked(mutexgear_completion_cancelablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
{
	_mutexgear_completion_queueditem_unsafefinish__unlocked(&__queue_instance->basic_queue, __item_instance, __worker_instance);

	// Clear the cancel request tag that might have been set for the item
	_mutexgear_completion_itemdata_settag(&__item_instance->data, mutexgear_completion_cancelablequeue_itemtag_cancelrequested, false);
}


//////////////////////////////////////////////////////////////////////////
// Completion Attribute Public APIs Implementation

/*_MUTEXGEAR_API */
int mutexgear_completion_genattr_init(mutexgear_completion_genattr_t *__attr)
{
	return _mutexgear_completion_genattr_init(__attr);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_genattr_destroy(mutexgear_completion_genattr_t *__attr)
{
	return _mutexgear_completion_genattr_destroy(__attr);
}


/*_MUTEXGEAR_API */
int mutexgear_completion_genattr_getpshared(const mutexgear_completion_genattr_t *__attr, int *__pshared)
{
	return _mutexgear_completion_genattr_getpshared(__attr, __pshared);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_genattr_setpshared(mutexgear_completion_genattr_t *__attr, int __pshared)
{
	return _mutexgear_completion_genattr_setpshared(__attr, __pshared);
}


/*_MUTEXGEAR_API */
int mutexgear_completion_genattr_getprioceiling(const mutexgear_completion_genattr_t *__attr, int *__prioceiling)
{
	return _mutexgear_completion_genattr_getprioceiling(__attr, __prioceiling);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_genattr_getprotocol(const mutexgear_completion_genattr_t *__attr, int *__protocol)
{
	return _mutexgear_completion_genattr_getprotocol(__attr, __protocol);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_genattr_setprioceiling(mutexgear_completion_genattr_t *__attr, int __prioceiling)
{
	return _mutexgear_completion_genattr_setprioceiling(__attr, __prioceiling);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_genattr_setprotocol(mutexgear_completion_genattr_t *__attr, int __protocol)
{
	return _mutexgear_completion_genattr_setprotocol(__attr, __protocol);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_genattr_setmutexattr(mutexgear_completion_genattr_t *__attr, const _MUTEXGEAR_LOCKATTR_T *__mutexattr)
{
	return _mutexgear_completion_genattr_setmutexattr(__attr, __mutexattr);
}


//////////////////////////////////////////////////////////////////////////
// Completion Queue Public APIs Implementation

/*_MUTEXGEAR_API */
int mutexgear_completion_worker_init(mutexgear_completion_worker_t *__worker_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/)
{
	return _mutexgear_completion_worker_init(__worker_instance, __attr);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_worker_destroy(mutexgear_completion_worker_t *__worker_instance)
{
	return _mutexgear_completion_worker_destroy(__worker_instance);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_worker_lock(mutexgear_completion_worker_t *__worker_instance)
{
	return _mutexgear_completion_worker_lock(__worker_instance);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_worker_unlock(mutexgear_completion_worker_t *__worker_instance)
{
	return _mutexgear_completion_worker_unlock(__worker_instance);
}


/*_MUTEXGEAR_API */
int mutexgear_completion_waiter_init(mutexgear_completion_waiter_t *__waiter_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/)
{
	return _mutexgear_completion_waiter_init(__waiter_instance, __attr);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_waiter_destroy(mutexgear_completion_waiter_t *__waiter_instance)
{
	return _mutexgear_completion_waiter_destroy(__waiter_instance);
}


/*_MUTEXGEAR_API */
int mutexgear_completion_queue_init(mutexgear_completion_queue_t *__queue_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/)
{
	return _mutexgear_completion_queue_init(__queue_instance, __attr);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_queue_destroy(mutexgear_completion_queue_t *__queue_instance)
{
	return _mutexgear_completion_queue_destroy(__queue_instance);
}


/*_MUTEXGEAR_API */
int mutexgear_completion_queue_lock(mutexgear_completion_locktoken_t *__out_acquired_lock/*=NULL*/, mutexgear_completion_queue_t *__queue_instance)
{
	return _mutexgear_completion_queue_lock(__out_acquired_lock, __queue_instance);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_queue_plainunlock(mutexgear_completion_queue_t *__queue_instance)
{
	return _mutexgear_completion_queue_plainunlock(__queue_instance);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_queue_unlockandwait(mutexgear_completion_queue_t *__queue_instance,
	mutexgear_completion_item_t *__item_to_be_waited, mutexgear_completion_waiter_t *__waiter_instance)
{
	return _mutexgear_completion_queue_unlockandwait(__queue_instance, __item_to_be_waited, __waiter_instance);
}


/*_MUTEXGEAR_API */
int mutexgear_completion_queue_enqueue(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
{
	return _mutexgear_completion_queue_enqueue(__queue_instance, __item_instance, __lock_hint);
}

/*_MUTEXGEAR_API */
void mutexgear_completion_queue_unsafedequeue(mutexgear_completion_item_t *__item_instance)
{
	_mutexgear_completion_queue_unsafedequeue(__item_instance);
}


/*_MUTEXGEAR_API */
int mutexgear_completion_queueditem_safefinish(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
{
	return _mutexgear_completion_queueditem_safefinish(__queue_instance, __item_instance, __worker_instance);
}

/*_MUTEXGEAR_API */
void mutexgear_completion_queueditem_unsafefinish__locked(mutexgear_completion_item_t *__item_instance)
{
	_mutexgear_completion_queueditem_unsafefinish__locked(__item_instance);
}

/*_MUTEXGEAR_API */
void mutexgear_completion_queueditem_unsafefinish__unlocked(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
{
	_mutexgear_completion_queueditem_unsafefinish__unlocked(__queue_instance, __item_instance, __worker_instance);
}

//////////////////////////////////////////////////////////////////////////
// Completion DrainableQueue Public APIs Implementation

/*_MUTEXGEAR_API */
int mutexgear_completion_drain_init(mutexgear_completion_drain_t *__drain_instance)
{
	return _mutexgear_completion_drain_init(__drain_instance);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_drain_destroy(mutexgear_completion_drain_t *__drain_instance)
{
	return _mutexgear_completion_drain_destroy(__drain_instance);
}


/*_MUTEXGEAR_API */
int mutexgear_completion_drainablequeue_init(mutexgear_completion_drainablequeue_t *__queue_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/)
{
	return _mutexgear_completion_drainablequeue_init(__queue_instance, __attr);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_drainablequeue_destroy(mutexgear_completion_drainablequeue_t *__queue_instance)
{
	return _mutexgear_completion_drainablequeue_destroy(__queue_instance);
}


/*_MUTEXGEAR_API */
int mutexgear_completion_drainablequeue_lock(mutexgear_completion_locktoken_t *__out_acquired_lock/*=NULL*/,
	mutexgear_completion_drainablequeue_t *__queue_instance)
{
	return _mutexgear_completion_drainablequeue_lock(__out_acquired_lock, __queue_instance);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_drainablequeue_getindex(mutexgear_completion_drainidx_t *__out_queue_drain_index,
	mutexgear_completion_drainablequeue_t *__queue_instance, mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
{
	return _mutexgear_completion_drainablequeue_getindex(__out_queue_drain_index, __queue_instance, __lock_hint);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_drainablequeue_safedrain(mutexgear_completion_drainablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__drain_head_item, mutexgear_completion_drainidx_t __item_drain_index,
	mutexgear_completion_drain_t *__target_drain, bool *__out_drain_execution_status/*=NULL*/)
{
	return _mutexgear_completion_drainablequeue_safedrain(__queue_instance, __drain_head_item, __item_drain_index, __target_drain, __out_drain_execution_status);
}

/*_MUTEXGEAR_API */
void mutexgear_completion_drainablequeue_unsafedrain__locked(mutexgear_completion_drainablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__drain_head_item, mutexgear_completion_drainidx_t __item_drain_index,
	mutexgear_completion_drain_t *__target_drain, bool *__out_drain_execution_status/*=NULL*/)
{
	_mutexgear_completion_drainablequeue_unsafedrain__locked(__queue_instance, __drain_head_item, __item_drain_index, __target_drain, __out_drain_execution_status);
}


/*_MUTEXGEAR_API */
int mutexgear_completion_drainablequeue_plainunlock(mutexgear_completion_drainablequeue_t *__queue_instance)
{
	return _mutexgear_completion_drainablequeue_plainunlock(__queue_instance);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_drainablequeue_unlockandwait(mutexgear_completion_drainablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_to_be_waited, mutexgear_completion_waiter_t *__waiter_instance)
{
	return _mutexgear_completion_drainablequeue_unlockandwait(__queue_instance, __item_to_be_waited, __waiter_instance);
}


/*_MUTEXGEAR_API */
int mutexgear_completion_drainablequeue_enqueue(mutexgear_completion_drainablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/, mutexgear_completion_drainidx_t *__out_queue_drain_index/*=NULL*/)
{
	return _mutexgear_completion_drainablequeue_enqueue(__queue_instance, __item_instance, __lock_hint, __out_queue_drain_index);
}

/*_MUTEXGEAR_API */
void mutexgear_completion_drainablequeue_unsafedequeue(mutexgear_completion_item_t *__item_instance)
{
	_mutexgear_completion_drainablequeue_unsafedequeue(__item_instance);
}


/*_MUTEXGEAR_API */
int mutexgear_completion_drainablequeueditem_safefinish(mutexgear_completion_drainablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance,
	mutexgear_completion_drainidx_t __item_drain_index/*=MUTEXGEAR_COMPLETION_INVALID_DRAINIDX*/, mutexgear_completion_drain_t *__target_drain/*=NULL*/)
{
	return _mutexgear_completion_drainablequeueditem_safefinish(__queue_instance, __item_instance, __worker_instance, __item_drain_index, __target_drain);
}

/*_MUTEXGEAR_API */
void mutexgear_completion_drainablequeueditem_unsafefinish__locked(mutexgear_completion_drainablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_instance,
	mutexgear_completion_drainidx_t __item_drain_index/*=MUTEXGEAR_COMPLETION_INVALID_DRAINIDX*/, mutexgear_completion_drain_t *__target_drain/*=NULL*/)
{
	_mutexgear_completion_drainablequeueditem_unsafefinish__locked(__queue_instance, __item_instance, __item_drain_index, __target_drain);
}

/*_MUTEXGEAR_API */
void mutexgear_completion_drainablequeueditem_unsafefinish__unlocked(mutexgear_completion_drainablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
{
	_mutexgear_completion_drainablequeueditem_unsafefinish__unlocked(__queue_instance, __item_instance, __worker_instance);
}


//////////////////////////////////////////////////////////////////////////
// Completion CancelableQueue Public APIs Implementation

/*_MUTEXGEAR_API */
int mutexgear_completion_cancelablequeue_init(mutexgear_completion_cancelablequeue_t *__queue_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/)
{
	return _mutexgear_completion_cancelablequeue_init(__queue_instance, __attr);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_cancelablequeue_destroy(mutexgear_completion_cancelablequeue_t *__queue_instance)
{
	return _mutexgear_completion_cancelablequeue_destroy(__queue_instance);
}


/*_MUTEXGEAR_API */
int mutexgear_completion_cancelablequeue_lock(mutexgear_completion_locktoken_t *__out_acquired_lock/*=NULL*/, mutexgear_completion_cancelablequeue_t *__queue_instance)
{
	return _mutexgear_completion_cancelablequeue_lock(__out_acquired_lock, __queue_instance);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_cancelablequeue_plainunlock(mutexgear_completion_cancelablequeue_t *__queue_instance)
{
	return _mutexgear_completion_cancelablequeue_plainunlock(__queue_instance);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_cancelablequeue_unlockandwait(mutexgear_completion_cancelablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_to_be_waited, mutexgear_completion_waiter_t *__waiter_instance)
{
	return _mutexgear_completion_cancelablequeue_unlockandwait(__queue_instance, __item_to_be_waited, __waiter_instance);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_cancelablequeue_unlockandcancel(mutexgear_completion_cancelablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_to_be_canceled, mutexgear_completion_waiter_t *__waiter_instance,
	void(*__item_cancel_fn/*=NULL*/)(void *__cancel_context, mutexgear_completion_cancelablequeue_t *__queue, mutexgear_completion_worker_t *__worker, mutexgear_completion_item_t *__item), void *__cancel_context/*=NULL*/,
	mutexgear_completion_ownership_t *__out_item_resulting_ownership)
{
	return _mutexgear_completion_cancelablequeue_unlockandcancel(__queue_instance, __item_to_be_canceled, __waiter_instance, __item_cancel_fn, __cancel_context, __out_item_resulting_ownership);
}


/*_MUTEXGEAR_API */
int mutexgear_completion_cancelablequeue_enqueue(mutexgear_completion_cancelablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
{
	return _mutexgear_completion_cancelablequeue_enqueue(__queue_instance, __item_instance, __lock_hint);
}

/*_MUTEXGEAR_API */
void mutexgear_completion_cancelablequeue_unsafedequeue(mutexgear_completion_item_t *__item_instance)
{
	_mutexgear_completion_cancelablequeue_unsafedequeue(__item_instance);
}

// /*_MUTEXGEAR_API */
// int mutexgear_completion_cancelablequeue_locateandstart(mutexgear_completion_item_t **__out_acquired_item,
// 	mutexgear_completion_cancelablequeue_t *__queue_instance, mutexgear_completion_worker_t *__worker_instance,
// 	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
// {
// 	return _mutexgear_completion_cancelablequeue_locateandstart(__out_acquired_item, __queue_instance, __worker_instance, __lock_hint);
// }


/*_MUTEXGEAR_API */
bool mutexgear_completion_cancelablequeueditem_iscanceled(const mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
{
	return _mutexgear_completion_cancelablequeueditem_iscanceled(__item_instance, __worker_instance);
}

/*_MUTEXGEAR_API */
int mutexgear_completion_cancelablequeueditem_safefinish(mutexgear_completion_cancelablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
{
	return _mutexgear_completion_cancelablequeueditem_safefinish(__queue_instance, __item_instance, __worker_instance);
}

_MUTEXGEAR_API 
void mutexgear_completion_cancelablequeueditem_unsafefinish__locked(mutexgear_completion_item_t *__item_instance)
{
	_mutexgear_completion_cancelablequeueditem_unsafefinish__locked(__item_instance);
}

_MUTEXGEAR_API 
void mutexgear_completion_cancelablequeueditem_unsafefinish__unlocked(mutexgear_completion_cancelablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
{
	_mutexgear_completion_cancelablequeueditem_unsafefinish__unlocked(__queue_instance, __item_instance, __worker_instance);
}
