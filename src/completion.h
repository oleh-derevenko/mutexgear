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
#include "wheel.h"
#include "dlralist.h"
#include "utility.h"


//////////////////////////////////////////////////////////////////////////
// Completion Generic Attributes function definitions

_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_genattr_init(mutexgear_completion_genattr_t *__attr);
_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_genattr_destroy(mutexgear_completion_genattr_t *__attr);
_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_genattr_getpshared(const mutexgear_completion_genattr_t *__attr, int *__pshared);
_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_genattr_setpshared(mutexgear_completion_genattr_t *__attr, int __pshared);
_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_genattr_getprioceiling(const mutexgear_completion_genattr_t *__attr, int *__prioceiling);
_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_genattr_getprotocol(const mutexgear_completion_genattr_t *__attr, int *__protocol);
_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_genattr_setprioceiling(mutexgear_completion_genattr_t *__attr, int __prioceiling);
_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_genattr_setprotocol(mutexgear_completion_genattr_t *__attr, int __protocol);
_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_genattr_setmutexattr(mutexgear_completion_genattr_t *__attr, const _MUTEXGEAR_LOCKATTR_T *__mutexattr);


//////////////////////////////////////////////////////////////////////////
// Completion Generic Attributes function implementations

 _MUTEXGEAR_PURE_INLINE 
int _mutexgear_completion_genattr_init(mutexgear_completion_genattr_t *__attr)
{
	int ret = _mutexgear_lockattr_init(&__attr->lock_attr);
	return ret;
}

_MUTEXGEAR_PURE_INLINE 
int _mutexgear_completion_genattr_destroy(mutexgear_completion_genattr_t *__attr)
{
	int ret = _mutexgear_lockattr_destroy(&__attr->lock_attr);
	return ret;
}


_MUTEXGEAR_PURE_INLINE 
int _mutexgear_completion_genattr_getpshared(const mutexgear_completion_genattr_t *__attr, int *__out_pshared)
{
	int ret = _mutexgear_lockattr_getpshared(&__attr->lock_attr, __out_pshared);
	return ret;
}

_MUTEXGEAR_PURE_INLINE 
int _mutexgear_completion_genattr_setpshared(mutexgear_completion_genattr_t *__attr, int __pshared)
{
	int ret = _mutexgear_lockattr_setpshared(&__attr->lock_attr, __pshared);
	return ret;
}


_MUTEXGEAR_PURE_INLINE 
int _mutexgear_completion_genattr_getprioceiling(const mutexgear_completion_genattr_t *__attr, int *__out_prioceiling)
{
	int ret = _mutexgear_lockattr_getprioceiling(&__attr->lock_attr, __out_prioceiling);
	return ret;
}

_MUTEXGEAR_PURE_INLINE 
int _mutexgear_completion_genattr_getprotocol(const mutexgear_completion_genattr_t *__attr, int *__out_protocol)
{
	int ret = _mutexgear_lockattr_getprotocol(&__attr->lock_attr, __out_protocol);
	return ret;
}

_MUTEXGEAR_PURE_INLINE 
int _mutexgear_completion_genattr_setprioceiling(mutexgear_completion_genattr_t *__attr, int __prioceiling)
{
	int ret = _mutexgear_lockattr_setprioceiling(&__attr->lock_attr, __prioceiling);
	return ret;
}

_MUTEXGEAR_PURE_INLINE 
int _mutexgear_completion_genattr_setprotocol(mutexgear_completion_genattr_t *__attr, int __protocol)
{
	int ret = _mutexgear_lockattr_setprotocol(&__attr->lock_attr, __protocol);
	return ret;
}


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

_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_worker_init(mutexgear_completion_worker_t *__worker_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/);
_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_worker_destroy(mutexgear_completion_worker_t *__worker_instance);

_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_worker_lock(mutexgear_completion_worker_t *__worker_instance);
_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_worker_unlock(mutexgear_completion_worker_t *__worker_instance);

