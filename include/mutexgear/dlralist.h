#ifndef __MUTEXGEAR_DLRALIST_H_INCLUDED
#define __MUTEXGEAR_DLRALIST_H_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear Double-Linked Relative Atomic List API Implementation      */
/*                                                                      */
/* WARNING!                                                             */
/* This library contains a synchronization technique protected by       */
/* the U.S. Parent 9,983,913.                                           */
/*                                                                      */
/* THIS IS A PRE-RELEASE LIBRARY SNAPSHOT FOR EVALUATION PURPOSES ONLY. */
/* AWAIT THE RELEASE AT https://mutexgear.com                           */
/*                                                                      */
/* Copyright (c) 2016-2019 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/

/**
 *	\file
 *	\brief MutexGear Double-Linked Relative Atomic List API implementation
 *
 *	The header defines a double-linked list object to be embedded within
 *	other memory structures. The list uses relative offsets instead of 
 *	direct pointers to be possible used to be used in shared memory sections.
 *	Previous element link is accessed with atomic functions to make some
 *	operations directly available in multi-threaded environment.
 */


#include <mutexgear/config.h>
#include <mutexgear/utility.h>


_MUTEXGEAR_BEGIN_EXTERN_C();

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// The list is defined via template

#define _MUTEXGEAR_LITEM_GETNEXT(p_item_instance) ((uint8_t *)(p_item_instance) + (p_item_instance)->p_next_item)
#define _MUTEXGEAR_LITEM_SAFEGETPREV(p_item_instance) ((uint8_t *)(p_item_instance) + _mg_atomic_load_relaxed_ptrdiff(_MG_PCVA_PTRDIFF(&(p_item_instance)->p_prev_item)))
#define _MUTEXGEAR_LITEM_SETNEXT(p_item_instance, p_next_instance) (p_item_instance)->p_next_item = (uint8_t *)(p_next_instance) - (uint8_t *)(p_item_instance)
#define _MUTEXGEAR_LITEM_CONSTRPREV(p_item_instance, p_prev_instance) (_mg_atomic_construct_ptrdiff(_MG_PA_PTRDIFF(&(p_item_instance)->p_prev_item)), _mg_atomic_init_ptrdiff(_MG_PA_PTRDIFF(&(p_item_instance)->p_prev_item), (uint8_t *)(p_prev_instance) - (uint8_t *)(p_item_instance)))
#define _MUTEXGEAR_LITEM_DESTRPREV(p_item_instance) _mg_atomic_destroy_ptrdiff(_MG_PA_PTRDIFF(&(p_item_instance)->p_prev_item))
#define _MUTEXGEAR_LITEM_UNSAFESETPREV(p_item_instance, p_prev_instance) _mg_atomic_reinit_ptrdiff(_MG_PVA_PTRDIFF(&(p_item_instance)->p_prev_item), (uint8_t *)(p_prev_instance) - (uint8_t *)(p_item_instance))
#define _MUTEXGEAR_LITEM_SAFESETPREV(p_item_instance, p_prev_instance) _mg_atomic_store_relaxed_ptrdiff(_MG_PVA_PTRDIFF(&(p_item_instance)->p_prev_item), (uint8_t *)(p_prev_instance) - (uint8_t *)(p_item_instance))

#include <mutexgear/_llisttmpl.h>

_MUTEXGEAR_DEFINE_LLIST_T(dlra, ptrdiff_t, ptrdiff_t);

#undef _MUTEXGEAR_LITEM_GETNEXT
#undef _MUTEXGEAR_LITEM_SAFEGETPREV
#undef _MUTEXGEAR_LITEM_SETNEXT
#undef _MUTEXGEAR_LITEM_CONSTRPREV
#undef _MUTEXGEAR_LITEM_DESTRPREV
#undef _MUTEXGEAR_LITEM_UNSAFESETPREV
#undef _MUTEXGEAR_LITEM_SAFESETPREV


//////////////////////////////////////////////////////////////////////////
// mutexgear_dlraitem_t declaration

typedef _t_mutexgear_dlraitem_t mutexgear_dlraitem_t;


//////////////////////////////////////////////////////////////////////////
// Public mutexgear_dlraitem_t API Definitions

_MUTEXGEAR_PURE_INLINE void mutexgear_dlraitem_init(mutexgear_dlraitem_t *p_item_instance) { _t_mutexgear_dlraitem_init(p_item_instance); }
_MUTEXGEAR_PURE_INLINE void mutexgear_dlraitem_destroy(mutexgear_dlraitem_t *p_item_instance) { _t_mutexgear_dlraitem_destroy(p_item_instance); }

_MUTEXGEAR_PURE_INLINE bool mutexgear_dlraitem_islinked(const mutexgear_dlraitem_t *p_item_instance) { return _t_mutexgear_dlraitem_islinked(p_item_instance);  }

_MUTEXGEAR_PURE_INLINE mutexgear_dlraitem_t *mutexgear_dlraitem_getnext(const mutexgear_dlraitem_t *p_item_instance) { return _t_mutexgear_dlraitem_getnext(p_item_instance); }
_MUTEXGEAR_PURE_INLINE mutexgear_dlraitem_t *mutexgear_dlraitem_getprevious(const mutexgear_dlraitem_t *p_item_instance) { return _t_mutexgear_dlraitem_getprevious(p_item_instance);  }


