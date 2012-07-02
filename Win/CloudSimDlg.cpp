/*
<copyright>
	This file contains proprietary software owned by Motorola Mobility, Inc.
	No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.
	(c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
</copyright>
*/

#include "stdafx.h"
#include "NinjaCloudSimulator.h"
#include "CloudSimDlg.h"
#include "afxdialogex.h"

#include "..\Core\HttpServerWrapper.h"
#include "..\Core\Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


static int gDPIX = 96;
static int gDPIY = 96;

// DPI resolution independence helper methods
int ScaleX(int x) { return MulDiv(x, gDPIX, 96); }
int ScaleY(int y) { return MulDiv(y, gDPIY, 96); }
int PointsToPixels(int pt) { return MulDiv(pt, gDPIY, 72); }


// CAboutDlg dialog used for App About
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    CStatic m_versionNumberCtrl;

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_ABOUT_VERSIONCTRL, m_versionNumberCtrl);
    
    if(pDX->m_bSaveAndValidate == FALSE)
    {
        TCHAR fname[_MAX_PATH];
        fname[0] = 0;
        if(::GetModuleFileName(AfxGetInstanceHandle( ), fname, _MAX_PATH))
        {
            DWORD dwH = 0, viSize = ::GetFileVersionInfoSize(fname, &dwH);
            if(viSize)
            {
                VS_FIXEDFILEINFO *pInfo;
                UINT nSize;
                LPVOID pBuf = malloc(viSize);
                if(::GetFileVersionInfo(fname, 0, viSize, pBuf) && ::VerQueryValue(pBuf, TEXT("\\"), (LPVOID*)(&pInfo), &nSize))
                {
                    CString verStr;
                    verStr.Format(L"%u.%u.%u.%u", HIWORD(pInfo->dwFileVersionMS), LOWORD(pInfo->dwFileVersionMS), HIWORD(pInfo->dwFileVersionLS), LOWORD(pInfo->dwFileVersionLS));                    
                    m_versionNumberCtrl.SetWindowText(verStr);
                }
                delete pBuf;
            }
        }
    }
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


class WinPlatformUtils : public NinjaUtilities::PlatformUtility
{
public:
	WinPlatformUtils(CCloudSimDlg *dlg);

	void LogMessage(const wchar_t *msg);
    bool GetLocalNinjaOrigin(std::wstring &valueOut); 
    bool GetVersionNumber(std::wstring &valueOut);

	bool GetPreferenceBool(const wchar_t *key, bool defaultValue);
	int GetPreferenceInt(const wchar_t *key, int defaultValue);
	double GetPreferenceDouble(const wchar_t *key, double defaultValue);
	void GetPreferenceString(const wchar_t *key, std::wstring valueOut, const wchar_t *defaultValue);
	
private:
	CCloudSimDlg *m_hostDlg;
};

WinPlatformUtils::WinPlatformUtils(CCloudSimDlg *dlg)
{
	m_hostDlg = dlg;
}

void WinPlatformUtils::LogMessage(const wchar_t *msg)
{
	if(msg && m_hostDlg)
		m_hostDlg->LogMessage(msg);
}

bool WinPlatformUtils::GetLocalNinjaOrigin(std::wstring &valueOut) 
{
    bool ret = false;

    if(m_hostDlg)
    {
        m_hostDlg->GetLocalNinjaOrigin(valueOut);
        ret = true;
    }

    return ret;
}

bool WinPlatformUtils::GetVersionNumber(std::wstring &valueOut)
{
    bool ret = false;

    if(m_hostDlg)
    {
        m_hostDlg->GetVersionNumber(valueOut);
        ret = true;
    }

    return ret;
}

bool WinPlatformUtils::GetPreferenceBool(const wchar_t *key, bool defaultValue)
{
	bool ret = defaultValue;
	
	if(key && wcslen(key))
	{
		CWinApp *ncsApp = AfxGetApp();
		ret = ncsApp->GetProfileInt(OPTIONS_REG_KEY, key, defaultValue == true ? 1 : 0) > 0;		
	}
	
	return ret;
}

int WinPlatformUtils::GetPreferenceInt(const wchar_t *key, int defaultValue)
{
	int ret = defaultValue;
	
	if(key && wcslen(key))
	{
		CWinApp *ncsApp = AfxGetApp();
		ret = ncsApp->GetProfileInt(OPTIONS_REG_KEY, key, defaultValue);		
	}
	
	return ret;
}

