#include "DynamicArray.h"
#ifndef DYNAMIC_ARRAY_CPP
#define DYNAMIC_ARRAY_CPP



template <class T> DynamicArray<T>::DynamicArray(unsigned short step)
{
	if (step < 1) step = 1;
	this->array = (T*) malloc(step * sizeof(T));
	memset(this->array, 0, sizeof(T) * step);
	this->_size = step;
	this->step = step;
}



template <class T> DynamicArray<T>::~DynamicArray()
{
	delete[] this->array;
}



template <class T> T DynamicArray<T>::operator[](unsigned int index)
{
	if (index >= this->_size)
	{
		return NULL;
	}
	return this->array[index];
}



template <typename T> void DynamicArray<T>::write(unsigned int index, T value)
{
	if (index >= this->_size)
	{
		this->array = (T*) realloc(this->array, (index + this->step) * sizeof(T));
		memset(this->array + this->_size, 0, sizeof(T) * (index - this->_size + this->step));
		this->_size = index + this->step;
	}
	this->array[index] = value;
}



template <typename T> void DynamicArray<T>::erase(unsigned int index)
{
	if (index >= this->_size || this->array[index] == NULL) return;
	this->array[index] = NULL;

	for (unsigned int j = index; j < this->_size; j++)
	{
		if (this->array[index] != NULL) return;
	}

	unsigned int minIndex = index;
	for (__int64 j = index; j >= 0; j--)
	{
		minIndex = j;
		if (this->array[j] != NULL) break;
	}

	if (minIndex < this->_size - this->step)
	{
		this->array = (T*) realloc(this->array, sizeof(T) * (minIndex + this->step));
		this->_size = minIndex + this->step;
	}
}



template <typename T> void DynamicArray<T>::erase(unsigned int index, bool resize)
{// Možná ještì ovìøit jestli to je v rozsahu
	if (resize) erase(index);
	else if (index >= this->_size || this->array[index] == NULL) this->array[index] = NULL;
}



template <class T> unsigned int DynamicArray<T>::size()
{
	return this->_size;
}



#endif