/*
<copyright>
This file contains proprietary software owned by Motorola Mobility, Inc.
No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.
(c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
</copyright>
*/
#include "StdAfx.h"
#include "WinFileIOManager.h"
#include "..\Core\Utils.h"

#include <ShellAPI.h>
#include <afxinet.h>
#include <sstream>
#include <deque>

namespace NinjaFileIO
{
    // this converts a windows FILETIME to the number of milliseconds since 1/1/1970 UTC
    static unsigned long long int WinFileTimeToUnixEpochMS(FILETIME winTime)
    {
        ULARGE_INTEGER	ul;
        ul.HighPart = winTime.dwHighDateTime;
        ul.LowPart = winTime.dwLowDateTime;

        return ul.QuadPart / 10000LL - 11644473600000LL;
    }  

    WinFileIOManager::WinFileIOManager()
    {
        m_inetSession = NULL;
    }

    WinFileIOManager::~WinFileIOManager()
    {
        if(m_inetSession)
        {
            m_inetSession->Close();
            delete m_inetSession;
            m_inetSession = NULL;
        }
    }

    // we override this to use the native Windows file copy routine
    bool WinFileIOManager::CopyFile(const std::wstring &srcFilename, const std::wstring &destFilename, bool overwriteExistingFile)
    {
        bool ret = false;

        try
        {
            if(srcFilename.length() && destFilename.length())
            {
                if(::CopyFile(srcFilename.c_str(), destFilename.c_str(), overwriteExistingFile == false) != 0)
                    ret = true;
            }
        }
        catch (...)
        {
        }

        return ret;
    }

