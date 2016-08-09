#include "TwoWayConfirmatoryChannel.h"

#include "SendingConfirmatoryChannel.h"
#include "TwoWaySendingConfirmatoryChannel.h"
#include "ConfirmatoryDataBuilder.h"
#include "FlexibleConfirmatoryDataBuilder.h"



TwoWayConfirmatoryChannel::TwoWayConfirmatoryChannel()
{
	this->sendingConfirmatoryChannel = new TwoWaySendingConfirmatoryChannel;
	this->sendingConfirmatoryChannel->twoWayConfirmatoryChannel = this;
	this->wasOnCreateCalled = false;
}


TwoWayConfirmatoryChannel::~TwoWayConfirmatoryChannel()
{
}



void TwoWayConfirmatoryChannel::onCreate()
{
	if (this->wasOnCreateCalled) onCreate_();
	else this->wasOnCreateCalled = true;
}



SendingConfirmatoryChannel* TwoWayConfirmatoryChannel::getSendingConfirmatoryChannel()
{
	return this->sendingConfirmatoryChannel;
}



void TwoWayConfirmatoryChannel::send(ConfirmatoryDataBuilder* data)
{
	this->sendingConfirmatoryChannel->send(data);
}



void TwoWayConfirmatoryChannel::send(FlexibleConfirmatoryDataBuilder* data)
{
	this->sendingConfirmatoryChannel->send(data);
}