/************************************************************************/
/* The MutexGear Library                                                */
/* The library test application main file                               */
/*                                                                      */
/* Copyright (c) 2016-2021 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/

#include "pch.h"
#include "pwtest.h"
#include "rwltest.h"
#include "mgtest_common.h"



//////////////////////////////////////////////////////////////////////////

static const char g_aszFeatureTestLevel_Quick[] = "quick";
static const char g_aszFeatureTestLevel_Basic[] = "basic";
static const char g_aszFeatureTestLevel_Extra[] = "extra";

static inline 
EMGRWLOCKFEATLEVEL DecodeMGRWLockFeatLevel(const char *szLevelName)
{
	EMGRWLOCKFEATLEVEL flResult = 
		strcmp(szLevelName, g_aszFeatureTestLevel_Quick) == 0 ? MGMFL_QUICK :
		strcmp(szLevelName, g_aszFeatureTestLevel_Basic) == 0 ? MGMFL_BASIC :
		strcmp(szLevelName, g_aszFeatureTestLevel_Extra) == 0 ? MGMFL_EXTRA :
		MGMFL__MAX;
	MG_STATIC_ASSERT(MGMFL__MAX == 3);
	
	return flResult;
}


enum EMUTEXGEARSUBSYSTEMTEST
{
	MGST__MIN,

	MGST_PARENT_WRAPPER = MGST__MIN,
	MGST_RWLOCK,
	MGST_TRDL_RWLOCK,

	MGST__MAX,
};

typedef bool (*CMGSubsystemTestProcedure)(unsigned int &nOutSuccessCount, unsigned int &nOutTestCount);


static const CMGSubsystemTestProcedure g_afnMGSubsystemTestProcedures[MGST__MAX] =
{
	&CParentWrapperTest::RunTheTest, // MGST_PARENT_WRAPPER,
	&CRWLockTest::RunTheBasicImplementationTest, // MGST_RWLOCK,
	&CRWLockTest::RunTheTRDLImplementationTest, // MGST_TRDL_RWLOCK,
};

static const char *const g_aszMGSubsystemNames[MGST__MAX] =
{
	"parent_wrapper", // MGST_PARENT_WRAPPER,
	"RWLock ("
#if _MGTEST_TEST_TWRL
	"with"
#else
	"w/o"
#endif
	" try-write, " MAKE_STRING_LITERAL(MGTEST_RWLOCK_ITERATION_COUNT) " cycles/thr)", // MGST_RWLOCK,
	"TRDL-RWLock ("
#if _MGTEST_TEST_TWRL
	"with"
#else
	"w/o"
#endif
	" try-write, "
#if _MGTEST_TEST_TRDL
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

	for (EMUTEXGEARSUBSYSTEMTEST stSubsystemTest = MGST__MIN; stSubsystemTest != MGST__MAX; ++stSubsystemTest)
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

	unsigned int nFailureCount = MGST__MAX - nSuccessCount;

	printf("\n==============================================\n");
	printf("Subsystem tests failed:         %3u out of %3u\n", nFailureCount, (unsigned int)MGST__MAX);

	nOutFailureCount = nFailureCount;
	return nSuccessCount == MGST__MAX;
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

			EMGRWLOCKFEATLEVEL flLevelValue = DecodeMGRWLockFeatLevel(szCurrentValue);
			if (flLevelValue == MGMFL__MAX)
			{
				fprintf(stderr, "Feature test level value is invalid: %s\n", szCurrentValue);
				bAnyFault = true;
				break;
			}

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

