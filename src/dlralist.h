#ifndef __MUTEXGEAR_MG_DLRALIST_H_INCLUDED
#define __MUTEXGEAR_MG_DLRALIST_H_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear Double-Linked Relative Atomic List Internal Definitions    */
/*                                                                      */
/* WARNING!                                                             */
/* This library contains a synchronization technique protected by       */
/* the U.S. Patent 9,983,913.                                           */
/*                                                                      */
/* THIS IS A PRE-RELEASE LIBRARY SNAPSHOT.                              */
/* AWAIT THE RELEASE AT https://mutexgear.com                           */
/*                                                                      */
/* Copyright (c) 2016-2022 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/

/**
 *	\file
 *	\brief MutexGear Double-Linked Relative Atomic List internal definitions
 *
 *	The DLRA List related types and functions internally used by other parts of
 *	the Library.
 */


#include <mutexgear/dlralist.h>


 //////////////////////////////////////////////////////////////////////////
 // RWLock helper functions

_MUTEXGEAR_PURE_INLINE _MUTEXGEAR_ALWAYS_INLINE mutexgear_dlraitem_t *_mutexgear_dlraitem_getfromprevious(mutexgear_dlraitem_prev_t *p_item_previous)
{
	MG_DECLARE_TYPE_CAST(mutexgear_dlraitem_t);
	return MG_PERFORM_TYPE_CAST((uint8_t *)p_item_previous - offsetof(mutexgear_dlraitem_t, p_prev_item));
}

_MUTEXGEAR_PURE_INLINE _MUTEXGEAR_ALWAYS_INLINE void _mutexgear_dlraitem_initprevious(mutexgear_dlraitem_t *p_item_instance, mutexgear_dlraitem_t *prev_instance) { _t__mutexgear_dlraitem_constructprevious(p_item_instance, prev_instance); }
_MUTEXGEAR_PURE_INLINE _MUTEXGEAR_ALWAYS_INLINE void _mutexgear_dlraitem_destroyprevious(mutexgear_dlraitem_t *p_item_instance) { _t__mutexgear_dlraitem_destroyprevious(p_item_instance); }

_MUTEXGEAR_PURE_INLINE _MUTEXGEAR_ALWAYS_INLINE void _mutexgear_dlraitem_setnext(mutexgear_dlraitem_t *p_item_instance, mutexgear_dlraitem_t *next_instance) { _t__mutexgear_dlraitem_setnext(p_item_instance, next_instance); }
_MUTEXGEAR_PURE_INLINE _MUTEXGEAR_ALWAYS_INLINE void _mutexgear_dlraitem_setunsafeprevious(mutexgear_dlraitem_t *p_item_instance, mutexgear_dlraitem_t *prev_instance) { _t__mutexgear_dlraitem_setunsafeprevious(p_item_instance, prev_instance); }
_MUTEXGEAR_PURE_INLINE _MUTEXGEAR_ALWAYS_INLINE void _mutexgear_dlraitem_setprevious(mutexgear_dlraitem_t *p_item_instance, mutexgear_dlraitem_t *prev_instance) { _t__mutexgear_dlraitem_setprevious(p_item_instance, prev_instance); }
_MUTEXGEAR_PURE_INLINE _MUTEXGEAR_ALWAYS_INLINE bool _mutexgear_dlraitem_trysetprevious(mutexgear_dlraitem_t *p_item_instance, mutexgear_dlraitem_t **p_expected_prev_instance, mutexgear_dlraitem_t *new_prev_instance) { return _t__mutexgear_dlraitem_trysetprevious(p_item_instance, p_expected_prev_instance, new_prev_instance); }
_MUTEXGEAR_PURE_INLINE _MUTEXGEAR_ALWAYS_INLINE mutexgear_dlraitem_t *_mutexgear_dlraitem_swapprevious(mutexgear_dlraitem_t *p_item_instance, mutexgear_dlraitem_t *new_prev_instance) { return _t__mutexgear_dlraitem_swapprevious(p_item_instance, new_prev_instance); }
_MUTEXGEAR_PURE_INLINE _MUTEXGEAR_ALWAYS_INLINE void _mutexgear_dlraitem_unsaferesettounlinked(mutexgear_dlraitem_t *p_item_instance) { _t__mutexgear_dlraitem_unsaferesettounlinked(p_item_instance); }


_MUTEXGEAR_PURE_INLINE void _mutexgear_dlralist_multilinkat(mutexgear_dlralist_t *p_list_instance, mutexgear_dlraitem_t *p_first_item_to_be_linked, mutexgear_dlraitem_t *p_last_item_to_be_linked, mutexgear_dlraitem_t *p_insert_before_item) { _t_mutexgear_dlralist_multilinkat(p_list_instance, p_first_item_to_be_linked, p_last_item_to_be_linked, p_insert_before_item); }


#endif // #ifndef __MUTEXGEAR_MG_DLRALIST_H_INCLUDED
