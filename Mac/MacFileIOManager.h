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
#ifndef __MacFileIOManager__
#define __MacFileIOManager__

#include "../Core/FileIOManager.h"

namespace NinjaFileIO
{
	class MacFileIOManager : public FileIOManager
	{
	public:
		MacFileIOManager();
		virtual ~MacFileIOManager();
		
		bool FileIsWritable(const std::wstring &filename);
		
		bool CopyFile(const std::wstring &srcFilename, const std::wstring &destFilename, bool overwriteExistingFile);
		bool DeleteFile(const std::wstring &filename);
		bool GetFileTimes(const std::wstring &filename, unsigned long long &createdTime, unsigned long long &modifiedTime);
        bool GetFileSize(const std::wstring &filename, unsigned long long &fileSize);
		
		bool DeleteDirectory(const std::wstring &path);
		
		bool CopyDirectory(const std::wstring &srcPath, const std::wstring &destPath, bool overwriteExistingDirectory);
		bool MoveDirectory(const std::wstring &path, const std::wstring &newPath);
		bool ReadDirectory(const std::wstring &path, DirectoryContentTypes types, const std::wstring &filterList, FileSystemNodeList &dirContents, 
						   time_t requestStartTime, bool &requestTimedOut); 
		bool DirectoryIsEmpty(const std::wstring &path);
		bool GetDirectoryTimes(const std::wstring &path, unsigned long long &createdTime, unsigned long long &modifiedTime);
			
        bool ReadTextFromURL(const std::wstring &url, char **fileContents, unsigned int &contentLength); // caller must free fileContents
        bool ReadBinaryFromURL(const std::wstring &url, unsigned char **fileContents, unsigned int &contentLength); // caller must free fileContents

	private:
//		void FileOpenHelper(const std::wstring &initialPath, bool directoryMode, const std::wstring &title, std::vector<std::wstring> &uRIsOut);
	};
}
#endif // __MacFileIOManager__

