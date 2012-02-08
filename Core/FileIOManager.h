/*
<copyright>
	This file contains proprietary software owned by Motorola Mobility, Inc.
	No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.
	(c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
</copyright>
*/
#ifndef __FileIOManager__
#define __FileIOManager__

#include <deque>
#include <vector>
#include <string>

#ifdef  _DEBUG	
#define READ_DIR_TIMEOUT_SECONDS 300
#else
#define READ_DIR_TIMEOUT_SECONDS 10
#endif

namespace NinjaFileIO
{
	struct FileSystemNode 
	{
		FileSystemNode();

		enum FileNodeTypes {
			unknown = 0,
			file,
			directory
		} type;
		std::wstring name;
		std::wstring uri;
		unsigned long long int filesize; // for files only
		unsigned long long int creationDate; // ms since 1/1/1970
		unsigned long long int modifiedDate; // ms since 1/1/1970, 
		bool isWritable;
	};
	typedef std::deque<FileSystemNode> FileSystemNodeList;

	enum DirectoryContentTypes 
	{
		allDirectoryContents = 0,
		filesOnly,
		directoriesOnly
	};

	// core abstract interface for OS specific file operations
	class FileIOManager
	{
	public:
		FileIOManager(void);
		virtual ~FileIOManager(void);

		// File APIs
		virtual bool FileExists(const std::wstring &filename);
		virtual bool FileIsWritable(const std::wstring &filename) = 0;
		virtual bool GetFileTimes(const std::wstring &filename, unsigned long long &createdTime, unsigned long long &modifiedTime) = 0;
        virtual bool GetFileSize(const std::wstring &filename, unsigned long long &fileSize) = 0;
		virtual bool CreateNewFile(const std::wstring &filename);
		virtual bool DeleteFile(const std::wstring &filename) = 0;
		virtual bool SaveFile(const std::wstring &filename, char *fileContents, unsigned int contentLength); 
		virtual bool CopyFile(const std::wstring &srcFilename, const std::wstring &destFilename, bool overwriteExistingFile) = 0;
		virtual bool RenameFile(const std::wstring &filename, const std::wstring &newFilename);
		virtual bool ReadFile(const std::wstring &filename, char **fileContents, unsigned int &contentLength); // caller must free fileContents

		// Directory APIs
		virtual bool DirectoryExists(const std::wstring &path);
		virtual bool GetDirectoryTimes(const std::wstring &path, unsigned long long &createdTime, unsigned long long &modifiedTime) = 0;
		virtual bool DirectoryIsEmpty(const std::wstring &path) = 0;
		virtual bool CreateNewDirectory(const std::wstring &path);
		virtual bool DeleteDirectory(const std::wstring &path) = 0;
		virtual bool CopyDirectory(const std::wstring &srcPath, const std::wstring &destPath, bool overwriteExistingFile) = 0;
		virtual bool MoveDirectory(const std::wstring &path, const std::wstring &newPath) = 0;
		virtual bool ReadDirectory(const std::wstring &path, DirectoryContentTypes types, const std::wstring &filterList, FileSystemNodeList &dirContents,
			time_t requestStartTime, bool &requestTimedOut) = 0; 
		
	protected:
#ifdef _MAC
		NSFileManager *m_nsFMgr;
#endif
	};
}

#endif //__FileIOManager__
