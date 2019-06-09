/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear Wheel API Implementation                                   */
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
 *	\brief MutexGear Wheel API Implementation
 *
 *	NOTE:
 *
 *	The "Wheel" set of functions defined in this file
 *	implements a synchronization mechanism being a subject
 *	of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and Image Database search
 *	(currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm) to view the patent text.
 */




#include <mutexgear/wheel.h>
#include "utility.h"


//////////////////////////////////////////////////////////////////////////

/*extern */
int mutexgear_wheelattr_init(mutexgear_wheelattr_t *__attr)
{
	int ret = _mutexgear_lockattr_init(&__attr->mutexattr);
	return ret;
}

/*extern */
int mutexgear_wheelattr_destroy(mutexgear_wheelattr_t *__attr)
{
	int ret = _mutexgear_lockattr_destroy(&__attr->mutexattr);
	return ret;
}


/*extern */
int mutexgear_wheelattr_getpshared(const mutexgear_wheelattr_t *__attr, int *__out_pshared)
{
	int ret = _mutexgear_lockattr_getpshared(&__attr->mutexattr, __out_pshared);
	return ret;
}

/*extern */
int mutexgear_wheelattr_setpshared(mutexgear_wheelattr_t *__attr, int __pshared)
{
	int ret = _mutexgear_lockattr_setpshared(&__attr->mutexattr, __pshared);
	return ret;
}


/*extern */
int mutexgear_wheelattr_getprioceiling(const mutexgear_wheelattr_t *__attr, int *__out_prioceiling)
{
	int ret = _mutexgear_lockattr_getprioceiling(&__attr->mutexattr, __out_prioceiling);
	return ret;
}

/*extern */
int mutexgear_wheelattr_getprotocol(const mutexgear_wheelattr_t *__attr, int *__out_protocol)
{
	int ret = _mutexgear_lockattr_getprotocol(&__attr->mutexattr, __out_protocol);
	return ret;
}

/*extern */
int mutexgear_wheelattr_setprioceiling(mutexgear_wheelattr_t *__attr, int __prioceiling)
{
	int ret = _mutexgear_lockattr_setprioceiling(&__attr->mutexattr, __prioceiling);
	return ret;
}

/*extern */
int mutexgear_wheelattr_setprotocol(mutexgear_wheelattr_t *__attr, int __protocol)
{
	int ret = _mutexgear_lockattr_setprotocol(&__attr->mutexattr, __protocol);
	return ret;
}

/*extern */
int mutexgear_wheelattr_setmutexattr(mutexgear_wheelattr_t *__attr, const _MUTEXGEAR_LOCKATTR_T *__mutexattr)
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

		if ((ret = _mutexgear_lockattr_getprioceiling(__mutexattr, &prioceiling_value)) != EOK)
		{
			break;
		}

		if ((ret = _mutexgear_lockattr_getprotocol(__mutexattr, &protocol_value)) != EOK)
		{
			break;
		}

		if (!pshared_missing && (ret = _mutexgear_lockattr_setpshared(&__attr->mutexattr, pshared_value)) != EOK)
		{
			break;
		}

		if ((ret = _mutexgear_lockattr_setprioceiling(&__attr->mutexattr, prioceiling_value)) != EOK)
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

/*extern */
int mutexgear_wheel_init(mutexgear_wheel_t *__wheel, const mutexgear_wheelattr_t *__attr)
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
		__wheel->slave_index = MUTEXGEAR_WHEELELEMENT_INVALID;
		// Store the ..._lockslave operation start index to master 
		// to aid pushon operations acting on the same indices as the slave is going to do.
		__wheel->master_index = ENCODE_WHEEL_PUSHON_INDEX(MUTEXGEAR_WHEELELEMENT_ATTACH_FIRST);
	}

	return !fault ? EOK : ret;
}

/*extern */
int mutexgear_wheel_destroy(mutexgear_wheel_t *__wheel)
{
	int ret;

	bool fault = false;
	unsigned int mutexindex;

	if (__wheel->slave_index == MUTEXGEAR_WHEELELEMENT_DESTROYED)
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
		__wheel->slave_index = MUTEXGEAR_WHEELELEMENT_DESTROYED;
	}

	return !fault ? EOK : ret;
}


