#pragma once
#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H



template<typename T> class DynamicArray
{
public:
	DynamicArray(unsigned short step);
	virtual ~DynamicArray();

	T operator[](unsigned int index);
	void write(unsigned int index, T value);
	void erase(unsigned int index);
	void erase(unsigned int index, bool resize);
	unsigned int size();

protected:
	T* array;
	unsigned int _size;
	unsigned short step;
};

#include "DynamicArray.cpp"
#endif
