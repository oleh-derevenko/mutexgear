#ifndef __MUTEXGEAR_MG_WHEEL_H_INCLUDED
#define __MUTEXGEAR_MG_WHEEL_H_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear Wheel API Internal Definitions                             */
/*                                                                      */
/* WARNING!                                                             */
/* This library contains a synchronization technique protected by       */
/* the U.S. Patent 9,983,913.                                           */
/*                                                                      */
/* THIS IS A PRE-RELEASE LIBRARY SNAPSHOT.                              */
/* AWAIT THE RELEASE AT https://mutexgear.com                           */
/*                                                                      */
/* Copyright (c) 2016-2025 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/

/**
 *	\file
 *	\brief MutexGear Wheel API internal definitions
 *
 *	The header defines a "wheel" object.
 *	The wheel implements independent operation mode as described
 *	in the Patent specification.
 *
 *	NOTE:
 *
 *	The "Wheel" set of functions declared in this file
 *	implements a synchronization mechanism being a subject
 *	of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and Image Database search
 *	(currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm) to view the patent text.
 */


#include "utility.h"
#include <mutexgear/wheel.h>


_MUTEXGEAR_PURE_INLINE int _mutexgear_wheelattr_init(mutexgear_wheelattr_t *__attr);
_MUTEXGEAR_PURE_INLINE int _mutexgear_wheelattr_destroy(mutexgear_wheelattr_t *__attr);

_MUTEXGEAR_PURE_INLINE int _mutexgear_wheelattr_getpshared(const mutexgear_wheelattr_t *__attr, int *__pshared);
_MUTEXGEAR_PURE_INLINE int _mutexgear_wheelattr_setpshared(mutexgear_wheelattr_t *__attr, int __pshared);

_MUTEXGEAR_PURE_INLINE int _mutexgear_wheelattr_getprioceiling(const mutexgear_wheelattr_t *__attr, int *__prioceiling);
_MUTEXGEAR_PURE_INLINE int _mutexgear_wheelattr_getprotocol(const mutexgear_wheelattr_t *__attr, int *__protocol);
_MUTEXGEAR_PURE_INLINE int _mutexgear_wheelattr_setprioceiling(mutexgear_wheelattr_t *__attr, int __prioceiling);
_MUTEXGEAR_PURE_INLINE int _mutexgear_wheelattr_setprotocol(mutexgear_wheelattr_t *__attr, int __protocol);
_MUTEXGEAR_PURE_INLINE int _mutexgear_wheelattr_setmutexattr(mutexgear_wheelattr_t *__attr, const _MUTEXGEAR_LOCKATTR_T *__mutexattr);


_MUTEXGEAR_PURE_INLINE int _mutexgear_wheel_init(mutexgear_wheel_t *__wheel, const mutexgear_wheelattr_t *__attr);
_MUTEXGEAR_PURE_INLINE int _mutexgear_wheel_destroy(mutexgear_wheel_t *__wheel);

static int _mutexgear_wheel_engaged(mutexgear_wheel_t *__wheel);
static int _mutexgear_wheel_advanced(mutexgear_wheel_t *__wheel);
static int _mutexgear_wheel_disengaged(mutexgear_wheel_t *__wheel);

static int _mutexgear_wheel_gripon(mutexgear_wheel_t *__wheel);
static int _mutexgear_wheel_turn(mutexgear_wheel_t *__wheel);
static int _mutexgear_wheel_release(mutexgear_wheel_t *__wheel);


//////////////////////////////////////////////////////////////////////////
// Function implementations

/*static */
int _mutexgear_wheelattr_init(mutexgear_wheelattr_t *__attr)
{
	int ret = _mutexgear_lockattr_init(&__attr->mutexattr);
	return ret;
}

/*static */
int _mutexgear_wheelattr_destroy(mutexgear_wheelattr_t *__attr)
{
	int ret = _mutexgear_lockattr_destroy(&__attr->mutexattr);
	return ret;
}


