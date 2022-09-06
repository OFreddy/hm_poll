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

#include "peripherals.h"
#include "app.h"
#include "hm_crc.h"
#include "hm_packets.h"

#define SER_BAUDRATE (115200)

#define INV1_RADIO_ID ((uint64_t)0x1946107301ULL) // 114173104619 = WR1
#define INV2_RADIO_ID ((uint64_t)0x3944107301ULL) // 114173104439 = WR2
#define INV3_RADIO_ID ((uint64_t)0x0121678101ULL) // 116181672101 = WR3
#define INV4_RADIO_ID ((uint64_t)0x3503506301ULL) // 104163500335 = WR4

uint32_t previousMillis = 0; // will store last time LED was updated
const long interval = 250;	 // interval at which to blink (milliseconds)
int ledState = LOW;			 // ledState used to set the LED

// Set up nRF24L01 radio on SPI bus plus CE/CS pins
// If more than one RF24 unit is used the another CS pin than 10 must be used
// This pin is used hard coded in SPI library
static RF24 radio(RF1_CE_PIN, RF1_CS_PIN);

// Hoymiles application instance
static app hmComm;

// Hoymiles packets instance
static HM_Packets hmPackets(radio);

// Function forward declaration
static void DumpConfig();


static IRAM_ATTR void handleNrf1Irq()
{
	hmPackets.RadioIrqCallback();
}

static void activateConf(void)
{
	// Attach interrupt handler to NRF IRQ output. Overwrites any earlier handler.
	attachInterrupt(digitalPinToInterrupt(RF1_IRQ_PIN), handleNrf1Irq, FALLING); // NRF24 Irq pin is active low.

	DumpConfig();
}

static void DumpConfig()
{
	Serial.println(F("\nRadio:"));
	radio.printDetails();

	Serial.println("");
}

void setup(void)
{
    hmComm.setup(5000);

#if defined(ARDUINO_AVR_NANO)
	pinMode(LED_PIN_STATUS, OUTPUT);

	// Test hardware => disable second RF24
	pinMode(RF2_CE_PIN, OUTPUT);
	pinMode(RF2_CS_PIN, OUTPUT);
	digitalWrite(RF2_CE_PIN, false);
	digitalWrite(RF2_CS_PIN, true);
	pinMode(RF2_IRQ_PIN, INPUT);
#endif
	// Configure nRF IRQ input
	pinMode(RF1_IRQ_PIN, INPUT);

	Serial.begin(SER_BAUDRATE);
	Serial.flush();

	// Add inverter instances - check HM_MAXINVERTERINSTANCES in hm_config.h
	bool res = hmPackets.AddInverterInstance(INV1_RADIO_ID);
	res &= hmPackets.AddInverterInstance(INV2_RADIO_ID);
	res &= hmPackets.AddInverterInstance(INV3_RADIO_ID);
	res &= hmPackets.AddInverterInstance(INV4_RADIO_ID);
	if (!res)
	{
		Serial.println(F("Failed to add inverter instances!"));
		while (1)
			;
	}

	if (!hmPackets.Begin())
	{
		Serial.println(F("Failed to start up radio!\n"));
		while (1)
			;
	}

	Serial.println(F("\r\n\r\n-- HM communication example --"));

	activateConf();
}

void loop(void)
{
    hmComm.loop();

	// Cyclic communication processing
	hmPackets.SetUnixTimeStamp(hmComm.getUnixTimeStamp());
	hmPackets.Cyclic();

	// Config info
	if (Serial.available())
	{
		uint8_t cmd = Serial.read();

		if (cmd == 'c')
		{
			DumpConfig();
		}
	}

	// Status LED
	unsigned long currentMillis = millis();
	if ((currentMillis - previousMillis) >= interval)
	{
		// save the last time you blinked the LED
		previousMillis = currentMillis;
		// if the LED is off turn it on and vice-versa:
		ledState = not(ledState);
		// set the LED with the ledState of the variable:
#if defined(ARDUINO_AVR_NANO)
		digitalWrite(LED_PIN_STATUS, ledState);
#endif
	}
}

