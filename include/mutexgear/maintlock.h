#ifndef __MUTEXGEAR_MAINTLOCK_H_INCLUDED
#define __MUTEXGEAR_MAINTLOCK_H_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear MaintLock API Definitions                                  */
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
*	\brief MutexGear MaintLock API definitions
*
*	The header defines "maintenance lock" (\c maintlock) object.
* 
*	A maintenance lock is an object that can be used to manage multi-threaded access
*	to a resource that needs to be periodicallly locked down for maintenance. 
*	It allows resource user threads to try-acquire read locks for accessing the resource
*	(the acquisition fails if the resource is switched to maintenance mode) and then release 
*	their read locks after they finished the access.
* 
*	At the same time, a thread that needs to modify the resource can switch it to maintenance mode, 
*	preventing new read locks, and wait untils all the existing read locks are released 
*	to proceed with its changes; then clear the maintenance flag to allow further access.
*
*	Alternatively, the modification thread can skip altering object's operation mode, 
*	use atomic exchange to remove or replace the resource and then wait 
*	for the read locks acquired before the call to be released (to make sure 
*	that threads that might have seen the old resource stopped accessing it),
*	at the same time, allowing new user threads to acquire new read locks 
*	and continue accessing the new resource state without delays.
* 
*	Since the implementation relies on muteces only, the wait operation is going
*	to provide priority inheritance (if supported by the operating system), 
*	one by one, to the user threads while the modification thread will be waiting 
*	for read locks to be released.
*
*	NOTE:
*
*	The \c mutexgear_maintlock_t object depends on a synchronization
*	mechanism being a subject of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and
*	Image Database search (currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm)
*	to view the patent text.
*/


#include <mutexgear/config.h>
#include <mutexgear/completion.h>
#include <mutexgear/constants.h>


_MUTEXGEAR_BEGIN_EXTERN_C();


//////////////////////////////////////////////////////////////////////////
// mutexgear_maintlockattr_t 

/**
*	\struct mutexgear_maintlockattr_t
*	\brief An opaque attribute structure to be used for \c mutexgear_maintlock_t initialization.
*/
typedef struct _mutexgear_maintlockattr
{
	_MUTEXGEAR_LOCKATTR_T	lock_attr;
	unsigned int			mode_flags;

} mutexgear_maintlockattr_t;


/**
*	\fn int mutexgear_maintlockattr_init(mutexgear_maintlockattr_t *__attr_instance)
*	\brief A function to initialize a \c mutexgear_maintlockattr_t instance.
*
*	The attributes are initialized with defaults that are counterparts of the underlying
*	system structure defaults. Typically these are:
*	\li MUTEXGEAR_PROCESS_PRIVATE for "pshared";
*	\li MUTEXGEAR_PRIO_INHERIT for "protocol".
*	\return EOK on success or a system error code on failure.
*/
_MUTEXGEAR_API int mutexgear_maintlockattr_init(mutexgear_maintlockattr_t *__attr_instance);

/**
*	\fn int mutexgear_maintlockattr_destroy(mutexgear_maintlockattr_t *__attr_instance)
*	\brief A function to destroy a previously initialized \c mutexgear_maintlockattr_t
*	instance and free any resources possibly allocated for it.
*	\return EOK on success or a system error code on failure.
*/
_MUTEXGEAR_API int mutexgear_maintlockattr_destroy(mutexgear_maintlockattr_t *__attr_instance);


/**
*	\fn int mutexgear_maintlockattr_setpshared(mutexgear_maintlockattr_t *__attr_instance, int __pshared_value)
*	\brief A function to assign process shared attribute value to
*	a \c mutexgear_maintlockattr_t  structure (similar to \c the pthread_mutexattr_setpshared).
*	\param __pshared_value one of \c MUTEXGEAR_PROCESS_PRIVATE or \c MUTEXGEAR_PROCESS_SHARED
*	\return EOK on success or a system error code on failure.
*/
_MUTEXGEAR_API int mutexgear_maintlockattr_setpshared(mutexgear_maintlockattr_t *__attr_instance, int __pshared_value);

