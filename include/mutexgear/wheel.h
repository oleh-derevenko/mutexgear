#ifndef __MUTEXGEAR_WHEEL_H_INCLUDED
#define __MUTEXGEAR_WHEEL_H_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear Wheel API Definitions                                      */
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
 *	\brief MutexGear Wheel API definitions
 *
 *	The header defines a "wheel" object.
 *	The wheel implements independent operation mode as described
 *	in the Patent specification.
 *
 *	NOTE:
 *
 *	The "Wheel" set of functions declared in this file
 *	implements a synchronization mechanism being a subject
 *	of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and Image Database search
 *	(currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm) to view the patent text.
 */


#include <mutexgear/config.h>
#include <limits.h>


_MUTEXGEAR_BEGIN_EXTERN_C();

//////////////////////////////////////////////////////////////////////////

/**
 *	\typedef mutexgear_wheelattr_t
 *	\brief An attributes structure used to define the attributes of a \c wheel object on creation.
 */
typedef struct _mutexgear_wheelattr_t
{
	_MUTEXGEAR_LOCKATTR_T	mutexattr;

} mutexgear_wheelattr_t;


/**
 *	\fn int mutexgear_wheelattr_init(mutexgear_wheelattr_t *__attr)
 *	\brief A function to initialize a \c mutexgear_wheelattr_t instance.
 *	\return EOK on success or a system error code on failure.
 */
_MUTEXGEAR_API int mutexgear_wheelattr_init(mutexgear_wheelattr_t *__attr);
/**
 *	\fn int mutexgear_wheelattr_destroy(mutexgear_wheelattr_t *__attr)
 *	\brief A function to destroy a previously initialized \c mutexgear_wheelattr_t
 *	instance and free any resources possibly allocated for it.
 *	\return EOK on success or a system error code on failure.
 */
_MUTEXGEAR_API int mutexgear_wheelattr_destroy(mutexgear_wheelattr_t *__attr);

/**
 *	\fn int mutexgear_wheelattr_getpshared(const mutexgear_wheelattr_t *__attr, int *__pshared)
 *	\brief A function to get process shared attribute value stored in
 *	a \c mutexgear_wheelattr_t structure (similar to the \c pthread_mutexattr_getpshared).
 *	\return EOK on success or a system error code on failure.
 */
_MUTEXGEAR_API int mutexgear_wheelattr_getpshared(const mutexgear_wheelattr_t *__attr, int *__pshared);
/*
*	\fn int mutexgear_wheelattr_setpshared(mutexgear_wheelattr_t *__attr, int __pshared)
*	\brief A function to assign process shared attribute value to
*	a \c mutexgear_wheelattr_t structure (similar to \c the pthread_mutexattr_setpshared).
 *	\return EOK on success or a system error code on failure.
*/
_MUTEXGEAR_API int mutexgear_wheelattr_setpshared(mutexgear_wheelattr_t *__attr, int __pshared);

/**
 *	\fn int mutexgear_wheelattr_getprioceiling(const mutexgear_wheelattr_t *__attr, int *__prioceiling)
 *	\brief A function to query priority ceiling attribute value
 *	stored in a \c mutexgear_wheelattr_t structure (similar to the \c pthread_mutexattr_getprioceiling).
 *	\return EOK on success or a system error code on failure.
 */
_MUTEXGEAR_API int mutexgear_wheelattr_getprioceiling(const mutexgear_wheelattr_t *__attr, int *__prioceiling);
/**
 *	\fn int mutexgear_wheelattr_getprotocol(const mutexgear_wheelattr_t *__attr, int *__protocol)
 *	\brief A function to retrieve lock protocol attribute value
 *	stored in a \c mutexgear_wheelattr_t structure (similar to the \c pthread_mutexattr_getprotocol).
 *	\return EOK on success or a system error code on failure.
 */
