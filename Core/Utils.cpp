/*
<copyright>
	This file contains proprietary software owned by Motorola Mobility, Inc.
	No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.
	(c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
</copyright>
*/
#include "Utils.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <time.h>

#ifdef _WINDOWS
#include <shlobj.h>
#include "rpc.h"
#else
#include "netinet/in.h"
#endif

namespace NinjaUtilities
{			
    void GetHttpResponseTime(char *buf, unsigned int bufflen)
    {
        if(buf && bufflen)
        {
            time_t now = time(0);
            
#ifdef _WINDOWS
            tm t;
            gmtime_s(&t, &now);
            strftime(buf, bufflen, "%a, %d %b %Y %H:%M:%S GMT", &t);
#else
            tm *t = NULL;
            t = gmtime(&now);
            strftime(buf, bufflen, "%a, %d %b %Y %H:%M:%S GMT", t);
#endif
        }
    }

    /*bool CreateSecurityToken(std::string &tokenOut)
    {
        bool ret = false;
        
        tokenOut.clear();

#ifdef _WINDOWS
        UUID idObj;
        if(UuidCreate(&idObj) == RPC_S_OK)
        {
            RPC_WSTR idStr = NULL;
            UuidToString( &idObj, &idStr);  
            std::wstring tokenW = reinterpret_cast<wchar_t*>(idStr);
            if(tokenW.length())
            {
                WStringToString(tokenW, tokenOut);
                ret = true;
            }
            RpcStringFree(&idStr);
        }
#else
        CFUUIDRef uuidObject = CFUUIDCreate(nil);
        NSString *uuidString = (NSString*)CFUUIDCreateString(nil, uuidObj);
        CFRelease(uuidObj);
        tokenOut = NSStringToWString(uuidString);
#endif

        return ret;
    }*/

    int FindAvailablePort()
    {
        int ret = -1, socketDescriptor;

        try
        {
            bool initialized = false;
#ifdef _WINDOWS
            WSADATA wsaData;
            initialized = (WSAStartup(MAKEWORD(2, 2), &wsaData) == NO_ERROR);
#else
            initialized = true;
#endif
            if (initialized)
            {
                socketDescriptor = socket(PF_INET, (int)SOCK_STREAM, IPPROTO_TCP);

                if(socketDescriptor != -1)
                {
                    sockaddr_in socketAddr;					
                    memset(&socketAddr, 0, sizeof(socketAddr));
                    socketAddr.sin_family = AF_INET;
                    socketAddr.sin_port = htons(0);
                    socketAddr.sin_addr.s_addr = INADDR_ANY;

#ifdef _WINDOWS
                    int sockAddrSize = sizeof(sockaddr);
#else
					socklen_t sockAddrSize = sizeof(sockaddr);
#endif 
                    if(bind(socketDescriptor, (sockaddr*)&socketAddr, sizeof(socketAddr)) != -1)
                    {
                        memset(&socketAddr, 0, sizeof(socketAddr));
                        if(getsockname(socketDescriptor, (sockaddr*)&socketAddr, &sockAddrSize) != -1)
                            ret = ntohs(socketAddr.sin_port);
                    }
                }
            }
        }
        catch(...)
        {
            ret = -1;
        }

#ifdef _WINDOWS
        closesocket(socketDescriptor);
#else
		close(socketDescriptor);
#endif

#ifdef _WINDOWS
        WSACleanup();
#endif
        return ret;
    }
	
	bool IsValidPortNumber(unsigned int port)
	{
		return (port >= PORT_MIN && port <= PORT_MAX);		
	}
	
	bool GetLocalURLForPort(unsigned int port, std::wstring& urlOut)
	{
		bool ret = false;
		
		if(IsValidPortNumber(port))
		{
			std::wstringstream tmp;
			tmp << L"http://localhost:";
			tmp << port;
			urlOut = tmp.str();
			ret = true;
		}
		
		return ret;
	}


	void uri_unescape(const std::wstring& in, std::wstring& out)
	{
		std::wistringstream a;
		int tmp(0);
		out.clear();
		for (size_t i=0; i<in.size();)
		{
			if (in[i] != '%')
			{
				out.push_back(in[i]);
				++i;
			}
			else
			{
				a.str(in.substr(i+1, 2));
				i += 3;
				a >> std::hex >> tmp;
				out.push_back(tmp);
				a.clear();
			}
		}
	}

