#pragma once
#include <process.h>
#include <winsock2.h>
#include <atomic>

#include "ReceivedDatagram.h"
#include "Communication.h"

class Communication;



class Processing
{
public:
	Processing(Communication* communication, CONDITION_VARIABLE* CV_processing, CRITICAL_SECTION& CS_processing, ReceivedDatagram* frontReceivedDatagram, CRITICAL_SECTION& CS_frontReceivedDatagram);
	~Processing();

private:
	static void start(void*);
	void run();

	Communication* communication;
	CONDITION_VARIABLE* CV_processing;
	CRITICAL_SECTION CS_processing;
    ReceivedDatagram* frontReceivedDatagram;
	CRITICAL_SECTION CS_frontReceivedDatagram;
	HANDLE thread;
	std::atomic<bool> isInterrupted;
};

