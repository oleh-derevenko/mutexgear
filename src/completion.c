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
/* Copyright (c) 2016-2020 Oleh Derevenko. All rights are reserved.     */
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
#include <errno.h>


//////////////////////////////////////////////////////////////////////////
// Completion Attributes Implementation


 /*extern */
int _mutexgear_completion_genattr_init(mutexgear_completion_genattr_t *__attr)
{
	int ret = _mutexgear_lockattr_init(&__attr->lock_attr);
	return ret;
}

/*extern */
int _mutexgear_completion_genattr_destroy(mutexgear_completion_genattr_t *__attr)
{
	int ret = _mutexgear_lockattr_destroy(&__attr->lock_attr);
	return ret;
}


/*extern */
int _mutexgear_completion_genattr_getpshared(const mutexgear_completion_genattr_t *__attr, int *__out_pshared)
{
	int ret = _mutexgear_lockattr_getpshared(&__attr->lock_attr, __out_pshared);
	return ret;
}

/*extern */
int _mutexgear_completion_genattr_setpshared(mutexgear_completion_genattr_t *__attr, int __pshared)
{
	int ret = _mutexgear_lockattr_setpshared(&__attr->lock_attr, __pshared);
	return ret;
}


/*extern */
int _mutexgear_completion_genattr_getprioceiling(const mutexgear_completion_genattr_t *__attr, int *__out_prioceiling)
{
	int ret = _mutexgear_lockattr_getprioceiling(&__attr->lock_attr, __out_prioceiling);
	return ret;
}

/*extern */
int _mutexgear_completion_genattr_getprotocol(const mutexgear_completion_genattr_t *__attr, int *__out_protocol)
{
	int ret = _mutexgear_lockattr_getprotocol(&__attr->lock_attr, __out_protocol);
	return ret;
}

/*extern */
int _mutexgear_completion_genattr_setprioceiling(mutexgear_completion_genattr_t *__attr, int __prioceiling)
{
	int ret = _mutexgear_lockattr_setprioceiling(&__attr->lock_attr, __prioceiling);
	return ret;
}

/*extern */
int _mutexgear_completion_genattr_setprotocol(mutexgear_completion_genattr_t *__attr, int __protocol)
{
	int ret = _mutexgear_lockattr_setprotocol(&__attr->lock_attr, __protocol);
	return ret;
}

/*extern */
int _mutexgear_completion_genattr_setmutexattr(mutexgear_completion_genattr_t *__attr, const _MUTEXGEAR_LOCKATTR_T *__mutexattr)
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


//////////////////////////////////////////////////////////////////////////
// Completion Queue Implementation

/*extern */
int _mutexgear_completion_worker_init(mutexgear_completion_worker_t *__worker_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/)
{
	bool success = false;
	int ret, wheelattr_destroy_status;
	
	mutexgear_wheelattr_t wheelattr;
	bool wheelattr_was_allocated = false;

	do
	{
		if (__attr != NULL)
		{
			if ((ret = mutexgear_wheelattr_init(&wheelattr)) != EOK)
			{
				break;
			}
		}
		wheelattr_was_allocated = true;

		if (__attr != NULL)
		{
			if ((ret = mutexgear_wheelattr_setmutexattr(&wheelattr, &__attr->lock_attr)) != EOK)
			{
				break;
			}
		}

		if ((ret = mutexgear_wheel_init(&__worker_instance->progress_wheel, __attr != NULL ? &wheelattr : NULL)) != EOK)
		{
			break;
		}

		if (__attr != NULL)
		{
			MG_CHECK(wheelattr_destroy_status, (wheelattr_destroy_status = mutexgear_wheelattr_destroy(&wheelattr)) == EOK); // This should succeed normally
		}
		// wheelattr_was_allocated = false;

		ret = EOK;
		
		success = true;
	}
	while (false);

	if (!success)
	{
		if (wheelattr_was_allocated)
		{
			if (__attr != NULL)
			{
				MG_CHECK(wheelattr_destroy_status, (wheelattr_destroy_status = mutexgear_wheelattr_destroy(&wheelattr)) == EOK); // This should succeed normally
			}
		}
	}

	return ret;
}

