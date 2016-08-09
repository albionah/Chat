#pragma once
#include "DataParser.h"



class __declspec(dllexport) ConfirmatoryDataParser : public DataParser
{
public:
	ConfirmatoryDataParser(Data* data) : DataParser(data) { this->pointer = 4; }
};

