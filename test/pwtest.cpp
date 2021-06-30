/************************************************************************/
/* The MutexGear Library                                                */
/* The library parent_wrapper implementation test file                  */
/*                                                                      */
/* Copyright (c) 2016-2021 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/

#include "pch.h"
#include "pwtest.h"
#include "mgtest_common.h"


#if _MGTEST_HAVE_CXX11

#include <mutexgear/dlps_list.hpp>
#include <mutexgear/parent_wrapper.hpp>

using mg::dlps_info;
using mg::dlps_list;
using mg::parent_wrapper;


#endif // #if _MGTEST_HAVE_CXX11


//////////////////////////////////////////////////////////////////////////
// ParentWrapper

enum EMGPARENTWRAPPERFEATURE
{
	MGPWF__MIN,

	MGPWF_GENERAL = MGPWF__MIN,

	MGPWF__MAX,

	MGPWF__TESTBEGIN = MGPWF__MIN,
	MGPWF__TESTEND = MGPWF__MAX,
	MGPWF__TESTCOUNT = MGPWF__TESTEND - MGPWF__TESTBEGIN,
};
MG_STATIC_ASSERT(MGPWF__TESTBEGIN <= MGPWF__TESTEND);


static bool PerformParentWrapperGeneralTest();

static const CFeatureTestProcedure g_afnParentWrapperFeatureTestProcedures[MGPWF__MAX] =
{
	&PerformParentWrapperGeneralTest, // MGPWF_GENERAL,
};

static const char *const g_aszParentWrapperFeatureTestNames[MGPWF__MAX] =
{
	"Class features", // MGPWF_GENERAL,
};


/*static */
bool CParentWrapperTest::RunTheTest(unsigned int &nOutSuccessCount, unsigned int &nOutTestCount)
{
	unsigned int nSuccessCount = 0;

	for (EMGPARENTWRAPPERFEATURE wfParentWrapperFeature = MGPWF__TESTBEGIN; wfParentWrapperFeature != MGPWF__TESTEND; ++wfParentWrapperFeature)
	{
		const char *szFeatureName = g_aszParentWrapperFeatureTestNames[wfParentWrapperFeature];
		printf("Testing %29s: ", szFeatureName);

		CFeatureTestProcedure fnTestProcedure = g_afnParentWrapperFeatureTestProcedures[wfParentWrapperFeature];
		bool bTestResult = fnTestProcedure();
		printf("%s\n", bTestResult ? "success" : "failure");

		if (bTestResult)
		{
			nSuccessCount += 1;
		}
	}

	nOutSuccessCount = nSuccessCount;
	nOutTestCount = MGPWF__TESTCOUNT;
	return nSuccessCount == MGPWF__TESTCOUNT;
}

class CLogicallyNegableIntWrapper
{
public:
	CLogicallyNegableIntWrapper() = default; // For g++
	CLogicallyNegableIntWrapper(int iValue) : m_iValue(iValue) {}

	bool operator !() const { return !m_iValue; }
	CLogicallyNegableIntWrapper &operator ++() { ++m_iValue; return *this; }
	CLogicallyNegableIntWrapper &operator --() { --m_iValue; return *this; }

private:
	int			m_iValue;
};


static
bool PerformParentWrapperGeneralTest()
{
	bool bResult = false;

	do
	{
#if _MGTEST_HAVE_CXX11

		dlps_list slEmptyLists[2];
		dlps_list &slEmptyList = slEmptyLists[0], &slAnotherEmptyList = slEmptyLists[1];

		parent_wrapper<dlps_list::const_iterator, 0> witUninitializedIterator;
		const parent_wrapper<dlps_list::const_iterator, 1> witBeginIterator(slEmptyList.begin());
		parent_wrapper<dlps_list::const_iterator, 2> witCopiedIterator(witBeginIterator);
		parent_wrapper<dlps_list::const_iterator, 3> witMovedIterator(std::move(witCopiedIterator));
		parent_wrapper<dlps_list::const_iterator, 4> witMoveSourceIterator(slAnotherEmptyList.begin());
		parent_wrapper<dlps_list::const_iterator, 5> witMoveTargetIterator;

		witUninitializedIterator = witMoveSourceIterator;
		witMoveTargetIterator = std::move(witUninitializedIterator);

		witMovedIterator.swap(witMoveTargetIterator);

		if (witMovedIterator == witBeginIterator
			|| !(witBeginIterator == witMoveTargetIterator)
			|| witMovedIterator == witMoveTargetIterator)
		{
			break;
		}

		if (witMovedIterator != witMoveSourceIterator
			|| !(witMoveSourceIterator != witMoveTargetIterator)
			|| !(witMovedIterator != witMoveTargetIterator))
		{
			break;
		}

		if (!(witMoveTargetIterator < witMovedIterator)
			|| (witMovedIterator < witMoveTargetIterator)
			|| (witMoveTargetIterator < witMoveTargetIterator)
			|| (witMovedIterator < witMovedIterator))
		{
			break;
		}

		if ((witMoveTargetIterator > witMovedIterator)
			|| !(witMovedIterator > witMoveTargetIterator)
			|| (witMoveTargetIterator > witMoveTargetIterator)
			|| (witMovedIterator > witMovedIterator))
		{
			break;
		}

		if (!(witMoveTargetIterator <= witMovedIterator)
			|| (witMovedIterator <= witMoveTargetIterator)
			|| !(witMoveTargetIterator <= witMoveTargetIterator)
			|| !(witMovedIterator <= witMovedIterator))
		{
			break;
		}

		if ((witMoveTargetIterator >= witMovedIterator)
			|| !(witMovedIterator >= witMoveTargetIterator)
			|| !(witMoveTargetIterator >= witMoveTargetIterator)
			|| !(witMovedIterator >= witMovedIterator))
		{
			break;
		}

		parent_wrapper<CLogicallyNegableIntWrapper, 0> wtsiZero(0), wtsiOne(1);
		parent_wrapper<CLogicallyNegableIntWrapper, 1> wtsiTwo(2);

		if (!!wtsiZero
			|| !wtsiOne
			|| !wtsiTwo)
		{
			break;
		}

		if (!++wtsiZero
			|| !wtsiZero)
		{
			break;
		}

		if (!!--wtsiOne
			|| !!wtsiOne)
		{
			break;
		}

		std::swap(wtsiOne, wtsiZero);
		if (!!wtsiZero
			|| !wtsiOne)
		{
			break;
		}

		std::swap(wtsiZero, wtsiTwo);
		if (!wtsiZero
			|| !!wtsiTwo)
		{
			break;
		}


#endif // #if _MGTEST_HAVE_CXX11


		bResult = true;
	}
	while (false);

	return bResult;
}

