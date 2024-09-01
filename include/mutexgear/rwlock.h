#ifndef __MUTEXGEAR_RWLOCK_H_INCLUDED
#define __MUTEXGEAR_RWLOCK_H_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear RWLock API Definitions                                     */
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
*	\brief MutexGear RWLock API definitions
*
*	The header defines a "read-write lock" (\c rwlock) object.
*	A read-write lock is an object that can be locked by single thread at a time 
*	(exclusively) for "write" or be locked by multiple threads simultaneously 
*	(shared) for "read". This particular implementation is based solely on muteces 
*	and atomic operations. No polling, freewheeling or similar iterative techniques
*	are explicitly used.
*
*	Author considers "try-lock" operations being of little practical value. 
*	Therefore, the base implementation only provides "write" try-lock option
*	which could be implemented without adding extra internal components.
*	If try-read lock operation is required there is a separate more heavy-weight 
*	object version in this header to support it.
*
*	Also, the implementation features write lock priority. That is, whenever there are 
*	write lock attempts waiting for the object to be released by current read locks, all 
*	additional read lock attempts will be blocked and will wait until all the write locks 
*	(the ones that were there before the read lock attempts and all that might have been 
*	added after) complete and release the object. The read locks are allowed on the object 
*	only when there are no write locks waiting. This helps to resolve an issue with multiple 
*	readers constantly sharing object locked for read and not allowing a write lock that 
*	needs the object exclusively. On the other hand, this may increase lock acquisition times
*	for read threads that will be more often forced into kernel for blocking when greater
*	number of readers are frequently and regularly interrupted for short durations by 
*	smaller number of writers.
*
*	Additionally, the implementation allows write lock multi-channel waiting for read locks to 
*	release the object. This means that several read locks may be waited by several 
*	write locks attempts in parallel and thus, since the waiting is performed on muteces,
*	the system may apply priority inheritance on multiple execution cores that would be
*	running those read lock acquired threads. Currently, 1, 2 or 4 channels are supported.
*
*	Keep in mind to not mistake read-write lock for mutex. A read-write lock is to be 
*	used when there mainly are relatively lengthy operations that can run in parallel 
*	but sometimes (less frequently) they need to be expelled by one or a few operations 
*	that need to execute exclusively. Whenever operations are quick, use plain muteces.
*	Whenever frequencies of read and write access are comparable or writes prevail, 
*	consider aggregating or packeting the writes at earlier stages separately 
*	to decrease their number and have the object write locks being requested less frequently.
*
*	NOTE:
*
*	The \c mutexgear_rwlock_t and \c mutexgear_trdl_rwlock_t objects depend on a synchronization
*	mechanism being a subject of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and
*	Image Database search (currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm)
*	to view the patent text.
*/


#include <mutexgear/config.h>
#include <mutexgear/completion.h>
#include <mutexgear/constants.h>


_MUTEXGEAR_BEGIN_EXTERN_C();


//////////////////////////////////////////////////////////////////////////
// mutexgear_rwlockattr_t 

/**
*	\struct mutexgear_rwlockattr_t
*	\brief An opaque attribute structure to be used for \c mutexgear_rwlock_t initialization.
*/
typedef struct _mutexgear_rwlockattr
{
	_MUTEXGEAR_LOCKATTR_T	lock_attr;
	unsigned int			mode_flags;

} mutexgear_rwlockattr_t;


/**
*	\fn int mutexgear_rwlockattr_init(mutexgear_rwlockattr_t *__attr_instance)
*	\brief A function to initialize a \c mutexgear_rwlockattr_t instance.
*
*	The attributes are initialized with defaults that are counterparts of the underlying
*	system structure defaults. Typically these are:
*	\li MUTEXGEAR_PROCESS_PRIVATE for "pshared";
*	\li MUTEXGEAR_PRIO_INHERIT for "protocol".
*	Also, 0 is used for "writechannels".
*	\return EOK on success or a system error code on failure.
*/
_MUTEXGEAR_API int mutexgear_rwlockattr_init(mutexgear_rwlockattr_t *__attr_instance);

/**
*	\fn int mutexgear_rwlockattr_destroy(mutexgear_rwlockattr_t *__attr_instance)
*	\brief A function to destroy a previously initialized \c mutexgear_rwlockattr_t 
*	instance and free any resources possibly allocated for it.
*	\return EOK on success or a system error code on failure.
*/
_MUTEXGEAR_API int mutexgear_rwlockattr_destroy(mutexgear_rwlockattr_t *__attr_instance);


/**
*	\fn int mutexgear_rwlockattr_setpshared(mutexgear_rwlockattr_t *__attr_instance, int __pshared_value)
*	\brief A function to assign process shared attribute value to
*	a \c mutexgear_rwlockattr_t  structure (similar to \c the pthread_mutexattr_setpshared).
*	\param __pshared_value one of \c MUTEXGEAR_PROCESS_PRIVATE or \c MUTEXGEAR_PROCESS_SHARED
*	\return EOK on success or a system error code on failure.
*/
_MUTEXGEAR_API int mutexgear_rwlockattr_setpshared(mutexgear_rwlockattr_t *__attr_instance, int __pshared_value);

/**
*	\fn int mutexgear_rwlockattr_getpshared(const mutexgear_rwlockattr_t *__attr_instance, int *__out_pshared_value)
*	\brief A function to get process shared attribute value stored in
*	a \c mutexgear_rwlockattr_t  structure (similar to the \c pthread_mutexattr_getpshared).
*	\param __out_pshared_value pointer to a variable to receive process shared attribute value stored in the structure (either \c MUTEXGEAR_PROCESS_PRIVATE or \c MUTEXGEAR_PROCESS_SHARED)
*	\return EOK on success or a system error code on failure.
*/
_MUTEXGEAR_API int mutexgear_rwlockattr_getpshared(const mutexgear_rwlockattr_t *__attr_instance, int *__out_pshared_value);


