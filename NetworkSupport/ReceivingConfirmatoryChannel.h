#pragma once
#include <deque>

#include "Channel.h"
#include "Data.h"
#include "FragmentedData.h"
#include "ConfirmatoryDataParser.h"
#include "DataForSending.h"
#include "Connection.h"
class Connection;



class __declspec(dllexport) ReceivingConfirmatoryChannel: public Channel
{
	friend class Connection;

public:
	ReceivingConfirmatoryChannel();
	virtual ~ReceivingConfirmatoryChannel();
	
	virtual void processReceivedData(ConfirmatoryDataParser* data) = 0;

private:
	bool getData(Data** data);
	void processReceivedData(Data* data);

	int getDifference(unsigned short id);
	bool giveData();

	int waitingTime;
	unsigned short expectedId;
	unsigned short lastDatagramIdentification;
	std::deque<FragmentedData*> dataBuffer;
};

