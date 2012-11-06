#include "callback.h"

Callback::Callback()
{

}


size_t
Callback::writeCallback(char* buf, size_t size, size_t nmemb, void* up)
{
	data = "";

	for (int c = 0; c<size*nmemb; c++)
	{
		data.push_back(buf[c]);
	}
	return size*nmemb;
}
