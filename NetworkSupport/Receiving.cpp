#include "Receiving.h"



// Konstruktor
Receiving::Receiving(Communication* communication, SOCKET socket, CONDITION_VARIABLE* CV_processing, CRITICAL_SECTION& CS_processing, ReceivedDatagram* lastReceivedDatagram, CRITICAL_SECTION& CS_lastReceivedDatagram)
{
	this->communication = communication;
	this->socket = socket;
	this->CV_processing = CV_processing;
	this->CS_processing = CS_processing;
	this->CS_lastReceivedDatagram = CS_lastReceivedDatagram;
	this->isInterrupted = false;
	this->lastReceivedDatagram = lastReceivedDatagram;
	this->thread = (HANDLE) _beginthread(&Receiving::start, 0, this);
}



Receiving::~Receiving()
{
	// !!!   Tohle pot�eba ov��it   !!!
	this->isInterrupted = true;
	WaitForSingleObject(this->thread, INFINITE);
	CloseHandle(this->thread);
}



// Spust� run
void Receiving::start(void* parameter)
{
	((Receiving*) parameter)->run();
}



// �innost vl�kna
void Receiving::run()
{
	ReceivedDatagram* nextReceivedDatagram;
	sockaddr* sourceAddress = new sockaddr;
	int sourceAddressLength = sizeof(*sourceAddress);
	char* buffer = new char[Communication::DATAGRAM_SIZE];
	int length;
	while (!this->isInterrupted)
	{
		// nutno vydebugovat ukon�en� p��j�m�n� datagram�
		length = recvfrom(this->socket, buffer, Communication::DATAGRAM_SIZE, 0, sourceAddress, &sourceAddressLength);
		//std::cout << "prijmuto " << (int) length << " bytu" << std::endl;
		if (length < 2) continue;

		nextReceivedDatagram = new ReceivedDatagram;
		nextReceivedDatagram->next = NULL;
		nextReceivedDatagram->datagram = new Datagram;
		nextReceivedDatagram->datagram->address = (sockaddr_in*) sourceAddress;
		nextReceivedDatagram->datagram->data = new Data;
		nextReceivedDatagram->datagram->data->length = length;
		sourceAddress = new sockaddr;

		if (length == Communication::DATAGRAM_SIZE)
		{
			nextReceivedDatagram->datagram->data->bytes = buffer;
			buffer = new char[Communication::DATAGRAM_SIZE];
		}
		else
		{
			nextReceivedDatagram->datagram->data->bytes = new char[length];
			memcpy(nextReceivedDatagram->datagram->data->bytes, buffer, length);
		}

		// P�id� datagram a probud� vl�kno, kter� provede jeho zpracov�n�
		EnterCriticalSection(&this->CS_lastReceivedDatagram); // LOCK
		this->lastReceivedDatagram->next = nextReceivedDatagram;
		WakeConditionVariable(this->CV_processing);
		LeaveCriticalSection(&this->CS_lastReceivedDatagram); // UNLOCK

		this->lastReceivedDatagram = nextReceivedDatagram;
	}
}