/*extern */
int _mutexgear_completion_worker_destroy(mutexgear_completion_worker_t *__worker_instance)
{
	int ret;

	do
	{
		if ((ret = mutexgear_wheel_destroy(&__worker_instance->progress_wheel)) != EOK)
		{
			break;
		}

		MG_ASSERT(ret == EOK);
	}
	while (false);

	return ret;
}


/*extern */
int _mutexgear_completion_worker_lock(mutexgear_completion_worker_t *__worker_instance)
{
	int ret = mutexgear_wheel_lockslave(&__worker_instance->progress_wheel);
	return ret;
}

/*extern */
int _mutexgear_completion_worker_unlock(mutexgear_completion_worker_t *__worker_instance)
{
	int ret = mutexgear_wheel_unlockslave(&__worker_instance->progress_wheel);
	return ret;
}


/*extern */
int _mutexgear_completion_waiter_init(mutexgear_completion_waiter_t *__waiter_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/)
{
	int ret;

	do 
	{
		if ((ret = _mutexgear_lock_init(&__waiter_instance->wait_detach_lock, __attr != NULL ? &__attr->lock_attr : NULL)) != EOK)
		{
			break;
		}

		MG_ASSERT(ret == EOK);
	}
	while (false);

	return ret;
}

/*extern */
int _mutexgear_completion_waiter_destroy(mutexgear_completion_waiter_t *__waiter_instance)
{
	int ret;

	do
	{
		if ((ret = _mutexgear_lock_destroy(&__waiter_instance->wait_detach_lock)) != EOK)
		{
			break;
		}

		MG_ASSERT(ret == EOK);
	}
	while (false);

	return ret;
}


/*extern */
int _mutexgear_completion_queue_init(mutexgear_completion_queue_t *__queue_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/)
{
	bool success = false;
	int ret, mutex_destroy_status;

	bool access_was_initialized = false;

	do
	{
		if ((ret = _mutexgear_lock_init(&__queue_instance->access_lock, __attr != NULL ? &__attr->lock_attr : NULL)) != EOK)
		{
			break;
		}
		access_was_initialized = true;

		if ((ret = _mutexgear_lock_init(&__queue_instance->worker_detach_lock, __attr != NULL ? &__attr->lock_attr : NULL)) != EOK)
		{
			break;
		}

		mutexgear_dlralist_init(&__queue_instance->work_list);

		MG_ASSERT(ret == EOK);

		success = true;
	}
	while (false);

	if (!success)
	{
		if (access_was_initialized)
		{
			MG_CHECK(mutex_destroy_status, (mutex_destroy_status = _mutexgear_lock_destroy(&__queue_instance->access_lock)) == EOK); // This should succeed normally
		}
	}

	return ret;
}

/*extern */
int _mutexgear_completion_queue_destroy(mutexgear_completion_queue_t *__queue_instance)
{
	int ret;

	do
	{
		if ((ret = _mutexgear_completion_queue_preparedestroy(__queue_instance)) != EOK)
		{
			break;
		}

		_mutexgear_completion_queue_completedestroy(__queue_instance);

		ret = EOK;
	}
	while (false);

	return ret;
}


/*extern */
int _mutexgear_completion_queue_preparedestroy(mutexgear_completion_queue_t *__queue_instance)
{
	int ret, mutex_unlock_status;

	bool access_lock_acquired = false;

	do
	{
		// The queue is not supposed to be destroyed while there are items in it.
		if (!mutexgear_dlralist_isempty(&__queue_instance->work_list))
		{
			ret = EBUSY;
			break;
		}

		if ((ret = _mutexgear_lock_tryacquire(&__queue_instance->access_lock)) != EOK)
		{
			break;
		}
		access_lock_acquired = true;

		if ((ret = _mutexgear_lock_tryacquire(&__queue_instance->worker_detach_lock)) != EOK)
		{
			break;
		}

		ret = EOK;
	}
	while (false);

	if (ret != EOK)
	{
		if (access_lock_acquired)
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__queue_instance->access_lock)) == EOK);
		}
	}

	return ret;
}

