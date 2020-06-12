#ifndef __MUTEXGEAR_COMPLETION_HPP_INCLUDED
#define __MUTEXGEAR_COMPLETION_HPP_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear Completion Class Definitions                               */
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
 *	\brief MutexGear Completion class set definitions
 *
 *	The header defines a namespace with a set of classes being header-only wrappers
 *  for \c mutexgear_completion_*_t objects.
 *
 */


#include <mutexgear/completion.h>
#include <system_error>


_MUTEXGEAR_BEGIN_NAMESPACE()

_MUTEXGEAR_BEGIN_COMPLETION_NAMESPACE()


class worker
{
public:
	typedef mutexgear_completion_worker_t &reference;

	worker()
	{
		int iInitializationResult = mutexgear_completion_worker_init(&m_cwWorkerInstance, NULL);

		if (iInitializationResult != EOK)
		{
			throw std::system_error(std::error_code(iInitializationResult, std::system_category()));
		}
	}

	worker(const worker &wAnotherWorker) = delete;

	~worker()
	{
		int iCompletionWaiterDestructionResult;
		MG_CHECK(iCompletionWaiterDestructionResult, (iCompletionWaiterDestructionResult = mutexgear_completion_worker_destroy(&m_cwWorkerInstance)) == EOK);
	}

	worker &operator =(const worker &wAnotherWorker) = delete;

public:
	void lock()
	{
		int iLockResult = mutexgear_completion_worker_lock(&m_cwWorkerInstance);

		if (iLockResult != EOK)
		{
			throw std::system_error(std::error_code(iLockResult, std::system_category()));
		}
	}

	void unlock()
	{
		int iCompletionWaiterUnlockResult;
		MG_CHECK(iCompletionWaiterUnlockResult, (iCompletionWaiterUnlockResult = mutexgear_completion_worker_unlock(&m_cwWorkerInstance)) == EOK);
	}

public:
	operator reference() { return m_cwWorkerInstance; }

private:
	mutexgear_completion_worker_t		m_cwWorkerInstance;
};

class waiter
{
public:
	typedef mutexgear_completion_waiter_t &reference;

	waiter()
	{
		int iInitializationResult = mutexgear_completion_waiter_init(&m_cwWaiterInstance, NULL);

		if (iInitializationResult != EOK)
		{
			throw std::system_error(std::error_code(iInitializationResult, std::system_category()));
		}
	}
	
	waiter(const waiter &wAnotherWaiter) = delete; 

	~waiter()
	{
		int iCompletionWaiterDestructionResult;
		MG_CHECK(iCompletionWaiterDestructionResult, (iCompletionWaiterDestructionResult = mutexgear_completion_waiter_destroy(&m_cwWaiterInstance)) == EOK);
	}

	waiter &operator =(const waiter &wAnotherWaiter) = delete;

	operator reference() { return m_cwWaiterInstance; }

private:
	mutexgear_completion_waiter_t		m_cwWaiterInstance;
};

class item
{
public:
	typedef mutexgear_completion_item_t &reference;

	item() { mutexgear_completion_item_init(&m_ciItemInstance); }
	item(const item &iAnotherItem) = delete;

	~item() { mutexgear_completion_item_destroy(&m_ciItemInstance); }

	item &operator =(const item &iAnotherItem) = delete;

	operator reference() { return m_ciItemInstance; }

private:
	mutexgear_completion_item_t			m_ciItemInstance;
};


_MUTEXGEAR_END_COMPLETION_NAMESPACE();

_MUTEXGEAR_END_NAMESPACE();


#endif // #ifndef __MUTEXGEAR_COMPLETION_HPP_INCLUDED
