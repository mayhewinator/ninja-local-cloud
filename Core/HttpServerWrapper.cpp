/*
<copyright>
	This file contains proprietary software owned by Motorola Mobility, Inc.
	No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.
	(c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
</copyright>
*/
#include "HttpServerWrapper.h"
#include "Mongoose/mongoose.h"
#include "Utils.h"
#include "FileIOManager.h"
#include <stdarg.h>
#include <sstream>
#include <time.h>
#include <algorithm>

#ifdef _MAC
// byte doesn't seem to be defined on Mac
typedef char byte;
#endif

#define OptionTemplate_DocRoot_Index 1
#define OptionTemplate_ListeningPort_Index 3

// This is the origin for requests that come from our packaged Chrome app. By default only file IO API requests from this app are allowed.
// Requests from any other origin are denied and we respond with 403 FORBIDDEN
#define Approved_Ninja_Origin "chrome-extension://jjdndclgmfdgpiccmlamlicfjmkognne"

#define NINJA_SANDBOXED_LOGICALDRIVE L"Ninja Cloud"

static const char *mongooseOptionsTemplate[] = 
{
	// example of setting an alias and using multiple document roots, in case we need this
	//"document_root", "C:\\Documents\\Source,/SomeAlias=C:\\Documents\\Source\\SomeDirectory",

	"document_root",            ".",
	"listening_ports",          "9980",
	"num_threads",              "10",
	"enable_directory_listing", "no",
	"enable_keep_alive",        "yes",
    "access_control_list", "-0.0.0.0/0,+127.0.0.1", // limit normal http to local host for requests handled by mongoose
	NULL
};

enum ResponseStatuCodes {
	responseStatus200,
	responseStatus201,
	responseStatus204,
	responseStatus304,
	responseStatus400,
	responseStatus401,
	responseStatus403,
	responseStatus404,
	responseStatus406,
	responseStatus413,
	responseStatus414,
	responseStatus500,
	responseStatus501,
	responseStatus502,
	responseStatus503,
	responseStatus504,
	responseStatus505
};

static const char *responseStatusDescriptions[] = 
{
	"200 OK",
	"201 Created",
	"204 No Content",
	"304 Not Modified",
	"400 Bad Request",
	"401 Unauthorized",
	"403 Forbidden",
	"404 Not Found",
	"406 Not Acceptable",
	"413 Request Entity Too Large",
	"414 Request-URI Too Long",
	"500 Internal Server Error",
	"501 Not Implemented",
	"502 Bad Gateway",
	"503 Service Unavailable",
	"504 Gateway Timeout",
	"505 HTTP Version Not Supported"
};

struct HttpResponseData
{
    HttpResponseData()
    {
    }

	~HttpResponseData()
	{
		additionalHeaders.clear();
	}

	typedef std::pair<std::string, std::string> HeaderNameValuePair;
	typedef std::deque<HeaderNameValuePair> HeaderNameValueList;

	ResponseStatuCodes responseStatus;
	std::string contentType;
	std::string origin;
	std::string responseBody;
	HeaderNameValueList additionalHeaders;

	void AddHeader(const char *name, const char *val)
	{
		HeaderNameValuePair p;
		if(name && val)
		{
			p.first = name;
			p.second = val;
			additionalHeaders.push_back(p);
		}
	}

	std::string GetResponsString() {
		std::stringstream sstr;
		sstr << "HTTP/1.1 " << responseStatusDescriptions[responseStatus] << "\r\n";

        sstr << "Access-Control-Allow-Origin: " << origin << "\r\n";
        sstr << "Access-Control-Allow-Methods: POST, GET, DELETE, PUT\r\n";
        sstr << "Access-Control-Allow-Headers: Content-Type, sourceURI, overwrite-destination, check-existence-only, recursive, return-type, operation, delete-source, file-filters, if-modified-since, get-file-info\r\n";
        sstr << "Access-Control-Max-Age: 1728000\r\n";
        
        char timeBuffer[100];
        NinjaUtilities::GetHttpResponseTime(timeBuffer, 100);
        sstr << "Date:" << timeBuffer << "\r\n";

		// output additional headers
		if(additionalHeaders.size() > 0)
		{
			HeaderNameValueList::const_iterator it;
			for(it = additionalHeaders.begin(); it != additionalHeaders.end(); it++)
			{
				std::pair<std::string, std::string> hdr = (*it);
				if(hdr.first.length() && hdr.second.length())
				{
					sstr << hdr.first << ": " << hdr.second << "\r\n";
				}
			}
		}

		sstr << "Cache-Control: no-cache\r\n";
		sstr << "Content-Type: " << contentType << "\r\n";
		sstr << "Content-Length: " << responseBody.length() << "\r\n";
		if(responseBody.length())
			sstr << "\r\n" << responseBody;
		else
			sstr << "\r\n";

		return sstr.str();
	}
};

