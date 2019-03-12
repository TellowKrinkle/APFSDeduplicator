#pragma once

#include <string>

namespace apfsdedup {

class IOException: public std::exception {
public:
	virtual const char *what() const noexcept override;

	const std::string message;

	inline IOException(std::string message): message(std::move(message)) {}
};

[[ noreturn ]]
void throwIOException(std::string message);

} // namespace apfsdedup