/**
*	\fn int mutexgear_rwlockattr_setprioceiling(mutexgear_rwlockattr_t *__attr_instance, int __prioceiling_value)
*	\brief A function to assign a priority ceiling value to
*	a \c mutexgear_rwlockattr_t  structure (similar to the \c pthread_mutexattr_setprioceiling).
*	\param __prioceiling_value a priority value to be used as the ceiling
*	\return EOK on success or a system error code on failure.
*/
_MUTEXGEAR_API int mutexgear_rwlockattr_setprioceiling(mutexgear_rwlockattr_t *__attr_instance, int __prioceiling_value);

/**
*	\fn int mutexgear_rwlockattr_setprotocol(mutexgear_rwlockattr_t *__attr_instance, int __protocol_value)
*	\brief A function to set a lock protocol value to
*	a \c mutexgear_rwlockattr_t  structure (similar to the \c pthread_mutexattr_setprotocol).
*	\param __protocol_value one of MUTEXGEAR_PRIO_INHERIT, MUTEXGEAR_PRIO_PROTECT, MUTEXGEAR_PRIO_NONE
*	\return EOK on success or a system error code on failure.
*/
_MUTEXGEAR_API int mutexgear_rwlockattr_setprotocol(mutexgear_rwlockattr_t *__attr_instance, int __protocol_value);

/**
*	\fn int mutexgear_rwlockattr_getprioceiling(const mutexgear_rwlockattr_t *__attr_instance, int *__out_prioceiling_value)
*	\brief A function to query priority ceiling attribute value
*	stored in a \c mutexgear_rwlockattr_t  structure (similar to the \c pthread_mutexattr_getprioceiling).
*	\param __out_prioceiling_value pointer to a variable to receive priority ceiling attribute value stored in the structure (either a positive value or -1 if priority ceiling is not supported by the OS)
*	\return EOK on success or a system error code on failure.
*/
_MUTEXGEAR_API int mutexgear_rwlockattr_getprioceiling(const mutexgear_rwlockattr_t *__attr_instance, int *__out_prioceiling_value);

/**
*	\fn int mutexgear_rwlockattr_getprotocol(const mutexgear_rwlockattr_t *__attr_instance, int *__out_protocol_value)
*	\brief A function to retrieve lock protocol attribute value
*	stored in a \c mutexgear_rwlockattr_t  structure (similar to the \c pthread_mutexattr_getprotocol).
*	\param __out_protocol_value pointer to a variable to receive priority inheritance protocol attribute value stored in the structure (MUTEXGEAR_PRIO_INHERIT, MUTEXGEAR_PRIO_PROTECT, MUTEXGEAR_PRIO_NONE; or MUTEXGEAR__INVALID_PROTOCOL if the feature is not supported by the OS)
*	\return EOK on success or a system error code on failure.
*/
_MUTEXGEAR_API int mutexgear_rwlockattr_getprotocol(const mutexgear_rwlockattr_t *__attr_instance, int *__out_protocol_value);


/**
*	\fn int mutexgear_rwlockattr_setmutexattr(mutexgear_rwlockattr_t *__attr_instance, const _MUTEXGEAR_LOCKATTR_T *__mutexattr_instance)
*	\brief A function to copy attributes from a mutex attributes structure to
*	the \c mutexgear_rwlockattr_t  structure.
*
*	Values of "pshared", "protocol" and "prioceiling" are copied.
*
*	Due to inability to clear previously assigned priority ceiling setting from attributes on some targets
*	it's recommended to call this function with freshly initialized \a __attr_instance only.
*	\return EOK on success or a system error code on failure.
*/
_MUTEXGEAR_API int mutexgear_rwlockattr_setmutexattr(mutexgear_rwlockattr_t *__attr_instance, const _MUTEXGEAR_LOCKATTR_T *__mutexattr_instance);


/**
*	\fn int mutexgear_rwlockattr_setwritechannels(mutexgear_rwlockattr_t *__attr_instance, unsigned int __channel_count)
*	\brief A function to assign number of parallel locks write requests are to be split into while waiting for reads.
*
*	If several write requests are waiting for readers to release the lock, by default, they use a single lock. 
*	This way priority inheritance from all the write waiters is combined together and applied onto a singe reader thread
*	at a time acting on a single execution core. The function allows specifying bigger number for parallel locks that 
*	could potentially act with priority inheritance on more execution cores at a cost of more code executed by the write waiter.
*	It is possible to control write wait thread's lock channel selection by allocating the related \c mutexgear_completion_waiter_t
*	instance with a proper alignment (see _MUTEXGEAR_RWLOCK_READERPUSHSELECTOR_FACTOR use).
*
*	The parameter is automatically adjusted to the nearest acceptable value. Pass 0 for the system default (currently, a single channel).
*	To determine the default value at runtime assign 0 and read the value back with \c mutexgear_rwlockattr_getwritechannels.
*
*	Multiple channels can have positive effect when there are frequent cases of a few competing write lock requests at a time 
*	and at the same time there is even bigger number of read requests having the object acquired shared.
*	
*	\return EOK on success or a system error code on failure.
*	\see mutexgear_rwlockattr_getwritechannels
*/
_MUTEXGEAR_API int mutexgear_rwlockattr_setwritechannels(mutexgear_rwlockattr_t *__attr_instance, unsigned int __channel_count);

