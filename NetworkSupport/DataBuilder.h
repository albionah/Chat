#pragma once
#include <vector>

#include "Data.h"



// Tøída vytváøí z bytù informace nebo z informací byty
class __declspec(dllexport) DataBuilder
{
public:
	DataBuilder(unsigned short length);
	DataBuilder(unsigned short length, unsigned short channelNumber);
	DataBuilder(const DataBuilder& base);

	Data* getData();
	char* getBytes();
	unsigned short getLength();
	void clear();

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
	Data* data;
	unsigned short pointer;
};

