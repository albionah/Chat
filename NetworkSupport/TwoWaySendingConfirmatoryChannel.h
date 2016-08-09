#pragma once
#include "SendingConfirmatoryChannel.h"

class TwoWayConfirmatoryChannel;


class TwoWaySendingConfirmatoryChannel : public SendingConfirmatoryChannel
{
	friend TwoWayConfirmatoryChannel;

public:
	TwoWaySendingConfirmatoryChannel() { }
	virtual ~TwoWaySendingConfirmatoryChannel() { }
	void onCreate();

private:
	TwoWayConfirmatoryChannel* twoWayConfirmatoryChannel;
};

