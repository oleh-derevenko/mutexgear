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
/* Copyright (c) 2016-2025 Oleh Derevenko. All rights are reserved.     */
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


/**
 *	\class mutex_wheel
 *	\brief A wrapper for \c mutexgear_wheel_t and its related functions.
 *
 *	The class implements a mutex wheel with default initialization attributes.
 *
 *	\see mutexgear_wheel_t
 */
class mutex_wheel
{
public:
	typedef mutexgear_wheel_t *native_handle_type;

	mutex_wheel()
	{
		int iInitializationResult = mutexgear_wheel_init(&m_wWheelInstance, nullptr);

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
	// Wheel Side

	void engaged()
	{
		int iEngageResult = mutexgear_wheel_engaged(&m_wWheelInstance);

		if (iEngageResult != EOK)
		{
			throw std::system_error(std::error_code(iEngageResult, std::system_category()));
		}
	}

	void advanced()
	{
		int iAdvanceResult = mutexgear_wheel_advanced(&m_wWheelInstance);

		if (iAdvanceResult != EOK)
		{
			throw std::system_error(std::error_code(iAdvanceResult, std::system_category()));
		}
	}

	void disengaged() noexcept
	{
		int iMutexWheelDisengageResult;
		MG_CHECK(iMutexWheelDisengageResult, (iMutexWheelDisengageResult = mutexgear_wheel_disengaged(&m_wWheelInstance)) == EOK);
	}


	//////////////////////////////////////////////////////////////////////////
	// Client Side

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
