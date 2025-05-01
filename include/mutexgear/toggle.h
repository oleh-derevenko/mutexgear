#ifndef __MUTEXGEAR_TOGGLE_H_INCLUDED
#define __MUTEXGEAR_TOGGLE_H_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear Toggle API Definitions                                     */
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
 *	\brief MutexGear Toggle API definitions
 *
 *	The header defines a "toggle" object.
 *	The toggle implements the coordinated operation mode as described
 *	in the Patent specification.
 *
 *	The toggle object implementation comprises of two muteces treated as a loop
 *	and two iterator indices: one for the toggle side and one for client side --
 *	being incremented in cycle over the loop. The toggle side index is initialized 
 *	to point one step ahead of the client side index but only the toggle side 
 *	acquires its mutex. For the toggle side it's called "the engaged state".
 *
 *	An elementary step for the toggle side (a flip) is: acquiring the next mutex 
 *	after the one owned, releasing the mutex that was the current one by the time, 
 *	and cyclically incrementing the iterated index to move "the current" status 
 *	to the formerly acquired new mutex that was previously "the next one". 
 *	An elementary step for the client side (a push-on) is acquiring the next mutex 
 *	after the one pointed by its iterator index, releasing the acquired mutex, 
 *	and cyclically incrementing the iterated index. The elementary steps must 
 *	be executed with external coordination so that the toggle could not be flipped 
 *	two times in a row withot a client executing a push-on operation for the first 
 *	flip of the two.
 *	Since the toggle side is the only one owning a mutex and there is one mutex free,
 *	the toggle side is able to freely execute its step and that constitutes 
 *	signaling the object.
 *	For the reason of being right behind in the loop, the client side blocks 
 *	while trying to acquire its "next" mutex being unable to execute its 
 *	elementary step until the respective step is done by the toggle side and 
 *	this constitutes client's waiting for a signal on the object.
 *
 *	Toggle flip steps are to be done each time after there is a state change 
 *	that needs to be notified to clients. A client must push on the toggle
 *	each time a state change is anticipated to wait for the change and 
 *	keep the client side index in sync with the toggle side one.
 *
 *	Typical usage pattern of a toggle object is:
 *	\code{.c}
 *	// NOTE: Unlike the example code below, please DO NOT ignore the function invocation results.
 *	//
 *	// ************ In a context accessible to all below threads ************
 *	mutexgear_toggle_t toggleInstance;
 *	// Make sure toggleInstance is initialized after the declaration
 *	// and is destroyed later, after it has become unnecessary.
 *
 *	// ************************ In the toggle thread *************************
 *	mutexgear_toggle_engaged(&toggleInstance);
 *	// ...
 *	// Allow client access to the toggle
 *	// ...
 *	DoSomeWork(...);
 *	mutexgear_toggle_flipped(&toggleInstance);
 *	// ...
 *	// Use external means to make sure a client has pushed-on the toggle already
 *	// ...
 *	DoSomeMoreWork(...);
 *	mutexgear_toggle_flipped(&toggleInstance);
 *	// ...
 *	// Use external means to make sure a client has pushed-on the toggle already
 *	// ...
 *	// Make sure clients have stopped accessing the toggle
 *	// ...
 *	mutexgear_toggle_disengaged(&toggleInstance);
 *
 *	// ************************* In a client thread *************************
 *	// ...
 *
 *	// Waiting for the "SomeWork" to be done
 *	mutexgear_toggle_pushon(&toggleInstance);
 *
 *	// ...
 *
 *	// Waiting for the "SomeMoreWork" to be done
 *	mutexgear_toggle_pushon(&toggleInstance);
 *
 *	// ...
 *	\endcode
 *
 *	NOTE:
 *
 *	The "Toggle" set of functions declared in this file
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
 *	\struct mutexgear_toggleattr_t
 *	\brief An attributes structure used to define the attributes of a \c mutexgear_toggle_t object on creation
 */
