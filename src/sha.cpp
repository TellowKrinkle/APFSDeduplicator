#include "sha.hpp"
#include "file.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <iomanip>

using namespace apfsdedup;

#define BUFFER_SIZE (256 * 1024)

std::ostream& operator<<(std::ostream& os, SHA256Hash const& hash) {
	for (const uint8_t byte : hash.data) {
		os << std::setfill('0') << std::setw(2) << std::hex << (int)byte;
	}
	return os;
}

SHA256Hash SHA256Hash::fromFile(const char *path) {
	SHA256Hasher hasher;
	file inFile(path, O_RDONLY);

	uint8_t buffer[BUFFER_SIZE];
	int amtRead;
	while ((amtRead = inFile.read(buffer, sizeof(buffer)))) {
		hasher.add(buffer, amtRead);
	}
	
	return hasher.finalize();
}
