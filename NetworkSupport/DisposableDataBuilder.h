#pragma once
#include "DataBuilder.h"



class __declspec(dllexport) DisposableDataBuilder : public DataBuilder
{
public:
	DisposableDataBuilder(unsigned short length) : DataBuilder(length) { };
	DisposableDataBuilder(unsigned short length, unsigned short channelNumber) : DataBuilder(length, channelNumber) { };
	~DisposableDataBuilder() { delete this->data; };
};

