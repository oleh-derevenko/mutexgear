#ifndef __MUTEXGEAR__MTX_HELPERS_HPP_INCLUDED
#define __MUTEXGEAR__MTX_HELPERS_HPP_INCLUDED


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
/* Copyright (c) 2016-2025 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/

/**
*	\file
*	\brief Muteces helper namespace
*
*	The header defines helper classes to be used with the library's public
*	mutex classes.
*/


#include <mutexgear/completion.hpp>


_MUTEXGEAR_BEGIN_NAMESPACE()

_MUTEXGEAR_BEGIN_MTX_HELPERS_NAMESPACE()

using _MUTEXGEAR_COMPLETION_NAMESPACE::worker;
using _MUTEXGEAR_COMPLETION_NAMESPACE::waiter;
using _MUTEXGEAR_COMPLETION_NAMESPACE::item;


class bourgeois
{
public:
	bourgeois() = default;

	bourgeois(const bourgeois &bAnotherBourgeois) = delete;

	~bourgeois() noexcept = default;

	bourgeois &operator =(const bourgeois &bAnotherBourgeois) = delete;

public:
	void lock()
	{
		m_wWorkerInstance.lock();
	}

	void unlock() noexcept
	{
		m_wWorkerInstance.unlock();
	}

public:
	operator worker::pointer() noexcept { return static_cast<worker::pointer>(m_wWorkerInstance); }
	operator item::pointer() noexcept { return static_cast<item::pointer>(m_iItemInstance); }

private:
	worker			m_wWorkerInstance;
	item			m_iItemInstance;
};


_MUTEXGEAR_END_MTX_HELPERS_NAMESPACE();


_MUTEXGEAR_END_NAMESPACE();


#endif // #ifndef __MUTEXGEAR__MTX_HELPERS_HPP_INCLUDED