typedef struct _mutexgear_toggleattr_t
{
	_MUTEXGEAR_LOCKATTR_T	mutexattr;

} mutexgear_toggleattr_t;

/**
 *	\fn int mutexgear_toggleattr_init(mutexgear_toggleattr_t *__attr)
 *	\brief A function to initialize a \c mutexgear_toggleattr_t instance.
 *
 *	The attributes are initialized with the defaults \c pthread_mutexattr_init would use.
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_toggleattr_getpshared
 *	\see mutexgear_toggleattr_setpshared
 *	\see mutexgear_toggleattr_getprioceiling
 *	\see mutexgear_toggleattr_getprotocol
 *	\see mutexgear_toggleattr_setprioceiling
 *	\see mutexgear_toggleattr_setprotocol
 *	\see mutexgear_toggleattr_setmutexattr
 *	\see mutexgear_toggle_init
 *	\see mutexgear_toggleattr_destroy
 */
_MUTEXGEAR_API int mutexgear_toggleattr_init(mutexgear_toggleattr_t *__attr);
/**
 *	\fn int mutexgear_toggleattr_destroy(mutexgear_toggleattr_t *__attr)
 *	\brief A function to destroy a previously initialized \c mutexgear_toggleattr_t
 *	instance and free any resources possibly allocated for it.
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_toggleattr_init
 */
_MUTEXGEAR_API int mutexgear_toggleattr_destroy(mutexgear_toggleattr_t *__attr);

/**
 *	\fn int mutexgear_toggleattr_getpshared(const mutexgear_toggleattr_t *__attr, int *__pshared)
 *	\brief A function to get process shared attribute value stored in a \c mutexgear_toggleattr_t
 *	structure (similar to the \c pthread_mutexattr_getpshared).
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_toggleattr_init
 */
_MUTEXGEAR_API int mutexgear_toggleattr_getpshared(const mutexgear_toggleattr_t *__attr, int *__pshared);
/**
 *	\fn int mutexgear_toggleattr_setpshared(mutexgear_toggleattr_t *__attr, int __pshared)
 *	\brief A function to assign process shared attribute value to a \c mutexgear_toggleattr_t
 *	structure (similar to the \c pthread_mutexattr_setpshared).
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_toggleattr_init
 */
_MUTEXGEAR_API int mutexgear_toggleattr_setpshared(mutexgear_toggleattr_t *__attr, int __pshared);

/**
 *	\fn int mutexgear_toggleattr_getprioceiling(const mutexgear_toggleattr_t *__attr, int *__prioceiling)
 *	\brief A function to query priority ceiling attribute value stored in a \c mutexgear_toggleattr_t
 *	structure (similar to the \c pthread_mutexattr_getprioceiling).
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_toggleattr_init
 */
_MUTEXGEAR_API int mutexgear_toggleattr_getprioceiling(const mutexgear_toggleattr_t *__attr, int *__prioceiling);
/**
 *	\fn int mutexgear_toggleattr_getprotocol(const mutexgear_toggleattr_t *__attr, int *__protocol)
 *	\brief A function to retrieve lock protocol attribute value stored in a \c mutexgear_toggleattr_t
 *	structure (similar to the \c pthread_mutexattr_getprotocol).
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_toggleattr_init
 */
_MUTEXGEAR_API int mutexgear_toggleattr_getprotocol(const mutexgear_toggleattr_t *__attr, int *__protocol);
/**
 *	\fn int mutexgear_toggleattr_setprioceiling(mutexgear_toggleattr_t *__attr, int __prioceiling)
 *	\brief A function to assign a priority ceiling value to a \c mutexgear_toggleattr_t
 *	structure (similar to the \c pthread_mutexattr_setprioceiling).
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_toggleattr_init
 */
