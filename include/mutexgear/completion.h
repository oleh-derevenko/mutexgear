#ifndef __MUTEXGEAR_COMPLETION_H_INCLUDED
#define __MUTEXGEAR_COMPLETION_H_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear Completion API Public Definitions                          */
/*                                                                      */
/* WARNING!                                                             */
/* This library contains a synchronization technique protected by       */
/* the U.S. Patent 9,983,913.                                           */
/*                                                                      */
/* THIS IS A PRE-RELEASE LIBRARY SNAPSHOT.                              */
/* AWAIT THE RELEASE AT https://mutexgear.com                           */
/*                                                                      */
/* Copyright (c) 2016-2024 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/

/**
 *	\file
 *	\brief MutexGear Completion API public definitions
 *
 *	The header defines a "Completion" object collection. The Completion is 
 *	a mechanism that allows waiting for an operation to end. Its main components
 *	are a "Completion Worker" and a "Completion Waiter" implemented with
 *	\c mutexgear_completion_worker_t and \c mutexgear_completion_waiter_t respectively. 
 *	Additionally, three "Completion Queues" are implemented as well as 
 *	a "Completion Item" structure (\c mutexgear_completion_item_t) to be 
 *	a link element in these queues. Two of the queues are used as building blocks
 *	for \c mutexgear_rwlock_t object implementation. The third one is provided rather as 
 *	an example that should be usable though. The \c mutexgear_completion_item_t
 *	is intended for inclusion as a member into heap allocated end-objects to be linked 
 *	or for allocation on stack for execution control logic.
 *
 *	Some of the queue methods are implemented as "static inline" functions.
 *
 *	NOTE:
 *
 *	The \c mutexgear_completion_worker_t object depends on a synchronization mechanism 
 *	being a subject of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and
 *	Image Database search (currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm)
 *	to view the patent text.
 */


#include <mutexgear/config.h>
#include <mutexgear/wheel.h>
#include <mutexgear/dlralist.h>
#include <mutexgear/utility.h>


_MUTEXGEAR_BEGIN_EXTERN_C();


//////////////////////////////////////////////////////////////////////////
// mutexgear_completion_genattr_t 

/**
 *	\struct mutexgear_completion_genattr_t
 *	\brief An opaque attribute structure to be used to initialize completion objects.
 */
typedef struct _mutexgear_completion_genattr
{
	_MUTEXGEAR_LOCKATTR_T	lock_attr;

} mutexgear_completion_genattr_t;


/**
 *	\fn int mutexgear_completion_genattr_init(mutexgear_completion_genattr_t *__attr_instance)
 *	\brief A function to initialize a \c mutexgear_completion_genattr_t instance.
 *
 *	The attributes are initialized with defaults that are counterparts of the underlying
 *	system structure defaults. Typically these are:
 *	\li MUTEXGEAR_PROCESS_PRIVATE for "pshared";
 *	\li MUTEXGEAR_PRIO_INHERIT for "protocol".
 *	\return EOK on success or a system error code on failure.
 */
_MUTEXGEAR_API int mutexgear_completion_genattr_init(mutexgear_completion_genattr_t *__attr_instance);

/**
 *	\fn int mutexgear_completion_genattr_destroy(mutexgear_completion_genattr_t *__attr_instance)
 *	\brief A function to destroy a previously initialized \c mutexgear_completion_genattr_t
 *	instance and free any resources possibly allocated for it.
 *	\return EOK on success or a system error code on failure.
 */
_MUTEXGEAR_API int mutexgear_completion_genattr_destroy(mutexgear_completion_genattr_t *__attr_instance);


/**
 *	\fn int mutexgear_completion_genattr_setpshared(mutexgear_completion_genattr_t *__attr_instance, int __pshared)
 *	\brief A function to assign process shared attribute value to
 *	a \c mutexgear_completion_genattr_t structure (similar to \c the pthread_mutexattr_setpshared).
 *	\param __pshared_value one of \c MUTEXGEAR_PROCESS_PRIVATE or \c MUTEXGEAR_PROCESS_SHARED
 *	\return EOK on success or a system error code on failure.
 */
_MUTEXGEAR_API int mutexgear_completion_genattr_setpshared(mutexgear_completion_genattr_t *__attr_instance, int __pshared_value);

/**
 *	\fn int mutexgear_completion_genattr_getpshared(const mutexgear_completion_genattr_t *__attr_instance, int *__pshared)
 *	\brief A function to get process shared attribute value stored in
 *	a \c mutexgear_completion_genattr_t structure (similar to the \c pthread_mutexattr_getpshared).
 *	\param __out_pshared_value pointer to a variable to receive process shared attribute value stored in the structure (either \c MUTEXGEAR_PROCESS_PRIVATE or \c MUTEXGEAR_PROCESS_SHARED)
 *	\return EOK on success or a system error code on failure.
 */
_MUTEXGEAR_API int mutexgear_completion_genattr_getpshared(const mutexgear_completion_genattr_t *__attr_instance, int *__out_pshared_value);


/**
 *	\fn int mutexgear_completion_genattr_setprioceiling(mutexgear_completion_genattr_t *__attr_instance, int __prioceiling)
 *	\brief A function to assign a priority ceiling value to
 *	a \c mutexgear_completion_genattr_t structure (similar to the \c pthread_mutexattr_setprioceiling).
 *	\param __prioceiling_value a priority value to be used as the ceiling
 *	\return EOK on success or a system error code on failure.
 */
_MUTEXGEAR_API int mutexgear_completion_genattr_setprioceiling(mutexgear_completion_genattr_t *__attr_instance, int __prioceiling_value);

/**
 *	\fn int mutexgear_completion_genattr_setprotocol(mutexgear_completion_genattr_t *__attr_instance, int __protocol)
 *	\brief A function to set a lock protocol value to
 *	a \c mutexgear_completion_genattr_t structure (similar to the \c pthread_mutexattr_setprotocol).
 *	\param __protocol_value one of MUTEXGEAR_PRIO_INHERIT, MUTEXGEAR_PRIO_PROTECT, MUTEXGEAR_PRIO_NONE
 *	\return EOK on success or a system error code on failure.
 */
_MUTEXGEAR_API int mutexgear_completion_genattr_setprotocol(mutexgear_completion_genattr_t *__attr_instance, int __protocol_value);

/**
 *	\fn int mutexgear_completion_genattr_getprioceiling(const mutexgear_completion_genattr_t *__attr_instance, int *__prioceiling)
 *	\brief A function to query priority ceiling attribute value
 *	stored in a \c mutexgear_completion_genattr_t structure (similar to the \c pthread_mutexattr_getprioceiling).
 *	\param __out_prioceiling_value pointer to a variable to receive priority ceiling attribute value stored in the structure (either a positive value or -1 if priority ceiling is not supported by the OS)
 *	\return EOK on success or a system error code on failure.
 */
_MUTEXGEAR_API int mutexgear_completion_genattr_getprioceiling(const mutexgear_completion_genattr_t *__attr_instance, int *__out_prioceiling_value);

/**
 *	\fn int mutexgear_completion_genattr_getprotocol(const mutexgear_completion_genattr_t *__attr_instance, int *__protocol)
 *	\brief A function to retrieve lock protocol attribute value
 *	stored in a \c mutexgear_completion_genattr_t structure (similar to the \c pthread_mutexattr_getprotocol).
 *	\param __out_protocol_value pointer to a variable to receive priority inheritance protocol attribute value stored in the structure (MUTEXGEAR_PRIO_INHERIT, MUTEXGEAR_PRIO_PROTECT, MUTEXGEAR_PRIO_NONE; or MUTEXGEAR__INVALID_PROTOCOL if the feature is not supported by the OS)
 *	\return EOK on success or a system error code on failure.
 */
_MUTEXGEAR_API int mutexgear_completion_genattr_getprotocol(const mutexgear_completion_genattr_t *__attr_instance, int *__out_protocol_value);

/**
 *	\fn int mutexgear_completion_genattr_setmutexattr(mutexgear_completion_genattr_t *__attr_instance, const _MUTEXGEAR_LOCKATTR_T *__mutexattr)
 *	\brief A function to copy attributes from a mutex attributes structure to
 *	a \c mutexgear_completion_genattr_t structure.
 *
 *	Values of "pshared", "protocol" and "prioceiling" are copied.
 *
 *	Due to inability to clear previously assigned priority ceiling setting from attributes on some targets
 *	it's recommended to call this function with freshly initialized \a __attr_instance only.
 *	\return EOK on success or a system error code on failure.
 */
_MUTEXGEAR_API int mutexgear_completion_genattr_setmutexattr(mutexgear_completion_genattr_t *__attr_instance, const _MUTEXGEAR_LOCKATTR_T *__mutexattr_instance);


//////////////////////////////////////////////////////////////////////////
// Completion Object Types

/**
 *	\struct mutexgear_completion_worker_t
 *	\brief A structure to represent a side performing an operation.
 *
 *	The structure contains a Wheel to be advanced whenever
 *	operation progress is made.
 */
typedef struct _mutexgear_completion_worker
{
	mutexgear_wheel_t	progress_wheel;

} mutexgear_completion_worker_t;


/**
 *	\struct mutexgear_completion_waiter_t
 *	\brief A structure to represent a side waiting for an operation to complete.
 *
 *	The structure contains a mutex to be used to let a Worker know about 
 *	the Waiter having received the completion signal and finished accessing 
 *	related shared objects.
 */
typedef struct _mutexgear_completion_waiter
{
	_MUTEXGEAR_LOCK_T	wait_detach_lock;

} mutexgear_completion_waiter_t;


//////////////////////////////////////////////////////////////////////////
// Completion Queue Types

// Helper definitions for mutexgear_completion_item_t
typedef ptrdiff_t _mutexgear_completion_item_extradata_t; // It does not really matter which type to use. The ptrdiff_t is OK for alignment and the library already has atomics for it.
#define _mg_atomic_construct_completion_item_extradata(p, v) _mg_atomic_construct_ptrdiff(p, v)
#define _mg_atomic_destroy_completion_item_extradata(p) _mg_atomic_destroy_ptrdiff(p)
#define _mg_atomic_reinit_completion_item_extradata(p, v) _mg_atomic_reinit_ptrdiff(p, v)
#define _mg_atomic_unsafeor_relaxed_completion_item_extradata(p, v) _mg_atomic_unsafeor_relaxed_ptrdiff(p, v)
#define _mg_atomic_unsafeand_relaxed_completion_item_extradata(p, v) _mg_atomic_unsafeand_relaxed_ptrdiff(p, v)
#define _mg_atomic_load_relaxed_completion_item_extradata(p) _mg_atomic_load_relaxed_ptrdiff(p)
#define _MG_PA_COMPLETION_ITEM_EXTRADATA(p) _MG_PA_PTRDIFF(p)
#define _MG_PVA_COMPLETION_ITEM_EXTRADATA(p) _MG_PVA_PTRDIFF(p)
#define _MG_PCVA_COMPLETION_ITEM_EXTRADATA(p) _MG_PCVA_PTRDIFF(p)
#define _MUTEXGEAR_COMPLETION_ITEM_EXTRA___HIGHEST_BIT (PTRDIFF_MAX - (PTRDIFF_MAX >> 1))
MG_STATIC_ASSERT(_MUTEXGEAR_COMPLETION_ITEM_EXTRA___HIGHEST_BIT != 0);
MG_STATIC_ASSERT((_MUTEXGEAR_COMPLETION_ITEM_EXTRA___HIGHEST_BIT & (_MUTEXGEAR_COMPLETION_ITEM_EXTRA___HIGHEST_BIT - 1)) == 0);

typedef struct __mutexgear_completion_itemdata
{
	mutexgear_dlraitem_t		work_item;
	_mutexgear_completion_item_extradata_t extra_data;

} _mutexgear_completion_itemdata_t;

/**
 *	\struct mutexgear_completion_item_t
 *	\brief A structure to represent an operation that can be waited for.
 *
 *	The structure contains a Relative-Atomic List Item to be used for building 
 *	a queue, an atomically accessed relative pointer that indicates that the Item 
 *	is being worked on when a Worker is assigned there or that the former is being 
 *	waited for if it points to a Waiter, and an extra integral field
 *	to keep helper flags for client code.
 */
typedef struct _mutexgear_completion_item
{
	_mutexgear_completion_itemdata_t data;
	ptrdiff_t					p_worker_or_waiter;

} mutexgear_completion_item_t;


// mutexgear_completion_item_t field public accessors

/**
 *	\fn mutexgear_dlraitem_t *mutexgear_completion_item_getworkitem(const mutexgear_completion_item_t *__item_instance)
 *	\brief Returns a pointer to the work list item contained within a completion item
 */
_MUTEXGEAR_PURE_INLINE
mutexgear_dlraitem_t *mutexgear_completion_item_getworkitem(const mutexgear_completion_item_t *__item_instance)
{
	const mutexgear_dlraitem_t *ret = &__item_instance->data.work_item;
	return (mutexgear_dlraitem_t *)ret;
}

/**
 *	\def  MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT
 *	\brief Number of bit size tags that can be stored within a \c mutexgear_completion_item_t
 *
 *	The tags are provided for client code use. The completion item does not need them for anything other than 
 *	structure size alignment. Other higher level objects that use the \c mutexgear_completion_item_t in this Library
 *	make use of the tags though.
 *
 *	The flags are initialized to zeros with client creation and are not altered by Completion group of functions after that.
 */
