#pragma once
#include <process.h>
#include <winsock2.h>
#include <atomic>

#include "ReceivedDatagram.h"
#include "Communication.h"

class Communication;



// T��da, kter� vytv��� vl�kno, slou�� pro p�ij�m�n� paket� a jejich ukl�d�n�
class Receiving
{
public:
	// Ov��it, jestli by se to nem�lo pos�lat p�es ukazatele
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