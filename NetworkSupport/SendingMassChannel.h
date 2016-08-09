#pragma once
#include <list>
#include <queue>
#include <atomic>

#include "Channel.h"
#include "Data.h"
#include "DataBuilder.h"
#include "Connection.h"
class Connection;


class __declspec(dllexport) SendingMassChannel : public Channel
{
	friend class Connection;

public:
	SendingMassChannel(); // (Connection* connection, unsigned short sourceChannelNumber, unsigned short destinationChannelNumber);
	virtual ~SendingMassChannel();

	void onCreate() { notifyDataReady(); std::cout << "jsem pripraven vysilat" << std::endl; }

	virtual int onGetData(char* data, unsigned short size) = 0;

private:
	bool getData(Data** data);
	void processReceivedData(Data* data);

	char* dataBuffer;
	Data* data;
	std::queue<unsigned short> requestsForResending;
	std::atomic<unsigned int> positionToSend;
	std::atomic<unsigned int> positionConfirmed;
	std::atomic<bool> isEnd;
	CRITICAL_SECTION CS_requestsForResending;
	int delajiciChyby;
};