/**
*	\fn int mutexgear_maintlockattr_getpshared(const mutexgear_maintlockattr_t *__attr_instance, int *__out_pshared_value)
*	\brief A function to get process shared attribute value stored in
*	a \c mutexgear_maintlockattr_t  structure (similar to the \c pthread_mutexattr_getpshared).
*	\param __out_pshared_value pointer to a variable to receive process shared attribute value stored in the structure (either \c MUTEXGEAR_PROCESS_PRIVATE or \c MUTEXGEAR_PROCESS_SHARED)
*	\return EOK on success or a system error code on failure.
*/
_MUTEXGEAR_API int mutexgear_maintlockattr_getpshared(const mutexgear_maintlockattr_t *__attr_instance, int *__out_pshared_value);


/**
*	\fn int mutexgear_maintlockattr_setprioceiling(mutexgear_maintlockattr_t *__attr_instance, int __prioceiling_value)
*	\brief A function to assign a priority ceiling value to
*	a \c mutexgear_maintlockattr_t  structure (similar to the \c pthread_mutexattr_setprioceiling).
*	\param __prioceiling_value a priority value to be used as the ceiling
*	\return EOK on success or a system error code on failure.
*/
_MUTEXGEAR_API int mutexgear_maintlockattr_setprioceiling(mutexgear_maintlockattr_t *__attr_instance, int __prioceiling_value);

/**
*	\fn int mutexgear_maintlockattr_setprotocol(mutexgear_maintlockattr_t *__attr_instance, int __protocol_value)
*	\brief A function to set a lock protocol value to
*	a \c mutexgear_maintlockattr_t  structure (similar to the \c pthread_mutexattr_setprotocol).
*	\param __protocol_value one of MUTEXGEAR_PRIO_INHERIT, MUTEXGEAR_PRIO_PROTECT, MUTEXGEAR_PRIO_NONE
*	\return EOK on success or a system error code on failure.
*/
_MUTEXGEAR_API int mutexgear_maintlockattr_setprotocol(mutexgear_maintlockattr_t *__attr_instance, int __protocol_value);

/**
*	\fn int mutexgear_maintlockattr_getprioceiling(const mutexgear_maintlockattr_t *__attr_instance, int *__out_prioceiling_value)
*	\brief A function to query priority ceiling attribute value
*	stored in a \c mutexgear_maintlockattr_t  structure (similar to the \c pthread_mutexattr_getprioceiling).
*	\param __out_prioceiling_value pointer to a variable to receive priority ceiling attribute value stored in the structure (either a positive value or -1 if priority ceiling is not supported by the OS)
*	\return EOK on success or a system error code on failure.
*/
_MUTEXGEAR_API int mutexgear_maintlockattr_getprioceiling(const mutexgear_maintlockattr_t *__attr_instance, int *__out_prioceiling_value);

/**
*	\fn int mutexgear_maintlockattr_getprotocol(const mutexgear_maintlockattr_t *__attr_instance, int *__out_protocol_value)
*	\brief A function to retrieve lock protocol attribute value
*	stored in a \c mutexgear_maintlockattr_t  structure (similar to the \c pthread_mutexattr_getprotocol).
*	\param __out_protocol_value pointer to a variable to receive priority inheritance protocol attribute value stored in the structure (MUTEXGEAR_PRIO_INHERIT, MUTEXGEAR_PRIO_PROTECT, MUTEXGEAR_PRIO_NONE; or MUTEXGEAR__INVALID_PROTOCOL if the feature is not supported by the OS)
*	\return EOK on success or a system error code on failure.
*/
_MUTEXGEAR_API int mutexgear_maintlockattr_getprotocol(const mutexgear_maintlockattr_t *__attr_instance, int *__out_protocol_value);


/**
*	\fn int mutexgear_maintlockattr_setmutexattr(mutexgear_maintlockattr_t *__attr_instance, const _MUTEXGEAR_LOCKATTR_T *__mutexattr_instance)
*	\brief A function to copy attributes from a mutex attributes structure to
*	the \c mutexgear_maintlockattr_t  structure.
*
*	Values of "pshared", "protocol" and "prioceiling" are copied.
*
*	Due to inability to clear previously assigned priority ceiling setting from attributes on some targets
*	it's recommended to call this function with freshly initialized \a __attr_instance only.
*	\return EOK on success or a system error code on failure.
*/
_MUTEXGEAR_API int mutexgear_maintlockattr_setmutexattr(mutexgear_maintlockattr_t *__attr_instance, const _MUTEXGEAR_LOCKATTR_T *__mutexattr_instance);


//////////////////////////////////////////////////////////////////////////

