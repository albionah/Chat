#include "ReceivingMassChannel.h"



ReceivingMassChannel::ReceivingMassChannel() //(Connection* connection, unsigned short sourceChannelNumber, unsigned short destinationChannelNumber) : Channel(connection, sourceChannelNumber, destinationChannelNumber)
{
	this->type = MASS_CHANNEL;
	this->dataBuffer = new Data*[15000];
	//for (int j = 0; j < 15000; j++) this->dataBuffer[j] = NULL; // Pak smazat
	this->startPosition = 0;
	this->expectedId = 0;
	this->confirmedId = 0;
	this->commingOfLastDatagramTime = Communication::timeInMiliseconds();
	InitializeCriticalSection(&this->CS_sendState);
	InitializeSRWLock(&this->LOCK_thread);
	InitializeConditionVariable(&this->CV_thread);

	this->isInterrupted = false;
	this->thread = (HANDLE) _beginthread(&ReceivingMassChannel::start, 0, this);
}



ReceivingMassChannel::~ReceivingMassChannel()
{
	this->isInterrupted = true;
	AcquireSRWLockExclusive(&this->LOCK_thread);
	WakeConditionVariable(&this->CV_thread);
	ReleaseSRWLockExclusive(&this->LOCK_thread);
	WaitForSingleObject(this->thread, INFINITE);

	// TODO: vymazat data, která nebyla zpracována
	// asi nejjednodušším øešením by bylo ukládat do ukazatelù NULL a podle toho mazat
	//for (unsigned int j = this->confirmedId; this->confirmedId != this->expectedId; j = (j + 1) % 15000) delete this->dataBuffer[j];
	delete[] this->dataBuffer;
	DeleteCriticalSection(&this->CS_sendState);
}



bool ReceivingMassChannel::getData(Data** data)
{
	return false;
}



void ReceivingMassChannel::processReceivedData(Data* data)
{
	this->commingOfLastDatagramTime = Communication::timeInMiliseconds();

	EnterCriticalSection(&this->CS_sendState);
	unsigned int id = *((int*) &data->bytes[2]);

	if (id == this->expectedId)
	{
		if (this->missingIds.empty())
		{
			processReceivedData(data->bytes + 6, data->length - 6);
			delete data;
		}
		else
		{
			this->dataBuffer[(this->startPosition + id - *(this->missingIds.begin()) - 1) % 15000] = data;
		}
		this->expectedId++;
	}
	else if (id > this->expectedId)
	{
		unsigned int position;
		if (this->missingIds.empty()) position = id - this->expectedId - 1; // pokud by rozdíl byl vìtší než kapacita bufferu, tak to je prùser
		else position = (this->startPosition + id - *(this->missingIds.begin()) - 1) % 15000;
		this->dataBuffer[position] = data;

		for (unsigned int j = this->expectedId; j < id; j++) this->missingIds.push_back(j);
		this->expectedId = id + 1;
	}
	else if (id < this->expectedId)
	{
		bool found = false;
		for (auto iterator = this->missingIds.begin(); iterator != this->missingIds.end(); iterator++)
		{
			if (*iterator == id)
			{
				found = true;
				if (iterator == this->missingIds.begin())
				{
					processReceivedData(data->bytes + 6, data->length - 6);
					delete data;

					unsigned int count;
					unsigned int lastStartPosition = this->startPosition;
					if (this->missingIds.size() > 1)
					{
						iterator++;
						count = *iterator - id - 1;
						this->startPosition = (this->startPosition + *iterator - id) % 15000;
					}
					else
					{
						count = this->expectedId - id - 1;
						this->startPosition = 0;
					}
					count += lastStartPosition;
					for (int j = lastStartPosition; j < count; j++)
					{
						processReceivedData(this->dataBuffer[j % 15000]->bytes + 6, this->dataBuffer[j % 15000]->length - 6);
						delete this->dataBuffer[j % 15000];
					}
					this->missingIds.erase(this->missingIds.begin());
				}
				else
				{
					this->dataBuffer[(this->startPosition + id - *(this->missingIds.begin()) - 1) % 15000] = data;
					this->missingIds.erase(iterator);
				}
				break;
			}
		}
		if (!found) delete data;
	}

	/*
	Zašleme potvrzení o pøijatých datagramech, pøípadnì pošleme id datagramù,
	které nedorazily.
	*/
	if (this->expectedId - this->confirmedId > 3750) // 3750 taky ovlivnìna latencí; pøidat další podmínku s èasem
	{
		sendState(false);
	}

	LeaveCriticalSection(&this->CS_sendState);
	

	/*
	Pokoušel jsem se o efektivnìjší zasílání zpráv

	if (this->missingIds.empty())
	{
		if (this->expectedId - this->confirmedId > 3750) // 3750 taky ovlivnìna latencí; pøidat další podmínku s èasem
		{
			DatagramData data(getDestinationChannelNumber(), 4);
			data.set(this->expectedId); // abych se vyhl 0 - 1, tak to bude bez -1
			this->confirmationDatagram.data = data.getData();
			this->confirmationDatagram.length = data.getSize();
		}
		getConnection()->getCommunication()->sendDatagram(&this->confirmationDatagram);
		this->confirmedId = this->expectedId - 1;
	}
	else if (this->expectedId - *(this->missingIds.begin()) > 2000)
	{
		std::list<int>::
		RequestForResending* requestForResending = new RequestForResending;
		requestForResending->id = *(this->missingIds.begin());
		requestForResending->time = Communication::timeInMiliseconds();
		this->requestsForResending.push_back(requestForResending);

		int count = 0;
		for (auto iterator = this->missingIds.begin(); iterator != this->missingIds.end(); iterator++)
		{
			if (this->expectedId - *iterator < 1000) break;
			count++;
		}
		int j = 0;
		DatagramData* data;
		for (auto iterator = this->missingIds.begin(); iterator != this->missingIds.end(); iterator++)
		{
			if (j % 359 == 0)
			{
				data = new DatagramData(getDestinationChannelNumber(), 4 + 4 * (count - j >= 359 ? 359 : (count - j)));
				if (j == 0) data->set(*iterator); // abych se vyhl 0 - 1, tak to bude bez -1
			}

			data->set(*iterator);

			if (j % 359 == 358)
			{
				this->confirmationDatagram.data = data->getData();
				this->confirmationDatagram.length = data->getSize();
				getConnection()->getCommunication()->sendDatagram(&this->confirmationDatagram);
				delete data;
				delete this->confirmationDatagram.data;
			}
			j++;
			if (j >= count) break;
		}
	}
	*/
}



