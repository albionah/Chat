#pragma once
#include "DataBuilder.h"



// Tøída vytváøí informací byty
class __declspec(dllexport) FlexibleDataBuilder: public DataBuilder
{
public:
	FlexibleDataBuilder() : DataBuilder(1) { };
	FlexibleDataBuilder(unsigned short channelNumber) : DataBuilder(2, channelNumber) { };
	FlexibleDataBuilder(unsigned short channelNumber, unsigned short startLength) : DataBuilder(startLength, channelNumber) { };

	void set(bool);
	void set(char);
	void set(unsigned char);
	void set(short);
	void set(unsigned short);
	void set(int);
	void set(unsigned int);
	void set(int64_t);
	void set(uint64_t);
	void set(const char* text);
	void set(unsigned char* bytes, unsigned short length);
	void set(std::vector<unsigned char>*);
	void set(std::string* text);

protected:
	void lengthen(unsigned short required);
};

