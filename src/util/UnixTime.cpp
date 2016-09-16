/*******************************************************************************
* Project:  DataAnalysis
* File:     UnixTime.cpp
* Description: neb库time_t型时间处理API
* Author:        bwarliao
* Created date:  2010-12-14
* Modify history:
*******************************************************************************/


#include "UnixTime.hpp"

namespace neb
{

time_t GetCurrentTime()
{
    return time(NULL);
}

std::string GetCurrentTime(int iTimeSize)
{
    char szCurrentTime[20] = {0};
    time_t lTime = time(NULL);
    struct tm stTime;
    localtime_r(&lTime, &stTime);

    strftime(szCurrentTime, iTimeSize, "%Y-%m-%d %H:%M:%S", &stTime);
    return std::string(szCurrentTime);
}

time_t TimeStr2time_t(
        const std::string& strTime,
        const std::string& strTimeFormat)
{
    std::ostringstream osStream;
    if ("YYYY-MM-DD HH:MI:SS" == strTimeFormat || "yyyy-mm-dd hh:mi:ss"
            == strTimeFormat || "YYYY-MM-DD" == strTimeFormat || "yyyy-mm-dd"
            == strTimeFormat || "%Y-%m-%d %H:%M:%S" == strTimeFormat
            || "%Y-%m-%d" == strTimeFormat)
    {
        return TimeStr2time_t(strTime.c_str());
    }
    else if ("YYYYMMDDHHMISS" == strTimeFormat || "yyyymmddhhmiss"
            == strTimeFormat || "%Y%m%d%H%M%S" == strTimeFormat)
    {
        osStream << strTime[0] << strTime[1] << strTime[2]
                << strTime[3] << std::string("-") << strTime[4]
                << strTime[5] << std::string("-") << strTime[6]
                << strTime[7] << std::string(" ") << strTime[8]
                << strTime[9] << std::string(":") << strTime[10]
                << strTime[11] << std::string(":") << strTime[12]
                << strTime[13];
        return TimeStr2time_t(osStream.str().c_str());
    }
    else if ("YYYYMMDD" == strTimeFormat || "yyyymmdd" == strTimeFormat
            || "%Y-%m-%d" == strTimeFormat)
    {
        osStream << strTime[0] << strTime[1] << strTime[2]
                << strTime[3] << std::string("-") << strTime[4]
                << strTime[5] << std::string("-") << strTime[6]
                << strTime[7];
        return TimeStr2time_t(osStream.str().c_str());
    }

    return 0;
}

time_t TimeStr2time_t(const char *szTimeStr)
{
    int iYear = 1971;
    int iMonth = 1;
    int iDay = 1;
    int iHour = 0;
    int iMinute = 0;
    int iSecond = 0;
    int iTimeLen = strlen(szTimeStr);
    int iSegment = 0;
    char szTime[20] = {0};
    struct tm stTime;
    time_t lTime;

    if (iTimeLen > 19)
    {
        return -1;
    }

    for (int i = 0; i < iTimeLen; i++)
    {
        switch (szTimeStr[i])
        {
            case '-':
            case ':':
            case ' ':
                iSegment++;
                if (strlen(szTime) != 0)
                {
                    switch (iSegment)
                    {
                        case 1:
                            iYear = atoi(szTime);
                            break;
                        case 2:
                            iMonth = atoi(szTime);
                            break;
                        case 3:
                            iDay = atoi(szTime);
                            break;
                        case 4:
                            iHour = atoi(szTime);
                            break;
                        case 5:
                            iMinute = atoi(szTime);
                            break;
                        default:
                            return 0;
                    }
                    memset(szTime, 0, 20);
                }
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                sprintf(szTime, "%s%c", szTime, szTimeStr[i]);
                break;
            default:
                return 0;
        }
    }
    if (strlen(szTime) != 0)
    {
        switch (iSegment)
        {
            case 0:
                iYear = atoi(szTime);
                break;
            case 1:
                iMonth = atoi(szTime);
                break;
            case 2:
                iDay = atoi(szTime);
                break;
            case 3:
                iHour = atoi(szTime);
                break;
            case 4:
                iMinute = atoi(szTime);
                break;
            case 5:
                iSecond = atoi(szTime);
                break;
            default:
                ;
        }
    }

    /*
    if (iYear > 2037 || iYear < 1970)
    {
        return -1;
    }
    else */
    if (iHour >= 24 || iHour < 0)
    {
        return -1;
    }
    else if (iMinute >= 60 || iMinute < 0)
    {
        return -1;
    }
    else if (iSecond >= 60 || iSecond < 0)
    {
        return -1;
    }
    else
    {
        switch (iMonth)
        {
            case 1:
                if (iDay > 31 || iDay <= 0)
                {
                    return -1;
                }
                break;
            case 2:
                if (iYear % 400 == 0 || (iYear % 4 == 0 && iYear % 100 != 0))
                {
                    if (iDay > 29 || iDay <= 0)
                    {
                        return -1;
                    }
                }
                else if (iDay > 28 || iDay <= 0)
                {
                    return -1;
                }
                break;
            case 3:
                if (iDay > 31 || iDay <= 0)
                {
                    return -1;
                }
                break;
            case 4:
                if (iDay > 30 || iDay <= 0)
                {
                    return -1;
                }
                break;
            case 5:
                if (iDay > 31 || iDay <= 0)
                {
                    return -1;
                }
                break;
            case 6:
                if (iDay > 30 || iDay <= 0)
                {
                    return -1;
                }
                break;
            case 7:
                if (iDay > 31 || iDay <= 0)
                {
                    return -1;
                }
                break;
            case 8:
                if (iDay > 31 || iDay <= 0)
                {
                    return -1;
                }
                break;
            case 9:
                if (iDay > 30 || iDay <= 0)
                {
                    return -1;
                }
                break;
            case 10:
                if (iDay > 31 || iDay <= 0)
                {
                    return -1;
                }
                break;
            case 11:
                if (iDay > 30 || iDay <= 0)
                {
                    return -1;
                }
                break;
            case 12:
                if (iDay > 31 || iDay <= 0)
                {
                    return -1;
                }
                break;
            default:
                return -1;
        }
    }

    stTime.tm_year = iYear - 1900;
    stTime.tm_mon = iMonth - 1;
    stTime.tm_mday = iDay;
    stTime.tm_hour = iHour;
    stTime.tm_min = iMinute;
    stTime.tm_sec = iSecond;

    /* A positive or 0 value for tm_isdst shall cause mktime() to presume
     * initially that Daylight Savings Time, respectively, is or is not in
     * effect for the specified time. A negative value for tm_isdst shall
     * cause mktime() to attempt to determine whether Daylight Savings Time
     * is in effect for the specified time.
     *  */
    stTime.tm_isdst = -1;       // 特别注意这个变量的值

    tzset();
    lTime = mktime(&stTime);

    return lTime;
}

const std::string time_t2TimeStr(
        time_t lTime,
        const std::string& strTimeFormat)
{
    char szTime[20] = {0};
    struct tm stTime;
    localtime_r(&lTime, &stTime);

    if ("YYYY-MM-DD HH:MI:SS" == strTimeFormat
            || "yyyy-mm-dd hh:mi:ss" == strTimeFormat
            || "%Y-%m-%d %H:%M:%S" == strTimeFormat)
    {
        strftime(szTime, 20, "%Y-%m-%d %H:%M:%S", &stTime);
    }
    else if ("YYYY-MM-DD" == strTimeFormat || "yyyy-mm-dd" == strTimeFormat
            || "%Y-%m-%d" == strTimeFormat)
    {
        strftime(szTime, 20, "%Y-%m-%d", &stTime);
    }
    else if ("YYYYMMDDHHMISS" == strTimeFormat
            || "yyyymmddhhmiss" == strTimeFormat
            || "%Y%m%d%H%M%S" == strTimeFormat)
    {
        strftime(szTime, 20, "%Y%m%d%H%M%S", &stTime);
    }
    else if ("YYYYMMDDHHMI" == strTimeFormat || "yyyymmddhhmi" == strTimeFormat
            || "%Y%m%d%h%M" == strTimeFormat)
    {
        strftime(szTime, 20, "%Y%m%d%H%M", &stTime);
    }
    else if ("YYYYMMDDHH" == strTimeFormat || "yyyymmddhh" == strTimeFormat
            || "%Y%m%d%h" == strTimeFormat)
    {
        strftime(szTime, 20, "%Y%m%d%H", &stTime);
    }
    else if ("YYYYMMDD" == strTimeFormat || "yyyymmdd" == strTimeFormat
            || "%Y%m%d" == strTimeFormat)
    {
        strftime(szTime, 20, "%Y%m%d", &stTime);
    }
    else if ("YYYYMM" == strTimeFormat || "yyyymm" == strTimeFormat
            || "%Y%m" == strTimeFormat)
    {
        strftime(szTime, 20, "%Y%m", &stTime);
    }
    else if ("YYYY-MM" == strTimeFormat || "yyyy-mm" == strTimeFormat
            || "%Y-%m" == strTimeFormat)
    {
        strftime(szTime, 20, "%Y-%m", &stTime);
    }

    return szTime;
}

long LocalTimeDiffGmTime()
{
    time_t lTime = time(NULL);
    struct tm stLocalTime;
    struct tm stGmTime;
    tzset();
    localtime_r(&lTime, &stLocalTime);
    gmtime_r(&lTime, &stGmTime);

    return mktime(&stLocalTime) - mktime(&stGmTime);
}

long DiffTime(
        const std::string& strTime1,
        const std::string& strTime2,
        const std::string& strTimeFormat1,
        const std::string& strTimeFormat2)
{
    time_t lTime1, lTime2;
    long lSeconds;

    lTime1 = TimeStr2time_t(strTime1, strTimeFormat1);
    lTime2 = TimeStr2time_t(strTime2, strTimeFormat2);
    lSeconds = lTime2 - lTime1;
    return (lSeconds > 0)?(lSeconds):(lSeconds * -1);
}

time_t GetBeginTimeOfTheHour(time_t lTime)
{
    time_t lHourBeginTime = 0;
    struct tm stTime;

    tzset();
    localtime_r(&lTime, &stTime);

    stTime.tm_min = 0;
    stTime.tm_sec = 0;
    lHourBeginTime = mktime(&stTime);

    return lHourBeginTime;
}

const std::string GetBeginTimeOfTheHour(
        const std::string& strTime,
        const std::string& strTimeFormat)
{
    time_t lTime = 0;
    time_t lHourBeginTime = 0;
    lTime = TimeStr2time_t(strTime, strTimeFormat);
    lHourBeginTime = GetBeginTimeOfTheHour(lTime);
    return time_t2TimeStr(lHourBeginTime, strTimeFormat);
}

time_t GetEndTimeOfTheHour(time_t lTime)
{
    time_t lHourEndTime = 0;
    struct tm stTime;

    tzset();
    localtime_r(&lTime, &stTime);
    stTime.tm_min = 59;
    stTime.tm_sec = 59;
    lHourEndTime = mktime(&stTime);

    return lHourEndTime;
}

const std::string GetEndTimeOfTheHour(
        const std::string& strTime,
        const std::string& strTimeFormat)
{
    time_t lTime = 0;
    time_t lHourEndTime = 0;
    lTime = TimeStr2time_t(strTime, strTimeFormat);
    lHourEndTime = GetEndTimeOfTheHour(lTime);
    return time_t2TimeStr(lHourEndTime, strTimeFormat);
}

time_t GetBeginTimeOfTheDay(time_t lTime)
{
    time_t lDayBeginTime = 0;
    struct tm stTime;

    tzset();
    localtime_r(&lTime, &stTime);
    stTime.tm_hour = 0;
    stTime.tm_min = 0;
    stTime.tm_sec = 0;
    lDayBeginTime = mktime(&stTime);

    return lDayBeginTime;
}

const std::string GetBeginTimeOfTheDay(
        const std::string& strTime,
        const std::string& strTimeFormat)
{
    time_t lTime = 0;
    time_t lDayBeginTime = 0;
    lTime = TimeStr2time_t(strTime, strTimeFormat);
    lDayBeginTime = GetBeginTimeOfTheDay(lTime);
    return time_t2TimeStr(lDayBeginTime, strTimeFormat);
}

time_t GetEndTimeOfTheDay(time_t lTime)
{
    struct tm stTime;
    time_t lDayEndTime;

    tzset();
    localtime_r(&lTime, &stTime);
    stTime.tm_hour = 23;
    stTime.tm_min = 59;
    stTime.tm_sec = 59;
    lDayEndTime = mktime(&stTime);

    return lDayEndTime;
}

const std::string GetEndTimeOfTheDay(
        const std::string& strTime,
        const std::string& strTimeFormat)
{
    time_t lTime = 0;
    time_t lDayEndTime = 0;
    lTime = TimeStr2time_t(strTime, strTimeFormat);
    lDayEndTime = GetEndTimeOfTheDay(lTime);
    return time_t2TimeStr(lDayEndTime, strTimeFormat);
}

time_t GetBeginTimeOfTheWeek(time_t lTime)
{
    struct tm stTime;
    time_t lDayBeginTime;
    time_t lWeekBeginTime;

    tzset();
    localtime_r(&lTime, &stTime);
    stTime.tm_hour = 0;
    stTime.tm_min = 0;
    stTime.tm_sec = 0;
    lDayBeginTime = mktime(&stTime);
    lWeekBeginTime = lDayBeginTime - (stTime.tm_wday * 86400);

    return lWeekBeginTime;
}

const std::string GetBeginTimeOfTheWeek(
        const std::string& strTime,
        const std::string& strTimeFormat)
{
    time_t lTime = 0;
    time_t lWeekBeginTime = 0;
    lTime = TimeStr2time_t(strTime, strTimeFormat);
    lWeekBeginTime = GetBeginTimeOfTheWeek(lTime);
    return time_t2TimeStr(lWeekBeginTime, strTimeFormat);
}

time_t GetBeginTimeOfTheMonth(time_t lTime)
{
    struct tm stTime;
    time_t lMonthBeginTime;

    tzset();
    localtime_r(&lTime, &stTime);
    stTime.tm_mday = 1;
    stTime.tm_hour = 0;
    stTime.tm_min = 0;
    stTime.tm_sec = 0;
    lMonthBeginTime = mktime(&stTime);

    return lMonthBeginTime;
}

const std::string GetBeginTimeOfTheMonth(
        const std::string& strTime,
        const std::string& strTimeFormat)
{
    time_t lTime = 0;
    time_t lMonthBeginTime = 0;
    lTime = TimeStr2time_t(strTime, strTimeFormat);
    lMonthBeginTime = GetBeginTimeOfTheMonth(lTime);
    return time_t2TimeStr(lMonthBeginTime, strTimeFormat);
}

time_t GetEndTimeOfTheMonth(time_t lTime)
{
    struct tm stTime;
    time_t lMonthEndTime;

    tzset();
    localtime_r(&lTime, &stTime);
    stTime.tm_mon = (stTime.tm_mon + 1) % 12;
    stTime.tm_mday = 1;
    stTime.tm_hour = 23;
    stTime.tm_min = 59;
    stTime.tm_sec = 59;
    if (stTime.tm_mon == 0)
    {
        stTime.tm_year += 1;
    }
    lMonthEndTime = mktime(&stTime);

    return lMonthEndTime - 86400;
}

const std::string GetEndTimeOfTheMonth(
        const std::string& strTime,
        const std::string& strTimeFormat)
{
    time_t lTime = 0;
    time_t lMonthEndTime = 0;
    lTime = TimeStr2time_t(strTime, strTimeFormat);
    lMonthEndTime = GetEndTimeOfTheMonth(lTime);
    return time_t2TimeStr(lMonthEndTime, strTimeFormat);
}

time_t GetBeginTimeOfNextMonth(time_t lTime)
{
    struct tm stTime;
    time_t lMonthBeginTime;

    tzset();
    localtime_r(&lTime, &stTime);
    stTime.tm_mon = (stTime.tm_mon + 1) % 12;
    stTime.tm_mday = 1;
    stTime.tm_hour = 0;
    stTime.tm_min = 0;
    stTime.tm_sec = 0;
    if (stTime.tm_mon == 0)
    {
        stTime.tm_year += 1;
    }
    lMonthBeginTime = mktime(&stTime);

    return lMonthBeginTime;
}

const std::string GetBeginTimeOfNextMonth(
        const std::string& strTime,
        const std::string& strTimeFormat)
{
    time_t lTime = 0;
    time_t lMonthEndTime = 0;
    lTime = TimeStr2time_t(strTime, strTimeFormat);
    lMonthEndTime = GetBeginTimeOfNextMonth(lTime);
    return time_t2TimeStr(lMonthEndTime, strTimeFormat);
}

time_t GetDaysBefore(
        time_t lTime,
        int iDaysBefore)
{
    return lTime - (iDaysBefore * 86400);
}

const std::string GetDaysBefore(
        const std::string& strTime,
        int iDaysBefore,
        const std::string& strTimeFormat)
{
    time_t lTime;
    time_t lDaysBefore;

    lTime = TimeStr2time_t(strTime, strTimeFormat);
    lDaysBefore = GetDaysBefore(lTime, iDaysBefore);
    return time_t2TimeStr(lDaysBefore, strTimeFormat);
}

int GetTotalDaysOfTheMonth(time_t lTime)
{
    time_t lLastMoment = GetEndTimeOfTheMonth(lTime);
    struct tm stTime;

    localtime_r(&lLastMoment, &stTime);
    return stTime.tm_mday;
}

int GetTotalDaysOfTheMonth(
        const std::string& strTime,
        const std::string& strTimeFormat)
{
    time_t lTime;
    lTime = TimeStr2time_t(strTime, strTimeFormat);
    return GetTotalDaysOfTheMonth(lTime);
}

unsigned long long GetMicrosecond()
{
    timeval stTime;
    gettimeofday(&stTime, NULL);
    unsigned long long ullMsgId = stTime.tv_sec * 1000000 + stTime.tv_usec;
    return(ullMsgId);
}

}