#define MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT	15U
MG_STATIC_ASSERT((_mutexgear_completion_item_extradata_t)1 << MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT <= _MUTEXGEAR_COMPLETION_ITEM_EXTRA___HIGHEST_BIT);

/**
 *	\fn void mutexgear_completion_item_settag(mutexgear_completion_item_t *__item_instance, unsigned int __tag_index, bool __tag_value)
 *	\brief Assigns a value to a tag.
 *
 *	Tags are handled in thread safe manned with respect to other tags of the same Item, however the modifications of each single tag 
 *	are not atomic and must be serialized with external means. Also, no memory barriers are issued on tag assignments. 
 *	If more strict atomicity is needed, derive a child structure from \c mutexgear_completion_item_t and implement the necessary
 *	storage as an extension there.
 */
_MUTEXGEAR_PURE_INLINE void mutexgear_completion_item_settag(mutexgear_completion_item_t *__item_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value);

/**
 *	\fn void mutexgear_completion_item_setunsafetag(mutexgear_completion_item_t *__item_instance, unsigned int __tag_index, bool __tag_value)
 *	\brief Assigns a value to a tag without ensuring thread safety
 *
 *	The function operates on tag in a thread unsafe manner considering accesses to other tags. If there are other threads trying to access tags 
 *	of the same item the accesses must be serialized with external means.
 */
_MUTEXGEAR_PURE_INLINE void mutexgear_completion_item_setunsafetag(mutexgear_completion_item_t *__item_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value);

/**
*	\fn void mutexgear_completion_item_setallunsafetags(mutexgear_completion_item_t *__item_instance, bool __tag_value)
*	\brief Assigns same value to all tags without ensuring thread safety
*
*	The function operates on tag in a thread unsafe manner. If there are other threads trying to access tags
*	of the same item the accesses must be serialized with external means.
*/
_MUTEXGEAR_PURE_INLINE void mutexgear_completion_item_setallunsafetags(mutexgear_completion_item_t *__item_instance, bool __tag_value);

/**
 *	\fn bool mutexgear_completion_item_unsafemodifytag(mutexgear_completion_item_t *__item_instance, unsigned int __tag_index, bool __tag_value)
 *	\brief Modifies a tag in an unsafe manner (a test and an assignment without atomicity) and returns whether the modification has actually occurred
 *
 *	Tags are handled in thread safe manned with respect to other tags of the same Item, however the modifications of each single tag
 *	are not atomic and must be serialized with external means. Also, no memory barriers are issued on tag assignments.
 *	If more strict atomicity is needed, derive a child structure from \c mutexgear_completion_item_t and implement the necessary
 *	storage as an extension there.
 *	\return Whether modification occurred
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_item_unsafemodifytag(mutexgear_completion_item_t *__item_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value);

/**
 *	\fn bool mutexgear_completion_item_modifyunsafetag(mutexgear_completion_item_t *__item_instance, unsigned int __tag_index, bool __tag_value)
 *	\brief Modifies a tag without ensuring thread safety
 *
 *	The function operates on tag in a thread unsafe manner considering accesses to other tags. If there are other threads trying to access tags
 *	of the same item the accesses must be serialized with external means.
 *	\return Whether modification occurred
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_item_modifyunsafetag(mutexgear_completion_item_t *__item_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value);

/**
 *	\fn bool mutexgear_completion_item_gettag(const mutexgear_completion_item_t *__item_instance, unsigned int __tag_index)
 *	\brief Returns value of an item's tag
 *
 *	Tags are handled in thread safe manned with respect to other tags of the same Item, however the modifications of each single tag
 *	are not atomic and must be serialized with external means. Also, no memory barriers are issued on tag assignments.
 *	If more strict atomicity is needed, derive a child structure from \c mutexgear_completion_item_t and implement the necessary
 *	storage as an extension there.
 *	\return Current value of the tag
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_item_gettag(const mutexgear_completion_item_t *__item_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/);

/**
*	\fn bool mutexgear_completion_item_getanytags(const mutexgear_completion_item_t *__item_instance)
*	\brief Returns if any tag is set
*
*	Tags are handled in thread safe manned with respect to other tags of the same Item, however the modifications of each single tag
*	are not atomic and must be serialized with external means. Also, no memory barriers are issued on tag assignments.
*	If more strict atomicity is needed, derive a child structure from \c mutexgear_completion_item_t and implement the necessary
*	storage as an extension there.
*	\return Current value of the tag
*/
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_item_getanytags(const mutexgear_completion_item_t *__item_instance);



/**
 *	\struct mutexgear_completion_queue_t
 *	\brief A basic Completion Queue that supports waiting on its items.
 *
 *	The structure contains two muteces, one being used for access serialization
 *	and the other for letting Waiters know when Workers finish accessing 
 *	synchronization structures of the former. Also there is a Relative-Atomic List 
 *	of the queue items. The list is relative to allow queue sharing among processes.
 */
typedef struct _mutexgear_completion_queue
{
	_MUTEXGEAR_LOCK_T	access_lock;
	mutexgear_dlralist_t work_list;
	_MUTEXGEAR_LOCK_T	worker_detach_lock;

} mutexgear_completion_queue_t;


/**
 *	\typedef mutexgear_completion_locktoken_t
 *	\brief An opaque value to serve as a hint for the implementation that 
 *	queue access mutex is already locked and does not need to be locked in the call.
 *
 *	Note that since the muteces used are likely to be not recursive, 
 *	caller must not omit the indication of a previous lock to queue methods.
 */
struct _mutexgear_completion_locktoken;
typedef struct _mutexgear_completion_locktoken *mutexgear_completion_locktoken_t;


//////////////////////////////////////////////////////////////////////////
// Completion DrainableQueue Types

/**
 *	\typedef mutexgear_completion_drainidx_t
 *	\brief A type to contain drain sequential counters.
 */
typedef size_t mutexgear_completion_drainidx_t;


/**
 *	\struct mutexgear_completion_drainablequeue_t
 *	\brief A "Drainable" Queue that supports removing its items and moving them 
 *	into a separate list - a "Drain".
 *
 *	The structure contains a Basic Queue and a drain counter.
 */
typedef struct _mutexgear_completion_drainablequeue
{
	mutexgear_completion_queue_t basic_queue;
	mutexgear_completion_drainidx_t drain_index;

} mutexgear_completion_drainablequeue_t;


/**
 *	\struct mutexgear_completion_drain_t
 *	\brief A Drain List to be used with Drainable Queues.
 *
 *	The structure contains a Relative-Atomic list to contain queue items.
 */
typedef struct _mutexgear_completion_drain
{
	mutexgear_dlralist_t	drain_list;

} mutexgear_completion_drain_t;


//////////////////////////////////////////////////////////////////////////
// Completion CancelableQueue Types

/**
 *	\struct mutexgear_completion_cancelablequeue_t
 *	\brief A "Cancelable" Queue that supports delayed start of Item handling
 *	by several Worker threads as well as requesting Item handling cancel along with
 *	waiting for operations on the Item to be actually canceled.
 *
 *	The structure contains a Basic Queue as a member. No extra fields are required.
 *
 *	The Queue is provided as an example and is not used by the library internally.
 *	Normally, when its is necessary to add an indication that some items have already been
 *	taken into work, a pointer to the first available item or additional linked lists 
 *	should be used, rather than setting and checking bits in Item extra field, to avoid iteration.
 *	This would be an unfit feature for a library, however.
 */
typedef struct _mutexgear_completion_cancelablequeue
{
	mutexgear_completion_queue_t basic_queue;

} mutexgear_completion_cancelablequeue_t;


/**
 *	\typedef mutexgear_completion_ownership_t
 *	\brief An enumeration to indicate Item ownership
 */
typedef enum _mutexgear_completion_ownership
{
	mg_completion_ownership__min,

	mg_completion_not_owner = mg_completion_ownership__min,	//!< The caller is not the Item owner and must not access or delete it
	mg_completion_owner,		//!< The caller is the Item owner and is responsible for deleting it if that is necessary

	mg_completion_ownership__max,

} mutexgear_completion_ownership_t;

/**
 *	\typedef typedef void (*mutexgear_completion_cancel_fn_t)(void *__cancel_context, mutexgear_completion_cancelablequeue_t *__queue, mutexgear_completion_worker_t *__worker, mutexgear_completion_item_t *__item)
 *	\brief A type definition for completion item operation canceling callback.
 */
typedef void (*mutexgear_completion_cancel_fn_t)(void *__cancel_context, mutexgear_completion_cancelablequeue_t *__queue, mutexgear_completion_worker_t *__worker, mutexgear_completion_item_t *__item);

//////////////////////////////////////////////////////////////////////////
// Completion Object APIs

/**
 *	\fn int mutexgear_completion_worker_init(mutexgear_completion_worker_t *__worker_instance, const mutexgear_completion_genattr_t *__attr_instance)
 *	\brief Initialize a Completion Worker structure
 *
 *	\param __attr_instance The attributes to be used for initialization or NULL for the defaults
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_worker_destroy
 */
_MUTEXGEAR_API int mutexgear_completion_worker_init(mutexgear_completion_worker_t *__worker_instance, const mutexgear_completion_genattr_t *__attr_instance/*=NULL*/);

/**
 *	\fn int mutexgear_completion_worker_destroy(mutexgear_completion_worker_t *__worker_instance)
 *	\brief Destroy an initialized Completion Worker structure
 *
 *	The Worker must be unlocked to be destroyed.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_worker_init
 *	\see mutexgear_completion_worker_unlock
 */
_MUTEXGEAR_API int mutexgear_completion_worker_destroy(mutexgear_completion_worker_t *__worker_instance);

/**
 *	\fn int mutexgear_completion_worker_lock(mutexgear_completion_worker_t *__worker_instance)
 *	\brief Locks a Completion Worker structure
 *
 *	The structure must be locked by the thread performing Completion calls to be used in the calls.
 *	The function was separated from initialization to allow the latter to be performed in another thread in advance.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_worker_unlock
 */
_MUTEXGEAR_API int mutexgear_completion_worker_lock(mutexgear_completion_worker_t *__worker_instance);

/**
 *	\fn int mutexgear_completion_worker_unlock(mutexgear_completion_worker_t *__worker_instance)
 *	\brief Unlocks a previously locked Completion Worker structure
 *
 *	The function was separated from destruction to allow the latter to be performed in another thread at later time.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_worker_unlock
 */
_MUTEXGEAR_API int mutexgear_completion_worker_unlock(mutexgear_completion_worker_t *__worker_instance);


/**
 *	\fn int mutexgear_completion_waiter_init(mutexgear_completion_waiter_t *__waiter_instance, const mutexgear_completion_genattr_t *__attr_instance)
 *	\brief Initialize a Completion Waiter structure
 *
 *	\param __attr_instance The attributes to be used for initialization or NULL for the defaults
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_waiter_destroy
 */
_MUTEXGEAR_API int mutexgear_completion_waiter_init(mutexgear_completion_waiter_t *__waiter_instance, const mutexgear_completion_genattr_t *__attr_instance/*=NULL*/);

/**
 *	\fn int mutexgear_completion_waiter_destroy(mutexgear_completion_waiter_t *__waiter_instance)
 *	\brief Destroy an initialized Completion Waiter structure
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_waiter_init
 */
_MUTEXGEAR_API int mutexgear_completion_waiter_destroy(mutexgear_completion_waiter_t *__waiter_instance);


//////////////////////////////////////////////////////////////////////////
// Completion Queue APIs

/**
 *	\fn void mutexgear_completion_item_init(mutexgear_completion_item_t *__item_instance)
 *	\brief Initializes a Completion Item
 *
 *	The function additionally initializes all item's tag values to false.
 *
 *	The function is implemented as an inline call.
 *	\see mutexgear_completion_item_destroy
 */
_MUTEXGEAR_PURE_INLINE void mutexgear_completion_item_init(mutexgear_completion_item_t *__item_instance);

/**
 *	\fn void mutexgear_completion_item_reinit(mutexgear_completion_item_t *__item_instance)
 *	\brief Returns a Completion Item to the state as if just initialized not affecting item's tags
 *
 *	The function is implemented as an inline call.
 *	\see mutexgear_completion_item_destroy
 */
_MUTEXGEAR_PURE_INLINE void mutexgear_completion_item_reinit(mutexgear_completion_item_t *__item_instance);

/**
 *	\fn bool mutexgear_completion_item_isasinit(const mutexgear_completion_item_t *__item_instance)
 *	\brief Checks whether an item is in the state as if just initialized.
 *
 *	The function is useful for assertion checks.
 *
 *	The function is implemented as an inline call.
 *	\return true if the parameter is as just initialized
 *	\see mutexgear_completion_item_init
 *	\see mutexgear_completion_item_reinit
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_item_isasinit(const mutexgear_completion_item_t *__item_instance);

/**
 *	\fn void mutexgear_completion_item_destroy(mutexgear_completion_item_t *__item_instance)
 *	\brief Destroys a previously initialized Completion Item instance
 *
 *	The function is implemented as an inline call.
 *	\see mutexgear_completion_item_init
 */
_MUTEXGEAR_PURE_INLINE void mutexgear_completion_item_destroy(mutexgear_completion_item_t *__item_instance);