void RespondToUnauthorizedRequest(mg_connection *conn, CHttpServerWrapper *wrapper)
{
    if(conn)
    {
        const char *originHdr = mg_get_header(conn, "Origin");	
        if(originHdr)
            wrapper->LogMessage("Ignoring request from origin \"%s\"", originHdr);

        char timeBuffer[100];
        NinjaUtilities::GetHttpResponseTime(timeBuffer, 100);

        std::stringstream sstr;
        sstr << "HTTP/1.1 " << responseStatusDescriptions[responseStatus403] << "\r\n";
        sstr << "Date: " << timeBuffer<< "\r\n";
        sstr << "Cache-Control: no-cache\r\n";
        sstr << "Content-Type: text/html; charset=UTF-8\r\nContent-Length: 159\r\n\r\n<!DOCTYPE html><html lang=en><title>Error 403 (Forbidden)</title><body><h2>Access Forbidden</h2>You are not authorized to connect to this server.</body></html>";
        
        std::string respStr = sstr.str();
        mg_write(conn, respStr.c_str(), respStr.length());
    }
}

bool GetRequestDataHelper(mg_connection *conn, const mg_request_info *request_info, byte** bytesOut, size_t &sizeOut)
{
	bool ret = false;

	if(conn && request_info && bytesOut)
	{
		sizeOut = 0;
		*bytesOut = NULL;
		const char *cl = mg_get_header(conn, "Content-Length");
		if(cl)
		{
			int contentLen = atoi(cl);
			if(contentLen > 0)
			{
				byte *buff = new byte[contentLen + 1];
				memset(buff, 0, (contentLen + 1)*sizeof(byte));
				sizeOut = mg_read(conn, buff, contentLen);
				*bytesOut = buff;
				ret = sizeOut == contentLen;
			}
		}
	}

	return ret;
}

void *mongooseEventHandler(enum mg_event event, struct mg_connection *conn, const struct mg_request_info *request_info) 
{
	void *processed = NULL;	 

	if(event == MG_NEW_REQUEST)
	{
#ifdef _MAC
		NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
#endif
		
		CHttpServerWrapper *wrapper = static_cast<CHttpServerWrapper*>(request_info->user_data);

		std::string uriStr = request_info->uri;
		std::wstring uriWstr;	
		NinjaUtilities::StringToWString(uriStr, uriWstr);

        const char *originHdr = mg_get_header(conn, "Origin");
        if(originHdr != NULL)
        {
            if(wrapper->ApproveConnectionRequest(originHdr))
            {
                if(NinjaUtilities::IsUrlForNinjaFileService(uriWstr.c_str()))
                {
                    wrapper->HandleFileServiceRequest(conn, request_info);
                }
                else if(NinjaUtilities::IsUrlForNinjaDirectoryService(uriWstr.c_str()))
                {
                    wrapper->HandleDirectoryServiceRequest(conn, request_info);
                }
                else if(NinjaUtilities::IsUrlForNinjaCloudStatus(uriWstr.c_str()))
                {
                    // process the request
                    HttpResponseData resp;
                    resp.responseStatus =  responseStatus200;
                    resp.origin = originHdr;	
                    resp.contentType = "application/json";
                    std::wstring drw = wrapper->GetDocumentRoot(), verNumw;
                    
                    NinjaUtilities::ConvertBackslashtoForwardSlash(drw);
                    std::string dr, verNum;
                    NinjaUtilities::WStringToString(drw, dr);

                    resp.responseBody = "{\"name\":\"Ninja Local Cloud\",\"status\": \"running\",\"server-root\":\"";  
                    resp.responseBody += dr;
                    if(wrapper->GetVersionNumber(verNumw))
                    {
                        NinjaUtilities::WStringToString(verNumw, verNum);
                        resp.responseBody += "\",\"version\":\"";
                        resp.responseBody += verNum;
                    }
                    resp.responseBody += "\"}";  

                    std::string respStr = resp.GetResponsString();
                    mg_write(conn, respStr.c_str(), respStr.length());
                    wrapper->LogMessage(L"Cloud status request received");
                }
            }
            else
            {
                RespondToUnauthorizedRequest(conn, wrapper);
            }
            processed = (void*)1; // mark this request as processed by us so Mongoose does not attempt to handle it
        }
		
#ifdef _MAC
		[pool drain];
#endif
	}

	return processed;
}

CHttpServerWrapper::CHttpServerWrapper()
{
	serverContext = NULL;
	m_platformUtils = NULL;
	m_fileMgr = NULL;
}

CHttpServerWrapper::~CHttpServerWrapper(void)
{
	Stop();
}

void CHttpServerWrapper::SetPlatformUtilities(NinjaUtilities::PlatformUtility *p)
{
	ASSERT(p);
	m_platformUtils = p;
}

void CHttpServerWrapper::LogMessage(const wchar_t *fmt, ...)
{
	if(m_platformUtils)
	{
		va_list ptr;
		va_start(ptr, fmt);

		wchar_t msgOut[501];
		vswprintf(msgOut, 300, fmt, ptr);
		va_end(ptr);
		m_platformUtils->LogMessage(msgOut);
	}
}

void CHttpServerWrapper::LogMessage(const char *fmt, ...)
{
	if(m_platformUtils)
	{
		va_list ptr;
		va_start(ptr, fmt);

		char msgOut[501];
#ifdef _WINDOWS
		vsprintf_s(msgOut, fmt, ptr);
#else
		vsprintf(msgOut, fmt, ptr);
#endif
		va_end(ptr);
		
		std::string s = msgOut;
		std::wstring sw;
		NinjaUtilities::StringToWString(s, sw);
		LogMessage(sw.c_str());
	}
}

