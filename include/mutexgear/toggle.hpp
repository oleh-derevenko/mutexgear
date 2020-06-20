#ifndef __MUTEXGEAR_TOGGLE_HPP_INCLUDED
#define __MUTEXGEAR_TOGGLE_HPP_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear mutex_toggle Class Definition                              */
/*                                                                      */
/* WARNING!                                                             */
/* This library contains a synchronization technique protected by       */
/* the U.S. Patent 9,983,913.                                           */
/*                                                                      */
/* THIS IS A PRE-RELEASE LIBRARY SNAPSHOT FOR EVALUATION PURPOSES ONLY. */
/* AWAIT THE RELEASE AT https://mutexgear.com                           */
/*                                                                      */
/* Copyright (c) 2016-2020 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/

/**
 *	\file
 *	\brief MutexGear \c mutex_toggle class definition
 *
 *	The header defines \c mutex_toggle class which is a header-only wrapper
 *  for \c mutexgear_toggle_t object.
 *
 *	NOTE:
 *
 *	The \c mutexgear_toggle_t object implements a synchronization mechanism being a subject
 *	of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and Image Database search
 *	(currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm) to view the patent text.
 */


#include <mutexgear/toggle.h>
#include <system_error>


_MUTEXGEAR_BEGIN_NAMESPACE()


class mutex_toggle
{
public:
	typedef mutexgear_toggle_t *native_handle_type;

	mutex_toggle()
	{
		int iInitializationResult = mutexgear_toggle_init(&m_tToggleInstance, NULL);

		if (iInitializationResult != EOK)
		{
			throw std::system_error(std::error_code(iInitializationResult, std::system_category()));
		}
	}

	mutex_toggle(const mutex_toggle &mtAnotherInstance) = delete;

	~mutex_toggle() noexcept
	{
		int iMutexToggleDestructionResult;
		MG_CHECK(iMutexToggleDestructionResult, (iMutexToggleDestructionResult = mutexgear_toggle_destroy(&m_tToggleInstance)) == EOK);
	}

	mutex_toggle &operator =(const mutex_toggle &mtAnotherInstance) = delete;

public:
	//////////////////////////////////////////////////////////////////////////
	// Slave Side

	void lock_slave()
	{
		int iLockResult = mutexgear_toggle_lockslave(&m_tToggleInstance);

		if (iLockResult != EOK)
		{
			throw std::system_error(std::error_code(iLockResult, std::system_category()));
		}
	}

	void slaveswitch()
	{
		int iSwitchResult = mutexgear_toggle_slaveswitch(&m_tToggleInstance);

		if (iSwitchResult != EOK)
		{
			throw std::system_error(std::error_code(iSwitchResult, std::system_category()));
		}
	}

	void unlock_slave() noexcept
	{
		int iMutexToggleUnlockSlaveResult;
		MG_CHECK(iMutexToggleUnlockSlaveResult, (iMutexToggleUnlockSlaveResult = mutexgear_toggle_unlockslave(&m_tToggleInstance)) == EOK);
	}


	//////////////////////////////////////////////////////////////////////////
	// Master Side

	void push_on()
	{
		int iPushOnResult = mutexgear_toggle_pushon(&m_tToggleInstance);

		if (iPushOnResult != EOK)
		{
			throw std::system_error(std::error_code(iPushOnResult, std::system_category()));
		}
	}

public:
	native_handle_type native_handle() const noexcept { return static_cast<native_handle_type>(const_cast<mutexgear_toggle_t *>(&m_tToggleInstance)); }

private:
	mutexgear_toggle_t		m_tToggleInstance;
};


_MUTEXGEAR_END_NAMESPACE();


#endif // #ifndef __MUTEXGEAR_TOGGLE_HPP_INCLUDED
