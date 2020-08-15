#ifndef __VS2013ATOMICS_H_INCLUDED
#define __VS2013ATOMICS_H_INCLUDED


#include <intrin.h>


#ifdef __cplusplus


#else // #ifndef __cplusplus

#if defined(_M_IX86) || defined(_M_X64)

#define __MUTEXGEAR_ATOMIC_PTRDIFF_NS
#define __MUTEXGEAR_ATOMIC_PTRDIFF_T ptrdiff_t
#define __MUTEGEAR_ATOMIC_CONSTRUCT_PTRDIFF(destination, value) (*(destination) = (ptrdiff_t)(value))
#define __MUTEGEAR_ATOMIC_DESTROY_PTRDIFF(destination) ((void)(destination))
#define __MUTEGEAR_ATOMIC_REINIT_PTRDIFF(destination, value) (*(destination) = (ptrdiff_t)(value))
#define __MUTEGEAR_ATOMIC_STORE_RELAXED_PTRDIFF(destination, value) (*(volatile ptrdiff_t *)(destination) = (ptrdiff_t)(value))

#ifdef _WIN64

#define __MUTEGEAR_ATOMIC_STORE_RELEASE_PTRDIFF(destination, value) ((void)_InterlockedExchange64_HLERelease(destination, (ptrdiff_t)(value)))

static inline 
int __mg_atomic_cas_relaxed_ptrdiff_helper(volatile ptrdiff_t *ppdDestination, ptrdiff_t *ppdComparansAndUpdate, ptrdiff_t pdValue)
{
	ptrdiff_t pdOldValue = _InterlockedCompareExchange64_HLEAcquire(ppdDestination, pdValue, *ppdComparansAndUpdate);
	return pdOldValue == *ppdComparansAndUpdate || (*ppdComparansAndUpdate = pdOldValue, 0);
}
#define __MUTEGEAR_ATOMIC_CAS_RELAXED_PTRDIFF(destination, comparand_and_update, value) __mg_atomic_cas_relaxed_ptrdiff_helper(destination, comparand_and_update, (ptrdiff_t)(value))

static inline 
int __mg_atomic_cas_release_ptrdiff_helper(volatile ptrdiff_t *ppdDestination, ptrdiff_t *ppdComparansAndUpdate, ptrdiff_t pdValue)
{
	ptrdiff_t pdOldValue = _InterlockedCompareExchange64_HLERelease(ppdDestination, pdValue, *ppdComparansAndUpdate);
	return pdOldValue == *ppdComparansAndUpdate || (*ppdComparansAndUpdate = pdOldValue, 0);
}
#define __MUTEGEAR_ATOMIC_CAS_RELEASE_PTRDIFF(destination, comparand_and_update, value) __mg_atomic_cas_release_ptrdiff_helper(destination, comparand_and_update, (ptrdiff_t)(value))
#define __MUTEGEAR_ATOMIC_SWAP_RELEASE_PTRDIFF(destination, value) _InterlockedExchange64_HLERelease(destination, (ptrdiff_t)(value))
#define __MUTEGEAR_ATOMIC_FETCH_ADD_RELAXED_PTRDIFF(destination, value) _InterlockedExchangeAdd64_HLEAcquire(destination, (ptrdiff_t)(value))
#define __MUTEGEAR_ATOMIC_FETCH_SUB_RELAXED_PTRDIFF(destination, value) _InterlockedExchangeAdd64_HLEAcquire(destination, -(ptrdiff_t)(value))
#define __MUTEGEAR_ATOMIC_UNSAFEOR_RELAXED_PTRDIFF(destination, value, original_storage) ((original_storage) = *(const volatile ptrdiff_t *)(destination), (void)_InterlockedExchangeAdd64_HLEAcquire(destination, (ptrdiff_t)(~(original_storage) & (ptrdiff_t)(value))))
#define __MUTEGEAR_ATOMIC_UNSAFEAND_RELAXED_PTRDIFF(destination, value, original_storage) ((original_storage) = *(const volatile ptrdiff_t *)(destination), (void)_InterlockedExchangeAdd64_HLEAcquire(destination, -(ptrdiff_t)((original_storage) & ~(ptrdiff_t)(value))))
#define __MUTEGEAR_ATOMIC_LOAD_RELAXED_PTRDIFF(source) (*(const volatile ptrdiff_t *)(source))
#define __MUTEGEAR_ATOMIC_LOAD_ACQUIRE_PTRDIFF(source) ((ptrdiff_t)_InterlockedExchangeAdd64_HLEAcquire(source, 0))


