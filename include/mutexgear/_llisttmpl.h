// #ifndef __MUTEXGEAR__LLISTTMPL_H_INCLUDED -- allow multiple inclusions
// #define __MUTEXGEAR__LLISTTMPL_H_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear Linked List API Template                                   */
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
 *	\brief MutexGear Linked List API template
 *
 *	The header defines a linked list object template to be used for linked list generation
 */


#include <mutexgear/utility.h>


 // NITPIDR = This is not expected to be participating in data races


//////////////////////////////////////////////////////////////////////////
// Definition checks

// NOTE: Remember to add #undefs for all the new accessors at the end of the file

#ifndef _MUTEXGEAR_LITEM_GETNEXT
#error Please define _MUTEXGEAR_LITEM_GETNEXT
#endif

#ifndef _MUTEXGEAR_LITEM_SAFEGETPREV
#error Please define _MUTEXGEAR_LITEM_SAFEGETPREV
#endif

#ifndef _MUTEXGEAR_LITEM_SETNEXT
#error Please define _MUTEXGEAR_LITEM_SETNEXT
#endif

#ifndef _MUTEXGEAR_LITEM_CONSTRPREV
#error Please define _MUTEXGEAR_LITEM_CONSTRPREV
#endif

#ifndef _MUTEXGEAR_LITEM_DESTRPREV
#error Please define _MUTEXGEAR_LITEM_DESTRPREV
#endif

#ifndef _MUTEXGEAR_LITEM_UNSAFESETPREV
#error Please define _MUTEXGEAR_LITEM_UNSAFESETPREV
#endif

#ifndef _MUTEXGEAR_LITEM_SAFESETPREV
#error Please define _MUTEXGEAR_LITEM_SAFESETPREV
#endif

// NOTE: Remember to add #undefs for all the new accessors at the end of the file


 //////////////////////////////////////////////////////////////////////////