/*extern */
void _mutexgear_completion_queue_completedestroy(mutexgear_completion_queue_t *__queue_instance)
{
	int mutex_unlock_status, mutex_destroy_status, list_emptiness_check;

	MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__queue_instance->worker_detach_lock)) == EOK); // This should succeed normally
	MG_CHECK(mutex_destroy_status, (mutex_destroy_status = _mutexgear_lock_destroy(&__queue_instance->worker_detach_lock)) == EOK); // This should succeed normally

	MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__queue_instance->access_lock)) == EOK); // This should succeed normally
	MG_CHECK(mutex_destroy_status, (mutex_destroy_status = _mutexgear_lock_destroy(&__queue_instance->access_lock)) == EOK); // This should succeed normally

	MG_CHECK(list_emptiness_check, mutexgear_dlralist_isempty(&__queue_instance->work_list) || (list_emptiness_check = EBUSY, false));

	mutexgear_dlralist_destroy(&__queue_instance->work_list);
}

/*extern */
void _mutexgear_completion_queue_unpreparedestroy(mutexgear_completion_queue_t *__queue_instance)
{
	int mutex_unlock_status;

	MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__queue_instance->worker_detach_lock)) == EOK); // This should succeed normally
	MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__queue_instance->access_lock)) == EOK); // This should succeed normally
}


/*extern */
void _mutexgear_completion_queueditem_commcompletiontowaiter(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, 
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_waiter_t *__item_waiter)
{
	int mutex_lock_status, mutex_unlock_status, wheel_roll_status;

	// Acquire the worker detach lock to allow the waiter to know when this thread finishes its access to the item_waiter
	MG_CHECK(mutex_lock_status, (mutex_lock_status = _mutexgear_lock_acquire(&__queue_instance->worker_detach_lock)) == EOK); // No way to handle -- must succeed

	// Assign NULL pointer to the shared variable to let the waiter know the work is completed
	_mutexgear_completion_item_setwow(__item_instance, __item_instance); // = NULL
	// Roll the worker wheel to notify the waiter about progress
	MG_CHECK(wheel_roll_status, (wheel_roll_status = mutexgear_wheel_slaveroll(&__worker_instance->progress_wheel)) == EOK); // No way to handle -- must succeed
			
	// Acquire and immediately release the wait detach lock to know the waiter finished accessing the worker pointer it had
	MG_CHECK(mutex_lock_status, (mutex_lock_status = _mutexgear_lock_acquire(&__item_waiter->wait_detach_lock)) == EOK); // No way to handle -- must succeed
	MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__item_waiter->wait_detach_lock)) == EOK); // No way to handle -- must succeed

	// Release the worker to signal the waiter this thread has finished its operations on the item_waiter
	MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__queue_instance->worker_detach_lock)) == EOK); // No way to handle -- must succeed
}


static void _mutexgear_completion_wait_item_completion_and_detach(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_to_be_waited,
	mutexgear_completion_waiter_t *__waiter_instance, mutexgear_completion_worker_t *__worker_instance);

/*extern */
int _mutexgear_completion_queue_unlockandwait(mutexgear_completion_queue_t *__queue_instance,
	mutexgear_completion_item_t *__item_to_be_waited, mutexgear_completion_waiter_t *__waiter_instance)
{
	int ret, wait_detach_lock_status, mutex_unlock_status;

	do
	{
		mutexgear_completion_worker_t *worker_instance = (mutexgear_completion_worker_t *)_mutexgear_completion_item_getwow(__item_to_be_waited);

		if ((void *)worker_instance != (void *)__item_to_be_waited) // != NULL
		{
			wait_detach_lock_status = _mutexgear_lock_acquire(&__waiter_instance->wait_detach_lock);

			if (wait_detach_lock_status == EOK)
			{
				_mutexgear_completion_item_setwow(__item_to_be_waited, __waiter_instance);
			}
		}

		// Due to the function contract, the mutex must be unlocked regardless of the return status
		MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__queue_instance->access_lock)) == EOK);

		if ((void *)worker_instance == (void *)__item_to_be_waited) // == NULL
		{
			ret = ESRCH;
			break;
		}

		if (wait_detach_lock_status != EOK)
		{
			ret = wait_detach_lock_status;
			break;
		}

		// After the wait_info has been made available to the worker there is no way to cancel the waiting -- it must be executed in full
		_mutexgear_completion_wait_item_completion_and_detach(__queue_instance, __item_to_be_waited, __waiter_instance, worker_instance);

		ret = EOK;
	}
	while (false);

	return ret;
}


