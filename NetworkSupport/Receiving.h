#pragma once
#include <process.h>
#include <winsock2.h>
#include <atomic>

#include "ReceivedDatagram.h"
#include "Communication.h"

class Communication;



// Tøída, která vytváøí vlákno, slouží pro pøijímání paketù a jejich ukládání
class Receiving
{
public:
	// Ovìøit, jestli by se to nemìlo posílat pøes ukazatele
	Receiving(Communication* communication, SOCKET socket, CONDITION_VARIABLE* CV_processing, CRITICAL_SECTION& CS_processing, ReceivedDatagram* lastReceivedDatagram, CRITICAL_SECTION& CS_lastReceivedDatagram);
	~Receiving();

private:
	static void start(void*);
	void run();

	Communication* communication;
	SOCKET socket;
	CONDITION_VARIABLE* CV_processing;
	CRITICAL_SECTION CS_processing;
	ReceivedDatagram* lastReceivedDatagram;
	CRITICAL_SECTION CS_lastReceivedDatagram;
	HANDLE thread;
	std::atomic<bool> isInterrupted;
};