/**
 *	\fn void mutexgear_completion_item_prestart(mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
 *	\brief Marks an Item as started to be handled by the given Worker while the item is not queued yet
 *
 *	The function is not thread safe and must be executed before the item is inserted into a queue.
 *
 *	The function is implemented as an inline call.
 *	\see mutexgear_completion_queueditem_start
 *	\see mutexgear_completion_cancelablequeueditem_start
 *	\see mutexgear_completion_item_isstarted
 *	\see mutexgear_completion_queueditem_safefinish
 *	\see mutexgear_completion_cancelablequeueditem_safefinish
 */
_MUTEXGEAR_PURE_INLINE void mutexgear_completion_item_prestart(mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance);

/**
 *	\fn bool mutexgear_completion_item_isstarted(const mutexgear_completion_item_t *__item_instance)
 *	\brief Tests if item was marked as started
 *
 *	The function can be called regardless of the item and its queue state, provided the item is known to have not been finished or canceled yet.
 *
 *	The function is implemented as an inline call.
 *	\see mutexgear_completion_item_prestart
 *	\see mutexgear_completion_queueditem_start
 *	\see mutexgear_completion_cancelablequeueditem_start
 *	\see mutexgear_completion_queueditem_safefinish
 *	\see mutexgear_completion_cancelablequeueditem_safefinish
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_item_isstarted(const mutexgear_completion_item_t *__item_instance);



/**
 *	\fn int mutexgear_completion_queue_init(mutexgear_completion_queue_t *__queue_instance, const mutexgear_completion_genattr_t *__attr_instance)
 *	\brief Initialize a Completion Queue instance
 *
 *	\param __attr_instance Attributes to be used for the initialization or NULL to use defaults
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_queue_destroy
 */
_MUTEXGEAR_API int mutexgear_completion_queue_init(mutexgear_completion_queue_t *__queue_instance, const mutexgear_completion_genattr_t *__attr_instance/*=NULL*/);

/**
 *	\fn int mutexgear_completion_queue_destroy(mutexgear_completion_queue_t *__queue_instance)
 *	\brief Destroy a previously initialized Completion Queue
 *
 *	The queue must be empty to be destroyed.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_queue_init
 */
_MUTEXGEAR_API int mutexgear_completion_queue_destroy(mutexgear_completion_queue_t *__queue_instance);


/**
 *	\fn int mutexgear_completion_queue_lock(mutexgear_completion_locktoken_t *__out_acquired_lock, mutexgear_completion_queue_t *__queue_instance)
 *	\brief Lock a Completion Queue access mutex
 *
 *	The call can be used to widen a critical section used for queue operations.
 *
 *	\c __out_acquired_lock can be NULL or can point to a variable to receive a lock token that can later be passed
 *	to other queue functions to indicate that the queue is already locked and needs not to be locked in the function 
 *	implementations internally.
 *
 *	\param __out_acquired_lock NULLL or a pointer to a variable to receive lock token
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_queue_plainunlock
 */
_MUTEXGEAR_API int mutexgear_completion_queue_lock(mutexgear_completion_locktoken_t *__out_acquired_lock/*=NULL*/, mutexgear_completion_queue_t *__queue_instance);

/**
 *	\fn int mutexgear_completion_queue_plainunlock(mutexgear_completion_queue_t *__queue_instance)
 *	\brief Unlocks a previously locked Completion Queue
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_queue_lock
 */
_MUTEXGEAR_API int mutexgear_completion_queue_plainunlock(mutexgear_completion_queue_t *__queue_instance);

/**
 *	\fn int mutexgear_completion_queue_unlockandwait(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_to_be_waited, mutexgear_completion_waiter_t *__waiter_instance)
 *	\brief Unlocks a previously locked Completion Queue and atomically waits for an Item to be finished being handled by a Worker
 *
 *	The queue must be previously locked by \c mutexgear_completion_queue_lock.
 *
 *	If the item was already finished to be handled at the time of the call the function returns success immediately.
 *	If the item handling has not been started at the moment of the call yet the function blocks until 
 *	a Worker starts and then finishes handling the item. The caller must not access the Item after the function returns.
 *
 *	The waiting cannot be canceled.
 *
 *	\param __item_to_be_waited Item to be waited for completion of
 *	\param __waiter_instance An initialized instance of Completion Waiter to be used for the operation
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_queue_lock
 */
_MUTEXGEAR_API int mutexgear_completion_queue_unlockandwait(mutexgear_completion_queue_t *__queue_instance,
	mutexgear_completion_item_t *__item_to_be_waited, mutexgear_completion_waiter_t *__waiter_instance);

/**
 *	\fn bool mutexgear_completion_queue_lodisempty(const mutexgear_completion_queue_t *__queue_instance)
 *	\brief Checks whether a Completion Queue is empty
 *
 *	The function is safe to be called on an unlocked queue. The "lod" abbreviation in the name stands for 
 *	"Locked or Declining". That is, positive result of the function is meaningful only when the queue is locked 
 *	or it is known to be not growing. Similarly, negative result is meaningful to be used when the queue is locked 
 *	or is known to be not declining.
 *
 *	The function is implemented as an inline call.
 *	\return true if the queue was empty
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_queue_lodisempty(const mutexgear_completion_queue_t *__queue_instance);

/**
 *	\fn bool mutexgear_completion_queue_gettail(mutexgear_completion_item_t **__out_tail_item, mutexgear_completion_queue_t *__queue_instance)
 *	\brief Returns last inserted Item of a Queue
 *
 *	The function is safe to be called on an unlocked queue when it is known to be not declining. Though, naturally, the item
 *	returned may not be the actual tail already then.
 *
 *	If the queue is empty the function returns false and the value pointed to by \c __out_tail_item contains the value that would be returned 
 *	by \c mutexgear_completion_queue_getrend.
 *
 *	The function is implemented as an inline call.
 *	\param __out_next_item Pointer to a variable to receive Queue's tail item pointer
 *	\return true if the queue was not empty
 *	\see mutexgear_completion_queue_getpreceding
 *	\see mutexgear_completion_queue_getunsafepreceding
 *	\see mutexgear_completion_queue_getrend
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_queue_gettail(mutexgear_completion_item_t **__out_tail_item,
	mutexgear_completion_queue_t *__queue_instance);

/**
 *	\fn bool mutexgear_completion_queue_getpreceding(mutexgear_completion_item_t **__out_preceding_item, mutexgear_completion_queue_t *__queue_instance, const mutexgear_completion_item_t *__item_instance)
 *	\brief Returns the preceding Item for a given Item in the Queue
 *
 *	The function is safe to be called on an unlocked queue when it is known to be not declining.
 *
 *	If \c __item_instance is the queue's first item the function returns false and the value pointed to by \c __out_preceding_item 
 *	contains the value that would be returned by \c mutexgear_completion_queue_getrend.
 *
 *	The function is implemented as an inline call.
 *	\param __out_preceding_item Pointer to a variable to receive the preceding item pointer
 *	\return true if the preceding item existed
 *	\see mutexgear_completion_queue_gettail
 *	\see mutexgear_completion_queue_getunsafepreceding
 *	\see mutexgear_completion_queue_getrend
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_queue_getpreceding(mutexgear_completion_item_t **__out_preceding_item,
	mutexgear_completion_queue_t *__queue_instance, const mutexgear_completion_item_t *__item_instance);

/**
 *	\fn mutexgear_completion_item_t *mutexgear_completion_queue_getunsafepreceding(const mutexgear_completion_item_t *__item_instance)
 *	\brief Returns the preceding Item for a given Item without checking for reaching queue begin
 *
 *	The function is safe to be called on an unlocked queue when it is known to be not declining.
 *
 *	The function can be called only for a valid queue item.
 *	If \c __item_instance is the queue's first item the the function returns the value that would be returned
  *	by \c mutexgear_completion_queue_getrend.
 *
 *	The function is implemented as an inline call.
 *	\return The preceding item for the parameter
 *	\see mutexgear_completion_queue_gettail
 *	\see mutexgear_completion_queue_getpreceding
 *	\see mutexgear_completion_queue_getrend
 */
_MUTEXGEAR_PURE_INLINE mutexgear_completion_item_t *mutexgear_completion_queue_getunsafepreceding(
	const mutexgear_completion_item_t *__item_instance);

/**
 *	\fn mutexgear_completion_item_t *mutexgear_completion_queue_getrend(mutexgear_completion_queue_t *__queue_instance)
 *	\brief Returns the queue "end item" pointer for reverse direction iteration
 *
 *	The function is safe to be called on an unlocked queue.
 *
 *	The end item pointer does not point to a valid queue item.
 *
 *	The function is implemented as an inline call.
 *	\return The reverse direction iteration end item of the queue
 *	\see mutexgear_completion_queue_gettail
 *	\see mutexgear_completion_queue_getpreceding
 *	\see mutexgear_completion_queue_getunsafepreceding
 */
_MUTEXGEAR_PURE_INLINE mutexgear_completion_item_t *mutexgear_completion_queue_getrend(mutexgear_completion_queue_t *__queue_instance);

/**
 *	\fn bool mutexgear_completion_queue_unsafegethead(mutexgear_completion_item_t **__out_head_item, mutexgear_completion_queue_t *__queue_instance)
 *	\brief Returns the first Item in a Queue
 *
 *	The function can be called only while the queue is locked.
 *
 *	If the queue is empty the function returns false and the value pointed to by \c __out_head_item contains the value that would be returned
 *	by \c mutexgear_completion_queue_getend.
 *
 *	The function is implemented as an inline call.
 *	\param __out_head_item Pointer to a variable to receive Queue's head item pointer
 *	\return true if the queue was not empty
 *	\see mutexgear_completion_queue_unsafegetunsafehead
 *	\see mutexgear_completion_queue_unsafegetnext
 *	\see mutexgear_completion_queue_unsafegetunsafenext
 *	\see mutexgear_completion_queue_getend
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_queue_unsafegethead(mutexgear_completion_item_t **__out_head_item,
	mutexgear_completion_queue_t *__queue_instance);

/**
 *	\fn _MUTEXGEAR_PURE_INLINE mutexgear_completion_item_t *mutexgear_completion_queue_unsafegetunsafehead(mutexgear_completion_queue_t *__queue_instance)
 *	\brief Returns the first Item in a Queue without checking for the queue to be not empty
 *
 *	The function can be called only while the queue is locked.
 *
 *	If \c __queue_instance is an empty queue the the function returns the value that would be returned
  *	by \c mutexgear_completion_queue_getend.
 *
 *	The function is implemented as an inline call.
 *	\return The head item of the the parameter queue
 *	\see mutexgear_completion_queue_unsafegethead
 *	\see mutexgear_completion_queue_unsafegetnext
 *	\see mutexgear_completion_queue_unsafegetunsafenext
 *	\see mutexgear_completion_queue_getend
 */
_MUTEXGEAR_PURE_INLINE mutexgear_completion_item_t *mutexgear_completion_queue_unsafegetunsafehead(
	mutexgear_completion_queue_t *__queue_instance);

/**
 *	\fn bool mutexgear_completion_queue_unsafegetnext(mutexgear_completion_item_t **__out_next_item, mutexgear_completion_queue_t *__queue_instance, const mutexgear_completion_item_t *__item_instance)
 *	\brief Returns the next Item for a given Item in the Queue
 *
 *	The function can be called only while the queue is locked.
 *
 *	If \c __item_instance is the queue's last item the function returns false and the value pointed to by \c __out_next_item
 *	contains the value that would be returned by \c mutexgear_completion_queue_getend.
 *
 *	The function is implemented as an inline call.
 *	\param __out_next_item Pointer to a variable to receive the next item pointer
 *	\return true if the next item existed
 *	\see mutexgear_completion_queue_unsafegethead
 *	\see mutexgear_completion_queue_unsafegetunsafenext
 *	\see mutexgear_completion_queue_getend
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_queue_unsafegetnext(mutexgear_completion_item_t **__out_next_item,
	mutexgear_completion_queue_t *__queue_instance, const mutexgear_completion_item_t *__item_instance);

/**
 *	\fn mutexgear_completion_item_t *mutexgear_completion_queue_unsafegetunsafenext(const mutexgear_completion_item_t *__item_instance)
 *	\brief Returns the next Item for a given Item without checking for reaching queue end
 *
 *	The function can be called only while the queue is locked.
 *
 *	The function can be called only for a valid queue item.
 *	If \c __item_instance is the queue's last item the the function returns the value that would be returned
  *	by \c mutexgear_completion_queue_getend.
 *
 *	The function is implemented as an inline call.
 *	\return The next item for the parameter
 *	\see mutexgear_completion_queue_unsafegethead
 *	\see mutexgear_completion_queue_unsafegetnext
 *	\see mutexgear_completion_queue_getend
 */
_MUTEXGEAR_PURE_INLINE mutexgear_completion_item_t *mutexgear_completion_queue_unsafegetunsafenext(
	const mutexgear_completion_item_t *__item_instance);

/**
 *	\fn mutexgear_completion_item_t *mutexgear_completion_queue_getend(mutexgear_completion_queue_t *__queue_instance)
 *	\brief Returns the queue "end item" pointer for forward direction iteration
 *
 *	The function is safe to be called on an unlocked queue.
 *
 *	The end item pointer does not point to a valid queue item.
 *
 *	The function is implemented as an inline call.
 *	\return The forward direction iteration end item of the queue
 *	\see mutexgear_completion_queue_unsafegethead
 *	\see mutexgear_completion_queue_unsafegetnext
 *	\see mutexgear_completion_queue_unsafegetunsafenext
 */
