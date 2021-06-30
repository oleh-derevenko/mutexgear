#ifndef __MGTEST_RWLTEST_H_INCLUDED
#define __MGTEST_RWLTEST_H_INCLUDED

/************************************************************************/
/* The MutexGear Library                                                */
/* The library RWLock implementation test header                        */
/*                                                                      */
/* Copyright (c) 2016-2021 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/


#define MGTEST_RWLOCK_ITERATION_COUNT	1000000
#define MGTEST_RWLOCK_WRITE_CHANNELS	4


enum EMGRWLOCKFEATLEVEL
{
	MGMFL__MIN,

	MGMFL_QUICK = MGMFL__MIN,

	MGMFL__DUMP_MIN,

	MGMFL_BASIC = MGMFL__DUMP_MIN,
	MGMFL_EXTRA,

	MGMFL__DUMP_MAX,

	MGMFL__MAX = MGMFL__DUMP_MAX,

	MGMFL__DEFAULT = MGMFL_QUICK,
};


class CRWLockTest
{
public:
	static bool RunTheBasicImplementationTest(unsigned int &nOutSuccessCount, unsigned int &nOutTestCount);
	static bool RunTheTRDLImplementationTest(unsigned int &nOutSuccessCount, unsigned int &nOutTestCount);

public:
	static void AssignSelectedFeatureTestLevel(EMGRWLOCKFEATLEVEL flLevelValue) { m_flSelectedFeatureRestLevel = flLevelValue; }
	static EMGRWLOCKFEATLEVEL RetrieveSelectedFeatureTestLevel() { return m_flSelectedFeatureRestLevel; }

private:
	static EMGRWLOCKFEATLEVEL	m_flSelectedFeatureRestLevel;
};


#endif // !__MGTEST_RWLTEST_H_INCLUDED

