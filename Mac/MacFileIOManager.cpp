/*
 <copyright>
 This file contains proprietary software owned by Motorola Mobility, Inc.
 No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.
 (c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
 </copyright>
 */
#include "MacFileIOManager.h"
#include "../Core/Utils.h"

namespace NinjaFileIO
{
	MacFileIOManager::MacFileIOManager()
	{
	}
	
	MacFileIOManager::~MacFileIOManager()
	{
	}

	bool MacFileIOManager::FileIsWritable(const std::wstring &filename)
	{
		bool ret = false;
		
		try 
		{
			if(filename.length() && FileExists(filename))
			{
				NSString *path = NinjaUtilities::WStringToNSString(filename);

				ret = [m_nsFMgr isWritableFileAtPath:path] == YES;
			}
		}
		catch (...) 
		{
			
		}
		
		return ret;
	}
	
	bool MacFileIOManager::CopyFile(const std::wstring &srcFilename, const std::wstring &destFilename, bool overwriteExistingFile)
	{
		bool ret = false;
		
		try 
		{
			if(srcFilename.length() && FileExists(srcFilename) && destFilename.length())
			{
				NSString *srcPath = NinjaUtilities::WStringToNSString(srcFilename),
				*destPath = NinjaUtilities::WStringToNSString(destFilename);
				
				bool canProceed = true;
				if(overwriteExistingFile && FileExists(destFilename))
				{
					if([m_nsFMgr removeItemAtPath:destPath error:nil] == NO)
					{
						canProceed = false;
					}
				}
				
				if(canProceed && [m_nsFMgr copyItemAtPath:srcPath toPath:destPath error:nil] == YES)
				{
					ret = true;
				}
			}
		}
		catch (...) 
		{
			
		}
		
		return ret;
	}
	
	bool MacFileIOManager::DeleteFile(const std::wstring &filename)
	{
		bool ret = false;
		
		try 
		{
			NSString *filePath = NinjaUtilities::WStringToNSString(filename);
			NSString *directory = [filePath stringByDeletingLastPathComponent];
			NSString *file = [filePath lastPathComponent];
			NSInteger tag;
			
			if([[NSWorkspace sharedWorkspace] performFileOperation:NSWorkspaceRecycleOperation source:directory destination:nil files:[NSArray arrayWithObject:file] tag:&tag] == YES)
				ret = true;
			
			[filePath release];
		}
		catch (...) 
		{
			
		}
		
		return ret;
	}

	bool MacFileIOManager::GetFileTimes(const std::wstring &filename, unsigned long long &createdTime, unsigned long long &modifiedTime)
	{
		bool ret = false;

		try 
		{
			if(filename.length())
			{
				NSString *filePath = NinjaUtilities::WStringToNSString(filename);		
				NSURL *fileURL = [NSURL fileURLWithPath:filePath];
				
				NSDate *createDate = nil;
				[fileURL getResourceValue:&createDate forKey:NSURLCreationDateKey error:nil];
				
				NSDate *modDate = nil;
				[fileURL getResourceValue:&modDate forKey:NSURLContentModificationDateKey error:nil];
				
				modifiedTime = [modDate timeIntervalSince1970] * 1000; // we return milliseconds so we must multiply by 1000				
				createdTime = [createDate timeIntervalSince1970] * 1000; // we return milliseconds so we must multiply by 1000
				
				[filePath release];
				
				ret = true;
			}
		}
		catch (...) 
		{
		}

		return ret;
	}

    bool MacFileIOManager::GetFileSize(const std::wstring &filename, unsigned long long &fileSize)
    {
        bool ret = false;

        try 
        {
            if(filename.length())
            {
                NSString *filePath = NinjaUtilities::WStringToNSString(filename);		
                NSURL *fileURL = [NSURL fileURLWithPath:filePath];

                NSNumber *nsFileSize = nil;					
                [fileURL getResourceValue:&nsFileSize forKey:NSURLFileSizeKey error:nil];
                if(nsFileSize)
                    fileSize = [nsFileSize unsignedLongLongValue];
                else 
                    fileSize = 0;			

                [filePath release];

                ret = true;
            }
        }
        catch (...) 
        {
        }

        return ret;
    }
	
	bool MacFileIOManager::DeleteDirectory(const std::wstring &path)
	{
		bool ret = false;
		
		try
		{
			NSString *dirPath = NinjaUtilities::WStringToNSString(path);
			
			if([[NSWorkspace sharedWorkspace] performFileOperation:NSWorkspaceRecycleOperation 
															source:[dirPath stringByDeletingLastPathComponent]
													   destination:@"" 
															 files:[NSArray arrayWithObject:[dirPath lastPathComponent]]
															   tag:nil] == YES)
			{
				ret = true;
			}
			
			[dirPath release];			
		}
		catch (...) 
		{
		}		
		
		return ret;
	}
	
