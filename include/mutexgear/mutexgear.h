#ifndef __MUTEXGEAR_MUTEXGEAR_H_INCLUDED
#define __MUTEXGEAR_MUTEXGEAR_H_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear API Definitions                                            */
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
 *	\brief MutexGear API definitions
 * 
 *	The header includes all the library exported APIs.
 */


#include <mutexgear/rwlock.h>
#include <mutexgear/completion.h>
#include <mutexgear/wheel.h>
#include <mutexgear/toggle.h>
// Note: lists are not included here


/*
 *	Some usage examples:
 *	
 *	1. Scheduling a task in a worker thread an waiting for it to complete
 *	(a coordinated execution mode example)
 *  
 *	\code
 *	
 *	typedef struct worker_context
 *	{
 *		mutexgear_toggle_t		m_completion_toggle;
 *		// ...
 *
 *	} worker_context_t;
 *	
 *	typedef struct work_item
 *	{
 *		//...
 *
 *	} work_item_t;
 *
 *	
 *	// Notify the upper logical level that the worker has finished initializing and is ready.
 *	extern void NotifyHostWorkerThreadIsReady(worker_context_t *context);
 *	// Test if the worker has already notified the upper logical level about being ready.
 *	extern int IsHostNotifiedOfWorkerThreadBeingReady(worker_context_t *worker);
 *	// Assign the item for execution and wake the worker up.
 *	extern void AssignAWorkItem(worker_context_t *context, work_item_t *item);
 *	// Extract next work item to be processed or block the caller if there are none.
 *	extern work_item_t *ExtractPendingWorkItem(worker_context_t *context);
 *	// Perform work item intended operations and mark it as completed.
 *	// Return exit request status as implied by the work item.
 *	extern void HandleWorkItem(work_item_t *item, int *out_exit_requested);
 *	// Test if the item has already been marked as completed by a worker.
 *	extern int IsWorkItemMarkedCompleted(work_item_t *item);
 *
 *
 *	static
 *	void ClientThread_ScheduleWorkItem(worker_context_t *worker, work_item_t *item)
 *	{
 *		// The function can only be called after the worker initialization phase ends
 *		assert(IsHostNotifiedOfWorkerThreadBeingReady(worker));
 *
 *		// Assign a work item and wake the worker up.
 *		AssignAWorkItem(worker, item);
 *	}
 *	
 *	static
 *	void WorkerThread_ThreadRoutine(worker_context_t *context)
 *	{
 *		// Initially, make the toggle engaged
 *		mutexgear_toggle_engaged(&context->m_completion_toggle);
 *
 *		// Let the upper level know the worked has been started
 *		// and is ready to serve requests (the toggle has been attached)
 *		NotifyHostWorkerThreadIsReady(context);
 *
 *		int exit_requested;
 *		for (exit_requested = 0; !exit_requested; )
 *		{
 *			// Extract a next item to handle. Block if there are no items queued.
 *			work_item_t *item = ExtractPendingWorkItem(context);
 *			// Handle the work item extracted
 *			HandleWorkItem(item, &exit_requested); // This is to mark the work item completed
 *
 *			// Flip the toggle each time after an item has been handled
 *			mutexgear_toggle_flipped(&context->m_completion_toggle);
 *		}
 *
 *		// Finally, make the toggle disengaged on exit
 *		mutexgear_toggle_disengaged(&context->m_completion_toggle);
 *	}
 *
 *	static
 *	void ClientThread_WaitWorkItemCompletion(worker_context_t *worker, work_item_t *item)
 *	{
 *		// Just push the toggle on...
 *		mutexgear_toggle_pushon(&worker->m_completion_toggle);
 *
 *		// ...and assert that the work item has indeed completed
 *		assert(IsWorkItemMarkedCompleted(item));
 *	}
 *
 *	\endcode
 *
 *
 *	2. Waiting for room in a limited size work queue to push work item into
 *
 *	\code
 *	
 *	typedef struct worker_context
 *	{
 *		mutexgear_wheel_t		m_item_extraction_wheel;
 *		// ...
 *
 *	} worker_context_t;
 *	
 *	typedef struct work_item
 *	{
 *		// ...
 *
 *	} work_item_t;
 *
 *	// Notify the upper logical level that the worker has finished initializing and is ready
 *	extern void NotifyHostWorkerThreadIsReady(worker_context_t *context);
 *	// Test if the worker has already notified the upper logical level about being ready
 *	extern int IsHostNotifiedOfWorkerThreadBeingReady(worker_context_t *worker);
 *	// Insert the item into the work queue and wake the worker up if the queue was empty.
 *	extern int QueueAWorkItem(worker_context_t *context, work_item_t *item);
 *	// Extract next work item to be processed or block the caller if there are none.
 *	extern work_item_t *ExtractPendingWorkItem(worker_context_t *context);
 *	// Perform work item intended operations and release resources allocated to the item.
 *	// Return exit request status as implied by the work item.
 *	extern void HandleAndDeleteWorkItem(work_item_t *item, int *out_exit_requested);
 *
 *	static
 *	void WorkerThread_ThreadRoutine(worker_context_t *context)
 *	{
 *		// Initially, make the wheel engaged
 *		mutexgear_wheel_engaged(&context->m_item_extraction_wheel);
 *
 *		// Let the upper level know the worked has been started
 *		// and is ready to serve requests (the toggle has been attached)
 *		NotifyHostWorkerThreadIsReady(context);
 *
 *		int exit_requested;
 *		for (exit_requested = 0; !exit_requested; )
 *		{
 *			// Extract a next item to handle. Block if there are no items queued.
 *			work_item_t *item = ExtractPendingWorkItem(context);
 *			// Advance the wheel to let the other side know there is some room in the queue
 *			mutexgear_wheel_advanced(&context->m_item_extraction_wheel);
 *
 *			// Handle the work item extracted
 *			HandleAndDeleteWorkItem(item, &exit_requested);
 *		}
 *
 *		// Finally, make the wheel disengaged on exit
 *		mutexgear_wheel_disengaged(&context->m_item_extraction_wheel);
 *	}
 *
 *	static
 *	void ClientThread_ScheduleWorkItem(worker_context_t *worker, work_item_t *item)
 *	{
 *		// The function can only be called after the worker initialization phase ends
 *		assert(IsHostNotifiedOfWorkerThreadBeingReady(worker));
 *
 *		// Push the item into the work queue. Wake the worker up if the queue was empty.
 *		// False return indicates there is no room for the item.
 *		if (!QueueAWorkItem(worker, item))
 *		{
 *			// Grip on the wheel
 *			mutexgear_wheel_gripon(&worker->m_item_extraction_wheel);
 *
 *			// Retry queuing attempts...
 *			while (!QueueAWorkItem(worker, item))
 *			{
 *				// ... and turn the wheel until the queuing succeeds
 *				mutexgear_wheel_turn(&worker->m_item_extraction_wheel);
 *			}
 *
 *			// Finally, release the wheel
 *			mutexgear_wheel_release(&worker->m_item_extraction_wheel);
 *		}
 *	}
 *	
 *	\endcode
 *
 *
 *	3. Canceling all work items scheduled into a thread pool by relation to a particular object.
 *	
 *	\code
 *	
 *	typedef struct pool_context
 *	{
 *		// ...
 *
 *	} pool_context_t;
 *
 *	typedef struct worker_context
 *	{
 *		pool_context_t		*m_pool;
 *		mutexgear_wheel_t		m_item_completion_wheel;
 *		// ...
 *
 *	} worker_context_t;
 *
 *	typedef struct work_item
 *	{
 *		void				*volatile m_worker_wheel_ptr; // To be initialized with NULL
 *		// ...
 *	
 *	} work_item_t;
 *
 *	typedef struct related_object
 *	{
 *		// ...
 *
 *	} related_object_t;
 *
 *	// Atomically swap the value with the destination content and return the latter.
 *	extern void *AtomicSwapPointer(void *volatile *destination_ptr, void *value);
 *	// Establish a relation connection between the object and the item (e.g. link the item in an object-local list).
 *	extern void LinkItemIntoRelatedObjectList(related_object_t *relation, work_item_t *item);
 *	// Break the relation connection of the item (e.g. by unlinking the item from its current object-local list).
 *	// The return status is "false" if the relation has already been broken by other party.
 *	extern int UnlinkItemFromRelatedObjectList(work_item_t *item);
 *	// Remove and return next item from the object-local relation list. The boolean result indicates item availability.
 *	extern int UnlinkRelatedObjectNextWorkItem(related_object_t *relation, work_item_t **out_item);
 *	// Insert the item into the pool-local work queue and wake up a worker if necessary.
 *	extern void QueueAWorkItem(pool_context_t *pool, work_item_t *item);
 *	// Extract next work item to be processed or block the caller if there are none.
 *	extern work_item_t *ExtractPendingWorkItem(pool_context_t *pool);
 *	// Remove the given item from pool execution queue. The return status is "false"
 *	// if the item has already been removed by a worker.
 *	extern int RemoveItemFromPoolWorkQueue(pool_context_t *pool, work_item_t *item);
 *	// Perform work item intended operations and mark it as completed.
 *	// Return exit request status as implied by the work item.
 *	extern void HandleWorkItem(work_item_t *item, int *out_exit_requested);
 *	// Test if the item has already been marked as completed by a worker.
 *	extern int IsWorkItemMarkedCompleted(work_item_t *item);
 *	// Release any resources allocated to the item
 *	extern void DeleteWorkItem(work_item_t *item);
 *
 *	static
 *	void ClientThread_ScheduleWorkItem(pool_context_t *pool, related_object_t *relation, work_item_t *item)
 *	{
 *		// Establish item to object relation.
 *		LinkItemIntoRelatedObjectList(relation, item);
 *		// Push the item into the pool queue. Wake a new worker up if necessary.
 *		QueueAWorkItem(pool, item);
 *	}
 *
 *
 *	static
 *	void WorkerThread_ThreadRoutine(worker_context_t *context)
 *	{
 *		// Initially, make the wheel engaged
 *		mutexgear_wheel_engaged(&context->m_item_completion_wheel);
 *
 *		pool_context_t *pool = context->m_pool;
 *
 *		int exit_requested;
 *		for (exit_requested = 0; !exit_requested; )
 *		{
 *			// Extract a next item to handle. Block if there are no items queued.
 *			work_item_t *item = ExtractPendingWorkItem(pool);
 *
 *			// Assign own completion wheel pointer to the item to indicate the item is being processed.
 *			// If the pointer value was already not null, it is an indication that
 *			// the work item is being canceled and no processing is necessary for it.
 *			if (AtomicSwapPointer(&item->m_worker_wheel_ptr, &context->m_item_completion_wheel) == NULL)
 *			{
 *				// Handle the work item extracted. The item pointer is to remain valid.
 *				HandleWorkItem(item, &exit_requested); // This is to mark the work item completed, if necessary
 *
 *				// Break the item item-object relation. Failure to do it is again an indication
 *				// that the item is being canceled and is going to be deleted by the canceling thread.
 *				int unlinked = UnlinkItemFromRelatedObjectList(item);
 *
 *				// Advance the wheel to let the other party know that the item processing is completed.
 *				mutexgear_wheel_advanced(&context->m_item_completion_wheel);
 *
 *				// If this thread was to break the item-object relation...
 *				if (unlinked)
 *				{
 *					// ... it is responsible for deleting the item.
 *					DeleteWorkItem(item);
 *				}
 *			}
 *		}
 *
 *		// Finally, make the wheel disengaged on exit
 *		mutexgear_wheel_disengaged(&context->m_item_completion_wheel);
 *	}
 *
 *	
 *	static
 *	void ClientThread_CancelWorkItemsByRelation(pool_context_t *pool, related_object_t *relation)
 *	{
 *		work_item_t *item;
 *		// Iterate removing the relation items one by one...
 *		while (UnlinkRelatedObjectNextWorkItem(relation, &item))
 *		{
 *			// If removing the item from the pool work queue succeeds it means
 *			// that the work on the item has not been started yet and it can be safely deleted.
 *			if (!RemoveItemFromPoolWorkQueue(pool, item))
 *			{
 *				// Otherwise, swap a not-NULL value (e.g. the item pointer itself) with the wheel pointer content.
 *				// This is to indicate to any workers that the item is being canceled and does not need to be handled
 *				// and, at the same time, this is to extract the item completion wheel pointer
 *				// that might have been already assigned by a worker there.
 *				mutexgear_wheel_t *assigned_completion_wheel = (mutexgear_wheel_t *)AtomicSwapPointer(&item->m_worker_wheel_ptr, item);
 *
 *				// If the current thread was the first to assign the value, it is safe to proceed with deletion immediately
 *				if (assigned_completion_wheel != NULL
 *					// Otherwise it may be necessary to wait until the item's handling is over.
 *					&& !IsWorkItemMarkedCompleted(item))
 *				{
 *					// Grip on the assigned wheel
 *					mutexgear_wheel_gripon(assigned_completion_wheel);
 *	
 *					// Retry item completion tests...
 *					while (!IsWorkItemMarkedCompleted(item))
 *					{
 *						// ... and turn the wheel until the handling is over
 *						mutexgear_wheel_turn(assigned_completion_wheel);
 *					}
 *	
 *					// Finally release the wheel
 *					mutexgear_wheel_release(assigned_completion_wheel);
 *				}
 *			}
 *
 *			// Release resources allocated to the item
 *			DeleteWorkItem(item);
 *		}
 *	}
 *
 *	\endcode
 */


#endif // #ifndef __MUTEXGEAR_MUTEXGEAR_H_INCLUDED
