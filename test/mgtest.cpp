/************************************************************************/
/* The MutexGear Library                                                */
/* The Library Test Application Main File                               */
/*                                                                      */
/* Copyright (c) 2016-2025 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/

#include "pch.h"
#include "pwtest.h"
#include "rwltest.h"
#include "cqtest.h"
#include "mgtest_common.h"



//////////////////////////////////////////////////////////////////////////

static const char g_aszFeatureTestLevel_Quick[] = "quick";
static const char g_aszFeatureTestLevel_Basic[] = "basic";
static const char g_aszFeatureTestLevel_Extra[] = "extra";

static inline 
EMGTESTFEATURELEVEL DecodeMGTestFeatLevel(const char *szLevelName)
{
	EMGTESTFEATURELEVEL flResult = 
		strcmp(szLevelName, g_aszFeatureTestLevel_Quick) == 0 ? MGTFL_QUICK :
		strcmp(szLevelName, g_aszFeatureTestLevel_Basic) == 0 ? MGTFL_BASIC :
		strcmp(szLevelName, g_aszFeatureTestLevel_Extra) == 0 ? MGTFL_EXTRA :
		MGTFL__MAX;
	MG_STATIC_ASSERT(MGTFL__MAX == 3);
	
	return flResult;
}


enum EMUTEXGEARSUBSYSTEMTEST
{
	MGST__MIN,

	MGST_PARENT_WRAPPER = MGST__MIN,
	MGST_COMPLETION_QUEUES,
	MGST_RWLOCK,
	MGST_TRDL_RWLOCK,

	MGST__MAX,

	MGST__TESTBEGIN = MGST__MIN,
	MGST__TESTEND = MGST__MAX,
	MGST__TESTCOUNT = MGST__TESTEND - MGST__TESTBEGIN,
};

typedef bool (*CMGSubsystemTestProcedure)(unsigned int &nOutSuccessCount, unsigned int &nOutTestCount);


static const CMGSubsystemTestProcedure g_afnMGSubsystemTestProcedures[MGST__MAX] =
{
	&CParentWrapperTest::RunTheTest, // MGST_PARENT_WRAPPER,
	&CCompletionQueueTest::RunTheTest, // MGST_COMPLETION_QUEUES,
	&CRWLockTest::RunBasicImplementationTest, // MGST_RWLOCK,
	&CRWLockTest::RunTRDLImplementationTest, // MGST_TRDL_RWLOCK,
};

static const char *const g_aszMGSubsystemNames[MGST__MAX] =
{
	"parent_wrapper", // MGST_PARENT_WRAPPER,
	"Completion Queues", // MGST_COMPLETION_QUEUES,
	"RWLock ("
#if MGTEST_RWLOCK_TEST_TRYWRLOCK
	"with"
#else
	"w/o"
#endif
	" try-write, " MAKE_STRING_LITERAL(MGTEST_RWLOCK_ITERATION_COUNT) " cycles/thr)", // MGST_RWLOCK,
	"TRDL-RWLock ("
#if MGTEST_RWLOCK_TEST_TRYWRLOCK
	"with"
#else
	"w/o"
#endif
	" try-write, "
#if MGTEST_RWLOCK_TEST_TRYRDLOCK
	"with"
#else
	"w/o"
#endif
	" try-read, " MAKE_STRING_LITERAL(MGTEST_RWLOCK_ITERATION_COUNT) " cycles/thr)", // MGST_TRDL_RWLOCK,
};

static 
bool PerformMGCoverageTests(unsigned int &nOutFailureCount)
{
	unsigned int nSuccessCount = 0;

	for (EMUTEXGEARSUBSYSTEMTEST stSubsystemTest = MGST__TESTBEGIN; stSubsystemTest != MGST__TESTEND; ++stSubsystemTest)
	{
		const char *szSubsystemName = g_aszMGSubsystemNames[stSubsystemTest];
		printf("\nTesting subsystem \"%s\"\n", szSubsystemName);
		printf("----------------------------------------------\n");

		unsigned int nSubsysytemSuccessCount = 0, nSubsystemTestCount = 1;

		CMGSubsystemTestProcedure fnTestProcedure = g_afnMGSubsystemTestProcedures[stSubsystemTest];
		if (fnTestProcedure(nSubsysytemSuccessCount, nSubsystemTestCount) && nSubsysytemSuccessCount == nSubsystemTestCount)
		{
			nSuccessCount += 1;
		}

		unsigned int nSubsysytemFailureCount = nSubsystemTestCount - nSubsysytemSuccessCount;
		printf("----------------------------------------------\n");
		printf("Feature tests failed:           %3u out of %3u\n", nSubsysytemFailureCount, nSubsystemTestCount);
	}

	unsigned int nFailureCount = MGST__TESTCOUNT - nSuccessCount;

	printf("\n==============================================\n");
	printf("Subsystem tests failed:         %3u out of %3u\n", nFailureCount, (unsigned int)MGST__TESTCOUNT);

	nOutFailureCount = nFailureCount;
	return nSuccessCount == MGST__TESTCOUNT;
}


static const char g_ascFeatureSwitchString_FeatureTestLevel[] = "-l";
static const unsigned int g_uiFeatureSwitchLength_FeatureTestLevel = ARRAY_SIZE(g_ascFeatureSwitchString_FeatureTestLevel) - 1;

static 
bool ParseCommandLineArguments(unsigned int uiArgCount, char *pszArgValues[])
{
	bool bAnyFault = false;
	
	for (unsigned int uiCurrentArgIndex = 1; uiCurrentArgIndex < uiArgCount; ++uiCurrentArgIndex)
	{
		const char *szCurrentValue = pszArgValues[uiCurrentArgIndex];

		const char *szSwitchLine;
		unsigned int uiSwitchLength;
		if (strncmp(szCurrentValue, (szSwitchLine = g_ascFeatureSwitchString_FeatureTestLevel), (uiSwitchLength = g_uiFeatureSwitchLength_FeatureTestLevel)) == 0)
		{
			if (szCurrentValue[uiSwitchLength] != '\0')
			{
				szCurrentValue += uiSwitchLength;
			}
			else if (++uiCurrentArgIndex == uiArgCount || (szCurrentValue = pszArgValues[uiCurrentArgIndex])[0] == '\0')
			{
				fprintf(stderr, "Level value is required after %s\n", szSwitchLine);
				bAnyFault = true;
				break;
			}

			EMGTESTFEATURELEVEL flLevelValue = DecodeMGTestFeatLevel(szCurrentValue);
			if (flLevelValue == MGTFL__MAX)
			{
				fprintf(stderr, "Feature test level value is invalid: %s\n", szCurrentValue);
				bAnyFault = true;
				break;
			}

			CCompletionQueueTest::AssignSelectedFeatureTestLevel(flLevelValue);
			CRWLockTest::AssignSelectedFeatureTestLevel(flLevelValue);
		}
		else
		{
			fprintf(stderr, "Unknown command line argument: %s\n", szCurrentValue);
			bAnyFault = true;
			break;
		}
	}

	bool bResult = !bAnyFault;
	return bResult;
}


static 
void PrintUsage(const char *szProgramPath)
{
	fprintf(stderr, "Usage:\n"
		"%s [%s <Level>]\n"
		"\t<Level> - feature test level: %s|%s|%s\n", 
		szProgramPath, g_ascFeatureSwitchString_FeatureTestLevel,
		g_aszFeatureTestLevel_Quick, g_aszFeatureTestLevel_Basic, g_aszFeatureTestLevel_Extra);
}


int main(int iArgCount, char *pszArgValues[])
{
	unsigned int nFailureCount;

	do 
	{
		if (!ParseCommandLineArguments(iArgCount, pszArgValues))
		{
			PrintUsage(pszArgValues[0]);
			nFailureCount = 1;
			break;
		}

		PerformMGCoverageTests(nFailureCount);
	}
	while (false);

	return nFailureCount;
}