/**
*	\struct mutexgear_maintlock_t
*	\brief An opaque structure to represent a maintenance lock (\c maintlock) object
*
*/
typedef struct _mutexgear_maintlock
{
	union
	{
		ptrdiff_t                   mode_and_lock_flags;
		size_t                      _reserved1; // For alignment of the subsequent fields

	} fl_un;

	mutexgear_completion_drainablequeue_t acquired_reads;
	mutexgear_completion_queue_t awaited_reads;

} mutexgear_maintlock_t;

typedef mutexgear_completion_drainidx_t mutexgear_maintlock_rdlock_token_t;


/**
*	\fn int mutexgear_maintlock_init(mutexgear_maintlock_t *__maintlock_instance, mutexgear_maintlockattr_t *__attr_instance=NULL)
*	\brief Creates a \c maintlock object
*
*	The object is initialized accordingly to the \p __attr_instance values.
*	\param __attr_instance An initialized attributes structure or NULL to use defaults
*	\return EOK on success or a system error code on failure.
*	\see mutexgear_maintlock_destroy
*/
_MUTEXGEAR_API int mutexgear_maintlock_init(mutexgear_maintlock_t *__maintlock_instance, mutexgear_maintlockattr_t *__attr_instance/*=NULL*/);


/**
*	\fn int mutexgear_maintlock_destroy(mutexgear_maintlock_t *__maintlock_instance)
*	\brief Destroys the \c maintlock object
*
*	The object must be free of locks to be destroyed successfully.
*	\return EOK on success or a system error code on failure.
*	\see mutexgear_maintlock_init
*/
_MUTEXGEAR_API int mutexgear_maintlock_destroy(mutexgear_maintlock_t *__maintlock_instance);


/**
*	\fn int mutexgear_maintlock_set_maintenance(mutexgear_maintlock_t *__maintlock_instance)
*	\brief Switches \c maintlock object to maintenance mode preventing further \c mutexgear_maintlock_tryrdlock calls from succeeding.
* 
*	The functions is not counting. If two calls to the function are made in a row 
*	the second call will fail with EBUSY. Also, the function uses relaxed memory store operation 
*	and is not a synchronization point with respect to user threads attempting 
*	to acquire read locks. Some of them may still succeed acquiring their read locks 
*	after the call subject to visibility of the memory write. On the other hand, 
*	this alllows to avoidd competing for internal serialization while setting
*	maintenance mode for the object.
* 
*	\return EOK on success, EBUSY if the oblect is already in maintenance mode, or any other system error code on failure.
*	\see mutexgear_maintlock_tryrdlock
*	\see mutexgear_maintlock_clear_maintenance
*/
_MUTEXGEAR_API int mutexgear_maintlock_set_maintenance(mutexgear_maintlock_t *__maintlock_instance);

/**
*	\fn int mutexgear_maintlock_clear_maintenance(mutexgear_maintlock_t *__maintlock_instance)
*	\brief Clears maintenance locked state from \c maintlock object allowing further \c mutexgear_maintlock_tryrdlock calls to succeed.
* 
*	If the function is called without prior call to \c mutexgear_maintlock_set_maintenance it returns EOK and has no effect.
*
*	\return EOK on success, or a system error code on failure.
*	\see mutexgear_maintlock_set_maintenance
*	\see mutexgear_maintlock_tryrdlock
*/
_MUTEXGEAR_API int mutexgear_maintlock_clear_maintenance(mutexgear_maintlock_t *__maintlock_instance);

/**
*	\fn int mutexgear_maintlock_test_maintenance(mutexgear_maintlock_t *__maintlock_instance, int *__out_wrlock_state)
*	\brief Retrieves current maintenance lock state value from \c maintlock object.
* 
*	The functions is intended to be used, primarily, within assertion checks, logs, and similar.
*
*	\return 0 if object is not in maintenance mode, 1 if the oblect is in maintenance mode, or a negated system error code on failure.
*	\see mutexgear_maintlock_set_maintenance
*	\see mutexgear_maintlock_tryrdlock
*/
_MUTEXGEAR_API int mutexgear_maintlock_test_maintenance(const mutexgear_maintlock_t *__maintlock_instance);