#else // #ifndef _WIN64

#define __MUTEGEAR_ATOMIC_STORE_RELEASE_PTRDIFF(destination, value) ((void)_InterlockedExchange_HLERelease(destination, (ptrdiff_t)(value)))

static inline 
int __mg_atomic_cas_relaxed_ptrdiff_helper(volatile ptrdiff_t *ppdDestination, ptrdiff_t *ppdComparansAndUpdate, ptrdiff_t pdValue)
{
	ptrdiff_t pdOldValue = _InterlockedCompareExchange_HLEAcquire(ppdDestination, pdValue, *ppdComparansAndUpdate);
	return pdOldValue == *ppdComparansAndUpdate || (*ppdComparansAndUpdate = pdOldValue, 0);
}
#define __MUTEGEAR_ATOMIC_CAS_RELAXED_PTRDIFF(destination, comparand_and_update, value) __mg_atomic_cas_relaxed_ptrdiff_helper(destination, comparand_and_update, (ptrdiff_t)(value))

static inline 
int __mg_atomic_cas_release_ptrdiff_helper(volatile ptrdiff_t *ppdDestination, ptrdiff_t *ppdComparansAndUpdate, ptrdiff_t pdValue)
{
	ptrdiff_t pdOldValue = _InterlockedCompareExchange_HLERelease(ppdDestination, pdValue, *ppdComparansAndUpdate);
	return pdOldValue == *ppdComparansAndUpdate || (*ppdComparansAndUpdate = pdOldValue, 0);
}
#define __MUTEGEAR_ATOMIC_CAS_RELEASE_PTRDIFF(destination, comparand_and_update, value) __mg_atomic_cas_release_ptrdiff_helper(destination, comparand_and_update, (ptrdiff_t)(value))
#define __MUTEGEAR_ATOMIC_SWAP_RELEASE_PTRDIFF(destination, value) _InterlockedExchange_HLERelease(destination, (ptrdiff_t)(value))
#define __MUTEGEAR_ATOMIC_FETCH_ADD_RELAXED_PTRDIFF(destination, value) _InterlockedExchangeAdd_HLEAcquire(destination, (ptrdiff_t)(value))
#define __MUTEGEAR_ATOMIC_FETCH_SUB_RELAXED_PTRDIFF(destination, value) _InterlockedExchangeAdd_HLEAcquire(destination, -(ptrdiff_t)(value))
#define __MUTEGEAR_ATOMIC_UNSAFEOR_RELAXED_PTRDIFF(destination, value, original_storage) ((original_storage) = *(const volatile ptrdiff_t *)(destination), (void)_InterlockedExchangeAdd_HLEAcquire(destination, (ptrdiff_t)(~(original_storage) & (ptrdiff_t)(value))))
#define __MUTEGEAR_ATOMIC_UNSAFEAND_RELAXED_PTRDIFF(destination, value, original_storage) ((original_storage) = *(const volatile ptrdiff_t *)(destination), (void)_InterlockedExchangeAdd_HLEAcquire(destination, -(ptrdiff_t)((original_storage) & ~(ptrdiff_t)(value))))
#define __MUTEGEAR_ATOMIC_LOAD_RELAXED_PTRDIFF(source) (*(const volatile ptrdiff_t *)(source))
#define __MUTEGEAR_ATOMIC_LOAD_ACQUIRE_PTRDIFF(source) ((ptrdiff_t)_InterlockedExchangeAdd_HLEAcquire(source, 0))


#endif // #ifndef _WIN64


#endif // #ifdef defined(_M_IX86) || defined(_M_X64)


#endif // #ifndef __cplusplus


#endif // #ifndef __VS2013ATOMICS_H_INCLUDED