/**
*	\fn int mutexgear_rwlockattr_getwritechannels(mutexgear_rwlockattr_t *__attr_instance, unsigned int *__out_channel_count)
*	\brief A function to retrieve number of parallel locks write requests are to be split into while waiting for reads.
*
*	See \c mutexgear_rwlockattr_setwritechannels for details.
*	\return EOK on success or a system error code on failure.
*	\see mutexgear_rwlockattr_setwritechannels
*/
_MUTEXGEAR_API int mutexgear_rwlockattr_getwritechannels(mutexgear_rwlockattr_t *__attr_instance, unsigned int *__out_channel_count);



//////////////////////////////////////////////////////////////////////////

#define _MUTEXGEAR_RWLOCK_READERPUSHLOCK_MAXCOUNT		4U

#ifndef _MUTEXGEAR_RWLOCK_READERPUSHSELECTOR_FACTOR
#ifdef MEMORY_ALLOCATION_ALIGNMENT
#define _MUTEXGEAR_RWLOCK_READERPUSHSELECTOR_FACTOR		((size_t)MEMORY_ALLOCATION_ALIGNMENT)
#else
#define _MUTEXGEAR_RWLOCK_READERPUSHSELECTOR_FACTOR		(2 * sizeof(void *))
#endif
#endif // #ifndef _MUTEXGEAR_RWLOCK_READERPUSHSELECTOR_FACTOR

/**
*	\struct mutexgear_rwlock_t
*	\brief An opaque structure to represent a read-write lock (\c rwlock) object without tryrdlock operation support
*
*	This is a basic \c rwlock class with wrlock, wrunlock, rdlock, rdunlock, and with trywrlock 
*	but without tryrdlock operation support. For additional tryrdlock, use 
*	\c mutexgear_trdl_rwlock_t object instead.
*
*	The operation pseudo-codes are provided below.
*
*	wrlock(__worker_instance, __waiter_instance, __item_instance)
*
*	----------
*
*	\li 1. acquire the mutex of \c acquired_reads;
*	\li 2. if \c acquired_reads is empty then exit;
*	\li 3. release the mutex of \c acquired_reads;
*	\li 4. add \c __item_instance (or an internally allocated item) with \c __worker_instance into \c waiting_writes;
*	\li 5. acquire one of \c reader_push_locks;
*	\li 6. acquire the mutex of \c acquired_reads;
*	\li 7. if \c acquired_reads is not empty then wait on its tail with \c __waiter_instance, atomically releasing the mutex of \c acquired_reads; then goto 6;
*	\li 8. release the owned mutex of \c reader_push_locks;
*	\li 9. remove \c __item_instance from \c waiting_writes awakening a reader waiting there, if any.
*
*	(exiting having the mutex of \c acquired_reads acquired)
*
*
*	trywrlock()
*
*	----------
*
*	\li 1. check whether \c acquired_reads is empty; if not then exit with EBUSY;
*	\li 2. try-acquire the mutex of \c acquired_reads, exit with EBUSY if the mutex is busy;
*	\li 3. if \c acquired_reads is empty then exit with EOK;
*	\li 4. release the mutex of \c acquired_reads;
*	\li 5. exit with EBUSY;
*
*	(exiting having the mutex of \c acquired_reads acquired in case of success)
*
*
*	wrunlock()
*
*	----------
*
*	\li 1. release the mutex of \c acquired_reads.
*
*
*	rdlock(__worker_instance, __waiter_instance, __item_instance)
*
*	----------
*
*	\li 1. Check if \c waiting_writes is empty; if not then goto 9;
*	\li 2. Make 8 attempts to link \c __item_instance with its "previous" pointer into \c express_reads using CAS atomic operations; if all the attempts fail then goto 6;
*	\li 3. Check whether \c acquired_reads contains at least one item; if yes then exit with success status (a lock-free acquisition)
*	\li 4. acquire the mutex of \c acquired_reads;
*	\li 5. goto 21
*	\li 6. acquire the mutex of \c acquired_reads; 
*	\li 7. if \c waiting_writes is empty then goto 20;
*	\li 8. release the mutex of \c acquired_reads;
*	\li 9. acquire the mutex of \c waiting_reads;
*	\li 10. add \c __item_instance with \c __worker_instance into \c waiting_reads; if the queue was not empty then goto 12;
*	\li 11. release the mutex of \c waiting_reads;
*	\li 12. acquire the mutex of \c waiting_writes;
*	\li 13. if \c waiting_writes is not empty then wait on its tail with \c __waiter_instance, atomically releasing the mutex of \c waiting_writes; then goto 17;
*	\li 14. release the mutex of \c waiting_writes;
*	\li 15. splice all the items of \c waiting_reads into \c read_wait_drain; remove \c __item_instance from the moved queue awakening the next queued reader there, if any;
*	\li 16. goto 6;
*	\li 17. wait on the former last item in \c waiting_reads with \c __waiter_instance, atomically releasing the mutex of \c waiting_reads;
*	\li 18. remove \c __item_instance from its current queue awakening the next queued reader there, if any;
*	\li 19. goto 6;
*	\li 20. add \c __item_instance with \c __worker_instance into \c acquired_reads;
*	\li 21. atomically extract the single-linked list from the \c express_reads, link the list's element "next" pointers and splice it whole into the \c acquired_reads; 
*	\li 22. release the mutex of \c acquired_reads.
*
*	(exiting with having the \c __item_instance with the \c __worker_instance added either into \c express_reads or into \c acquired_reads)
*
*
*	rdunlock(__worker_instance, __item_instance)
*
*	----------
*
*	\li 1. acquire the mutex of \c acquired_reads;
*	\li 2. check whether \c __item_instance is already linked into \c acquired_reads; if not (i.e. the item is still in \c express_reads) then goto 7
*	\li 3. remove \c __item_instance from \c acquired_reads;
*	\li 4. atomically extract the single-linked list from the \c express_reads, link the list's element "next" pointers and splice it whole into the \c acquired_reads;
*	\li 5. release the mutex of \c acquired_reads;
*	\li 6. awaken a writer blocked on \c __worker_instance in the \c __item_instance, if any, and exit.
*	\li 7. atomically extract the single-linked list from the \c express_reads, link the list's element "next" pointers removing the \c __item_instance from the chain, then splice the remainder into the \c acquired_reads;
*	\li 8. release the mutex of \c acquired_reads;
*
*
*	\see mutexgear_rwlock_init
*	\see mutexgear_rwlock_destroy
*	\see mutexgear_rwlock_wrlock
*	\see mutexgear_rwlock_trywrlock
*	\see mutexgear_rwlock_wrunlock
*	\see mutexgear_rwlock_rdlock
*	\see mutexgear_rwlock_rdunlock
*	\see mutexgear_trdl_rwlock_t
*/
typedef struct _mutexgear_rwlock
{
	// Fields modified by readers
	mutexgear_completion_drainablequeue_t waiting_reads;
	mutexgear_dlraitem_prev_t    express_reads;
	ptrdiff_t                    express_commits;
	// Rarely modified fields for separation
	mutexgear_completion_queue_t waiting_writes;
	mutexgear_completion_drain_t read_wait_drain;
	// Fields accessed by writers
	union
	{
		unsigned int                mode_flags;
		size_t                      _reserved1; // For alignment of the subsequent fields

	} fl_un;
	// Fields modified by writers
	_MUTEXGEAR_LOCK_T            reader_push_locks[_MUTEXGEAR_RWLOCK_READERPUSHLOCK_MAXCOUNT];
	// Fields modified by both readers and writers are to be kept at an end to minimize cache invalidations among the threads on other fields
	mutexgear_completion_queue_t acquired_reads;

} mutexgear_rwlock_t;


