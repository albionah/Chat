#include "SendingMassChannel.h"


#include <chrono>
time_t getNanoTime()
{
	return ((float) clock()) / CLOCKS_PER_SEC * 1000000;
}

SendingMassChannel::SendingMassChannel() // (Connection* connection, unsigned short sourceChannelNumber, unsigned short destinationChannelNumber) : Channel(connection, sourceChannelNumber, destinationChannelNumber)
{
	this->type = MASS_CHANNEL;
	this->dataBuffer = new char[1444 * 15000];
	//this->data = new Data[15000];
	this->data = (Data*) malloc(15000 * sizeof(Data));
	for (int j = 0; j < 15000; j++)
	{
		this->data[j].bytes = this->dataBuffer + 1444 * j;
		this->data[j].bytes[0] = getDestinationChannelNumber(); // rozmyslet si, jestli nastavit tady nebo v connection
		this->data[j].bytes[1] = getDestinationChannelNumber() >> 8;
	}
	this->positionToSend = 0;
	this->positionConfirmed = 0;
	this->isEnd = false;
	InitializeCriticalSection(&this->CS_requestsForResending);

	//this->delajiciChyby = 0;
}



SendingMassChannel::~SendingMassChannel()
{
	operator delete[](this->data);
	delete[] this->dataBuffer;
	DeleteCriticalSection(&this->CS_requestsForResending);
}



void SendingMassChannel::processReceivedData(Data* data)
{
	DataParser parser(data, 2);
	unsigned int positionConfirmed = parser.getUint();
	std::cout << positionConfirmed << std::endl;

	if (positionConfirmed < this->positionConfirmed) { } // staré potvrzení
	else if (data->length == 7)
	{
		unsigned int positionToSend = this->positionToSend;
		EnterCriticalSection(&this->CS_requestsForResending); // LOCK
		for (int j = positionConfirmed; j < positionToSend; j++) requestsForResending.push(j);
		LeaveCriticalSection(&this->CS_requestsForResending); // UNLOCK
		this->positionConfirmed = positionConfirmed;
		if (this->isEnd)
		{
			std::cout << "zavri kanal" << std::endl;
			close();
		}
	}
	else if (positionConfirmed > this->positionConfirmed)
	{
		this->positionConfirmed = positionConfirmed;

		short count = (data->length - 6) / 4;
		std::cout << "pocet" << count << std::endl;
		EnterCriticalSection(&this->CS_requestsForResending); // LOCK
		for (int j = 0; j < count; j++) requestsForResending.push(parser.getUint());
		LeaveCriticalSection(&this->CS_requestsForResending); // UNLOCK
		notifyDataReady();
	}
	else if (positionConfirmed == this->positionConfirmed)
	{
		short count = (data->length - 6) / 4;
		EnterCriticalSection(&this->CS_requestsForResending); // LOCK
		if (requestsForResending.empty())
		{
			for (int j = 0; j < count; j++) requestsForResending.push(parser.getUint());
		}
		LeaveCriticalSection(&this->CS_requestsForResending); // UNLOCK
		notifyDataReady();
	}

	delete data;
}



bool SendingMassChannel::getData(Data** data)
{
	bool isEmpty;
	EnterCriticalSection(&this->CS_requestsForResending); // LOCK
	isEmpty = this->requestsForResending.empty();
	LeaveCriticalSection(&this->CS_requestsForResending); // UNLOCK

	if (!isEmpty)
	{
		unsigned int requestForResending;
		EnterCriticalSection(&this->CS_requestsForResending); // LOCK
		requestForResending = this->requestsForResending.front();
		this->requestsForResending.pop();
		LeaveCriticalSection(&this->CS_requestsForResending); // UNLOCK

		*data = &this->data[requestForResending % 15000];
		return true;
	}

	if (this->positionToSend - this->positionConfirmed >= 15000) return false;

	unsigned int positionToSend = this->positionToSend;
	Data& d = this->data[positionToSend % 15000];
	int length = onGetData(d.bytes + 6, 1444 - 6);
	if (length == 0)
	{
		this->isEnd = true;
		return false;
	}

	d.length = length + 6;

	d.bytes[0] = getDestinationChannelNumber();
	d.bytes[1] = getDestinationChannelNumber() >> 8;
	d.bytes[2] = positionToSend;
	d.bytes[3] = positionToSend >> 8;
	d.bytes[4] = positionToSend >> 16;
	d.bytes[5] = positionToSend >> 24;
	this->positionToSend++;

	/*this->delajiciChyby++;
	if (this->delajiciChyby % 500 == 0) return false;*/
	*data = &d;

	return true;
}