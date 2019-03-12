#pragma once

#include "ioexception.hpp"
#include <fcntl.h>
#include <unistd.h>

namespace apfsdedup {

class file {
	int _fd;
	const char *_path;

public:
	inline int fd() const {
		return _fd;
	}

	inline const char *path() const {
		return _path;
	}

	inline file(const char *path, int mode): _path(path) {
		_fd = open(path, mode);
		if (_fd == -1) {
			throwIOException(std::string{"Failed to open "} + path);
		}
	}

	file(const file&) = delete;
	inline file(file&& other): _fd(other._fd), _path(other._path) {
		other._fd = -1;
	}

	inline ~file() {
		if (_fd != -1) {
			close(_fd);
		}
	}

	[[ nodiscard ]]
	inline ssize_t read(void *buffer, size_t size) {
		ssize_t out = ::read(_fd, buffer, size);
		if (out == -1) {
			throwIOException(std::string{"Failed to read "} + _path);
		}
		return out;
	}
};

}
