#include "ReceivingConfirmatoryChannel.h"



ReceivingConfirmatoryChannel::ReceivingConfirmatoryChannel()//(Connection* connection, unsigned short sourceChannelNumber, unsigned short destinationChannelNumber): Channel(connection, sourceChannelNumber, destinationChannelNumber)
{
	// pozdìji isChanged pøenést do channelu at je to obecné
	//this->isChanged = isChanged;

	this->waitingTime = 1000;
	this->type = CONFIRMATORY_CHANNEL;
	//this->lastDatagramIdentification = -1;
	this->expectedId = 0;
}



ReceivingConfirmatoryChannel::~ReceivingConfirmatoryChannel()
{
	for (FragmentedData* fragmentedData : this->dataBuffer)
	{
		delete fragmentedData;
	}
}



bool ReceivingConfirmatoryChannel::getData(Data** data)
{
	return false;
}



void ReceivingConfirmatoryChannel::processReceivedData(Data* data)
{
	// Pošleme potvrzení o pøijetí
	char* bytes = new char[4];
	bytes[0] = getDestinationChannelNumber();
	bytes[1] = getDestinationChannelNumber() >> 8;
	bytes[2] = data->bytes[2];
	bytes[3] = data->bytes[3];
	Datagram confirmation(getDestinationAddress(), bytes, 4);
	getConnection()->getCommunication()->send(&confirmation);
	delete bytes;

	// Obalíme další strukturou
	FragmentedData* fragmentedData = new FragmentedData;
	fragmentedData->data = data;
	fragmentedData->isThisFirstFragment = data->bytes[3] >> 7;
	fragmentedData->isThisLastFragment = (data->bytes[3] & 0x7f) >> 6;
	data->bytes[3] = data->bytes[3] & 0x3f;

	// Zpracování
	unsigned short id = *((unsigned short*) (data->bytes + 2));
	int difference = getDifference(id);
	if (difference >= 0)
	{
		auto iterator = std::lower_bound(this->dataBuffer.begin(), this->dataBuffer.end(), id, [](FragmentedData* fragmentedData, unsigned short i)
		{
			return (*((unsigned short*) (fragmentedData->data->bytes + 2))) < i;
		});
		if (iterator == this->dataBuffer.end()) this->dataBuffer.push_back(fragmentedData);
		else if (*((unsigned short*) ((*iterator)->data->bytes + 2)) == *((unsigned short*) (data->bytes + 2))) // duplicitní datagram
		{
			delete fragmentedData;
			return;
		}
		else this->dataBuffer.insert(iterator, fragmentedData);
	}
	else
	{
		delete fragmentedData;
		return;
	}

	if (difference == 0)
	{
		this->expectedId = (this->expectedId + 1) & 0x3fff;

		while (!this->dataBuffer.empty())
		{
			if (!giveData()) break;
		}
	}




	/*
	unsigned short id = *((unsigned short*) (data->bytes + 2));

	// Pošleme potvrzení o pøijetí
	char* bytes = new char[4];
	bytes[0] = getDestinationChannelNumber();
	bytes[1] = getDestinationChannelNumber() >> 8;
	bytes[2] = id;
	bytes[3] = id >> 8;
	Datagram confirmation(getDestinationAddress(), bytes, 4);
	getConnection()->getCommunication()->send(&confirmation);

	// Zpracování
	int difference = getDifference(id);
	if (difference == 0)
	{
		giveData(data);

		if (!this->dataBuffer.empty())
		{
			for (auto iterator = this->dataBuffer.begin(); iterator != this->dataBuffer.end(); iterator++)
			{
				if ((*((unsigned short*) ((*iterator) + 2))) == this->expectedId)
				{
					giveData(*iterator);
				}
				else break;
			}
		}
	}
	else if (difference > 0)
	{
		bool added = false; // ZMÌNIT NA BINÁRNÍ VYHLEDÁVÁNÍ
		for (auto iterator = this->dataBuffer.rbegin(); iterator != this->dataBuffer.rend(); iterator++)
		{
			if ((*((unsigned short*) ((*iterator) + 2))) < ((id + 1) % (USHRT_MAX + 1)))
			{
				this->dataBuffer.emplace(iterator, data);
				added = true;
			}
		}
		if (!added) this->dataBuffer.push_back(data);
	}
	else delete data;

	*/

	/*
	unsigned short difference = datagramIdentification - this->lastDatagramIdentification;
	if (difference < 0) difference += USHRT_MAX;

	if (difference == 1 || difference + USHRT_MAX == 1)
	{
		AcknowledgedDataParser acknowledgedDataParser(data);
		processReceivedData(&acknowledgedDataParser);
		delete data;
		this->lastDatagramIdentification = datagramIdentification;

		bool found;
		do
		{
			found = false;
			for (Data* d : this->receivedData)
			{
				datagramIdentification = *((unsigned short*) data->bytes);
				difference = datagramIdentification - this->lastDatagramIdentification;
				if (difference == 1 || difference + USHRT_MAX == 1)
				{
					acknowledgedDatagramData = new AcknowledgedDatagramData(datagram);
					processReceivedData(acknowledgedDatagramData);
					delete acknowledgedDatagramData;
					this->lastDatagramIdentification = datagramIdentification;
					found = true;
					break;
				}
			}
		} while (found);
	}
	else if (difference > 1) // ještì promyslet pøeteèení
	{
		this->receivedDatagrams.push_back(datagram);
	}
	*/

	// Odeslat potvrzení o pøijetí, lepší bude pøímé odeslání
	//processReceivedData(new DatagramData(datagram->data + 4, datagram->length - 4));
	// Smazat datagram !!!
}



