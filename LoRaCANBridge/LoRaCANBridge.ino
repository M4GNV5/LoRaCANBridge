#include <CAN.h>

#include "./config.h"
#include "./log.hpp"

#include "./IConnection.hpp"
#include "./MKRLoRa.hpp"

#include "./messages.hpp"



IConnection *connection;
uint8_t **messageData;
Print *logger;

inline uint8_t readBit(uint8_t *buff, uint32_t pos)
{
	uint8_t byte = pos / 8;
	uint8_t mask = 1 << (7 - pos % 8);

	return buff[byte] & mask;
}

inline bool writeBit(uint8_t *buff, uint32_t pos, uint8_t val)
{
	uint8_t byte = pos / 8;
	uint8_t mask = 1 << (7 - pos % 8);

	if(val)
		val = mask;

	uint8_t oldByte = buff[byte];
	buff[byte] = (oldByte & ~mask) | val;

	return buff[byte] != oldByte;
}

void handleFrame(uint32_t id, uint8_t *data)
{
	LOG(DEBUG, "Received CAN frame with id ", id);

	Message *msg;
	CANExtraction *extraction = nullptr;
	uint8_t outputPos;
	for(size_t i = 0; i < MESSAGE_COUNT; i++)
	{
		msg = &messages[i];
		outputPos = 0;

		for(size_t j = 0; j < msg->extractionCount; j++)
		{
			if(msg->extractions[j].id == id)
			{
				extraction = &msg->extractions[j];
				break;
			}

			outputPos += msg->extractions[j].len;
		}
	}

	if(extraction == nullptr)
		return;

	LOG(DEBUG, "Found extraction for CAN frame");

	uint8_t end = extraction->pos + extraction->len;
	for(uint8_t pos = extraction->pos; pos < end; pos++)
	{
		uint8_t val = readBit(data, pos);

		if(writeBit(msg->data, outputPos, val))
			msg->changed = true;
		outputPos++;
	}

	LOG(DEBUG, "Copied ", extraction->len, " bytes to the message buffer");
}

void setup()
{
	Serial.begin(9600);
	logger = &Serial;

	LOG(DEBUG, "Initializing messages");
	for(size_t i = 0; i < MESSAGE_COUNT; i++)
	{
		messages[i].data = (uint8_t *)calloc(messages[i].len, 1);
		messages[i].nextTransmit = messages[i].repetition * 60 * 1000;
		messages[i].changed = false;
	}

	LOG(DEBUG, "Initializing LoRa");
	connection = new MKRLoRa(LORA_BAND, LORA_EUI, LORA_KEY);

	LOG(DEBUG, "Initializing CAN");
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

	if(connection->canSend())
	{
		uint32_t now = millis();

		for(size_t i = 0; i < MESSAGE_COUNT; i++)
		{
			if(!messages[i].changed)
				continue;

			uint32_t repetition = messages[i].repetition * 60 * 1000;
			if(now >= messages[i].nextTransmit
				&& (messages[i].nextTransmit >= repetition || now <= repetition))
			{
				LOG(DEBUG, "Transmitting message ", i);
				connection->send(messages[i].data, messages[i].len);
				messages[i].nextTransmit = now + repetition;
				messages[i].changed = false;
			}
		}
	}

	delay(1);
}