bool CHttpServerWrapper::GetVersionNumber(std::wstring &valOut)
{
    return m_platformUtils->GetVersionNumber(valOut);
}

void CHttpServerWrapper::SetFileIOManager(NinjaFileIO::FileIOManager *f)
{
	ASSERT(f);
	m_fileMgr = f;
}

bool CHttpServerWrapper::Start(unsigned int port, const wchar_t *documentRoots)
{
	bool ret = false;

	serverRoots = documentRoots;
	serverPort = port;

	limitIOToRootDirectory = m_platformUtils->GetPreferenceBool(L"Limit IO to Document Root", true);
	
	bool rootExists = m_fileMgr->DirectoryExists(documentRoots);

    // if the root does not exist, try to create it
	if(!rootExists)
        rootExists = m_fileMgr->CreateNewDirectory(documentRoots);

    if(!rootExists)
    	LogMessage(L"Error: The root directory \"%ls\" does not exist!", documentRoots);
	
	if(!NinjaUtilities::IsValidPortNumber(serverPort))
		LogMessage(L"Invalid port number! The port number mus be between %u and %u.", PORT_MIN, PORT_MAX);

	if(rootExists && NinjaUtilities::IsValidPortNumber(serverPort) && serverContext == NULL && serverRoots.length())
	{
		std::string strRoot, strPort;
		NinjaUtilities::WStringToString(serverRoots, strRoot);
		mongooseOptionsTemplate[OptionTemplate_DocRoot_Index] = strRoot.c_str();

		char portStr[30];
#ifdef _WINDOWS
		sprintf_s(portStr, 30, "%u", serverPort);
#else
		sprintf(portStr, "%u", serverPort);
#endif
		mongooseOptionsTemplate[OptionTemplate_ListeningPort_Index] = portStr;

		serverContext = mg_start(&mongooseEventHandler, this, mongooseOptionsTemplate);
		if(serverContext)
		{
			NinjaUtilities::GetLocalURLForPort(serverPort, serverUrl);		
			LogMessage(L"Local Cloud Server URL: %ls", serverUrl.c_str());

			ret = true;
		}
	}

	return ret;
}

void CHttpServerWrapper::Stop()
{
	if(serverContext)
	{
		mg_stop(serverContext);
		serverContext = NULL;
	}
}


bool CHttpServerWrapper::ApproveConnectionRequest(const char *origin)
{
    bool ret = false;

    if(origin && strlen(origin) && NinjaUtilities::CompareStringsNoCase(origin, Approved_Ninja_Origin) == 0)
    {
        ret = true;
    }
    else
    {
        // check if a secondary origin has been specified
        std::wstring localOrigin;
        if(m_platformUtils->GetLocalNinjaOrigin(localOrigin) && localOrigin.length())
        {
            std::string o;
            NinjaUtilities::WStringToString(localOrigin, o);
            if(NinjaUtilities::CompareStringsNoCase(origin, o.c_str()) == 0 || 
                (NinjaUtilities::CompareStringsNoCase("chrome-extension://*", o.c_str()) == 0 && strstr(origin, "chrome-extension://") != NULL))
            {
                ret = true;
            }
        }
    }

    return ret;
}

const wchar_t *CHttpServerWrapper::GetURL() const
{
	const wchar_t *ret = serverUrl.c_str();

	return ret;
}

bool CHttpServerWrapper::PathIsUnderDocumentRoot(std::wstring &path)
{
	bool ret = false;
	
	if(path.length())
	{
		std::wstring root = serverRoots, pathCopy = path;
		NinjaUtilities::ConvertBackslashtoForwardSlash(root);
		NinjaUtilities::ConvertBackslashtoForwardSlash(pathCopy);
		
		std::transform(root.begin(), root.end(), root.begin(), ::tolower);
		std::transform(pathCopy.begin(), pathCopy.end(), pathCopy.begin(), ::tolower);

        if(pathCopy.length() >= root.length())
        {
            // if the lengths are equal then it is a special case where the path could be the actual document root itself.
            // If it the path is longer we want to ensure the root has a trailing slash so we do not accept c:/foo/text.txt if the root is c:/foobar
            if(pathCopy.length() > root.length())
            {
                if(root.rfind(L'/') != root.length() - 1)
                    root += L'/';
            }
            
            int srchIndex = pathCopy.find(root, 0);
		    ret = (srchIndex == 0);
        }
	}
	
	return ret;
}

