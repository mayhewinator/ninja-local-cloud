/*
<copyright>
	This file contains proprietary software owned by Motorola Mobility, Inc.
	No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.
	(c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
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
