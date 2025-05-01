/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear Toggle API Implementation                                  */
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
 *	\brief MutexGear Toggle API implementation
 *
 *	NOTE:
 *
 *	The "Toggle" set of functions defined in this file
 *	implements a synchronization mechanism being a subject
 *	of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and Image Database search
 *	(currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm) to view the patent text.
 */


#include <mutexgear/toggle.h>
#include "utility.h"


//////////////////////////////////////////////////////////////////////////

/*extern */
int mutexgear_toggleattr_init(mutexgear_toggleattr_t *__attr)
{
	int ret = _mutexgear_lockattr_init(&__attr->mutexattr);
	return ret;
}

/*extern */
int mutexgear_toggleattr_destroy(mutexgear_toggleattr_t *__attr)
{
	int ret = _mutexgear_lockattr_destroy(&__attr->mutexattr);
	return ret;
}


/*extern */
int mutexgear_toggleattr_getpshared(const mutexgear_toggleattr_t *__attr, int *__out_pshared)
{
	int ret = _mutexgear_lockattr_getpshared(&__attr->mutexattr, __out_pshared);
	return ret;
}

/*extern */
int mutexgear_toggleattr_setpshared(mutexgear_toggleattr_t *__attr, int __pshared)
{
	int ret = _mutexgear_lockattr_setpshared(&__attr->mutexattr, __pshared);
	return ret;
}


/*extern */
int mutexgear_toggleattr_getprioceiling(const mutexgear_toggleattr_t *__attr, int *__out_prioceiling)
{
	int ret = _mutexgear_lockattr_getprioceiling(&__attr->mutexattr, __out_prioceiling);
	return ret;
}

/*extern */
int mutexgear_toggleattr_getprotocol(const mutexgear_toggleattr_t *__attr, int *__out_protocol)
{
	int ret = _mutexgear_lockattr_getprotocol(&__attr->mutexattr, __out_protocol);
	return ret;
}

/*extern */
int mutexgear_toggleattr_setprioceiling(mutexgear_toggleattr_t *__attr, int __prioceiling)
{
	int ret = _mutexgear_lockattr_setprioceiling(&__attr->mutexattr, __prioceiling);
	return ret;
}

/*extern */
int mutexgear_toggleattr_setprotocol(mutexgear_toggleattr_t *__attr, int __protocol)
{
	int ret = _mutexgear_lockattr_setprotocol(&__attr->mutexattr, __protocol);
	return ret;
}

/*extern */
int mutexgear_toggleattr_setmutexattr(mutexgear_toggleattr_t *__attr, const _MUTEXGEAR_LOCKATTR_T *__mutexattr)
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

// Use negative offsets from MUTEXGEAR_TOGGLEELEMENT_INVALID to store pushon indices
#define ENCODE_TOGGLE_PUSHON_INDEX(idx) (MUTEXGEAR_TOGGLEELEMENT_INVALID - (idx))
#define DECODE_TOGGLE_PUSHON_INDEX(val) (MUTEXGEAR_TOGGLEELEMENT_INVALID - (val))

/*extern */
int mutexgear_toggle_init(mutexgear_toggle_t *__toggle, const mutexgear_toggleattr_t *__attr)
{
	int ret, mutex_destroy_status;

	const _MUTEXGEAR_LOCKATTR_T *__mutexattr = __attr ? &__attr->mutexattr : NULL;

	bool fault = false;
	unsigned int mutexindex;
	for (mutexindex = 0; mutexindex != MUTEXGEAR_TOGGLE_NUMELEMENTS; ++mutexindex)
	{
		if ((ret = _mutexgear_lock_init(__toggle->muteces + mutexindex, __mutexattr)) != EOK)
		{
			for (; mutexindex != 0;)
			{
				--mutexindex;
				MG_CHECK(mutex_destroy_status, (mutex_destroy_status = _mutexgear_lock_destroy(__toggle->muteces + mutexindex)) == EOK);
			}

			fault = true;
			break;
		}
	}

	if (!fault)
	{
		__toggle->thumb_position = MUTEXGEAR_TOGGLEELEMENT_INVALID;
		// Store pushon position the attach operation start index 
		// to aid pushon operations acting on the same indices as the switching is going to do.
		__toggle->push_position = ENCODE_TOGGLE_PUSHON_INDEX(MUTEXGEAR_TOGGLEELEMENT_ATTACH_FIRST);
	}

	return !fault ? EOK : ret;
}

/*extern */
int mutexgear_toggle_destroy(mutexgear_toggle_t *__toggle)
{
	int ret;

	bool fault = false;
	unsigned int mutexindex;

	if (__toggle->thumb_position == MUTEXGEAR_TOGGLEELEMENT_DESTROYED)
	{
		// The toggle has already been destroyed
		ret = EINVAL;
		fault = true;
	}

	for (mutexindex = !fault ? MUTEXGEAR_TOGGLE_NUMELEMENTS : 0; mutexindex != 0;)
	{
		--mutexindex;

		if ((ret = _mutexgear_lock_destroy(__toggle->muteces + mutexindex)) != EOK)
		{
			fault = true;
			break;
		}
	}

	if (!fault)
	{
		__toggle->thumb_position = MUTEXGEAR_TOGGLEELEMENT_DESTROYED;
	}

	return !fault ? EOK : ret;
}