_MUTEXGEAR_PURE_INLINE mutexgear_completion_item_t *mutexgear_completion_queue_getend(mutexgear_completion_queue_t *__queue_instance);

/**
 *	\fn int mutexgear_completion_queue_enqueue(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_locktoken_t __lock_hint)
 *	\brief Adds Item to Queue's tail
 *
 *	Items can be enqueued pre-started with \c mutexgear_completion_item_prestart. They may also be started later 
 *	with \c mutexgear_completion_queueditem_start call, however waiting for an item that has not been stared yet fails with an error.
 *
 *	If the queue is locked, the \c __lock_hint must the corresponding lock token. Otherwise, NULL is to be passed.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_queue_unsafedequeue
 *	\see mutexgear_completion_item_prestart
 *	\see mutexgear_completion_queueditem_start
 */
_MUTEXGEAR_API int mutexgear_completion_queue_enqueue(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/);

/**
 *	\fn void mutexgear_completion_queue_unsafedequeue(mutexgear_completion_item_t *__item_instance)
 *	\brief Remove Item from its Queue
 *
 *	The function can be called only while the respective queue is locked. The call is intended for use by servers working 
 *	on derived queue classes and should be considered "protected" in terms of OOP.
 *
 *	\see mutexgear_completion_queue_enqueue
 */
_MUTEXGEAR_API void mutexgear_completion_queue_unsafedequeue(mutexgear_completion_item_t *__item_instance);


/**
 *	\fn void mutexgear_completion_queueditem_start(mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
 *	\brief Marks a queued Item as started to be handled by the given Worker
 *
 *	The function can be called only while the queue is locked.
 *
 *	The function is implemented as an inline call.
 *	\see mutexgear_completion_item_isstarted
 *	\see mutexgear_completion_queueditem_safefinish
 *	\see mutexgear_completion_item_prestart
 */
_MUTEXGEAR_PURE_INLINE void mutexgear_completion_queueditem_start(mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance);


/**
 *	\fn int mutexgear_completion_queueditem_safefinish(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
 *	\brief Mark Item as complete, remove it from Queue and notify possible Waiter locking and unlocking the queue as necessary.
 *
 *	The caller (the Worker) is responsible for disposing the Item.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_queue_enqueue
 *	\see mutexgear_completion_item_prestart
 *	\see mutexgear_completion_queueditem_start
 *	\see mutexgear_completion_item_isstarted
 *	\see mutexgear_completion_queueditem_unsafefinish__locked
 *	\see mutexgear_completion_queueditem_unsafefinish__unlocked
 */
_MUTEXGEAR_API int mutexgear_completion_queueditem_safefinish(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance);

/**
 *	\fn void mutexgear_completion_queueditem_unsafefinish__locked(mutexgear_completion_item_t *__item_instance)
 *	\brief A part of routing to mark an Item as complete, remove it from a Queue and notify possible Waiter to be called with the queue locked
 *
 *	The function implements the "locked" part of the routine while \c mutexgear_completion_queueditem_unsafefinish__unlocked, being the other part, is to be called
 *	after the queue lock is released. Together these correspond to a call of \c mutexgear_completion_queueditem_safefinish.
 *	The separation can be used to include extra code in the queue's critical section.
 *
 *	Calling the function while \c __queue_instance is not locked may result in threading races and memory structure corruptions.
 *
 *	The caller (the Worker) is responsible for disposing the Item.
 *
 *	\see mutexgear_completion_queueditem_unsafefinish__unlocked
 *	\see mutexgear_completion_queueditem_safefinish
 */
_MUTEXGEAR_API void mutexgear_completion_queueditem_unsafefinish__locked(mutexgear_completion_item_t *__item_instance);

/**
 *	\fn mutexgear_completion_queueditem_unsafefinish__unlocked(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
 *	\brief A part of routing to mark an Item as complete, remove it from a Queue and notify possible Waiter to be called after the queue is unlocked
 *
 *	The function implements the "unlocked" part of the routine while \c mutexgear_completion_queueditem_unsafefinish__locked, being the other part, is to be called
 *	within the queue's locked section, before it is exited. Together these correspond to a call of \c mutexgear_completion_queueditem_safefinish.
 *	The separation can be used to include extra code in the queue's critical section.
 *
 *	Calling the function while \c __queue_instance is locked will be an unnecessary extension of the critical section.
 *
 *	The caller (the Worker) is responsible for disposing the Item.
 *
 *	\see mutexgear_completion_queueditem_unsafefinish__locked
 *	\see mutexgear_completion_queueditem_safefinish
 */
_MUTEXGEAR_API void mutexgear_completion_queueditem_unsafefinish__unlocked(mutexgear_completion_queue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance);


//////////////////////////////////////////////////////////////////////////
// Completion DrainableQueue APIs

/**
 *	\fn int mutexgear_completion_drain_init(mutexgear_completion_drain_t *__drain_instance)
 *	\brief Initialize a drain instance
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_drain_destroy
 */
_MUTEXGEAR_API int mutexgear_completion_drain_init(mutexgear_completion_drain_t *__drain_instance);

/**
 *	\fn int mutexgear_completion_drain_destroy(mutexgear_completion_drain_t *__drain_instance)
 *	\brief Delete a previously initialized drain instance
 *
 *	The drain must be empty to be deleted.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_drain_destroy
 */
_MUTEXGEAR_API int mutexgear_completion_drain_destroy(mutexgear_completion_drain_t *__drain_instance);


/**
 *	\fn int mutexgear_completion_drainablequeue_init(mutexgear_completion_drainablequeue_t *__queue_instance, const mutexgear_completion_genattr_t *__attr_instance)
 *	\brief Initialize a Drainable Queue instance
 *
 *	\param __attr_instance Attributes to be used for the initialization or NULL to use defaults
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_drainablequeue_destroy
 */
_MUTEXGEAR_API int mutexgear_completion_drainablequeue_init(mutexgear_completion_drainablequeue_t *__queue_instance, const mutexgear_completion_genattr_t *__attr_instance/*=NULL*/);

/**
 *	\fn int mutexgear_completion_drainablequeue_destroy(mutexgear_completion_drainablequeue_t *__queue_instance)
 *	\brief Destroy a previously initialized Drainable Queue
 *
 *	The queue must be empty to be destroyed.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_drainablequeue_init
 */
_MUTEXGEAR_API int mutexgear_completion_drainablequeue_destroy(mutexgear_completion_drainablequeue_t *__queue_instance);


/**
 *	\fn int mutexgear_completion_drainablequeue_lock(mutexgear_completion_locktoken_t *__out_acquired_lock, mutexgear_completion_drainablequeue_t *__queue_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_lock
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_queue_lock
 */
_MUTEXGEAR_API int mutexgear_completion_drainablequeue_lock(mutexgear_completion_locktoken_t *__out_acquired_lock/*=NULL*/,
	mutexgear_completion_drainablequeue_t *__queue_instance);

/**
 *	\fn int mutexgear_completion_drainablequeue_getindex(mutexgear_completion_drainidx_t *__out_queue_drain_index, mutexgear_completion_drainablequeue_t *__queue_instance, mutexgear_completion_locktoken_t __lock_hint)
 *	\brief Retrieve current drain index for the queue
 *
 *	Drain index is incremented each time \c mutexgear_completion_drainablequeue_safedrain is called. 
 *	The index can be used to determine whether an Item is still in the Queue or whether it was drained.
 *
 *	If the queue is locked, the \c __lock_hint must the corresponding lock token. Otherwise, NULL is to be passed.
 *
 *	\param __out_queue_drain_index Pointer to a variable to receive queue's current drain index
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_drainablequeue_safedrain
 */
_MUTEXGEAR_API int mutexgear_completion_drainablequeue_getindex(mutexgear_completion_drainidx_t *__out_queue_drain_index,
	mutexgear_completion_drainablequeue_t *__queue_instance, mutexgear_completion_locktoken_t __lock_hint/*=NULL*/);

/**
 *	\fn int mutexgear_completion_drainablequeue_safedrain(mutexgear_completion_drainablequeue_t *__queue_instance, mutexgear_completion_item_t *__drain_head_item, mutexgear_completion_drainidx_t __item_drain_index, mutexgear_completion_drain_t *__target_drain, bool *__out_drain_execution_status)
 *	\brief Drain queue's items locking and unlocking the queue as necessary
 *
 *	The function considers items starting with \c __drain_head_item to be removed into the drain.
 *	If the specified queue tail is drained if \c __drain_head_item is the actual queue head 
 *	or if \c __item_drain_index matches the current queue's drain index. Otherwise the drain request is ignored.
 *	The \c __out_drain_execution_status (if provided) receives the drain status.
 *
 *	Calling the function while \c __queue_instance is locked may result in a mutex lock error.
 *
 *	\param __drain_head_item Head item to be drained
 *	\param __item_drain_index The drain index retrieved at the time when \c __drain_head_item was inserted
 *	\param __target_drain Drain instance to receive the items (if removed)
 *	\param __out_drain_execution_status Optional pointer to a variable to receive the drain execution status
 *	\return EOK on success or a system error code on failure (an ignored drain is still a success).
 *	\see mutexgear_completion_drainablequeue_unsafedrain__locked
 *	\see mutexgear_completion_drainablequeue_getindex
 */
_MUTEXGEAR_API int mutexgear_completion_drainablequeue_safedrain(mutexgear_completion_drainablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__drain_head_item, mutexgear_completion_drainidx_t __item_drain_index,
	mutexgear_completion_drain_t *__target_drain, bool *__out_drain_execution_status/*=NULL*/);

/**
 *	\fn void mutexgear_completion_drainablequeue_unsafedrain__locked(mutexgear_completion_drainablequeue_t *__queue_instance, mutexgear_completion_item_t *__drain_head_item, mutexgear_completion_drainidx_t __item_drain_index, mutexgear_completion_drain_t *__target_drain, bool *__out_drain_execution_status);
 *	\brief Drain queue's items assuming the queue is already locked
 *
 *	The function considers items starting with \c __drain_head_item to be removed into the drain.
 *	If the specified queue tail is drained if \c __drain_head_item is the actual queue head
 *	or if \c __item_drain_index matches the current queue's drain index. Otherwise the drain request is ignored.
 *	The \c __out_drain_execution_status (if provided) receives the drain status.
 *
 *	Calling the function while \c __queue_instance is not locked may result in threading races and memory structure corruptions.
 *
 *	\param __drain_head_item Head item to be drained
 *	\param __item_drain_index The drain index retrieved at the time when \c __drain_head_item was inserted
 *	\param __target_drain Drain instance to receive the items (if removed)
 *	\param __out_drain_execution_status Optional pointer to a variable to receive the drain execution status
 *	\see mutexgear_completion_drainablequeue_safedrain
 *	\see mutexgear_completion_drainablequeue_getindex
 */
_MUTEXGEAR_API void mutexgear_completion_drainablequeue_unsafedrain__locked(mutexgear_completion_drainablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__drain_head_item, mutexgear_completion_drainidx_t __item_drain_index,
	mutexgear_completion_drain_t *__target_drain, bool *__out_drain_execution_status/*=NULL*/);

/**
 *	\fn int mutexgear_completion_drainablequeue_plainunlock(mutexgear_completion_drainablequeue_t *__queue_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_plainunlock
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_queue_plainunlock
 */
_MUTEXGEAR_API int mutexgear_completion_drainablequeue_plainunlock(mutexgear_completion_drainablequeue_t *__queue_instance);

/**
 *	\fn int mutexgear_completion_drainablequeue_unlockandwait(mutexgear_completion_drainablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_to_be_waited, mutexgear_completion_waiter_t *__waiter_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_unlockandwait
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_queue_unlockandwait
 */
_MUTEXGEAR_API int mutexgear_completion_drainablequeue_unlockandwait(mutexgear_completion_drainablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_to_be_waited, mutexgear_completion_waiter_t *__waiter_instance);


/**
 *	\fn bool mutexgear_completion_drainablequeue_lodisempty(const mutexgear_completion_drainablequeue_t *__queue_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_lodisempty
 *
 *	\return true if the queue was empty
 *	\see mutexgear_completion_queue_lodisempty
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_drainablequeue_lodisempty(const mutexgear_completion_drainablequeue_t *__queue_instance);

/**
 *	\fn bool mutexgear_completion_drainablequeue_gettail(mutexgear_completion_item_t **__out_tail_item, mutexgear_completion_drainablequeue_t *__queue_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_gettail
 *
 *	\return true if the queue was not empty
 *	\see mutexgear_completion_queue_gettail
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_drainablequeue_gettail(mutexgear_completion_item_t **__out_tail_item,
	mutexgear_completion_drainablequeue_t *__queue_instance);

/**
 *	\fn bool mutexgear_completion_drainablequeue_getpreceding(mutexgear_completion_item_t **__out_preceding_item, mutexgear_completion_drainablequeue_t *__queue_instance, const mutexgear_completion_item_t *__item_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_getpreceding
 *
 *	\return true if the preceding item existed
 *	\see mutexgear_completion_queue_getpreceding
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_drainablequeue_getpreceding(mutexgear_completion_item_t **__out_preceding_item,
	mutexgear_completion_drainablequeue_t *__queue_instance, const mutexgear_completion_item_t *__item_instance);

/**
 *	\fn mutexgear_completion_item_t *mutexgear_completion_drainablequeue_getunsafepreceding(const mutexgear_completion_item_t *__item_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_getunsafepreceding
 *
 *	\return The preceding item for the parameter
 *	\see mutexgear_completion_queue_getunsafepreceding
 */
