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
		//0x155 byte 0: Charging speed in 300W
		.id = 0x155,
		.pos = 0,
		.len = 8,
	},
	{
		//0x155 bytes 4, 5: Battery in 0.0025%
		.id = 0x155,
		.pos = 4 * 8,
		.len = 16,
	},
};

Message messages[] = {
	{
		.repetition = 1,
		.len = 19,
		.extractions = message0Extractions,
		.extractionCount = sizeof(message0Extractions) / sizeof(message0Extractions),
	},
};
#define MESSAGE_COUNT (sizeof(messages) / sizeof(Message))