#undef _MUTEXGEAR_DEFINE_LLIST_T
#define _MUTEXGEAR_DEFINE_LLIST_T(t_prefix, t_next_t, t_prev_t) \
\
/************************** Item implementation **************************/ \
\
typedef struct _t__mutexgear_##t_prefix##item _t_mutexgear_##t_prefix##item_t; \
struct _t__mutexgear_##t_prefix##item \
{ \
	t_next_t				p_next_item; \
	t_prev_t				p_prev_item; \
\
}; \
\
/* Forward declarations */ \
\
_MUTEXGEAR_PURE_INLINE void _t__mutexgear_##t_prefix##item_internalinitunlinked(_t_mutexgear_##t_prefix##item_t *p_item_instance); \
_MUTEXGEAR_PURE_INLINE void _t__mutexgear_##t_prefix##item_internaldestroy(_t_mutexgear_##t_prefix##item_t *p_item_instance); \
_MUTEXGEAR_PURE_INLINE _t_mutexgear_##t_prefix##item_t *_t_mutexgear_##t_prefix##item_getprevious(const _t_mutexgear_##t_prefix##item_t *p_item_instance); \
\
/* Public litem_t definitions */ \
\
_MUTEXGEAR_PURE_INLINE \
void _t_mutexgear_##t_prefix##item_init(_t_mutexgear_##t_prefix##item_t *p_item_instance) \
{ \
	_t__mutexgear_##t_prefix##item_internalinitunlinked(p_item_instance); \
} \
\
_MUTEXGEAR_PURE_INLINE \
bool _t_mutexgear_##t_prefix##item_islinked(const _t_mutexgear_##t_prefix##item_t *p_item_instance) \
{ \
	return _t_mutexgear_##t_prefix##item_getprevious(p_item_instance) != p_item_instance; \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t_mutexgear_##t_prefix##item_destroy(_t_mutexgear_##t_prefix##item_t *p_item_instance) \
{ \
	MG_ASSERT(!_t_mutexgear_##t_prefix##item_islinked(p_item_instance)); \
\
	_t__mutexgear_##t_prefix##item_internaldestroy(p_item_instance); \
} \
\
_MUTEXGEAR_PURE_INLINE \
_t_mutexgear_##t_prefix##item_t *_t_mutexgear_##t_prefix##item_getnext(const _t_mutexgear_##t_prefix##item_t *p_item_instance) \
{ \
	return (_t_mutexgear_##t_prefix##item_t *)_MUTEXGEAR_LITEM_GETNEXT(p_item_instance); \
} \
\
_MUTEXGEAR_PURE_INLINE \
_t_mutexgear_##t_prefix##item_t *_t_mutexgear_##t_prefix##item_getprevious(const _t_mutexgear_##t_prefix##item_t *p_item_instance) \
{ \
	return (_t_mutexgear_##t_prefix##item_t *)_MUTEXGEAR_LITEM_SAFEGETPREV(p_item_instance); \
} \
\
/* Private litem_t definitions */ \
\
_MUTEXGEAR_PURE_INLINE \
void _t__mutexgear_##t_prefix##item_setnext(_t_mutexgear_##t_prefix##item_t *p_item_instance, _t_mutexgear_##t_prefix##item_t *p_next_instance) \
{ \
	_MUTEXGEAR_LITEM_SETNEXT(p_item_instance, p_next_instance); \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t__mutexgear_##t_prefix##item_constructprevious(_t_mutexgear_##t_prefix##item_t *p_item_instance, _t_mutexgear_##t_prefix##item_t *p_prev_instance) \
{ \
	_MUTEXGEAR_LITEM_CONSTRPREV(p_item_instance, p_prev_instance); \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t__mutexgear_##t_prefix##item_destroyprevious(_t_mutexgear_##t_prefix##item_t *p_item_instance) \
{ \
	_MUTEXGEAR_LITEM_DESTRPREV(p_item_instance); \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t__mutexgear_##t_prefix##item_unsafesetprevious(_t_mutexgear_##t_prefix##item_t *p_item_instance, _t_mutexgear_##t_prefix##item_t *p_prev_instance) \
{ \
	_MUTEXGEAR_LITEM_UNSAFESETPREV(p_item_instance, p_prev_instance); \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t__mutexgear_##t_prefix##item_setprevious(_t_mutexgear_##t_prefix##item_t *p_item_instance, _t_mutexgear_##t_prefix##item_t *p_prev_instance) \
{ \
	_MUTEXGEAR_LITEM_SAFESETPREV(p_item_instance, p_prev_instance); \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t__mutexgear_##t_prefix##item_internalinitselflinked(_t_mutexgear_##t_prefix##item_t *p_item_instance) \
{ \
	_t__mutexgear_##t_prefix##item_setnext(p_item_instance, p_item_instance); \
	_t__mutexgear_##t_prefix##item_constructprevious(p_item_instance, p_item_instance); \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t__mutexgear_##t_prefix##item_internalinitunlinked(_t_mutexgear_##t_prefix##item_t *p_item_instance) \
{ \
	_t__mutexgear_##t_prefix##item_constructprevious(p_item_instance, p_item_instance); \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t__mutexgear_##t_prefix##item_internaldestroy(_t_mutexgear_##t_prefix##item_t *p_item_instance) \
{ \
	_t__mutexgear_##t_prefix##item_destroyprevious(p_item_instance); \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t__mutexgear_##t_prefix##item_resettounlinked(_t_mutexgear_##t_prefix##item_t *p_item_instance) \
{ \
	/* Store relaxed since this is the only write and there are no dependent ones */ \
	_t__mutexgear_##t_prefix##item_setprevious(p_item_instance, p_item_instance); \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t__mutexgear_##t_prefix##item_unsaferesettoselflinked(_t_mutexgear_##t_prefix##item_t *p_item_instance) \
{ \
	_t__mutexgear_##t_prefix##item_setnext(p_item_instance, p_item_instance); \
	_t__mutexgear_##t_prefix##item_unsafesetprevious(p_item_instance, p_item_instance); \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t__mutexgear_##t_prefix##item_naive_swap(_t_mutexgear_##t_prefix##item_t *p_item_instance, _t_mutexgear_##t_prefix##item_t *p_another_item) \
{ \
	_t_mutexgear_##t_prefix##item_t *p_next_item = _t_mutexgear_##t_prefix##item_getnext(p_item_instance); \
	_t__mutexgear_##t_prefix##item_setnext(p_item_instance, _t_mutexgear_##t_prefix##item_getnext(p_another_item)); \
	_t__mutexgear_##t_prefix##item_setnext(p_another_item, p_next_item); \
 \
	/* Note: The function is not intended to be invoked concurrently with mutexgear_is_dlitem_linked and thus, */ \
	/* the atomic assignment/retrieval is not necessary here. */ \
	_t_mutexgear_##t_prefix##item_t *p_prev_item = _t_mutexgear_##t_prefix##item_getprevious(p_item_instance); \
	_t__mutexgear_##t_prefix##item_unsafesetprevious(p_item_instance, _t_mutexgear_##t_prefix##item_getprevious(p_another_item)); \
	_t__mutexgear_##t_prefix##item_unsafesetprevious(p_another_item, p_prev_item); \
} \
\
/************************** List implementation **************************/ \
\
typedef struct _t__mutexgear_##t_prefix##list \
{ \
	_t_mutexgear_##t_prefix##item_t		end_item; \
\
} _t_mutexgear_##t_prefix##list_t; \
\
/* Public llist_t definitions */ \
\
_MUTEXGEAR_PURE_INLINE \
void _t_mutexgear_##t_prefix##list_init(_t_mutexgear_##t_prefix##list_t *p_list_instance) \
{ \
	_t__mutexgear_##t_prefix##item_internalinitselflinked(&p_list_instance->end_item); \
} \
\
_MUTEXGEAR_PURE_INLINE  \
void _t_mutexgear_##t_prefix##list_destroy(_t_mutexgear_##t_prefix##list_t *p_list_instance) \
{ \
	_t__mutexgear_##t_prefix##item_internaldestroy(&p_list_instance->end_item); \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t_mutexgear_##t_prefix##list_swap(_t_mutexgear_##t_prefix##list_t *p_list_instance, _t_mutexgear_##t_prefix##list_t *p_another_list) \
{ \
	if (p_list_instance != p_another_list) \
	{ \
		_t_mutexgear_##t_prefix##item_t *p_own_next = _t_mutexgear_##t_prefix##item_getnext(&p_list_instance->end_item); \
		_t_mutexgear_##t_prefix##item_t *p_own_prev = _t_mutexgear_##t_prefix##item_getprevious(&p_list_instance->end_item); \
		_t__mutexgear_##t_prefix##item_setprevious(p_own_next, &p_another_list->end_item); \
		_t__mutexgear_##t_prefix##item_setnext(p_own_prev, &p_another_list->end_item); \
		\
		_t_mutexgear_##t_prefix##item_t *p_theother_next = _t_mutexgear_##t_prefix##item_getnext(&p_another_list->end_item); \
		_t_mutexgear_##t_prefix##item_t *p_theoother_prev = _t_mutexgear_##t_prefix##item_getprevious(&p_another_list->end_item); \
		_t__mutexgear_##t_prefix##item_setprevious(p_theother_next, &p_list_instance->end_item); \
		_t__mutexgear_##t_prefix##item_setnext(p_theoother_prev, &p_list_instance->end_item); \
		\
		_t__mutexgear_##t_prefix##item_naive_swap(&p_list_instance->end_item, &p_another_list->end_item); \
	} \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t_mutexgear_##t_prefix##list_detach(_t_mutexgear_##t_prefix##list_t *p_list_instance) \
{ \
	_t__mutexgear_##t_prefix##item_unsaferesettoselflinked(&p_list_instance->end_item); \
} \
\
_MUTEXGEAR_PURE_INLINE \
bool _t_mutexgear_##t_prefix##list_isempty(const _t_mutexgear_##t_prefix##list_t *p_list_instance) \
{ \
	return !_t_mutexgear_##t_prefix##item_islinked(&p_list_instance->end_item); \
} \
\
_MUTEXGEAR_PURE_INLINE \
_t_mutexgear_##t_prefix##item_t *_t_mutexgear_##t_prefix##list_getbegin(const _t_mutexgear_##t_prefix##list_t *p_list_instance) \
{ \
	return _t_mutexgear_##t_prefix##item_getnext(&p_list_instance->end_item); \
} \
_MUTEXGEAR_PURE_INLINE \
_t_mutexgear_##t_prefix##item_t *_t_mutexgear_##t_prefix##list_getend(const _t_mutexgear_##t_prefix##list_t *p_list_instance) \
{ \
	const _t_mutexgear_##t_prefix##item_t *p_result = &p_list_instance->end_item; return (_t_mutexgear_##t_prefix##item_t *)p_result; \
} \
\
_MUTEXGEAR_PURE_INLINE \
_t_mutexgear_##t_prefix##item_t *_t_mutexgear_##t_prefix##list_getrbegin(const _t_mutexgear_##t_prefix##list_t *p_list_instance) \
{ \
	return _t_mutexgear_##t_prefix##item_getprevious(&p_list_instance->end_item); \
} \
\
_MUTEXGEAR_PURE_INLINE \
_t_mutexgear_##t_prefix##item_t *_t_mutexgear_##t_prefix##list_getrend(const _t_mutexgear_##t_prefix##list_t *p_list_instance) \
{ \
	const _t_mutexgear_##t_prefix##item_t *p_result = &p_list_instance->end_item; return (_t_mutexgear_##t_prefix##item_t *)p_result; \
} \
\
_MUTEXGEAR_PURE_INLINE \
bool _t_mutexgear_##t_prefix##list_trygetprevious(_t_mutexgear_##t_prefix##item_t **p_out_previous_item, \
	const _t_mutexgear_##t_prefix##list_t *p_list_instance, const _t_mutexgear_##t_prefix##item_t *p_item_instance) \
{ \
	_t_mutexgear_##t_prefix##item_t *previous_item = _t_mutexgear_##t_prefix##item_getprevious(p_item_instance); \
	return previous_item != &p_list_instance->end_item && (*p_out_previous_item = previous_item, true); \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t_mutexgear_##t_prefix##list_linkat(_t_mutexgear_##t_prefix##list_t *p_list_instance, _t_mutexgear_##t_prefix##item_t *p_item_to_be_linked, _t_mutexgear_##t_prefix##item_t *p_insert_before_item) \
{ \
	MG_ASSERT(!_t_mutexgear_##t_prefix##item_islinked(p_item_to_be_linked)); \
	\
	_t__mutexgear_##t_prefix##item_setnext(p_item_to_be_linked, p_insert_before_item); \
	\
	_t_mutexgear_##t_prefix##item_t *p_insert_after_item = _t_mutexgear_##t_prefix##item_getprevious(p_insert_before_item); \
	_t__mutexgear_##t_prefix##item_unsafesetprevious(p_item_to_be_linked, p_insert_after_item); /* NITPIDR */ \
	\
	/* This is OK to be stored relaxed -- stronger modes are of no benefit */ \
	_t__mutexgear_##t_prefix##item_setprevious(p_insert_before_item, p_item_to_be_linked); \
	_t__mutexgear_##t_prefix##item_setnext(p_insert_after_item, p_item_to_be_linked); \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t_mutexgear_##t_prefix##list_linkfront(_t_mutexgear_##t_prefix##list_t *p_list_instance, _t_mutexgear_##t_prefix##item_t *p_item_to_be_linked) \
{ \
	_t_mutexgear_##t_prefix##list_linkat(p_list_instance, p_item_to_be_linked, _t_mutexgear_##t_prefix##list_getbegin(p_list_instance)); \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t_mutexgear_##t_prefix##list_linkback(_t_mutexgear_##t_prefix##list_t *p_list_instance, _t_mutexgear_##t_prefix##item_t *p_item_to_be_linked) \
{ \
	_t_mutexgear_##t_prefix##list_linkat(p_list_instance, p_item_to_be_linked, _t_mutexgear_##t_prefix##list_getend(p_list_instance)); \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t_mutexgear_##t_prefix##list_unlink(_t_mutexgear_##t_prefix##item_t *p_item_to_be_unlinked) \
{ \
	MG_ASSERT(_t_mutexgear_##t_prefix##item_islinked(p_item_to_be_unlinked)); \
	\
	_t_mutexgear_##t_prefix##item_t *p_following_item = _t_mutexgear_##t_prefix##item_getnext(p_item_to_be_unlinked); \
	_t_mutexgear_##t_prefix##item_t *p_preceding_item = _t_mutexgear_##t_prefix##item_getprevious(p_item_to_be_unlinked); \
	\
	/* Here the preceding and the next item may be the list end item and it may look as if */ \
	/* the "next" assignment could be a dependency to go before the atomic store. */ \
	/* However, the "linked/empty" checks only test the previous item and using stronger */ \
	/* store modes with correct write order is unnecessary. */ \
	_t__mutexgear_##t_prefix##item_setprevious(p_following_item, p_preceding_item); \
	_t__mutexgear_##t_prefix##item_setnext(p_preceding_item, p_following_item); \
	\
	_t__mutexgear_##t_prefix##item_resettounlinked(p_item_to_be_unlinked); \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t_mutexgear_##t_prefix##list_spliceat(_t_mutexgear_##t_prefix##list_t *p_list_instance, _t_mutexgear_##t_prefix##item_t *p_insert_before_item, \
	_t_mutexgear_##t_prefix##item_t *p_to_be_spliced_begin, _t_mutexgear_##t_prefix##item_t *p_to_be_spliced_end) \
{ \
	if (p_to_be_spliced_begin != p_to_be_spliced_end) \
	{ \
		_t_mutexgear_##t_prefix##item_t *p_last_spliced_item = _t_mutexgear_##t_prefix##item_getprevious(p_to_be_spliced_end); \
		_t__mutexgear_##t_prefix##item_setnext(p_last_spliced_item, p_insert_before_item); \
		\
		/* Save the "prev_item" pointer of the first item to be spliced as the former is going to be overwritten. */ \
		_t_mutexgear_##t_prefix##item_t *p_one_before_first_spliced_item = _t_mutexgear_##t_prefix##item_getprevious(p_to_be_spliced_begin); \
		\
		_t_mutexgear_##t_prefix##item_t *p_insert_after_item = _t_mutexgear_##t_prefix##item_getprevious(p_insert_before_item); \
		_t__mutexgear_##t_prefix##item_setprevious(p_to_be_spliced_begin, p_insert_after_item); \
		\
		_t__mutexgear_##t_prefix##item_setprevious(p_insert_before_item, p_last_spliced_item); \
		_t__mutexgear_##t_prefix##item_setnext(p_insert_after_item, p_to_be_spliced_begin); \
		\
		_t__mutexgear_##t_prefix##item_setnext(p_one_before_first_spliced_item, p_to_be_spliced_end); \
		_t__mutexgear_##t_prefix##item_setprevious(p_to_be_spliced_end, p_one_before_first_spliced_item); \
	} \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t_mutexgear_##t_prefix##list_splicefront(_t_mutexgear_##t_prefix##list_t *p_list_instance, \
	_t_mutexgear_##t_prefix##item_t *p_to_be_spliced_begin, _t_mutexgear_##t_prefix##item_t *p_to_be_spliced_end) \
{ \
	_t_mutexgear_##t_prefix##list_spliceat(p_list_instance, _t_mutexgear_##t_prefix##list_getbegin(p_list_instance), p_to_be_spliced_begin, p_to_be_spliced_end); \
} \
\
_MUTEXGEAR_PURE_INLINE \
void _t_mutexgear_##t_prefix##list_spliceback(_t_mutexgear_##t_prefix##list_t *p_list_instance, \
	_t_mutexgear_##t_prefix##item_t *p_to_be_spliced_begin, _t_mutexgear_##t_prefix##item_t *p_to_be_spliced_end) \
{ \
	_t_mutexgear_##t_prefix##list_spliceat(p_list_instance, _t_mutexgear_##t_prefix##list_getend(p_list_instance), p_to_be_spliced_begin, p_to_be_spliced_end); \
}


// #endif // #ifndef __MUTEXGEAR__LLISTTMPL_H_INCLUDED -- allow multiple inclusions
