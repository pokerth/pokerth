#ifndef LOGUPLOADCALLBACK_H
#define LOGUPLOADCALLBACK_H

#include <iostream>

class LogUploadCallback
{
public:
	LogUploadCallback();

	size_t writeCallback(char* buf, size_t size, size_t nmemb, void* up);
	std::string getData() { return data; }

private:

	std::string data;

};

#endif // LOGUPLOADCALLBACK_H