_MUTEXGEAR_API int mutexgear_toggleattr_setprioceiling(mutexgear_toggleattr_t *__attr, int __prioceiling);
/**
 *	\fn int mutexgear_toggleattr_setprotocol(mutexgear_toggleattr_t *__attr, int __protocol)
 *	\brief A function to set a lock protocol value to a \c mutexgear_toggleattr_t
 *	structure (similar to the \c pthread_mutexattr_setprotocol).
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_toggleattr_init
 */
_MUTEXGEAR_API int mutexgear_toggleattr_setprotocol(mutexgear_toggleattr_t *__attr, int __protocol);
/**
 *	\fn int mutexgear_toggleattr_setmutexattr(mutexgear_toggleattr_t *__attr, const _MUTEXGEAR_LOCKATTR_T *__mutexattr)
 *	\brief A function to copy attributes from a mutex attributes structure to
 *	a \c mutexgear_toggleattr_t structure.
 *
 *	Values of "pshared", "protocol" and "prioceiling" are copied.
 *
 *	Due to inability to clear previously assigned priority ceiling setting from attributes on some targets
 *	it's recommended to call this function with freshly initialized \a __attr_instance only.
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_toggleattr_init
 */
_MUTEXGEAR_API int mutexgear_toggleattr_setmutexattr(mutexgear_toggleattr_t *__attr, const _MUTEXGEAR_LOCKATTR_T *__mutexattr);


enum
{
	MUTEXGEAR_TOGGLEELEMENT_DESTROYED = INT_MIN,
	// The commented elements below are occupied (implicitly used in code)
	// _MUTEXGEAR_TOGGLEELEMENT_PUSHON_SECOND = -2, == MUTEXGEAR_TOGGLEELEMENT_INVALID - _MUTEXGEAR_TOGGLEELEMENT_SECOND
	// _MUTEXGEAR_TOGGLEELEMENT_PUSHON_FIRST = -1, == MUTEXGEAR_TOGGLEELEMENT_INVALID - _MUTEXGEAR_TOGGLEELEMENT_FIRST
	MUTEXGEAR_TOGGLEELEMENT_INVALID = -1,
	_MUTEXGEAR_TOGGLEELEMENT_FIRST = 0,
	// _MUTEXGEAR_TOGGLEELEMENT_SECOND = 1,

	MUTEXGEAR_TOGGLE_NUMELEMENTS = 2,
	MUTEXGEAR_TOGGLEELEMENT_ATTACH_FIRST = _MUTEXGEAR_TOGGLEELEMENT_FIRST,
};

/**
 *	\struct mutexgear_toggle_t
 *	\brief The \c toggle object structure.
 */
typedef struct _mutexgear_toggle_t
{
	_MUTEXGEAR_LOCK_T	muteces[MUTEXGEAR_TOGGLE_NUMELEMENTS];
	int					thumb_position;
	int					push_position;

} mutexgear_toggle_t;

/**
 *	\def MUTEXGEAR_TOGGLE_INITIALIZER
 *	\brief A \c toggle object in-place static initializer (similar to \c PTHREAD_MUTEX_INITIALIZER).
 */
#define MUTEXGEAR_TOGGLE_INITIALIZER	{ { PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER }, MUTEXGEAR_TOGGLEELEMENT_INVALID }
MG_STATIC_ASSERT(MUTEXGEAR_TOGGLE_NUMELEMENTS == 2); // Fix MUTEXGEAR_TOGGLE_INITIALIZER to match the actual number of elements, then update the assertion check


/**
 *	\fn int mutexgear_toggle_init(mutexgear_toggle_t *__toggle, const mutexgear_toggleattr_t *__attr)
 *	\brief A function to create a \c toggle object with attributes defined in the \c mutexgear_toggleattr_t instance.
 *
 *	\param __attr NULL or attributes to be used to create the object
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_toggleattr_init
 *	\see mutexgear_toggle_engaged
 *	\see mutexgear_toggle_pushon
 *	\see mutexgear_toggle_destroy
 */