/*static */
int _mutexgear_wheelattr_getpshared(const mutexgear_wheelattr_t *__attr, int *__out_pshared)
{
	int ret = _mutexgear_lockattr_getpshared(&__attr->mutexattr, __out_pshared);
	return ret;
}

/*static */
int _mutexgear_wheelattr_setpshared(mutexgear_wheelattr_t *__attr, int __pshared)
{
	int ret = _mutexgear_lockattr_setpshared(&__attr->mutexattr, __pshared);
	return ret;
}


/*static */
int _mutexgear_wheelattr_getprioceiling(const mutexgear_wheelattr_t *__attr, int *__out_prioceiling)
{
	int ret = _mutexgear_lockattr_getprioceiling(&__attr->mutexattr, __out_prioceiling);
	return ret;
}

/*static */
int _mutexgear_wheelattr_getprotocol(const mutexgear_wheelattr_t *__attr, int *__out_protocol)
{
	int ret = _mutexgear_lockattr_getprotocol(&__attr->mutexattr, __out_protocol);
	return ret;
}

/*static */
int _mutexgear_wheelattr_setprioceiling(mutexgear_wheelattr_t *__attr, int __prioceiling)
{
	int ret = _mutexgear_lockattr_setprioceiling(&__attr->mutexattr, __prioceiling);
	return ret;
}

/*static */
int _mutexgear_wheelattr_setprotocol(mutexgear_wheelattr_t *__attr, int __protocol)
{
	int ret = _mutexgear_lockattr_setprotocol(&__attr->mutexattr, __protocol);
	return ret;
}


/*_MUTEXGEAR_PURE_INLINE */
int _mutexgear_wheelattr_setmutexattr(mutexgear_wheelattr_t *__attr, const _MUTEXGEAR_LOCKATTR_T *__mutexattr)
{
	int ret;

	do
	{
		int prioceiling_value, protocol_value, pshared_value = 0;

		bool pshared_missing = false;
		if ((ret = _mutexgear_lockattr_getpshared(__mutexattr, &pshared_value)) != EOK)
		{
			if (ret != _MUTEXGEAR_ERRNO__PSHARED_MISSING)
			{
				break;
			}

			pshared_missing = true;
		}

		bool prioceiling_missing = false;
		if ((ret = _mutexgear_lockattr_getprioceiling(__mutexattr, &prioceiling_value)) != EOK)
		{
			if (ret != _MUTEXGEAR_ERRNO__PRIOCEILING_MISSING)
			{
				break;
			}

			prioceiling_missing = true;
		}

		if ((ret = _mutexgear_lockattr_getprotocol(__mutexattr, &protocol_value)) != EOK)
		{
			break;
		}

		if (!pshared_missing && (ret = _mutexgear_lockattr_setpshared(&__attr->mutexattr, pshared_value)) != EOK)
		{
			break;
		}

		// The priority ceiling of 0 means that there is no priority ceiling defined (the default) and some targets fail with EINVAL when when attempting to assign the 0.
		if (!prioceiling_missing && prioceiling_value != 0 && (ret = _mutexgear_lockattr_setprioceiling(&__attr->mutexattr, prioceiling_value)) != EOK)
		{
			break;
		}

		if ((ret = _mutexgear_lockattr_setprotocol(&__attr->mutexattr, protocol_value)) != EOK)
		{
			break;
		}

		MG_ASSERT(ret == EOK);
	}
	while (false);

	return ret;
}



//////////////////////////////////////////////////////////////////////////

// Use negative offsets from MUTEXGEAR_WHEELELEMENT_INVALID to store pushon indices
#define ENCODE_WHEEL_PUSHON_INDEX(idx) (MUTEXGEAR_WHEELELEMENT_INVALID - (idx))
#define DECODE_WHEEL_PUSHON_INDEX(val) (MUTEXGEAR_WHEELELEMENT_INVALID - (val))