double WinPlatformUtils::GetPreferenceDouble(const wchar_t *key, double defaultValue)
{	
	double ret = defaultValue;
	
	if(key && wcslen(key))
	{
		CWinApp *ncsApp = AfxGetApp();
        wchar_t defVal[100];
        swprintf_s(defVal, L"%d", defaultValue);
		CString keyVal = ncsApp->GetProfileString(OPTIONS_REG_KEY, key, defVal);		
        ret = _wtof(keyVal);
	}
	
	return ret;
}

void WinPlatformUtils::GetPreferenceString(const wchar_t *key, std::wstring valueOut, const wchar_t *defaultValue)
{
	valueOut = defaultValue;
	if(key && wcslen(key))
	{
		CWinApp *ncsApp = AfxGetApp();
		CString keyVal = ncsApp->GetProfileString(OPTIONS_REG_KEY, key, defaultValue);	
        valueOut = keyVal;
	}
}

// CCloudSimDlg dialog

CCloudSimDlg::CCloudSimDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CCloudSimDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	
    m_advancedOptionsShown = true; // initially they are visible at launch since the dialog resource is full sized

    m_server = NULL;
	
    m_platformUtils = new WinPlatformUtils(this);
	m_fileIOManager = new NinjaFileIO::WinFileIOManager();
    m_loggingEnabled = false;
}

CCloudSimDlg::~CCloudSimDlg()
{
	if(m_server != NULL)
	{
		if(m_server->IsRunning())
			m_server->Stop();
		delete m_server;
		m_server = NULL;
	}
	delete m_fileIOManager;
	delete m_platformUtils;
    delete m_largeFont;
}

void CCloudSimDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PORTNUMEDIT, m_portNumberCtrl);
    DDX_Control(pDX, IDC_ROOTDIREDIT, m_rootDirCtrl);
    DDX_Control(pDX, IDC_LOGEDIT, m_logEditCtrl);
    DDX_Control(pDX, IDC_STARTBUTTON, m_startBtn);
    DDX_Control(pDX, IDC_STOPBUTTON, m_stopBtn);
    DDX_Control(pDX, IDC_URL_CTRL, m_statusCtrl);
    DDX_Control(pDX, IDC_ENABLELOGCHK, m_enableLoggingChk);
}

BEGIN_MESSAGE_MAP(CCloudSimDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_STARTBUTTON, &CCloudSimDlg::OnBnClickedStartbutton)
	ON_BN_CLICKED(IDC_STOPBUTTON, &CCloudSimDlg::OnBnClickedStopbutton)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_CLEARLOG, &CCloudSimDlg::OnBnClickedClearlog)
    ON_BN_CLICKED(IDC_COPYURL_BTN, &CCloudSimDlg::OnCopyURLToClibpoard)
    ON_BN_CLICKED(IDC_ADVANCEDOPSBTN, &CCloudSimDlg::OnAdvancedOptionsClick)
    ON_EN_CHANGE(IDC_PORTNUMEDIT, &CCloudSimDlg::OnChangePortNumber)
    ON_BN_CLICKED(IDC_ENABLELOGCHK, &CCloudSimDlg::OnEnableLoggingClick)
END_MESSAGE_MAP()

// CCloudSimDlg message handlers

