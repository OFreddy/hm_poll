/*
 Copyright (C)
	2022            OFreddy

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 3 as published by the Free Software Foundation.
*/

#ifndef __HM_TYPES_H__
#define __HM_TYPES_H__

#include <stdint.h>
#include "hm_config.h"

typedef struct _Serial_header_t
{
  uint32_t timestamp;
  uint8_t packetsLost;
  uint8_t address[HM_RF_ADDRESSWIDTH]; // MSB first, always RF_MAX_ADDR_WIDTH bytes.
} Serial_header_t;

typedef struct _HM_Packet_t
{
    uint32_t timestamp;
    uint8_t packetsLost;
    uint8_t channel;
    uint8_t packet[HM_MAXPAYLOADSIZE];
} HM_Packet_t;

typedef struct _HM_InverterInstance_t
{
    // Inverter data
    uint64_t address;
    uint8_t addressBytes[HM_RF_ADDRESSWIDTH];

    // Channel handling
    HM_TICKCOUNTTYPE channelHopTick;
    uint8_t activeRcvChannel;
    uint8_t activeSndChannel;

    // Timeout monitoring
    HM_TICKCOUNTTYPE sndTimeoutTick;
    HM_TICKCOUNTTYPE rcvTimeoutTick;
} HM_InverterInstance_t;

typedef enum 
{
    HM_State_Idle,
    HM_State_SetInvInstance,
    HM_State_Send,
    HM_State_CheckResponse,
    HM_State_CycleEnd
} HM_InternalState_e;

#endif // __HM_TYPES_H__