	bool MacFileIOManager::CopyDirectory(const std::wstring &srcPath, const std::wstring &destPath, bool overwriteExistingDirectory)
	{
		bool ret = false;
		
		try 
		{
			if(srcPath.length() && destPath.length() && DirectoryExists(srcPath))
			{
				if(overwriteExistingDirectory && DirectoryExists(destPath))
					DeleteDirectory(destPath);
				
				NSString *src = NinjaUtilities::WStringToNSString(srcPath), *dest = NinjaUtilities::WStringToNSString(destPath);
				if([m_nsFMgr copyItemAtPath:src toPath:dest error:nil] == YES)
					ret = true;
			}
		}
		catch (...) 
		{
		
		}
		
		return ret;
	}

	bool MacFileIOManager::MoveDirectory(const std::wstring &path, const std::wstring &newPath)
	{
		bool ret = false;
		
		try 
		{
			if(path.length() && newPath.length() && DirectoryExists(path) && !DirectoryExists(newPath))
			{
				NSString *src = NinjaUtilities::WStringToNSString(path), *dest = NinjaUtilities::WStringToNSString(newPath);
				if([m_nsFMgr moveItemAtPath:src toPath:dest error:nil] == YES)
					ret = true;
			}			
		}
		catch(...)
		{
		}
		
		return ret;
	}

	bool MacFileIOManager::ReadDirectory(const std::wstring &path, DirectoryContentTypes types, const std::wstring &filterList, FileSystemNodeList &dirContents,
		time_t requestStartTime, bool &requestTimedOut) 
	{
		bool ret = false;
		
		try 
		{
			ret = true;
			FileSystemNode node;

			if(path.length() == 0)
			{
				node.type = FileSystemNode::directory;
				node.modifiedDate = 0;					
				node.creationDate = 0;
				node.filesize = 0;
				node.isWritable = true;

				// get a listing of all local drives/devices
				NSArray *mountedVolumes = [[NSWorkspace sharedWorkspace] mountedLocalVolumePaths];
				for(NSString *vol in mountedVolumes)
				{					
					NSDictionary* fsAttributes;
					NSString *description, *type, *name;
					BOOL removable, writable, unmountable, res;
					
					res = [[NSWorkspace sharedWorkspace] getFileSystemInfoForPath:vol 
										   isRemovable:&removable 
											isWritable:&writable 
										 isUnmountable:&unmountable
										   description:&description
												  type:&type];
					if (res && writable) 
					{
						name = [m_nsFMgr displayNameAtPath:vol];
						node.name = NinjaUtilities::NSStringToWString(name);			
						node.uri = NinjaUtilities::NSStringToWString(vol);			
					
						fsAttributes = [m_nsFMgr attributesOfFileSystemForPath:vol error:nil];
						NSNumber *fileSize = [fsAttributes objectForKey:NSFileSystemSize];
						if(fileSize)
							node.filesize = [fileSize unsignedLongLongValue];
						else 
							node.filesize = 0;							
						
						dirContents.push_back(node);					
					}

					if((time(NULL) - requestStartTime) > READ_DIR_TIMEOUT_SECONDS)
					{
						ret = false;
						requestTimedOut = true;
						break;
					}
				}				
			}
			else if(DirectoryExists(path))
			{
				// handle the filters list if it is specified
				std::vector<std::wstring> filters;
				if(types != directoriesOnly && filterList.length())
					NinjaUtilities::SplitWString(filterList, L';', filters);
				
				
				NSString *dirPath = NinjaUtilities::WStringToNSString(path);
				
				//NSArray *contents = [m_nsFMgr contentsOfDirectoryAtPath:dirPath error:nil];
				
				NSArray *properties = [NSArray arrayWithObjects: NSURLLocalizedNameKey, NSURLIsRegularFileKey, NSURLIsDirectoryKey, NSURLCreationDateKey, NSURLContentModificationDateKey, NSURLFileSizeKey, nil];
				NSURL *dirURL = [NSURL fileURLWithPath:dirPath];
				NSArray *contents = [m_nsFMgr contentsOfDirectoryAtURL:dirURL includingPropertiesForKeys:properties 
															   options:NSDirectoryEnumerationSkipsHiddenFiles | NSDirectoryEnumerationSkipsSubdirectoryDescendants error:nil];
								
				BOOL isDir;
				for (NSURL *url in contents) 
				{
					NSNumber *isDirectory = nil;
					[url getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];
					
					NSNumber *isFile = nil;					
					[url getResourceValue:&isFile forKey:NSURLIsRegularFileKey error:nil];
					
					NSDate *createDate = nil;
					[url getResourceValue:&createDate forKey:NSURLCreationDateKey error:nil];
					
					NSDate *modDate = nil;
					[url getResourceValue:&modDate forKey:NSURLContentModificationDateKey error:nil];
					
					NSString *locName = nil;
					[url getResourceValue:&locName forKey:NSURLLocalizedNameKey error:nil];
					
					if([isFile boolValue] || [isDirectory boolValue])
					{
						isDir = [isDirectory boolValue];
						
						if(isDir)
							node.type = FileSystemNode::directory;
						else
							node.type = FileSystemNode::file;
						
						// check to see if this file is in the filter list
						if(node.type == FileSystemNode::file && filters.size() > 0)
						{
							std::wstring fileExt = NinjaUtilities::NSStringToWString([locName pathExtension]), f;
							bool fileMatchesFilter = false;
							for(std::vector<std::wstring>::const_iterator it = filters.begin(); it != filters.end(); it++)
							{
								f = (*it);
								if(NinjaUtilities::CompareWideStringsNoCase(fileExt.c_str(), f.c_str()) == 0)
								{
									fileMatchesFilter = true;
									break;
								}
							}
							
							if(!fileMatchesFilter)
								continue;
						}
						
						if(types == allDirectoryContents || (types == filesOnly && node.type == FileSystemNode::file) || (types == directoriesOnly && node.type == FileSystemNode::directory))
						{
							node.name = NinjaUtilities::NSStringToWString([locName lastPathComponent]);				
							
							NSString *locPath = [url path];
							node.uri = NinjaUtilities::NSStringToWString(locPath);				
							
							//NSDictionary *fattrs = [m_nsFMgr attributesOfItemAtPath:locPath error:nil];
							//node.filesize = [[fattrs objectForKey:NSFileSize] unsignedLongLongValue];			
							
							NSNumber *fileSize = nil;					
							[url getResourceValue:&fileSize forKey:NSURLFileSizeKey error:nil];
							if(fileSize)
								node.filesize = [fileSize unsignedLongLongValue];
							else 
								node.filesize = 0;							

							node.modifiedDate = [modDate timeIntervalSince1970] * 1000; // we return milliseconds so we must multiply by 1000
							
							node.creationDate = [createDate timeIntervalSince1970] * 1000; // we return milliseconds so we must multiply by 1000
							
							
							if(node.type == FileSystemNode::file)
							{
								node.isWritable = [m_nsFMgr isWritableFileAtPath:locPath];
							}
							else 
							{
								node.isWritable = true; // defautl directories to true for now. If we need to implement this we will in the future. 
							}

								

							dirContents.push_back(node);					
						}						
					}

					if((time(NULL) - requestStartTime) > READ_DIR_TIMEOUT_SECONDS)
					{
						ret = false;
						requestTimedOut = true;
						break;
					}
				}
			}
		}
		catch(...)
		{
			ret = false;
		}
		
		return ret;
	}

