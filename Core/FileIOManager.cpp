/*
<copyright>
	This file contains proprietary software owned by Motorola Mobility, Inc.
	No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.
	(c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
</copyright>
*/
#include "FileIOManager.h"

#ifdef _WINDOWS
#include <io.h>
#include <direct.h>
#else
#include "Utils.h"
#endif

namespace NinjaFileIO
{
	FileSystemNode::FileSystemNode()
	{
		type = unknown;	
		filesize = 0; 
		creationDate = 0;
		modifiedDate = 0;
		isWritable = false;
	}

	FileIOManager::FileIOManager(void)
	{
#ifdef _MAC
		m_nsFMgr = [[NSFileManager alloc] init];
#endif
	}

	FileIOManager::~FileIOManager(void)
	{
#ifdef _MAC
		[m_nsFMgr release];
#endif
	}

	// File APIs
	bool FileIOManager::FileExists(const std::wstring &filename)
	{
		bool ret = false;

		try
		{
			if(filename.length())
			{
#ifdef _WINDOWS
				if (_waccess_s(filename.c_str(), 0) == 0 )
					ret = true;
#else
				NSString *fs = NinjaUtilities::WStringToNSString(filename);
				
				if([m_nsFMgr fileExistsAtPath:fs] == YES)
					ret = true;
				
				[fs release];
#endif
			}
		}
		catch(...)
		{
		}

		return ret;
	}

	bool FileIOManager::CreateNewFile(const std::wstring &filename, char *fileContents /*= NULL*/, unsigned int contentLength /*= 0*/)
	{
		bool ret = false;

		try
		{
			if(filename.length() && !FileExists(filename))
			{
#ifdef _WINDOWS
				FILE *stream = NULL;
				if(_wfopen_s(&stream, filename.c_str(), L"a") != 0)
					stream = NULL;
				if(stream != NULL)
				{
					// the file has been created

                    if(fileContents && contentLength)
                    {
                        if(fwrite(fileContents, sizeof(char), contentLength, stream) == contentLength)
                            ret = true;
                    }
                    else
                        ret = true;

					fclose(stream);					
				}
#else // MAC
                NSData *data = nil;
                if(fileContents && contentLength)
                    NSData *data = [NSData dataWithBytes: fileContents length:contentLength];

				NSString *filePath = NinjaUtilities::WStringToNSString(filename);
				if ([m_nsFMgr createFileAtPath:filePath contents:data attributes:nil] == YES) 
					ret = true;

				[filePath release];
#endif
			}
		}
		catch(...)
		{
		}

		return ret;
	}

	bool FileIOManager::SaveFile(const std::wstring &filename, char *fileContents, unsigned int contentLength)
	{
		bool ret = false;

		try
		{
			if(filename.length() && fileContents && contentLength)
			{
#ifdef _WINDOWS
				FILE *stream = NULL;
				if(_wfopen_s(&stream, filename.c_str(), L"wb") != 0)
					stream = NULL;
				if(stream != NULL)
				{
					if(fwrite(fileContents, sizeof(char), contentLength, stream) == contentLength)
						ret = true;
					
					fclose(stream);
				}
#else	// MAC
				NSString *filePath = NinjaUtilities::WStringToNSString(filename);
				NSData *data = [NSData dataWithBytes: fileContents length:contentLength];
				if([data writeToFile:filePath atomically:YES] == YES)
					ret = true;
				[filePath release];
#endif
			}
		}
		catch(...)
		{
		}

		return ret;
	}

	bool FileIOManager::RenameFile(const std::wstring &filename, const std::wstring &newFilename)
	{
		bool ret = false;

		try
		{
			if(filename.length() && newFilename.length() && FileExists(filename) && !FileExists(newFilename))
			{
#ifdef _WINDOWS
				if(_wrename(filename.c_str(), newFilename.c_str()) == 0)
					ret = true;
#else
				NSString *srcPath = NinjaUtilities::WStringToNSString(filename), *destPath = NinjaUtilities::WStringToNSString(newFilename);
				if([m_nsFMgr moveItemAtPath:srcPath toPath:destPath error:nil] == YES)
					ret = true;
				
				[srcPath release];
				[destPath release];
#endif
			}
		}
		catch(...)
		{
		}

		return ret;
	}

	bool FileIOManager::ReadFile(const std::wstring &filename, char **fileContents, unsigned int &contentLength)
	{
		bool ret = false;

		char *returnBuffer = NULL;
		try
		{
			if(filename.length() && FileExists(filename) && fileContents)
			{
				FILE *stream = NULL;
#ifdef _WINDOWS
				if(_wfopen_s(&stream, filename.c_str(), L"rb") != 0)
					stream = NULL;
#else // MAC
				NSString *nsFname = NinjaUtilities::WStringToNSString(filename);
				stream = fopen([nsFname UTF8String], "r");
#endif
				if(stream != NULL)
				{
					// determine the file size
					fseek(stream , 0, SEEK_END);
					long fileSize = ftell(stream), bufSize = fileSize + 1; // we add one to ensure a null terminator in the buffer
					fseek(stream , 0, SEEK_SET);

					if(fileSize > 0)
					{
						char *returnBuffer = (char*)calloc(bufSize, sizeof(char)); 
						memset(returnBuffer, 0, sizeof(char)*bufSize);
						if(returnBuffer)
						{
							size_t bytesRead = 0, totBytes = 0;
							
							do 
							{
								bytesRead = fread(&returnBuffer[bytesRead], sizeof(char), bufSize, stream);	
								totBytes += bytesRead;
							} while (bytesRead > 0 && bytesRead != (size_t)feof);

							if(totBytes > 0 && totBytes == fileSize)
							{
								*fileContents = returnBuffer;
								contentLength = (unsigned int)fileSize;
								ret = true;
							}
						}
					}
					else if(fileSize == 0) // the file is empty
					{
						*fileContents = NULL;
						contentLength = 0;
						ret = true;
					}

					fclose(stream);
				}
				
#ifdef _MAC
				[nsFname release];
#endif
			}
		}
		catch(...)
		{
			if(returnBuffer)
				free(returnBuffer);
		}

		return ret;
	}

	// Directory APIs
	bool FileIOManager::DirectoryExists(const std::wstring &path)
	{
		bool ret = false;

		try
		{
			if(path.length())
			{
#ifdef _WINDOWS
				if (_waccess_s(path.c_str(), 0) == 0 )
					ret = true;
#else
				NSString *dir = NinjaUtilities::WStringToNSString(path);
				
				BOOL isDir = NO;
				if([m_nsFMgr fileExistsAtPath:dir isDirectory:&isDir] && isDir)
					ret = true;
				
				[dir release];				
#endif
			}
		}
		catch(...)
		{
		}

		return ret;
	}

	// NOTE - this currently will not create multiple directories at once. i.e. if c:\foo exists and is empty
	//        this will not create bar and subbar if you call CreateDirectory() passing "c:\foo\bar\subbar".
	//        You must first create the "bar" directory.
	bool FileIOManager::CreateNewDirectory(const std::wstring &path)
	{
		bool ret = false;

		try
		{
			if(path.length() && !DirectoryExists(path))
			{
#ifdef _WINDOWS
				if (_wmkdir(path.c_str()) == 0 )
					ret = true;
#else
				NSString *dir = NinjaUtilities::WStringToNSString(path);
				
				if([m_nsFMgr createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:nil])
					ret = true;
				
				[dir release];				
#endif
			}
		}
		catch(...)
		{
		}

		return ret;
	}
}
