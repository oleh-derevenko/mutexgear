#ifndef __MGTEST_RWLTEST_H_INCLUDED
#define __MGTEST_RWLTEST_H_INCLUDED

/************************************************************************/
/* The MutexGear Library                                                */
/* The Library RWLock Implementation Test Header                        */
/*                                                                      */
/* Copyright (c) 2016-2026 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/*                                                                      */
/************************************************************************/


#include "mgtest_common.h"


#ifndef MGTEST_RWLOCK_TEST_TRYWRLOCK
#define MGTEST_RWLOCK_TEST_TRYWRLOCK	1
#endif
#ifndef MGTEST_RWLOCK_TEST_TRYRDLOCK
#define MGTEST_RWLOCK_TEST_TRYRDLOCK	1
#endif

#ifndef MGTEST_RWLOCK_ITERATION_COUNT
#define MGTEST_RWLOCK_ITERATION_COUNT	1000000
#endif
#ifndef MGTEST_RWLOCK_WRITE_CHANNELS
#define MGTEST_RWLOCK_WRITE_CHANNELS	4
#endif

#define MGTEST_RWLOCK_MINIMAL_READERS_TILL_WP		1
#define MGTEST_RWLOCK_AVERAGE_READERS_TILL_WP		2
#define MGTEST_RWLOCK_SUBSTANTIAL_READERS_TILL_WP	4
#define MGTEST_RWLOCK_INFINITE_READERS_TILL_WP		(-1)


class CRWLockTest
{
public:
	static bool RunBasicImplementationTest(unsigned int &nOutSuccessCount, unsigned int &nOutTestCount);
	static bool RunTRDLImplementationTest(unsigned int &nOutSuccessCount, unsigned int &nOutTestCount);

public:
	static void AssignSelectedFeatureTestLevel(EMGTESTFEATURELEVEL flLevelValue) { m_flSelectedFeatureTestLevel = flLevelValue; }
	static EMGTESTFEATURELEVEL RetrieveSelectedFeatureTestLevel() { return m_flSelectedFeatureTestLevel; }

private:
	static EMGTESTFEATURELEVEL	m_flSelectedFeatureTestLevel;
};


#endif // !__MGTEST_RWLTEST_H_INCLUDED