_MUTEXGEAR_PURE_INLINE mutexgear_completion_item_t *mutexgear_completion_drainablequeue_getunsafepreceding(const mutexgear_completion_item_t *__item_instance);

/**
 *	\fn mutexgear_completion_item_t *mutexgear_completion_drainablequeue_getrend(mutexgear_completion_drainablequeue_t *__queue_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_getrend
 *
 *	\return The queue reverse direction iteration end element
 *	\see mutexgear_completion_queue_getrend
 */
_MUTEXGEAR_PURE_INLINE mutexgear_completion_item_t *mutexgear_completion_drainablequeue_getrend(mutexgear_completion_drainablequeue_t *__queue_instance);

/**
 *	\fn bool mutexgear_completion_drainablequeue_unsafegethead(mutexgear_completion_item_t **__out_head_item, mutexgear_completion_drainablequeue_t *__queue_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_unsafegethead
 *
 *	\return true if the queue was not empty
 *	\see mutexgear_completion_queue_unsafegethead
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_drainablequeue_unsafegethead(mutexgear_completion_item_t **__out_head_item,
	mutexgear_completion_drainablequeue_t *__queue_instance);

/**
 *	\fn bool mutexgear_completion_drainablequeue_unsafegetnext(mutexgear_completion_item_t **__out_next_item, mutexgear_completion_drainablequeue_t *__queue_instance, const mutexgear_completion_item_t *__item_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_unsafegetnext
 *
 *	\return true if the next item existed
 *	\see mutexgear_completion_queue_unsafegetnext
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_drainablequeue_unsafegetnext(mutexgear_completion_item_t **__out_next_item,
	mutexgear_completion_drainablequeue_t *__queue_instance, const mutexgear_completion_item_t *__item_instance);

/**
 *	\fn mutexgear_completion_item_t *mutexgear_completion_drainablequeue_unsafegetunsafenext(const mutexgear_completion_item_t *__item_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_unsafegetunsafenext
 *
 *	\return The next item for the parameter
 *	\see mutexgear_completion_queue_unsafegetunsafenext
 */
_MUTEXGEAR_PURE_INLINE mutexgear_completion_item_t *mutexgear_completion_drainablequeue_unsafegetunsafenext(const mutexgear_completion_item_t *__item_instance);

/**
 *	\fn mutexgear_completion_item_t *mutexgear_completion_drainablequeue_getend(mutexgear_completion_drainablequeue_t *__queue_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_getend
 *
 *	\return The queue forward direction iteration end element
 *	\see mutexgear_completion_queue_getend
 */
_MUTEXGEAR_PURE_INLINE mutexgear_completion_item_t *mutexgear_completion_drainablequeue_getend(mutexgear_completion_drainablequeue_t *__queue_instance);

/**
 *	\fn int mutexgear_completion_drainablequeue_enqueue(mutexgear_completion_drainablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_locktoken_t __lock_hint, mutexgear_completion_drainidx_t *__out_queue_drain_index)
 *	\brief Atomically insert Item into a Queue and optionally get Queue's drain index
 *
 *	The function is an atomic combination of \c mutexgear_completion_drainablequeue_getindex 
 *	and inherited \c mutexgear_completion_queue_enqueue.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_drainablequeue_getindex 
 *	\see mutexgear_completion_queue_enqueue
 */
_MUTEXGEAR_API int mutexgear_completion_drainablequeue_enqueue(mutexgear_completion_drainablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/, mutexgear_completion_drainidx_t *__out_queue_drain_index/*=NULL*/);

/**
 *	\fn void mutexgear_completion_drainablequeue_unsafedequeue(mutexgear_completion_item_t *__item_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_unsafedequeue
 *
 *	\see mutexgear_completion_queue_unsafedequeue
 */
_MUTEXGEAR_API void mutexgear_completion_drainablequeue_unsafedequeue(mutexgear_completion_item_t *__item_instance);


/**
 *	\fn int mutexgear_completion_drainablequeueditem_safefinish(mutexgear_completion_drainablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_drainidx_t __item_drain_index, mutexgear_completion_drain_t *__target_drain)
 *	\brief Atomically mark Item as finished, remove it, and request an optional drain of the Queue tail after the Item
 *
 *	The function is an atomic combination of \c mutexgear_completion_drainablequeue_safedrain
 *	and inherited \c mutexgear_completion_queueditem_safefinish.
 *
 *	Calling the function while \c __queue_instance is locked may result in a mutex lock error and will be an inefficient call.
 *	The combination of \c mutexgear_completion_drainablequeueditem_unsafefinish__locked and \c mutexgear_completion_drainablequeueditem_unsafefinish__unlocked
 *	must be used in such cases instead.
 *
 *	\see mutexgear_completion_drainablequeue_safedrain
 *	\see mutexgear_completion_queueditem_safefinish
 *	\see mutexgear_completion_drainablequeueditem_unsafefinish__locked
 *	\see mutexgear_completion_drainablequeueditem_unsafefinish__unlocked
 */
_MUTEXGEAR_API int mutexgear_completion_drainablequeueditem_safefinish(mutexgear_completion_drainablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance,
	mutexgear_completion_drainidx_t __item_drain_index/*=MUTEXGEAR_COMPLETION_INVALID_DRAINIDX*/, mutexgear_completion_drain_t *__target_drain/*=NULL*/);

/**
 *	\fn void mutexgear_completion_drainablequeueditem_unsafefinish__locked(mutexgear_completion_drainablequeue_t *__queue_instance,	mutexgear_completion_item_t *__item_instance, mutexgear_completion_drainidx_t __item_drain_index, mutexgear_completion_drain_t *__target_drain)
 *	\brief A part of atomically marking Item as finished, removing it, and requesting an optionally drain of the Queue tail after the Item to be called with the queue locked
 *
 *	The function is a locked routine part of an atomic queue drain along with an item finish. The other counterpart (\c mutexgear_completion_drainablequeueditem_unsafefinish__unlocked)
 *	is to be called after the \c __queue_instance locked section is exited. Together, these function calls correspond to a call to \c mutexgear_completion_drainablequeueditem_safefinish.
 *	The separation can be used to include extra code in the queue's critical section.
 *
 *	Calling the function while \c __queue_instance is not locked may result in threading races and memory structure corruptions.
 *
 *	\see mutexgear_completion_drainablequeueditem_unsafefinish__unlocked
 *	\see mutexgear_completion_drainablequeueditem_safefinish
 *	\see mutexgear_completion_drainablequeue_unsafedrain__locked
 *	\see mutexgear_completion_queueditem_unsafefinish__locked
 */
_MUTEXGEAR_API void mutexgear_completion_drainablequeueditem_unsafefinish__locked(mutexgear_completion_drainablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_instance,
	mutexgear_completion_drainidx_t __item_drain_index/*=MUTEXGEAR_COMPLETION_INVALID_DRAINIDX*/, mutexgear_completion_drain_t *__target_drain/*=NULL*/);

/**
 *	\fn void mutexgear_completion_drainablequeueditem_unsafefinish__unlocked(mutexgear_completion_drainablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
 *	\brief A part of atomically marking Item as finished, removing it, and requesting an optionally drain of the Queue tail after the Item to be called after the queue is unlocked
 *
 *	The function is an unlocked routine part of an atomic queue drain along with an item finish. The other counterpart (\c mutexgear_completion_drainablequeueditem_unsafefinish__locked)
 *	is to be called withing the critical section, before the \c __queue_instance lock is released. Together, these function calls correspond to a call to \c mutexgear_completion_drainablequeueditem_safefinish.
 *	The separation can be used to include extra code in the queue's critical section.
 *
 *	Calling the function while \c __queue_instance is locked will be an unnecessary extension of the critical section.
 *
 *	\return EOK on success or a system error code on failure (an ignored drain is still a success).
 *	\see mutexgear_completion_drainablequeueditem_unsafefinish__locked
 *	\see mutexgear_completion_drainablequeueditem_safefinish
 *	\see mutexgear_completion_queueditem_unsafefinish__unlocked
 */
_MUTEXGEAR_API void mutexgear_completion_drainablequeueditem_unsafefinish__unlocked(mutexgear_completion_drainablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance);


//////////////////////////////////////////////////////////////////////////
// Completion CancelableQueue APIs


/**
 *	\enum _mutexgear_completion_cancelablequeue_itemtag
 *	\brief An enumeration of tag indices used by \c mutexgear_completion_cancelablequeue_t
 */
enum _mutexgear_completion_cancelablequeue_itemtag
{
	mutexgear_completion_cancelablequeue_itemtag__min,
	mutexgear_completion_cancelablequeue_itemtag__premin = mutexgear_completion_cancelablequeue_itemtag__min - 1,

	mutexgear_completion_cancelablequeue_itemtag_cancelrequested, //!< The Item is marked for cancel and work on it is to be aborted

	mutexgear_completion_cancelablequeue_itemtag__max, //!< The first available tag index to be used as a base for enumerations in derived classes
};


/**
 *	\fn int mutexgear_completion_cancelablequeue_init(mutexgear_completion_cancelablequeue_t *__queue_instance, const mutexgear_completion_genattr_t *__attr_instance)
 *	\brief Initialize a Cancelable Queue instance
 *
 *	\param __attr_instance Attributes to be used for the initialization or NULL to use defaults
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_cancelablequeue_destroy
 */
_MUTEXGEAR_API int mutexgear_completion_cancelablequeue_init(mutexgear_completion_cancelablequeue_t *__queue_instance, const mutexgear_completion_genattr_t *__attr_instance/*=NULL*/);

/**
 *	\fn int mutexgear_completion_cancelablequeue_destroy(mutexgear_completion_cancelablequeue_t *__queue_instance)
 *	\brief Destroy a previously initialized Cancelable Queue
 *
 *	The queue must be empty to be destroyed.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_cancelablequeue_init
 */
_MUTEXGEAR_API int mutexgear_completion_cancelablequeue_destroy(mutexgear_completion_cancelablequeue_t *__queue_instance);


/**
 *	\fn int mutexgear_completion_cancelablequeue_lock(mutexgear_completion_locktoken_t *__out_acquired_lock, mutexgear_completion_cancelablequeue_t *__queue_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_lock
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_queue_lock
 */
_MUTEXGEAR_API int mutexgear_completion_cancelablequeue_lock(mutexgear_completion_locktoken_t *__out_acquired_lock/*=NULL*/, mutexgear_completion_cancelablequeue_t *__queue_instance);

/**
 *	\fn int mutexgear_completion_cancelablequeue_plainunlock(mutexgear_completion_cancelablequeue_t *__queue_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_plainunlock
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_queue_plainunlock
 */
_MUTEXGEAR_API int mutexgear_completion_cancelablequeue_plainunlock(mutexgear_completion_cancelablequeue_t *__queue_instance);

/**
 *	\fn int mutexgear_completion_cancelablequeue_unlockandwait(mutexgear_completion_cancelablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_to_be_waited, mutexgear_completion_waiter_t *__waiter_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_unlockandwait
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_queue_unlockandwait
 */
_MUTEXGEAR_API int mutexgear_completion_cancelablequeue_unlockandwait(mutexgear_completion_cancelablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_to_be_waited, mutexgear_completion_waiter_t *__waiter_instance);


/**
 *	\fn int mutexgear_completion_cancelablequeue_unlockandcancel(mutexgear_completion_cancelablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_to_be_canceled, mutexgear_completion_waiter_t *__waiter_instance, mutexgear_completion_cancel_fn_t __item_cancel_fn, void *__cancel_context, mutexgear_completion_ownership_t *__out_item_resulting_ownership)
 *	\brief Mark Item to be canceled and either remove it from Queue if work on the Item was not started yet, or unlock the Queue and wait for the Item to be finished and removed by its Worker
 *
 *	The call requests its Item to be canceled. If work was not started on the Item yet 
 *	the Item is simply removed from the Queue and the caller becomes its owner (is responsible for disposing the Item).
 *	If the Item already has a Worker assigned at the moment of the call the Item is marked to be canceled 
 *	and waited for being finished and removed by its Worker. Workers are supposed to check their item statuses 
 *	with \c mutexgear_completion_cancelablequeueditem_iscanceled and abort any operations requiring significant effort if necessary.
 *	Additionally, a function \c __item_cancel_fn can be provided to be called after the cancel request has been set on the Item
 *	to unblock the Worker from any possible blocking states. If there was a Worker assigned to the Item the caller will not become 
 *	the Item's owner and must not access the Item after the function returns.
 *
 *	The function must be called with the queue locked.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_cancelablequeue_unlockandwait
 *	\see mutexgear_completion_cancelablequeue_enqueue
 */
_MUTEXGEAR_API int mutexgear_completion_cancelablequeue_unlockandcancel(mutexgear_completion_cancelablequeue_t *__queue_instance,
	mutexgear_completion_item_t *__item_to_be_canceled, mutexgear_completion_waiter_t *__waiter_instance,
	mutexgear_completion_cancel_fn_t __item_cancel_fn/*=NULL*/, void *__cancel_context/*=NULL*/,
	mutexgear_completion_ownership_t *__out_item_resulting_ownership);


