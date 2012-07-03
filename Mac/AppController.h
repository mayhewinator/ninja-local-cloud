/*
 <copyright>
 This file contains proprietary software owned by Motorola Mobility, Inc.
 No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.
 (c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
 </copyright>
 */


#import <Cocoa/Cocoa.h>

class CHttpServerWrapper;
namespace NinjaFileIO 
{
	class MacFileIOManager;
}

namespace NinjaUtilities
{
	class PlatformUtility;
}

@interface AppController : NSObject {
	IBOutlet NSTextField *portNumberCtrl;
	IBOutlet NSTextField *rootCtrl;
	IBOutlet NSTextField *urlCtrl;
	IBOutlet NSTextView *logCtrl;
    IBOutlet NSButton *enableLoggingCtrl;
	IBOutlet NSButton *startBtnCtrl;
	IBOutlet NSButton *stopBtnCtrl;
	IBOutlet NSButton *advancedBtnCtrl;
	
	BOOL showingAdvancedOptions;
    BOOL loggingEnabled;
	
	CHttpServerWrapper *svrWrapper;
	NinjaFileIO::MacFileIOManager *fileMgr;
	NinjaUtilities::PlatformUtility *platformUtils;
	NSString *title;
}

-(IBAction)start:(id)sender;
-(IBAction)stop:(id)sender;
-(IBAction)clearLog:(id)sender;
-(IBAction)advancedOptions:(id)sender;
-(IBAction)copyURL:(id)sender;
-(IBAction)enableLogging:(id)sender;
-(void)LogMessage:(NSString*)str;
-(void)SaveSettings;
-(void)UpdateURL;
-(void)UpdateStatus;
-(void)UpdateForAdvancedOptions;
-(void)InitAfterWindowLoad;
-(void)PrepareForExit;

@end