/**
*	\struct mutexgear_trdl_rwlock_t
*	\brief An opaque structure to represent a read-write lock (\c rwlock) object with tryrdlock operation support
*
*	This is an enhanced \c rwlock class with the full operation set support, namely wrlock, wrunlock, rdlock, rdunlock, 
*	trywrlock, and tryrdlock. However, it allocates an extra atomic counter, an extra mutex, and a \c _mutexgear_completion_itemdata_t structure 
*	thus requiring more memory and kernel objects. Also, wrlock and trywrlock operations perform an extra lock-unlock sequence 
*	on the extra mutex to guard against potential tryrdlock's.
*
*	If you don't intend using tryrdlock restrain from using this object and create basic \c mutexgear_rwlock_t objects instead.
*
*	The operation pseudo-codes are provided below.
*
*	wrlock(__worker_instance, __waiter_instance, __item_instance)
*
*	----------
*
*	\li 1. atomically increment \c wrlock_waits;
*	\li 2. if the increment shown that readers could witness a zero \c wrlock_waits value lock and imediately unlock \c tryread_queue_lock;
*
*	(the following steps match those of the basic implementation starting from its step 1.)
*
*
*	trywrlock()
*
*	----------
*
*	\li 1. check whether \c acquired_reads is empty; if not then exit with EBUSY;
*	\li 2. atomically increment \c wrlock_waits;
*	\li 3. if the increment shown that readers could witness a zero \c wrlock_waits value lock and imediately unlock \c tryread_queue_lock;
*	\li 4. check whether \c acquired_reads is empty; if not then atomically decrement \c wrlock_waits and exit with EBUSY;
*	\li 5. try-acquire the mutex of \c acquired_reads, atomically decrementing the \c wrlock_waits and exiting with EBUSY if the mutex was busy;
*	\li 6. if \c acquired_reads is empty then exit with EOK;
*	\li 7. release the mutex of \c acquired_reads and atomically decrement \c wrlock_waits;
*	\li 8. exit with EBUSY.
*
*	(exiting having the mutex of \c acquired_reads acquired in case of success)
*
*
*	wrunlock()
*
*	----------
*
*	\li 1. atomically decrement \c wrlock_waits;
*	\li 2. release the mutex of \c acquired_reads.
*
*
*	rdlock(__worker_instance, __waiter_instance, __item_instance)
*
*	----------
*
*  (the algorithm is identical with an exception that \c __item_instance is linked into \c acquired_reads before \c tryread_queue_separator (i.e., into the list section dedicated for normal reads))
*
*
*	tryrdlock(__worker_instance, __item_instance)
*
*	----------
*
*	\li 1. check if wrlock_waits is zero; exit with EBUSY if not;
*	\li 2. acquire \c tryread_queue_lock;
*	\li 3. check if \c wrlock_waits is zero, releasing the \c tryread_queue_lock and exiting with EBUSY if not;
*	\li 4. add \c __item_instance with \c __worker_instance after the \c tryread_queue_separator into \c acquired_reads (i.e., into the list section dedicated for try-reads);
*	\li 5. release \c tryread_queue_lock and exit with EOK.
*
*	(exiting with having the \c __item_instance with the \c __worker_instance added into \c acquired_reads)
*
*
*	rdunlock(__worker_instance, __item_instance)
*
*	----------
*
*	\li 1. acquire the mutex of \c acquired_reads;
*	\li 2. check whether \c __item_instance is already linked into \c acquired_reads; if not (i.e. the item is still in \c express_reads) then goto 9
*	\li 3. if the read-lock being released for \c __item_instance has been acquired with a try-lock operation then acquire \c tryread_queue_lock;
*	\li 4. remove \c __item_instance from \c acquired_reads;
*	\li 5. release \c tryread_queue_lock if it was locked in step 3;
*	\li 6. atomically extract the single-linked list from the \c express_reads, link the list's element "next" pointers and splice it whole before \c tryread_queue_separator into the \c acquired_reads;
*	\li 7. release the mutex of \c acquired_reads;
*	\li 8. awaken a writer blocked on \c __worker_instance in the \c __item_instance, if any, and exit.
*	\li 9. atomically extract the single-linked list from the \c express_reads, link the list's element "next" pointers removing the \c __item_instance from the chain, then splice the remainder before \c tryread_queue_separator into the \c acquired_reads;
*	\li 10. release the mutex of \c acquired_reads;
*
*
*	\see mutexgear_rwlock_init
*	\see mutexgear_rwlock_destroy
*	\see mutexgear_rwlock_wrlock
*	\see mutexgear_rwlock_trywrlock
*	\see mutexgear_rwlock_wrunlock
*	\see mutexgear_rwlock_rdlock
*	\see mutexgear_rwlock_tryrdlock
*	\see mutexgear_rwlock_rdunlock
*	\see mutexgear_rwlock_t
*/
typedef struct _mutexgear_trdl_rwlock
{
	// The basic rwlock
	mutexgear_rwlock_t           basic_lock;
	// Extra fields for tryrdlock operation support
	ptrdiff_t                    wrlock_waits;
	_MUTEXGEAR_LOCK_T            tryread_queue_lock;
	_mutexgear_completion_itemdata_t  tryread_queue_separator;

} mutexgear_trdl_rwlock_t;


