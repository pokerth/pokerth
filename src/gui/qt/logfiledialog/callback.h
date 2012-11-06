#ifndef CALLBACK_H
#define CALLBACK_H

#include <iostream>

class Callback
{
public:
	Callback();

	size_t writeCallback(char* buf, size_t size, size_t nmemb, void* up);

	std::string getData() { return data; }

private:

	std::string data;

};

#endif // CALLBACK_H
