#ifndef __MUTEXGEAR_MG_UTILITY_H_INCLUDED
#define __MUTEXGEAR_MG_UTILITY_H_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* Private Utility Definitions of the Library                           */
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
*	\brief MutexGear Library private utility definitions
*
*	The file defines helper types, macros and symbols needed by the library.
*/


#include <mutexgear/utility.h>
#include <mutexgear/constants.h>


//////////////////////////////////////////////////////////////////////////
// Lock Function Definitions

#ifdef _WIN32

// #ifndef _WIN32_WINNT
// #define _WIN32_WINNT 0x0400
// #endif

#include <Windows.h>


#define _MUTEXGEAR_ERRNO__PSHARED_MISSING		ENOSYS


_MUTEXGEAR_PURE_INLINE
int _mutexgear_lockattr_init(_MUTEXGEAR_LOCKATTR_T *__attr)
{
	MG_DO_NOTHING(__attr);

	return EOK;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_lockattr_destroy(_MUTEXGEAR_LOCKATTR_T *__attr)
{
	MG_DO_NOTHING(__attr);

	return EOK;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_lockattr_getpshared(const _MUTEXGEAR_LOCKATTR_T *__attr, int *__out_pshared)
{
	MG_DO_NOTHING(__attr);
	MG_DO_NOTHING(__out_pshared);

	return _MUTEXGEAR_ERRNO__PSHARED_MISSING;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_lockattr_setpshared(_MUTEXGEAR_LOCKATTR_T *__attr, int __pshared)
{
	MG_DO_NOTHING(__attr);
	MG_DO_NOTHING(__pshared);

	return ENOSYS;
}


_MUTEXGEAR_PURE_INLINE
int _mutexgear_lockattr_getprioceiling(const _MUTEXGEAR_LOCKATTR_T *__attr, int *__out_prioceiling)
{
	MG_DO_NOTHING(__attr);

	*__out_prioceiling = -1;
	return EOK;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_lockattr_setprioceiling(_MUTEXGEAR_LOCKATTR_T *__attr, int __prioceiling)
{
	MG_DO_NOTHING(__attr);
	MG_DO_NOTHING(__prioceiling);

	return EOK;
}


_MUTEXGEAR_PURE_INLINE
int _mutexgear_lockattr_getprotocol(const _MUTEXGEAR_LOCKATTR_T *__attr, int *__out_protocol)
{
	MG_DO_NOTHING(__attr);

	*(__out_protocol) = MUTEXGEAR__INVALID_PROTOCOL;
	return EOK;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_lockattr_setprotocol(_MUTEXGEAR_LOCKATTR_T *__attr, int __protocol)
{
	MG_DO_NOTHING(__attr);
	MG_DO_NOTHING(__protocol);

	return EOK;
}


_MUTEXGEAR_PURE_INLINE
int _mutexgear_lock_init(_MUTEXGEAR_LOCK_T *__lock, const _MUTEXGEAR_LOCKATTR_T *__attr)
{
	MG_DO_NOTHING(__attr);

	InitializeCriticalSection(__lock);
	return EOK;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_lock_destroy(_MUTEXGEAR_LOCK_T *__lock)
{
	DeleteCriticalSection(__lock);
	return EOK;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_lock_acquire(_MUTEXGEAR_LOCK_T *__lock)
{
	EnterCriticalSection(__lock);
	return EOK;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_lock_tryacquire(_MUTEXGEAR_LOCK_T *__lock)
{
	return TryEnterCriticalSection(__lock) != FALSE ? EOK : EBUSY;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_lock_release(_MUTEXGEAR_LOCK_T *__lock)
{
	LeaveCriticalSection(__lock);
	return EOK;
}


#else // #ifndef _WIN32


#if HAVE_PTHREAD_H
#include <pthread.h>
#endif // #if HAVE_PTHREAD_H


#define _MUTEXGEAR_ERRNO__PSHARED_MISSING		EOK


_MUTEXGEAR_PURE_INLINE
int _mutexgear_lockattr_init(_MUTEXGEAR_LOCKATTR_T *__attr)
{
	return pthread_mutexattr_init(__attr);
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_lockattr_destroy(_MUTEXGEAR_LOCKATTR_T *__attr)
{
	return pthread_mutexattr_destroy(__attr);
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_lockattr_getpshared(const _MUTEXGEAR_LOCKATTR_T *__attr, int *__out_pshared)
{
	int pshared_value = -1;
	int ret = pthread_mutexattr_getpshared(__attr, &pshared_value);
	return ret == EOK 
		? pshared_value == PTHREAD_PROCESS_PRIVATE 
			? (*__out_pshared = MUTEXGEAR_PROCESS_PRIVATE, EOK) 
			: pshared_value == PTHREAD_PROCESS_SHARED 
				? (*__out_pshared = MUTEXGEAR_PROCESS_SHARED, EOK) 
				: EINVAL
		: ret;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_lockattr_setpshared(_MUTEXGEAR_LOCKATTR_T *__attr, int __pshared)
{
	int ret;

	do 
	{
		int pshared_value;

		if (__pshared == MUTEXGEAR_PROCESS_SHARED)
		{
			pshared_value = PTHREAD_PROCESS_SHARED;
		}
		else if (__pshared == MUTEXGEAR_PROCESS_PRIVATE)
		{
			pshared_value = PTHREAD_PROCESS_PRIVATE;
		}
		else
		{
			ret = EINVAL;
			break;
		}

		ret = pthread_mutexattr_setpshared(__attr, pshared_value);
	}
	while (false);

	return ret;
}


_MUTEXGEAR_PURE_INLINE
int _mutexgear_lockattr_getprioceiling(const _MUTEXGEAR_LOCKATTR_T *__attr, int *__out_prioceiling)
{
	int ret =
#if HAVE_PTHREAD_MUTEXATTR_GETPRIOCEILING
		pthread_mutexattr_getprioceiling(__attr, __out_prioceiling);
#else
		(*__out_prioceiling = -1, EOK);
#endif
	return ret;
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_lockattr_setprioceiling(_MUTEXGEAR_LOCKATTR_T *__attr, int __prioceiling)
{
	int ret =
#if HAVE_PTHREAD_MUTEXATTR_GETPRIOCEILING
		pthread_mutexattr_setprioceiling(__attr, __prioceiling);
#else
		EOK;
#endif
	return ret;
}


_MUTEXGEAR_PURE_INLINE
int _mutexgear_lockattr_getprotocol(const _MUTEXGEAR_LOCKATTR_T *__attr, int *__out_protocol)
{
#if HAVE_PTHREAD_MUTEXATTR_GETPROTOCOL
	int protocol_value;
	int ret = pthread_mutexattr_getprotocol(__attr, &protocol_value);
	return ret == EOK
		? protocol_value == PTHREAD_PRIO_INHERIT 
			? (*__out_protocol = MUTEXGEAR_PRIO_INHERIT, EOK)
			: protocol_value == PTHREAD_PRIO_PROTECT 
				? (*__out_protocol = MUTEXGEAR_PRIO_PROTECT, EOK)
				: protocol_value == PTHREAD_PRIO_NONE 
					? (*__out_protocol = MUTEXGEAR_PRIO_NONE, EOK)
					: EINVAL
		: ret;
#else
	*__out_protocol = MUTEXGEAR__INVALID_PROTOCOL;
	return EOK;
#endif
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_lockattr_setprotocol(_MUTEXGEAR_LOCKATTR_T *__attr, int __protocol)
{
#if HAVE_PTHREAD_MUTEXATTR_GETPROTOCOL
	int ret;

	do
	{
		int protocol_value;

		if (__protocol == MUTEXGEAR_PRIO_NONE)
		{
			protocol_value = PTHREAD_PRIO_NONE;
		}
		else if (__protocol == MUTEXGEAR_PRIO_PROTECT)
		{
			protocol_value = PTHREAD_PRIO_PROTECT;
		}
		else if (__protocol == MUTEXGEAR_PRIO_INHERIT)
		{
			protocol_value = PTHREAD_PRIO_INHERIT;
		}
		else
		{
			ret = EINVAL;
			break;
		}

		ret = pthread_mutexattr_setprotocol(__attr, protocol_value);
	}
	while (false);

	return ret;
#else
	return EOK;
#endif
}


_MUTEXGEAR_PURE_INLINE
int _mutexgear_lock_init(_MUTEXGEAR_LOCK_T *__lock, const _MUTEXGEAR_LOCKATTR_T *__attr)
{
	return pthread_mutex_init(__lock, __attr);
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_lock_destroy(_MUTEXGEAR_LOCK_T *__lock)
{
	return pthread_mutex_destroy(__lock);
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_lock_acquire(_MUTEXGEAR_LOCK_T *__lock)
{
	return pthread_mutex_lock(__lock);
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_lock_tryacquire(_MUTEXGEAR_LOCK_T *__lock)
{
	return pthread_mutex_trylock(__lock);
}

_MUTEXGEAR_PURE_INLINE
int _mutexgear_lock_release(_MUTEXGEAR_LOCK_T *__lock)
{
	return pthread_mutex_unlock(__lock);
}


#endif // #ifndef _WIN32


#endif // #ifndef __MUTEXGEAR_MG_UTILITY_H_INCLUDED

