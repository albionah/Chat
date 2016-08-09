#include "DataParser.h"



DataParser::DataParser(Data* data)
{
	this->bytes = (unsigned char*) data->bytes;
	this->length = data->length;
	this->pointer = 0;
}



DataParser::DataParser(Data* data, unsigned short pointer)
{
	this->bytes = (unsigned char*) data->bytes;
	this->length = data->length;
	this->pointer = pointer;
}



DataParser::DataParser(char* bytes, unsigned short length)
{
	this->bytes = (unsigned char*) bytes;
	this->length = length;
	this->pointer = 0;
}



char* DataParser::getBytes()
{
	return (char*) this->bytes;
}



unsigned short DataParser::getLength()
{
	return this->length;
}



bool DataParser::getBool()
{
	return (this->bytes[this->pointer++] == 1);
}



char DataParser::getChar()
{
	return (char) this->bytes[this->pointer++];
}



unsigned char DataParser::getUchar()
{
	return this->bytes[this->pointer++];
}



const char* DataParser::getText()
{
	int length = strlen((const char*) (this->bytes + this->pointer)) + 1;
	char* data = new char[length];
	memcpy(data, (this->bytes + this->pointer), length);
	this->pointer += length;
	return data;
}



short DataParser::getShort()
{
	this->pointer += 2;
	return *((short*) &this->bytes[this->pointer - 2]);
}



unsigned short DataParser::getUshort()
{
	this->pointer += 2;
	return *((short*) &this->bytes[this->pointer - 2]);
}



int DataParser::getInt()
{
	this->pointer += 4;
	return *((int*) &this->bytes[this->pointer - 4]);
}



unsigned int DataParser::getUint()
{
	this->pointer += 4;
	return *((int*) &this->bytes[this->pointer - 4]);
}



int64_t DataParser::getInt64()
{
	this->pointer += 8;
	return *((int64_t*) &this->bytes[this->pointer - 8]);
}



uint64_t DataParser::getUint64()
{
	this->pointer += 8;
	return *((uint64_t*) &this->bytes[this->pointer - 8]);
}



unsigned short DataParser::getChars(unsigned char** value)
{
	unsigned short length = getShort();
	*value = new unsigned char[length];
	memcpy(*value, this->bytes + this->pointer, length);
	this->pointer += length;
	return length;
}



unsigned short DataParser::getPointerToChars(unsigned char** value)
{
	unsigned short length = getShort();
	*value = this->bytes + this->pointer;
	this->pointer += length;
	return length;
}



std::vector<unsigned char>* DataParser::getChars()
{
	unsigned short length = getShort();
	this->pointer += 2 + length;
	return new std::vector<unsigned char>(this->bytes + this->pointer - length, this->bytes + this->pointer);
}



std::string DataParser::getString()
{
	std::string string((char*) this->bytes + this->pointer);
	this->pointer += string.size() + 1;
	return string;
}