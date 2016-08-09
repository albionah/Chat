#pragma once
#include <queue>
#include <chrono>

#include "Channel.h"
#include "Datagram.h"
#include "Data.h"
#include "Connection.h"
class Connection;



class __declspec(dllexport) SendingStreamChannel : public Channel
{
	friend class Connection;

public:
	SendingStreamChannel();
	virtual ~SendingStreamChannel();
	void onCreate() { notifyDataReady(); }

	//void send(Data* data);
	virtual unsigned int onGetData(char** data) = 0;

private:
	bool getData(Data** data);

	std::queue<Data*> queue;
	unsigned short nextId;
	std::chrono::system_clock::time_point last;

	/*int waitingTime;
	std::queue<Data*> dataForSending;
	CRITICAL_SECTION CS_dataForSending;
	bool wasFirstSent;*/
};

