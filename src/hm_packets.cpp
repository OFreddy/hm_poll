/*
 Copyright (C)
	2022            OFreddy

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 3 as published by the Free Software Foundation.
*/

#include "Arduino.h"

#include "hm_crc.h"
#include "hm_packets.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Macros
#define HM_CNT_CHANNELS (sizeof(usedChannels) / sizeof(usedChannels[0]))

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Constructor
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HM_Packets::HM_Packets(RF24 radio)
{
	// initialize internal structures
	cntInvInst = 0;

	// RF 24 radio instance
	rf24Radio = radio;

	// CRC Generator for crc8 generation in frame
	crc8.setPolynome(0x01);
	crc8.setStartXOR(0);
	crc8.setEndXOR(0);

	// CRC Generator for crc16 generation in frame (Modbus compatible)
	crc16.setPolynome((uint16_t)0x18005);
	crc16.setStartXOR(0xFFFF);
	crc16.setEndXOR(0x0000);
	crc16.setReverseIn(true);
	crc16.setReverseOut(true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool HM_Packets::Begin()
{
	// Startup radio hardware
	if (!rf24Radio.begin())
		return false;

	// Unix timestamp emulation
	epochTick = HM_GETTICKCOUNT;

	// Prepare serial header for shockburst crc calculation
	// Initialize serial header's address member to promiscuous address.
	uint64_t addr = HM_DTU_RADIO_ID;
	for (int8_t i = sizeof(serialHdr.address) - 1; i >= 0; --i)
	{
		serialHdr.address[i] = addr;
		addr >>= 8;
	}

	// Radio settings
	rf24Radio.setDataRate(HM_RF_DATARATE);
	rf24Radio.disableCRC();
	rf24Radio.setPayloadSize(HM_MAXPAYLOADSIZE);
	rf24Radio.setAddressWidth(HM_RF_ADDRESSWIDTH);

	// Send PA level for tra´nsmitting packages
	rf24Radio.setPALevel(HM_RF_PA_LEVEL);

	// Disable shockburst for receiving and decode payload manually
	rf24Radio.setAutoAck(false);
	rf24Radio.setRetries(0, 0);

	// We wan't only RX irqs
	rf24Radio.maskIRQ(true, true, false);

	// Configure listening pipe with the simulated DTU address and start listening
	rf24Radio.openReadingPipe(1, HM_DTU_RADIO_ID);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool HM_Packets::AddInverterInstance(uint64_t addr)
{
	// Number of configured inverter instances exceeded
	if (cntInvInst >= HM_MAXINVERTERINSTANCES)
		return false;

	// inverter address
	tInverter[cntInvInst].address = addr;
	for (int8_t i = sizeof(tInverter[cntInvInst].addressBytes) - 1; i >= 0; --i)
	{
		tInverter[cntInvInst].addressBytes[i] = addr;
		addr >>= 8;
	}

	// preset instance data
	tInverter[cntInvInst].activeSndChannel = usedChannels[0];
	tInverter[cntInvInst].activeRcvChannel = usedChannels[HM_CNT_CHANNELS - 1];

	// Inkrement inverter instance
	cntInvInst++;

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HM_Packets::RadioIrqCallback(void)
{
	static uint8_t lostPacketCount = 0;
	uint8_t pipe;

	// Loop until RX buffer(s) contain no more packets.
	while (rf24Radio.available(&pipe))
	{
		if (!packetBuffer.isFull())
		{
			HM_Packet_t p;
			p.timestamp = millis();
			p.channel = tInverter[curInvInst].activeRcvChannel;
			p.packetsLost = lostPacketCount;
			uint8_t packetLen = rf24Radio.getPayloadSize();
			if (packetLen > HM_MAXPAYLOADSIZE)
				packetLen = HM_MAXPAYLOADSIZE;

			rf24Radio.read(p.packet, packetLen);

			// Get payload length and id from PCF
			uint8_t payloadLen = ((p.packet[0] & 0xFC) >> 2);
			// uint8_t payloadID = (p.packet[0] & 0x03); // For processing of repeated frames

			// Add to buffer for further processing
			if (HM_MAXPAYLOADSIZE >= payloadLen)
			{
				packetBuffer.unshift(p);
			}

			lostPacketCount = 0;
		}
		else
		{
			// Buffer full. Increase lost packet counter.
			bool tx_ok, tx_fail, rx_ready;
			if (lostPacketCount < 255)
				lostPacketCount++;
			// Call 'whatHappened' to reset interrupt status.
			rf24Radio.whatHappened(tx_ok, tx_fail, rx_ready);
			// Flush buffer to drop the packet.
			rf24Radio.flush_rx();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool HM_Packets::Cyclic(void)
{
	// Simulate unix timestamp
	if (HM_GETTICKCOUNT >= epochTick)
	{
		epochTick += 1000;
		UnixTimeStampTick();
	}

	return StateMachine();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool HM_Packets::PacketAvailable(void)
{
	return (!packetBuffer.isEmpty());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HM_Packet_t HM_Packets::GetPacket(void)
{
	return packetBuffer.pop();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HM_Packets::SetUnixTimeStamp(uint32_t ts)
{
	epochTime = ts;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t HM_Packets::GetUnixTimeStamp(void)
{
	return epochTime;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HM_Packets::UnixTimeStampTick()
{
	epochTime++;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HM_Packets::prepareBuffer(uint8_t *buf)
{
	// minimal buffer size of 32 bytes is assumed
	memset(buf, 0x00, HM_MAXPAYLOADSIZE);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HM_Packets::copyToBuffer(uint8_t *buf, uint32_t val)
{
	buf[0] = (uint8_t)(val >> 24);
	buf[1] = (uint8_t)(val >> 16);
	buf[2] = (uint8_t)(val >> 8);
	buf[3] = (uint8_t)(val & 0xFF);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HM_Packets::copyToBufferBE(uint8_t *buf, uint32_t val)
{
	memcpy(buf, &val, sizeof(uint32_t));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t HM_Packets::PrepareTimePacket(uint32_t wrAdr, uint32_t dtuAdr, uint8_t uk1, uint8_t uk2)
{
	prepareBuffer(reinterpret_cast<uint8_t *>(sendBuffer));

	sendBuffer[0] = 0x15;
	copyToBufferBE(&sendBuffer[1], wrAdr);
	copyToBufferBE(&sendBuffer[5], dtuAdr);
	sendBuffer[9] = 0x80;
	sendBuffer[10] = uk1;
	sendBuffer[11] = uk2;

	copyToBuffer(&sendBuffer[12], epochTime);

	sendBuffer[19] = 0x05;

	// CRC16
	crc16.restart();
	crc16.add(&sendBuffer[10], 14);
	sendBuffer[24] = crc16.getCRC() >> 8;
	sendBuffer[25] = crc16.getCRC() & 0xFF;

	// CRC16
	crc8.restart();
	crc8.add(&sendBuffer[0], 26);
	sendBuffer[26] = crc8.getCRC();

	return 27;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t HM_Packets::PrepareCmdPacket(uint32_t wrAdr, uint32_t dtuAdr, uint8_t mid, uint8_t cmd)
{
	sendBuffer[0] = mid;
	copyToBufferBE(&sendBuffer[1], wrAdr);
	copyToBufferBE(&sendBuffer[5], dtuAdr);
	sendBuffer[9] = cmd;

	// crc8
	crc8.restart();
	crc8.add(&sendBuffer[0], 26);
	sendBuffer[10] = crc8.getCRC();

	return 11;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HM_Packets::dumpMillis(HM_TICKCOUNTTYPE mil)
{
	HM_PRINTF(HM_PSTR("%05u."), mil / 1000);
	HM_PRINTF(HM_PSTR("%03u|"), mil % 1000);
}

void HM_Packets::dumpData(uint8_t *p, int len)
{
	while (len--)
	{
		HM_PRINTF(HM_PSTR("%02X"), *p++);
	}
	HM_PRINTF(HM_PSTR("|"));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// private member implementation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HM_Packets::IncrementChannel(uint8_t *pChannel)
{
	int curChannel;
	int cntChannels = HM_CNT_CHANNELS;
	for (curChannel = 0; curChannel < cntChannels; curChannel++)
	{
		if (*pChannel == usedChannels[curChannel])
			break;
	}
	if (curChannel >= cntChannels - 1)
		*pChannel = usedChannels[0];
	else
		*pChannel = usedChannels[curChannel + 1];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool HM_Packets::StateMachine(void)
{
	bool ret = false;
	switch (eState)
	{
	case HM_State_Idle:
		if ((cntInvInst > 0) && (cntInvInst < HM_MAXINVERTERINSTANCES))
			eState = HM_State_SetInvInstance;
		break;

	case HM_State_SetInvInstance:
		if (curInvInst >= cntInvInst)
			curInvInst = 0;
		eState = HM_State_Send;
		break;

	case HM_State_Send:
		PreparePacket();
		SendPacket();
		tInverter[curInvInst].sndTimeoutTick = HM_GETTICKCOUNT + 1000;
		eState = HM_State_CheckResponse;
		ret = true;
		break;

	case HM_State_CheckResponse:
		if (HM_GETTICKCOUNT < tInverter[curInvInst].sndTimeoutTick)
		{
			ProcessPacket();
		}
		else
		{
			IncrementChannel(&tInverter[curInvInst].activeRcvChannel);

			if (HM_GETTICKCOUNT > tInverter[curInvInst].rcvTimeoutTick)
			{
				tInverter[curInvInst].rcvTimeoutTick = HM_GETTICKCOUNT + 10000;
				IncrementChannel(&tInverter[curInvInst].activeSndChannel);
			}
			curInvInst++;
			eState = HM_State_CycleEnd;
		}
		break;

	case HM_State_CycleEnd:
		eState = HM_State_SetInvInstance;
		break;

	default:
		// Illegal state
		eState = HM_State_Idle;
		break;
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HM_Packets::PreparePacket(void)
{
	sendBytes = PrepareTimePacket(tInverter[curInvInst].address >> 8, HM_DTU_RADIO_ID >> 8, 0x11, 0x00);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HM_Packets::SendPacket(void)
{
	HM_DISABLE_EINT;
	rf24Radio.stopListening();

	// Debugging output
	HM_PRINTF(HM_PSTR("%02u|"), tInverter[curInvInst].activeSndChannel);
	dumpMillis(HM_GETTICKCOUNT);
	dumpMillis(HM_GETTICKCOUNT - lastDump);
	lastDump = HM_GETTICKCOUNT;

	HM_PRINTF(HM_PSTR("00|%02u|%08lX01|"), tInverter[curInvInst].activeSndChannel, tInverter[curInvInst].address >> 8);
	HM_PRINTF(HM_PSTR("    |  | |%02x|"), sendBuffer[0]);
	dumpData(&sendBuffer[1], 4);
	dumpData(&sendBuffer[5], 4);
	dumpData(&sendBuffer[9], sendBytes - 9);
	HM_PRINTF(HM_PSTR("\r"));

	// Overwrite send dump output
	sendLineLF = false;

	rf24Radio.setChannel(tInverter[curInvInst].activeSndChannel);
	rf24Radio.openWritingPipe(tInverter[curInvInst].address);
	rf24Radio.setCRCLength(RF24_CRC_16);
	rf24Radio.enableDynamicPayloads();
	rf24Radio.setAutoAck(true);
	rf24Radio.setRetries(3, 15);

	rf24Radio.write(sendBuffer, sendBytes);

	// Try to avoid zero payload acks => has no effect Reason for zero payload acks is unknown
	rf24Radio.openWritingPipe(HM_DUMMY_RADIO_ID);

	rf24Radio.setAutoAck(false);
	rf24Radio.setRetries(0, 0);
	rf24Radio.disableDynamicPayloads();
	rf24Radio.setCRCLength(RF24_CRC_DISABLED);
	rf24Radio.setChannel(tInverter[curInvInst].activeRcvChannel);
	rf24Radio.startListening();
	HM_ENABLE_EINT;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HM_Packets::ProcessPacket(void)
{
	while (PacketAvailable())
	{
		// One or more records present
		HM_Packet_t p = GetPacket();

		// Shift payload data due to 9-bit packet control field
		for (int16_t j = sizeof(p.packet) - 1; j >= 0; j--)
		{
			if (j > 0)
				p.packet[j] = (byte)(p.packet[j] >> 7) | (byte)(p.packet[j - 1] << 1);
			else
				p.packet[j] = (byte)(p.packet[j] >> 7);
		}

		serialHdr.timestamp = p.timestamp;
		serialHdr.packetsLost = p.packetsLost;

		// Check CRC
		uint16_t crc = 0xFFFF;
		crc = HM_crc16((uint8_t *)&serialHdr.address, sizeof(serialHdr.address), crc, 0, BYTES_TO_BITS(sizeof(serialHdr.address)));
		// Payload length
		uint8_t payloadLen = ((p.packet[0] & 0x01) << 5) | (p.packet[1] >> 3);

		// Add one byte and one bit for 9-bit packet control field
		crc = HM_crc16((uint8_t *)&p.packet[0], sizeof(p.packet), crc, 7, BYTES_TO_BITS(payloadLen + 1) + 1);

		if (checkCRC)
		{
			// If CRC is invalid only show lost packets
			if (((crc >> 8) != p.packet[payloadLen + 2]) || ((crc & 0xFF) != p.packet[payloadLen + 3]))
			{
				if (p.packetsLost > 0)
				{
					HM_PRINTF(HM_PSTR(" Lost: %u"), p.packetsLost);
				}
				continue;
			}

			// Dump a decoded packet only once
			if (lastPacketCRC == crc)
			{
				continue;
			}
			lastPacketCRC = crc;
		}

		// Valid packet received. Set timeout
		tInverter[curInvInst].rcvTimeoutTick = HM_GETTICKCOUNT + 10000;

		// Don't dump mysterious ack packages
		if (payloadLen == 0)
		{
			continue;
		}

		if (sendLineLF)
			HM_PRINTF(HM_PSTR("\n"));

		sendLineLF = false;
		// lastPacketRcv = serialHdr.timestamp;

		// Channel
		HM_PRINTF(HM_PSTR("%02u|"), p.channel);

		// Write timestamp, packets lost, address and payload length
		dumpMillis(serialHdr.timestamp);
		dumpMillis(serialHdr.timestamp - lastDump);

		lastDump = serialHdr.timestamp;
		dumpData((uint8_t *)&serialHdr.packetsLost, sizeof(serialHdr.packetsLost));
		printf_P(PSTR("%02u|"), p.channel);
		dumpData((uint8_t *)&serialHdr.address, sizeof(serialHdr.address));

		// Trailing bit?!?
		dumpData(&p.packet[0], 2);

		// Payload length from PCF
		dumpData(&payloadLen, sizeof(payloadLen));

		// Packet control field - PID Packet identification
		uint8_t val = (p.packet[1] >> 1) & 0x03;
		HM_PRINTF(HM_PSTR("%u|"), val);

		if (payloadLen > 9)
		{
			dumpData(&p.packet[2], 1);
			dumpData(&p.packet[3], 4);
			dumpData(&p.packet[7], 4);

			uint16_t remain = payloadLen - 2 - 1 - 4 - 4 + 4;

			if (remain < 32)
			{
				dumpData(&p.packet[11], remain);
				HM_PRINTF(HM_PSTR("%04X|"), crc);

				if (((crc >> 8) != p.packet[payloadLen + 2]) || ((crc & 0xFF) != p.packet[payloadLen + 3]))
					HM_PRINTF(HM_PSTR("0"));
				else
					HM_PRINTF(HM_PSTR("1"));
			}
			else
			{
				HM_PRINTF(HM_PSTR("Ill remain %u\n"), remain);
			}
		}
		else
		{
			dumpData(&p.packet[2], payloadLen + 2);
			HM_PRINTF(HM_PSTR("%04X|"), crc);
		}

		HM_PRINTF(HM_PSTR("\n"));
	}
}
