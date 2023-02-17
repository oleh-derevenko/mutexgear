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
*	\brief MutexGear Library configuration definitions
*
*	The file defines configuration dependent symbols used in other declarations.
*/


#include "_confvars.h"
#include <stddef.h>
#include <stdint.h>

#if !defined(_MUTEXGEAR_HAVE_NO_STDBOOL_H)
#include <stdbool.h>
#else // #if defined(_MUTEXGEAR_HAVE_NO_STDBOOL_H)
#if !defined(__cplusplus)
#if !defined(bool)
#define bool int
#define true 1
#define false 0
#endif // !defined(bool)
#endif // #if !defined(__cplusplus)
#endif // #if defined(_MUTEXGEAR_HAVE_NO_STDBOOL_H)


#if defined(_MSC_VER) || (defined(_WIN32) && (defined(__GNUC__) || defined(__clang__)))

#if defined(_MUTEXGEAR_DLL)

#define _MUTEXGEAR_API __declspec(dllexport)


#elif !defined(_MUTEXGEAR_LIB)

#define _MUTEXGEAR_API __declspec(dllimport)


#endif // #if !defined(MUTEXGEAR_LIB)


#endif // #if defined(_MSC_VER) || (defined(_WIN32) && (defined(__GNUC__) || defined(__clang__)))


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


#if defined(__cplusplus)

#ifndef _MUTEXGEAR_NAMESPACE

#define _MUTEXGEAR_NAMESPACE mg


#endif // #ifndef _MUTEXGEAR_NAMESPACE


#define _MUTEXGEAR_BEGIN_NAMESPACE() namespace _MUTEXGEAR_NAMESPACE {
#define _MUTEXGEAR_END_NAMESPACE() }


#define _MUTEXGEAR_COMPLETION_NAMESPACE completion
#define _MUTEXGEAR_BEGIN_COMPLETION_NAMESPACE() namespace _MUTEXGEAR_COMPLETION_NAMESPACE {
#define _MUTEXGEAR_END_COMPLETION_NAMESPACE() }

#define _MUTEXGEAR_TRDL_NAMESPACE trdl
#define _MUTEXGEAR_BEGIN_TRDL_NAMESPACE() namespace _MUTEXGEAR_TRDL_NAMESPACE {
#define _MUTEXGEAR_END_TRDL_NAMESPACE() }

#define _MUTEXGEAR_SHMTX_HELPERS_NAMESPACE shmtx_helpers
#define _MUTEXGEAR_BEGIN_SHMTX_HELPERS_NAMESPACE() namespace _MUTEXGEAR_SHMTX_HELPERS_NAMESPACE {
#define _MUTEXGEAR_END_SHMTX_HELPERS_NAMESPACE() }

#endif // #if !defined(__cplusplus)


//////////////////////////////////////////////////////////////////////////
// Inline specifiers

#if !defined(_MUTEXGEAR_PURE_INLINE)

#define _MUTEXGEAR_PURE_INLINE static inline


#endif // #if !defined(_MUTEXGEAR_PURE_INLINE)


#if !defined(_MUTEXGEAR_ALWAYS_INLINE)

#if defined(__GNUC__) || defined(__clang__)

#define _MUTEXGEAR_ALWAYS_INLINE __attribute__((always_inline))


#elif defined(_MSC_VER)

#define _MUTEXGEAR_ALWAYS_INLINE __forceinline


#else // No compiler match

#define _MUTEXGEAR_ALWAYS_INLINE


#endif // No compiler match


#endif // #if !defined(_MUTEXGEAR_ALWAYS_INLINE)


//////////////////////////////////////////////////////////////////////////
// Lock Definitions

#ifdef _WIN32

#include <Windows.h>


#define _MUTEXGEAR_LOCKATTR_T		bool
#define _MUTEXGEAR_LOCK_T			CRITICAL_SECTION


#else // #ifndef _WIN32

#if defined(_MUTEXGEAR_HAVE_PTHREAD_H)
#include <pthread.h>
#endif // #if defined(_MUTEXGEAR_HAVE_PTHREAD_H)


#define _MUTEXGEAR_LOCKATTR_T		pthread_mutexattr_t
#define _MUTEXGEAR_LOCK_T			pthread_mutex_t


#endif // #ifndef _WIN32


#if __STDC_VERSION__ >= 201112L && !defined(MUTEXGEAR_NO_C11_GENERICS)

#define MUTEXGEAR_USE_C11_GENERICS


#endif // #if __STDC_VERSION__ >= 201112L && !defined(MUTEXGEAR_NO_C11_GENERICS)


#endif // #ifndef __MUTEXGEAR_CONFIG_H_INCLUDED
