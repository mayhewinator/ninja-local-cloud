/*
<copyright>
	This file contains proprietary software owned by Motorola Mobility, Inc.
	No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.
	(c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
</copyright>
*/

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

#define OPTIONS_REG_KEY L"Options"
#define PORT_REG_KEY L"Port"
#define ROOTDIR_REG_KEY L"Root Directory"
#define LOCALORIGIN_REG_KEY L"Local Ninja Origin"
#define ENABLELOGGING_REG_KEY L"Enable Logging"

// CCloudSimApp:
// See NinjaCloudSimulator.cpp for the implementation of this class
//

class CCloudSimApp : public CWinAppEx 
{
public:
	CCloudSimApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CCloudSimApp theApp;