void ReceivingMassChannel::sendState(bool noDatagramLongTime)
{
	if (this->missingIds.empty())
	{
		DisposableDataBuilder data(4 + (noDatagramLongTime ? 1 : 0), getDestinationChannelNumber());
		data.set(this->expectedId); // abych se vyhl 0 - 1, tak to bude bez -1
		if (noDatagramLongTime) data.set((char) 0);
		if (noDatagramLongTime) std::cout << "je prazdne: " << data.getLength() << std::endl;
		getConnection()->getCommunication()->send(getDestinationAddress(), data.getData());
	}
	else
	{
		int count = 0;
		for (auto iterator = this->missingIds.begin(); iterator != this->missingIds.end(); iterator++)
		{
			if (this->expectedId - *iterator < 1000 && !noDatagramLongTime) break;
			count++;
		}
		int j = 0;
		if (noDatagramLongTime) std::cout << "uz dlouho nic: " << count << std::endl;
		DisposableDataBuilder* data = NULL;
		for (auto iterator = this->missingIds.begin(); iterator != this->missingIds.end(); iterator++)
		{
			if (j % 359 == 0)
			{
				data = new DisposableDataBuilder(4 + 4 * (count - j >= 359 ? 359 : (count - j)), getDestinationChannelNumber());
				if (j == 0) data->set(*this->missingIds.begin()); // abych se vyhl 0 - 1, tak to bude bez -1
			}

			if (count > 0) data->set(*iterator);

			if (j % 359 == 358 || j + 1 >= count)
			{
				getConnection()->getCommunication()->send(getDestinationAddress(), data->getData());
				delete data;
			}
			j++;
			if (j >= count) break;
		}
	}
	if (this->missingIds.empty()) this->confirmedId = this->expectedId;
	else this->confirmedId = *this->missingIds.begin();
}



// Spustí run
void ReceivingMassChannel::start(void* parameter)
{
	((ReceivingMassChannel*) parameter)->run();
}



// Èinnost vlánka
void ReceivingMassChannel::run()
{
	while (!this->isInterrupted)
	{
		if (Communication::timeInMiliseconds() - this->commingOfLastDatagramTime > 1000) // 1000 by mìla být vypoèítána z latence
		{
			if (TryEnterCriticalSection(&this->CS_sendState))
			{
				sendState(true);
				LeaveCriticalSection(&this->CS_sendState);
			}

		}

		AcquireSRWLockExclusive(&this->LOCK_thread);
		if (!this->isInterrupted) SleepConditionVariableSRW(&this->CV_thread, &this->LOCK_thread, 1000, 0);
		ReleaseSRWLockExclusive(&this->LOCK_thread);
	}
}