/**
*	\fn int mutexgear_rwlock_init(mutexgear_rwlock_t *__rwlock_instance, mutexgear_rwlockattr_t *__attr_instance=NULL)
*	\brief Creates a \c rwlock object
*
*	The function can also be called for \c mutexgear_trdl_rwlock_t objects.
*
*	The object is initialized accordingly to the \p __attr_instance values.
*	\param __attr_instance An initialized attributes structure or NULL to use defaults
*	\return EOK on success or a system error code on failure.
*	\see mutexgear_rwlock_destroy
*/
_MUTEXGEAR_API int mutexgear_rwlock_init(mutexgear_rwlock_t *__rwlock_instance, mutexgear_rwlockattr_t *__attr_instance/*=NULL*/);

_MUTEXGEAR_API int mutexgear_trdl_rwlock_init(mutexgear_trdl_rwlock_t *__rwlock_instance, mutexgear_rwlockattr_t *__attr_instance/*=NULL*/);

#if defined(__cplusplus)
_MUTEXGEAR_END_EXTERN_C();

static inline
int mutexgear_rwlock_init(mutexgear_trdl_rwlock_t *__rwlock_instance, mutexgear_rwlockattr_t *__attr_instance/*=NULL*/)
{
	return mutexgear_trdl_rwlock_init(__rwlock_instance, __attr_instance);
}

_MUTEXGEAR_BEGIN_EXTERN_C();
#endif // #if defined(__cplusplus)

#if defined(MUTEXGEAR_USE_C11_GENERICS)
#define mutexgear_rwlock_init(__rwlock_instance, __attr_instance) _Generic(__rwlock_instance, \
	mutexgear_trdl_rwlock_t *: mutexgear_trdl_rwlock_init, \
	default: mutexgear_rwlock_init)(__rwlock_instance, __attr_instance)
#endif // #if defined(MUTEXGEAR_USE_C11_GENERICS)


/**
*	\fn int mutexgear_rwlock_destroy(mutexgear_rwlock_t *__rwlock_instance)
*	\brief Destroys the \c rwlock object
*
*	The function can also be called for \c mutexgear_trdl_rwlock_t objects.
*
*	The object must be free of locks to be destroyed successfully.
*	\return EOK on success or a system error code on failure.
*	\see mutexgear_rwlock_init
*/
_MUTEXGEAR_API int mutexgear_rwlock_destroy(mutexgear_rwlock_t *__rwlock_instance);

_MUTEXGEAR_API int mutexgear_trdl_rwlock_destroy(mutexgear_trdl_rwlock_t *__rwlock_instance);

#if defined(__cplusplus)
_MUTEXGEAR_END_EXTERN_C();

static inline
int mutexgear_rwlock_destroy(mutexgear_trdl_rwlock_t *__rwlock_instance)
{
	return mutexgear_trdl_rwlock_destroy(__rwlock_instance);
}

_MUTEXGEAR_BEGIN_EXTERN_C();
#endif // #if defined(__cplusplus)

#if defined(MUTEXGEAR_USE_C11_GENERICS)
#define mutexgear_rwlock_destroy(__rwlock_instance) _Generic(__rwlock_instance, \
	mutexgear_trdl_rwlock_t *: mutexgear_trdl_rwlock_destroy, \
	default: mutexgear_rwlock_destroy)(__rwlock_instance)
#endif // #if defined(MUTEXGEAR_USE_C11_GENERICS)