bool CHttpServerWrapper::HandleFileServiceRequest(mg_connection *conn, const mg_request_info *request_info)
{
	bool ret = false;

	try
	{
		if(conn && request_info)
		{
			std::string urlstr = request_info->uri;
			std::wstring url, path;
			NinjaUtilities::StringToWString(urlstr, url);
			url = url.erase(0, wcslen(NINJA_FILEAPI_URL_TOKEN));
			NinjaUtilities::PathUtility pathUtil;
			pathUtil.SetPath(url.c_str());
			path = url;

#ifdef _WINDOWS
			if(path.find(L'/') == 0) // remove leading / char
				path.erase(0, 1);

			int firstSlash = path.find(L'/');
			if(firstSlash != -1)
				path.insert(firstSlash, L":"); // insert the : before the first slash
#endif

			LogMessage(L"File service request with path=\"%ls\"", path.c_str());

			// process the request
			HttpResponseData resp;
			resp.responseStatus =  responseStatus501;
			resp.contentType = "text/plain, charset=utf-8";
			resp.origin = mg_get_header(conn, "ORIGIN");			

			size_t size = 0;
			byte* bytes = NULL;
			bool dataRead = GetRequestDataHelper(conn, request_info, &bytes, size);

			if(NinjaUtilities::CompareStringsNoCase(request_info->request_method, "options") == 0) 
			{
				// this is a cross origin preflight request from the browser so we simply need to response
				// with the correct cross origin responses.
				LogMessage(L"     Method = OPTIONS. Handling Cross Origin Preflight.");
				resp.responseStatus = responseStatus200;
			}
            else if(limitIOToRootDirectory && PathIsUnderDocumentRoot(path) == false)
            {
                resp.responseStatus = responseStatus403;
            }
			else if(NinjaUtilities::CompareStringsNoCase(request_info->request_method, "post") == 0) // create a new file
			{
				LogMessage(L"     Method = POST. Creating a new file.");

				resp.responseStatus =  responseStatus404;
				bool fileExists = m_fileMgr->FileExists(path);
				if(fileExists == false)
				{
					if(m_fileMgr->CreateNewFile(path))
					{
						// see if there is post data to save into the new file
						if(dataRead && size && bytes)
						{
							if(m_fileMgr->SaveFile(path, (char*)bytes, size))
								resp.responseStatus = responseStatus201;
							else
								resp.responseStatus = responseStatus500;
						}
						else
						{
							resp.responseStatus = responseStatus201;
						}
					}
					else
					{
						resp.responseStatus = responseStatus500;
					}
				}
				else
				{
					LogMessage(L"     File already exists.");
					resp.responseStatus = responseStatus400;
				}
			}
			else if(NinjaUtilities::CompareStringsNoCase(request_info->request_method, "put") == 0) // Save, Copy or Move an existing file
			{
				resp.responseStatus = responseStatus500;
				const char *overwriteDest = mg_get_header(conn, "overwrite-destination"),
					*deleteSource = mg_get_header(conn, "delete-source"), *srcHdr = mg_get_header(conn, "sourceURI");

				if(srcHdr)  // its a copy/move operation
				{
					std::string srcURI = srcHdr;
#ifdef _WINDOWS
					if(srcURI.find(L'/') == 0) // remove leading / char
						srcURI.erase(0, 1);
#endif
					std::wstring srcURIw;
					NinjaUtilities::StringToWString(srcURI, srcURIw);

					bool overwrite = false, deleteSrc = false; 
					if(overwriteDest && NinjaUtilities::CompareStringsNoCase(overwriteDest, "true") == 0)
						overwrite = true;
					if(deleteSource && NinjaUtilities::CompareStringsNoCase(deleteSource, "true") == 0)
						deleteSrc = true;

					if(deleteSrc) // its a rename
					{
						LogMessage(L"     Method = PUT. Renaming file");

						bool canContinue = true;
						if(overwrite && m_fileMgr->FileExists(path))
							canContinue = m_fileMgr->DeleteFile(path);

						if(canContinue && m_fileMgr->RenameFile(srcURIw, path))
							resp.responseStatus = responseStatus204; // success
					}
					else // its a copy
					{
						LogMessage(L"     Method = PUT. Copying file");

						if(m_fileMgr->CopyFile(srcURIw, path, overwrite))
							resp.responseStatus = responseStatus204; // success
					}
					
					LogMessage(L"          Source Path: %ls", srcURIw.c_str());
					LogMessage(L"          Dest Path: %ls", path.c_str());
				}
				else // its a simple file save
				{
					if(m_fileMgr->FileExists(path))
					{	
						LogMessage(L"     Method = PUT. Saving file");

						if(dataRead && size && bytes)
						{
							if(m_fileMgr->SaveFile(path, (char*)bytes, size))
								resp.responseStatus = responseStatus204;
							else
								resp.responseStatus = responseStatus500;
						}
						else
							resp.responseStatus = responseStatus500;
					}
					else
						resp.responseStatus = responseStatus404;
				}
			}
			else if(NinjaUtilities::CompareStringsNoCase(request_info->request_method, "delete") == 0) // delete an existing file
			{
				LogMessage(L"     Method = DELETE. Deleting file.");

				resp.responseStatus = responseStatus500;
				bool fileExists = m_fileMgr->FileExists(path);
				if(fileExists)
				{
					if(m_fileMgr->DeleteFile(path))
						resp.responseStatus = responseStatus204;
				}
				else
				{
					resp.responseStatus = responseStatus404;
				}
			}
			else if(NinjaUtilities::CompareStringsNoCase(request_info->request_method, "get") == 0) // Read an existing file 
			{
				LogMessage(L"     Method = GET. Reading a file.");

				char *fileBytes = NULL;
				unsigned int contentLen = 0;
				const char *chkForExistence = mg_get_header(conn, "check-existence-only");
				const char *ifModifiedSince = mg_get_header(conn, "if-modified-since");
                const char *getFileInfo = mg_get_header(conn, "get-file-info");

				bool fileExists = m_fileMgr->FileExists(path);
				if(ifModifiedSince && strlen(ifModifiedSince)) // its a query to see if the file has been changed
				{
					if(fileExists)
					{
						unsigned long long createTime = 0, modTime = 0;
						if(m_fileMgr->GetFileTimes(path, createTime, modTime))
						{
							unsigned long long val = 0;
#ifdef _WINDOWS							
							val = _atoi64(ifModifiedSince);
#else // MAC
							val = atoll(ifModifiedSince);
#endif
							if(createTime > val || modTime > val)
								resp.responseStatus = responseStatus200;
							else
								resp.responseStatus = responseStatus304;
						}
					}
					else
					{
						resp.responseStatus = responseStatus404;
					}
				}
				else if(chkForExistence && NinjaUtilities::CompareStringsNoCase(chkForExistence, "true") == 0) // checking for file existence only
				{
					if(fileExists)
						resp.responseStatus = responseStatus204;
					else
						resp.responseStatus = responseStatus404;
				}
				else if(getFileInfo && NinjaUtilities::CompareStringsNoCase(getFileInfo, "true") == 0) // query the file info only
				{
					if(fileExists)
                    {
                        unsigned long long createTime = 0, modTime = 0, fileSize = 0;
                        m_fileMgr->GetFileTimes(path, createTime, modTime);
                        m_fileMgr->GetFileSize(path, fileSize);
                        bool readOnly = !m_fileMgr->FileIsWritable(path);

                        std::stringstream infoStr;

                        infoStr << "{\"creationDate\": \"" << createTime << "\",";
                        infoStr << "\"modifiedDate\": \"" << modTime << "\",";
                        infoStr << "\"size\": \"" << fileSize << "\",";
                        infoStr << "\"readOnly\": \"" << (readOnly ? "true" : "false") << "\"}";

                        resp.contentType = "application/json";
                        resp.responseBody = infoStr.str();
						resp.responseStatus = responseStatus200;
                    }
					else
						resp.responseStatus = responseStatus404;
				}
				else 
				{
					if(m_fileMgr->ReadFile(path, &fileBytes, contentLen))
					{
						if(contentLen > 0 && fileBytes)
							resp.responseBody = fileBytes;

						resp.responseStatus = responseStatus200;						
					}
					else
					{
						resp.responseStatus = responseStatus404;
					}
				}
			}

			if(dataRead && bytes)
				delete [] bytes;

			LogMessage("     Response: %s", responseStatusDescriptions[resp.responseStatus]);
			std::string respStr = resp.GetResponsString();
			mg_write(conn, respStr.c_str(), respStr.length());
		}
	}
	catch(...)
	{
		
	}

	return ret;
}

