#ifndef __MUTEXGEAR_UTILITY_H_INCLUDED
#define __MUTEXGEAR_UTILITY_H_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* Public Utility Definitions of the Library                            */
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
*	\brief MutexGear Library public utility definitions
*
*	The file defines helper types, macros and symbols needed by the library.
*
*	Note: The declarations from this header are not necessary for the library public headers.
*	They are kept public to be used from tests.
*/


#include <mutexgear/config.h>


//////////////////////////////////////////////////////////////////////////
// MG_UNUSED Macro

#if defined(__GNUC__) || defined(__clang__)

#define _MG_UNUSED(name) name __attribute__((unused))


#else // other compilers

#define _MG_UNUSED(name) name


#endif // other compilers


//////////////////////////////////////////////////////////////////////////
// Asserts


#define MG_DO_NOTHING(x)	((void)(x))


#ifndef MG_ASSERT

#include <assert.h>

#define MG_ASSERT(x) assert(x)


#endif // #ifndef MG_ASSERT


#ifndef MG_VERIFY

#include <assert.h>

#ifndef NDEBUG
#define MG_VERIFY(x) assert(x)
#else // #ifdef NDEBUG
#define MG_VERIFY(x) ((void)(x))
#endif // // #ifdef NDEBUG


#endif // #ifndef MG_VERIFY


#ifndef MG_CHECK

#include <assert.h>
#include <stdlib.h>

extern volatile int mg_failed_check_status;

#ifndef NDEBUG
#define MG_CHECK(v, x) if (x); else (mg_failed_check_status = (v), assert(!#v))
#else // #ifdef NDEBUG
#define MG_CHECK(v, x) if (x); else (mg_failed_check_status = (v), abort())
#endif // // #ifdef NDEBUG


#endif // #ifndef MG_CHECK


#ifndef MG_STATIC_ASSERT

#define _MG_STATIC_ASSERT_HELPER2(x, y) x ## y
#define _MG_STATIC_ASSERT_HELPER1(x, y) _MG_STATIC_ASSERT_HELPER2(x, y)
#define MG_STATIC_ASSERT(e) typedef char _MG_UNUSED(_MG_STATIC_ASSERT_HELPER1(_mg_Static_assertion_failed_, __LINE__)[(e)?1:-1])


#endif // #ifndef MG_STATIC_ASSERT


//////////////////////////////////////////////////////////////////////////
// Type cast union

#define MG_DECLARE_TYPE_CAST(type) union { void *v; type *t; } _mg_type_cast_union
#define MG_PERFORM_TYPE_CAST(value) (_mg_type_cast_union.v = (void *)(value), _mg_type_cast_union.t)


//////////////////////////////////////////////////////////////////////////
// Error codes

#include <errno.h>

#ifndef EOK
#define EOK 0
#endif // #ifndef EOK


//////////////////////////////////////////////////////////////////////////
// Atomics

#ifdef _MUTEXGEAR_WITH_C11

#ifdef __cplusplus

#include <atomic>
#include <new>


