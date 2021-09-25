//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2021 Ioan Chera
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#include "SafeOutFile.h"

#include <chrono>

enum
{
	RANDOM_PATH_ATTEMPTS = 16
};

// Limited set of ASCII
static const char skSafeAscii[] = "123456789(0)-_=qQwWeErRtTyYuUiIoOpPaAsSdDfFg"
	"GhHjJkKlLzZcCvVbBnNmM";

//
// Prepare the path now
//
SafeOutFile::SafeOutFile(const SString &path) : mPath(path)
{
	auto seed = std::chrono::system_clock::now().time_since_epoch().count();
	mRandom.seed(static_cast<std::mt19937::result_type>(seed));
}

//
// Open the file for writing
//
bool SafeOutFile::openForWriting()
{
	if(!makeValidRandomPath(mRandomPath))
		return false;

	SString randomPath = mRandomPath;
	close();
	mRandomPath = randomPath;

	mFile = fopen(mRandomPath.c_str(), "wb");
	if(!mFile)
		return false;

	return true;
}

//
// Commit the writing to the final file.
//
bool SafeOutFile::commit()
{
	if(!mFile)
		return false;
	// First, to be ultra-safe, make another temp path
	SString safeRandomPath;
	int i = 0;
	for(; i < RANDOM_PATH_ATTEMPTS; ++i)
	{
		if(!makeValidRandomPath(safeRandomPath))
			return false;
		if(!safeRandomPath.noCaseEqual(mRandomPath))
		{
			// also make sure it doesn't collide with ours
			break;
		}
	}
	if(i == RANDOM_PATH_ATTEMPTS)
		return false;

	// Now we need to close our work. Store the paths of interest in a variable
	SString finalPath = mPath;
	SString writtenPath = mRandomPath;
	if(mFile)
	{
		fclose(mFile);
		mFile = nullptr;	// we can close it now
	}
	// Rename the old file, if any, to a safe random path. It may fail if the
	// file doesn't exist
	bool overwriteOldFile = true;
	if(rename(finalPath.c_str(), safeRandomPath.c_str()))
	{
		if(errno != ENOENT)
			return false;
		overwriteOldFile = false;
	}
	if(rename(writtenPath.c_str(), finalPath.c_str()))
		return false;
	if(overwriteOldFile && remove(safeRandomPath.c_str()))
		return false;
	close();
	return true;
}

//
// Closes the file. WARNING: merely doing this will just remove the temp file
// and cancel everything. You need to commit first
//
void SafeOutFile::close()
{
	if(mFile)
	{
		fclose(mFile);
		remove(mRandomPath.c_str());	// hopefully it works
	}
	mFile = nullptr;
	mRandomPath.clear();
}

//
// Writes data to file
//
bool SafeOutFile::write(const void *data, size_t size) const
{
	if(!mFile)
		return false;
	return fwrite(data, 1, size, mFile) == size;
}

//
// Generate the random path
//
SString SafeOutFile::generateRandomPath() const
{
	return mPath + skSafeAscii[mRandom() % (sizeof(skSafeAscii) - 1)] +
		skSafeAscii[mRandom() % (sizeof(skSafeAscii) - 1)] +
		skSafeAscii[mRandom() % (sizeof(skSafeAscii) - 1)] +
		skSafeAscii[mRandom() % (sizeof(skSafeAscii) - 1)];
}

//
// Try to make a random path for writing
//
bool SafeOutFile::makeValidRandomPath(SString &path) const
{
	SString randomPath;
	int i = 0;
	for(; i < RANDOM_PATH_ATTEMPTS; ++i)
	{
		randomPath = generateRandomPath();
		FILE *checkExisting = fopen(randomPath.c_str(), "rb");
		if(!checkExisting)
			break;
		fclose(checkExisting);
	}
	if(i == RANDOM_PATH_ATTEMPTS)
		return false;
	path = std::move(randomPath);
	return true;
}