_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_waiter_init(mutexgear_completion_waiter_t *__waiter_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/);
_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_waiter_destroy(mutexgear_completion_waiter_t *__waiter_instance);


_MUTEXGEAR_PURE_INLINE
mutexgear_dlraitem_t *_mutexgear_completion_item_getworkitem(const mutexgear_completion_item_t *__item_instance)
{
	return mutexgear_completion_item_getworkitem(__item_instance);
}


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
bool _mutexgear_completion_item_isstarted(const mutexgear_completion_item_t *__item_instance)
{
	return mutexgear_completion_item_isstarted(__item_instance);
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_item_settag(mutexgear_completion_item_t *__item_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value)
{
	mutexgear_completion_item_settag(__item_instance, __tag_index, __tag_value);
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_item_setunsafetag(mutexgear_completion_item_t *__item_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value)
{
	mutexgear_completion_item_setunsafetag(__item_instance, __tag_index, __tag_value);
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_item_setallunsafetags(mutexgear_completion_item_t *__item_instance, bool __tag_value)
{
	mutexgear_completion_item_setallunsafetags(__item_instance, __tag_value);
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_item_unsafemodifytag(mutexgear_completion_item_t *__item_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value)
{
	return mutexgear_completion_item_unsafemodifytag(__item_instance, __tag_index, __tag_value);
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_item_modifyunsafetag(mutexgear_completion_item_t *__item_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value)
{
	return mutexgear_completion_item_modifyunsafetag(__item_instance, __tag_index, __tag_value);
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_item_gettag(const mutexgear_completion_item_t *__item_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/)
{
	return mutexgear_completion_item_gettag(__item_instance, __tag_index);
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_item_getanytags(const mutexgear_completion_item_t *__item_instance)
{
	return mutexgear_completion_item_getanytags(__item_instance);
}


_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_queue_init(mutexgear_completion_queue_t *__queue_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/);
_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_queue_destroy(mutexgear_completion_queue_t *__queue_instance);

_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_queue_preparedestroy(mutexgear_completion_queue_t *__queue_instance);
_MUTEXGEAR_PURE_INLINE void _mutexgear_completion_queue_completedestroy(mutexgear_completion_queue_t *__queue_instance);
_MUTEXGEAR_PURE_INLINE void _mutexgear_completion_queue_unpreparedestroy(mutexgear_completion_queue_t *__queue_instance);


_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_queue_lock(mutexgear_completion_locktoken_t *__out_acquired_lock/*=NULL*/, mutexgear_completion_queue_t *__queue_instance)
{
	int ret = _mutexgear_lock_acquire(&__queue_instance->access_lock);
	return ret == EOK && (__out_acquired_lock == NULL || (*__out_acquired_lock = _mutexgear_completion_queue_derivetoken(__queue_instance), true)) ? EOK : ret;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_queue_trylock(mutexgear_completion_locktoken_t *__out_acquired_lock/*=NULL*/, mutexgear_completion_queue_t *__queue_instance)
{
	int ret = _mutexgear_lock_tryacquire(&__queue_instance->access_lock);
	return ret == EOK && (__out_acquired_lock == NULL || (*__out_acquired_lock = _mutexgear_completion_queue_derivetoken(__queue_instance), true)) ? EOK : ret;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_queue_plainunlock(mutexgear_completion_queue_t *__queue_instance)
{
	int ret = _mutexgear_lock_release(&__queue_instance->access_lock);
	return ret;
}

static int _mutexgear_completion_queue_unlockandwait(mutexgear_completion_queue_t *__queue_instance,
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
mutexgear_completion_item_t *_mutexgear_completion_queue_getrend(mutexgear_completion_queue_t *__queue_instance)
{
	return mutexgear_completion_queue_getrend(__queue_instance);
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_queue_unsafegethead(mutexgear_completion_item_t **__out_head_item,
	mutexgear_completion_queue_t *__queue_instance)
{
	return mutexgear_completion_queue_unsafegethead(__out_head_item, __queue_instance);
}

_MUTEXGEAR_PURE_INLINE
mutexgear_completion_item_t *_mutexgear_completion_queue_unsafegetunsafehead(
	mutexgear_completion_queue_t *__queue_instance)
{
	return mutexgear_completion_queue_unsafegetunsafehead(__queue_instance);
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_queue_unsafegetnext(mutexgear_completion_item_t **__out_next_item,
	mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance)
{
	return mutexgear_completion_queue_unsafegetnext(__out_next_item, __queue_instance, __item_instance);
}

_MUTEXGEAR_PURE_INLINE 
mutexgear_completion_item_t *_mutexgear_completion_queue_unsafegetunsafenext(
	const mutexgear_completion_item_t *__item_instance)
{
	return mutexgear_completion_queue_unsafegetunsafenext(__item_instance);
}

_MUTEXGEAR_PURE_INLINE
mutexgear_completion_item_t *_mutexgear_completion_queue_getend(mutexgear_completion_queue_t *__queue_instance)
{
	return mutexgear_completion_queue_getend(__queue_instance);
}


_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_queue_enqueue_before(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__before_item, mutexgear_completion_item_t *__item_instance,
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

		mutexgear_dlralist_linkat(&__queue_instance->work_list, &__item_instance->work_item, &__before_item->work_item);
		
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
int _mutexgear_completion_queue_enqueue_back(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
{
	mutexgear_completion_item_t *end_item = _mutexgear_completion_queue_getend(__queue_instance);
	return _mutexgear_completion_queue_enqueue_before(__queue_instance, end_item, __item_instance, __lock_hint);
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_queue_enqueue(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/)
{
	return _mutexgear_completion_queue_enqueue_back(__queue_instance, __item_instance, __lock_hint);
}


_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_queue_unsafeenqueue_before(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__before_item, 
	mutexgear_completion_item_t *__item_instance)
{
	mutexgear_dlralist_linkat(&__queue_instance->work_list, &__item_instance->work_item, &__before_item->work_item);
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_queue_unsafemultiqueue_before(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__before_item, 
	mutexgear_completion_item_t *__first_item, mutexgear_completion_item_t *__last_item)
{
	_mutexgear_dlralist_multilinkat(&__queue_instance->work_list, &__first_item->work_item, &__last_item->work_item, &__before_item->work_item);
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_queue_unsafeenqueue_back(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance)
{
	mutexgear_completion_item_t *end_item = _mutexgear_completion_queue_getend(__queue_instance);
	_mutexgear_completion_queue_unsafeenqueue_before(__queue_instance, end_item, __item_instance);
}


_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_queue_unsafespliceat(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__before_item, mutexgear_completion_item_t *__item_instance)
{
	mutexgear_dlraitem_t *next_item = mutexgear_dlraitem_getnext(&__item_instance->work_item);
	mutexgear_dlralist_spliceat(&__queue_instance->work_list, &__before_item->work_item, &__item_instance->work_item, next_item);
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_queue_unsafedequeue(mutexgear_completion_item_t *__item_instance)
{
	mutexgear_dlralist_unlink(&__item_instance->work_item);
}

static void _mutexgear_completion_queueditem_commcompletiontowaiter(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance,
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_waiter_t *__item_waiter);


_MUTEXGEAR_PURE_INLINE 
void _mutexgear_completion_queueditem_start(mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
{
	mutexgear_completion_queueditem_start(__item_instance, __worker_instance);
}

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

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_queueditem_unsafefinish__locked(mutexgear_completion_item_t *__item_instance)
{
	mutexgear_dlralist_unlink(&__item_instance->work_item);
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_queueditem_unsafefinish__unlocked(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
{
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
}


//////////////////////////////////////////////////////////////////////////
// Completion DrainableQueue APIs

_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_drain_init(mutexgear_completion_drain_t *__drain_instance);
_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_drain_destroy(mutexgear_completion_drain_t *__drain_instance);

_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_drain_preparedestroy(mutexgear_completion_drain_t *__drain_instance);
_MUTEXGEAR_PURE_INLINE void _mutexgear_completion_drain_completedestroy(mutexgear_completion_drain_t *__drain_instance);
_MUTEXGEAR_PURE_INLINE void _mutexgear_completion_drain_unpreparedestroy(mutexgear_completion_drain_t *__drain_instance);

_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_drainablequeue_init(mutexgear_completion_drainablequeue_t *__queue_instance, const mutexgear_completion_genattr_t *__attr/*=NULL*/);
_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_drainablequeue_destroy(mutexgear_completion_drainablequeue_t *__queue_instance);

_MUTEXGEAR_PURE_INLINE int _mutexgear_completion_drainablequeue_preparedestroy(mutexgear_completion_drainablequeue_t *__queue_instance);
_MUTEXGEAR_PURE_INLINE void _mutexgear_completion_drainablequeue_completedestroy(mutexgear_completion_drainablequeue_t *__queue_instance);
_MUTEXGEAR_PURE_INLINE void _mutexgear_completion_drainablequeue_unpreparedestroy(mutexgear_completion_drainablequeue_t *__queue_instance);


_MUTEXGEAR_PURE_INLINE
int _mutexgear_completion_drainablequeue_lock(mutexgear_completion_locktoken_t *__out_acquired_lock/*=NULL*/,
	mutexgear_completion_drainablequeue_t *__queue_instance)
{
	int ret = _mutexgear_completion_queue_lock(__out_acquired_lock, &__queue_instance->basic_queue);
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
void _mutexgear_completion_drainablequeue_unsafegetindex(mutexgear_completion_drainidx_t *__out_queue_drain_index,
	mutexgear_completion_drainablequeue_t *__queue_instance)
{
	MG_ASSERT(__out_queue_drain_index != NULL);

	mutexgear_completion_drainidx_t queue_drain_index = __queue_instance->drain_index;

	*__out_queue_drain_index = queue_drain_index;
	MG_ASSERT(queue_drain_index != MUTEXGEAR_COMPLETION_INVALID_DRAINIDX);
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
		mutexgear_dlraitem_t *p_item_work = _mutexgear_completion_item_getworkitem(__drain_head_item);
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
void _mutexgear_completion_drainablequeue_unsafeenqueue(mutexgear_completion_drainablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance,
	mutexgear_completion_drainidx_t *__out_queue_drain_index/*=NULL*/)
{
	if (__out_queue_drain_index != NULL)
	{
		_mutexgear_completion_drainablequeue_unsafegetindex(__out_queue_drain_index, __queue_instance);
	}

	_mutexgear_completion_queue_unsafeenqueue_back(&__queue_instance->basic_queue, __item_instance);
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

		_mutexgear_completion_queueditem_unsafefinish__locked(__item_instance);

		if (__lock_hint == NULL)
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_completion_queue_plainunlock(&__queue_instance->basic_queue)) == EOK); // Should succeed normally
		}
		// mutex_locked = false;

		_mutexgear_completion_queueditem_unsafefinish__unlocked(&__queue_instance->basic_queue, __item_instance, __worker_instance);

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

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_cancelablequeueditem_start(mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
{
	mutexgear_completion_cancelablequeueditem_start(__item_instance, __worker_instance);
}


//////////////////////////////////////////////////////////////////////////
// Completion Attributes Implementation

/*_MUTEXGEAR_PURE_INLINE */
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

/*_MUTEXGEAR_PURE_INLINE */
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
			if ((ret = _mutexgear_wheelattr_init(&wheelattr)) != EOK)
			{
				break;
			}
		}
		wheelattr_was_allocated = true;

		if (__attr != NULL)
		{
			if ((ret = _mutexgear_wheelattr_setmutexattr(&wheelattr, &__attr->lock_attr)) != EOK)
			{
				break;
			}
		}

		if ((ret = _mutexgear_wheel_init(&__worker_instance->progress_wheel, __attr != NULL ? &wheelattr : NULL)) != EOK)
		{
			break;
		}

		if (__attr != NULL)
		{
			MG_CHECK(wheelattr_destroy_status, (wheelattr_destroy_status = _mutexgear_wheelattr_destroy(&wheelattr)) == EOK); // This should succeed normally
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
				MG_CHECK(wheelattr_destroy_status, (wheelattr_destroy_status = _mutexgear_wheelattr_destroy(&wheelattr)) == EOK); // This should succeed normally
			}
		}
	}

	return ret;
}

/*_MUTEXGEAR_PURE_INLINE */
int _mutexgear_completion_worker_destroy(mutexgear_completion_worker_t *__worker_instance)
{
	int ret;

	do
	{
		if ((ret = _mutexgear_wheel_destroy(&__worker_instance->progress_wheel)) != EOK)
		{
			break;
		}

		MG_ASSERT(ret == EOK);
	}
	while (false);

	return ret;
}


/*_MUTEXGEAR_PURE_INLINE */
int _mutexgear_completion_worker_lock(mutexgear_completion_worker_t *__worker_instance)
{
	int ret = _mutexgear_wheel_lockslave(&__worker_instance->progress_wheel);
	return ret;
}

/*_MUTEXGEAR_PURE_INLINE */
int _mutexgear_completion_worker_unlock(mutexgear_completion_worker_t *__worker_instance)
{
	int ret = _mutexgear_wheel_unlockslave(&__worker_instance->progress_wheel);
	return ret;
}


/*_MUTEXGEAR_PURE_INLINE */
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

/*_MUTEXGEAR_PURE_INLINE */
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


/*_MUTEXGEAR_PURE_INLINE */
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

/*_MUTEXGEAR_PURE_INLINE */
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


/*_MUTEXGEAR_PURE_INLINE */
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

/*_MUTEXGEAR_PURE_INLINE */
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

/*_MUTEXGEAR_PURE_INLINE */
void _mutexgear_completion_queue_unpreparedestroy(mutexgear_completion_queue_t *__queue_instance)
{
	int mutex_unlock_status;

	MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__queue_instance->worker_detach_lock)) == EOK); // This should succeed normally
	MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__queue_instance->access_lock)) == EOK); // This should succeed normally
}


/*static */
void _mutexgear_completion_queueditem_commcompletiontowaiter(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance,
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_waiter_t *__item_waiter)
{
	int mutex_lock_status, mutex_unlock_status, wheel_roll_status;

	// Acquire the worker detach lock to allow the waiter to know when this thread finishes its access to the item_waiter
	MG_CHECK(mutex_lock_status, (mutex_lock_status = _mutexgear_lock_acquire(&__queue_instance->worker_detach_lock)) == EOK); // No way to handle -- must succeed

	// Assign NULL pointer to the shared variable to let the waiter know the work is completed
	_mutexgear_completion_item_setwow(__item_instance, __item_instance); // = NULL
	// Roll the worker wheel to notify the waiter about progress
	MG_CHECK(wheel_roll_status, (wheel_roll_status = _mutexgear_wheel_slaveroll(&__worker_instance->progress_wheel)) == EOK); // No way to handle -- must succeed

	// Acquire and immediately release the wait detach lock to know the waiter finished accessing the worker pointer it had
	MG_CHECK(mutex_lock_status, (mutex_lock_status = _mutexgear_lock_acquire(&__item_waiter->wait_detach_lock)) == EOK); // No way to handle -- must succeed
	MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__item_waiter->wait_detach_lock)) == EOK); // No way to handle -- must succeed

	// Release the worker to signal the waiter this thread has finished its operations on the item_waiter
	MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__queue_instance->worker_detach_lock)) == EOK); // No way to handle -- must succeed
}


static void _mutexgear_completion_wait_item_completion_and_detach(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_to_be_waited,
	mutexgear_completion_waiter_t *__waiter_instance, mutexgear_completion_worker_t *__worker_instance);

/*static */
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
		MG_CHECK(wheel_grip_status, (wheel_grip_status = _mutexgear_wheel_gripon(&__worker_instance->progress_wheel)) == EOK); // No way to handle -- must succeed
		enter_loop = true;
	}

	// Continue turning the wheel until the worker marks the item as complete if the wheel had been gripped on to
	for (continue_loop = enter_loop && _mutexgear_completion_item_getwow(__item_to_be_waited) != (void *)__item_to_be_waited/* != NULL*/;
		continue_loop; continue_loop = _mutexgear_completion_item_getwow(__item_to_be_waited) != (void *)__item_to_be_waited/* != NULL*/)
	{
		MG_CHECK(wheel_turn_status, (wheel_turn_status = _mutexgear_wheel_turn(&__worker_instance->progress_wheel)) == EOK); // No way to handle -- must succeed
	}

	// Release the wheel if it was gripped on to
	if (enter_loop)
	{
		MG_CHECK(wheel_release_status, (wheel_release_status = _mutexgear_wheel_release(&__worker_instance->progress_wheel)) == EOK); // No way to handle -- must succeed
	}

	// Release own detach lock to let the worker know the thread has finished accessing the worker instance
	MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__waiter_instance->wait_detach_lock)) == EOK); // No way to handle -- must succeed

	// Acquire and immediately release the worker detach lock to make sure the worker has received this thread's signal and has finished accessing the wait_info pointer it had.
	MG_CHECK(mutex_lock_status, (mutex_lock_status = _mutexgear_lock_acquire(&__queue_instance->worker_detach_lock)) == EOK); // No way to handle -- must succeed
	MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(&__queue_instance->worker_detach_lock)) == EOK); // No way to handle -- must succeed
}