/**
*	\fn int mutexgear_rwlock_wrlock(mutexgear_rwlock_t *__rwlock_instance, mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_waiter_t *__waiter_instance, mutexgear_completion_item_t *__item_instance)
*	\brief Acquires the object write (exclusive) lock with write priority
*
*	The function can also be called for \c mutexgear_trdl_rwlock_t objects.
* 
*	The function claims write priority if there are readers holding locks on the object. This prevents all 
*	new read lock attempts from sharing the lock with existing readers and puts them into wait queue instead.
*	As soon as all existing readers release their locks, the caller or other write waits will be satisfied.
*	Read locks can succeed on the object only after no write lock waiting threads claiming write priority remain.
*
*	The function requires initialized \p __worker_instance and \p __waiter_instance objects
*	that must be allocated per calling thread (e.g. on stack) and may be disposed after the call returns.
*	The \p __worker_instance must be locked with a \c mutexgear_completion_worker_lock call after initialization.
*
*	The \p __item_instance can be passed as NULL unless when the \c rwlock is used for inter-process synchronization. 
*	It can also be disposed immediately after the call returns. If provided, an initialized object is to be passed.
*	Performance could be a tiny bit better when the parameter is not NULL. It is recommended to keep the parameter 
*	allocated with calling thread for reuse.
*
*	If a thread may call both \c mutexgear_rwlock_wrlock and \c mutexgear_rwlock_rdlock 
*	a single instance of \c mutexgear_completion_item_t may be used for the both.
*
*	In case of inter-process synchronization all the parameters must be allocated in the same 
*	shared section as the \c rwlock object.
*
*	The object must not be locked for write by a single thread recursively.
*
*	\return EOK on success or a system error code on failure.
*	\see mutexgear_rwlock_wrunlock
*	\see mutexgear_completion_worker_lock
*	\see mutexgear_rwlock_trywrlock
*	\see mutexgear_rwlock_wrlock_cwp
*/
_MUTEXGEAR_API int mutexgear_rwlock_wrlock(mutexgear_rwlock_t *__rwlock_instance,
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_waiter_t *__waiter_instance, mutexgear_completion_item_t *__item_instance/*=NULL*/);

_MUTEXGEAR_API int mutexgear_trdl_rwlock_wrlock(mutexgear_trdl_rwlock_t *__rwlock_instance,
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_waiter_t *__waiter_instance, mutexgear_completion_item_t *__item_instance/*=NULL*/);

#if defined(__cplusplus)
_MUTEXGEAR_END_EXTERN_C();

static inline
int mutexgear_rwlock_wrlock(mutexgear_trdl_rwlock_t *__rwlock_instance,
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_waiter_t *__waiter_instance, mutexgear_completion_item_t *__item_instance/*=NULL*/)
{
	return mutexgear_trdl_rwlock_wrlock(__rwlock_instance, __worker_instance, __waiter_instance, __item_instance);
}

_MUTEXGEAR_BEGIN_EXTERN_C();
#endif // #if defined(__cplusplus)

#if defined(MUTEXGEAR_USE_C11_GENERICS)
#define mutexgear_rwlock_wrlock(__rwlock_instance, __worker_instance, __waiter_instance, __item_instance) _Generic(__rwlock_instance, \
	mutexgear_trdl_rwlock_t *: mutexgear_trdl_rwlock_wrlock, \
	default: mutexgear_rwlock_wrlock)(__rwlock_instance, __worker_instance, __waiter_instance, __item_instance)
#endif // #if defined(MUTEXGEAR_USE_C11_GENERICS)


/**
*	\fn int mutexgear_rwlock_wrlock_cwp(mutexgear_rwlock_t *__rwlock_instance, mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_waiter_t *__waiter_instance, mutexgear_completion_item_t *__item_instance, int __readers_till_wp)
*	\brief Acquires the object write (exclusive) lock with ability to customize write priority behavior.
*
*	The function can also be called for \c mutexgear_trdl_rwlock_t objects.
* 
*	The function behaves identically to \c mutexgear_rwlock_wrlock with the exception that 
*	writer priority behavior can be controlled with \p __readers_till_wp parameter. If the parameter is 
*	positive the function claims write priority only after it witnesses that number of readers releasing the lock 
*	while having not acquired the write lock on its own yet. If the parameter is negative the function never claims write priority
*	and behaves like most other rwlock implementations do. If the parameter is zero 
*	the function claims write priority immediately if there are active readers.
*
*	\note
*	Keep in mind that a positive \p __readers_till_wp value can't be interpreted as an exact number of readers allowed before write priority
*	is claimed. Write locking threads wait for readers serialized into one or a few channels as set with \c mutexgear_rwlockattr_setwritechannels.
*	Therefore, in case of competing writers exceeding the number of available channels, it is generally undefined how many readers release their locks 
*	until a particular writer gets access to a waiting channel and starts counting read lock releases. The general rule is though:
*	the higher the parameter value is - the longer a writer could wait before forcing new incoming readers to let it ahead.
*
*	\return EOK on success or a system error code on failure.
*	\see mutexgear_rwlock_wrlock
*	\see mutexgear_rwlock_wrunlock
*	\see mutexgear_rwlockattr_setwritechannels
*	\see mutexgear_rwlock_trywrlock
*/
_MUTEXGEAR_API int mutexgear_rwlock_wrlock_cwp(mutexgear_rwlock_t *__rwlock_instance,
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_waiter_t *__waiter_instance, mutexgear_completion_item_t *__item_instance/*=NULL*/, int __readers_till_wp);

_MUTEXGEAR_API int mutexgear_trdl_rwlock_wrlock_cwp(mutexgear_trdl_rwlock_t *__rwlock_instance,
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_waiter_t *__waiter_instance, mutexgear_completion_item_t *__item_instance/*=NULL*/, int __readers_till_wp);

#if defined(__cplusplus)
_MUTEXGEAR_END_EXTERN_C();

static inline
int mutexgear_rwlock_wrlock_cwp(mutexgear_trdl_rwlock_t *__rwlock_instance,
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_waiter_t *__waiter_instance, mutexgear_completion_item_t *__item_instance/*=NULL*/, int __readers_till_wp)
{
	return mutexgear_trdl_rwlock_wrlock_cwp(__rwlock_instance, __worker_instance, __waiter_instance, __item_instance, __readers_till_wp);
}

_MUTEXGEAR_BEGIN_EXTERN_C();
#endif // #if defined(__cplusplus)