BOOL CCloudSimDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

    // calculate the screen DPI
    CDC *pdc = GetDC();
    if (pdc)
    {
        gDPIX = pdc->GetDeviceCaps(LOGPIXELSX);
        gDPIY = pdc->GetDeviceCaps(LOGPIXELSY);        
    }
    ReleaseDC(pdc);


	// Add "About..." menu item to system menu.
	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    // provide a folder image for the directory picker
    CMFCEditBrowseCtrl *dirBrowser = (CMFCEditBrowseCtrl*)GetDlgItem(IDC_ROOTDIREDIT);
    HBITMAP bmFolder = LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_FOLDERICON));
    dirBrowser->SetBrowseButtonImage(bmFolder, TRUE);

	// init our controls
	CWinApp *ncsApp = AfxGetApp();
	m_portNum = ncsApp->GetProfileInt(OPTIONS_REG_KEY, PORT_REG_KEY, -1);
    if(m_portNum == -1)
    {
        m_portNum = NinjaUtilities::FindAvailablePort();
        if(m_portNum <= 0)
            m_portNum = 16280; // must pick some default port value
    }

    m_loggingEnabled = ncsApp->GetProfileInt(OPTIONS_REG_KEY, ENABLELOGGING_REG_KEY, 0) != 0;
    m_enableLoggingChk.SetCheck(m_loggingEnabled ? BST_CHECKED : BST_UNCHECKED);

    m_localNinjaOrigin = ncsApp->GetProfileString(OPTIONS_REG_KEY, LOCALORIGIN_REG_KEY, L"");

	m_rootDir = ncsApp->GetProfileString(OPTIONS_REG_KEY, ROOTDIR_REG_KEY, L"");
	if(m_rootDir.GetLength() == 0)
	{
		PWSTR docPath;
		HRESULT shRes = ::SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &docPath);		
		if(SUCCEEDED(shRes) && docPath)
		{
			m_rootDir = docPath; 
            m_rootDir += "\\Ninja Projects"; 
			CoTaskMemFree(docPath);
		}
	}
	CString portStr;
	portStr.Format(L"%u", m_portNum);
	m_portNumberCtrl.SetWindowText(portStr);
	m_rootDirCtrl.SetWindowText(m_rootDir);

    m_largeFont = new CFont();
    m_largeFont->CreateFont(ScaleY(16), 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Tahoma"); 

    // set our larger font for the URL label and URL controls
    m_statusCtrl.SetFont(m_largeFont);
    GetDlgItem(IDC_URL_LABEL_CTRL)->SetFont(m_largeFont);

    GetWindowText(m_baseCaptionString); // store this so we can later append the running/stopped status

    StartServer();

    UpdateStatus();

    ShowOptions(false); // start in basic mode

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CCloudSimDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else if ((nID & 0xFFF0) == SC_CLOSE)
	{
		// user clicked the "X"
		Shutdown();

		EndDialog(IDOK);   //Close the dialog with IDOK (or IDCANCEL)
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CCloudSimDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

void CCloudSimDlg::LogMessage(LPCTSTR msg)
{
    if(m_loggingEnabled)
    {
	    int cnt = m_logEditCtrl.GetLineCount();

	    CString str;
	    m_logEditCtrl.GetWindowText(str);
	    str += msg;
	    str += "\r\n";
	    m_logEditCtrl.SetWindowText(str);

	    // Scroll the edit control so that the first visible line
	    m_logEditCtrl.LineScroll(cnt, 0);
    }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CCloudSimDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CCloudSimDlg::StartServer()
{
	bool serverStarted = false;

	if(m_server == NULL)
	{
		m_server = new CHttpServerWrapper();
		m_server->SetPlatformUtilities(m_platformUtils);
		m_server->SetFileIOManager(m_fileIOManager);
	}

	if(m_server)
	{
		if(m_server->IsRunning() == false)
		{
			// get the settings from the main UI
			GetPortNumber();
			GetRootDir();

			serverStarted = m_server->Start(m_portNum, m_rootDir);
		}
	}

	if(serverStarted)
	{
		m_startBtn.EnableWindow(FALSE);
		m_stopBtn.EnableWindow(TRUE);
		m_portNumberCtrl.EnableWindow(FALSE);
		m_rootDirCtrl.EnableWindow(FALSE);
	}
	else
    {
		LogMessage(L"Failed to start cloud server!!!!!");
    }
    UpdateStatus();
}

void CCloudSimDlg::StopServer()
{
	bool serverStopped = false;

	if(m_server)
	{
		if(m_server->IsRunning() == true)
		{
			m_server->Stop();
			serverStopped = !m_server->IsRunning();

			if(serverStopped)
			{
				m_startBtn.EnableWindow(TRUE);
				m_stopBtn.EnableWindow(FALSE);
				m_portNumberCtrl.EnableWindow(TRUE);
				m_rootDirCtrl.EnableWindow(TRUE);

				LogMessage(L"Stopped cloud server");
			}
			else
				LogMessage(L"Failed to stop cloud server!!!!!");
		}
	}
    UpdateStatus();
}

void CCloudSimDlg::UpdateStatus()
{
    CString caption = m_baseCaptionString, url;
    if(m_server)
    {
        if(m_server->IsRunning())
            caption += L" - Running";
        url = m_server->GetURL();
    }

    SetWindowText(caption);
    m_statusCtrl.SetWindowText(url);
}

void CCloudSimDlg::OnBnClickedStartbutton()
{
	StartServer();
}


void CCloudSimDlg::OnBnClickedStopbutton()
{
	StopServer();
}

int CCloudSimDlg::GetPortNumber()
{
	CString valStr;
	m_portNumberCtrl.GetWindowText(valStr);
	if(valStr.GetLength())
	{
		m_portNum = _wtoi(valStr);
	}
	return m_portNum;
}

LPCTSTR CCloudSimDlg::GetRootDir()
{
	m_rootDirCtrl.GetWindowText(m_rootDir);
	return m_rootDir;
}


void CCloudSimDlg::OnClose()
{
	Shutdown();

	CDialogEx::OnClose();
}


void CCloudSimDlg::OnBnClickedClearlog()
{
	m_logEditCtrl.SetWindowText(L"");
}

void CCloudSimDlg::Shutdown()
{
	StopServer();

	// store our last option values so the app can get them and store them in the registry
	GetPortNumber();
	GetRootDir();

	// save last settings to registry
	CWinApp *ncsApp = AfxGetApp();
	ncsApp->WriteProfileInt(OPTIONS_REG_KEY, PORT_REG_KEY, m_portNum);
	ncsApp->WriteProfileString(OPTIONS_REG_KEY, ROOTDIR_REG_KEY, m_rootDir);
    ncsApp->WriteProfileInt(OPTIONS_REG_KEY, ENABLELOGGING_REG_KEY, m_loggingEnabled ? 1 : 0);
}

void CCloudSimDlg::GetLocalNinjaOrigin(std::wstring &valOut)
{
    valOut = m_localNinjaOrigin;
}

void CCloudSimDlg::GetVersionNumber(std::wstring &valOut)
{
    TCHAR fname[_MAX_PATH];
    fname[0] = 0;
    if(::GetModuleFileName(AfxGetInstanceHandle( ), fname, _MAX_PATH))
    {
        DWORD dwH = 0, viSize = ::GetFileVersionInfoSize(fname, &dwH);
        if(viSize)
        {
            VS_FIXEDFILEINFO *pInfo;
            UINT nSize;
            LPVOID pBuf = malloc(viSize);
            if(::GetFileVersionInfo(fname, 0, viSize, pBuf) && ::VerQueryValue(pBuf, TEXT("\\"), (LPVOID*)(&pInfo), &nSize))
            {
                CString verStr;
                verStr.Format(L"%u.%u.%u.%u", HIWORD(pInfo->dwFileVersionMS), LOWORD(pInfo->dwFileVersionMS), HIWORD(pInfo->dwFileVersionLS), LOWORD(pInfo->dwFileVersionLS));                    
                valOut = verStr;
            }
            delete pBuf;
        }
    }
}

void CCloudSimDlg::OnCopyURLToClibpoard()
{
    if (m_server && OpenClipboard())
    {
        ::EmptyClipboard();

        CString urlInfo = m_server->GetURL();
        int len = urlInfo.GetLength();
        HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE,  (len + 1) * sizeof(TCHAR)); 
        if (hglbCopy != NULL) 
        { 
            LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hglbCopy); 
            memset(lptstrCopy, 0, (len + 1) * sizeof(TCHAR));
            memcpy(lptstrCopy, urlInfo.GetBuffer(), len * sizeof(TCHAR)); 
            GlobalUnlock(hglbCopy); 

            SetClipboardData(CF_UNICODETEXT, hglbCopy); 
        } 

        ::CloseClipboard();
    }
}


void CCloudSimDlg::OnAdvancedOptionsClick()
{
    ShowOptions(!m_advancedOptionsShown);
}

void CCloudSimDlg::UpdateOptionsState()
{
    CWnd *optsBtn = GetDlgItem(IDC_ADVANCEDOPSBTN), *logBox = GetDlgItem(IDC_LOGGROUPBOX),
        *optsBox = GetDlgItem(IDC_OPTIONSGROPBOX);    
    CRect curRect, optsBtnRect, logRect;
    GetWindowRect(&curRect);
    optsBtn->GetWindowRect(&optsBtnRect);
    logBox->GetWindowRect(&logRect);

    if(m_advancedOptionsShown)
    {        
        curRect.bottom += (logRect.bottom - optsBtnRect.bottom);
        optsBox->ShowWindow(SW_SHOW); // ensure it is visible again
        GetDlgItem(IDC_ADVANCEDOPSBTN)->SetWindowText(L"Basic");
    }
    else // in basic mode
    {
        curRect.bottom -= (logRect.bottom - optsBtnRect.bottom);
        optsBox->ShowWindow(SW_HIDE); // hide this or its title can be seen in basic mode
        GetDlgItem(IDC_ADVANCEDOPSBTN)->SetWindowText(L"Advanced");
    }

    SetWindowPos(&CWnd::wndBottom, 0, 0, curRect.Width(), curRect.Height(), SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
}

void CCloudSimDlg::ShowOptions(bool showOpts)
{
    if(m_advancedOptionsShown != showOpts)
    {
        m_advancedOptionsShown = showOpts;
        UpdateOptionsState();
    }
}


void CCloudSimDlg::OnChangePortNumber()
{
    GetPortNumber();    
	std::wstring newURL;
	if(NinjaUtilities::GetLocalURLForPort(m_portNum, newURL))
		m_statusCtrl.SetWindowText(newURL.c_str());
}


void CCloudSimDlg::OnEnableLoggingClick()
{
    m_loggingEnabled = (m_enableLoggingChk.GetCheck() == BST_CHECKED);
}
