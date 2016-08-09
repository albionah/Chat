#pragma once
#include <vector>

#include "Data.h"



// Tøída z bytù vytváøí informace
class __declspec(dllexport) DataParser
{
public:
	DataParser(Data* data);
	DataParser(Data* data, unsigned short pointer);
	DataParser(char* bytes, unsigned short length);

	char* getBytes();
	unsigned short getLength();

	bool getBool();
	char getChar();
	unsigned char getUchar();
	short getShort();
	unsigned short getUshort();
	int getInt();
	unsigned int getUint();
	int64_t getInt64();
	uint64_t getUint64();
	const char* getText();
	unsigned short getChars(unsigned char**);
	unsigned short getPointerToChars(unsigned char**);
	std::vector<unsigned char>* getChars();
	std::string getString();

protected:
	unsigned char* bytes;
	unsigned short length;
	unsigned short pointer;
};