#define __MUTEXGEAR_ATOMIC_PTRDIFF_NS std::
#define __MUTEXGEAR_ATOMIC_PTRDIFF_T atomic_ptrdiff_t
#define __MUTEGEAR_ATOMIC_CONSTRUCT_PTRDIFF(destination, value) new(__destination) __MUTEXGEAR_ATOMIC_PTRDIFF_NS __MUTEXGEAR_ATOMIC_PTRDIFF_T(value)
#define __MUTEGEAR_ATOMIC_DESTROY_PTRDIFF(destination) (destination)->__MUTEXGEAR_ATOMIC_PTRDIFF_NS __MUTEXGEAR_ATOMIC_PTRDIFF_T::~__MUTEXGEAR_ATOMIC_PTRDIFF_T()
#define __MUTEGEAR_ATOMIC_REINIT_PTRDIFF(destination, value) (destination)->store((ptrdiff_t)(value), std::memory_order_relaxed)
#define __MUTEGEAR_ATOMIC_STORE_RELAXED_PTRDIFF(destination, value) (destination)->store((ptrdiff_t)(value), std::memory_order_relaxed)
#define __MUTEGEAR_ATOMIC_STORE_RELEASE_PTRDIFF(destination, value) (destination)->store((ptrdiff_t)(value), std::memory_order_release)
#define __MUTEGEAR_ATOMIC_CAS_RELAXED_PTRDIFF(destination, comparand_and_update, value) (destination)->compare_exchange_strong(*(comparand_and_update), (ptrdiff_t)(value), std::memory_order_relaxed, std::memory_order_relaxed)
#define __MUTEGEAR_ATOMIC_CAS_RELEASE_PTRDIFF(destination, comparand_and_update, value) (destination)->compare_exchange_strong(*(comparand_and_update), (ptrdiff_t)(value), std::memory_order_release, std::memory_order_relaxed)
#define __MUTEGEAR_ATOMIC_SWAP_RELEASE_PTRDIFF(destination, value) (destination)->exchange((ptrdiff_t)(value), std::memory_order_release)
#define __MUTEGEAR_ATOMIC_FETCH_ADD_RELAXED_PTRDIFF(destination, value) (destination)->fetch_add((ptrdiff_t)(value), std::memory_order_relaxed)
#define __MUTEGEAR_ATOMIC_FETCH_SUB_RELAXED_PTRDIFF(destination, value) (destination)->fetch_sub((ptrdiff_t)(value), std::memory_order_relaxed)
#define __MUTEGEAR_ATOMIC_UNSAFEOR_RELAXED_PTRDIFF(destination, value) ((void)(destination)->fetch_or((ptrdiff_t)(value), std::memory_order_relaxed))
#define __MUTEGEAR_ATOMIC_FETCH_OR_RELAXED_PTRDIFF(destination, value) (destination)->fetch_or((ptrdiff_t)(value), std::memory_order_relaxed)
#define __MUTEGEAR_ATOMIC_UNSAFEAND_RELAXED_PTRDIFF(destination, value) ((void)(destination)->fetch_and((ptrdiff_t)(value), std::memory_order_relaxed))
#define __MUTEGEAR_ATOMIC_LOAD_RELAXED_PTRDIFF(source) (source)->load(std::memory_order_relaxed)
#define __MUTEGEAR_ATOMIC_LOAD_ACQUIRE_PTRDIFF(source) (source)->load(std::memory_order_acquire)


#else // #ifndef __cplusplus

#ifdef _MUTEXGEAR_WITH_ATOMIC_NOT_STDATOMIC
#include <atomic.h>
#else
#include <stdatomic.h>
#endif


#define __MUTEXGEAR_ATOMIC_PTRDIFF_NS
#define __MUTEXGEAR_ATOMIC_PTRDIFF_T atomic_ptrdiff_t
#define __MUTEGEAR_ATOMIC_CONSTRUCT_PTRDIFF(destination, value) atomic_store_explicit(destination, (ptrdiff_t)(value), memory_order_relaxed) // atomic_init(destination, (ptrdiff_t)(value)))
#define __MUTEGEAR_ATOMIC_DESTROY_PTRDIFF(destination) ((void)destination)
#define __MUTEGEAR_ATOMIC_REINIT_PTRDIFF(destination, value) atomic_store_explicit(destination, (ptrdiff_t)(value), memory_order_relaxed)
#define __MUTEGEAR_ATOMIC_STORE_RELAXED_PTRDIFF(destination, value) atomic_store_explicit(destination, (ptrdiff_t)(value), memory_order_relaxed)
#define __MUTEGEAR_ATOMIC_STORE_RELEASE_PTRDIFF(destination, value) atomic_store_explicit(destination, (ptrdiff_t)(value), memory_order_release)
#define __MUTEGEAR_ATOMIC_CAS_RELAXED_PTRDIFF(destination, comparand_and_update, value) atomic_compare_exchange_strong_explicit(destination, comparand_and_update, (ptrdiff_t)(value), memory_order_relaxed, memory_order_relaxed)
#define __MUTEGEAR_ATOMIC_CAS_RELEASE_PTRDIFF(destination, comparand_and_update, value) atomic_compare_exchange_strong_explicit(destination, comparand_and_update, (ptrdiff_t)(value), memory_order_release, memory_order_relaxed)
#define __MUTEGEAR_ATOMIC_SWAP_RELEASE_PTRDIFF(destination, value) atomic_exchange_explicit(destination, (ptrdiff_t)(value), memory_order_release)
#define __MUTEGEAR_ATOMIC_FETCH_ADD_RELAXED_PTRDIFF(destination, value) atomic_fetch_add_explicit(destination, (ptrdiff_t)(value), memory_order_relaxed)
#define __MUTEGEAR_ATOMIC_FETCH_SUB_RELAXED_PTRDIFF(destination, value) atomic_fetch_sub_explicit(destination, (ptrdiff_t)(value), memory_order_relaxed)
#define __MUTEGEAR_ATOMIC_UNSAFEOR_RELAXED_PTRDIFF(destination, value) ((void)atomic_fetch_or_explicit(destination, (ptrdiff_t)(value), memory_order_relaxed))
#define __MUTEGEAR_ATOMIC_FETCH_OR_RELAXED_PTRDIFF(destination, value) atomic_fetch_or_explicit(destination, (ptrdiff_t)(value), memory_order_relaxed)
#define __MUTEGEAR_ATOMIC_UNSAFEAND_RELAXED_PTRDIFF(destination, value) ((void)atomic_fetch_and_explicit(destination, (ptrdiff_t)(value), memory_order_relaxed))
#define __MUTEGEAR_ATOMIC_LOAD_RELAXED_PTRDIFF(source) ((ptrdiff_t)atomic_load_explicit(source, memory_order_relaxed))
#define __MUTEGEAR_ATOMIC_LOAD_ACQUIRE_PTRDIFF(source) ((ptrdiff_t)atomic_load_explicit(source, memory_order_acquire))


