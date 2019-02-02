#pragma once

#include <stdint.h>

//TODO generate this file from a KCD/DBC file or something similar
// currently this file is configured to extract information from the CAN bus of any Renault Twizy.

typedef struct
{
	uint32_t repetition; //every n seconds
	uint8_t len; //needs to be <= than what LoRaModem::maxLength() might return

	//dont set the properties below
	uint32_t nextTransmit;
	bool changed;
	uint8_t *data;
} Message;

typedef struct
{
	uint32_t can_id; //can id
	uint8_t mask[8]; //mask of bits which are extracted
	uint8_t message; //message index
	uint8_t bitPos; //index where to put the extracted bits in the message payload
} CANExtraction;

Message messages[] = {
	{
		.repetition = 1,
		.len = 19,
	},
};
#define MESSAGE_COUNT (sizeof(messages) / sizeof(Message))

CANExtraction extractions[] = {
	{
		//byte 0: Charging speed in 300W
		//bytes 4, 5: Battery in 0.0025%
		.can_id = 0x155,
		.mask = {0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00},
		.message = 0,
		.bitPos = 0,
	},
	{
		//byte 5: Motor temperature in °C + 40
		.can_id = 0x196,
		.mask = {0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00},
		.message = 0,
		.bitPos = 24,
	},
	//TODO 0x436 bytes 1-4: minutes since power on
	{
		//byte 0-7: Cell temperatures in °C + 40
		// TODO, are there Twizys with 8 cell packs? (the OVMSv3 source code suggests this)
		.can_id = 0x554,
		.mask = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00},
		.message = 0,
		.bitPos = 24 + 8,
	},
	{
		//byte 0, bit 6: connected to power
		//byte 1, bit 6: 1=off, 0=on
		//byte 1, bit 7: 1=charging, 0=not charging
		//byte 1, bit 5: "Klemme 15" (on when ignition key is turned)
		//byte 2: 12V power consumption in 0.2A
		//byte 3, bit 7, 6: 12V DC/DC converter status. 11=off, 10=charging 12V battery, 01=providing 12V power
		//byte 7: Charger temperature in °C + 40
		.can_id = 0x597,
		.mask = {0x20, 0x70, 0xff, 0xC0, 0x00, 0x00, 0x00, 0xff},
		.message = 0,
		.bitPos = 24 + 8 + 7 * 8,
	},
	//2 bit padding
	{
		//byte 0-3: odometer in km
		//byte 5: range in km
		.can_id = 0x599,
		.mask = {0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 0x00, 0x00},
		.message = 0,
		.bitPos = 24 + 8 + 7 * 8 + 24,
	},
	//TODO 0x69F bytes 1-4: vehicle identification number
	//pos 24 + 8 + 7 * 8 + 24 + 40 = 152 => length = 19 byte
};
#define EXTRACTION_COUNT (sizeof(extractions) / sizeof(CANExtraction))