/**
 *	\fn bool mutexgear_completion_cancelablequeue_lodisempty(const mutexgear_completion_cancelablequeue_t *__queue_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_lodisempty
 *
 *	\return true if the queue was empty
 *	\see mutexgear_completion_queue_lodisempty
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_cancelablequeue_lodisempty(const mutexgear_completion_cancelablequeue_t *__queue_instance);

/**
 *	\fn bool mutexgear_completion_cancelablequeue_gettail(mutexgear_completion_item_t **__out_tail_item, mutexgear_completion_cancelablequeue_t *__queue_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_gettail
 *
 *	\return true if the queue was not empty
 *	\see mutexgear_completion_queue_gettail
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_cancelablequeue_gettail(mutexgear_completion_item_t **__out_tail_item,
	mutexgear_completion_cancelablequeue_t *__queue_instance);

/**
 *	\fn bool mutexgear_completion_cancelablequeue_getpreceding(mutexgear_completion_item_t **__out_preceding_item, mutexgear_completion_cancelablequeue_t *__queue_instance, const mutexgear_completion_item_t *__item_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_getpreceding
 *
 *	\return true if the preceding item existed
 *	\see mutexgear_completion_queue_getpreceding
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_cancelablequeue_getpreceding(mutexgear_completion_item_t **__out_preceding_item,
	mutexgear_completion_cancelablequeue_t *__queue_instance, const mutexgear_completion_item_t *__item_instance);

/**
 *	\fn mutexgear_completion_item_t *mutexgear_completion_cancelablequeue_getunsafepreceding(const mutexgear_completion_item_t *__item_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_getunsafepreceding
 *
 *	\return The preceding item for the parameter
 *	\see mutexgear_completion_queue_getunsafepreceding
 */
_MUTEXGEAR_PURE_INLINE mutexgear_completion_item_t *mutexgear_completion_cancelablequeue_getunsafepreceding(const mutexgear_completion_item_t *__item_instance);

/**
 *	\fn mutexgear_completion_item_t *mutexgear_completion_cancelablequeue_getrend(mutexgear_completion_cancelablequeue_t *__queue_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_getrend
 *
 *	\return The queue reverse direction iteration end element
 *	\see mutexgear_completion_queue_getrend
 */
_MUTEXGEAR_PURE_INLINE mutexgear_completion_item_t *mutexgear_completion_cancelablequeue_getrend(mutexgear_completion_cancelablequeue_t *__queue_instance);

/**
 *	\fn bool mutexgear_completion_cancelablequeue_unsafegethead(mutexgear_completion_item_t **__out_head_item, mutexgear_completion_cancelablequeue_t *__queue_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_unsafegethead
 *
 *	\return true if the queue was not empty
 *	\see mutexgear_completion_queue_unsafegethead
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_cancelablequeue_unsafegethead(mutexgear_completion_item_t **__out_head_item,
	mutexgear_completion_cancelablequeue_t *__queue_instance);

/**
 *	\fn bool mutexgear_completion_cancelablequeue_unsafegetnext(mutexgear_completion_item_t **__out_next_item, mutexgear_completion_cancelablequeue_t *__queue_instance, const mutexgear_completion_item_t *__item_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_unsafegetnext
 *
 *	\return true if the next item existed
 *	\see mutexgear_completion_queue_unsafegetnext
 */
_MUTEXGEAR_PURE_INLINE bool mutexgear_completion_cancelablequeue_unsafegetnext(mutexgear_completion_item_t **__out_next_item,
	mutexgear_completion_cancelablequeue_t *__queue_instance, const mutexgear_completion_item_t *__item_instance);

/**
 *	\fn mutexgear_completion_item_t *mutexgear_completion_cancelablequeue_unsafegetunsafenext(const mutexgear_completion_item_t *__item_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_unsafegetunsafenext
 *
 *	\return The next item for the parameter
 *	\see mutexgear_completion_queue_unsafegetunsafenext
 */
_MUTEXGEAR_PURE_INLINE mutexgear_completion_item_t *mutexgear_completion_cancelablequeue_unsafegetunsafenext(const mutexgear_completion_item_t *__item_instance);

/**
 *	\fn mutexgear_completion_item_t *mutexgear_completion_cancelablequeue_getend(mutexgear_completion_cancelablequeue_t *__queue_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_getend
 *
 *	\return The queue forward direction iteration end element
 *	\see mutexgear_completion_queue_getend
 */
_MUTEXGEAR_PURE_INLINE mutexgear_completion_item_t *mutexgear_completion_cancelablequeue_getend(mutexgear_completion_cancelablequeue_t *__queue_instance);

/**
 *	\fn int mutexgear_completion_cancelablequeue_enqueue(mutexgear_completion_cancelablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_locktoken_t __lock_hint)
 *	\brief An inherited method for \c mutexgear_completion_queue_enqueue
 *
 *	The item inserted does not necessary need to be pre-started and can also be started later with \c mutexgear_completion_cancelablequeueditem_start.
 *
 *	\return EOK on success or a system error code on failure
 *	\see mutexgear_completion_queue_enqueue
 *	\see mutexgear_completion_cancelablequeueditem_start
 */
_MUTEXGEAR_API int mutexgear_completion_cancelablequeue_enqueue(mutexgear_completion_cancelablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance,
	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/);

/**
 *	\fn void mutexgear_completion_cancelablequeue_unsafedequeue(mutexgear_completion_item_t *__item_instance)
 *	\brief An inherited method for \c mutexgear_completion_queue_unsafedequeue
 *
 *	\see mutexgear_completion_queue_unsafedequeue
 */
_MUTEXGEAR_API void mutexgear_completion_cancelablequeue_unsafedequeue(mutexgear_completion_item_t *__item_instance);

// /**
//  *	\fn int mutexgear_completion_cancelablequeue_locateandstart(mutexgear_completion_item_t **__out_acquired_item, mutexgear_completion_cancelablequeue_t *__queue_instance, mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_locktoken_t __lock_hint)
//  *	\brief Retrieve a queued Item assigning it Worker to mark the former as being handled
//  *
//  *	The function scans the Queue for an Item without a Worker assigned and occupies and returns one if found.
//  *	The Item remains in the queue until being finished with a subsequent call to \c mutexgear_completion_cancelablequeueditem_safefinish
//  *	to indicate that the handling was completed.
//  *
//  *	If the queue is locked, the \c __lock_hint must the corresponding lock token. Otherwise, NULL is to be passed.
//  *
//  *	\param __out_acquired_item Pointer to a variable to receive the Item retrieved or NULL if there were no available Items in the Queue
//  *	\return EOK on success or a system error code on failure
//  *	\see mutexgear_completion_cancelablequeue_enqueue
//  *	\see mutexgear_completion_cancelablequeueditem_iscanceled
//  *	\see mutexgear_completion_cancelablequeueditem_safefinish
//  */
// _MUTEXGEAR_API int mutexgear_completion_cancelablequeue_locateandstart(mutexgear_completion_item_t **__out_acquired_item,
// 	mutexgear_completion_cancelablequeue_t *__queue_instance, mutexgear_completion_worker_t *__worker_instance,
// 	mutexgear_completion_locktoken_t __lock_hint/*=NULL*/);


/**
 *	\fn void mutexgear_completion_cancelablequeueditem_start(mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
 *	\brief An inherited method for \c mutexgear_completion_queueditem_start
 *
 *	The function can be called only while the queue is locked.
 *
 *	The function is implemented as an inline call.
 *	\see mutexgear_completion_item_isstarted
 *	\see mutexgear_completion_cancelablequeueditem_safefinish
 */
_MUTEXGEAR_PURE_INLINE void mutexgear_completion_cancelablequeueditem_start(mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance);

/**
 *	\fn bool mutexgear_completion_cancelablequeueditem_iscanceled(const mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
 *	\brief Check if Item was requested to be canceled
 *
 *	The function is to be called periodically by Workers after they start their work on items with \c mutexgear_completion_cancelablequeueditem_start
 *	to check whether Waiters have not requested the work to be aborted. If so, the Workers are supposed to skip 
 *	any work remainders on such the Items and finish them with \c mutexgear_completion_cancelablequeueditem_safefinish ASAP.
 *
 *	The function is OK to be called regardless of the respective Queue lock state.
 *
 *	\param __worker_instance The Worker being handling the Item (self)
 *	\return EOK on success or a system error code on failure
 *	\see mutexgear_completion_cancelablequeueditem_start
 *	\see mutexgear_completion_cancelablequeueditem_safefinish
 */
_MUTEXGEAR_API bool mutexgear_completion_cancelablequeueditem_iscanceled(const mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance);

/**
 *	\fn int mutexgear_completion_cancelablequeueditem_safefinish(mutexgear_completion_cancelablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
 *	\brief An inherited method for \c mutexgear_completion_queueditem_safefinish
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_completion_cancelablequeueditem_start
 *	\see mutexgear_completion_item_isstarted
 *	\see mutexgear_completion_queueditem_safefinish
 *	\see mutexgear_completion_cancelablequeueditem_unsafefinish__locked
 *	\see mutexgear_completion_cancelablequeueditem_unsafefinish__unlocked
 */
_MUTEXGEAR_API int mutexgear_completion_cancelablequeueditem_safefinish(mutexgear_completion_cancelablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance);

/**
 *	\fn void mutexgear_completion_cancelablequeueditem_unsafefinish__locked(mutexgear_completion_item_t *__item_instance)
 *	\brief An inherited method for \c mutexgear_completion_queueditem_unsafefinish__locked
 *
 *	\see mutexgear_completion_cancelablequeueditem_safefinish
 *	\see mutexgear_completion_queueditem_unsafefinish__locked
 *	\see mutexgear_completion_cancelablequeueditem_unsafefinish__unlocked
 */
_MUTEXGEAR_API void mutexgear_completion_cancelablequeueditem_unsafefinish__locked(mutexgear_completion_item_t *__item_instance);

/**
 *	\fn void mutexgear_completion_cancelablequeueditem_unsafefinish__unlocked(mutexgear_completion_cancelablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
 *	\brief An inherited method for \c mutexgear_completion_queueditem_unsafefinish__unlocked
 *
 *	\see mutexgear_completion_cancelablequeueditem_safefinish
 *	\see mutexgear_completion_queueditem_unsafefinish__unlocked
 *	\see mutexgear_completion_cancelablequeueditem_unsafefinish__locked
 */
_MUTEXGEAR_API void mutexgear_completion_cancelablequeueditem_unsafefinish__unlocked(mutexgear_completion_cancelablequeue_t *__queue_instance, mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance);


//////////////////////////////////////////////////////////////////////////
// Completion Queue Inline Method Implementations

_MUTEXGEAR_PURE_INLINE
mutexgear_completion_item_t *_mutexgear_completion_item_getfromworkitem(const mutexgear_dlraitem_t *__work_item)
{
	MG_ASSERT(__work_item != NULL);

	MG_DECLARE_TYPE_CAST(mutexgear_completion_item_t);
	return MG_PERFORM_TYPE_CAST((uint8_t *)__work_item - offsetof(mutexgear_completion_item_t, data.work_item));
}


