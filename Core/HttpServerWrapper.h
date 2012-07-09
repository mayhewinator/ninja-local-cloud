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
#ifndef __HttpServerWrapper__
#define __HttpServerWrapper__
#include <string>
#include <deque>

namespace NinjaUtilities
{
	class PlatformUtility;
}
namespace NinjaFileIO
{
	class FileIOManager;
}	


struct mg_context;
struct mg_connection;
struct mg_request_info;

// internal structure for validating requests
//struct CloudConnectionInfo;

// this represents an instance of a running HTTP server
class CHttpServerWrapper
{
public:
	CHttpServerWrapper();
	~CHttpServerWrapper(void);

	bool Start(unsigned int port, const wchar_t *documentRoots);
	void Stop();
	bool IsRunning() { return serverContext != NULL; }
	const wchar_t *GetURL() const;
	unsigned int GetPort() const { return serverPort; }
    const wchar_t *GetDocumentRoot() const { return serverRoots.c_str(); }

	void SetPlatformUtilities(NinjaUtilities::PlatformUtility *p);
	void SetFileIOManager(NinjaFileIO::FileIOManager *f);

	void LogMessage(const wchar_t *fmt, ...);
	void LogMessage(const char *fmt, ...);

	bool HandleFileServiceRequest(mg_connection *conn, const mg_request_info *request_info);
	bool HandleDirectoryServiceRequest(mg_connection *conn, const mg_request_info *request_info);
    bool HandleWebServiceRequest(mg_connection *conn, const mg_request_info *request_info);

    bool ApproveConnectionRequest(const char *origin);

    bool GetVersionNumber(std::wstring &valOut);

private:
	mg_context *serverContext;
	std::wstring serverRoots;
	std::wstring serverUrl;
	unsigned int serverPort;
    
	NinjaUtilities::PlatformUtility *m_platformUtils;
	NinjaFileIO::FileIOManager *m_fileMgr;

	bool limitIOToRootDirectory;
	bool PathIsUnderDocumentRoot(std::wstring &path);
};

#endif //__HttpServerWrapper__

