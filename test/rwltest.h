#ifndef __MGTEST_RWLTEST_H_INCLUDED
#define __MGTEST_RWLTEST_H_INCLUDED

/************************************************************************/
/* The MutexGear Library                                                */
/* The Library RWLock Implementation Test Header                        */
/*                                                                      */
/* Copyright (c) 2016-2022 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/


#include "mgtest_common.h"


#define MGTEST_RWLOCK_TEST_TRYWRLOCK	1
#define MGTEST_RWLOCK_TEST_TRYRDLOCK	1

#define MGTEST_RWLOCK_ITERATION_COUNT	1000000
#define MGTEST_RWLOCK_WRITE_CHANNELS	4


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