_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_item_constructwow(mutexgear_completion_item_t *__item_instance, void *__worker_or_waiter)
{
	_mg_atomic_construct_ptrdiff(_MG_PA_PTRDIFF(&__item_instance->p_worker_or_waiter), (uint8_t *)__worker_or_waiter - (uint8_t *)__item_instance);
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_item_destroywow(mutexgear_completion_item_t *__item_instance)
{
	_mg_atomic_destroy_ptrdiff(_MG_PA_PTRDIFF(&__item_instance->p_worker_or_waiter));
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_item_unsafesetwow(mutexgear_completion_item_t *__item_instance, void *__worker_or_waiter)
{
	_mg_atomic_reinit_ptrdiff(_MG_PVA_PTRDIFF(&__item_instance->p_worker_or_waiter), (uint8_t *)__worker_or_waiter - (uint8_t *)__item_instance);
}

_MUTEXGEAR_PURE_INLINE
void *_mutexgear_completion_item_getwow(const mutexgear_completion_item_t *__item_instance)
{
	return (uint8_t *)__item_instance + _mg_atomic_load_relaxed_ptrdiff(_MG_PCVA_PTRDIFF(&__item_instance->p_worker_or_waiter));
}


_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_itemdata_constructextra(_mutexgear_completion_itemdata_t *__data_instance, _mutexgear_completion_item_extradata_t __extra_value)
{
	_mg_atomic_construct_completion_item_extradata(_MG_PA_COMPLETION_ITEM_EXTRADATA(&__data_instance->extra_data), __extra_value);
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_itemdata_destroyextra(_mutexgear_completion_itemdata_t *__data_instance)
{
	_mg_atomic_destroy_completion_item_extradata(_MG_PA_COMPLETION_ITEM_EXTRADATA(&__data_instance->extra_data));
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_itemdata_assignextrabit(_mutexgear_completion_itemdata_t *__data_instance, unsigned int __bit_index, bool __bit_value)
{
	MG_ASSERT(__bit_index < MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT);

	if (__bit_value)
	{
		_mg_atomic_unsafeor_relaxed_completion_item_extradata(_MG_PVA_COMPLETION_ITEM_EXTRADATA(&__data_instance->extra_data), (_mutexgear_completion_item_extradata_t)1 << __bit_index);
	}
	else
	{
		_mg_atomic_unsafeand_relaxed_completion_item_extradata(_MG_PVA_COMPLETION_ITEM_EXTRADATA(&__data_instance->extra_data), ~((_mutexgear_completion_item_extradata_t)1 << __bit_index));
	}
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_itemdata_assignunsafeextrabit(_mutexgear_completion_itemdata_t *__data_instance, unsigned int __bit_index, bool __bit_value)
{
	MG_ASSERT(__bit_index < MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT);

	if (__bit_value)
	{
		_mg_atomic_reinit_completion_item_extradata(_MG_PVA_COMPLETION_ITEM_EXTRADATA(&__data_instance->extra_data), __data_instance->extra_data | ((_mutexgear_completion_item_extradata_t)1 << __bit_index));
	}
	else
	{
		_mg_atomic_reinit_completion_item_extradata(_MG_PVA_COMPLETION_ITEM_EXTRADATA(&__data_instance->extra_data), __data_instance->extra_data & ~((_mutexgear_completion_item_extradata_t)1 << __bit_index));
	}
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_itemdata_assignunsafeallextrabits(_mutexgear_completion_itemdata_t *__data_instance, unsigned int __bit_count, bool __bit_value)
{
	MG_ASSERT(__bit_count != 0);

	if (__bit_value)
	{
		const _mutexgear_completion_item_extradata_t change_bitmask = (((((_mutexgear_completion_item_extradata_t)1 << ((__bit_count) - 1)) - 1) << 1) | 1);
		_mg_atomic_reinit_completion_item_extradata(_MG_PVA_COMPLETION_ITEM_EXTRADATA(&__data_instance->extra_data), __data_instance->extra_data | change_bitmask);
	}
	else
	{
		const _mutexgear_completion_item_extradata_t change_bitmask = (((((_mutexgear_completion_item_extradata_t)1 << ((__bit_count) - 1)) - 1) << 1) | 1);
		_mg_atomic_reinit_completion_item_extradata(_MG_PVA_COMPLETION_ITEM_EXTRADATA(&__data_instance->extra_data), __data_instance->extra_data & ~change_bitmask);
	}
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_itemdata_unsafemodifyextrabit(_mutexgear_completion_itemdata_t *__data_instance, unsigned int __bit_index, bool __bit_value)
{
	MG_ASSERT(__bit_index < MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT);

	bool ret;

	if (__bit_value)
	{
		ret = (_mg_atomic_load_relaxed_completion_item_extradata(_MG_PCVA_COMPLETION_ITEM_EXTRADATA(&__data_instance->extra_data)) & ((_mutexgear_completion_item_extradata_t)1 << __bit_index)) == 0
			&& (_mg_atomic_unsafeor_relaxed_completion_item_extradata(_MG_PVA_COMPLETION_ITEM_EXTRADATA(&__data_instance->extra_data), (_mutexgear_completion_item_extradata_t)1 << __bit_index), true);
	}
	else
	{
		ret = (_mg_atomic_load_relaxed_completion_item_extradata(_MG_PCVA_COMPLETION_ITEM_EXTRADATA(&__data_instance->extra_data)) & ((_mutexgear_completion_item_extradata_t)1 << __bit_index)) != 0
			&& (_mg_atomic_unsafeand_relaxed_completion_item_extradata(_MG_PVA_COMPLETION_ITEM_EXTRADATA(&__data_instance->extra_data), ~((_mutexgear_completion_item_extradata_t)1 << __bit_index)), true);
	}

	return ret;
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_itemdata_modifyunsafeextrabit(_mutexgear_completion_itemdata_t *__data_instance, unsigned int __bit_index, bool __bit_value)
{
	MG_ASSERT(__bit_index < MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT);

	bool ret;

	if (__bit_value)
	{
		ret = (__data_instance->extra_data & ((_mutexgear_completion_item_extradata_t)1 << __bit_index)) == 0
			&& (_mg_atomic_reinit_completion_item_extradata(_MG_PVA_COMPLETION_ITEM_EXTRADATA(&__data_instance->extra_data), __data_instance->extra_data | ((_mutexgear_completion_item_extradata_t)1 << __bit_index)), true);
	}
	else
	{
		ret = (__data_instance->extra_data & ((_mutexgear_completion_item_extradata_t)1 << __bit_index)) != 0
			&& (_mg_atomic_reinit_completion_item_extradata(_MG_PVA_COMPLETION_ITEM_EXTRADATA(&__data_instance->extra_data), __data_instance->extra_data & ~((_mutexgear_completion_item_extradata_t)1 << __bit_index)), true);
	}

	return ret;
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_itemdata_testextrabit(const _mutexgear_completion_itemdata_t *__data_instance, unsigned int __bit_index)
{
	bool ret = (_mg_atomic_load_relaxed_completion_item_extradata(_MG_PCVA_COMPLETION_ITEM_EXTRADATA(&__data_instance->extra_data)) & ((_mutexgear_completion_item_extradata_t)1 << __bit_index)) != 0;
	return ret;
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_itemdata_testanyextrabits(const _mutexgear_completion_itemdata_t *__data_instance, unsigned int __bit_count)
{
	MG_ASSERT(__bit_count != 0);

	const _mutexgear_completion_item_extradata_t test_bitmask = (((((_mutexgear_completion_item_extradata_t)1 << ((__bit_count) - 1)) - 1) << 1) | 1);
	bool ret = (_mg_atomic_load_relaxed_completion_item_extradata(_MG_PCVA_COMPLETION_ITEM_EXTRADATA(&__data_instance->extra_data)) & test_bitmask) != 0;
	return ret;
}


_MUTEXGEAR_PURE_INLINE void _mutexgear_completion_itemdata_init(_mutexgear_completion_itemdata_t *__data_instance);
_MUTEXGEAR_PURE_INLINE void _mutexgear_completion_itemdata_destroy(_mutexgear_completion_itemdata_t *__data_instance);

_MUTEXGEAR_PURE_INLINE
void mutexgear_completion_item_init(mutexgear_completion_item_t *__item_instance)
{
	_mutexgear_completion_itemdata_init(&__item_instance->data);
	_mutexgear_completion_item_constructwow(__item_instance, __item_instance); // = NULL
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_itemdata_init(_mutexgear_completion_itemdata_t *__data_instance)
{
	mutexgear_dlraitem_init(&__data_instance->work_item);
	_mutexgear_completion_itemdata_constructextra(__data_instance, 0); // NOTE: Extra Data is initialized for client's convenience
}

_MUTEXGEAR_PURE_INLINE
void mutexgear_completion_item_reinit(mutexgear_completion_item_t *__item_instance)
{
	MG_ASSERT(!mutexgear_dlraitem_islinked(&__item_instance->data.work_item));

	_mutexgear_completion_item_unsafesetwow(__item_instance, __item_instance); // = NULL
	// NOTE: Extra Data is kept for client code and is not manager by the Item
}

_MUTEXGEAR_PURE_INLINE
bool mutexgear_completion_item_isasinit(const mutexgear_completion_item_t *__item_instance)
{
	bool ret = !mutexgear_dlraitem_islinked(&__item_instance->data.work_item) && _mutexgear_completion_item_getwow(__item_instance) == (void *)__item_instance
		/*&& NOTE: Extra Data is kept for client code and is not manager by the Item*/;
	return ret;
}

_MUTEXGEAR_PURE_INLINE
void mutexgear_completion_item_destroy(mutexgear_completion_item_t *__item_instance)
{
	_mutexgear_completion_item_destroywow(__item_instance);
	_mutexgear_completion_itemdata_destroy(&__item_instance->data);
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_itemdata_destroy(_mutexgear_completion_itemdata_t *__data_instance)
{
	_mutexgear_completion_itemdata_destroyextra(__data_instance);
	mutexgear_dlraitem_destroy(&__data_instance->work_item);
}


_MUTEXGEAR_PURE_INLINE void _mutexgear_completion_itemdata_settag(_mutexgear_completion_itemdata_t *__data_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value);
_MUTEXGEAR_PURE_INLINE void _mutexgear_completion_itemdata_setunsafetag(_mutexgear_completion_itemdata_t *__data_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value);
_MUTEXGEAR_PURE_INLINE void _mutexgear_completion_itemdata_setallunsafetags(_mutexgear_completion_itemdata_t *__data_instance, bool __tag_value);
_MUTEXGEAR_PURE_INLINE bool _mutexgear_completion_itemdata_unsafemodifytag(_mutexgear_completion_itemdata_t *__data_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value);
_MUTEXGEAR_PURE_INLINE bool _mutexgear_completion_itemdata_modifyunsafetag(_mutexgear_completion_itemdata_t *__data_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value);
_MUTEXGEAR_PURE_INLINE bool _mutexgear_completion_itemdata_gettag(const _mutexgear_completion_itemdata_t *__data_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/);
_MUTEXGEAR_PURE_INLINE bool _mutexgear_completion_itemdata_getanytags(const _mutexgear_completion_itemdata_t *__data_instance);

_MUTEXGEAR_PURE_INLINE
void mutexgear_completion_item_settag(mutexgear_completion_item_t *__item_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value)
{
	_mutexgear_completion_itemdata_settag(&__item_instance->data, __tag_index, __tag_value);
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_itemdata_settag(_mutexgear_completion_itemdata_t *__data_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value)
{
	_mutexgear_completion_itemdata_assignextrabit(__data_instance, __tag_index, __tag_value);
}

_MUTEXGEAR_PURE_INLINE
void mutexgear_completion_item_setunsafetag(mutexgear_completion_item_t *__item_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value)
{
	_mutexgear_completion_itemdata_setunsafetag(&__item_instance->data, __tag_index, __tag_value);
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_itemdata_setunsafetag(_mutexgear_completion_itemdata_t *__data_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value)
{
	_mutexgear_completion_itemdata_assignunsafeextrabit(__data_instance, __tag_index, __tag_value);
}

_MUTEXGEAR_PURE_INLINE
void mutexgear_completion_item_setallunsafetags(mutexgear_completion_item_t *__item_instance, bool __tag_value)
{
	_mutexgear_completion_itemdata_setallunsafetags(&__item_instance->data, __tag_value);
}

_MUTEXGEAR_PURE_INLINE
void _mutexgear_completion_itemdata_setallunsafetags(_mutexgear_completion_itemdata_t *__data_instance, bool __tag_value)
{
	_mutexgear_completion_itemdata_assignunsafeallextrabits(__data_instance, MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT, __tag_value);
}

_MUTEXGEAR_PURE_INLINE 
bool mutexgear_completion_item_unsafemodifytag(mutexgear_completion_item_t *__item_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value)
{
	return _mutexgear_completion_itemdata_unsafemodifytag(&__item_instance->data, __tag_index, __tag_value);
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_itemdata_unsafemodifytag(_mutexgear_completion_itemdata_t *__data_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value)
{
	return _mutexgear_completion_itemdata_unsafemodifyextrabit(__data_instance, __tag_index, __tag_value);
}

_MUTEXGEAR_PURE_INLINE
bool mutexgear_completion_item_modifyunsafetag(mutexgear_completion_item_t *__item_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value)
{
	return _mutexgear_completion_itemdata_modifyunsafetag(&__item_instance->data, __tag_index, __tag_value);
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_itemdata_modifyunsafetag(_mutexgear_completion_itemdata_t *__data_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/, bool __tag_value)
{
	return _mutexgear_completion_itemdata_modifyunsafeextrabit(__data_instance, __tag_index, __tag_value);
}

_MUTEXGEAR_PURE_INLINE
bool mutexgear_completion_item_gettag(const mutexgear_completion_item_t *__item_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/)
{
	return _mutexgear_completion_itemdata_gettag(&__item_instance->data, __tag_index);
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_itemdata_gettag(const _mutexgear_completion_itemdata_t *__data_instance, unsigned int __tag_index/*<MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT*/)
{
	return _mutexgear_completion_itemdata_testextrabit(__data_instance, __tag_index);
}

_MUTEXGEAR_PURE_INLINE
bool mutexgear_completion_item_getanytags(const mutexgear_completion_item_t *__item_instance)
{
	return _mutexgear_completion_itemdata_getanytags(&__item_instance->data);
}

_MUTEXGEAR_PURE_INLINE
bool _mutexgear_completion_itemdata_getanytags(const _mutexgear_completion_itemdata_t *__data_instance)
{
	return _mutexgear_completion_itemdata_testanyextrabits(__data_instance, MUTEXGEAR_COMPLETION_ITEM_TAGINDEX_COUNT);
}


_MUTEXGEAR_PURE_INLINE
void mutexgear_completion_item_prestart(mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
{
	MG_ASSERT(__worker_instance != NULL);
	MG_ASSERT(_mutexgear_completion_item_getwow(__item_instance) == (void *)__item_instance); // == NULL

	_mutexgear_completion_item_unsafesetwow(__item_instance, __worker_instance);
}

_MUTEXGEAR_PURE_INLINE
bool mutexgear_completion_item_isstarted(const mutexgear_completion_item_t *__item_instance)
{
	return _mutexgear_completion_item_getwow(__item_instance) != (void *)__item_instance; // != NULL
}

_MUTEXGEAR_PURE_INLINE
mutexgear_completion_worker_t *mutexgear_completion_item_getworker(const mutexgear_completion_item_t *__item_instance)
{
	void *worker_instance = _mutexgear_completion_item_getwow(__item_instance);
	return worker_instance != (void *)__item_instance/* != NULL */ ? (mutexgear_completion_worker_t *)worker_instance : NULL;
}


_MUTEXGEAR_PURE_INLINE
bool mutexgear_completion_queue_lodisempty(const mutexgear_completion_queue_t *__queue_instance)
{
	bool ret = mutexgear_dlralist_isempty(&__queue_instance->work_list);
	return ret;
}

_MUTEXGEAR_PURE_INLINE
bool mutexgear_completion_queue_gettail(mutexgear_completion_item_t **__out_tail_item,
	mutexgear_completion_queue_t *__queue_instance)
{
	MG_ASSERT(__queue_instance != NULL);

	mutexgear_dlraitem_t *last_work_item, *work_item_to_use = mutexgear_dlralist_getend(&__queue_instance->work_list);
	bool ret = mutexgear_dlralist_trygetprevious(&last_work_item, &__queue_instance->work_list, work_item_to_use);

	*__out_tail_item = _mutexgear_completion_item_getfromworkitem(last_work_item);
	return ret;
}

_MUTEXGEAR_PURE_INLINE
bool mutexgear_completion_queue_getpreceding(mutexgear_completion_item_t **__out_preceding_item,
	mutexgear_completion_queue_t *__queue_instance, const mutexgear_completion_item_t *__item_instance)
{
	MG_ASSERT(__queue_instance != NULL);
	MG_ASSERT(__item_instance != NULL);

	mutexgear_dlraitem_t *previous_work_item;
	const mutexgear_dlraitem_t *current_work_item = mutexgear_completion_item_getworkitem(__item_instance);
	bool ret = mutexgear_dlralist_trygetprevious(&previous_work_item, &__queue_instance->work_list, current_work_item);
	
	*__out_preceding_item = _mutexgear_completion_item_getfromworkitem(previous_work_item);
	return ret;
}

_MUTEXGEAR_PURE_INLINE
mutexgear_completion_item_t *mutexgear_completion_queue_getunsafepreceding(const mutexgear_completion_item_t *__item_instance)
{
	MG_ASSERT(__item_instance != NULL);

	const mutexgear_dlraitem_t *current_work_item = mutexgear_completion_item_getworkitem(__item_instance);
	mutexgear_dlraitem_t *previous_work_item = mutexgear_dlraitem_getprevious(current_work_item);
	return _mutexgear_completion_item_getfromworkitem(previous_work_item);
}

_MUTEXGEAR_PURE_INLINE
mutexgear_completion_item_t *mutexgear_completion_queue_getrend(mutexgear_completion_queue_t *__queue_instance)
{
	MG_ASSERT(__queue_instance != NULL);

	mutexgear_dlraitem_t *work_items_end = mutexgear_dlralist_getrend(&__queue_instance->work_list);
	return _mutexgear_completion_item_getfromworkitem(work_items_end);
}

_MUTEXGEAR_PURE_INLINE
bool mutexgear_completion_queue_unsafegethead(mutexgear_completion_item_t **__out_head_item,
	mutexgear_completion_queue_t *__queue_instance)
{
	MG_ASSERT(__queue_instance != NULL);

	mutexgear_dlraitem_t *first_work_item = mutexgear_dlralist_getbegin(&__queue_instance->work_list), *work_items_end = mutexgear_dlralist_getend(&__queue_instance->work_list);

	*__out_head_item = _mutexgear_completion_item_getfromworkitem(first_work_item);
	return first_work_item != work_items_end;
}

_MUTEXGEAR_PURE_INLINE 
mutexgear_completion_item_t *mutexgear_completion_queue_unsafegetunsafehead(
	mutexgear_completion_queue_t *__queue_instance)
{
	MG_ASSERT(__queue_instance != NULL);

	mutexgear_dlraitem_t *first_work_item = mutexgear_dlralist_getbegin(&__queue_instance->work_list);
	return _mutexgear_completion_item_getfromworkitem(first_work_item);
}

_MUTEXGEAR_PURE_INLINE
bool mutexgear_completion_queue_unsafegetnext(mutexgear_completion_item_t **__out_next_item,
	mutexgear_completion_queue_t *__queue_instance, const mutexgear_completion_item_t *__item_instance)
{
	MG_ASSERT(__queue_instance != NULL);
	MG_ASSERT(__item_instance != NULL);

	const mutexgear_dlraitem_t *current_work_item = mutexgear_completion_item_getworkitem(__item_instance);
	mutexgear_dlraitem_t *next_work_item = mutexgear_dlraitem_getnext(current_work_item), *work_items_end = mutexgear_dlralist_getend(&__queue_instance->work_list);
	
	*__out_next_item = _mutexgear_completion_item_getfromworkitem(next_work_item);
	return next_work_item != work_items_end;
}

_MUTEXGEAR_PURE_INLINE
mutexgear_completion_item_t *mutexgear_completion_queue_unsafegetunsafenext(const mutexgear_completion_item_t *__item_instance)
{
	MG_ASSERT(__item_instance != NULL);

	const mutexgear_dlraitem_t *current_work_item = mutexgear_completion_item_getworkitem(__item_instance);
	mutexgear_dlraitem_t *next_work_item = mutexgear_dlraitem_getnext(current_work_item);
	return _mutexgear_completion_item_getfromworkitem(next_work_item);
}

_MUTEXGEAR_PURE_INLINE 
mutexgear_completion_item_t *mutexgear_completion_queue_getend(mutexgear_completion_queue_t *__queue_instance)
{
	MG_ASSERT(__queue_instance != NULL);

	mutexgear_dlraitem_t *work_items_end = mutexgear_dlralist_getend(&__queue_instance->work_list);
	return _mutexgear_completion_item_getfromworkitem(work_items_end);
}


_MUTEXGEAR_PURE_INLINE 
void mutexgear_completion_queueditem_start(mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
{
	MG_ASSERT(__worker_instance != NULL);
	MG_ASSERT(_mutexgear_completion_item_getwow(__item_instance) == (void *)__item_instance); // == NULL

	_mutexgear_completion_item_unsafesetwow(__item_instance, __worker_instance);
}


//////////////////////////////////////////////////////////////////////////
// Completion DrainableQueue Inline Method Implementations

_MUTEXGEAR_PURE_INLINE
bool mutexgear_completion_drainablequeue_lodisempty(const mutexgear_completion_drainablequeue_t *__queue_instance)
{
	bool ret = mutexgear_completion_queue_lodisempty(&__queue_instance->basic_queue);
	return ret;
}

_MUTEXGEAR_PURE_INLINE
bool mutexgear_completion_drainablequeue_gettail(mutexgear_completion_item_t **__out_tail_item,
	mutexgear_completion_drainablequeue_t *__queue_instance)
{
	bool ret = mutexgear_completion_queue_gettail(__out_tail_item, &__queue_instance->basic_queue);
	return ret;
}

_MUTEXGEAR_PURE_INLINE
bool mutexgear_completion_drainablequeue_getpreceding(mutexgear_completion_item_t **__out_preceding_item,
	mutexgear_completion_drainablequeue_t *__queue_instance, const mutexgear_completion_item_t *__item_instance)
{
	bool ret = mutexgear_completion_queue_getpreceding(__out_preceding_item, &__queue_instance->basic_queue, __item_instance);
	return ret;
}

_MUTEXGEAR_PURE_INLINE
mutexgear_completion_item_t *mutexgear_completion_drainablequeue_getunsafepreceding(const mutexgear_completion_item_t *__item_instance)
{
	mutexgear_completion_item_t *preceding_basic_item = mutexgear_completion_queue_getunsafepreceding(__item_instance);
	return preceding_basic_item;
}

_MUTEXGEAR_PURE_INLINE
mutexgear_completion_item_t *mutexgear_completion_drainablequeue_getrend(mutexgear_completion_drainablequeue_t *__queue_instance)
{
	MG_ASSERT(__queue_instance != NULL);

	mutexgear_completion_item_t *end_basic_item = mutexgear_completion_queue_getrend(&__queue_instance->basic_queue);
	return end_basic_item;
}

_MUTEXGEAR_PURE_INLINE
bool mutexgear_completion_drainablequeue_unsafegethead(mutexgear_completion_item_t **__out_head_item,
	mutexgear_completion_drainablequeue_t *__queue_instance)
{
	bool ret = mutexgear_completion_queue_unsafegethead(__out_head_item, &__queue_instance->basic_queue);
	return ret;
}

_MUTEXGEAR_PURE_INLINE 
bool mutexgear_completion_drainablequeue_unsafegetnext(mutexgear_completion_item_t **__out_next_item,
	mutexgear_completion_drainablequeue_t *__queue_instance, const mutexgear_completion_item_t *__item_instance)
{
	bool ret = mutexgear_completion_queue_unsafegetnext(__out_next_item, &__queue_instance->basic_queue, __item_instance);
	return ret;
}

_MUTEXGEAR_PURE_INLINE mutexgear_completion_item_t *mutexgear_completion_drainablequeue_unsafegetunsafenext(const mutexgear_completion_item_t *__item_instance)
{
	mutexgear_completion_item_t *next_basic_item = mutexgear_completion_queue_unsafegetunsafenext(__item_instance);
	return next_basic_item;
}

_MUTEXGEAR_PURE_INLINE
mutexgear_completion_item_t *mutexgear_completion_drainablequeue_getend(mutexgear_completion_drainablequeue_t *__queue_instance)
{
	MG_ASSERT(__queue_instance != NULL);

	mutexgear_completion_item_t *end_basic_item = mutexgear_completion_queue_getend(&__queue_instance->basic_queue);
	return end_basic_item;
}


//////////////////////////////////////////////////////////////////////////
// Completion CancelableQueue Inline Method Implementations

_MUTEXGEAR_PURE_INLINE
bool mutexgear_completion_cancelablequeue_lodisempty(const mutexgear_completion_cancelablequeue_t *__queue_instance)
{
	bool ret = mutexgear_completion_queue_lodisempty(&__queue_instance->basic_queue);
	return ret;
}

_MUTEXGEAR_PURE_INLINE
bool mutexgear_completion_cancelablequeue_gettail(mutexgear_completion_item_t **__out_tail_item,
	mutexgear_completion_cancelablequeue_t *__queue_instance)
{
	bool ret = mutexgear_completion_queue_gettail(__out_tail_item, &__queue_instance->basic_queue);
	return ret;
}

_MUTEXGEAR_PURE_INLINE
bool mutexgear_completion_cancelablequeue_getpreceding(mutexgear_completion_item_t **__out_preceding_item,
	mutexgear_completion_cancelablequeue_t *__queue_instance, const mutexgear_completion_item_t *__item_instance)
{
	bool ret = mutexgear_completion_queue_getpreceding(__out_preceding_item, &__queue_instance->basic_queue, __item_instance);
	return ret;
}

_MUTEXGEAR_PURE_INLINE
mutexgear_completion_item_t *mutexgear_completion_cancelablequeue_getunsafepreceding(const mutexgear_completion_item_t *__item_instance)
{
	mutexgear_completion_item_t *preceding_basic_item = mutexgear_completion_queue_getunsafepreceding(__item_instance);
	return preceding_basic_item;
}

_MUTEXGEAR_PURE_INLINE
mutexgear_completion_item_t *mutexgear_completion_cancelablequeue_getrend(mutexgear_completion_cancelablequeue_t *__queue_instance)
{
	MG_ASSERT(__queue_instance != NULL);

	mutexgear_completion_item_t *rend_basic_item = mutexgear_completion_queue_getrend(&__queue_instance->basic_queue);
	return rend_basic_item;
}

_MUTEXGEAR_PURE_INLINE
bool mutexgear_completion_cancelablequeue_unsafegethead(mutexgear_completion_item_t **__out_head_item,
	mutexgear_completion_cancelablequeue_t *__queue_instance)
{
	bool ret = mutexgear_completion_queue_unsafegethead(__out_head_item, &__queue_instance->basic_queue);
	return ret;
}

_MUTEXGEAR_PURE_INLINE
bool mutexgear_completion_cancelablequeue_unsafegetnext(mutexgear_completion_item_t **__out_next_item,
	mutexgear_completion_cancelablequeue_t *__queue_instance, const mutexgear_completion_item_t *__item_instance)
{
	bool ret = mutexgear_completion_queue_unsafegetnext(__out_next_item, &__queue_instance->basic_queue, __item_instance);
	return ret;
}

_MUTEXGEAR_PURE_INLINE 
mutexgear_completion_item_t *mutexgear_completion_cancelablequeue_unsafegetunsafenext(const mutexgear_completion_item_t *__item_instance)
{
	mutexgear_completion_item_t *next_basic_item = mutexgear_completion_queue_unsafegetunsafenext(__item_instance);
	return next_basic_item;
}

_MUTEXGEAR_PURE_INLINE
mutexgear_completion_item_t *mutexgear_completion_cancelablequeue_getend(mutexgear_completion_cancelablequeue_t *__queue_instance)
{
	MG_ASSERT(__queue_instance != NULL);

	mutexgear_completion_item_t *end_basic_item = mutexgear_completion_queue_getend(&__queue_instance->basic_queue);
	return end_basic_item;
}


_MUTEXGEAR_PURE_INLINE 
void mutexgear_completion_cancelablequeueditem_start(mutexgear_completion_item_t *__item_instance, mutexgear_completion_worker_t *__worker_instance)
{
	mutexgear_completion_queueditem_start(__item_instance, __worker_instance);
}


_MUTEXGEAR_END_EXTERN_C();


#endif // #ifndef __MUTEXGEAR_COMPLETION_H_INCLUDED