/**
*	\fn int mutexgear_maintlock_tryrdlock(mutexgear_maintlock_t *__maintlock_instance, mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_item_t *__item_instance)
*	\brief Tries to acquire read (shared) lock on the object.
* 
*	The function succeeds locking object for read if the object has not been switched to maintenance mode with \c mutexgear_maintlock_set_maintenance.
*	On success, the \p __out_lock_token receives an opaque value to be passed to \c mutexgear_maintlock_rdunlock.
*
*	The function does not block other than for serialization on internal operations.
*
*	The function requires initialized \p __worker_instance and \p __item_instance objects that must be allocated 
*	per calling thread (e.g. on stack). The \p __worker_instance and \p __item_instance must remain valid and 
*	not reused for other purposes until the read lock is released with a call to \c mutexgear_maintlock_rdunlock 
*	(in particular, the same parameters must not be used for enclosed read locks of other objects).
*	The \p __worker_instance must be locked with a \c mutexgear_completion_worker_lock call after initialization.
*	It is recommended that the objects are allocated with calling thread for its lifetime.
* 
*	In case of inter-process synchronization all the parameters must be allocated in the same
*	shared section as the \c maintlock object.
*
*	\return EOK on success, EBUSY if the object is in maintenance mode, or any other system error code on failure.
*	\see mutexgear_maintlock_rdunlock
*	\see mutexgear_completion_worker_lock
*/
_MUTEXGEAR_API int mutexgear_maintlock_tryrdlock(mutexgear_maintlock_t *__maintlock_instance,
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_item_t *__item_instance, mutexgear_maintlock_rdlock_token_t *__out_lock_token);


/**
*	\fn mutexgear_maintlock_rdunlock(mutexgear_maintlock_t *__maintlock_instance, mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_item_t *__item_instance)
*	\brief Releases the previously acquired object lock
*
*	The \p __worker_instance and \p __item_instance must be the same objects that were used in the previous call
*	to \c mutexgear_maintlock_tryrdlock on the \c maintlock. The \p __lock_token
*	must point to the value received from the respective \c mutexgear_maintlock_tryrdlock call.
* 
*	\return EOK on success or a system error code on failure.
*	\see mutexgear_maintlock_tryrdlock
*/
_MUTEXGEAR_API int mutexgear_maintlock_rdunlock(mutexgear_maintlock_t *__maintlock_instance,
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_item_t *__item_instance, const mutexgear_maintlock_rdlock_token_t *__lock_token);


/**
*	\fn int mutexgear_maintlock_wait_rdunlock(mutexgear_maintlock_t *__maintlock_instance, mutexgear_completion_waiter_t *__waiter_instance)
*	\brief Waits until all read locks acquired before this call are released.
* 
*	If the function is called while the object is switched to maintenance mode with \c mutexgear_maintlock_set_maintenance no new read locks can be added
*	and on the call exit all read locks are released. This way, the \c maintlock object can be used to isolate a resource for updates and then to allow further access
*	with \c mutexgear_maintlock_clear_maintenance call.
*
*	If no prior call to \c mutexgear_maintlock_set_maintenance was made, the furction will wait only those read locks to be released 
*	that had been acquired before the function call. New read locks can be acquired and released at the time the call is waiting but they will not affect the wait.
*	This approach can be used if external means are employed to initate update process on the resource being protected. 
*	Alternatively, the resource can be substituted/removed with an atomic exchange and the call can be used to wait those threads 
*	that might have seen and be still accessing the old resource instance. Since there is internal serialization and reads lock attempts do not block, 
*	there is strict separation of the read locks to those requested before and after the call entry. 
*
*	The function requires initialized \p __waiter_instance object that must be allocated per calling thread (e.g. on stack) 
*	and may be disposed after the call returns.
*
*	In case of inter-process synchronization all the parameters must be allocated in the same
*	shared section as the \c maintlock object.
*
*	The function is expected to be called in one thread at a time: new call is allowed only after the previous one returns.
* 
*	In case of a failure return, the object remains operational and other theares are not affected. 
*
*	\return EOK on success or a system error code on failure.
*	\see mutexgear_maintlock_set_maintenance
*	\see mutexgear_maintlock_tryrdlock
*	\see mutexgear_maintlock_rdunlock
*/
_MUTEXGEAR_API int mutexgear_maintlock_wait_rdunlock(mutexgear_maintlock_t *__maintlock_instance,
	mutexgear_completion_waiter_t *__waiter_instance);


//////////////////////////////////////////////////////////////////////////


_MUTEXGEAR_END_EXTERN_C();


#endif // #ifndef __MUTEXGEAR_MAINTLOCK_H_INCLUDED
