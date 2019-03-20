#include "filecopy.hpp"
#include "file.hpp"
#include "ioexception.hpp"
#include "defer.hpp"
#include <fcntl.h>
#include <copyfile.h>
#include <sys/stat.h>
#include <sys/attr.h>

/// Replaces `to` with an APFS clone of `from`
/// @warning Not threadsafe for multiple files in the same folder
void apfsdedup::copyFile(const boost::filesystem::path& from, const boost::filesystem::path& to) {
	// Note: While the tmpFile could be easily made threadsafe, there's still an issue of the second thread reading the wrong mtime from the parent folder for the parent folder mtime preservation code.
	boost::filesystem::path parent = to.parent_path();
	boost::filesystem::path tmpFile = parent / ".apfsdedup_tmp";

	// Save containing folder metadata to preserve its modification time
	struct stat parentInfo;
	if (stat((parent/".").c_str(), &parentInfo) == -1) {
		throwIOException(std::string{"Failed to stat "} + parent.string());
	}

	if (copyfile(from.c_str(), tmpFile.c_str(), NULL, COPYFILE_DATA | COPYFILE_CLONE_FORCE) < 0) {
		throwIOException(std::string{"Failed to copy "} + from.string() + " to " + tmpFile.string());
	}

	auto fixParentTime = defer([&parentInfo, &parent]() {
		struct timespec parentTimes[2];
		parentTimes[0] = parentInfo.st_atimespec;
		parentTimes[1] = parentInfo.st_mtimespec;
		// If we fail to restore the folder times it's not a huge deal
		utimensat(AT_FDCWD, parent.c_str(), parentTimes, 0);
	});

	auto unlinkTmpFile = defer([&tmpFile]() {
		unlink(tmpFile.c_str());
	});
	// Scope for files to be autoclosed
	{
		// copyfile breaks when you try to use COPYFILE_STAT from a compressed file over an uncompressed one
		// copyfile also decompresses compressed files if you let it open them itself because it uses O_WRONLY.  Open the files for it with O_RDONLY
		file openTo(to.c_str(), O_RDONLY), openTmp(tmpFile.c_str(), O_RDONLY);
		if (fcopyfile(openTo.fd(), openTmp.fd(), 0, COPYFILE_ACL | COPYFILE_XATTR) < 0) {
			throwIOException(std::string{"Failed to copy metadata from "} + to.string() + " to " + tmpFile.string());
		}

		// Modified version of the code for copyfile_stat from https://opensource.apple.com/source/copyfile/copyfile-146.200.3/copyfile.c.auto.html

		struct stat fromStat, toStat;
		if (stat(from.c_str(), &fromStat) < 0) {
			throwIOException(std::string{"Failed to stat "} + from.string());
		}
		if (fstat(openTo.fd(), &toStat) < 0) {
			throwIOException(std::string{"Failed to stat "} + to.string());
		}
		// Use the compressed flag from the source file
		uint32_t flags = toStat.st_flags;
		flags &= ~UF_COMPRESSED;
		flags |= (fromStat.st_flags & UF_COMPRESSED);
		(void)fchflags(openTmp.fd(), flags);

		(void)fchown(openTmp.fd(), toStat.st_uid, toStat.st_gid);
		(void)fchmod(openTmp.fd(), toStat.st_mode & ~S_IFMT);

		struct attrlist attrlist;
		struct {
			/* Order of these structs matters for setattrlist. */
			struct timespec mod_time;
			struct timespec acc_time;
		} ma_times;

		memset(&attrlist, 0, sizeof(attrlist));
		attrlist.bitmapcount = ATTR_BIT_MAP_COUNT;
		attrlist.commonattr = ATTR_CMN_MODTIME | ATTR_CMN_ACCTIME;
		ma_times.mod_time = toStat.st_mtimespec;
		ma_times.acc_time = toStat.st_atimespec;
		(void)fsetattrlist(openTmp.fd(), &attrlist, &ma_times, sizeof(ma_times), 0);
	}
	// Sometimes copyfile breaks when you try to copy metadata from a file without filesystem compression to one with
	// That should be fixed, but let's make sure
	if (boost::filesystem::file_size(tmpFile) != boost::filesystem::file_size(to)) {
		errno = 0;
		throwIOException("Copy attempt broke something!");
	}
	if (rename(tmpFile.c_str(), to.c_str()) == -1) {
		throwIOException(std::string{"Failed to rename " + tmpFile.string() + " to " + to.string()});
	}

	unlinkTmpFile.cancel();
}