#endif // #ifndef __cplusplus


#else // #ifndef _MUTEXGEAR_WITH_C11

#ifndef __MUTEXGEAR_ATOMIC_HEADER
// If case if there is no user supplied definition define the header (see below) to self to avoid compilation errors on the inclusion
#define __MUTEXGEAR_ATOMIC_HEADER <mutexgear/config.h>
#endif

// This should include a user supplied header to define atomic types/functions
#include __MUTEXGEAR_ATOMIC_HEADER


#ifndef __MUTEXGEAR_ATOMIC_PTRDIFF_NS
#define __MUTEXGEAR_ATOMIC_PTRDIFF_NS
#endif

#ifndef __MUTEXGEAR_ATOMIC_PTRDIFF_T
#error Please define __MUTEXGEAR_ATOMIC_PTRDIFF_T
#endif

#ifndef __MUTEGEAR_ATOMIC_CONSTRUCT_PTRDIFF
#error Please define __MUTEGEAR_ATOMIC_CONSTRUCT_PTRDIFF
#endif

#ifndef __MUTEGEAR_ATOMIC_DESTROY_PTRDIFF
#error Please define __MUTEGEAR_ATOMIC_DESTROY_PTRDIFF
#endif

#ifndef __MUTEGEAR_ATOMIC_REINIT_PTRDIFF
#error Please define __MUTEGEAR_ATOMIC_REINIT_PTRDIFF
#endif

#ifndef __MUTEGEAR_ATOMIC_STORE_RELAXED_PTRDIFF
#error Please define __MUTEGEAR_ATOMIC_STORE_RELAXED_PTRDIFF
#endif

#ifndef __MUTEGEAR_ATOMIC_STORE_RELEASE_PTRDIFF
#error Please define __MUTEGEAR_ATOMIC_STORE_RELEASE_PTRDIFF
#endif

#ifndef __MUTEGEAR_ATOMIC_CAS_RELAXED_PTRDIFF
#error Please define __MUTEGEAR_ATOMIC_CAS_RELAXED_PTRDIFF
#endif

#ifndef __MUTEGEAR_ATOMIC_CAS_RELEASE_PTRDIFF
#error Please define __MUTEGEAR_ATOMIC_CAS_RELEASE_PTRDIFF
#endif

#ifndef __MUTEGEAR_ATOMIC_SWAP_RELEASE_PTRDIFF
#error Please define __MUTEGEAR_ATOMIC_SWAP_RELEASE_PTRDIFF
#endif

#ifndef __MUTEGEAR_ATOMIC_FETCH_ADD_RELAXED_PTRDIFF
#error Please define __MUTEGEAR_ATOMIC_FETCH_ADD_RELAXED_PTRDIFF
#endif

