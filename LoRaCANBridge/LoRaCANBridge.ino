#include "./definitions.hpp"
#include "./messages.hpp"
#include "./MKRLoRa.hpp"
#include "./config.h"

#include <CAN.h>

IConnection *connection;
uint8_t **messageData;

void handleFrame(uint32_t id, uint8_t *data)
{
	CANExtraction *extraction = nullptr;
	for(size_t i = 0; i < EXTRACTION_COUNT; i++)
	{
		if(extractions[i].can_id == id)
		{
			extraction = &extractions[i];
			break;
		}			
	}

	if(extraction == nullptr)
		return;

	Message *msg = &messages[extraction->message];
	uint8_t *output = msg->data;
	size_t outputPos = extraction->bitPos / 8;
	uint8_t outputMask = 0x80 >> (extraction->bitPos % 8);

	for(uint8_t pos = 0; pos < 8 * 8; pos++)
	{
		uint8_t byte = pos / 8;
		uint8_t mask = 1 << (7 - pos % 8);

		if(extraction->mask[byte] == 0)
			continue;

		if(extraction->mask[byte] & mask)
		{
			uint8_t value = (output[outputPos] & ~outputMask) | (data[byte] & mask);

			if(value != output[outputPos])
			{
				output[outputPos] = value;
				msg->changed = true;
			}

			outputMask >>= 1;
			if(outputMask == 0)
			{
				outputMask = 0x80;

				outputPos++;
				if(outputPos >= msg->len)
					return;
			}
		}
	}
}

void setup()
{
	for(size_t i = 0; i < MESSAGE_COUNT; i++)
	{
		messages[i].data = (uint8_t *)calloc(messages[i].len, 1);
		messages[i].nextTransmit = messages[i].repetition * 60 * 1000;
		messages[i].changed = false;
	}

	connection = new MKRLoRa(LORA_BAND, LORA_EUI, LORA_KEY);
	
	CAN.begin(CAN_SPEED);

	//TODO CAN.filter()
}

void loop()
{
	while(true)
	{
		int len = CAN.parsePacket();
		if(len == 0)
			break;
		if(CAN.packetRtr())
			continue;

		uint8_t data[8];
		for(uint8_t i = 0; i < 8; i++)
		{
			if(i < len)
				data[i] = CAN.read();
			else
				data[i] = 0;
		}

		handleFrame(CAN.packetId(), data);
	}

	if(connection->available())
	{
		size_t maxLen = connection->maxLength();
		uint8_t buff[maxLen];
		connection->receive(buff, maxLen);

		//TODO do something with `buff`
	}

	uint32_t now = millis();
	for(size_t i = 0; i < MESSAGE_COUNT; i++)
	{
		uint32_t repetition = messages[i].repetition * 60 * 1000;
		if(now >= messages[i].nextTransmit
			&& (messages[i].nextTransmit >= repetition || now <= repetition))
		{
			connection->send(messages[i].data, messages[i].len);
			messages[i].nextTransmit = now + repetition;
			messages[i].changed = false;
		}
	}

	delay(10);
}
