#ifndef __MUTEXGEAR_CONSTANTS_H_INCLUDED
#define __MUTEXGEAR_CONSTANTS_H_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* Public Constant Definitions of the Library                           */
/*                                                                      */
/* WARNING!                                                             */
/* This library contains a synchronization technique protected by       */
/* the U.S. Patent 9,983,913.                                           */
/*                                                                      */
/* THIS IS A PRE-RELEASE LIBRARY SNAPSHOT.                              */
/* AWAIT THE RELEASE AT https://mutexgear.com                           */
/*                                                                      */
/* Copyright (c) 2016-2021 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/

/**
*	\file
*	\brief MutexGear Library public constant definitions
*
*	The file defines constants for public APIs.
*/


#include <mutexgear/config.h>

// Values for ..._setpshared/..._getpshared calls
/**
 *	\def MUTEXGEAR_PROCESS_PRIVATE
 *	\brief A value to be used to indicate that an object is to be private for current process processes
 */
#define MUTEXGEAR_PROCESS_PRIVATE			0x0000
/**
 *	\def MUTEXGEAR_PROCESS_SHARED
 *	\brief A value to be used to indicate that an object is to be shared among processes (the object must reside in a shared memory section)
 */
#define MUTEXGEAR_PROCESS_SHARED			0x0100

// Values for ..._setprotocol/..._getprotocol calls
/**
 *	\def MUTEXGEAR__INVALID_PROTOCOL
 *	\brief A lock protocol value that can be returned if current system does not support the lock protocol feature
 */
#define MUTEXGEAR__INVALID_PROTOCOL			(-1)
/**
 *	\def MUTEXGEAR_PRIO_INHERIT
 *	\brief A lock protocol value that requires priority inheritance to be used
 */
#define MUTEXGEAR_PRIO_INHERIT				0x0000
/**
 *	\def MUTEXGEAR_PRIO_PROTECT
 *	\brief A lock protocol value that requires priority ceiling to be used
 */
#define MUTEXGEAR_PRIO_PROTECT				0x0001
/**
 *	\def MUTEXGEAR_PRIO_NONE
 *	\brief A lock protocol value that requires priority adjustments to be not applied
 */
#define MUTEXGEAR_PRIO_NONE					0x0002


#endif // #ifndef __MUTEXGEAR_CONSTANTS_H_INCLUDED