	bool MacFileIOManager::DirectoryIsEmpty(const std::wstring &path)
	{
		bool ret = false;
		
		try 
		{
			if(path.length() && DirectoryExists(path))
			{
				NSString *dirPath = NinjaUtilities::WStringToNSString(path);
				NSArray *contents = [m_nsFMgr contentsOfDirectoryAtPath:dirPath error:nil];
				
				ret = [contents count] > 0;
			}
		}
		catch(...)
		{
		}
		
		return ret;
	}

	bool MacFileIOManager::GetDirectoryTimes(const std::wstring &path, unsigned long long &createdTime, unsigned long long &modifiedTime)
	{
		bool ret = false;

		try 
		{
			if(path.length())
			{
				NSString *dirPath = NinjaUtilities::WStringToNSString(path);		
				NSURL *dirURL = [NSURL fileURLWithPath:dirPath];
				
				NSDate *createDate = nil;
				[dirURL getResourceValue:&createDate forKey:NSURLCreationDateKey error:nil];
				
				NSDate *modDate = nil;
				[dirURL getResourceValue:&modDate forKey:NSURLContentModificationDateKey error:nil];
				
				modifiedTime = [modDate timeIntervalSince1970] * 1000; // we return milliseconds so we must multiply by 1000				
				createdTime = [createDate timeIntervalSince1970] * 1000; // we return milliseconds so we must multiply by 1000
				
				[dirPath release];
				
				ret = true;
			}
		}
		catch (...) 
		{
		}

		return ret;
	}

    bool MacFileIOManager::ReadTextFromURL(const std::wstring &url, char **fileContents, unsigned int &contentLength)
    {
        bool ret = false;

        try 
        {

            // TODO

        }
        catch (...) 
        {
        }

        return ret;
    }

    bool MacFileIOManager::ReadBinaryFromURL(const std::wstring &url, unsigned char **fileContents, unsigned int &contentLength)
    {
        bool ret = false;

        try 
        {

            // TODO

        }
        catch (...) 
        {
        }

        return ret;
    }
}