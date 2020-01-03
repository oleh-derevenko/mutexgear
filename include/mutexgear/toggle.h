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
/* THIS IS A PRE-RELEASE LIBRARY SNAPSHOT FOR EVALUATION PURPOSES ONLY. */
/* AWAIT THE RELEASE AT https://mutexgear.com                           */
/*                                                                      */
/* Copyright (c) 2016-2020 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/

/**
 *	\file
 *	\brief MutexGear Toggle API Definitions
 *
 *	The header defines a "toggle" object.
 *	The toggle implements the coordinated operation mode as described
 *	in the Patent specification.
 *
 *	NOTE:
 *
 *	The "Toggle" set of functions declared in this file
 *	implements a synchronization mechanism being a subject
 *	of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and Image Database search
 *	(currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm) to view the patent text.
 */


#include <mutexgear/config.h>
#include <limits.h>


_MUTEXGEAR_BEGIN_EXTERN_C();

//////////////////////////////////////////////////////////////////////////

/**
 *	\typedef mutexgear_toggleattr_t
 *	\brief An attributes structure used to define the attributes of a \c toggle object on creation
 */
typedef struct _mutexgear_toggleattr_t
{
	_MUTEXGEAR_LOCKATTR_T	mutexattr;

} mutexgear_toggleattr_t;

/**
 *	\fn int mutexgear_toggleattr_init(mutexgear_toggleattr_t *__attr)
 *	\brief A function to initialize a \c mutexgear_toggleattr_t instance.
 */
_MUTEXGEAR_API int mutexgear_toggleattr_init(mutexgear_toggleattr_t *__attr);
/**
 *	\fn int mutexgear_toggleattr_destroy(mutexgear_toggleattr_t *__attr)
 *	\brief A function to destroy a previously initialized \c mutexgear_toggleattr_t
 *	instance and free any resources possibly allocated for it.
 */
_MUTEXGEAR_API int mutexgear_toggleattr_destroy(mutexgear_toggleattr_t *__attr);

/**
 *	\fn int mutexgear_toggleattr_getpshared(const mutexgear_toggleattr_t *__attr, int *__pshared)
 *	\brief A function to get process shared attribute value stored in a \c mutexgear_toggleattr_t
 *	structure (similar to the \c pthread_mutexattr_getpshared).
 */
_MUTEXGEAR_API int mutexgear_toggleattr_getpshared(const mutexgear_toggleattr_t *__attr, int *__pshared);
/**
 *	\fn int mutexgear_toggleattr_setpshared(mutexgear_toggleattr_t *__attr, int __pshared)
 *	\brief A function to assign process shared attribute value to a \c mutexgear_toggleattr_t
 *	structure (similar to the \c pthread_mutexattr_setpshared).
 */
_MUTEXGEAR_API int mutexgear_toggleattr_setpshared(mutexgear_toggleattr_t *__attr, int __pshared);

/**
 *	\fn int mutexgear_toggleattr_getprioceiling(const mutexgear_toggleattr_t *__attr, int *__prioceiling)
 *	\brief A function to query priority ceiling attribute value stored in a \c mutexgear_toggleattr_t
 *	structure (similar to the \c pthread_mutexattr_getprioceiling).
 */
_MUTEXGEAR_API int mutexgear_toggleattr_getprioceiling(const mutexgear_toggleattr_t *__attr, int *__prioceiling);
/**
 *	\fn int mutexgear_toggleattr_getprotocol(const mutexgear_toggleattr_t *__attr, int *__protocol)
 *	\brief A function to retrieve lock protocol attribute value stored in a \c mutexgear_toggleattr_t
 *	structure (similar to the \c pthread_mutexattr_getprotocol).
 */
_MUTEXGEAR_API int mutexgear_toggleattr_getprotocol(const mutexgear_toggleattr_t *__attr, int *__protocol);
/**
 *	\fn int mutexgear_toggleattr_setprioceiling(mutexgear_toggleattr_t *__attr, int __prioceiling)
 *	\brief A function to assign a priority ceiling value to a \c mutexgear_toggleattr_t
 *	structure (similar to the \c pthread_mutexattr_setprioceiling).
 */
_MUTEXGEAR_API int mutexgear_toggleattr_setprioceiling(mutexgear_toggleattr_t *__attr, int __prioceiling);
/**
 *	\fn int mutexgear_toggleattr_setprotocol(mutexgear_toggleattr_t *__attr, int __protocol)
 *	\brief A function to set a lock protocol value to a \c mutexgear_toggleattr_t
 *	structure (similar to the \c pthread_mutexattr_setprotocol).
 */
_MUTEXGEAR_API int mutexgear_toggleattr_setprotocol(mutexgear_toggleattr_t *__attr, int __protocol);
/**
 *	\fn int mutexgear_toggleattr_setmutexattr(mutexgear_toggleattr_t *__attr, const _MUTEXGEAR_LOCKATTR_T *__mutexattr)
 *	\brief A function to copy attributes from a mutex attributes structure to
 *	a \c mutexgear_toggleattr_t structure.
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
 *	\typedef mutexgear_toggle_t
 *	\brief A \c toggle object structure.
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


/**
 *	\fn int mutexgear_toggle_init(mutexgear_toggle_t *__toggle, const mutexgear_toggleattr_t *__attr)
 *	\brief A function to create a \c toggle object with attributes defined in the \c mutexgear_toggleattr_t instance.
 */
_MUTEXGEAR_API int mutexgear_toggle_init(mutexgear_toggle_t *__toggle, const mutexgear_toggleattr_t *__attr);
/**
 *	\fn int mutexgear_toggle_destroy(mutexgear_toggle_t *__toggle)
 *	\brief A function to destroy a \c toggle instance and release any resources that might be allocated for it.
 */
_MUTEXGEAR_API int mutexgear_toggle_destroy(mutexgear_toggle_t *__toggle);


/**
 *	\fn int mutexgear_toggle_attach(mutexgear_toggle_t *__toggle)
 *	\brief A function to attach to the \c toggle to be called
 *	by the signaler (the S) thread to accomplish the Precondition 1.
 */
_MUTEXGEAR_API int mutexgear_toggle_attach(mutexgear_toggle_t *__toggle);
/**
 *	\fn int mutexgear_toggle_switch(mutexgear_toggle_t *__toggle)
 *	\brief A function to be called by the signaler (the S) thread
 *	to notify a potential waiter of an event. This corresponds
 *	to the steps 1-3 of the "Event Signaling" section.
 */
_MUTEXGEAR_API int mutexgear_toggle_switch(mutexgear_toggle_t *__toggle);
/**
 *	\fn int mutexgear_toggle_detach(mutexgear_toggle_t *__toggle)
 *	\brief A function to be called by the signaler (the S) thread
 *	to release the \c toggle object whenever it is not going to be used any more.
 */
_MUTEXGEAR_API int mutexgear_toggle_detach(mutexgear_toggle_t *__toggle);

/**
 *	\fn int mutexgear_toggle_pushon(mutexgear_toggle_t *__toggle)
 *	\brief A function to be called by the waiter (the W) thread
 *	to block waiting for an event notification. This corresponds
 *	to the steps 1-3 of the "Event Waiting" section.
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