#ifndef __MUTEGEAR_ATOMIC_FETCH_SUB_RELAXED_PTRDIFF
#error Please define __MUTEGEAR_ATOMIC_FETCH_SUB_RELAXED_PTRDIFF
#endif

#ifndef __MUTEGEAR_ATOMIC_UNSAFEOR_RELAXED_PTRDIFF
#error Please define __MUTEGEAR_ATOMIC_UNSAFEOR_RELAXED_PTRDIFF
#endif

#ifndef __MUTEGEAR_ATOMIC_FETCH_OR_RELAXED_PTRDIFF
#error Please define __MUTEGEAR_ATOMIC_FETCH_OR_RELAXED_PTRDIFF
#endif

#ifndef __MUTEGEAR_ATOMIC_UNSAFEAND_RELAXED_PTRDIFF
#error Please define __MUTEGEAR_ATOMIC_UNSAFEAND_RELAXED_PTRDIFF
#endif

#ifndef __MUTEGEAR_ATOMIC_LOAD_RELAXED_PTRDIFF
#error Please define __MUTEGEAR_ATOMIC_LOAD_RELAXED_PTRDIFF
#endif

#ifndef __MUTEGEAR_ATOMIC_LOAD_ACQUIRE_PTRDIFF
#error Please define __MUTEGEAR_ATOMIC_LOAD_ACQUIRE_PTRDIFF
#endif


#endif // #ifndef _MUTEXGEAR_WITH_C11


typedef __MUTEXGEAR_ATOMIC_PTRDIFF_NS __MUTEXGEAR_ATOMIC_PTRDIFF_T _mg_atomic_ptrdiff_t;
#define _MG_PA_PTRDIFF(argument) _mg_make_p_atomic_ptrdiff(argument)
#define _MG_PVA_PTRDIFF(argument) _mg_make_pv_atomic_ptrdiff(argument)
#define _MG_PCVA_PTRDIFF(argument) _mg_make_pcv_atomic_ptrdiff(argument)

_MUTEXGEAR_PURE_INLINE
_mg_atomic_ptrdiff_t *_mg_make_p_atomic_ptrdiff(ptrdiff_t *__argument)
{
	MG_STATIC_ASSERT(sizeof(_mg_atomic_ptrdiff_t) == sizeof(*__argument));

	return (_mg_atomic_ptrdiff_t *)__argument;
}

_MUTEXGEAR_PURE_INLINE
volatile _mg_atomic_ptrdiff_t *_mg_make_pv_atomic_ptrdiff(ptrdiff_t *__argument)
{
	MG_STATIC_ASSERT(sizeof(_mg_atomic_ptrdiff_t) == sizeof(*__argument));

	return (volatile _mg_atomic_ptrdiff_t *)__argument;
}

_MUTEXGEAR_PURE_INLINE
const volatile _mg_atomic_ptrdiff_t *_mg_make_pcv_atomic_ptrdiff(const ptrdiff_t *__argument)
{
	MG_STATIC_ASSERT(sizeof(_mg_atomic_ptrdiff_t) == sizeof(*__argument));

	return (const volatile _mg_atomic_ptrdiff_t *)__argument;
}


_MUTEXGEAR_PURE_INLINE
void _mg_atomic_construct_ptrdiff(_mg_atomic_ptrdiff_t *__destination, ptrdiff_t __value)
{
	__MUTEGEAR_ATOMIC_CONSTRUCT_PTRDIFF(__destination, __value);
}

_MUTEXGEAR_PURE_INLINE
void _mg_atomic_destroy_ptrdiff(_mg_atomic_ptrdiff_t *__destination)
{
	__MUTEGEAR_ATOMIC_DESTROY_PTRDIFF(__destination);
}

_MUTEXGEAR_PURE_INLINE
void _mg_atomic_reinit_ptrdiff(volatile _mg_atomic_ptrdiff_t *__destination, ptrdiff_t __value)
{
	__MUTEGEAR_ATOMIC_REINIT_PTRDIFF(__destination, __value);
}

