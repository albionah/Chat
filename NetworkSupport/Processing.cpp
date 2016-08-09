#include "Processing.h"



// Kontruktor
Processing::Processing(Communication* communication, CONDITION_VARIABLE* CV_processing, CRITICAL_SECTION& CS_processing, ReceivedDatagram* frontReceivedDatagram, CRITICAL_SECTION& CS_frontReceivedDatagram)
{
	this->communication = communication;
	this->CV_processing = CV_processing;
	this->CS_processing = CS_processing;
    this->frontReceivedDatagram = frontReceivedDatagram;
	this->CS_frontReceivedDatagram = CS_frontReceivedDatagram;
	this->isInterrupted = false;
	this->thread = (HANDLE) _beginthread(&Processing::start, 0, this);
}



Processing::~Processing(void)
{
	// !!!   Tohle potøeba ovìøit   !!!
	this->isInterrupted = true;
	ResumeThread(this->thread);
	WaitForSingleObject(this->thread, INFINITE);
	CloseHandle(this->thread);
	// Taky by se mìly vymazat nezpracované datagramy
}



// Spustí run
void Processing::start(void* parameter)
{
	((Processing*) parameter)->run();
}



// Èinnost vlákna
void Processing::run()
{
	ReceivedDatagram* nextReceivedDatagram;

	while (!this->isInterrupted)
	{
		EnterCriticalSection(&this->CS_frontReceivedDatagram); // LOCK
		if (this->frontReceivedDatagram->next == NULL)
		{
			SleepConditionVariableCS(this->CV_processing, &this->CS_frontReceivedDatagram, INFINITE);
		}
		LeaveCriticalSection(&this->CS_frontReceivedDatagram); // UNLOCK

		nextReceivedDatagram = this->frontReceivedDatagram->next;
		if (nextReceivedDatagram == NULL) continue;
		delete this->frontReceivedDatagram;
		this->frontReceivedDatagram = nextReceivedDatagram;

		this->communication->processReceivedDatagram(this->frontReceivedDatagram->datagram);
	}
}
