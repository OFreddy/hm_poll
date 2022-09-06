/*
 Copyright (C)
    2022            OFreddy

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 3 as published by the Free Software Foundation.
*/

#include "_dbg.h"

#include "suntime.h"

suntime::suntime()
{
    sun = new SunSet();

    timeState = TimeState_Idle;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

suntime::~suntime()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void suntime::setup(double lat, double lon, int tz, int gmtoffset, int dstoffset, const char* tsname)
{
    timezone = tz;

    // Sunset / dawn calculation
    sun->setPosition(lat, lon, tz);

    DBG_PRINTF(DBG_PSTR("Configuring time for timezone %i\r\n"), tz);
    configTime(gmtoffset, dstoffset, tsname);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

time_t suntime::loop(void)
{
    time_t now = 0;

    if (timeState == TimeState_Idle)
    {
        timeState = TimeState_Init;
    }
    else if (timeState == TimeState_Init)
    {
        if ((now = time(nullptr)) >= NTP_MIN_VALID_EPOCH)
        {
            DBG_PRINTF(DBG_PSTR("Local time: %s\r\n"), asctime(localtime(&now))); // print formated local time, same as ctime(&now)
            DBG_PRINTF(DBG_PSTR("UTC time:   %s\r\n"), asctime(gmtime(&now)));    // print formated GMT/UTC time
            calcSun();

            timeState = TimeState_Valid;
        }
    }
    else if (timeState == TimeState_Valid)
    {
        now = time(nullptr);
    }

    return now;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void suntime::calcSun(void)
{
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);
    sun->setCurrentDate(1900 + timeinfo->tm_year, timeinfo->tm_mon + 1, timeinfo->tm_mday);

    // If you have daylight savings time, make sure you set the timezone appropriately as well
    sun->setTZOffset(timezone + 1);
    sunrise = (int)sun->calcSunrise();
    sunset = (int)sun->calcSunset();
    int moonphase = sun->moonPhase(std::time(nullptr));

    DBG_PRINTF(DBG_PSTR("Sun phase for %04u-%02u-%02u ********\r\n"), 1900 + timeinfo->tm_year, timeinfo->tm_mon + 1, timeinfo->tm_mday);
    DBG_PRINTF(DBG_PSTR("Sunrise: %i %02u:%02u\r\n"), sunrise, sunrise / 60, sunrise % 60);
    DBG_PRINTF(DBG_PSTR("Sunset:  %i %02u:%02u\r\n"), sunset, sunset / 60, sunset % 60);
    DBG_PRINTF(DBG_PSTR("Moon:    %i\r\n"), moonphase);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
