#pragma once

#include <stdint.h>

//TODO generate this file from a KCD/DBC file or something similar
// currently this file is configured to extract information from the CAN bus of any Renault Twizy.

typedef struct
{
	uint32_t id; //can id
	uint8_t pos; //bit index in the can message
	uint8_t len; //length in bits
} CANExtraction;

typedef struct
{
	uint32_t repetition; //every n seconds
	uint8_t len; //needs to be <= than what LoRaModem::maxLength() might return
	CANExtraction *extractions;
	size_t extractionCount;

	//dont set the properties below
	uint32_t nextTransmit;
	bool changed;
	uint8_t *data;
} Message;

CANExtraction message0Extractions[] = {
	{
		//0x554 bytes 0-8: Cell tempertures
		.id = 0x554,
		.pos = 0,
		.len = 64,
	},
	{
		//0x599 bytes 0-4: Odometer
		.id = 0x599,
		.pos = 0,
		.len = 32,
	},
	{
		//0x155 byte 0: Charging speed in 300W
		.id = 0x155,
		.pos = 0,
		.len = 8,
	},
	{
		//0x155 bytes 4, 5: Battery in 0.0025%
		.id = 0x155,
		.pos = 32,
		.len = 16,
	},
	{
		//0x424 bytes 5: Battery SOH in %
		.id = 0x424,
		.pos = 40,
		.len = 8,
	},
	{
		//0x597 bit 0: connected to grid
		.id = 0x597,
		.pos = 2,
		.len = 1,
	},
	{
		//0x597 bit 9, 10, 11: offline, charging, key turned
		.id = 0x597,
		.pos = 9,
		.len = 3,
	},
	{
		//0x597 bit 24, 25: DC/DC status
		.id = 0x597,
		.pos = 24,
		.len = 2,
	},
};

Message messages[] = {
	{
		.repetition = 1,
		.len = 17,
		.extractions = message0Extractions,
		.extractionCount = sizeof(message0Extractions) / sizeof(CANExtraction),
	},
};
#define MESSAGE_COUNT (sizeof(messages) / sizeof(Message))
