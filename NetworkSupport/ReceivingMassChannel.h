#pragma once
#include <list>
#include <atomic>

#include "Channel.h"
#include "Data.h"
#include "Connection.h"
class Connection;


typedef struct RequestForResending
{
	unsigned int id;
	time_t time;
} RequestForResending;


class __declspec(dllexport) ReceivingMassChannel : public Channel
{
	friend class Connection;

public:
	ReceivingMassChannel(); // (Connection* connection, unsigned short sourceChannelNumber, unsigned short destinationChannelNumber);
	virtual ~ReceivingMassChannel();

	virtual void processReceivedData(char* bytes, int length) = 0;

private:
	bool getData(Data** data);
	void processReceivedData(Data* data);

	void sendState(bool noDatagramLongTime);

	static void start(void*);
	void run();

	Data** dataBuffer;
	unsigned int startPosition;
	unsigned int expectedId;
	unsigned int confirmedId;
	std::atomic<time_t> commingOfLastDatagramTime;
	CRITICAL_SECTION CS_sendState;
	std::list<unsigned int> missingIds;
	std::list<RequestForResending*> requestsForResending;
	HANDLE thread;
	std::atomic<bool> isInterrupted;
	SRWLOCK LOCK_thread;
	CONDITION_VARIABLE CV_thread;
};