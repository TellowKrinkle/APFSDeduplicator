# APFSDeduplicator
A file deduplicator for APFS using clone files

Properly retains the original file's metadata on cloning

## Compiling
Requires Boost Filesystem

Compile using cmake
```bash
mkdir build
cd build
cmake ../src
make
```

## Usage
APFSDeduplicator doesn't save a list of file hashes to disk, so make sure to run it on all the files/folders at once.

Pass all files and folders you want scanned to the executable.  Optionally, pass `-p` as the first argument to print the duplicates it finds instead of deduplicating.
