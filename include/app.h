
#ifndef _HM_COMM_H
#define _HM_COMM_H

// Wifi
#if defined(ESP32)
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#endif

// NTP time and sun
#include "suntime.h"

#include "user_config.h"

class app
{
public:
    app(void);
    virtual void setup(uint32_t timeout);
    virtual void loop();
    String getDateTimeStr(time_t t);
    uint32_t getUnixTimeStamp();

protected:
private:
    // Wifi control
    unsigned long reconnectMillis = 0;          // will store last time of WiFi reconnection retry
    wl_status_t oldWifiStatus = WL_IDLE_STATUS; // Wifi status indication

    // Wifi configuration
    const char *host = "ESP-DTU";
    const char *ssid = WIFI_SSID;
    const char *password = WIFI_PASS;
    String escapedMac;
    char hostname[33];
    char serverDescription[33]; // Name of module

    suntime st;

    uint32_t mTimestamp;
    uint32_t mUptimeTicker;
    uint16_t mUptimeInterval;
    uint32_t mUptimeSecs;

    void prepareHostname(char *pdest, const char *pname);


    time_t offsetDayLightSaving(uint32_t local_t);
};

#endif