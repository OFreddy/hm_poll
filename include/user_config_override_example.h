/*
 Copyright (C)
    2022            OFreddy

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 3 as published by the Free Software Foundation.
*/

#ifndef _USER_CONFIG_OVERRIDE_H_
#define _USER_CONFIG_OVERRIDE_H_

/*****************************************************************************************************\
 * USAGE:
 *   To modify the stock configuration according to your personal settings:
 *   (1) copy this file to "user_config_override.h" (It will be ignored by Git)
 *   (2) define your own settings below
 *
\*****************************************************************************************************/

/* Examples

// -- Setup your own Wifi settings  ---------------
#undef  WIFI_SSID
#define WIFI_SSID         "YourSSID"         

#undef  WIFI_PASS
#define WIFI_PASS         "YourWifiPassword"     

#undef  TIMESERVER_NAME
#define TIMESERVER_NAME   "pool.ntp.org"

#undef  TIMEZONE
#define TIMEZONE          1        // Central European time +1

#undef  TIME_GMT_OFFSET_S  
#define TIME_GMT_OFFSET_S 3600     // The GMT offset in seconds for your location

#undef  TIME_DST_OFFSET_S  
#define TIME_DST_OFFSET_S 3600     // The DST offset in seconds dfor your location

#undef  LATITUDE
#define LATITUDE          53.2197  // [Latitude] Your location to be used with sunrise and sunset

#undef  LONGITUDE
#define LONGITUDE         7.98004  // [Longitude] Your location to be used with sunrise and sunset

*/

#endif // _USER_CONFIG_OVERRIDE_H_