	void StringToWString(const std::string& in, std::wstring& out)
	{
		out.assign(in.begin(), in.end());
	}
	void WStringToString(const std::wstring& in, std::string& out)
	{
		out.assign(in.begin(), in.end());
	}
	
#ifdef _MAC
	std::wstring NSStringToWString(NSString *str)
	{
		NSData* asData = [str dataUsingEncoding:CFStringConvertEncodingToNSStringEncoding(kCFStringEncodingUTF32LE)];
		return std::wstring((wchar_t*)[asData bytes], [asData length] / sizeof(wchar_t));
	}
	
	NSString* WStringToNSString(const std::wstring &str)
	{
		char* data = (char*)str.data();
		unsigned size = str.size() * sizeof(wchar_t);
		
		NSString* result = [[NSString alloc] initWithBytes:data length:size encoding:CFStringConvertEncodingToNSStringEncoding(kCFStringEncodingUTF32LE)];
		return result;
		
	}
	
	NSString* StringToNSString(const wchar_t *str)
	{
		NSString* result = nil;
		if(str && wcslen(str))
		{
			unsigned size = wcslen(str) * sizeof(wchar_t);		
			result = [[NSString alloc] initWithBytes:str length:size encoding:CFStringConvertEncodingToNSStringEncoding(kCFStringEncodingUTF32LE)];			
		}
		return result;
	}

#endif
	
	int CompareStringsNoCase(const char *s1, const char *s2)
	{
#ifdef _WINDOWS
		return _strcmpi(s1, s2);
#else
		return strcasecmp(s1, s2);
#endif
	}
	
#ifdef _MAC
	// this function doesn't seem to exist on OSX nor does wcscasecmp
	// so I am including my own for now
	int wcsicmp(const wchar_t* p, const wchar_t* q)
	{
		while (towupper(*p) == towupper(*q)) 
		{ 
			if (*p == '\0') 
			{ 
				return 0;
			}
			p += 1;
			q += 1;
		}
		return towupper(*p) - towupper(*q);
	}
#endif
	
	int CompareWideStringsNoCase(const wchar_t *s1, const wchar_t *s2)
	{
#ifdef _WINDOWS
		return _wcsicmp(s1, s2);
#else
		return wcsicmp(s1, s2);		
#endif	
	}

	void ConvertForwardSlashtoBackslash(std::wstring& path)
	{
		// replace all forward slashes with backslashes
		std::replace( path.begin(), path.end(), L'/', L'\\' );
	}

	void ConvertBackslashtoForwardSlash(std::wstring& path)
	{
		// replace all forward slashes with backslashes
		std::replace( path.begin(), path.end(), L'\\', L'/' );
	}

	bool IsUrlForNinjaCloudStatus(const wchar_t* pathIn)
	{
		bool ret = false;

		if(pathIn && wcsstr(pathIn, NINJA_CLOUDSTATUS_TOKEN) == pathIn)
			ret = true;

		return ret;
	}

    bool IsUrlForNinjaFileService(const wchar_t* pathIn)
    {
        bool ret = false;

        if(pathIn && wcsstr(pathIn, NINJA_FILEAPI_URL_TOKEN) == pathIn)
            ret = true;

        return ret;
    }

	bool IsUrlForNinjaDirectoryService(const wchar_t* pathIn)
	{
		bool ret = false;

		if(pathIn && wcsstr(pathIn, NINJA_DIRECTORYAPI_URL_TOKEN) == pathIn)
			ret = true;

		return ret;
	}

	std::wstring GetDefaultProjectRootFolder()
	{
		std::wstring ret;
	#ifdef _WINDOWS
		PWSTR docPath;
		HRESULT shRes = ::SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &docPath);		
		if(SUCCEEDED(shRes) && docPath)
		{
			ret = docPath; 

			CoTaskMemFree(docPath);
		}
	#else // MAC
		NSString *foo = NSHomeDirectory();
		ret = NSStringToWString(foo);
	#endif

