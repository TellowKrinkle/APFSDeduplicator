#include <boost/filesystem.hpp>
#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <thread>
#include "lock.hpp"
#include "sha.hpp"
#include "filecopy.hpp"
#include "ioexception.hpp"
#include <sys/fcntl.h>

using namespace apfsdedup;

static std::atomic<boost::uintmax_t> duplicatedBytes{0};
static UnfairLock printLock;
static bool action = false;

static void multithreadFunction(
	int argc,
	char const * const argv[],
	UnfairLock *iterLock,
	boost::filesystem::recursive_directory_iterator *iter,
	int *next,
	UnfairLock *mapLock,
	std::unordered_map<SHA256Hash, boost::filesystem::path> *map
) {
	bool done = false;
	while (true) {
		boost::filesystem::path path;
		iterLock->withLock([&path, &done, iter, next, argc, argv]() {
			while (true) {
				if (*iter != boost::filesystem::recursive_directory_iterator()) {
					auto entry = (**iter);
					(*iter)++;
					if (boost::filesystem::is_regular(entry)) {
						path = entry.path();
						return;
					}
				}
				else if (*next == argc) {
					done = true;
					return;
				}
				else {
					path = boost::filesystem::path(argv[*next]);
					(*next)++;
					if (boost::filesystem::is_directory(path)) {
						*iter = boost::filesystem::recursive_directory_iterator(path);
						continue;
					}
					else if (boost::filesystem::is_regular(path)) {
						return;
					}
				}
			}
		});
		if (done) { return; }
		SHA256Hash hash;
		try {
			hash = SHA256Hash::fromFile(path.c_str());
		}
		catch (IOException e) {
			printLock.withLock([&e, &path]() {
				std::cerr << "Failed to hash " << path << ": " << e.message << std::endl;
			});
			continue;
		}
		boost::filesystem::path other;
		mapLock->withLock([hash, &path, map, &other](){
			auto idx = map->find(hash);
			if (idx != map->end()) {
				other = idx->second;
			}
			else {
				(*map)[hash] = path;
			}
		});
		if (!other.empty()) {
			duplicatedBytes.fetch_add(boost::filesystem::file_size(other));
			try {
				if (action) {
					iterLock->withLock([&other, &path]() {
						copyFile(other.c_str(), path.c_str());
					});
				}
				else {
					printLock.withLock([&other, &path]() {
						std::cout << path.string() << " == " << other.string() << std::endl;
					});
				}
			}
			catch (IOException e) {
				printLock.withLock([&e, &other, &path]() {
					std::cerr << "Failed to copy " << other << " over " << path << ": " << e.message << std::endl;
				});
			}
		}
	}
}

#include <copyfile.h>

int main(int argc, char const * const argv[]) {
	int start = 1;
	if (argc == 1) {
		std::cout << "Usage: " << argv[0] << " [-p] path [path...]" << std::endl;
		std::cout << "-p: Print instead of actually deduplicating" << std::endl;
		return EXIT_FAILURE;
	}
	if (argc > 1 && strcmp(argv[1], "-p") == 0) {
		start = 2;
		action = false;
	}
	else {
		action = true;
	}
	std::unordered_map<SHA256Hash, boost::filesystem::path> map;
#if 0
	for (int i = start; i < argc; i++) {
		boost::filesystem::recursive_directory_iterator a(argv[i]);
		for (auto file : boost::filesystem::recursive_directory_iterator(argv[i])) {
			if (!boost::filesystem::is_regular(file)) { continue; }
			boost::filesystem::path path = file.path();
			SHA256Hash hash = SHA256Hash::fromFile(path.c_str());
			auto idx = map.find(hash);
			if (idx != map.end()) {
				duplicatedBytes.fetch_add(boost::filesystem::file_size(idx->second));
				if (action) {
					copyFile(idx->second, path);
				}
				else {
					std::cout << path.string() << " == " << idx->second.string() << std::endl;
				}
			}
			else {
				map[hash] = path;
			}
		}
	}
#else
	UnfairLock iterLock, mapLock;
	boost::filesystem::recursive_directory_iterator iter;
	int next = 1;

	std::vector<std::thread> threads;
	for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
		threads.emplace_back(multithreadFunction, argc, argv, &iterLock, &iter, &next, &mapLock, &map);
	}
	for (auto& thread : threads) {
		thread.join();
	}
#endif
	auto bytes = duplicatedBytes.load();
	std::cout << bytes << " bytes of duplicated files";
	if (bytes > 1024 * 1024 * 1024) {
		std::cout << " (" << std::setprecision(2) << std::fixed << (double)bytes / (1024 * 1024 * 1024) << " GB)" << std::endl;
	}
	else if (bytes > 1024 * 1024) {
		std::cout << " (" << std::setprecision(2) << std::fixed << (double)bytes / (1024 * 1024) << " MB)" << std::endl;
	}
	else if (bytes > 1024) {
		std::cout << " (" << std::setprecision(2) << std::fixed << (double)bytes / (1024) << " KB)" << std::endl;
	}
	else {
		std::cout << std::endl;
	}
}
