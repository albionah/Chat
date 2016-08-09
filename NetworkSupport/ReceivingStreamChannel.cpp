#include "ReceivingStreamChannel.h"



ReceivingStreamChannel::ReceivingStreamChannel() //(Connection* connection, unsigned short sourceChannelNumber, unsigned short destinationChannelNumber) : Channel(connection, sourceChannelNumber, destinationChannelNumber)
{
	// pozd�ji isChanged p�en�st do channelu at je to obecn�
	//this->isChanged = isChanged;

	//this->waitingTime = 1000;
	this->type = STREAM_CHANNEL;
	//this->lastDatagramIdentification = -1;
	this->expectedId = 0;
}



ReceivingStreamChannel::~ReceivingStreamChannel()
{
	for (FragmentedData* d : this->dataBuffer)
	{
		delete d;
	}
}



void ReceivingStreamChannel::processReceivedData(Data* data)
{
	FragmentedData* fragmentedData = new FragmentedData;
	fragmentedData->data = data;
	fragmentedData->isThisFirstFragment = data->bytes[3] >> 7;
	fragmentedData->isThisLastFragment = (data->bytes[3] & 0x7f) >> 6;
	data->bytes[3] = data->bytes[3] & 0x3f;
	unsigned short id = *((unsigned short*) (data->bytes + 2));

	int difference = getDifference(id);

	if (difference >= 0)
	{
		auto iterator = std::lower_bound(this->dataBuffer.begin(), this->dataBuffer.end(), id,
			[](const FragmentedData* d, unsigned short j)
			{
				return (*((unsigned short*) (d->data->bytes + 2)) < j);
			});

		if (iterator == this->dataBuffer.end()) // fronta je pr�zdn�
		{
			this->dataBuffer.push_back(fragmentedData);
			iterator = --this->dataBuffer.end();
		}
		else if (*((unsigned short*) ((*iterator)->data->bytes + 2)) == *((unsigned short*) (data->bytes + 2))) // p�i�el duplicitn� datagram
		{
			delete fragmentedData;
			return;
		}
		else
		{
			iterator = this->dataBuffer.insert(iterator, fragmentedData);
		}

		bool isFragmentComplete = false;
		unsigned short j = id;
		std::vector<FragmentedData*>::reverse_iterator startBoundRIt(iterator);
		startBoundRIt--;
		for (startBoundRIt; startBoundRIt != this->dataBuffer.rend(); startBoundRIt++) // hled�me za��tek, jdeme doleva
		{
			if (*((unsigned short*) ((*startBoundRIt)->data->bytes + 2)) != j) return; // na�el d�ru
			else if ((*startBoundRIt)->isThisFirstFragment)
			{
				isFragmentComplete = true;
				break;
			}
			else j = (j - 1) & 0x3fff; // dr��me j v povolen�ch hodnot�ch
		}
		if (!isFragmentComplete) return;

		isFragmentComplete = false;
		j = id;
		std::vector<FragmentedData*>::iterator endBoundIt(iterator);
		for (endBoundIt; endBoundIt != this->dataBuffer.end(); endBoundIt++) // hled�me konec, jdeme doprava
		{
			if (*((unsigned short*) ((*endBoundIt)->data->bytes + 2)) != j) return; // na�el d�ru
			else if ((*endBoundIt)->isThisLastFragment)
			{
				isFragmentComplete = true;
				break;
			}
			else j = (j + 1) & 0x3fff; // dr��me j v povolen�ch hodnot�ch
		}
		if (!isFragmentComplete) return;
		/*
		M�me iter�tor po��tku kompletn�ho fragmentu.
		D�le mus�me vytvo�it Data ze v�ech jeho ��st�.
		Pot� mus�me odstranit v�echny p�edchoz� data
		v bufferu.
		*/
		if (--(startBoundRIt.base()) != this->dataBuffer.begin()) std::cout << "zahazuju nedokoncene datagramy" << std::endl;

		int difference = std::distance(--(startBoundRIt.base()), endBoundIt);
		int length = difference * (Communication::DATAGRAM_SIZE - 4) + (*endBoundIt)->data->length - 4;
		Data* wholeData = new Data(length);
		int offset = 0;
		std::vector<FragmentedData*>::iterator it(--(startBoundRIt.base()));
		for (int i = 0; i <= difference; i++)
		{
			memcpy(wholeData->bytes + offset, (*it)->data->bytes + 4, (*it)->data->length - 4);
			offset += (*it)->data->length - 4;
			it++;
		}
		this->expectedId = (*((unsigned short*) ((*(--it))->data->bytes + 2)) + 1) & 0x3fff;
		it++;
		for (auto dIt = this->dataBuffer.begin(); dIt != it; dIt++)
		{
			delete (*dIt);
		}
		this->dataBuffer.erase(this->dataBuffer.begin(), it);

		giveData(wholeData);
	}
	else
	{
		std::cout << "datagram je stary" << std::endl;
		delete fragmentedData;
	}
}



/*
  Vrac� 0 pro o�ek�van� datagram.
  Vrac� kladnou hodnotu pro datagram, kter� je nap�ed.
  Vrac� z�pornou hodnotu pro datagram, kter� zpo�d�n�.
*/
int ReceivingStreamChannel::getDifference(unsigned short id)
{
	int difference1 = id - this->expectedId;
	if (difference1 == 0) return 0;

	int difference2;
	if (difference1 < 0) difference2 = difference1 + 0x3fff + 1;
	else difference2 = difference1 - (0x3fff + 1);

	if (abs(difference1) > abs(difference2)) return difference2;
	else return difference1;
}