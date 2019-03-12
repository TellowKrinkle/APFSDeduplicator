cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(APFSDeduplicator VERSION 0.8.2 LANGUAGES CXX)

include(GNUInstallDirs)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(apfs-dedup
	main.cpp
	sha.cpp
	filecopy.cpp
	ioexception.cpp

	lock.hpp
	sha.hpp
	filecopy.hpp
	file.hpp
	ioexception.hpp
	defer.hpp)

find_package(Boost 1.56 REQUIRED COMPONENTS filesystem)
find_package(Threads REQUIRED)

target_compile_features(apfs-dedup
	PRIVATE
		cxx_std_14)

target_link_libraries(apfs-dedup
	Threads::Threads
	Boost::filesystem)

install(TARGETS apfs-dedup RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
