#pragma once
#include "DataBuilder.h"



class __declspec(dllexport) ConfirmatoryDataBuilder : public DataBuilder
{
public:
	ConfirmatoryDataBuilder(unsigned short length) : DataBuilder(length + 4) { this->pointer = 4; }
};