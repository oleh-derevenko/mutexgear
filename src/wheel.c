/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear Wheel API Implementation                                   */
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
 *	\brief MutexGear Wheel API implementation
 *
 *	NOTE:
 *
 *	The "Wheel" set of functions defined in this file
 *	implements a synchronization mechanism being a subject
 *	of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and Image Database search
 *	(currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm) to view the patent text.
 */


#include "wheel.h"


static int _mutexgear_wheel_pushon(mutexgear_wheel_t *__wheel);


/*extern */
int _mutexgear_wheel_pushon(mutexgear_wheel_t *__wheel)
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


//////////////////////////////////////////////////////////////////////////
// Wheel Public APIs Implementation

/*_MUTEXGEAR_API */
int mutexgear_wheelattr_init(mutexgear_wheelattr_t *__attr)
{
	return _mutexgear_wheelattr_init(__attr);
}

/*_MUTEXGEAR_API */
int mutexgear_wheelattr_destroy(mutexgear_wheelattr_t *__attr)
{
	return _mutexgear_wheelattr_destroy(__attr);
}


/*_MUTEXGEAR_API */
int mutexgear_wheelattr_getpshared(const mutexgear_wheelattr_t *__attr, int *__pshared)
{
	return _mutexgear_wheelattr_getpshared(__attr, __pshared);
}

/*_MUTEXGEAR_API */
int mutexgear_wheelattr_setpshared(mutexgear_wheelattr_t *__attr, int __pshared)
{
	return _mutexgear_wheelattr_setpshared(__attr, __pshared);
}


/*_MUTEXGEAR_API */
int mutexgear_wheelattr_getprioceiling(const mutexgear_wheelattr_t *__attr, int *__prioceiling)
{
	return _mutexgear_wheelattr_getprioceiling(__attr, __prioceiling);
}

/*_MUTEXGEAR_API */
int mutexgear_wheelattr_getprotocol(const mutexgear_wheelattr_t *__attr, int *__protocol)
{
	return _mutexgear_wheelattr_getprotocol(__attr, __protocol);
}

/*_MUTEXGEAR_API */
int mutexgear_wheelattr_setprioceiling(mutexgear_wheelattr_t *__attr, int __prioceiling)
{
	return _mutexgear_wheelattr_setprioceiling(__attr, __prioceiling);
}

/*_MUTEXGEAR_API */
int mutexgear_wheelattr_setprotocol(mutexgear_wheelattr_t *__attr, int __protocol)
{
	return _mutexgear_wheelattr_setprotocol(__attr, __protocol);
}

/*_MUTEXGEAR_API */
int mutexgear_wheelattr_setmutexattr(mutexgear_wheelattr_t *__attr, const _MUTEXGEAR_LOCKATTR_T *__mutexattr)
{
	return _mutexgear_wheelattr_setmutexattr(__attr, __mutexattr);
}


/*_MUTEXGEAR_API */
int mutexgear_wheel_init(mutexgear_wheel_t *__wheel, const mutexgear_wheelattr_t *__attr)
{
	return _mutexgear_wheel_init(__wheel, __attr);
}

/*_MUTEXGEAR_API */
int mutexgear_wheel_destroy(mutexgear_wheel_t *__wheel)
{
	return _mutexgear_wheel_destroy(__wheel);
}


/*_MUTEXGEAR_API */
int mutexgear_wheel_lockslave(mutexgear_wheel_t *__wheel)
{
	return _mutexgear_wheel_lockslave(__wheel);
}

/*_MUTEXGEAR_API */
int mutexgear_wheel_slaveroll(mutexgear_wheel_t *__wheel)
{
	return _mutexgear_wheel_slaveroll(__wheel);
}

/*_MUTEXGEAR_API */
int mutexgear_wheel_unlockslave(mutexgear_wheel_t *__wheel)
{
	return _mutexgear_wheel_unlockslave(__wheel);
}


/*_MUTEXGEAR_API */
int mutexgear_wheel_gripon(mutexgear_wheel_t *__wheel)
{
	return _mutexgear_wheel_gripon(__wheel);
}

/*_MUTEXGEAR_API */
int mutexgear_wheel_turn(mutexgear_wheel_t *__wheel)
{
	return _mutexgear_wheel_turn(__wheel);
}

/*_MUTEXGEAR_API */
int mutexgear_wheel_release(mutexgear_wheel_t *__wheel)
{
	return _mutexgear_wheel_release(__wheel);
}


/*_MUTEXGEAR_API */
int mutexgear_wheel_pushon(mutexgear_wheel_t *__wheel)
{
	return _mutexgear_wheel_pushon(__wheel);
}


/*
 *	NOTE:
 *	The "Wheel" set of functions defined in this file
 *	implements a synchronization mechanism being a subject
 *	of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and Image Database search
 *	(currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm) to view the patent text.
 */