_MUTEXGEAR_API int mutexgear_wheelattr_getprotocol(const mutexgear_wheelattr_t *__attr, int *__protocol);
/**
 *	\fn int mutexgear_wheelattr_setprioceiling(mutexgear_wheelattr_t *__attr, int __prioceiling)
 *	\brief A function to assign a priority ceiling value to
 *	a \c mutexgear_wheelattr_t structure (similar to the \c pthread_mutexattr_setprioceiling).
 *	\return EOK on success or a system error code on failure.
 */
_MUTEXGEAR_API int mutexgear_wheelattr_setprioceiling(mutexgear_wheelattr_t *__attr, int __prioceiling);
/**
 *	\fn int mutexgear_wheelattr_setprotocol(mutexgear_wheelattr_t *__attr, int __protocol)
 *	\brief A function to set a lock protocol value to
 *	a \c mutexgear_wheelattr_t structure (similar to the \c pthread_mutexattr_setprotocol).
 *	\return EOK on success or a system error code on failure.
 */
_MUTEXGEAR_API int mutexgear_wheelattr_setprotocol(mutexgear_wheelattr_t *__attr, int __protocol);
/**
 *	\fn int mutexgear_wheelattr_setmutexattr(mutexgear_wheelattr_t *__attr, const _MUTEXGEAR_LOCKATTR_T *__mutexattr)
 *	\brief A function to copy attributes from a mutex attributes structure to
 *	a \c mutexgear_wheelattr_t structure.
 *
 *	Values of "pshared", "protocol" and "prioceiling" are copied.
 *
 *	Due to inability to clear previously assigned priority ceiling setting from attributes on some targets
 *	it's recommended to call this function with freshly initialized \a __attr_instance only.
 *	\return EOK on success or a system error code on failure.
 */
_MUTEXGEAR_API int mutexgear_wheelattr_setmutexattr(mutexgear_wheelattr_t *__attr, const _MUTEXGEAR_LOCKATTR_T *__mutexattr);


enum
{
	MUTEXGEAR_WHEELELEMENT_DESTROYED = INT_MIN,
	// The commented elements below are occupied (implicitly used in code)
	// _MUTEXGEAR_WHEELELEMENT_PUSHON_THIRD = -3, == MUTEXGEAR_WHEELELEMENT_INVALID - _MUTEXGEAR_WHEELELEMENT_THIRD
	// _MUTEXGEAR_WHEELELEMENT_PUSHON_SECOND = -2, == MUTEXGEAR_WHEELELEMENT_INVALID - _MUTEXGEAR_WHEELELEMENT_SECOND
	// _MUTEXGEAR_WHEELELEMENT_PUSHON_FIRST = -1, == MUTEXGEAR_WHEELELEMENT_INVALID - _MUTEXGEAR_WHEELELEMENT_FIRST
	MUTEXGEAR_WHEELELEMENT_INVALID = -1,
	_MUTEXGEAR_WHEELELEMENT_FIRST = 0,
	// _MUTEXGEAR_WHEELELEMENT_SECOND      = 1,
	// _MUTEXGEAR_WHEELELEMENT_THIRD       = 2,

	MUTEXGEAR_WHEEL_NUMELEMENTS = 3,
	MUTEXGEAR_WHEELELEMENT_ATTACH_FIRST = _MUTEXGEAR_WHEELELEMENT_FIRST,
};

/**
 *	\typedef mutexgear_wheel_t
 *	\brief The wheel object structure.
 */
typedef struct _mutexgear_wheel_t
{
	_MUTEXGEAR_LOCK_T	muteces[MUTEXGEAR_WHEEL_NUMELEMENTS];
	int					slave_index;
	int					master_index;

} mutexgear_wheel_t;

/**
 *	\def MUTEXGEAR_WHEEL_INITIALIZER
 *	\brief A \c wheel object in-place static initializer (similar to \c PTHREAD_MUTEX_INITIALIZER).
 */
#define MUTEXGEAR_WHEEL_INITIALIZER	{ { PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER }, MUTEXGEAR_WHEELELEMENT_INVALID, MUTEXGEAR_WHEELELEMENT_INVALID }