    bool WinFileIOManager::DeleteFile(const std::wstring &filename)
    {
        bool ret = false;

        try
        {
            HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE); 
            if (SUCCEEDED(hr))
            {
                IFileOperation *fileOp;
                hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_ALL, IID_PPV_ARGS(&fileOp));
                if (SUCCEEDED(hr))
                {
                    hr = fileOp->SetOperationFlags(FOF_NO_UI|FOF_FILESONLY|FOF_ALLOWUNDO);
                    if (SUCCEEDED(hr))
                    {
                        IShellItem *psiFile = NULL;
                        std::wstring filenameCopy = filename;
                        NinjaUtilities::ConvertForwardSlashtoBackslash(filenameCopy); // the call below will fail if the path contains '/'
                        hr = SHCreateItemFromParsingName(filenameCopy.c_str(), NULL, IID_PPV_ARGS(&psiFile));
                        if (SUCCEEDED(hr))
                        {
                            // now add the delete operation
                            hr = fileOp->DeleteItem(psiFile, NULL);

                            // perform the operation
                            if (SUCCEEDED(hr))
                            {
                                hr = fileOp->PerformOperations();
                                ret = SUCCEEDED(hr);
                            }

                            psiFile->Release();
                        }
                    }

                    fileOp->Release();
                }

                CoUninitialize();
            }
        }
        catch (...)
        {
        }

        return ret;
    }

    bool WinFileIOManager::GetFileTimes(const std::wstring &filename, unsigned long long &createdTime, unsigned long long &modifiedTime)
    {
        bool ret = false;

        HANDLE hFile = INVALID_HANDLE_VALUE;
        try 
        {
            if(filename.length())
            {
                hFile = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
                if(hFile != INVALID_HANDLE_VALUE)
                {
                    FILETIME ftCreate, ftAccess, ftWrite;

                    // Retrieve the file times for the file.
                    if (GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
                    {
                        createdTime = WinFileTimeToUnixEpochMS(ftCreate);
                        modifiedTime = WinFileTimeToUnixEpochMS(ftWrite);

                        ret = true;
                    }

                    CloseHandle(hFile);  
                }
            }
        }
        catch (...) 
        {
            if(hFile != INVALID_HANDLE_VALUE)
                CloseHandle(hFile);  
        }

        return ret;
    }

    bool WinFileIOManager::GetFileSize(const std::wstring &filename, unsigned long long &fileSize)
    {
        bool ret = false;

        HANDLE hFile = INVALID_HANDLE_VALUE;
        try 
        {
            if(filename.length())
            {
                hFile = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
                if(hFile != INVALID_HANDLE_VALUE)
                {
                    LARGE_INTEGER s;
                    if(::GetFileSizeEx(hFile, &s))
                    {
                        fileSize = (unsigned long long)s.QuadPart;
                        ret = true;
                    }

                    CloseHandle(hFile);  
                }
            }
        }
        catch (...) 
        {
            if(hFile != INVALID_HANDLE_VALUE)
                CloseHandle(hFile);  
        }

        return ret;
    }

    bool WinFileIOManager::FileIsWritable(const std::wstring &filename)
    {
        bool ret = false;

        try 
        {
            if(filename.length())
            {
                DWORD attr = GetFileAttributes(filename.c_str());
                if(attr != INVALID_FILE_ATTRIBUTES)
                    ret = (attr & FILE_ATTRIBUTE_READONLY) == 0;
            }
        }
        catch (...) 
        {
        }

        return ret;
    }

    bool WinFileIOManager::DeleteDirectory(const std::wstring &path)
    {
        bool ret = false;

        try
        {
            HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE); 
            if (SUCCEEDED(hr))
            {
                IFileOperation *fileOp;
                hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_ALL, IID_PPV_ARGS(&fileOp));
                if (SUCCEEDED(hr))
                {
                    hr = fileOp->SetOperationFlags(FOF_NO_UI|FOF_ALLOWUNDO);
                    if (SUCCEEDED(hr))
                    {
                        IShellItem *psiItem = NULL;
                        std::wstring pathCopy = path;
                        NinjaUtilities::ConvertForwardSlashtoBackslash(pathCopy); // the call below will fail if the path contains '/'
                        hr = SHCreateItemFromParsingName(pathCopy.c_str(), NULL, IID_PPV_ARGS(&psiItem));
                        if (SUCCEEDED(hr))
                        {
                            // now add the delete operation
                            hr = fileOp->DeleteItem(psiItem, NULL);

                            // perform the operation
                            if (SUCCEEDED(hr))
                            {
                                hr = fileOp->PerformOperations();
                                ret = SUCCEEDED(hr);
                            }

                            psiItem->Release();
                        }
                    }

                    fileOp->Release();
                }

                CoUninitialize();
            }
        }
        catch (...)
        {
        }

        return ret;
    }

    bool WinFileIOManager::CopyDirectory(const std::wstring &srcPath, const std::wstring &destPath, bool overwriteExistingDirectory)
    {
        bool ret = false;

        try
        {
            std::wstring destDirPath, destDir;
            std::wstring::size_type strIndex = destPath.find_last_of(PATH_SEPARATOR_CHAR, -1);
            destDirPath = destPath.substr(0, strIndex);
            destDir = destPath.substr(strIndex + 1, destPath.length() - strIndex - 1);

            if(srcPath.length() && DirectoryExists(srcPath))
            {
                if(overwriteExistingDirectory && DirectoryExists(destPath))
                {
                    // remove the existing directory
                    DeleteDirectory(destPath);
                }

                HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE); 
                if (SUCCEEDED(hr))
                {
                    IFileOperation *fileOp;
                    hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_ALL, IID_PPV_ARGS(&fileOp));
                    if (SUCCEEDED(hr))
                    {
                        hr = fileOp->SetOperationFlags(FOF_NO_UI|FOF_ALLOWUNDO);
                        if (SUCCEEDED(hr))
                        {
                            IShellItem *psiFrom = NULL;
                            std::wstring srcPathCopy = srcPath;
                            NinjaUtilities::ConvertForwardSlashtoBackslash(srcPathCopy); // the call below will fail if the path contains '/'
                            hr = SHCreateItemFromParsingName(srcPathCopy.c_str(), NULL, IID_PPV_ARGS(&psiFrom));
                            if (SUCCEEDED(hr))
                            {
                                IShellItem *psiTo = NULL;

                                std::wstring destDirPathCopy = destDirPath;
                                NinjaUtilities::ConvertForwardSlashtoBackslash(destDirPathCopy); // the call below will fail if the path contains '/'
                                hr = SHCreateItemFromParsingName(destDirPathCopy.c_str(), NULL, IID_PPV_ARGS(&psiTo));
                                if (SUCCEEDED(hr))
                                {
                                    // now add the copy operation
                                    hr = fileOp->CopyItem(psiFrom, psiTo, destDir.c_str(), NULL);

                                    if (psiTo != NULL)
                                    {
                                        psiTo->Release();
                                    }
                                }

                                psiFrom->Release();
                            }

                            // perform the operation
                            if (SUCCEEDED(hr))
                            {
                                hr = fileOp->PerformOperations();
                                ret = SUCCEEDED(hr);
                            }
                        }

                        fileOp->Release();
                    }

                    CoUninitialize();
                }
            }
        }
        catch(...)
        {
        }

        return ret;
    }


    bool WinFileIOManager::MoveDirectory(const std::wstring &path, const std::wstring &newPath)
    {
        bool ret = false;

        try
        {
            if(path.length() && DirectoryExists(path) && !DirectoryExists(newPath))
            {
                if(::MoveFileEx(path.c_str(),	newPath.c_str(), MOVEFILE_WRITE_THROUGH) == TRUE)
                    ret = true;
            }
        }
        catch(...)
        {
        }

        return ret;
    }

    bool WinFileIOManager::DirectoryIsEmpty(const std::wstring &path)
    {
        bool ret = false;

        HANDLE srchHandle = INVALID_HANDLE_VALUE;
        try
        {
            if(path.length() && DirectoryExists(path))
            {
                WIN32_FIND_DATA fd;

                NinjaUtilities::PathUtility pathUtil(path.c_str());

                std::wstring srchPath;
                pathUtil.CreateSearchPath(srchPath);			

                ret = true; // assume it is empty unless we actually find something in the directory
                srchHandle = ::FindFirstFileEx(srchPath.c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch , NULL, FIND_FIRST_EX_LARGE_FETCH);
                if(srchHandle != INVALID_HANDLE_VALUE)
                {
                    do 
                    {	
                        if(wcscmp(fd.cFileName, L".") != 0 && wcscmp(fd.cFileName, L"..") != 0)
                        {
                            ret = false;
                            break;
                        }
                    } while (::FindNextFile(srchHandle, &fd));

                    FindClose(srchHandle);
                }
            }
        }
        catch(...)
        {
            if(srchHandle != INVALID_HANDLE_VALUE)
                FindClose(srchHandle);
        }

        return ret;
    }

    bool WinFileIOManager::ReadDirectory(const std::wstring &path, DirectoryContentTypes types, const std::wstring &filterList, FileSystemNodeList &dirContents,
        time_t requestStartTime, bool &requestTimedOut)
    {
        bool ret = false;

        HANDLE srchHandle = INVALID_HANDLE_VALUE;
        try
        {
            // an empty path indicates that we should respond with the logical drives for a local system, or the list of top level folders for a cloud account
            if(path.length() == 0)
            {
                wchar_t driveStrBuffer[501];
                memset(driveStrBuffer, 0, sizeof(wchar_t)*501);
                if(GetLogicalDriveStrings(500, driveStrBuffer) > 0)
                {
                    if(wcslen(driveStrBuffer))
                    {
                        ret = true;

                        std::vector<std::wstring> logicalDrives;
                        wchar_t *drvStr = driveStrBuffer, *delim = NULL;
                        do 
                        {
                            logicalDrives.push_back(drvStr);

                            drvStr += wcslen(drvStr) + 1;
                        } while (*drvStr != 0);

                        FileSystemNode node;
                        node.type = FileSystemNode::directory;
                        node.creationDate = 0; // not currently available for logical drives in this api
                        node.modifiedDate = 0; // not currently available for logical drives in this api	
                        node.filesize = 0;   
                        node.isWritable = true;

                        std::wstring logDrv;
                        for(std::vector<std::wstring>::const_iterator it = logicalDrives.begin(); it != logicalDrives.end(); it++)
                        {
                            logDrv = (*it);
                            UINT dt = GetDriveType(logDrv.c_str()); 
                            if(dt == DRIVE_REMOVABLE || dt == DRIVE_FIXED || dt == DRIVE_REMOTE)
                            {
                                NinjaUtilities::ConvertBackslashtoForwardSlash(logDrv);

                                node.name = logDrv;
                                node.uri = logDrv;																

                                ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
                                if(GetDiskFreeSpaceEx(logDrv.c_str(), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes))
                                    node.filesize = totalNumberOfBytes.QuadPart;  

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
                }
            }
            else if(DirectoryExists(path))
            {
                WIN32_FIND_DATA fd;

                NinjaUtilities::PathUtility pathUtil(path.c_str());

                std::wstring srchPath;
                pathUtil.CreateSearchPath(srchPath);			

                FINDEX_SEARCH_OPS schOps = FindExSearchNameMatch;
                if(types == directoriesOnly)
                    schOps = FindExSearchLimitToDirectories;

                // handle the filters list if it is specified
                std::vector<std::wstring> filters;
                if(types != directoriesOnly && filterList.length())
                    NinjaUtilities::SplitWString(filterList, L';', filters);

                // iterate over the directory
                srchHandle = ::FindFirstFileEx(srchPath.c_str(), FindExInfoBasic, &fd,  schOps, NULL, FIND_FIRST_EX_LARGE_FETCH);
                if(srchHandle != INVALID_HANDLE_VALUE)
                {
                    ret = true;

                    FileSystemNode node;
                    std::wstring fdEntryPath;
                    do 
                    {	
                        if(wcscmp(fd.cFileName, L".") != 0 && wcscmp(fd.cFileName, L"..") != 0)
                        {
                            fdEntryPath = path + PATH_SEPARATOR_CHAR + fd.cFileName;
                            if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                node.type = FileSystemNode::directory;
                            else
                                node.type = FileSystemNode::file;

                            // check to see if this file is in the filter list
                            if(node.type == FileSystemNode::file && filters.size() > 0)
                            {
                                bool fileMatchesFilter = false;
                                std::wstring f;
                                for(std::vector<std::wstring>::const_iterator it = filters.begin(); it != filters.end(); it++)
                                {
                                    f = (*it);
                                    if(NinjaUtilities::FileExtensionMatches(fd.cFileName, f.c_str()) == true)
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
                                node.name = fd.cFileName;
                                node.uri = fdEntryPath;																
                                node.creationDate = WinFileTimeToUnixEpochMS(fd.ftCreationTime);
                                node.modifiedDate = WinFileTimeToUnixEpochMS(fd.ftLastWriteTime);	
                                ULARGE_INTEGER	fsUL;
                                fsUL.HighPart = fd.nFileSizeHigh;
                                fsUL.LowPart = fd.nFileSizeLow;
                                node.filesize = fsUL.QuadPart;
                                if(node.type == FileSystemNode::file)
                                    node.isWritable = ((fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0);
                                else
                                    node.isWritable = true; // directories are always writable. see http://support.microsoft.com/kb/326549

                                dirContents.push_back(node);
                            }
                        }

                        if((time(NULL) - requestStartTime) > READ_DIR_TIMEOUT_SECONDS)
                        {
                            ret = false;
                            requestTimedOut = true;
                            break;
                        }
                    } while (::FindNextFile(srchHandle, &fd));
                    FindClose(srchHandle);
                }
            }
        }
        catch(...)
        {
            if(srchHandle != INVALID_HANDLE_VALUE)
                FindClose(srchHandle);
        }

        return ret;
    }

    bool WinFileIOManager::GetDirectoryTimes(const std::wstring &path, unsigned long long &createdTime, unsigned long long &modifiedTime)
    {
        bool ret = false;

        HANDLE hFile = INVALID_HANDLE_VALUE;
        try 
        {
            if(path.length())
            {
                hFile = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
                if(hFile != INVALID_HANDLE_VALUE)
                {
                    FILETIME ftCreate, ftAccess, ftWrite;

                    // Retrieve the file times for the file.
                    if (GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
                    {
                        createdTime = WinFileTimeToUnixEpochMS(ftCreate);
                        modifiedTime = WinFileTimeToUnixEpochMS(ftWrite);

                        ret = true;
                    }

                    CloseHandle(hFile);  
                }
            }
        }
        catch (...) 
        {
            if(hFile != INVALID_HANDLE_VALUE)
                CloseHandle(hFile);  
        }

        return ret;
    }

    bool WinFileIOManager::ReadTextFromURL(const std::wstring &url, char **fileContents, unsigned int &contentLength)
    {
        bool ret = false;

        try 
        {
            if(url.length() && fileContents)
            {
                if(m_inetSession == NULL)
                    m_inetSession = new CInternetSession(_T("Ninja Local Cloud"));

                std::stringstream sstr;
                char szBuff[1024];
                memset(szBuff, 0, 1024);
                CHttpFile* pFile = NULL;
                pFile = dynamic_cast<CHttpFile*>(m_inetSession->OpenURL(url.c_str(), 0, 
                    INTERNET_FLAG_TRANSFER_ASCII|INTERNET_FLAG_RELOAD|INTERNET_FLAG_DONT_CACHE|INTERNET_FLAG_EXISTING_CONNECT, NULL, 0));

                if(pFile)
                {
                    unsigned int bytesRead = 0, totalBytes = 0;
                    while ((bytesRead = pFile->Read(szBuff, 1023)) > 0)
                    {         
                        sstr << szBuff;
                        memset(szBuff, 0, 1024);
                        totalBytes += bytesRead;
                    }
                    pFile->Close();
                    delete pFile;

                    std::string respStr = sstr.str();

                    if(respStr.length() > totalBytes)
                        respStr.resize(totalBytes);

                    if(respStr.length() > 0)
                    {
                        contentLength = totalBytes;
                        *fileContents = new char[contentLength + 1];
                        memset(*fileContents, 0, sizeof(char)*contentLength + 1);

                        if(*fileContents)
                        {
                            strcpy_s(*fileContents, contentLength+1, respStr.c_str());
                            ret = true;
                        }
                    }
                }
            }
        }
        catch(CInternetException *iex)
        {
            iex;            
        }
        catch (...) 
        {
        }

        return ret;
    }

    bool WinFileIOManager::ReadBinaryFromURL(const std::wstring &url, unsigned char **fileContents, unsigned int &contentLength)
    {
        bool ret = false;

        try 
        {
            if(url.length() && fileContents)
            {
                if(m_inetSession == NULL)
                    m_inetSession = new CInternetSession(_T("Ninja Local Cloud"));

                UINT bytesRead = 0, totalBytesRead = 0;
                std::deque<byte*> outputBuffer;
                std::deque<unsigned int> outputBufferSizes;
                byte readBuff[1024];
                CHttpFile* pFile = NULL;
                pFile = dynamic_cast<CHttpFile*>(m_inetSession->OpenURL(url.c_str(), 0, 
                    INTERNET_FLAG_TRANSFER_BINARY|INTERNET_FLAG_RELOAD|INTERNET_FLAG_DONT_CACHE|INTERNET_FLAG_EXISTING_CONNECT, NULL, 0));
                if(pFile)
                {
                    while((bytesRead = pFile->Read(readBuff, 1024)) > 0)
                    {         
                        byte *tmpBuff = new byte[bytesRead];
                        memcpy_s(tmpBuff, bytesRead, readBuff, bytesRead);
                        outputBuffer.push_back(tmpBuff);
                        outputBufferSizes.push_back(bytesRead);
                        totalBytesRead += bytesRead;
                    }
                    pFile->Close();
                    delete pFile;

                    if(totalBytesRead > 0)
                    {
                        *fileContents = new byte[totalBytesRead];
                        if(*fileContents)
                        {
                            byte *outBuf = NULL;
                            unsigned int outBufLen = 0, pos = 0;
                            for(unsigned int i = 0; i < outputBuffer.size(); i++)
                            {
                                outBuf = outputBuffer[i];
                                outBufLen = outputBufferSizes[i];
                                memcpy_s((*fileContents) + pos, outBufLen, outBuf, outBufLen);
                                pos += outBufLen;
                            }
                            contentLength = totalBytesRead;
                            ret = true;
                        }
                    }
                }
            }
        }
        catch(CInternetException *iex)
        {
            iex;            
        }
        catch (...) 
        {
        }

        return ret;
    }
}