#if defined(MUTEXGEAR_USE_C11_GENERICS)
#define mutexgear_rwlock_wrlock_cwp(__rwlock_instance, __worker_instance, __waiter_instance, __item_instance, __readers_till_wp) _Generic(__rwlock_instance, \
	mutexgear_trdl_rwlock_t *: mutexgear_trdl_rwlock_wrlock_cwp, \
	default: mutexgear_rwlock_wrlock_cwp)(__rwlock_instance, __worker_instance, __waiter_instance, __item_instance, __readers_till_wp)
#endif // #if defined(MUTEXGEAR_USE_C11_GENERICS)


/**
*	\fn int mutexgear_rwlock_trywrlock(mutexgear_rwlock_t *__rwlock_instance)
*	\brief Tries to acquire the object write (exclusive) lock without blocking
*
*	The function can also be called for \c mutexgear_trdl_rwlock_t objects.
*
*	The object must not be locked for write by a single thread recursively.
*
*	\return EOK on success, EBUSY if the object is already locked by another thread, or a system error code on failure.
*	\see mutexgear_rwlock_wrunlock
*	\see mutexgear_rwlock_wrlock
*/
_MUTEXGEAR_API int mutexgear_rwlock_trywrlock(mutexgear_rwlock_t *__rwlock_instance);

_MUTEXGEAR_API int mutexgear_trdl_rwlock_trywrlock(mutexgear_trdl_rwlock_t *__rwlock_instance);

#if defined(__cplusplus)
_MUTEXGEAR_END_EXTERN_C();

static inline
int mutexgear_rwlock_trywrlock(mutexgear_trdl_rwlock_t *__rwlock_instance)
{
	return mutexgear_trdl_rwlock_trywrlock(__rwlock_instance);
}

_MUTEXGEAR_BEGIN_EXTERN_C();
#endif // #if defined(__cplusplus)

#if defined(MUTEXGEAR_USE_C11_GENERICS)
#define mutexgear_rwlock_trywrlock(__rwlock_instance) _Generic(__rwlock_instance, \
	mutexgear_trdl_rwlock_t *: mutexgear_trdl_rwlock_trywrlock, \
	default: mutexgear_rwlock_trywrlock)(__rwlock_instance)
#endif // #if defined(MUTEXGEAR_USE_C11_GENERICS)


/**
*	\fn int mutexgear_rwlock_wrunlock(mutexgear_rwlock_t *__rwlock_instance)
*	\brief Releases the previously acquired object write (exclusive) lock
*
*	The function can also be called for \c mutexgear_trdl_rwlock_t objects.
*
*	\return EOK on success or a system error code on failure.
*	\see mutexgear_rwlock_wrlock
*	\see mutexgear_rwlock_trywrlock
*/
_MUTEXGEAR_API int mutexgear_rwlock_wrunlock(mutexgear_rwlock_t *__rwlock_instance);

_MUTEXGEAR_API int mutexgear_trdl_rwlock_wrunlock(mutexgear_trdl_rwlock_t *__rwlock_instance);

#if defined(__cplusplus)
_MUTEXGEAR_END_EXTERN_C();

static inline
int mutexgear_rwlock_wrunlock(mutexgear_trdl_rwlock_t *__rwlock_instance)
{
	return mutexgear_trdl_rwlock_wrunlock(__rwlock_instance);
}

_MUTEXGEAR_BEGIN_EXTERN_C();
#endif // #if defined(__cplusplus)

#if defined(MUTEXGEAR_USE_C11_GENERICS)
#define mutexgear_rwlock_wrunlock(__rwlock_instance) _Generic(__rwlock_instance, \
	mutexgear_trdl_rwlock_t *: mutexgear_trdl_rwlock_wrunlock, \
	default: mutexgear_rwlock_wrunlock)(__rwlock_instance)
#endif // #if defined(MUTEXGEAR_USE_C11_GENERICS)


/**
*	\fn int mutexgear_rwlock_rdlock(mutexgear_rwlock_t *__rwlock_instance, mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_waiter_t *__waiter_instance, mutexgear_completion_item_t *__item_instance)
*	\brief Acquires the object read (shared) lock
*
*	The function can also be called for \c mutexgear_trdl_rwlock_t objects.
*
*	The function requires initialized \p __worker_instance, \p __waiter_instance and
*	\p __item_instance objects that must be allocated per calling thread (e.g. on stack).
*	The \p __worker_instance and \p __item_instance must remain valid and not reused for other purposes 
*	until the read lock is released with a call to \c mutexgear_rwlock_rdunlock (in particular, 
*	the same parameters must not be used for enclosed read or write locks of other \c rwlock objects). 
*	The \p __worker_instance must be locked with a \c mutexgear_completion_worker_lock call after initialization. 
*	The \p __waiter_instance may be reused for nested acquisitions of other \c rwlock objects.
*	It is recommended that the objects are allocated with calling thread for its lifetime. 
*	If a thread may call to acquire both read and write locks during its execution 
*	the same set of objects may be used in all the calls.
*
*	In case of inter-process synchronization all the parameters must be allocated in the same
*	shared section as the \c rwlock object.
*
*	\return EOK on success or a system error code on failure.
*	\see mutexgear_rwlock_rdunlock
*	\see mutexgear_rwlock_tryrdlock
*	\see mutexgear_completion_worker_lock
*/
_MUTEXGEAR_API int mutexgear_rwlock_rdlock(mutexgear_rwlock_t *__rwlock_instance,
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_waiter_t *__waiter_instance, mutexgear_completion_item_t *__item_instance);

_MUTEXGEAR_API int mutexgear_trdl_rwlock_rdlock(mutexgear_trdl_rwlock_t *__rwlock_instance,
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_waiter_t *__waiter_instance, mutexgear_completion_item_t *__item_instance);

#if defined(__cplusplus)
_MUTEXGEAR_END_EXTERN_C();