static
void _mutexgear_completion_wait_item_completion_and_detach(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_to_be_waited,
	mutexgear_completion_waiter_t *__waiter_instance, mutexgear_completion_worker_t *__worker_instance)
{
	int wheel_grip_status, wheel_turn_status, wheel_release_status, mutex_unlock_status, mutex_lock_status;

	bool enter_loop = false, continue_loop;

	// Check if the worker has not marked the item as completed yet...
	if (_mutexgear_completion_item_getwow(__item_to_be_waited) != (void *)__item_to_be_waited) // != NULL
	{
		// ... grip on to the wheel if not
		MG_CHECK(wheel_grip_status, (wheel_grip_status = mutexgear_wheel_gripon(&__worker_instance->progress_wheel)) == EOK); // No way to handle -- must succeed
		enter_loop = true;
	}

	// Continue turning the wheel until the worker marks the item as complete if the wheel had been gripped on to
	for (continue_loop = enter_loop && _mutexgear_completion_item_getwow(__item_to_be_waited) != (void *)__item_to_be_waited/* != NULL*/;
		continue_loop; continue_loop = _mutexgear_completion_item_getwow(__item_to_be_waited) != (void *)__item_to_be_waited/* != NULL*/)
	{
		MG_CHECK(wheel_turn_status, (wheel_turn_status = mutexgear_wheel_turn(&__worker_instance->progress_wheel)) == EOK); // No way to handle -- must succeed
	}

	// Release the wheel if it was gripped on to
	if (enter_loop)
	{
		MG_CHECK(wheel_release_status, (wheel_release_status = mutexgear_wheel_release(&__worker_instance->progress_wheel)) == EOK); // No way to handle -- must succeed
	}

	// Release own detach lock to let the worker know the thread has finished accessing the worker instance
	MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__waiter_instance->wait_detach_lock)) == EOK); // No way to handle -- must succeed

	// Acquire and immediately release the worker detach lock to make sure the worker has received this thread's signal and has finished accessing the wait_info pointer it had.
	MG_CHECK(mutex_lock_status, (mutex_lock_status = _mutexgear_lock_acquire(&__queue_instance->worker_detach_lock)) == EOK); // No way to handle -- must succeed
	MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__queue_instance->worker_detach_lock)) == EOK); // No way to handle -- must succeed
}


//////////////////////////////////////////////////////////////////////////
// Completion DrainableQueue Implementation

/*extern */
int _mutexgear_completion_drain_init(mutexgear_completion_drain_t *__drain_instance)
{
	mutexgear_dlralist_init(&__drain_instance->drain_list);

	return EOK;
}

/*extern */
int _mutexgear_completion_drain_destroy(mutexgear_completion_drain_t *__drain_instance)
{
	int ret;

	do
	{
		if ((ret = _mutexgear_completion_drain_preparedestroy(__drain_instance)) != EOK)
		{
			break;
		}

		_mutexgear_completion_drain_completedestroy(__drain_instance);

		ret = EOK;
	} while (false);

	return ret;
}


/*extern */
int _mutexgear_completion_drain_preparedestroy(mutexgear_completion_drain_t *__drain_instance)
{
	int ret = mutexgear_dlralist_isempty(&__drain_instance->drain_list) ? EOK : EBUSY;
	return ret;
}

/*extern */
void _mutexgear_completion_drain_completedestroy(mutexgear_completion_drain_t *__drain_instance)
{
	int list_emptiness_check;

	MG_CHECK(list_emptiness_check, mutexgear_dlralist_isempty(&__drain_instance->drain_list) || (list_emptiness_check = EBUSY, false));

	mutexgear_dlralist_destroy(&__drain_instance->drain_list);
}

/*extern */
void _mutexgear_completion_drain_unpreparedestroy(mutexgear_completion_drain_t *__drain_instance)
{
	// Do nothing
}


/*extern */
int _mutexgear_completion_drainablequeue_init(mutexgear_completion_drainablequeue_t *__queue_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/)
{
	int ret;

	do
	{
		if ((ret = _mutexgear_completion_queue_init(&__queue_instance->basic_queue, __attr)) != EOK)
		{
			break;
		}

		__queue_instance->drain_index = _mutexgear_completion_drainidx_getmin();

		MG_ASSERT(ret == EOK);
	}
	while (false);

	return ret;
}

