#pragma once
#include "Data.h"



typedef struct FragmentedData
{
	Data* data;
	bool isThisFirstFragment;
	bool isThisLastFragment;

	~FragmentedData()
	{
		if (data != NULL) delete data;
	}
} FragmentedData;