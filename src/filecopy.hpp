#pragma once

#include <boost/filesystem.hpp>

namespace apfsdedup {

void copyFile(const boost::filesystem::path& from, const boost::filesystem::path& to);
void listxattrs(const char *path);

}