/*extern */
int mutexgear_wheel_lockslave(mutexgear_wheel_t *__wheel)
{
	int ret;

	bool fault = false;

	if (__wheel->slave_index == MUTEXGEAR_WHEELELEMENT_INVALID)
	{
		// The wheel has been initialized or a slave has been detached - OK to proceed

		int first_index = MUTEXGEAR_WHEELELEMENT_ATTACH_FIRST;
		// NOTE! _mutexgear_lock_tryacquire() is used instead of _mutexgear_lock_acquire() here!
		// External logic must guarantee the wheel is free for driven-attaching at the start position
		if ((ret = _mutexgear_lock_tryacquire(__wheel->muteces + first_index)) == EOK)
		{
			__wheel->slave_index = first_index;
		}
		else
		{
			fault = true;
		}
	}
	else if ((unsigned int)__wheel->slave_index >= (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
	{
		// The object is in an invalid state
		ret = EINVAL;
		fault = true;
	}
	else
	{
		// A slave had already been attached
		ret = EBUSY;
		fault = true;
	}

	return !fault ? EOK : ret;
}

/*extern */
int mutexgear_wheel_slaveroll(mutexgear_wheel_t *__wheel)
{
	// NOTE: This matches the mutexgear_wheel_turn() but operates with slave_index rather than the master_index
	int ret;

	bool fault = false;
	int mutex_unlock_status;

	if (__wheel->slave_index == MUTEXGEAR_WHEELELEMENT_INVALID)
	{
		// Slave has not been attached
		ret = EPERM;
		fault = true;
	}
	else if ((unsigned int)__wheel->slave_index >= (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
	{
		// The object is in an invalid state
		ret = EINVAL;
		fault = true;
	}
	else
	{
		// OK to proceed
		int slave_index = __wheel->slave_index;
		int next_index = slave_index != MUTEXGEAR_WHEEL_NUMELEMENTS - 1 ? slave_index + 1 : 0;

		if ((ret = _mutexgear_lock_tryacquire(__wheel->muteces + next_index)) == EOK)
		{
			if ((ret = _mutexgear_lock_release(__wheel->muteces + slave_index)) == EOK)
			{
				__wheel->slave_index = next_index;
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
			// and will be able to still freely check its predicate. Rolling ahead can therefore be skipped.
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
//		int slave_index = __wheel->slave_index;
//		int next_index = slave_index != MUTEXGEAR_WHEEL_NUMELEMENTS - 1 ? slave_index + 1 : 0;
//
//		if ((ret = _mutexgear_lock_acquire(__wheel->muteces + next_index)) == EOK
//			&& (ret = _mutexgear_lock_release(__wheel->muteces + slave_index)) == EOK)
//		{
//			__wheel->slave_index = next_index;
//		}
//		else
//		{
//			fault = 1;
//		}
//	}
//
	return !fault ? EOK : ret;
}

/*extern */
int mutexgear_wheel_unlockslave(mutexgear_wheel_t *__wheel)
{
	// NOTE: This matches the mutexgear_wheel_release() but operates with slave_index rather than the master_index
	int ret;

	bool fault = false;

	if (__wheel->slave_index == MUTEXGEAR_WHEELELEMENT_INVALID)
	{
		// Slave has not been attached
		ret = EPERM;
		fault = true;
	}
	else if ((unsigned int)__wheel->slave_index >= (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
	{
		// The object is in an invalid state
		ret = EINVAL;
		fault = true;
	}
	else
	{
		// OK to proceed
		if ((ret = _mutexgear_lock_release(__wheel->muteces + __wheel->slave_index)) == EOK)
		{
			__wheel->slave_index = MUTEXGEAR_WHEELELEMENT_INVALID;
		}
		else
		{
			fault = true;
		}
	}

	return !fault ? EOK : ret;
}


/*extern */
int mutexgear_wheel_pushon(mutexgear_wheel_t *__wheel)
{
	// NOTE: Pushons start with the same index as the mutexgear_wheel_lockslave() does, to come in agreement with it
	int ret;

	bool fault = false;
	int mutex_unlock_status;

	if ((unsigned int)DECODE_WHEEL_PUSHON_INDEX(__wheel->master_index) < (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
	{
		// OK to proceed
		int pushon_index = DECODE_WHEEL_PUSHON_INDEX(__wheel->master_index);

		if ((ret = _mutexgear_lock_acquire(__wheel->muteces + pushon_index)) == EOK)
		{
			MG_CHECK(mutex_unlock_status, (mutex_unlock_status = _mutexgear_lock_release(__wheel->muteces + pushon_index)) == EOK); // Should succeed normally

			__wheel->master_index = pushon_index != MUTEXGEAR_WHEEL_NUMELEMENTS - 1
				? ENCODE_WHEEL_PUSHON_INDEX(pushon_index + 1) : ENCODE_WHEEL_PUSHON_INDEX(0);
		}
		else
		{
			fault = true;
		}
	}
	else if ((unsigned int)__wheel->master_index >= (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
	{
		// The object is in an invalid state
		ret = EINVAL;
		fault = true;
	}
	else
	{
		// The object had been gripped on and is not available for pushing unless is released
		ret = EBUSY;
		fault = true;
	}

	return !fault ? EOK : ret;
}


/*extern */
int mutexgear_wheel_gripon(mutexgear_wheel_t *__wheel)
{
	int ret;

	bool fault = false;

	if ((unsigned int)DECODE_WHEEL_PUSHON_INDEX(__wheel->master_index) < (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
	{
		// Up to MUTEXGEAR_WHEEL_NUMELEMENTS down starting with MUTEXGEAR_WHEELELEMENT_INVALID indicate 
		// that there might be mutexgear_wheel_pushon calls before or there might be none of them. 
		// Both cases are OK.

		int trial_index;
		// Get the last pushon index (if any) to start seeking with
		for (trial_index = DECODE_WHEEL_PUSHON_INDEX(__wheel->master_index); ; )
		{
			// Do trials in reverse direction to avoid risk of following the other side's locks.
			// Start immediately with a decrement since the last pushon index is likely to be still occupied by the slave.
			trial_index = trial_index != 0 ? trial_index - 1 : MUTEXGEAR_WHEEL_NUMELEMENTS - 1;

			if ((ret = _mutexgear_lock_tryacquire(__wheel->muteces + trial_index)) == EOK)
			{
				__wheel->master_index = trial_index;
				break;
			}
			else if (ret != EBUSY)
			{
				fault = true;
				break;
			}
		}
	}
	else if ((unsigned int)__wheel->master_index >= (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
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

/*extern */
int mutexgear_wheel_turn(mutexgear_wheel_t *__wheel)
{
	// NOTE: This matches the mutexgear_wheel_slaveroll() but operates with master_index rather than the slave_index
	int ret;

	bool fault = false;
	int mutex_unlock_status;

	if ((unsigned int)DECODE_WHEEL_PUSHON_INDEX(__wheel->master_index) < (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
	{
		// Up to MUTEXGEAR_WHEEL_NUMELEMENTS down starting with MUTEXGEAR_WHEELELEMENT_INVALID indicate 
		// that there might or might not be mutexgear_wheel_pushon calls but the wheel has not been gripped on
		ret = EPERM;
		fault = true;
	}
	else if ((unsigned int)__wheel->master_index >= (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
	{
		// The object is in an invalid state
		ret = EINVAL;
		fault = true;
	}
	else
	{
		// OK to proceed
		int master_index = __wheel->master_index;
		int next_index = master_index != MUTEXGEAR_WHEEL_NUMELEMENTS - 1 ? master_index + 1 : EOK;

		if ((ret = _mutexgear_lock_acquire(__wheel->muteces + next_index)) == EOK)
		{
			if ((ret = _mutexgear_lock_release(__wheel->muteces + master_index)) == EOK)
			{
				__wheel->master_index = next_index;
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

/*extern */
int mutexgear_wheel_release(mutexgear_wheel_t *__wheel)
{
	// NOTE: This matches the mutexgear_wheel_unlockslave() but operates with master_index rather than the slave_index
	int ret;

	bool fault = false;

	if ((unsigned int)DECODE_WHEEL_PUSHON_INDEX(__wheel->master_index) < (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
	{
		// Up to MUTEXGEAR_WHEEL_NUMELEMENTS down starting with MUTEXGEAR_WHEELELEMENT_INVALID indicate 
		// that there might or might not be mutexgear_wheel_pushon calls but the wheel has not been gripped on
		ret = EPERM;
		fault = true;
	}
	else if ((unsigned int)__wheel->master_index >= (unsigned int)MUTEXGEAR_WHEEL_NUMELEMENTS)
	{
		// The object is in an invalid state
		ret = EINVAL;
		fault = true;
	}
	else
	{
		// OK to proceed
		int master_index = __wheel->master_index;

		if ((ret = _mutexgear_lock_release(__wheel->muteces + master_index)) == EOK)
		{
			// Store the next index as a pushon index to allow continuing 
			// with the object in pushon mode
			__wheel->master_index = master_index != MUTEXGEAR_WHEEL_NUMELEMENTS - 1 
				? ENCODE_WHEEL_PUSHON_INDEX(master_index + 1) : ENCODE_WHEEL_PUSHON_INDEX(0);
		}
		else
		{
			fault = true;
		}
	}

	return !fault ? EOK : ret;
}


/*
 *	NOTE:
 *	The "Wheel" set of functions defined in this file
 *	implements a synchronization mechanism being a subject
 *	of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and Image Database search
 *	(currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm) to view the patent text.
 */
