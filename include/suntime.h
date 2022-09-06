/*
 Copyright (C)
	2022            OFreddy

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 3 as published by the Free Software Foundation.
*/

#ifndef _SUNTIME_H
#define _SUNTIME_H

#pragma once

#include <Arduino.h>
#include <TimeLib.h>
#include <sunset.h>

// August 1st, 2018
#define NTP_MIN_VALID_EPOCH 1659312000

typedef enum TimeState
{
    TimeState_Idle,
    TimeState_Init,
    TimeState_Valid

} E_TIMESTATE;

class suntime
{
public:
    suntime();
    ~suntime();

    void setup(double lat, double lon, int tz, int gmtoffset, int dstoffset, const char* tsname);
    time_t loop();

    
protected:
private:
    // NTP Service and sunset
    TimeState timeState;
    SunSet *sun;
    int sunrise, sunset, moonphase;
    int timezone;

    void calcSun(void);
};

#endif // _SUNTIME_H