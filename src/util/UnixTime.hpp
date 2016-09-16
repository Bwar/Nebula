/*******************************************************************************
* Project:  DataAnalysis
* File:     UnixTime.hpp
* Description:   neb库time_t型时间处理API
* Author:        bwarliao
* Created date:  2010-12-14
* Modify history:
*******************************************************************************/

#ifndef UNIXTIME_HPP_
#define UNIXTIME_HPP_

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <iostream>
#include <string>
#include <sstream>
#include <sys/time.h>

namespace neb
{

time_t GetCurrentTime();
std::string GetCurrentTime(int iTimeSize);

time_t TimeStr2time_t(const char* szTimeStr);
time_t TimeStr2time_t(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

const std::string time_t2TimeStr(
        time_t lTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//本地时间与格林尼治时间相隔的秒数，返回正数为东时区，负数为西时区
long LocalTimeDiffGmTime();

//两时间之间相隔的秒数
long DiffTime(
        const std::string& strTime1,
        const std::string& strTime2,
        const std::string& strTimeFormat1 = "YYYY-MM-DD HH:MI:SS",
        const std::string& strTimeFormat2 = "YYYY-MM-DD HH:MI:SS");

//取指定时间所在小时开始一刻（00:00）
time_t GetBeginTimeOfTheHour(time_t lTime);
const std::string GetBeginTimeOfTheHour(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//取指定时间所在小时最后一刻（59:59）
time_t GetEndTimeOfTheHour(time_t lTime);
const std::string GetEndTimeOfTheHour(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//取指定时间所在天的开始一刻（00:00:00）
time_t GetBeginTimeOfTheDay(time_t lTime);
const std::string GetBeginTimeOfTheDay(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//取指定时间所在天的最后一刻（23:59:59）
time_t GetEndTimeOfTheDay(time_t lTime);
const std::string GetEndTimeOfTheDay(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//取指定时间所在月的开始一刻（00:00:00）
time_t GetBeginTimeOfTheWeek(time_t lTime);
const std::string GetBeginTimeOfTheWeek(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//取指定时间所在月的开始一刻（00:00:00）
time_t GetBeginTimeOfTheMonth(time_t lTime);
const std::string GetBeginTimeOfTheMonth(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//取指定时间所在月的最后一刻（23:59:59）
time_t GetEndTimeOfTheMonth(time_t lTime);
const std::string GetEndTimeOfTheMonth(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//取指定时间下一个月的开始一刻（00:00:00）
time_t GetBeginTimeOfNextMonth(time_t lTime);
const std::string GetBeginTimeOfNextMonth(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//取相对于指定时间nDaysBefore天前的时间，如果nDaysBefore为负数
//则取的是相对于指定时间nDaysBefore天后的时间
time_t GetDaysBefore(
        time_t lTime,
        int iDaysBefore);
const std::string GetDaysBefore(
        const std::string& strTime,
        int iDaysBefore,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//获取指定时间所在月共有多少天
int GetTotalDaysOfTheMonth(time_t lTime);
int GetTotalDaysOfTheMonth(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

unsigned long long GetMicrosecond();

}

#endif /* UNIXTIME_HPP_ */
