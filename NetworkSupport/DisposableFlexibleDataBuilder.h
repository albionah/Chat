#pragma once
#include "FlexibleDataBuilder.h"


class __declspec(dllexport) DisposableFlexibleDataBuilder : public FlexibleDataBuilder
{
public:
	DisposableFlexibleDataBuilder(): FlexibleDataBuilder() { };
	DisposableFlexibleDataBuilder(unsigned short channelNumber): FlexibleDataBuilder(channelNumber) { };
	DisposableFlexibleDataBuilder(unsigned short channelNumber, unsigned short startLength): FlexibleDataBuilder(channelNumber, startLength) { };
	~DisposableFlexibleDataBuilder() { delete this->data; };
};