/**
 *	\fn int mutexgear_wheel_init(mutexgear_wheel_t *__wheel, const mutexgear_wheelattr_t *__attr)
 *	\brief A function to create a \c wheel object with attributes defined in the \c mutexgear_wheelattr_t instance.
 */
_MUTEXGEAR_API int mutexgear_wheel_init(mutexgear_wheel_t *__wheel, const mutexgear_wheelattr_t *__attr);
/**
 *	\fn int mutexgear_wheel_destroy(mutexgear_wheel_t *__wheel)
 *	\brief A function to destroy a \c wheel instance and release any resources
 *	that might be allocated for it.
 */
_MUTEXGEAR_API int mutexgear_wheel_destroy(mutexgear_wheel_t *__wheel);


/**
 *	\fn int mutexgear_wheel_lockslave(mutexgear_wheel_t *__wheel)
 *	\brief A function to attach to the wheel to be called by the signaler (the S) thread
 *	to accomplish the independent operation Precondition 1.
 */
_MUTEXGEAR_API int mutexgear_wheel_lockslave(mutexgear_wheel_t *__wheel);
/**
 *	\fn int mutexgear_wheel_slaveroll(mutexgear_wheel_t *__wheel)
 *	\brief A function to be called by the signaler (the S) thread
 *	to notify a potential waiter of an event. This corresponds to
 *	the step 2.-Option_2 of the "Event Signaling" section
 *	(to be called after changing the V predicate to an "event happened" meaning).
 */
_MUTEXGEAR_API int mutexgear_wheel_slaveroll(mutexgear_wheel_t *__wheel);
/**
 *	\fn int mutexgear_wheel_unlockslave(mutexgear_wheel_t *__wheel)
 *	\brief A function to be called by the signaler (the S) thread
 *	to release the \c wheel object whenever it is not going to be used any more.
 */
_MUTEXGEAR_API int mutexgear_wheel_unlockslave(mutexgear_wheel_t *__wheel);

/**
 *	\fn int mutexgear_wheel_gripon(mutexgear_wheel_t *__wheel)
 *	\brief A function to be called by a waiter (the W) thread
 *	to initiate a waiting operation. This corresponds to the step 1 of the "Event Waiting" section.
 */
_MUTEXGEAR_API int mutexgear_wheel_gripon(mutexgear_wheel_t *__wheel);
/**
 *	\fn int mutexgear_wheel_turn(mutexgear_wheel_t *__wheel)
 *	\brief A function to be called by the waiter (the W) thread
 *	to re-lock serializing synchronization to the next object as described
 *	in the step 2 of the "Event Waiting" section.
 *
 *	This is to be done in a loop each time after evaluating the V predicate
 *	unless and until its value indicates the event occurrence.
 */
_MUTEXGEAR_API int mutexgear_wheel_turn(mutexgear_wheel_t *__wheel);
/**
 *	\fn int mutexgear_wheel_release(mutexgear_wheel_t *__wheel)
 *	\brief A function to be called by the waiter (the W) thread
 *	to end the event waiting operation releasing the currently locked
 *	serializing synchronization object as described in the step 3
 *	of the "Event Waiting" section.
 */
_MUTEXGEAR_API int mutexgear_wheel_release(mutexgear_wheel_t *__wheel);

/**
 *	\fn int mutexgear_wheel_pushon(mutexgear_wheel_t *__wheel)
 *	\brief A compatibility function to use a \c wheel object in coordinated mode
 *	replicating a \c toggle object's functionality. The synchronization effects
 *	this function achieves with the \c wheel object are the same as
 *	the \c mutexgear_toggle_pushon function with a \c toggle object would do.
 */
_MUTEXGEAR_API int mutexgear_wheel_pushon(mutexgear_wheel_t *__wheel);


//////////////////////////////////////////////////////////////////////////


_MUTEXGEAR_END_EXTERN_C();


/*
 *	NOTE:
 *
 *	The "Wheel" set of functions declared in this file
 *	implements a synchronization mechanism being a subject
 *	of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and Image Database search
 *	(currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm) to view the patent text.
 */

#endif // #ifndef __MUTEXGEAR_WHEEL_H_INCLUDED
