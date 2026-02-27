#ifndef __MGTEST_MGTEST_COMMON_H_INCLUDED
#define __MGTEST_MGTEST_COMMON_H_INCLUDED

/************************************************************************/
/* The MutexGear Library                                                */
/* The Library Test Application Shared Declarations                     */
/*                                                                      */
/* Copyright (c) 2016-2026 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/*                                                                      */
/************************************************************************/


enum EMGTESTFEATURELEVEL
{
	MGTFL__MIN,

	MGTFL_QUICK = MGTFL__MIN,

	MGTFL__DUMP_MIN,

	MGTFL_BASIC = MGTFL__DUMP_MIN,
	MGTFL_EXTRA,

	MGTFL__DUMP_MAX,

	MGTFL__MAX = MGTFL__DUMP_MAX,

	MGTFL__DEFAULT = MGTFL_QUICK,
};


#endif // !__MGTEST_MGTEST_COMMON_H_INCLUDED

