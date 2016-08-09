#include "SendingConfirmatoryChannel.h"



SendingConfirmatoryChannel::SendingConfirmatoryChannel()
{
	this->waitingTime = 1000;
	this->type = CONFIRMATORY_CHANNEL;
	this->nextId = 0;
	InitializeCriticalSection(&this->CS_dataForSending);
	/*auto iterator = destinationChannelNumbers.begin();
	this->destinationSendingChannelNumber = *iterator;
	iterator++;
	this->destinationReceivingChannelNumber = *iterator;

	iterator = sourceChannelNumbers.begin();
	this->sourceSendingChannelNumber = *iterator;
	iterator++;
	this->sourceReceivingChannelNumber = *iterator;*/
}



SendingConfirmatoryChannel::~SendingConfirmatoryChannel()
{
	EnterCriticalSection(&this->CS_dataForSending); // LOCK
	for (DataForSending* d: this->dataForSending)
	{
		delete d;
	}
	LeaveCriticalSection(&this->CS_dataForSending); // UNLOCK

	DeleteCriticalSection(&this->CS_dataForSending);
}



void SendingConfirmatoryChannel::send(ConfirmatoryDataBuilder* data)
{
	send((DataBuilder*) data);
}



void SendingConfirmatoryChannel::send(FlexibleConfirmatoryDataBuilder* data)
{
	send((DataBuilder*) data);
}



void SendingConfirmatoryChannel::send(DataBuilder* data)
{
	unsigned int remainingSize = data->getLength();
	unsigned short size;
	bool isThisFirstFragment = true;
	bool isThisLastFragment = false;

	while (remainingSize > 0)
	{
		DataForSending* dataForSending = new DataForSending;
		if (data->getLength() > Communication::DATAGRAM_SIZE) // Pro velké datagramy
		{
			if (remainingSize > Communication::DATAGRAM_SIZE - 4) size = Communication::DATAGRAM_SIZE - 4;
			else
			{
				size = remainingSize;
				isThisLastFragment = true;
			}

			dataForSending->data = new Data(size + 4);
			memcpy(dataForSending->data->bytes + 4, data->getData()->bytes + 4 + data->getLength() - remainingSize, size);
			if (isThisLastFragment) delete data->getData();
		}
		else // Pro málé datagram, aby se zbyteènì nekopírovaly
		{
			dataForSending->data = data->getData();
			size = data->getLength();
			isThisLastFragment = true;
		}

		// Nastavení hlavièky datagramu
		dataForSending->data->bytes[0] = getDestinationChannelNumber();
		dataForSending->data->bytes[1] = getDestinationChannelNumber() >> 8;
		dataForSending->data->bytes[2] = this->nextId;
		dataForSending->data->bytes[3] = (this->nextId >> 8) & 0x3f; // Oøežeme první 2 bity
		if (isThisFirstFragment) dataForSending->data->bytes[3] |= 0x80;
		if (isThisLastFragment) dataForSending->data->bytes[3] |= 0x40;
		isThisFirstFragment = false;
		this->nextId = (this->nextId + 1) % (USHRT_MAX + 1);

		EnterCriticalSection(&this->CS_dataForSending); // LOCK
		this->dataForSending.push_back(dataForSending);
		LeaveCriticalSection(&this->CS_dataForSending); // UNLOCK

		remainingSize -= size;
	}

	notifyDataReady();

	
	/*
	DataForSending* dataForSending = new DataForSending;
	dataForSending->data = data->getData();
	dataForSending->data->bytes[0] = getDestinationChannelNumber();
	dataForSending->data->bytes[1] = getDestinationChannelNumber() >> 8;
	dataForSending->data->bytes[2] = this->nextId;
	dataForSending->data->bytes[3] = this->nextId >> 8;
	this->nextId = (this->nextId + 1) % (USHRT_MAX + 1);

	EnterCriticalSection(&this->CS_dataForSending); // LOCK
	this->dataForSending.push_back(dataForSending);
	LeaveCriticalSection(&this->CS_dataForSending); // UNLOCK

	notifyDataReady();
	*/
}



bool SendingConfirmatoryChannel::getData(Data** data)
{
	time_t currentTime = Communication::timeInMiliseconds();

	EnterCriticalSection(&this->CS_dataForSending); // LOCK
	for (DataForSending* dataForSending: this->dataForSending)
	{
		if ((dataForSending->attempts == 0) || (currentTime - dataForSending->lastAttempt > this->waitingTime))
		{
			*data = dataForSending->data;
			dataForSending->attempts++;
			dataForSending->lastAttempt = currentTime;
			LeaveCriticalSection(&this->CS_dataForSending); // UNLOCK
			return true;
		}
	}
	LeaveCriticalSection(&this->CS_dataForSending); // UNLOCK

	return false;
}



void SendingConfirmatoryChannel::processReceivedData(Data* data)
{
	DataForSending* dataForSending;
	EnterCriticalSection(&this->CS_dataForSending); // LOCK
	for (auto iterator = this->dataForSending.begin(); iterator != this->dataForSending.end(); iterator++)
	{
		dataForSending = *(iterator);
		if (*((short*) (dataForSending->data->bytes + 2)) == *((short*) (data->bytes + 2)))
		{
			this->dataForSending.erase(iterator);
			delete dataForSending;
			break;
		}
	}
	LeaveCriticalSection(&this->CS_dataForSending); // UNLOCK

	delete data;
}