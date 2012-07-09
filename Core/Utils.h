/*
<copyright>
Copyright (c) 2012, Motorola Mobility LLC.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

* Neither the name of Motorola Mobility LLC nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, 
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR 
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#define NINJA_WEBAPI_URL_TOKEN L"/web"
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
    bool IsUrlForNinjaWebService(const wchar_t* pathIn);

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