_MUTEXGEAR_PURE_INLINE
void _mg_atomic_store_relaxed_ptrdiff(volatile _mg_atomic_ptrdiff_t *__destination, ptrdiff_t __value)
{
	__MUTEGEAR_ATOMIC_STORE_RELAXED_PTRDIFF(__destination, __value);
}

_MUTEXGEAR_PURE_INLINE
void _mg_atomic_store_release_ptrdiff(volatile _mg_atomic_ptrdiff_t *__destination, ptrdiff_t __value)
{
	__MUTEGEAR_ATOMIC_STORE_RELEASE_PTRDIFF(__destination, __value);
}

_MUTEXGEAR_PURE_INLINE
bool _mg_atomic_cas_relaxed_ptrdiff(volatile _mg_atomic_ptrdiff_t *__destination, ptrdiff_t *__comparand_and_update, ptrdiff_t __value)
{
	return __MUTEGEAR_ATOMIC_CAS_RELAXED_PTRDIFF(__destination, __comparand_and_update, __value);
}

_MUTEXGEAR_PURE_INLINE
bool _mg_atomic_cas_release_ptrdiff(volatile _mg_atomic_ptrdiff_t *__destination, ptrdiff_t *__comparand_and_update, ptrdiff_t __value)
{
	return __MUTEGEAR_ATOMIC_CAS_RELEASE_PTRDIFF(__destination, __comparand_and_update, __value);
}

_MUTEXGEAR_PURE_INLINE
ptrdiff_t _mg_atomic_swap_release_ptrdiff(volatile _mg_atomic_ptrdiff_t *__destination, ptrdiff_t __value)
{
	return __MUTEGEAR_ATOMIC_SWAP_RELEASE_PTRDIFF(__destination, __value);
}

_MUTEXGEAR_PURE_INLINE
ptrdiff_t _mg_atomic_fetch_add_relaxed_ptrdiff(volatile _mg_atomic_ptrdiff_t *__destination, ptrdiff_t __value)
{
	return __MUTEGEAR_ATOMIC_FETCH_ADD_RELAXED_PTRDIFF(__destination, __value);
}

_MUTEXGEAR_PURE_INLINE
ptrdiff_t _mg_atomic_fetch_sub_relaxed_ptrdiff(volatile _mg_atomic_ptrdiff_t *__destination, ptrdiff_t __value)
{
	return __MUTEGEAR_ATOMIC_FETCH_SUB_RELAXED_PTRDIFF(__destination, __value);
}

_MUTEXGEAR_PURE_INLINE
void _mg_atomic_unsafeor_relaxed_ptrdiff(volatile _mg_atomic_ptrdiff_t *__destination, ptrdiff_t __value)
{
	__MUTEGEAR_ATOMIC_UNSAFEOR_RELAXED_PTRDIFF(__destination, __value);
}

_MUTEXGEAR_PURE_INLINE
ptrdiff_t _mg_atomic_fetch_or_relaxed_ptrdiff(volatile _mg_atomic_ptrdiff_t *__destination, ptrdiff_t __value)
{
	return __MUTEGEAR_ATOMIC_FETCH_OR_RELAXED_PTRDIFF(__destination, __value);
}

_MUTEXGEAR_PURE_INLINE
void _mg_atomic_unsafeand_relaxed_ptrdiff(volatile _mg_atomic_ptrdiff_t *__destination, ptrdiff_t __value)
{
	__MUTEGEAR_ATOMIC_UNSAFEAND_RELAXED_PTRDIFF(__destination, __value);
}

_MUTEXGEAR_PURE_INLINE
ptrdiff_t _mg_atomic_load_relaxed_ptrdiff(const volatile _mg_atomic_ptrdiff_t *__source)
{
	return __MUTEGEAR_ATOMIC_LOAD_RELAXED_PTRDIFF((volatile _mg_atomic_ptrdiff_t *)__source);
}

_MUTEXGEAR_PURE_INLINE
ptrdiff_t _mg_atomic_load_acquire_ptrdiff(const volatile _mg_atomic_ptrdiff_t *__source)
{
	return __MUTEGEAR_ATOMIC_LOAD_ACQUIRE_PTRDIFF((volatile _mg_atomic_ptrdiff_t *)__source);
}


#endif // #ifndef __MUTEXGEAR_UTILITY_H_INCLUDED

