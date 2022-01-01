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


/**
 *	\class mutex_toggle
 *	\brief A wrapper for \c mutexgear_toggle_t and its related functions.
 *
 *	The class implements a mutex toggle with default initialization attributes.
 *
 *	\see mutexgear_toggle_t
 */
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
	// Wheel Side

	void engaged()
	{
		int iEngageResult = mutexgear_toggle_engaged(&m_tToggleInstance);

		if (iEngageResult != EOK)
		{
			throw std::system_error(std::error_code(iEngageResult, std::system_category()));
		}
	}

	void flipped()
	{
		int iFlipResult = mutexgear_toggle_flipped(&m_tToggleInstance);

		if (iFlipResult != EOK)
		{
			throw std::system_error(std::error_code(iFlipResult, std::system_category()));
		}
	}

	void disengaged() noexcept
	{
		int iMutexToggleDisengageResult;
		MG_CHECK(iMutexToggleDisengageResult, (iMutexToggleDisengageResult = mutexgear_toggle_disengaged(&m_tToggleInstance)) == EOK);
	}


	//////////////////////////////////////////////////////////////////////////
	// Client Side

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