bool DirContentsChanged(NinjaFileIO::FileIOManager *fMgr, std::wstring &path, bool recursive, std::wstring &filterList, unsigned long long ifModSinceTime, time_t requestStartTime, bool &requestTimedOut)
{
	bool ret = false;
	NinjaFileIO::FileSystemNode rootNode;
	rootNode.uri = path;
	NinjaUtilities::PathUtility pathUtil;
	pathUtil.SetPath(path.c_str());
	pathUtil.GetDirectoryOrFilename(rootNode.name);
	rootNode.type = NinjaFileIO::FileSystemNode::directory;

	NinjaFileIO::FileSystemNodeList dirContents;
	unsigned long long createTime = 0, modTime = 0;

	bool readRet = fMgr->ReadDirectory(rootNode.uri, NinjaFileIO::allDirectoryContents, filterList, dirContents, requestStartTime, requestTimedOut);
	if(readRet)
	{
		if(dirContents.size()) 
		{
			NinjaFileIO::FileSystemNode childNode;
			NinjaFileIO::FileSystemNodeList::const_iterator it;
			for(it = dirContents.begin(); it != dirContents.end() && !ret; it++)
			{
				childNode = (*it);

				if(childNode.type == NinjaFileIO::FileSystemNode::file)
				{
					if(fMgr->GetFileTimes(childNode.uri, createTime, modTime))
					{
						if(createTime > ifModSinceTime || modTime > ifModSinceTime)
							ret = true;
					}
				}
				else if(childNode.type == NinjaFileIO::FileSystemNode::directory)
				{
					if(fMgr->GetDirectoryTimes(childNode.uri, createTime, modTime))
					{
						if(createTime > ifModSinceTime || modTime > ifModSinceTime)
							ret = true;
					}

					if(!ret && recursive)
					{
						ret = DirContentsChanged(fMgr, childNode.uri, recursive, filterList, ifModSinceTime, requestStartTime, requestTimedOut);
					}
				}
				if(!ret && (time(NULL) - requestStartTime) > READ_DIR_TIMEOUT_SECONDS)
				{
					requestTimedOut = true;
					break;
				}
			}
		}
	}

	return ret;
}


