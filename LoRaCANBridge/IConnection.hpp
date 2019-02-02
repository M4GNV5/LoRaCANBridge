#pragma once

#include <stdlib.h>
#include <stdint.h>

class IConnection
{
public:
	virtual size_t maxLength();

	virtual bool available();
	virtual size_t receive(uint8_t *data, size_t maxLen);

	virtual void send(uint8_t *data, size_t len);
};
