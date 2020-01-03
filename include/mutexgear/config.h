#ifndef __MUTEXGEAR_CONFIG_H_INCLUDED
#define __MUTEXGEAR_CONFIG_H_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* The Library Configuration Definitions                                */
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
*	\brief MutexGear Library configuration definitions
*
*	The file defines configuration dependent symbols used in other declarations.
*/


#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>


#if defined(_MSC_VER) || (defined(__GNUC__) && defined(_WIN32))

#if defined(_MUTEXGEAR_DLL)

#define _MUTEXGEAR_API __declspec(dllexport)


#elif !defined(_MUTEXGEAR_LIB)

#define _MUTEXGEAR_API __declspec(dllimport)


#endif // #if !defined(MUTEXGEAR_LIB)


#endif // #if defined(_MSC_VER) || (defined(__GNUC__) && defined(_WIN32))


#if !defined(_MUTEXGEAR_API)

#define _MUTEXGEAR_API


#endif // #if !defined(_MUTEXGEAR_API)


#if defined(__cplusplus)

#define _MUTEXGEAR_EXTERN_C extern "C"

#define _MUTEXGEAR_BEGIN_EXTERN_C() extern "C" {
#define _MUTEXGEAR_END_EXTERN_C() }


#else // #if !defined(__cplusplus)

#define _MUTEXGEAR_EXTERN_C

#define _MUTEXGEAR_BEGIN_EXTERN_C()
#define _MUTEXGEAR_END_EXTERN_C()


#endif // #if !defined(__cplusplus)


#if !defined(_MUTEXGEAR_PURE_INLINE)

#define _MUTEXGEAR_PURE_INLINE static __inline


#endif // #if !defined(_MUTEXGEAR_PURE_INLINE)


//////////////////////////////////////////////////////////////////////////
// Lock Definitions

#ifdef _WIN32

#include <Windows.h>


#define _MUTEXGEAR_LOCKATTR_T		bool
#define _MUTEXGEAR_LOCK_T			CRITICAL_SECTION


#else // #ifndef _WIN32

#if HAVE_PTHREAD_H
#include <pthread.h>
#endif // #if HAVE_PTHREAD_H


#define _MUTEXGEAR_LOCKATTR_T		pthread_mutexattr_t
#define _MUTEXGEAR_LOCK_T			pthread_mutex_t


#endif // #ifndef _WIN32


#if __STDC_VERSION__ >= 201112L && !defined(MUTEXGEAR_NO_C11_GENERICS)

#define MUTEXGEAR_USE_C11_GENERICS


#endif // #if __STDC_VERSION__ >= 201112L && !defined(MUTEXGEAR_NO_C11_GENERICS)


#endif // #ifndef __MUTEXGEAR_CONFIG_H_INCLUDED
