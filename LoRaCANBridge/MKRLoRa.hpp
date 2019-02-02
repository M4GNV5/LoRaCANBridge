#pragma once

#include <MKRWAN.h>
#include "./definitions.hpp"

class MKRLoRa : public IConnection
{
private:
	LoRaModem modem;

public:
	MKRLoRa(_lora_band band, const char *eui, const char *key)
	{
		modem.begin(band);
		modem.joinOTAA(eui, key);
	}

	size_t maxLength()
	{
		// see https://github.com/arduino-libraries/MKRWAN/blob/master/src/MKRWAN.h#L790
		return 64;
	}

	bool available()
	{
		return modem.available();
	}
	
	size_t receive(uint8_t *data, size_t maxLen)
	{
		return modem.read(data, maxLen);
	}

	void send(uint8_t *data, size_t len)
	{
		modem.beginPacket();
		modem.write(data, len);
		modem.endPacket(true);
	}
};
