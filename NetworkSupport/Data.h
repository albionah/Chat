#pragma once
#include <string.h>



typedef struct Data
{
	Data() { }
	Data(unsigned short length) : length(length)
	{
		this->bytes = new char[length];
	}
	Data(char* bytes, unsigned short length) : bytes(bytes), length(length) { }
	Data(const Data& base): length(base.length)
	{
		this->bytes = new char[base.length];
		memcpy(this->bytes, base.bytes, base.length);
	}
	~Data()
	{
		delete[] bytes;
	}

	char* bytes;
	unsigned short length;

} Data;