		return ret;
	}

	void SplitWString(const std::wstring &s, wchar_t delim, std::vector<std::wstring> &tokensOut) 
	{
		std::wstringstream ss(s);
		std::wstring item;
		while(std::getline(ss, item, delim)) 
		{
			tokensOut.push_back(item);
		}
	}

	// returns true if the extension of the file specified in filename matches the extension 
	bool FileExtensionMatches(const wchar_t *filename, const wchar_t *extension)
	{
		bool ret = false;
		const wchar_t *fileExtSep = wcsrchr(filename, L'.');
		if(fileExtSep)
		{
			ret = CompareWideStringsNoCase(fileExtSep + 1, extension) == 0;
		}

		return ret;
	}

	///////////////////////////////////////////////////////////////////////////////////////
	// PathUtilities class
	PathUtility::PathUtility()
	{
		thePath.empty();
	}

	PathUtility::PathUtility(const wchar_t* pathIn)
	{
		SetPath(pathIn);
	}

	bool PathUtility::SetPath(const wchar_t *pathIn)
	{
		bool ret = false;

		if(pathIn)
		{
			thePath = pathIn;

			// remove any trailing path separators
			std::wstring::size_type strIndex = thePath.find_last_of(PATH_SEPARATOR_CHAR, std::wstring::npos);
			if(strIndex != std::wstring::npos && strIndex == (thePath.length()-1))
			{
				thePath = thePath.substr(0, thePath.length()-1);
			}

			ret = true;
		}

		return ret;
	}

	bool PathUtility::GetPath(std::wstring &pathOut)
	{
		bool ret = false;

		if(thePath.length())
		{
			pathOut = thePath;
			ret = true;
		}

		return ret;
	}

/*	bool PathUtility::SetPathFromApiUrl(const wchar_t *urlIn)
	{
		bool ret = false;

		if(urlIn && wcslen(urlIn))
		{
			std::wstring url = urlIn, path;
			std::wstring::size_type fileSvcFindInex = url.find(NINJA_FILEAPI_URL), dirSvcFindIndex = url.find(NINJA_DIRECTORYAPI_URL);
			if(fileSvcFindInex != std::wstring::npos) // its a file API call
			{
				// extract the file path
				// e.g. http://File.Services.Ninja.Motorola.com/c/users/jdoe/my%20documents/foo.html
				path = url.substr(fileSvcFindInex + wcslen(NINJA_FILEAPI_URL) + 1); // +1 is to accommodate the trailing '/' after ".com"
			}
			else if(dirSvcFindIndex != std::wstring::npos) // its a directory API call
			{
				// extract the directory path
				// http://Directory.Services.Ninja.Motorola.com/c/users/jdoe/my%20documents
				path = url.substr(dirSvcFindIndex + wcslen(NINJA_DIRECTORYAPI_URL) + 1); // +1 is to accommodate the trailing '/' after ".com"
			}

	#ifdef _WINDOWS
			// insert the ':' for the drive separator
			path.insert(1, 1, L':');
	#endif

			if (path.length())
			{
				uri_unescape(path, thePath);

				ret = true;
			}
		}

		return ret;
	}
*/

	bool PathUtility::GetParent(std::wstring &parentDirOut)
	{
		bool ret = false;

		if(thePath.length())
		{
			std::wstring::size_type strIndex = thePath.find_last_of(PATH_SEPARATOR_CHAR, std::wstring::npos);
			if(strIndex != std::wstring::npos)
			{
				parentDirOut = thePath.substr(0, strIndex);
				ret = true;
			}
		}

		return ret;
	}

	// gets the item after the last '\'
	bool PathUtility::GetDirectoryOrFilename(std::wstring &nameOut)
	{
		bool ret = false;

		if(thePath.length())
		{
			std::wstring::size_type strIndex = thePath.find_last_of(PATH_SEPARATOR_CHAR, std::wstring::npos);
			if(strIndex != std::wstring::npos)
			{
				nameOut = thePath.substr(strIndex + 1, thePath.length() - strIndex - 1);
				ret = true;
			}

			ret = true;
		}

		return ret;
	}


	bool PathUtility::CreateSearchPath(std::wstring &srchPathOut)
	{
		bool ret = false;

		if(thePath.length())
		{
			srchPathOut = thePath + PATH_SEPARATOR_CHAR + L"*.*";
		}

		return ret;
	}
}
