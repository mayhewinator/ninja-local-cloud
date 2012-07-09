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

#pragma once
#include "afxwin.h"
#include "WinFileIOManager.h"

class CHttpServerWrapper;
class WinPlatformUtils;

// CCloudSimDlg dialog
class CCloudSimDlg : public CDialogEx
{
// Construction
public:
	CCloudSimDlg(CWnd* pParent = NULL);	// standard constructor
	~CCloudSimDlg();

	void LogMessage(LPCTSTR msg);

    void GetLocalNinjaOrigin(std::wstring &valOut);
    void GetVersionNumber(std::wstring &valOut);

private:
	int m_portNum;
	CString m_rootDir;
    CString m_localNinjaOrigin;
	CHttpServerWrapper *m_server;
	WinPlatformUtils *m_platformUtils;
	NinjaFileIO::WinFileIOManager *m_fileIOManager;
    bool m_loggingEnabled;

	int GetPortNumber();
	LPCTSTR GetRootDir();
	void StartServer();
	void StopServer();
	void Shutdown();
    void UpdateStatus();

    CFont *m_largeFont;
    bool m_advancedOptionsShown;
    void UpdateOptionsState();
    void ShowOptions(bool showOpts);

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	HICON m_hIcon;

	// override to keep these from closing on enter/esc key
	void OnOK() {};
	void OnCancel() {};

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();	
	DECLARE_MESSAGE_MAP()
public:
// Dialog Data
	enum { IDD = IDD_NINJACLOUDSIMULATOR_DIALOG };
	afx_msg void OnBnClickedStartbutton();
	afx_msg void OnBnClickedStopbutton();
	CEdit m_portNumberCtrl;
	CMFCEditBrowseCtrl m_rootDirCtrl;
	CEdit m_logEditCtrl;
	CButton m_startBtn;
	CButton m_stopBtn;
	afx_msg void OnClose();
	afx_msg void OnBnClickedClearlog();
    CStatic m_statusCtrl;
    CString m_baseCaptionString;
    afx_msg void OnCopyURLToClibpoard();
    afx_msg void OnAdvancedOptionsClick();
    afx_msg void OnChangePortNumber();
    afx_msg void OnEnableLoggingClick();
    CButton m_enableLoggingChk;
};