/*_MUTEXGEAR_PURE_INLINE */
int _mutexgear_wheel_init(mutexgear_wheel_t *__wheel, const mutexgear_wheelattr_t *__attr)
{
	int ret, mutex_destroy_status;

	const _MUTEXGEAR_LOCKATTR_T *__mutexattr = __attr ? &__attr->mutexattr : NULL;

	bool fault = false;
	unsigned int mutexindex;
	for (mutexindex = 0; mutexindex != MUTEXGEAR_WHEEL_NUMELEMENTS; ++mutexindex)
	{
		if ((ret = _mutexgear_lock_init(__wheel->muteces + mutexindex, __mutexattr)) != EOK)
		{
			for (; mutexindex != 0;)
			{
				--mutexindex;
				MG_CHECK(mutex_destroy_status, (mutex_destroy_status = _mutexgear_lock_destroy(__wheel->muteces + mutexindex)) == EOK);
			}

			fault = true;
			break;
		}
	}

	if (!fault)
	{
		__wheel->wheel_side_index = MUTEXGEAR_WHEELELEMENT_INVALID;
		// To aid pushon operations, initialize client_side_index with the value
		// client side would be starting its pushons with
		__wheel->client_side_index = ENCODE_WHEEL_PUSHON_INDEX(MUTEXGEAR_WHEELELEMENT_ATTACH_FIRST);
	}

	return !fault ? EOK : ret;
}

/*_MUTEXGEAR_PURE_INLINE */
int _mutexgear_wheel_destroy(mutexgear_wheel_t *__wheel)
{
	int ret;

	bool fault = false;
	unsigned int mutexindex;

	if (__wheel->wheel_side_index == MUTEXGEAR_WHEELELEMENT_DESTROYED)
	{
		// The object has already been destroyed
		ret = EINVAL;
		fault = true;
	}

	for (mutexindex = !fault ? MUTEXGEAR_WHEEL_NUMELEMENTS : 0; mutexindex != 0;)
	{
		--mutexindex;

		if ((ret = _mutexgear_lock_destroy(__wheel->muteces + mutexindex)) != EOK)
		{
			fault = true;
			break;
		}
	}

	if (!fault)
	{
		__wheel->wheel_side_index = MUTEXGEAR_WHEELELEMENT_DESTROYED;
	}

	return !fault ? EOK : ret;
}


