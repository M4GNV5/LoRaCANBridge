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

int canIds[] = {
	0x597,
	0x599,
	0x424,
	0x155,
	0x554
};

CANExtraction message0Extractions[] = {
	{
		// Battery Status/SOC,
		.id = 0x155,
		.pos = 32,
		.len = 16,
	},
	{
		// Battery Health/SOH,
		.id = 0x424,
		.pos = 40,
		.len = 8,
	},
	{
		// Cell Temperatures/cell temperature 0, Cell Temperatures/cell temperature 1, Cell Temperatures/cell temperature 2, Cell Temperatures/cell temperature 3, Cell Temperatures/cell temperature 4, Cell Temperatures/cell temperature 5, Cell Temperatures/cell temperature 6,
		.id = 0x554,
		.pos = 0,
		.len = 56,
	},
	{
		// 12V Status/connected to grid,
		.id = 0x597,
		.pos = 2,
		.len = 1,
	},
	{
		// 12V Status/vehicle online, 12V Status/charging, 12V Status/key turned,
		.id = 0x597,
		.pos = 9,
		.len = 3,
	},
	{
		// 12V Status/DC/DC status,
		.id = 0x597,
		.pos = 24,
		.len = 2,
	},
	{
		// Driver Info/odometer,
		.id = 0x599,
		.pos = 0,
		.len = 32,
	},
	{
		// Driver Info/range,
		.id = 0x599,
		.pos = 40,
		.len = 8,
	},
};

Message messages[] = {
	{
		.repetition = 5,
		.len = 16,
		.extractions = message0Extractions,
		.extractionCount = sizeof(message0Extractions) / sizeof(CANExtraction)
	},
};

#define MESSAGE_COUNT (sizeof(messages) / sizeof(Message))