_MUTEXGEAR_API int mutexgear_toggle_init(mutexgear_toggle_t *__toggle, const mutexgear_toggleattr_t *__attr/*=NULL*/);
/**
 *	\fn int mutexgear_toggle_destroy(mutexgear_toggle_t *__toggle)
 *	\brief A function to destroy a \c toggle instance and release any resources that might be allocated for it.
 *
 *	The toggle must not be in the engaged state, or the function will fail.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_toggle_init
 *	\see mutexgear_toggle_disengaged
 */
_MUTEXGEAR_API int mutexgear_toggle_destroy(mutexgear_toggle_t *__toggle);


/**
 *	\fn int mutexgear_toggle_engaged(mutexgear_toggle_t *__toggle)
 *	\brief A function to attach to the \c toggle to be called
 *	by the signaler (the S) thread to accomplish the Precondition 1.
 *
 *	This function is to be called by the toggle host thread before any 
 *	client side access is performed to bring the object into the engaged state 
 *	and make it ready for use. Typically, the call is performed at 
 *	the host thread start.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_toggle_flipped
 *	\see mutexgear_toggle_disengaged
 */
_MUTEXGEAR_API int mutexgear_toggle_engaged(mutexgear_toggle_t *__toggle);
/**
 *	\fn int mutexgear_toggle_flipped(mutexgear_toggle_t *__toggle)
 *	\brief A function to be called by the signaler (the S) thread
 *	to notify a potential waiter of an event. This corresponds
 *	to the steps 1-3 of the "Event Signaling" section.
 *
 *	This function is to be called by the toggle host thread each time after
 *	an anticipated state change occurs to flip the toggle and thus, 
 *	signal the object for the client.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_toggle_engaged
 *	\see mutexgear_toggle_disengaged
 *	\see mutexgear_toggle_pushon
 */
_MUTEXGEAR_API int mutexgear_toggle_flipped(mutexgear_toggle_t *__toggle);
/**
 *	\fn int mutexgear_toggle_disengaged(mutexgear_toggle_t *__toggle)
 *	\brief A function to be called by the signaler (the S) thread
 *	to release the \c toggle object whenever it is not going to be used any more.
 *
 *	The function is to be called by the toggle host thread after the object
 *	has been finished to be used to release it and prepare for destruction. Typically, 
 *	the call is performed at the host thread end. The object can still be brought 
 *	back into the engaged state by calling \c mutexgear_toggle_engaged.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_toggle_engaged
 */
_MUTEXGEAR_API int mutexgear_toggle_disengaged(mutexgear_toggle_t *__toggle);

/**
 *	\fn int mutexgear_toggle_pushon(mutexgear_toggle_t *__toggle)
 *	\brief A function to be called by the waiter (the W) thread
 *	to block waiting for an event notification. This corresponds
 *	to the steps 1-3 of the "Event Waiting" section.
 *
 *	The function is to be called by a toggle client to wait for an anticipated 
 *	state change. If the state change has not been signaled yet, the call will block.
 *
 *	The function may be called only on a toggle that was brought into 
 *	the engaged state by the host thread.
 *
 *	\return EOK on success or a system error code on failure.
 *	\see mutexgear_toggle_engaged
 *	\see mutexgear_toggle_flipped
 *	\see mutexgear_toggle_disengaged
 */
_MUTEXGEAR_API int mutexgear_toggle_pushon(mutexgear_toggle_t *__toggle);


//////////////////////////////////////////////////////////////////////////


/*
 *	NOTE:
 *
 *	The "Toggle" set of functions declared in this file
 *	implements a synchronization mechanism being a subject
 *	of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and Image Database search
 *	(currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm) to view the patent text.
 */

_MUTEXGEAR_END_EXTERN_C();


#endif // #ifndef __MUTEXGEAR_TOGGLE_H_INCLUDED
