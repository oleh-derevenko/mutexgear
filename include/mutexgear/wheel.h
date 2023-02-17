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
/* Copyright (c) 2016-2023 Oleh Derevenko. All rights are reserved.     */
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
 *	The wheel object implementation comprises of three muteces treated as a loop
 *	and two iterator indices: one for the wheel side and one for client side --
 *	being incremented in cycle over the loop. The wheel side index is initialized 
 *	to point one step ahead of the client side index and each of the sides has 
 *	an acquired mutex as defined by its respective index. For the wheel side 
 * 	it's called "the engaged state" and for the client it's "the gripped on state". 
 *	A client in gripped on state (the client's presence) is optional at the object 
 *	and clients (one at a time) can share a wheel by gripping on and releasing it.
 *
 *	An elementary step for each side is: acquiring the next mutex after the one 
 *	owned, releasing the mutex that was the current one by the time, and 
 *	cyclically incrementing the iterated index to move "the current" status to 
 *	the formerly acquired new mutex that was previously "the next one". 
 *	Since the wheel side is one step ahead and there is one mutex free, 
 *	the wheel side is able to freely execute its step and that constitutes 
 *	signaling the object. For the reason of being right behind in the loop, 
 *	the client side will block and will not be able to execute its elementary step 
 *	until that is done by the wheel side and this constitutes client's waiting 
 *	for a signal on the object.
 *	Since there is at most one mutex free, the wheel cannot issue two signals 
 *	in a row unless the client completes its elementary step and wakes up from 
 *	its wait having "received" the first signal, or unless the wheel is 
 *	operating without a client gripped on at the moment.
 *
 *	Wheel advance steps are to be done each time after there is a state change 
 *	that could need to be notified to clients. A client is to grip on, check whether
 *	its condition of interest has not already occurred, start turning the wheel
 *	and re-checking the condition after each step, and release the wheel when
 *	the condition was found to have become true.
 *
 *	Typical usage pattern of a wheel object is:
 *	\code{.c}
*	// NOTE: Unlike the example code below, please DO NOT ignore the function invocation results.
 *	//
  *	// ************ In a context accessible to all below threads ************
 *	mutexgear_wheel_t wheelInstance;
 *	// Make sure wheelInstance is initialized after the declaration 
 *	// and is destroyed later, after it has become unnecessary.
 *
 *	// ************************ In the wheel thread *************************
 *	mutexgear_wheel_engaged(&wheelInstance);
 *	// ...
 *	// Allow client access to the wheel
 *	// ...
 *	ChangeStateInSomeWay(...);
 *	mutexgear_wheel_advanced(&wheelInstance);
 *	// ...
 *	ChangeStateInYetSomeOtherWay(...);
 *	mutexgear_wheel_advanced(&wheelInstance);
 *	// ...
 *	// Make sure clients stopped accessing the wheel
 *	// ...
 *	mutexgear_wheel_disengaged(&wheelInstance);
 *
 *	// *** In a client thread (serialized wrt. other clients of the wheel) ***
 *	// *** Waiting for the "YetSomeOther" state                            ***
 *
 *	bool continueIntoTheLoop, enterTheLoop = !TestTheYetSomeOtherStateIsRealized(...);
 *
 *	if (enterTheLoop)
 *	{
 *		mutexgear_wheel_gripon(&wheelInstance);
 *	}
 *
 *	for (continueIntoTheLoop = enterTheLoop; continueIntoTheLoop; continueIntoTheLoop = 1)
 *	{
 *		if (TestTheYetSomeOtherStateIsRealized(...))
 *		{
 *			break;
 *		}
 *
 *		mutexgear_wheel_turn(&wheelInstance);
 *	}
 *
 *	if (enterTheLoop)
 *	{
 *		mutexgear_wheel_release(&wheelInstance);
 *	}
 *	\endcode
 *
 *	Wheel object can also be used in toggle object compatibility mode to avoid 
 *	creating a toggle when there is an unused wheel available for the time.
 *
 *	NOTE:
 *
 *	The "Wheel" set of functions declared in this file
 *	implements a synchronization mechanism being a subject
 *	of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and Image Database search
 *	(currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm) to view the patent text.
 */


#include <mutexgear/config.h>
#include <mutexgear/utility.h>
#include <limits.h>


_MUTEXGEAR_BEGIN_EXTERN_C();

//////////////////////////////////////////////////////////////////////////

/**
 *	\struct mutexgear_wheelattr_t
 *	\brief An attributes structure used to define the attributes of a \c mutexgear_wheel_t object on creation.
 */
typedef struct _mutexgear_wheelattr_t
{
	_MUTEXGEAR_LOCKATTR_T	mutexattr;

} mutexgear_wheelattr_t;


/**
 *	\fn int mutexgear_wheelattr_init(mutexgear_wheelattr_t *__attr)
 *	\brief A function to initialize a \c mutexgear_wheelattr_t instance.
 *	\return EOK on success or a system error code on failure.
 *
 *	The attributes are initialized with the defaults \c pthread_mutexattr_init would use.
 *	\see mutexgear_wheelattr_getpshared
 *	\see mutexgear_wheelattr_setpshared
 *	\see mutexgear_wheelattr_getprioceiling
 *	\see mutexgear_wheelattr_getprotocol
 *	\see mutexgear_wheelattr_setprioceiling
 *	\see mutexgear_wheelattr_setprotocol
 *	\see mutexgear_wheelattr_setmutexattr
 *	\see mutexgear_wheel_init
 *	\see mutexgear_wheelattr_destroy
 */
_MUTEXGEAR_API int mutexgear_wheelattr_init(mutexgear_wheelattr_t *__attr);
/**
 *	\fn int mutexgear_wheelattr_destroy(mutexgear_wheelattr_t *__attr)
 *	\brief A function to destroy a previously initialized \c mutexgear_wheelattr_t
 *	instance and free any resources possibly allocated for it.
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_wheelattr_init
 */
_MUTEXGEAR_API int mutexgear_wheelattr_destroy(mutexgear_wheelattr_t *__attr);

/**
 *	\fn int mutexgear_wheelattr_getpshared(const mutexgear_wheelattr_t *__attr, int *__pshared)
 *	\brief A function to get process shared attribute value stored in
 *	a \c mutexgear_wheelattr_t structure (similar to the \c pthread_mutexattr_getpshared).
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_wheelattr_init
 */
_MUTEXGEAR_API int mutexgear_wheelattr_getpshared(const mutexgear_wheelattr_t *__attr, int *__pshared);
/**
*	\fn int mutexgear_wheelattr_setpshared(mutexgear_wheelattr_t *__attr, int __pshared)
*	\brief A function to assign process shared attribute value to
*	a \c mutexgear_wheelattr_t structure (similar to \c the pthread_mutexattr_setpshared).
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_wheelattr_init
*/
_MUTEXGEAR_API int mutexgear_wheelattr_setpshared(mutexgear_wheelattr_t *__attr, int __pshared);

/**
 *	\fn int mutexgear_wheelattr_getprioceiling(const mutexgear_wheelattr_t *__attr, int *__prioceiling)
 *	\brief A function to query priority ceiling attribute value
 *	stored in a \c mutexgear_wheelattr_t structure (similar to the \c pthread_mutexattr_getprioceiling).
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_wheelattr_init
 */
_MUTEXGEAR_API int mutexgear_wheelattr_getprioceiling(const mutexgear_wheelattr_t *__attr, int *__prioceiling);
/**
 *	\fn int mutexgear_wheelattr_getprotocol(const mutexgear_wheelattr_t *__attr, int *__protocol)
 *	\brief A function to retrieve lock protocol attribute value
 *	stored in a \c mutexgear_wheelattr_t structure (similar to the \c pthread_mutexattr_getprotocol).
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_wheelattr_init
 */
_MUTEXGEAR_API int mutexgear_wheelattr_getprotocol(const mutexgear_wheelattr_t *__attr, int *__protocol);
/**
 *	\fn int mutexgear_wheelattr_setprioceiling(mutexgear_wheelattr_t *__attr, int __prioceiling)
 *	\brief A function to assign a priority ceiling value to
 *	a \c mutexgear_wheelattr_t structure (similar to the \c pthread_mutexattr_setprioceiling).
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_wheelattr_init
 */
_MUTEXGEAR_API int mutexgear_wheelattr_setprioceiling(mutexgear_wheelattr_t *__attr, int __prioceiling);
/**
 *	\fn int mutexgear_wheelattr_setprotocol(mutexgear_wheelattr_t *__attr, int __protocol)
 *	\brief A function to set a lock protocol value to
 *	a \c mutexgear_wheelattr_t structure (similar to the \c pthread_mutexattr_setprotocol).
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_wheelattr_init
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
 *	\see mutexgear_wheelattr_init
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
 *	\struct mutexgear_wheel_t
 *	\brief The \c wheel object structure.
 */
typedef struct _mutexgear_wheel_t
{
	_MUTEXGEAR_LOCK_T	muteces[MUTEXGEAR_WHEEL_NUMELEMENTS];
	int					wheel_side_index;
	int					client_side_index;

} mutexgear_wheel_t;

/**
 *	\def MUTEXGEAR_WHEEL_INITIALIZER
 *	\brief A \c wheel object in-place static initializer (similar to \c PTHREAD_MUTEX_INITIALIZER).
 */
#define MUTEXGEAR_WHEEL_INITIALIZER	{ { PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER }, MUTEXGEAR_WHEELELEMENT_INVALID, MUTEXGEAR_WHEELELEMENT_INVALID }
MG_STATIC_ASSERT(MUTEXGEAR_WHEEL_NUMELEMENTS == 3); // Fix MUTEXGEAR_WHEEL_INITIALIZER to match the actual number of elements, then update the assertion check


/**
 *	\fn int mutexgear_wheel_init(mutexgear_wheel_t *__wheel, const mutexgear_wheelattr_t *__attr)
 *	\brief A function to create a \c wheel object with attributes defined in the \c mutexgear_wheelattr_t instance.
 *
 *	\param __attr NULL or attributes to be used to create the object
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_wheelattr_init
 *	\see mutexgear_wheel_engaged
 *	\see mutexgear_wheel_gripon
 *	\see mutexgear_wheel_pushon
 *	\see mutexgear_wheel_destroy
 */
_MUTEXGEAR_API int mutexgear_wheel_init(mutexgear_wheel_t *__wheel, const mutexgear_wheelattr_t *__attr/*=NULL*/);
/**
 *	\fn int mutexgear_wheel_destroy(mutexgear_wheel_t *__wheel)
 *	\brief A function to destroy a \c wheel instance and release any resources
 *	that might be allocated for it.
 *
 *	The wheel must be neither in the engaged state nor in the gripped-on state, or the function will fail.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_wheel_init
 *	\see mutexgear_wheel_disengaged
 *	\see mutexgear_wheel_release
 */
_MUTEXGEAR_API int mutexgear_wheel_destroy(mutexgear_wheel_t *__wheel);


/**
 *	\fn int mutexgear_wheel_engaged(mutexgear_wheel_t *__wheel)
 *	\brief A function to attach to the wheel to be called by the signaler (the S) thread
 *	to accomplish the independent operation Precondition 1.
 *
 *	This function is to be called by the wheel host thread before any 
 *	client side access is performed to bring the object into the engaged state 
 *	and make it ready for use. Typically, the call is performed at 
 *	the host thread start.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_wheel_advanced
 *	\see mutexgear_wheel_disengaged
 */
_MUTEXGEAR_API int mutexgear_wheel_engaged(mutexgear_wheel_t *__wheel);
/**
 *	\fn int mutexgear_wheel_advanced(mutexgear_wheel_t *__wheel)
 *	\brief A function to be called by the signaler (the S) thread
 *	to notify a potential waiter of an event. This corresponds to
 *	the step 2.-Option_2 of the "Event Signaling" section
 *	(to be called after changing the V predicate to an "event happened" meaning).
 *
 *	This function is to be called by the wheel host thread each time after
 *	a state change that could need to be notified to clients occurs. The function 
 *	advances the wheel and thus, signals the object for any client that might 
 *	be gripped on and executing a turn or might be doing toggle compatibility 
 *	mode push-on.
 *
 *	The function may block for a short time if there is a client gripped on
 *	and it had not been given a chance to make its call to \c mutexgear_wheel_turn
 *	and/or \c mutexgear_wheel_release yet. Note that these calls will not be blocking
 * for the client.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_wheel_engaged
 *	\see mutexgear_wheel_disengaged
 *	\see mutexgear_wheel_turn
 *	\see mutexgear_wheel_pushon
 */
_MUTEXGEAR_API int mutexgear_wheel_advanced(mutexgear_wheel_t *__wheel);
/**
 *	\fn int mutexgear_wheel_disengaged(mutexgear_wheel_t *__wheel)
 *	\brief A function to be called by the signaler (the S) thread
 *	to release the \c wheel object whenever it is not going to be used any more.
 *
 *	The function is to be called by the wheel host thread after the object
 *	has been finished to be used to release it and prepare for destruction. Typically, 
 *	the call is performed at the host thread end. The object can still be brought 
 *	back into the engaged state by calling \c mutexgear_wheel_engaged.
 *
 *	Calling the function without makind sure the state change predicate was set, 
 *	while there is a client thread still performing 
 *	\c mutexgear_wheel_gripon - \c mutexgear_wheel_turn - \c mutexgear_wheel_release
 *	call sequence, will result in endless loop for the client.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_wheel_engaged
 */
_MUTEXGEAR_API int mutexgear_wheel_disengaged(mutexgear_wheel_t *__wheel);

/**
 *	\fn int mutexgear_wheel_gripon(mutexgear_wheel_t *__wheel)
 *	\brief A function to be called by a waiter (the W) thread
 *	to initiate a waiting operation. This corresponds to the step 1 of the "Event Waiting" section.
 *
 *	This function is to be called by a client that would like to wait on the object 
 *	before performing the first anticipated state change predicate check.
 *	After the function call succeeds, if the predicate evaluates to 'false' the 
 *	client should proceed to calling \c mutexgear_wheel_turn; otherwise, if the 
 *	predicate evaluates to 'true', the client should proceed to calling \c mutexgear_wheel_release
 *	without any calls to \c mutexgear_wheel_turn.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_wheel_turn
 *	\see mutexgear_wheel_release
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
 *
 *	This function is to be called by the client that has previously gripped on the wheel
 *	with \c mutexgear_wheel_gripon and checked that the state change predicate evaluates
 *	to 'false'. The function will block until the wheel side calls \c mutexgear_wheel_advanced
 *	to signal a state change. The function is to be continued to be called as long as the 
 *	desired predicate remains 'false'. After it has changed to 'true' the client should 
 *	proceed to calling \c mutexgear_wheel_release.
 *
 *	The function may also immediately exit one time without blocking if the previous call 
 *	was to \c mutexgear_wheel_gripon. In this case, continuing with its usual routine, 
 *	the client, dependign on the predicate evaluation result, will either call \c mutexgear_wheel_turn again
 *	and block there, or it will proceed to calling \c mutexgear_wheel_release.
 *
 *	Executing the \c mutexgear_wheel_gripon - \c mutexgear_wheel_turn - \c mutexgear_wheel_release 
 *	sequence while the predicate is evaluating to 'false' and without the wheel host thread 
 *	having the object in engaged state will result to infinite loop for the client.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_wheel_gripon
 *	\see mutexgear_wheel_release
 *	\see mutexgear_wheel_advanced
 */
_MUTEXGEAR_API int mutexgear_wheel_turn(mutexgear_wheel_t *__wheel);
/**
 *	\fn int mutexgear_wheel_release(mutexgear_wheel_t *__wheel)
 *	\brief A function to be called by the waiter (the W) thread
 *	to end the event waiting operation releasing the currently locked
 *	serializing synchronization object as described in the step 3
 *	of the "Event Waiting" section.
 *
 *	The function is to be called by the client either immediately after \c mutexgear_wheel_gripon
 *	or after one or more calls to \c mutexgear_wheel_turn, after the state change predicate 
 *	evaluates to 'true'. After the function call, the anticipated state change waiting is complete.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_wheel_gripon
 *	\see mutexgear_wheel_turn
 */
_MUTEXGEAR_API int mutexgear_wheel_release(mutexgear_wheel_t *__wheel);

/**
 *	\fn int mutexgear_wheel_pushon(mutexgear_wheel_t *__wheel)
 *	\brief A compatibility function to use a \c wheel object in coordinated mode
 *	replicating a \c toggle object's functionality. The synchronization effects
 *	this function achieves with the \c wheel object are the same as
 *	the \c mutexgear_toggle_pushon function with a \c toggle object would do.
 *
 *	The function is to be used by a client when the wheel object is to be used 
 *	instead of a \c mutexgear_toggle_t object. This way the code can save 
 *	on allocating another toggle object if there is a free wheel available for 
 *	the duration of use.
 *
 *	The wheel must not be gripped on while being used in toggle mode and 
 *	all the regular toggle object client considerations and restrictions apply.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_wheel_advanced
 *	\see mutexgear_toggle_t
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
