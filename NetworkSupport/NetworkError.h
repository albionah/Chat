#pragma once



typedef struct NetworkError
{
	NetworkError(int type, const char* description) : type(type), description((char*) description) {};
	NetworkError(int type, std::string& description) : type(type)
	{
		this->description = new char[description.size() + 1];
		memcpy(this->description, description.c_str(), description.size() + 1);
	};
	~NetworkError()
	{
		delete[] description;
	}

	int type;
	char* description;
	

	enum Types
	{
		INITIALIZATION
	};
} NetworkError;