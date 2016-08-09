#include "TwoWaySendingConfirmatoryChannel.h"

#include "TwoWayConfirmatoryChannel.h"


void TwoWaySendingConfirmatoryChannel::onCreate()
{
	std::cout << "TwoWaySendingConfirmatoryChannel::onCreate()" << std::endl;
	this->twoWayConfirmatoryChannel->onCreate();
}