//////////////////////////////////////////////////////////////////////////
// mutexgear_dlralist_t declaration

typedef _t_mutexgear_dlralist_t mutexgear_dlralist_t;


//////////////////////////////////////////////////////////////////////////
// Public mutexgear_dlralist_t API Definitions

_MUTEXGEAR_PURE_INLINE void mutexgear_dlralist_init(mutexgear_dlralist_t *p_list_instance) { _t_mutexgear_dlralist_init(p_list_instance); }
_MUTEXGEAR_PURE_INLINE void mutexgear_dlralist_destroy(mutexgear_dlralist_t *p_list_instance) { _t_mutexgear_dlralist_destroy(p_list_instance); }

_MUTEXGEAR_PURE_INLINE void mutexgear_dlralist_swap(mutexgear_dlralist_t *p_list_instance, mutexgear_dlralist_t *p_another_list) { _t_mutexgear_dlralist_swap(p_list_instance, p_another_list); }

_MUTEXGEAR_PURE_INLINE void mutexgear_dlralist_detach(mutexgear_dlralist_t *p_list_instance) { _t_mutexgear_dlralist_detach(p_list_instance); }

_MUTEXGEAR_PURE_INLINE bool mutexgear_dlralist_isempty(const mutexgear_dlralist_t *p_list_instance) { return _t_mutexgear_dlralist_isempty(p_list_instance); }

_MUTEXGEAR_PURE_INLINE mutexgear_dlraitem_t *mutexgear_dlralist_getbegin(const mutexgear_dlralist_t *p_list_instance) { return _t_mutexgear_dlralist_getbegin(p_list_instance); }
_MUTEXGEAR_PURE_INLINE mutexgear_dlraitem_t *mutexgear_dlralist_getend(const mutexgear_dlralist_t *p_list_instance) { return _t_mutexgear_dlralist_getend(p_list_instance); }
_MUTEXGEAR_PURE_INLINE mutexgear_dlraitem_t *mutexgear_dlralist_getrbegin(const mutexgear_dlralist_t *p_list_instance) {return _t_mutexgear_dlralist_getrbegin(p_list_instance); }
_MUTEXGEAR_PURE_INLINE mutexgear_dlraitem_t *mutexgear_dlralist_getrend(const mutexgear_dlralist_t *p_list_instance) { return _t_mutexgear_dlralist_getrend(p_list_instance); }
_MUTEXGEAR_PURE_INLINE bool mutexgear_dlralist_trygetprevious(mutexgear_dlraitem_t **p_out_previous_item, const mutexgear_dlralist_t *p_list_instance, const mutexgear_dlraitem_t *p_item_instance) { return _t_mutexgear_dlralist_trygetprevious(p_out_previous_item, p_list_instance, p_item_instance); }

_MUTEXGEAR_PURE_INLINE void mutexgear_dlralist_linkat(mutexgear_dlralist_t *p_list_instance, mutexgear_dlraitem_t *p_item_to_be_linked, mutexgear_dlraitem_t *p_insert_before_item) { _t_mutexgear_dlralist_linkat(p_list_instance, p_item_to_be_linked, p_insert_before_item); }
_MUTEXGEAR_PURE_INLINE void mutexgear_dlralist_linkfront(mutexgear_dlralist_t *p_list_instance, mutexgear_dlraitem_t *p_item_to_be_linked) { _t_mutexgear_dlralist_linkfront(p_list_instance, p_item_to_be_linked); }
_MUTEXGEAR_PURE_INLINE void mutexgear_dlralist_linkback(mutexgear_dlralist_t *p_list_instance, mutexgear_dlraitem_t *p_item_to_be_linked) { _t_mutexgear_dlralist_linkback(p_list_instance, p_item_to_be_linked); }

_MUTEXGEAR_PURE_INLINE void mutexgear_dlralist_unlink(mutexgear_dlraitem_t *p_item_to_be_unlinked) { _t_mutexgear_dlralist_unlink(p_item_to_be_unlinked); }


_MUTEXGEAR_PURE_INLINE void mutexgear_dlralist_spliceat(mutexgear_dlralist_t *p_list_instance, mutexgear_dlraitem_t *p_insert_before_item, mutexgear_dlraitem_t *p_to_be_spliced_begin, mutexgear_dlraitem_t *p_to_be_spliced_end) { _t_mutexgear_dlralist_spliceat(p_list_instance, p_insert_before_item, p_to_be_spliced_begin, p_to_be_spliced_end); }
_MUTEXGEAR_PURE_INLINE void mutexgear_dlralist_splicefront(mutexgear_dlralist_t *p_list_instance, mutexgear_dlraitem_t *p_to_be_spliced_begin, mutexgear_dlraitem_t *p_to_be_spliced_end) { _t_mutexgear_dlralist_splicefront(p_list_instance, p_to_be_spliced_begin, p_to_be_spliced_end); }
_MUTEXGEAR_PURE_INLINE void mutexgear_dlralist_spliceback(mutexgear_dlralist_t *p_list_instance, mutexgear_dlraitem_t *p_to_be_spliced_begin, mutexgear_dlraitem_t *p_to_be_spliced_end) { _t_mutexgear_dlralist_spliceback(p_list_instance, p_to_be_spliced_begin, p_to_be_spliced_end); }


_MUTEXGEAR_END_EXTERN_C();


#endif // #ifndef __MUTEXGEAR_DLRALIST_H_INCLUDED
