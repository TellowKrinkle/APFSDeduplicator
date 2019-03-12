#include "ioexception.hpp"
#include <sys/errno.h>

void apfsdedup::throwIOException(std::string message) {
	char buffer[128];
	strerror_r(errno, buffer, sizeof(buffer));
	throw IOException(message + ": " + buffer);
}

const char *apfsdedup::IOException::what() const noexcept {
	return message.c_str();
}