/*extern */
int mutexgear_toggle_engaged(mutexgear_toggle_t *__toggle)
{
	int ret;

	bool fault = false;

	if (__toggle->thumb_position == MUTEXGEAR_TOGGLEELEMENT_INVALID)
	{
		// The toggle has been initialized or detached before - OK to proceed

		int first_index = MUTEXGEAR_TOGGLEELEMENT_ATTACH_FIRST;

		// NOTE! _mutexgear_lock_tryacquire() is used instead of _mutexgear_lock_acquire() here!
		// External logic must guarantee the toggle is free for attaching at the start position
		if ((ret = _mutexgear_lock_tryacquire(__toggle->muteces + first_index)) == EOK)
		{
			__toggle->thumb_position = first_index;
		}
		else
		{
			fault = true;
		}
	}
	else if ((unsigned int)__toggle->thumb_position >= (unsigned int)MUTEXGEAR_TOGGLE_NUMELEMENTS)
	{
		// The object is in an invalid state
		ret = EINVAL;
		fault = true;
	}
	else
	{
		// The toggle has already been attached
		ret = EBUSY;
		fault = true;
	}

	return !fault ? EOK : ret;
}

/*extern */
int mutexgear_toggle_flipped(mutexgear_toggle_t *__toggle)
{
	int ret;

	bool fault = false;
	int mutex_unlock_status;

	if (__toggle->thumb_position == MUTEXGEAR_TOGGLEELEMENT_INVALID)
	{
		// The toggle has not been attached
		ret = EPERM;
		fault = true;
	}
	else if ((unsigned int)__toggle->thumb_position >= (unsigned int)MUTEXGEAR_TOGGLE_NUMELEMENTS)
	{
		// The object is in an invalid state
		ret = EINVAL;
		fault = true;
	}
	else
	{
		// OK to proceed
		int thumb_position = __toggle->thumb_position;
		int next_position = thumb_position != MUTEXGEAR_TOGGLE_NUMELEMENTS - 1 ? thumb_position + 1 : 0;

		if ((ret = _mutexgear_lock_acquire(__toggle->muteces + next_position)) == EOK)
		{
			if ((ret = _mutexgear_lock_release(__toggle->muteces + thumb_position)) == EOK)
			{
				__toggle->thumb_position = next_position;
			}
			else
			{
				// Revert the next mutex to the initial state and fault
				MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(__toggle->muteces + next_position)) == EOK); // Should succeed normally

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

/*extern */
int mutexgear_toggle_disengaged(mutexgear_toggle_t *__toggle)
{
	int ret;

	bool fault = false;

	if (__toggle->thumb_position == MUTEXGEAR_TOGGLEELEMENT_INVALID)
	{
		// The toggle has not been attached
		ret = EPERM;
		fault = true;
	}
	else if ((unsigned int)__toggle->thumb_position >= (unsigned int)MUTEXGEAR_TOGGLE_NUMELEMENTS)
	{
		// The object is in an invalid state
		ret = EINVAL;
		fault = true;
	}
	else
	{
		// OK to proceed
		if ((ret = _mutexgear_lock_release(__toggle->muteces + __toggle->thumb_position)) == EOK)
		{
			__toggle->thumb_position = MUTEXGEAR_TOGGLEELEMENT_INVALID;
		}
		else
		{
			fault = true;
		}
	}

	return !fault ? EOK : ret;
}


/*extern */
int mutexgear_toggle_pushon(mutexgear_toggle_t *__toggle)
{
	int ret;

	bool fault = false;
	int mutex_unlock_status;

	if ((unsigned int)DECODE_TOGGLE_PUSHON_INDEX(__toggle->push_position) < (unsigned int)MUTEXGEAR_TOGGLE_NUMELEMENTS)
	{
		// OK to proceed
		int pushon_index = DECODE_TOGGLE_PUSHON_INDEX(__toggle->push_position);

		if ((ret = _mutexgear_lock_acquire(__toggle->muteces + pushon_index)) == EOK)
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(__toggle->muteces + pushon_index)) == EOK); // Should succeed normally

			__toggle->push_position = pushon_index != MUTEXGEAR_TOGGLE_NUMELEMENTS - 1 
				? ENCODE_TOGGLE_PUSHON_INDEX(pushon_index + 1) : ENCODE_TOGGLE_PUSHON_INDEX(0);
		}
		else
		{
			fault = true;
		}
	}
	else
	{
		// The object is in an invalid state
		ret = EINVAL;
		fault = true;
	}

	return !fault ? EOK : ret;
}


/*
 *	NOTE:
 *
 *	The "Toggle" set of functions defined in this file
 *	implements a synchronization mechanism being a subject
 *	of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and Image Database search
 *	(currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm) to view the patent text.
 */