//////////////////////////////////////////////////////////////////////////
// Completion DrainableQueue Implementation

/*_MUTEXGEAR_PURE_INLINE */
int _mutexgear_completion_drain_init(mutexgear_completion_drain_t *__drain_instance)
{
	mutexgear_dlralist_init(&__drain_instance->drain_list);

	return EOK;
}

/*_MUTEXGEAR_PURE_INLINE */
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
	}
	while (false);

	return ret;
}


/*_MUTEXGEAR_PURE_INLINE */
int _mutexgear_completion_drain_preparedestroy(mutexgear_completion_drain_t *__drain_instance)
{
	int ret = mutexgear_dlralist_isempty(&__drain_instance->drain_list) ? EOK : EBUSY;
	return ret;
}

/*_MUTEXGEAR_PURE_INLINE */
void _mutexgear_completion_drain_completedestroy(mutexgear_completion_drain_t *__drain_instance)
{
	int list_emptiness_check;

	MG_CHECK(list_emptiness_check, mutexgear_dlralist_isempty(&__drain_instance->drain_list) || (list_emptiness_check = EBUSY, false));

	mutexgear_dlralist_destroy(&__drain_instance->drain_list);
}

/*_MUTEXGEAR_PURE_INLINE */
void _mutexgear_completion_drain_unpreparedestroy(mutexgear_completion_drain_t *__drain_instance)
{
	// Do nothing
}


/*_MUTEXGEAR_PURE_INLINE */
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

/*_MUTEXGEAR_PURE_INLINE */
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


/*_MUTEXGEAR_PURE_INLINE */
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

/*_MUTEXGEAR_PURE_INLINE */
void _mutexgear_completion_drainablequeue_completedestroy(mutexgear_completion_drainablequeue_t *__queue_instance)
{
	_mutexgear_completion_queue_completedestroy(&__queue_instance->basic_queue);
}

/*_MUTEXGEAR_PURE_INLINE */
void _mutexgear_completion_drainablequeue_unpreparedestroy(mutexgear_completion_drainablequeue_t *__queue_instance)
{
	_mutexgear_completion_queue_unpreparedestroy(&__queue_instance->basic_queue);
}


#endif // #ifndef __MUTEXGEAR_MG_COMPLETION_H_INCLUDED
