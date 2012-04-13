/*
 <copyright>
 This file contains proprietary software owned by Motorola Mobility, Inc.
 No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.
 (c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
 </copyright>
 */
#ifndef __MacFileIOManager__
#define __MacFileIOManager__

#include "../Core/FileIOManager.h"

namespace NinjaFileIO
{
	class MacFileIOManager : public FileIOManager
	{
	public:
		MacFileIOManager();
		virtual ~MacFileIOManager();
		
		bool FileIsWritable(const std::wstring &filename);
		
		bool CopyFile(const std::wstring &srcFilename, const std::wstring &destFilename, bool overwriteExistingFile);
		bool DeleteFile(const std::wstring &filename);
		bool GetFileTimes(const std::wstring &filename, unsigned long long &createdTime, unsigned long long &modifiedTime);
        bool GetFileSize(const std::wstring &filename, unsigned long long &fileSize);
		
		bool DeleteDirectory(const std::wstring &path);
		
		bool CopyDirectory(const std::wstring &srcPath, const std::wstring &destPath, bool overwriteExistingDirectory);
		bool MoveDirectory(const std::wstring &path, const std::wstring &newPath);
		bool ReadDirectory(const std::wstring &path, DirectoryContentTypes types, const std::wstring &filterList, FileSystemNodeList &dirContents, 
						   time_t requestStartTime, bool &requestTimedOut); 
		bool DirectoryIsEmpty(const std::wstring &path);
		bool GetDirectoryTimes(const std::wstring &path, unsigned long long &createdTime, unsigned long long &modifiedTime);
			
        bool ReadTextFromURL(const std::wstring &url, char **fileContents, unsigned int &contentLength); // caller must free fileContents
        bool ReadBinaryFromURL(const std::wstring &url, unsigned char **fileContents, unsigned int &contentLength); // caller must free fileContents

	private:
//		void FileOpenHelper(const std::wstring &initialPath, bool directoryMode, const std::wstring &title, std::vector<std::wstring> &uRIsOut);
	};
}
#endif // __MacFileIOManager__

