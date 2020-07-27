#ifndef __MUTEXGEAR_DLPSLIST_H_INCLUDED
#define __MUTEXGEAR_DLPSLIST_H_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear Double-Linked Pointed Simple List API Implementation       */
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
 *	\brief MutexGear Double-Linked Pointed Simple List API implementation
 *
 *	The header defines a double-linked list object to be embedded within
 *	other memory structures. The list elements are connected with plain pointers.
 */


#include <mutexgear/config.h>
#include <mutexgear/utility.h>


_MUTEXGEAR_BEGIN_EXTERN_C();

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// The list is defined via template

#define _MUTEXGEAR_LITEM_GETNEXT(p_item_instance) ((p_item_instance)->p_next_item)
#define _MUTEXGEAR_LITEM_GETPREV(p_item_instance) ((p_item_instance)->p_prev_item)
#define _MUTEXGEAR_LITEM_SETNEXT(p_item_instance, new_next_instance) ((p_item_instance)->p_next_item = (new_next_instance))
#define _MUTEXGEAR_LITEM_CONSTRPREV(p_item_instance, new_prev_instance) ((p_item_instance)->p_next_item = (new_prev_instance))
#define _MUTEXGEAR_LITEM_DESTRPREV(p_item_instance) ((void)(p_item_instance))
#define _MUTEXGEAR_LITEM_UNSAFESETPREV(p_item_instance, new_prev_instance) ((p_item_instance)->p_prev_item = (new_prev_instance))
#define _MUTEXGEAR_LITEM_SAFESETPREV(p_item_instance, new_prev_instance) ((p_item_instance)->p_prev_item = (new_prev_instance))
#define _MUTEXGEAR_LITEM_TRYSETPREV(p_item_instance, p_expected_prev_instance, new_prev_instance) ((p_item_instance)->p_prev_item == *(p_expected_prev_instance) ? ((p_item_instance)->p_prev_item = (new_prev_instance), true) : (*(p_expected_prev_instance) = (p_item_instance)->p_prev_item, false))
#define _MUTEXGEAR_LITEM_SWAPPREV(p_item_instance, new_prev_instance) _mutexgear_dlpsitem_swap_items(&(p_item_instance)->p_prev_item, new_prev_instance)

#include "_llisttmpl.h"

_MUTEXGEAR_DEFINE_LLIST_T(dlps, _t_mutexgear_dlpsitem_t *, _t_mutexgear_dlpsitem_t *);

_MUTEXGEAR_PURE_INLINE
_t_mutexgear_dlpsitem_t *_mutexgear_dlpsitem_swap_items(_t_mutexgear_dlpsitem_t **p_item_storage, _t_mutexgear_dlpsitem_t *new_item_instance)
{
	_t_mutexgear_dlpsitem_t *existing_item_instance = *p_item_storage;
	*p_item_storage = new_item_instance;
	return existing_item_instance;
}

#undef _MUTEXGEAR_LITEM_GETNEXT
#undef _MUTEXGEAR_LITEM_GETPREV
#undef _MUTEXGEAR_LITEM_SETNEXT
#undef _MUTEXGEAR_LITEM_CONSTRPREV
#undef _MUTEXGEAR_LITEM_DESTRPREV
#undef _MUTEXGEAR_LITEM_UNSAFESETPREV
#undef _MUTEXGEAR_LITEM_SAFESETPREV
#undef _MUTEXGEAR_LITEM_TRYSETPREV
#undef _MUTEXGEAR_LITEM_SWAPPREV


//////////////////////////////////////////////////////////////////////////
// mutexgear_dlraitem_t declaration

typedef _t_mutexgear_dlpsitem_t mutexgear_dlpsitem_t;


//////////////////////////////////////////////////////////////////////////
// Public mutexgear_dlpsitem_t API Definitions

_MUTEXGEAR_PURE_INLINE void mutexgear_dlpsitem_init(mutexgear_dlpsitem_t *p_item_instance) { _t_mutexgear_dlpsitem_init(p_item_instance); }
_MUTEXGEAR_PURE_INLINE void mutexgear_dlpsitem_destroy(mutexgear_dlpsitem_t *p_item_instance) { _t_mutexgear_dlpsitem_destroy(p_item_instance); }

_MUTEXGEAR_PURE_INLINE bool mutexgear_dlpsitem_islinked(const mutexgear_dlpsitem_t *p_item_instance) { return _t_mutexgear_dlpsitem_islinked(p_item_instance); }

_MUTEXGEAR_PURE_INLINE mutexgear_dlpsitem_t *mutexgear_dlpsitem_getnext(const mutexgear_dlpsitem_t *p_item_instance) { return _t_mutexgear_dlpsitem_getnext(p_item_instance); }
_MUTEXGEAR_PURE_INLINE mutexgear_dlpsitem_t *mutexgear_dlpsitem_getprevious(const mutexgear_dlpsitem_t *p_item_instance) { return _t_mutexgear_dlpsitem_getprevious(p_item_instance); }


//////////////////////////////////////////////////////////////////////////
// mutexgear_dlpslist_t declaration

typedef _t_mutexgear_dlpslist_t mutexgear_dlpslist_t;


//////////////////////////////////////////////////////////////////////////
// Public mutexgear_dlpslist_t API Definitions