/*extern */
int _mutexgear_completion_drainablequeue_destroy(mutexgear_completion_drainablequeue_t *__queue_instance)
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


/*extern */
int _mutexgear_completion_drainablequeue_preparedestroy(mutexgear_completion_drainablequeue_t *__queue_instance)
{
	int ret;

	do
	{
		if ((ret = _mutexgear_completion_queue_preparedestroy(&__queue_instance->basic_queue)) != EOK)
		{
			break;
		}

		MG_ASSERT(ret == EOK);
	}
	while (false);

	return ret;
}

/*extern */
void _mutexgear_completion_drainablequeue_completedestroy(mutexgear_completion_drainablequeue_t *__queue_instance)
{
	_mutexgear_completion_queue_completedestroy(&__queue_instance->basic_queue);
}

/*extern */
void _mutexgear_completion_drainablequeue_unpreparedestroy(mutexgear_completion_drainablequeue_t *__queue_instance)
{
	_mutexgear_completion_queue_unpreparedestroy(&__queue_instance->basic_queue);
}


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
int _mutexgear_completion_cancelablequeue_lock(mutexgear_completion_locktoken_t *__out_lock_acquired/*=NULL*/,
	mutexgear_completion_cancelablequeue_t *__queue_instance)
{
	int ret = _mutexgear_completion_queue_lock(__out_lock_acquired, &__queue_instance->basic_queue);
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
				_mutexgear_completion_item_settag(__item_to_be_canceled, mutexgear_completion_cancelablequeue_itemtag_cancelrequested, true);

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
	int ret;

	do
	{
		if ((ret = _mutexgear_completion_queue_enqueue(&__queue_instance->basic_queue, __item_instance, __lock_hint)) != EOK)
		{
			break;
		}

		MG_ASSERT(ret == EOK);
	}
	while (false);

	return ret;
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_cancelablequeue_unsafedequeue(mutexgear_completion_item_t *__item_instance)
{
	_mutexgear_completion_queue_unsafedequeue(__item_instance);
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_cancelablequeueditem_start(mutexgear_completion_item_t **__out_acquired_item,
	mutexgear_completion_cancelablequeue_t *__queue_instance, mutexgear_completion_worker_t *__worker_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
{
	MG_ASSERT(__worker_instance != NULL);

	bool success = false;
	mutexgear_completion_item_t *acquired_item = NULL;
	int ret, mutex_unlock_status;

	// bool mutex_locked = false;

	do
	{
		if (__lock_hint == NULL && (ret = _mutexgear_completion_queue_lock(NULL, &__queue_instance->basic_queue)) != EOK)
		{
			break;
		}
		// mutex_locked = true; -- no breaks after this point at this time

		mutexgear_completion_item_t *current_item;
		bool continue_loop = _mutexgear_completion_queue_unsafegethead(&current_item, &__queue_instance->basic_queue);
		for (; continue_loop; continue_loop = _mutexgear_completion_queue_unsafegetnext(&current_item, &__queue_instance->basic_queue, current_item))
		{
			if (_mutexgear_completion_item_getwow(current_item) == (void *)current_item)
			{
				_mutexgear_completion_item_setwow(current_item, __worker_instance);
				acquired_item = current_item;
				break;
			}
		}

		if (__lock_hint == NULL)
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__queue_instance->basic_queue)) == EOK); // Should succeed normally
		}
		// mutex_locked = false;

		*__out_acquired_item = acquired_item != NULL ? acquired_item : NULL;
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
		// 		MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__queue_instance->basic_queue.access_lock)) == EOK); // Should succeed normally
		// 	}
		// }
	}

	return ret;
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_cancelablequeueditem_iscanceled(mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
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
		MG_ASSERT(current_worker != NULL);

		// Finally, check if the cancel flag is set
		ret = _mutexgear_completion_item_gettag(__item_instance, mutexgear_completion_cancelablequeue_itemtag_cancelrequested);
	}

	return ret;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_cancelablequeueditem_finish(mutexgear_completion_cancelablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
{
	int ret;

	do
	{
		if ((ret = _mutexgear_completion_queueditem_finish(&__queue_instance->basic_queue, __item_instance, __worker_instance, __lock_hint)) != EOK)
		{
			break;
		}

		// Clear the cancel request tag that might have been set for the item
		_mutexgear_completion_item_settag(__item_instance, mutexgear_completion_cancelablequeue_itemtag_cancelrequested, false);

		MG_ASSERT(ret == EOK);
	}
	while (false);

	return ret;
}


//////////////////////////////////////////////////////////////////////////
// Completion Attribute Public APIs Implementation

/*extern */
int mutexgear_completion_genattr_init(mutexgear_completion_genattr_t *__attr)
{
	return _mutexgear_completion_genattr_init(__attr);
}

/*extern */
int mutexgear_completion_genattr_destroy(mutexgear_completion_genattr_t *__attr)
{
	return _mutexgear_completion_genattr_destroy(__attr);
}


/*extern */
int mutexgear_completion_genattr_getpshared(const mutexgear_completion_genattr_t *__attr, int *__pshared)
{
	return _mutexgear_completion_genattr_getpshared(__attr, __pshared);
}

/*extern */
int mutexgear_completion_genattr_setpshared(mutexgear_completion_genattr_t *__attr, int __pshared)
{
	return _mutexgear_completion_genattr_setpshared(__attr, __pshared);
}


/*extern */
int mutexgear_completion_genattr_getprioceiling(const mutexgear_completion_genattr_t *__attr, int *__prioceiling)
{
	return _mutexgear_completion_genattr_getprioceiling(__attr, __prioceiling);
}

/*extern */
int mutexgear_completion_genattr_getprotocol(const mutexgear_completion_genattr_t *__attr, int *__protocol)
{
	return _mutexgear_completion_genattr_getprotocol(__attr, __protocol);
}

/*extern */
int mutexgear_completion_genattr_setprioceiling(mutexgear_completion_genattr_t *__attr, int __prioceiling)
{
	return _mutexgear_completion_genattr_setprioceiling(__attr, __prioceiling);
}

/*extern */
int mutexgear_completion_genattr_setprotocol(mutexgear_completion_genattr_t *__attr, int __protocol)
{
	return _mutexgear_completion_genattr_setprotocol(__attr, __protocol);
}

/*extern */
int mutexgear_completion_genattr_setmutexattr(mutexgear_completion_genattr_t *__attr, const _MUTEXGEAR_LOCKATTR_T *__mutexattr)
{
	return _mutexgear_completion_genattr_setmutexattr(__attr, __mutexattr);
}


//////////////////////////////////////////////////////////////////////////
// Completion Queue Public APIs Implementation

/*extern */
int mutexgear_completion_worker_init(mutexgear_completion_worker_t *__worker_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/)
{
	return _mutexgear_completion_worker_init(__worker_instance, __attr);
}

/*extern */
int mutexgear_completion_worker_destroy(mutexgear_completion_worker_t *__worker_instance)
{
	return _mutexgear_completion_worker_destroy(__worker_instance);
}

/*extern */
int mutexgear_completion_worker_lock(mutexgear_completion_worker_t *__worker_instance)
{
	return _mutexgear_completion_worker_lock(__worker_instance);
}

/*extern */
int mutexgear_completion_worker_unlock(mutexgear_completion_worker_t *__worker_instance)
{
	return _mutexgear_completion_worker_unlock(__worker_instance);
}


/*extern */
int mutexgear_completion_waiter_init(mutexgear_completion_waiter_t *__waiter_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/)
{
	return _mutexgear_completion_waiter_init(__waiter_instance, __attr);
}

/*extern */
int mutexgear_completion_waiter_destroy(mutexgear_completion_waiter_t *__waiter_instance)
{
	return _mutexgear_completion_waiter_destroy(__waiter_instance);
}


/*extern */
int mutexgear_completion_queue_init(mutexgear_completion_queue_t *__queue_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/)
{
	return _mutexgear_completion_queue_init(__queue_instance, __attr);
}

/*extern */
int mutexgear_completion_queue_destroy(mutexgear_completion_queue_t *__queue_instance)
{
	return _mutexgear_completion_queue_destroy(__queue_instance);
}


/*extern */
int mutexgear_completion_queue_lock(mutexgear_completion_locktoken_t *__out_lock_acquired/*=NULL*/, mutexgear_completion_queue_t *__queue_instance)
{
	return _mutexgear_completion_queue_lock(__out_lock_acquired, __queue_instance);
}

/*extern */
int mutexgear_completion_queue_plainunlock(mutexgear_completion_queue_t *__queue_instance)
{
	return _mutexgear_completion_queue_plainunlock(__queue_instance);
}

/*extern */
int mutexgear_completion_queue_unlockandwait(mutexgear_completion_queue_t *__queue_instance,
	mutexgear_completion_item_t *__item_to_be_waited, mutexgear_completion_waiter_t *__waiter_instance)
{
	return _mutexgear_completion_queue_unlockandwait(__queue_instance, __item_to_be_waited, __waiter_instance);
}


/*extern */
int mutexgear_completion_queue_enqueue(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
{
	return _mutexgear_completion_queue_enqueue(__queue_instance, __item_instance, __lock_hint);
}

/*extern */
void mutexgear_completion_queue_unsafedequeue(mutexgear_completion_item_t *__item_instance)
{
	_mutexgear_completion_queue_unsafedequeue(__item_instance);
}


/*extern */
int mutexgear_completion_queueditem_finish(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
{
	return _mutexgear_completion_queueditem_finish(__queue_instance, __item_instance, __worker_instance, __lock_hint);
}


//////////////////////////////////////////////////////////////////////////
// Completion DrainableQueue Public APIs Implementation

/*extern */
int mutexgear_completion_drain_init(mutexgear_completion_drain_t *__drain_instance)
{
	return _mutexgear_completion_drain_init(__drain_instance);
}

/*extern */
int mutexgear_completion_drain_destroy(mutexgear_completion_drain_t *__drain_instance)
{
	return _mutexgear_completion_drain_destroy(__drain_instance);
}


/*extern */
int mutexgear_completion_drainablequeue_init(mutexgear_completion_drainablequeue_t *__queue_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/)
{
	return _mutexgear_completion_drainablequeue_init(__queue_instance, __attr);
}

/*extern */
int mutexgear_completion_drainablequeue_destroy(mutexgear_completion_drainablequeue_t *__queue_instance)
{
	return _mutexgear_completion_drainablequeue_destroy(__queue_instance);
}


/*extern */
int mutexgear_completion_drainablequeue_lock(mutexgear_completion_locktoken_t *__out_lock_acquired/*=NULL*/,
	mutexgear_completion_drainablequeue_t *__queue_instance)
{
	return _mutexgear_completion_drainablequeue_lock(__out_lock_acquired, __queue_instance);
}

/*extern */
int mutexgear_completion_drainablequeue_getindex(mutexgear_completion_drainidx_t *__out_queue_drain_index,
	mutexgear_completion_drainablequeue_t *__queue_instance, mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
{
	return _mutexgear_completion_drainablequeue_getindex(__out_queue_drain_index, __queue_instance, __lock_hint);
}

/*extern */
int mutexgear_completion_drainablequeue_drain(mutexgear_completion_drainablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__drain_head_item, mutexgear_completion_drainidx_t __item_drain_index,
	mutexgear_completion_drain_t *__target_drain, mutexgear_completion_locktoken_t __lock_hint/*=NULL*/, bool *__out_drain_execution_status/*=NULL*/)
{
	return _mutexgear_completion_drainablequeue_drain(__queue_instance, __drain_head_item, __item_drain_index, __target_drain, __lock_hint, __out_drain_execution_status);
}

/*extern */
int mutexgear_completion_drainablequeue_plainunlock(mutexgear_completion_drainablequeue_t *__queue_instance)
{
	return _mutexgear_completion_drainablequeue_plainunlock(__queue_instance);
}

/*extern */
int mutexgear_completion_drainablequeue_unlockandwait(mutexgear_completion_drainablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_to_be_waited, mutexgear_completion_waiter_t *__waiter_instance)
{
	return _mutexgear_completion_drainablequeue_unlockandwait(__queue_instance, __item_to_be_waited, __waiter_instance);
}


/*extern */
int mutexgear_completion_drainablequeue_enqueue(mutexgear_completion_drainablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/, mutexgear_completion_drainidx_t *__out_queue_drain_index/*=NULL*/)
{
	return _mutexgear_completion_drainablequeue_enqueue(__queue_instance, __item_instance, __lock_hint, __out_queue_drain_index);
}

/*extern */
void mutexgear_completion_drainablequeue_unsafedequeue(mutexgear_completion_item_t *__item_instance)
{
	_mutexgear_completion_drainablequeue_unsafedequeue(__item_instance);
}


/*extern */
int mutexgear_completion_drainablequeueditem_finish(mutexgear_completion_drainablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance,
	mutexgear_completion_drainidx_t __item_drain_index/*=MUTEXGEAR_COMPLETION_INVALID_DRAINIDX*/, mutexgear_completion_drain_t *__target_drain/*=NULL*/,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
{
	return _mutexgear_completion_drainablequeueditem_finish(__queue_instance, __item_instance, __worker_instance, __item_drain_index, __target_drain, __lock_hint);
}


//////////////////////////////////////////////////////////////////////////
// Completion CancelableQueue Public APIs Implementation

/*extern */
int mutexgear_completion_cancelablequeue_init(mutexgear_completion_cancelablequeue_t *__queue_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/)
{
	return _mutexgear_completion_cancelablequeue_init(__queue_instance, __attr);
}

/*extern */
int mutexgear_completion_cancelablequeue_destroy(mutexgear_completion_cancelablequeue_t *__queue_instance)
{
	return _mutexgear_completion_cancelablequeue_destroy(__queue_instance);
}


/*extern */
int mutexgear_completion_cancelablequeue_lock(mutexgear_completion_locktoken_t *__out_lock_acquired/*=NULL*/, mutexgear_completion_cancelablequeue_t *__queue_instance)
{
	return _mutexgear_completion_cancelablequeue_lock(__out_lock_acquired, __queue_instance);
}

/*extern */
int mutexgear_completion_cancelablequeue_plainunlock(mutexgear_completion_cancelablequeue_t *__queue_instance)
{
	return _mutexgear_completion_cancelablequeue_plainunlock(__queue_instance);
}

/*extern */
int mutexgear_completion_cancelablequeue_unlockandwait(mutexgear_completion_cancelablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_to_be_waited, mutexgear_completion_waiter_t *__waiter_instance)
{
	return _mutexgear_completion_cancelablequeue_unlockandwait(__queue_instance, __item_to_be_waited, __waiter_instance);
}

/*extern */
int mutexgear_completion_cancelablequeue_unlockandcancel(mutexgear_completion_cancelablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_to_be_canceled, mutexgear_completion_waiter_t *__waiter_instance,
	void(*__item_cancel_fn/*=NULL*/)(void *__cancel_context, mutexgear_completion_cancelablequeue_t *__queue, mutexgear_completion_worker_t *__worker, mutexgear_completion_item_t *__item), void *__cancel_context/*=NULL*/,
	mutexgear_completion_ownership_t *__out_item_resulting_ownership)
{
	return _mutexgear_completion_cancelablequeue_unlockandcancel(__queue_instance, __item_to_be_canceled, __waiter_instance, __item_cancel_fn, __cancel_context, __out_item_resulting_ownership);
}


/*extern */
int mutexgear_completion_cancelablequeue_enqueue(mutexgear_completion_cancelablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
{
	return _mutexgear_completion_cancelablequeue_enqueue(__queue_instance, __item_instance, __lock_hint);
}

/*extern */
void mutexgear_completion_cancelablequeue_unsafedequeue(mutexgear_completion_item_t *__item_instance)
{
	_mutexgear_completion_cancelablequeue_unsafedequeue(__item_instance);
}


/*extern */
int mutexgear_completion_cancelablequeueditem_start(mutexgear_completion_item_t **__out_acquired_item,
	mutexgear_completion_cancelablequeue_t *__queue_instance, mutexgear_completion_worker_t *__worker_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
{
	return _mutexgear_completion_cancelablequeueditem_start(__out_acquired_item, __queue_instance, __worker_instance, __lock_hint);
}

bool mutexgear_completion_cancelablequeueditem_iscanceled(mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
{
	return _mutexgear_completion_cancelablequeueditem_iscanceled(__item_instance, __worker_instance);
}

/*extern */
int mutexgear_completion_cancelablequeueditem_finish(mutexgear_completion_cancelablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
{
	return _mutexgear_completion_cancelablequeueditem_finish(__queue_instance, __item_instance, __worker_instance, __lock_hint);
}