static inline
int mutexgear_rwlock_rdlock(mutexgear_trdl_rwlock_t *__rwlock_instance,
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_waiter_t *__waiter_instance, mutexgear_completion_item_t *__item_instance)
{
	return mutexgear_trdl_rwlock_rdlock(__rwlock_instance, __worker_instance, __waiter_instance, __item_instance);
}

_MUTEXGEAR_BEGIN_EXTERN_C();
#endif // #if defined(__cplusplus)

#if defined(MUTEXGEAR_USE_C11_GENERICS)
#define mutexgear_rwlock_rdlock(__rwlock_instance, __worker_instance, __waiter_instance, __item_instance) _Generic(__rwlock_instance, \
	mutexgear_trdl_rwlock_t *: mutexgear_trdl_rwlock_rdlock, \
	default: mutexgear_rwlock_rdlock)(__rwlock_instance, __worker_instance, __waiter_instance, __item_instance)
#endif // #if defined(MUTEXGEAR_USE_C11_GENERICS)


/**
*	\fn int mutexgear_rwlock_tryrdlock(mutexgear_trdl_rwlock_t *__rwlock_instance, mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_item_t *__item_instance)
*	\brief Tries to acquire the object read (shared) lock without blocking
*
*	The function is available for \c mutexgear_trdl_rwlock_t objects only.
*
*	The function may speculatively return EBUSY if there would be concurrent \c mutexgear_rwlock_trywrlock
*	calls from other threads even if those calls were going to fail due to the object being already read-locked 
*	by yet some other threads. Also, the call obeys write lock priority and will return EBUSY if there are
*	threads waiting to acquire the object for write.
*
*	The restrictions and requirements for \p __worker_instance, \p __waiter_instance objects and
*	\p __item_instance objects match those for \c mutexgear_rwlock_rdlock call.
*
*	In case of inter-process synchronization all the parameters must be allocated in the same
*	shared section as the \c rwlock object.
*
*	\return EOK on success, EBUSY if the object is already write-locked by another thread, or a system error code on failure.
*	\see mutexgear_rwlock_rdunlock
*	\see mutexgear_rwlock_rdlock
*	\see mutexgear_completion_worker_lock
*/
#ifdef __MUTEXGEAR_DOXYGEN__
_MUTEXGEAR_API int mutexgear_rwlock_tryrdlock(mutexgear_trdl_rwlock_t *__rwlock_instance,
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_item_t *__item_instance);
#endif
_MUTEXGEAR_API int mutexgear_trdl_rwlock_tryrdlock(mutexgear_trdl_rwlock_t *__rwlock_instance,
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_item_t *__item_instance);


#if defined(__cplusplus)
_MUTEXGEAR_END_EXTERN_C();

static inline
int mutexgear_rwlock_tryrdlock(mutexgear_trdl_rwlock_t *__rwlock_instance, 
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_item_t *__item_instance)
{
	return mutexgear_trdl_rwlock_tryrdlock(__rwlock_instance, __worker_instance, __item_instance);
}

_MUTEXGEAR_BEGIN_EXTERN_C();
#endif // #if defined(__cplusplus)

#if defined(MUTEXGEAR_USE_C11_GENERICS)
#define mutexgear_rwlock_tryrdlock(__rwlock_instance, __worker_instance, __item_instance) _Generic(__rwlock_instance, \
	mutexgear_trdl_rwlock_t *: mutexgear_trdl_rwlock_tryrdlock)(__rwlock_instance, __worker_instance, __item_instance)
#endif // #if defined(MUTEXGEAR_USE_C11_GENERICS)


/**
*	\fn mutexgear_rwlock_rdunlock(mutexgear_rwlock_t *__rwlock_instance, mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_item_t *__item_instance)
*	\brief Releases the previously acquired object read (shared) lock
*
*	The function can also be called for \c mutexgear_trdl_rwlock_t objects.
*
*	The \p __worker_instance and \p __item_instance must be the same objects that were used in the previous call
*	to \c mutexgear_rwlock_rdlock on the \c rwlock.
*	\return EOK on success or a system error code on failure.
*	\see mutexgear_rwlock_rdlock
*/
_MUTEXGEAR_API int mutexgear_rwlock_rdunlock(mutexgear_rwlock_t *__rwlock_instance,
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_item_t *__item_instance);

_MUTEXGEAR_API int mutexgear_trdl_rwlock_rdunlock(mutexgear_trdl_rwlock_t *__rwlock_instance,
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_item_t *__item_instance);

#if defined(__cplusplus)
_MUTEXGEAR_END_EXTERN_C();

static inline
int mutexgear_rwlock_rdunlock(mutexgear_trdl_rwlock_t *__rwlock_instance, 
	mutexgear_completion_worker_t *__worker_instance, mutexgear_completion_item_t *__item_instance)
{
	return mutexgear_trdl_rwlock_rdunlock(__rwlock_instance, __worker_instance, __item_instance);
}

_MUTEXGEAR_BEGIN_EXTERN_C();
#endif // #if defined(__cplusplus)

#if defined(MUTEXGEAR_USE_C11_GENERICS)
#define mutexgear_rwlock_rdunlock(__rwlock_instance, __worker_instance, __item_instance) _Generic(__rwlock_instance, \
	mutexgear_trdl_rwlock_t *: mutexgear_trdl_rwlock_rdunlock, \
	default: mutexgear_rwlock_rdunlock)(__rwlock_instance, __worker_instance, __item_instance)
#endif // #if defined(MUTEXGEAR_USE_C11_GENERICS)


//////////////////////////////////////////////////////////////////////////


_MUTEXGEAR_END_EXTERN_C();


#endif // #ifndef __MUTEXGEAR_RWLOCK_H_INCLUDED
