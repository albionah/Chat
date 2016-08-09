#include "FlexibleDataBuilder.h"



void FlexibleDataBuilder::set(bool value)
{
	lengthen(1);
	DataBuilder::set(value);
}



void FlexibleDataBuilder::set(char value)
{
	lengthen(1);
	DataBuilder::set(value);
}



void FlexibleDataBuilder::set(unsigned char value)
{
	lengthen(1);
	DataBuilder::set(value);
}



void FlexibleDataBuilder::set(short value)
{
	lengthen(2);
	DataBuilder::set(value);
}



void FlexibleDataBuilder::set(unsigned short value)
{
	lengthen(2);
	DataBuilder::set(value);
}



void FlexibleDataBuilder::set(int value)
{
	lengthen(4);
	DataBuilder::set(value);
}



void FlexibleDataBuilder::set(unsigned int value)
{
	lengthen(4);
	DataBuilder::set(value);
}



void FlexibleDataBuilder::set(int64_t value)
{
	lengthen(8);
	DataBuilder::set(value);
}



void FlexibleDataBuilder::set(uint64_t value)
{
	lengthen(8);
	DataBuilder::set(value);
}



void FlexibleDataBuilder::set(const char* text)
{
	lengthen(strlen(text) + 1);
	DataBuilder::set(text);
}



void FlexibleDataBuilder::set(unsigned char* bytes, unsigned short length)
{
	lengthen(length);
	DataBuilder::set(bytes, length);
}



void FlexibleDataBuilder::set(std::vector<unsigned char>* value)
{
	lengthen(value->size());
	DataBuilder::set(value);
}



void FlexibleDataBuilder::set(std::string* value)
{
	lengthen(value->size());
	DataBuilder::set(value);
}



void FlexibleDataBuilder::lengthen(unsigned short required)
{
	short difference = this->data->length - (this->pointer + required);
	if (difference < 0)
	{
		this->data->length -= difference; // difference bude vždy záporná, ale chceme ji pøièíst k length, proto mínus
		this->data->bytes = (char*) realloc(this->data->bytes, this->data->length);
	}
}