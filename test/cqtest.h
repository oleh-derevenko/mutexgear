#ifndef __MGTEST_WQTEST_H_INCLUDED
#define __MGTEST_WQTEST_H_INCLUDED

/************************************************************************/
/* The MutexGear Library                                                */
/* The Library Completion Queue Implementation Test Header              */
/*                                                                      */
/* Copyright (c) 2016-2025 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/


#include "mgtest_common.h"


#define MGTEST_CQ_QUEUE_SIZE_LIMIT			1000U
#define NGTEST_CQ_QUEUE_SIZE_HIWATER		900U

#define NGTEST_CQ_PRODUCER_COUNT			8U
#define NGTEST_CQ_CONSUMER_COUNT			8U



class CCompletionQueueTest
{
public:
	static bool RunTheTest(unsigned int &nOutSuccessCount, unsigned int &nOutTestCount);

public:
	static void AssignSelectedFeatureTestLevel(EMGTESTFEATURELEVEL flLevelValue) { m_flSelectedFeatureTestLevel = flLevelValue; }
	static EMGTESTFEATURELEVEL RetrieveSelectedFeatureTestLevel() { return m_flSelectedFeatureTestLevel; }

private:
	static EMGTESTFEATURELEVEL	m_flSelectedFeatureTestLevel;
};


#endif // ! __MGTEST_PWTEST_H_INCLUDED

