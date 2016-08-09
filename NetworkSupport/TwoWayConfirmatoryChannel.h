#pragma once
#include "ReceivingConfirmatoryChannel.h"

class TwoWaySendingConfirmatoryChannel;
class ConfirmatoryDataBuilder;
class FlexibleConfirmatoryDataBuilder;


class __declspec(dllexport) TwoWayConfirmatoryChannel : public ReceivingConfirmatoryChannel
{
public:
	TwoWayConfirmatoryChannel();
	virtual ~TwoWayConfirmatoryChannel();
	void onCreate();
	virtual void onCreate_() { };

	SendingConfirmatoryChannel* getSendingConfirmatoryChannel();
	void send(ConfirmatoryDataBuilder* data);
	void send(FlexibleConfirmatoryDataBuilder* data);

private:
	TwoWaySendingConfirmatoryChannel* sendingConfirmatoryChannel;
	bool wasOnCreateCalled;
};