#pragma once
#include <list>

#include "Channel.h"
#include "ConfirmatoryDataBuilder.h"
#include "Datagram.h"
#include "DataForSending.h"
#include "Connection.h"
class Connection;



class __declspec(dllexport) SendingConfirmatoryChannel: public Channel
{
	friend class Connection;

public:
	SendingConfirmatoryChannel();
	virtual ~SendingConfirmatoryChannel();

	void send(ConfirmatoryDataBuilder* data);
	void send(FlexibleConfirmatoryDataBuilder* data);

private:
	void send(DataBuilder* data);
	bool getData(Data** data);
	void processReceivedData(Data* data);

	int waitingTime;
	unsigned short nextId;
	std::list<DataForSending*> dataForSending;
	CRITICAL_SECTION CS_dataForSending;
};