/*static */
int _mutexgear_wheel_engaged(mutexgear_wheel_t *__wheel)
{
	int ret;

	bool fault = false;

	if (__wheel->wheel_side_index == MUTEXGEAR_WHEELELEMENT_INVALID)
	{
		// The wheel was initialized or the client has detached - OK to proceed

		int first_index = MUTEXGEAR_WHEELELEMENT_ATTACH_FIRST;
		// NOTE! _mutexgear_lock_tryacquire() is used instead of _mutexgear_lock_acquire() here!
		// External logic must guarantee the wheel is free for driven-attaching at the start position
		if ((ret = _mutexgear_lock_tryacquire(__wheel->muteces + first_index)) == EOK)
		{
			__wheel->wheel_side_index = first_index;
		}
		else
		{
			fault = true;
		}
	}
	else if ((unsigned int)__wheel->wheel_side_index >= (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
	{
		// The object is in an invalid state
		ret = EINVAL;
		fault = true;
	}
	else
	{
		// A client has already attached
		ret = EBUSY;
		fault = true;
	}

	return !fault ? EOK : ret;
}

/*static */
int _mutexgear_wheel_advanced(mutexgear_wheel_t *__wheel)
{
	// NOTE: This matches the mutexgear_wheel_turn() but operates with wheel_side_index rather than the client_side_index
	int ret;

	bool fault = false;
	int mutex_unlock_status;

	if (__wheel->wheel_side_index == MUTEXGEAR_WHEELELEMENT_INVALID)
	{
		// Client has not attached yet
		ret = EPERM;
		fault = true;
	}
	else if ((unsigned int)__wheel->wheel_side_index >= (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
	{
		// The object is in an invalid state
		ret = EINVAL;
		fault = true;
	}
	else
	{
		// OK to proceed
		int wheel_side_index = __wheel->wheel_side_index;
		int next_index = wheel_side_index != MUTEXGEAR_WHEEL_NUMELEMENTS - 1 ? wheel_side_index + 1 : 0;

		if ((ret = _mutexgear_lock_tryacquire(__wheel->muteces + next_index)) == EOK)
		{
			if ((ret = _mutexgear_lock_release(__wheel->muteces + wheel_side_index)) == EOK)
			{
				__wheel->wheel_side_index = next_index;
			}
			else
			{
				// Revert the next mutex to the initial state and fault
				MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(__wheel->muteces + next_index)) == EOK); // Should succeed normally

				fault = true;
			}
		}
		else if (ret == EBUSY)
		{
			// If the next mutex is busy, it is an indication that the other side still has a turn available/unfinished 
			// and will be able to still freely check its predicate. Advancing can, therefore, be skipped.
		}
		else
		{
			fault = true;
		}
	}
	// NOTE: The implementation above corresponds to the Option_2 of the "Event Signaling" step 2.
	// The Option_1 would have looked like this:
	//	...
	//	else
	//	{
	//		int wheel_side_index = __wheel->wheel_side_index;
	//		int next_index = wheel_side_index != MUTEXGEAR_WHEEL_NUMELEMENTS - 1 ? wheel_side_index + 1 : 0;
	//
	//		if ((ret = _mutexgear_lock_acquire(__wheel->muteces + next_index)) == EOK
	//			&& (ret = _mutexgear_lock_release(__wheel->muteces + wheel_side_index)) == EOK)
	//		{
	//			__wheel->wheel_side_index = next_index;
	//		}
	//		else
	//		{
	//			fault = 1;
	//		}
	//	}
	//
	return !fault ? EOK : ret;
}

/*static */
int _mutexgear_wheel_disengaged(mutexgear_wheel_t *__wheel)
{
	// NOTE: This matches the mutexgear_wheel_release() but operates with wheel_side_index rather than the client_side_index
	int ret;

	bool fault = false;

	if (__wheel->wheel_side_index == MUTEXGEAR_WHEELELEMENT_INVALID)
	{
		// Client has not attached
		ret = EPERM;
		fault = true;
	}
	else if ((unsigned int)__wheel->wheel_side_index >= (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
	{
		// The object is in an invalid state
		ret = EINVAL;
		fault = true;
	}
	else
	{
		// OK to proceed
		if ((ret = _mutexgear_lock_release(__wheel->muteces + __wheel->wheel_side_index)) == EOK)
		{
			__wheel->wheel_side_index = MUTEXGEAR_WHEELELEMENT_INVALID;
		}
		else
		{
			fault = true;
		}
	}

	return !fault ? EOK : ret;
}


/*static */
int _mutexgear_wheel_gripon(mutexgear_wheel_t *__wheel)
{
	int ret;

	bool fault = false;

	if ((unsigned int)DECODE_WHEEL_PUSHON_INDEX(__wheel->client_side_index) < (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
	{
		// Up to MUTEXGEAR_WHEEL_NUMELEMENTS down starting with MUTEXGEAR_WHEELELEMENT_INVALID indicate 
		// that there might be mutexgear_wheel_pushon calls before or there might be none of them. 
		// Both cases are OK.

		int trial_index;
		// Get the last pushon index (if any) to start seeking with
		for (trial_index = DECODE_WHEEL_PUSHON_INDEX(__wheel->client_side_index); ; )
		{
			// Do trials in reverse direction to avoid risk of following the other side's locks.
			// Start immediately with a decrement since the last pushon index is likely to still be occupied by the client.
			trial_index = trial_index != 0 ? trial_index - 1 : MUTEXGEAR_WHEEL_NUMELEMENTS - 1;

			if ((ret = _mutexgear_lock_tryacquire(__wheel->muteces + trial_index)) == EOK)
			{
				__wheel->client_side_index = trial_index;
				break;
			}
			else if (ret != EBUSY)
			{
				fault = true;
				break;
			}
		}
	}
	else if ((unsigned int)__wheel->client_side_index >= (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
	{
		// The object is in an invalid state
		ret = EINVAL;
		fault = true;
	}
	else
	{
		// The object had already been gripped on
		ret = EBUSY;
		fault = true;
	}

	return !fault ? EOK : ret;
}

/*static */
int _mutexgear_wheel_turn(mutexgear_wheel_t *__wheel)
{
	// NOTE: This matches the mutexgear_wheel_advanced() but operates with client_side_index rather than the wheel_side_index
	int ret;

	bool fault = false;
	int mutex_unlock_status;

	if ((unsigned int)DECODE_WHEEL_PUSHON_INDEX(__wheel->client_side_index) < (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
	{
		// Up to MUTEXGEAR_WHEEL_NUMELEMENTS down starting with MUTEXGEAR_WHEELELEMENT_INVALID indicate 
		// that there might or might not be mutexgear_wheel_pushon calls but the wheel has not been gripped on
		ret = EPERM;
		fault = true;
	}
	else if ((unsigned int)__wheel->client_side_index >= (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
	{
		// The object is in an invalid state
		ret = EINVAL;
		fault = true;
	}
	else
	{
		// OK to proceed
		int client_side_index = __wheel->client_side_index;
		int next_index = client_side_index != MUTEXGEAR_WHEEL_NUMELEMENTS - 1 ? client_side_index + 1 : 0;

		if ((ret = _mutexgear_lock_acquire(__wheel->muteces + next_index)) == EOK)
		{
			if ((ret = _mutexgear_lock_release(__wheel->muteces + client_side_index)) == EOK)
			{
				__wheel->client_side_index = next_index;
			}
			else
			{
				// Revert the next mutex to the initial state and fault
				MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(__wheel->muteces + next_index)) == EOK); // Should succeed normally

				fault = true;
			}
		}
		else
		{
			fault = true;
		}
	}

	return !fault ? EOK : ret;
}

/*static */
int _mutexgear_wheel_release(mutexgear_wheel_t *__wheel)
{
	// NOTE: This matches the mutexgear_wheel_disengaged() but operates with client_side_index rather than the wheel_side_index
	int ret;

	bool fault = false;

	if ((unsigned int)DECODE_WHEEL_PUSHON_INDEX(__wheel->client_side_index) < (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
	{
		// Up to MUTEXGEAR_WHEEL_NUMELEMENTS down starting with MUTEXGEAR_WHEELELEMENT_INVALID indicate 
		// that there might or might not be mutexgear_wheel_pushon calls but the wheel has not been gripped on
		ret = EPERM;
		fault = true;
	}
	else if ((unsigned int)__wheel->client_side_index >= (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
	{
		// The object is in an invalid state
		ret = EINVAL;
		fault = true;
	}
	else
	{
		// OK to proceed
		int client_side_index = __wheel->client_side_index;

		if ((ret = _mutexgear_lock_release(__wheel->muteces + client_side_index)) == EOK)
		{
			// Store the next index as a pushon index to allow continuing 
			// with the object in pushon mode
			__wheel->client_side_index = client_side_index != MUTEXGEAR_WHEEL_NUMELEMENTS - 1
				? ENCODE_WHEEL_PUSHON_INDEX(client_side_index + 1) : ENCODE_WHEEL_PUSHON_INDEX(0);
		}
		else
		{
			fault = true;
		}
	}

	return !fault ? EOK : ret;
}


#endif // #ifndef __MUTEXGEAR_MG_WHEEL_H_INCLUDED
