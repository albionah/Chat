#include "DataBuilder.h"



DataBuilder::DataBuilder(unsigned short length)
{
	this->data = new Data(length);
	this->pointer = 0;
}



DataBuilder::DataBuilder(unsigned short length, unsigned short channelNumber)
{
	this->data = new Data(length + 2);
	this->pointer = 0;
	set(channelNumber);
}



DataBuilder::DataBuilder(const DataBuilder& base): pointer(base.pointer)
{
	this->data = new Data((const Data) *(base.data));
}



Data* DataBuilder::getData()
{
	return this->data;
}



char* DataBuilder::getBytes()
{
	return this->data->bytes;
}



unsigned short DataBuilder::getLength()
{
	return this->data->length;
}



void DataBuilder::clear()
{
	delete this->data;
}



void DataBuilder::set(bool value)
{
	this->data->bytes[this->pointer++] = value;
}



void DataBuilder::set(char value)
{
	this->data->bytes[this->pointer++] = value;
}



void DataBuilder::set(unsigned char value)
{
	set((char) value);
}



void DataBuilder::set(short value)
{
	this->data->bytes[this->pointer++] = value;
	this->data->bytes[this->pointer++] = value >> 8;
}



void DataBuilder::set(unsigned short value)
{
	set((short) value);
}



void DataBuilder::set(int value)
{
	this->data->bytes[this->pointer++] = value;
	this->data->bytes[this->pointer++] = value >> 8;
	this->data->bytes[this->pointer++] = value >> 16;
	this->data->bytes[this->pointer++] = value >> 24;
}



void DataBuilder::set(unsigned int value)
{
	set((int) value);
}



void DataBuilder::set(int64_t value)
{
	this->data->bytes[this->pointer++] = value;
	this->data->bytes[this->pointer++] = value >> 8;
	this->data->bytes[this->pointer++] = value >> 16;
	this->data->bytes[this->pointer++] = value >> 24;
	this->data->bytes[this->pointer++] = value >> 32;
	this->data->bytes[this->pointer++] = value >> 40;
	this->data->bytes[this->pointer++] = value >> 48;
	this->data->bytes[this->pointer++] = value >> 56;
}



void DataBuilder::set(uint64_t value)
{
	set((int64_t) value);
}



void DataBuilder::set(const char* text)
{
	memcpy((this->data->bytes + this->pointer), text, strlen(text) + 1);
	this->pointer += strlen(text) + 1;
}



void DataBuilder::set(unsigned char* bytes, unsigned short length)
{
	set(length);
	memcpy(this->data->bytes + this->pointer, bytes, length);
	this->pointer += length;
}



void DataBuilder::set(std::vector<unsigned char>* value)
{
	set(value->data(), value->size());
}



void DataBuilder::set(std::string* value)
{
	memcpy(this->data->bytes + this->pointer, value->data(), value->size());
	this->pointer += value->size();
	this->data->bytes[this->pointer++] = 0;
}