/*
Vrací 0 pro oèekávaný datagram.
Vrací kladnou hodnotu pro datagram, který je napøed.
Vrací zápornou hodnotu pro datagram, který zpoždìný.
*/
int ReceivingConfirmatoryChannel::getDifference(unsigned short id)
{
	int difference1 = id - this->expectedId;
	if (difference1 == 0) return 0;

	int difference2;
	if (difference1 < 0) difference2 = difference1 + 0x3fff + 1;
	else difference2 = difference1 - (0x3fff + 1);

	if (abs(difference1) > abs(difference2)) return difference2;
	else return difference1;
}



bool ReceivingConfirmatoryChannel::giveData()
{
	bool isFragmentComplete = false;
	auto iterator = this->dataBuffer.begin();
	unsigned short j = *((unsigned short*) ((*iterator)->data->bytes + 2));

	for (iterator; iterator != this->dataBuffer.end(); iterator++)
	{
		if (*((unsigned short*) ((*iterator)->data->bytes + 2)) != j) return false; // našel díru
		else if ((*iterator)->isThisLastFragment)
		{
			isFragmentComplete = true;
			break;
		}
		else j = (j + 1) & 0x3fff; // držíme j v povolených hodnotách
	}
	if (!isFragmentComplete) return false;

	int difference = std::distance(this->dataBuffer.begin(), iterator);
	Data* wholeData;
	if (difference == 0)
	{
		wholeData = (*iterator)->data;
		(*iterator)->data = NULL;
		delete *iterator;
		this->dataBuffer.erase(iterator);
	}
	else
	{
		int length = difference * (Communication::DATAGRAM_SIZE - 4) + (*iterator)->data->length;
		wholeData = new Data(length);
		int offset = 4;
		for (int i = 0; i <= difference; i++)
		{
			memcpy(wholeData->bytes + offset, this->dataBuffer[i]->data->bytes + 4, this->dataBuffer[i]->data->length - 4);
			offset += this->dataBuffer[i]->data->length - 4;
			delete this->dataBuffer[i];
		}
		this->dataBuffer.erase(this->dataBuffer.begin(), ++iterator);
	}

	ConfirmatoryDataParser confirmatoryDataParser(wholeData);
	processReceivedData(&confirmatoryDataParser);
	delete wholeData;
}