#pragma once
#include "FlexibleDataBuilder.h"



// T��da vytv��� informac� byty
class __declspec(dllexport) FlexibleConfirmatoryDataBuilder : public FlexibleDataBuilder
{
public:
	FlexibleConfirmatoryDataBuilder(): FlexibleDataBuilder(0, 5) { this->pointer = 4; };
};