// returns true if text was actually output. false if nothing was added to the string stream.
bool DirContentsToJSON(NinjaFileIO::FileIOManager *fMgr,
	const NinjaFileIO::FileSystemNode &node, 
	NinjaFileIO::DirectoryContentTypes retTypes,
	bool isHeadNode, bool forceLeadingComma,
	bool recursive, std::wstring &filterList, std::wostringstream &strOut, time_t requestStartTime, bool &requestTimedOut)
{	
	bool ret = false;

	if(fMgr)
	{
		// see if this node should be output
		if(isHeadNode || retTypes == NinjaFileIO::allDirectoryContents || 
			(retTypes == NinjaFileIO::filesOnly && node.type == NinjaFileIO::FileSystemNode::file) ||
			(retTypes == NinjaFileIO::directoriesOnly && node.type == NinjaFileIO::FileSystemNode::directory))
		{
			ret = true; // we are going to write out text

			if(forceLeadingComma)
				strOut.put(L','); // we were told to add a delimiter before outputting this node

			strOut.put(L'{');

			strOut << L"\"type\":";
			if(node.type == NinjaFileIO::FileSystemNode::file)
				strOut << L"\"file\"";
			else if(node.type == NinjaFileIO::FileSystemNode::directory)
				strOut << L"\"directory\"";
			else
				strOut << L"\"unknown\"";

			strOut << L",\"name\":\"" << node.name << L"\"";


			strOut << L",\"uri\":\"" << node.uri << L"\"";
            if(isHeadNode)
            {
                unsigned long long createdTime = 0, modifiedTime = 0;
                fMgr->GetDirectoryTimes(node.uri, createdTime, modifiedTime);
                //strOut << L",\"size\":\"" << node.filesize << L"\""; we don't have the size info for the root/head node so don't report it
                strOut << L",\"creationDate\":\"" << createdTime << L"\"";
                strOut << L",\"modifiedDate\":\"" << modifiedTime << L"\"";
           }
            else
			{
				strOut << L",\"size\":\"" << node.filesize << L"\"";
				strOut << L",\"creationDate\":\"" << node.creationDate << L"\"";
				strOut << L",\"modifiedDate\":\"" << node.modifiedDate << L"\"";
				
				strOut << L",\"writable\":";
				if(node.isWritable)
					strOut << L"\"true\"";
				else
					strOut << L"\"false\"";
			}

			if(node.type == NinjaFileIO::FileSystemNode::directory && (recursive || isHeadNode))
			{
				NinjaFileIO::FileSystemNodeList dirContents;

				bool readRet = fMgr->ReadDirectory(node.uri, retTypes, filterList, dirContents, requestStartTime, requestTimedOut);
				if(readRet)
				{
					if(dirContents.size()) // not all responses will have a return body. A test for "modified since" may only return 304 and no body
					{
						strOut << L",\"children\":[";

						bool lastCallOutputText = false;
						NinjaFileIO::FileSystemNode childNode;
						NinjaFileIO::FileSystemNodeList::const_iterator it;
						for(it = dirContents.begin(); it != dirContents.end(); it++)
						{
							childNode = (*it);

							// pass true for forceLeadingComma if the preceding call did output an item. 
							lastCallOutputText = DirContentsToJSON(fMgr, childNode, retTypes, false, lastCallOutputText, recursive, filterList, strOut, requestStartTime, requestTimedOut);
							if((time(NULL) - requestStartTime) > READ_DIR_TIMEOUT_SECONDS)
							{
								requestTimedOut = true;
								break;
							}
						}

						strOut << L"]";
					}
				}
			}

			strOut.put(L'}');
		}
	}

	return ret;
}

