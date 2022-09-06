/*
 Copyright (C)
	2022            OFreddy

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 3 as published by the Free Software Foundation.
*/

#include <Arduino.h>
#include <SPI.h>
#include <CircularBuffer.h>
#include <RF24.h>
#include <RF24_config.h>

// NTP
#include <WiFiUdp.h>
#include <TimeLib.h>

#include "_dbg.h"

#include "ms_ticker.h"
#include "hm_config_x.h"
#include "app.h"



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

app::app(void)
{
	mUptimeSecs = 0;
	mUptimeTicker = 0xffffffff;
	mUptimeInterval = 1000;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void app::setup(uint32_t timeout)
{
#if defined(ESP8266) || defined(ESP32)
	WiFi.begin(ssid, password);
	// Set and check host name
	escapedMac = WiFi.macAddress();
	escapedMac.replace(":", "");
	escapedMac.toLowerCase();
	prepareHostname(hostname, host);
	WiFi.setHostname(hostname);
#if defined(ESP32)
	WiFi.setTxPower(WIFI_POWER_17dBm);
#endif
	HM_PRINTF(HM_PSTR("Setting hostname '%s'\r\n"), hostname);
	HM_PRINTF(HM_PSTR("Connecting to '%s'\r\n"), ssid);
#endif

	st.setup(LATITUDE, LONGITUDE, TIMEZONE, TIME_GMT_OFFSET_S, TIME_DST_OFFSET_S, TIMESERVER_NAME);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void app::loop()
{
#if defined(ESP8266) || defined(ESP32)
	unsigned long currentMillis = millis();

	// if WiFi is down, try reconnecting
	if ((WiFi.status() != WL_CONNECTED) && (currentMillis - reconnectMillis >= 30000))
	{
		Serial.print(millis());
		Serial.println("Reconnecting to WiFi...");
		WiFi.disconnect();
		WiFi.reconnect();
		reconnectMillis = currentMillis;
	}

	if (WiFi.status() != oldWifiStatus)
	{
		oldWifiStatus = WiFi.status();
		if (WiFi.status() == WL_CONNECTED)
		{
            DBG_PRINTF(DBG_PSTR("WiFi connected. IP address: %s\r\n"), WiFi.localIP().toString().c_str());
		}
		else
		{
			DBG_PRINTF(DBG_PSTR("WiFi status changed to %u\n"), WiFi.status());
		}
	}

	mTimestamp = st.loop();

	// Update NTP time
	if (checkTicker(&mUptimeTicker, mUptimeInterval))
	{
		mUptimeSecs++;
		{
		}
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void app::prepareHostname(char *pdest, const char *pname)
{
	const char *pC = pname;
	uint8_t pos = 0;

	while (*pC && pos < 24)
	{ // while !null and not over length
		if (isalnum(*pC))
		{ // if the current char is alpha-numeric append it to the hostname
			pdest[pos] = *pC;
			pos++;
		}
		else if (*pC == ' ' || *pC == '_' || *pC == '-' || *pC == '+' || *pC == '!' || *pC == '?' || *pC == '*')
		{
			pdest[pos] = '-';
			pos++;
		}
		// else do nothing - no leading hyphens and do not include hyphens for all other characters.
		pC++;
	}
	// if the hostname is left blank, use the mac address/default mdns name
	if (pos < 6)
	{
		sprintf(pdest + pos, "%*s", 6, escapedMac.c_str() + 6);
	}
	else
	{ // last character must not be hyphen
		while (pos > 0 && pname[pos - 1] == '-')
		{
			pdest[pos - 1] = 0;
			pos--;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

String app::getDateTimeStr(time_t t)
{
	char str[20] = {0};
	sprintf(str, "%04d-%02d-%02d %02d:%02d:%02d", year(t), month(t), day(t), hour(t), minute(t), second(t));
	return String(str);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t app::getUnixTimeStamp()
{
	return mTimestamp;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// calculates the daylight saving time for middle Europe. Input: Unixtime in UTC
// from: https://forum.arduino.cc/index.php?topic=172044.msg1278536#msg1278536
time_t app::offsetDayLightSaving(uint32_t local_t)
{
    int m = month(local_t);
    if (m < 3 || m > 10)
        return 0; // no DSL in Jan, Feb, Nov, Dez
    if (m > 3 && m < 10)
        return 1; // DSL in Apr, May, Jun, Jul, Aug, Sep
    int y = year(local_t);
    int h = hour(local_t);
    int hToday = (h + 24 * day(local_t));
    if ((m == 3 && hToday >= (1 + TIMEZONE + 24 * (31 - (5 * y / 4 + 4) % 7))) || (m == 10 && hToday < (1 + TIMEZONE + 24 * (31 - (5 * y / 4 + 1) % 7))))
        return 1;
    else
        return 0;
}

