/*
<copyright>
	This file contains proprietary software owned by Motorola Mobility, Inc.
	No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.
	(c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
</copyright>
*/
#ifndef __UTILS__
#define __UTILS__

#include <string>
#include <vector>
#ifdef _WINDOWS
	#include <windows.h>
	#include <atltrace.h>
#else // mac
	#include <assert.h>
#endif

#define NINJA_FILEAPI_URL_TOKEN L"/file"
#define NINJA_DIRECTORYAPI_URL_TOKEN L"/directory"
#define NINJA_CLOUDSTATUS_TOKEN L"/cloudstatus"

#define PATH_SEPARATOR_CHAR L'/'

#ifdef _WINDOWS
#ifdef _DEBUG
#ifndef ASSERT
#define ASSERT(condition) if(!(condition)) { DebugBreak(); }
#endif
#ifndef VERIFY
#define VERIFY(f)          ASSERT(f)
#endif
#ifndef TRACE
#define TRACE  OutputDebugString
#endif
#else // Release
#ifndef ASSERT
#define ASSERT(condition) __noop
#endif
#ifndef VERIFY
#define VERIFY(f) f
#endif
#ifndef TRACE
#define TRACE __noop
#endif
#endif
#else // mac
#ifdef _DEBUG
#define ASSERT(condition) if(!(condition)) { assert(false); }
#define VERIFY(f) ASSERT(f)
#define TRACE ((void)0)
#else
#define ASSERT(condition) ((void)0)
#define VERIFY(f) f
#define TRACE ((void)0)
#endif
#endif

#define PORT_MIN 1024
#define PORT_MAX 65535

namespace NinjaUtilities
{
    void GetHttpResponseTime(char *buf, unsigned int bufflen);

    int FindAvailablePort();
	bool IsValidPortNumber(unsigned int port);
	bool GetLocalURLForPort(unsigned int port, std::wstring& urlOut);

//    bool CreateSecurityToken(std::string &tokenOut);

	///////////////////////////////////////////
	// string helpers
	void uri_unescape(const std::wstring& in, std::wstring& out);
	//std::string GetParameterValueFromURL(const char *url, const char *paramName);
	void StringToWString(const std::string& in, std::wstring& out);
	void WStringToString(const std::wstring& in, std::string& out);
	void ConvertForwardSlashtoBackslash(std::wstring& path);
	void ConvertBackslashtoForwardSlash(std::wstring& path);
#ifdef _MAC
	std::wstring NSStringToWString(NSString *str);
	NSString* WStringToNSString(const std::wstring &str);
	NSString* StringToNSString(const wchar_t *str);
#endif
	int CompareStringsNoCase(const char *s1, const char *s2);
	int CompareWideStringsNoCase(const wchar_t *s1, const wchar_t *s2);
	void SplitWString(const std::wstring &s, wchar_t delim, std::vector<std::wstring> &tokensOut);
	bool FileExtensionMatches(const wchar_t *filename, const wchar_t *extension);

    bool IsUrlForNinjaCloudStatus(const wchar_t* pathIn);
    bool IsUrlForNinjaFileService(const wchar_t* pathIn);
    bool IsUrlForNinjaDirectoryService(const wchar_t* pathIn);

	class PathUtility
	{
	public:
		PathUtility();
		PathUtility(const wchar_t* pathIn);

		bool SetPath(const wchar_t *pathIn);
		bool GetPath(std::wstring &pathOut);

		//bool SetPathFromApiUrl(const wchar_t *urlIn);
		bool GetParent(std::wstring &nameOut);
		bool GetDirectoryOrFilename(std::wstring &dirNameOut);
		bool CreateSearchPath(std::wstring &srchPathOut);

	private:
		std::wstring thePath;
	};

	class PlatformUtility
	{
	public:
        PlatformUtility() {}
        virtual ~PlatformUtility() {}
        
		virtual void LogMessage(const wchar_t *msg) = 0;

        // this should get the optional secondary origin for Ninja (usually local for dev purposes).        
        virtual bool GetLocalNinjaOrigin(std::wstring &valueOut) = 0; 

        virtual bool GetVersionNumber(std::wstring &valueOut) = 0; 
		
		virtual bool GetPreferenceBool(const wchar_t *key, bool defaultValue) = 0;
		virtual int GetPreferenceInt(const wchar_t *key, int defaultValue) = 0;
		virtual double GetPreferenceDouble(const wchar_t *key, double defaultValue) = 0;
		virtual void GetPreferenceString(const wchar_t *key, std::wstring valueOut, const wchar_t *defaultValue) = 0;
	};
}

#endif // __UTILS__
