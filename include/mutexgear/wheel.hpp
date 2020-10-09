#ifndef __MUTEXGEAR_WHEEL_HPP_INCLUDED
#define __MUTEXGEAR_WHEEL_HPP_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear mutex_wheel Class Definition                               */
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
 *	\brief MutexGear \c mutex_wheel class definition
 *
 *	The header defines \c mutex_wheel class which is a header-only wrapper
 *  for \c mutexgear_wheel_t object.
 *
 *	NOTE:
 *
 *	The \c mutexgear_wheel_t object implements a synchronization mechanism being a subject
 *	of the U.S. Patent No. 9983913. Use USPTO Patent Full-Text and Image Database search
 *	(currently, http://patft.uspto.gov/netahtml/PTO/search-adv.htm) to view the patent text.
 */


#include <mutexgear/wheel.h>
#include <system_error>


_MUTEXGEAR_BEGIN_NAMESPACE()


class mutex_wheel
{
public:
	typedef mutexgear_wheel_t *native_handle_type;

	mutex_wheel()
	{
		int iInitializationResult = mutexgear_wheel_init(&m_wWheelInstance, NULL);

		if (iInitializationResult != EOK)
		{
			throw std::system_error(std::error_code(iInitializationResult, std::system_category()));
		}
	}

	mutex_wheel(const mutex_wheel &mwAnotherInstance) = delete;

	~mutex_wheel() noexcept
	{
		int iMutexWheelDestructionResult;
		MG_CHECK(iMutexWheelDestructionResult, (iMutexWheelDestructionResult = mutexgear_wheel_destroy(&m_wWheelInstance)) == EOK);
	}

	mutex_wheel &operator =(const mutex_wheel &mwAnotherInstance) = delete;

public:
	//////////////////////////////////////////////////////////////////////////
	// Slave Side

	void lock_slave()
	{
		int iLockResult = mutexgear_wheel_lockslave(&m_wWheelInstance);

		if (iLockResult != EOK)
		{
			throw std::system_error(std::error_code(iLockResult, std::system_category()));
		}
	}

	void slaveroll()
	{
		int iRollResult = mutexgear_wheel_slaveroll(&m_wWheelInstance);

		if (iRollResult != EOK)
		{
			throw std::system_error(std::error_code(iRollResult, std::system_category()));
		}
	}

	void unlock_slave() noexcept
	{
		int iMutexWheelUnlockSlaveResult;
		MG_CHECK(iMutexWheelUnlockSlaveResult, (iMutexWheelUnlockSlaveResult = mutexgear_wheel_unlockslave(&m_wWheelInstance)) == EOK);
	}


	//////////////////////////////////////////////////////////////////////////
	// Master Side

	void grip_on()
	{
		int iGripOnResult = mutexgear_wheel_gripon(&m_wWheelInstance);

		if (iGripOnResult != EOK)
		{
			throw std::system_error(std::error_code(iGripOnResult, std::system_category()));
		}
	}

	void turn()
	{
		int iTurnResult = mutexgear_wheel_turn(&m_wWheelInstance);

		if (iTurnResult != EOK)
		{
			throw std::system_error(std::error_code(iTurnResult, std::system_category()));
		}
	}

	void release() noexcept
	{
		int iMutexWheelReleaseResult;
		MG_CHECK(iMutexWheelReleaseResult, (iMutexWheelReleaseResult = mutexgear_wheel_release(&m_wWheelInstance)) == EOK);
	}


	//////////////////////////////////////////////////////////////////////////
	// Toggle Simulation

	void push_on()
	{
		int iPushOnResult = mutexgear_wheel_pushon(&m_wWheelInstance);

		if (iPushOnResult != EOK)
		{
			throw std::system_error(std::error_code(iPushOnResult, std::system_category()));
		}
	}

public:
	native_handle_type native_handle() const noexcept { return static_cast<native_handle_type>(const_cast<mutexgear_wheel_t *>(&m_wWheelInstance)); }

private:
	mutexgear_wheel_t		m_wWheelInstance;
};


_MUTEXGEAR_END_NAMESPACE();


#endif // #ifndef __MUTEXGEAR_WHEEL_HPP_INCLUDED
