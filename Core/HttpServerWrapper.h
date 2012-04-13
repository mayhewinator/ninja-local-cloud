/*
<copyright>
	This file contains proprietary software owned by Motorola Mobility, Inc.
	No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.
	(c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
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