bool CHttpServerWrapper::HandleDirectoryServiceRequest(mg_connection *conn, const mg_request_info *request_info)
{
	bool ret = false;

	try
	{
		if(conn && request_info)
		{
			std::string urlstr = request_info->uri;
			std::wstring url, path;
			NinjaUtilities::StringToWString(urlstr, url);
			url = url.erase(0, wcslen(NINJA_DIRECTORYAPI_URL_TOKEN));
			NinjaUtilities::PathUtility pathUtil;
			pathUtil.SetPath(url.c_str());
			path = url;
			
#ifdef _WINDOWS
			if(path.find(L'/') == 0) // remove leading / char
				path.erase(0, 1);

            if(path.length())
            {
			    int firstSlash = path.find(L'/');
			    if(firstSlash != -1)
				    path.insert(firstSlash, L":"); // insert the : before the first slash
                else
                    path.append(L":/"); // no slash means this is a root directory so add the :/
            }
#endif

			LogMessage(L"Directory service request with path=\"%ls\"", path.c_str());

			// process the request
			HttpResponseData resp;
			resp.responseStatus =  responseStatus501;
			resp.contentType = "text/plain, charset=utf-8";
			const char *origHdr = mg_get_header(conn, "ORIGIN");			
			if(origHdr)
				resp.origin = origHdr;

            bool pathIsUnderDocRoot = PathIsUnderDocumentRoot(path);          

			size_t size = 0;
			byte* bytes = NULL;
			bool dataRead = GetRequestDataHelper(conn, request_info, &bytes, size);

			if(NinjaUtilities::CompareStringsNoCase(request_info->request_method, "options") == 0) 
			{
				// this is a cross origin preflight request from the browser so we simply need to response
				// with the correct cross origin responses.
				LogMessage(L"     Method = OPTIONS. Handling Cross Origin Preflight.");
				resp.responseStatus = responseStatus200;
			}
			else if(NinjaUtilities::CompareStringsNoCase(request_info->request_method, "post") == 0) // create a new directory
			{

                if(!limitIOToRootDirectory || (limitIOToRootDirectory && pathIsUnderDocRoot))
                {
				    LogMessage(L"     Method = POST. Creating a new directory.");
				    resp.responseStatus = responseStatus400;
				    if(m_fileMgr->DirectoryExists(path) == false && m_fileMgr->CreateNewDirectory(path))
					    resp.responseStatus = responseStatus201; // created
                }
                else
                    resp.responseStatus = responseStatus403;
			}
			else if(NinjaUtilities::CompareStringsNoCase(request_info->request_method, "delete") == 0) // delete a directory
			{
                if(!limitIOToRootDirectory || (limitIOToRootDirectory && pathIsUnderDocRoot))
                {
				    LogMessage(L"     Method = DELETE. Deleting directory.");
				    if(m_fileMgr->DirectoryExists(path))
				    {
					    if(m_fileMgr->DeleteDirectory(path))
						    resp.responseStatus = responseStatus204; // success
					    else
						    resp.responseStatus = responseStatus500;
				    }
                }
                else
                    resp.responseStatus = responseStatus403;
			}
			else if(NinjaUtilities::CompareStringsNoCase(request_info->request_method, "get") == 0) // Read an existing directory 
			{
				LogMessage(L"     Method = GET. Reading a directory.");
				// get any optional headers
				const char *recursiveHdr = mg_get_header(conn, "recursive"), *returnTypeHdr = mg_get_header(conn, "return-type"), 
					*checkExistenceHdr = mg_get_header(conn, "check-existence-only"),
					*fileFilterHdr = mg_get_header(conn, "file-filters"),
					*ifModifiedSinceHdr = mg_get_header(conn, "if-modified-since");

				bool recursive = false, checkExistence = false, 
					isRootListingQuery = (path.length() == 0); // an empty path indicates that this request is asking for a listing of the top level folders/files available. 
				unsigned long long ifModSinceTime = 0;

				if(recursiveHdr && NinjaUtilities::CompareStringsNoCase(recursiveHdr, "true") == 0)
					recursive = true;
				if(checkExistenceHdr && NinjaUtilities::CompareStringsNoCase(checkExistenceHdr, "true") == 0)
					checkExistence = true;
				if(ifModifiedSinceHdr && strlen(ifModifiedSinceHdr))
				{
#ifdef _WINDOWS							
					ifModSinceTime = _atoi64(ifModifiedSinceHdr);
#else // MAC
					ifModSinceTime = atoll(ifModifiedSinceHdr);
#endif
				}
				
				NinjaFileIO::DirectoryContentTypes retTypes = NinjaFileIO::allDirectoryContents;
				if(returnTypeHdr && NinjaUtilities::CompareStringsNoCase(returnTypeHdr, "directories") == 0)
					retTypes = NinjaFileIO::directoriesOnly;
				if(returnTypeHdr && NinjaUtilities::CompareStringsNoCase(returnTypeHdr, "files") == 0)
					retTypes = NinjaFileIO::filesOnly;

				std::wstring filterList;
				if(fileFilterHdr && strlen(fileFilterHdr))
				{
					std::string f = fileFilterHdr;
					NinjaUtilities::StringToWString(f, filterList);
				}

				if(isRootListingQuery)
				{
					// ignore any parameters for a root listing query
					recursive = false;
					checkExistence = false;
					filterList.empty();
				}
				if(ifModSinceTime > 0)
				{
					// ignore some parameters for a root listing query
					checkExistence = false;
					isRootListingQuery = false;
				}

				bool dirExists = m_fileMgr->DirectoryExists(path);  
				if(dirExists || isRootListingQuery) 
				{		
					if(ifModSinceTime > 0) // its a query to see if the directory or its contents have changed
					{
                        if(!limitIOToRootDirectory || (limitIOToRootDirectory && pathIsUnderDocRoot))
                        {
						    unsigned long long createTime = 0, modTime = 0;
						    if(m_fileMgr->GetDirectoryTimes(path, createTime, modTime))
						    {
							    if(createTime > ifModSinceTime || modTime > ifModSinceTime)
								    resp.responseStatus = responseStatus200;
							    else
							    {
								    time_t requestStartTime = time(NULL);
								    bool requestTimedOut = false;

								    bool contentChanged = DirContentsChanged(m_fileMgr, path, recursive, filterList, ifModSinceTime, requestStartTime, requestTimedOut);
								    if(!requestTimedOut)
								    {
									    if(contentChanged)
										    resp.responseStatus = responseStatus200;
									    else
										    resp.responseStatus = responseStatus304;
								    }
								    else
								    {
									    resp.responseStatus = responseStatus413;
								    }
							    }
						    }
                        }
                        else
                            resp.responseStatus = responseStatus403;
					}
					else if(checkExistence)
					{
                        if(!limitIOToRootDirectory || (limitIOToRootDirectory && pathIsUnderDocRoot))
						    resp.responseStatus = responseStatus204;
                        else
                            resp.responseStatus = responseStatus403;
					}
					else // read the dir contents
					{
                        if(!limitIOToRootDirectory || (limitIOToRootDirectory && pathIsUnderDocRoot))
                        {
                            // read the contents
                            NinjaFileIO::FileSystemNode rootNode;
                            rootNode.uri = path;
                            pathUtil.GetDirectoryOrFilename(rootNode.name);
                            rootNode.type = NinjaFileIO::FileSystemNode::directory;

                            std::wostringstream jsonStr;
                            time_t requestStartTime = time(NULL);
                            bool requestTimedOut = false;
                            DirContentsToJSON(m_fileMgr, rootNode, retTypes, true, false, recursive, filterList, jsonStr, requestStartTime, requestTimedOut);

                            if(!requestTimedOut)
                            {
                                std::wstring theJson = jsonStr.str();						
                                if(theJson.length())
                                {
                                    resp.contentType = "application/json";
                                    NinjaUtilities::WStringToString(theJson, resp.responseBody);
                                    resp.responseStatus = responseStatus200; // success
                                }					
                            }
                            else
                            {
                                resp.responseStatus = responseStatus413;
                            }
                        }
                        else
                        {
                            if(isRootListingQuery && limitIOToRootDirectory)
                            {
                                std::wstringstream jsonOut;

                                jsonOut << "{\"type\":\"directory\",\"name\":\"\",\"uri\":\"\",\"creationDate\":\"0\",\"modifiedDate\":\"0\",\"children\":[{\"type\":\"directory\",\"name\":\"";
                                jsonOut << NINJA_SANDBOXED_LOGICALDRIVE;
                                jsonOut << "\",\"uri\":\"";
                                std::wstring tmpRoot = serverRoots;
                                NinjaUtilities::ConvertBackslashtoForwardSlash(tmpRoot);
                                jsonOut << tmpRoot;
                                jsonOut << "\",\"size\":\"0\",\"creationDate\":\"0\",\"modifiedDate\":\"0\",\"writable\":\"true\"}]}";
                                std::wstring theJson = jsonOut.str();			
                                NinjaUtilities::WStringToString(theJson, resp.responseBody);
                                resp.responseStatus = responseStatus200; // success
                                resp.contentType = "application/json";
                            }
                            else
                            {
                                resp.responseStatus = responseStatus403;
                            }
                        }
					}					
				}
				else 
				{
					resp.responseStatus = responseStatus404;
				}
			}
			else if(NinjaUtilities::CompareStringsNoCase(request_info->request_method, "put") == 0) // Copy or Move an existing directory
			{
				// get any optional headers
				const char *sourceURIHdr = mg_get_header(conn, "sourceURI"), *operationHdr = mg_get_header(conn, "operation");
				std::wstring srcPath;
				if(sourceURIHdr)
				{
					std::string tmp = sourceURIHdr;
#ifdef _WINDOWS
					if(tmp.find(L'/') == 0) // remove leading / char
						tmp.erase(0, 1);
#endif
					NinjaUtilities::StringToWString(tmp, srcPath);
				}
				bool isCopy = false, isMove = false;
				if(operationHdr && NinjaUtilities::CompareStringsNoCase(operationHdr, "copy") == 0)
					isCopy = true;
				if(operationHdr && NinjaUtilities::CompareStringsNoCase(operationHdr, "move") == 0)
					isMove = true;

				if(srcPath.length())
				{
					bool srcDirExists = m_fileMgr->DirectoryExists(srcPath), destDirExists = m_fileMgr->DirectoryExists(path);
					if(srcDirExists)
					{
                        bool srcDirIsUnderDocRoot = PathIsUnderDocumentRoot(srcPath);
                        if(!limitIOToRootDirectory || (limitIOToRootDirectory && pathIsUnderDocRoot && srcDirIsUnderDocRoot))
                        {
                            if(destDirExists == false)
                            {
                                if(isCopy) // its a copy operation
                                {
                                    LogMessage(L"     Method = PUT. Copying directory.");

                                    if(m_fileMgr->CopyDirectory(srcPath, path, false))
                                        resp.responseStatus = responseStatus204; // success
                                    else
                                        resp.responseStatus = responseStatus500; // error
                                }
                                else if(isMove)
                                {
                                    LogMessage(L"     Method = PUT. Moving directory.");

                                    if(m_fileMgr->MoveDirectory(srcPath, path))
                                        resp.responseStatus = responseStatus204; // success
                                    else
                                        resp.responseStatus = responseStatus500; // error
                                }
                                else
                                    resp.responseStatus = responseStatus400; // "Bad Request"

                                LogMessage(L"          Source Path: %ls", srcPath.c_str());
                                LogMessage(L"          Dest Path: %ls", path.c_str());

                            }
                            else
                                resp.responseStatus = responseStatus400; // "Bad Request"
                        }
                        else
                            resp.responseStatus = responseStatus403;
					}
				}
			}

			if(dataRead && bytes)
				delete [] bytes;

			LogMessage("     Response: %s", responseStatusDescriptions[resp.responseStatus]);
			std::string respStr = resp.GetResponsString();
			mg_write(conn, respStr.c_str(), respStr.length());
		}
	}
	catch (...)
	{
		ret = false;
	}

	return ret;
}