_MUTEXGEAR_PURE_INLINE void mutexgear_dlpslist_init(mutexgear_dlpslist_t *p_list_instance) { _t_mutexgear_dlpslist_init(p_list_instance); }
_MUTEXGEAR_PURE_INLINE void mutexgear_dlpslist_destroy(mutexgear_dlpslist_t *p_list_instance) { _t_mutexgear_dlpslist_destroy(p_list_instance); }

_MUTEXGEAR_PURE_INLINE void mutexgear_dlpslist_swap(mutexgear_dlpslist_t *p_list_instance, mutexgear_dlpslist_t *p_another_list) { _t_mutexgear_dlpslist_swap(p_list_instance, p_another_list); }

_MUTEXGEAR_PURE_INLINE void mutexgear_dlpslist_detach(mutexgear_dlpslist_t *p_list_instance) { _t_mutexgear_dlpslist_detach(p_list_instance); }

_MUTEXGEAR_PURE_INLINE bool mutexgear_dlpslist_isempty(const mutexgear_dlpslist_t *p_list_instance) { return _t_mutexgear_dlpslist_isempty(p_list_instance); }

_MUTEXGEAR_PURE_INLINE mutexgear_dlpsitem_t *mutexgear_dlpslist_getbegin(const mutexgear_dlpslist_t *p_list_instance) { return _t_mutexgear_dlpslist_getbegin(p_list_instance); }
_MUTEXGEAR_PURE_INLINE mutexgear_dlpsitem_t *mutexgear_dlpslist_getend(const mutexgear_dlpslist_t *p_list_instance) { return _t_mutexgear_dlpslist_getend(p_list_instance); }
_MUTEXGEAR_PURE_INLINE mutexgear_dlpsitem_t *mutexgear_dlpslist_getrbegin(const mutexgear_dlpslist_t *p_list_instance) { return _t_mutexgear_dlpslist_getrbegin(p_list_instance); }
_MUTEXGEAR_PURE_INLINE mutexgear_dlpsitem_t *mutexgear_dlpslist_getrend(const mutexgear_dlpslist_t *p_list_instance) { return _t_mutexgear_dlpslist_getrend(p_list_instance); }
_MUTEXGEAR_PURE_INLINE bool mutexgear_dlpslist_trygetprevious(mutexgear_dlpsitem_t **p_out_previous_item, const mutexgear_dlpslist_t *p_list_instance, const mutexgear_dlpsitem_t *p_item_instance) { return _t_mutexgear_dlpslist_trygetprevious(p_out_previous_item, p_list_instance, p_item_instance); }

_MUTEXGEAR_PURE_INLINE void mutexgear_dlpslist_linkat(mutexgear_dlpslist_t *p_list_instance, mutexgear_dlpsitem_t *p_item_to_be_linked, mutexgear_dlpsitem_t *p_insert_before_item) { _t_mutexgear_dlpslist_linkat(p_list_instance, p_item_to_be_linked, p_insert_before_item); }
_MUTEXGEAR_PURE_INLINE void mutexgear_dlpslist_linkfront(mutexgear_dlpslist_t *p_list_instance, mutexgear_dlpsitem_t *p_item_to_be_linked) { _t_mutexgear_dlpslist_linkfront(p_list_instance, p_item_to_be_linked); }
_MUTEXGEAR_PURE_INLINE void mutexgear_dlpslist_linkback(mutexgear_dlpslist_t *p_list_instance, mutexgear_dlpsitem_t *p_item_to_be_linked) { _t_mutexgear_dlpslist_linkback(p_list_instance, p_item_to_be_linked); }

_MUTEXGEAR_PURE_INLINE void mutexgear_dlpslist_unlink(mutexgear_dlpsitem_t *p_item_to_be_unlinked) { _t_mutexgear_dlpslist_unlink(p_item_to_be_unlinked); }


_MUTEXGEAR_PURE_INLINE void mutexgear_dlpslist_spliceat(mutexgear_dlpslist_t *p_list_instance, mutexgear_dlpsitem_t *p_insert_before_item, mutexgear_dlpsitem_t *p_to_be_spliced_begin, mutexgear_dlpsitem_t *p_to_be_spliced_end) { _t_mutexgear_dlpslist_spliceat(p_list_instance, p_insert_before_item, p_to_be_spliced_begin, p_to_be_spliced_end); }
_MUTEXGEAR_PURE_INLINE void mutexgear_dlpslist_splicefront(mutexgear_dlpslist_t *p_list_instance, mutexgear_dlpsitem_t *p_to_be_spliced_begin, mutexgear_dlpsitem_t *p_to_be_spliced_end) { _t_mutexgear_dlpslist_splicefront(p_list_instance, p_to_be_spliced_begin, p_to_be_spliced_end); }
_MUTEXGEAR_PURE_INLINE void mutexgear_dlpslist_spliceback(mutexgear_dlpslist_t *p_list_instance, mutexgear_dlpsitem_t *p_to_be_spliced_begin, mutexgear_dlpsitem_t *p_to_be_spliced_end) { _t_mutexgear_dlpslist_spliceback(p_list_instance, p_to_be_spliced_begin, p_to_be_spliced_end); }


_MUTEXGEAR_END_EXTERN_C();


#endif // #ifndef __MUTEXGEAR_DLPSLIST_H_INCLUDED
