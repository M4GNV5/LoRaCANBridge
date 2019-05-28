#include <CAN.h>
#include <MKRWAN.h>

#include "./config.h"
#include "./log.hpp"

#include "./messages.hpp"



LoRaModem modem;
uint8_t **messageData;
Print *logger;
uint32_t lastCanPacket;

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

void handleExtraction(uint8_t *data, Message *msg,
	CANExtraction *extraction, uint32_t outputPos)
{
	uint32_t end = extraction->pos + extraction->len;

	for(uint32_t pos = extraction->pos; pos < end; pos++)
	{
		uint8_t val = readBit(data, pos);

		if(writeBit(msg->data, outputPos, val))
			msg->changed = true;
		outputPos++;
	}

	LOG(DEBUG, "Copied ", extraction->len, " bits to the message buffer");
}

void handleFrame(uint32_t id, uint8_t *data)
{
	Message *msg;
	uint32_t outputPos;

	for(size_t i = 0; i < MESSAGE_COUNT; i++)
	{
		msg = &messages[i];
		outputPos = 0;

		for(size_t j = 0; j < msg->extractionCount; j++)
		{
			if(msg->extractions[j].id == id)
			{
				LOG(DEBUG, "Found extraction for CAN frame");
				handleExtraction(data, msg, &msg->extractions[j], outputPos);
			}

			outputPos += msg->extractions[j].len;
		}
	}
}

void setup()
{
	delay(5000);

	Serial.begin(9600);
	logger = &Serial;

	LOG(DEBUG, "Initializing messages");
	for(size_t i = 0; i < MESSAGE_COUNT; i++)
	{
		messages[i].data = (uint8_t *)calloc(messages[i].len, 1);
		messages[i].nextTransmit = messages[i].repetition * 60 * 1000;
		messages[i].changed = false;
	}

	LOG(INFO, "Initializing LoRa");
	modem.begin(LORA_BAND);
	LOG(INFO, "LoRa Device EUI: ", modem.deviceEUI());

	if(modem.joinOTAA(LORA_EUI, LORA_KEY))
		LOG(DEBUG, "Successfully connected to LoRa network");
	else
		LOG(ERROR, "Error connecting to LoRa Network!");

	LOG(INFO, "Initializing CAN");
	if(CAN.begin(CAN_SPEED))
		LOG(DEBUG, "Successfully initialized CAN");
	else
		LOG(ERROR, "Failed to initialize CAN");

	//TODO CAN.filter()

	lastCanPacket = millis();
}

void loop()
{
	while(true)
	{
		int len = CAN.parsePacket();
		if(len == 0)
			break;

		LOG(DEBUG, "Received CAN frame with id ", CAN.packetId(), " of length ", len);

		lastCanPacket = millis();

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

	if(modem.available())
	{
		uint8_t buff[64];
		modem.read(buff, 64);

		//TODO do something with `buff`
	}

	if(modem.connected())
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
				modem.beginPacket();
				modem.write(messages[i].data, messages[i].len);
				modem.endPacket(true);

				messages[i].nextTransmit = now + repetition;
				messages[i].changed = false;
			}
		}
	}

	// when we did not receive a new CAN frame for a longer period increase the delay
	// we could deepsleep here and be woken up by the RTC but the CAN & LoRa circuitry would run anyways.
	if(millis() - lastCanPacket > SLEEP_AFTER)
		delay(10 * 1000);
	else
		delay(1);
}
