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


#import "AppController.h"
#include "../Core/HttpServerWrapper.h"
#include "MacFileIOManager.h"
#include "../Core/Utils.h"

NSString *portKeyName = @"Local Cloud Port Number";
NSString *docRootKeyName = @"Local Cloud Document Root";
NSString *localOriginKeyName = @"Local Ninja Origin";
NSString *limitIOKeyName = @"Limit IO to Document Root";
NSString *loggingEnabledKeyName = @"Logging Enabled";

class MacPlatformUtils : public NinjaUtilities::PlatformUtility
{
public:
	MacPlatformUtils(AppController *ac)
	{
		appController = ac;
	}
    
    virtual ~MacPlatformUtils() {}
	
	void LogMessage(const wchar_t *msg)
	{
		NSString *str = NinjaUtilities::WStringToNSString(msg);
	
		// log the message on the main thread to allow us to output the string to the log text view.
		SEL logMethod = @selector(LogMessage:);
	    [appController performSelectorOnMainThread:logMethod withObject:str waitUntilDone:true];
		
		[str release];
	}

    bool GetLocalNinjaOrigin(std::wstring &valueOut)
	{
		bool ret = false;
		
		if(cachedLocalOrigin.length() == 0)
		{
			NSString *localOrigin = [[NSUserDefaults standardUserDefaults] stringForKey:localOriginKeyName];
			if(localOrigin)
			{
				cachedLocalOrigin = NinjaUtilities::NSStringToWString(localOrigin);
			}
		}
		
		valueOut = cachedLocalOrigin;
		ret = valueOut.length() > 0;

		return ret;
	}
	
	bool GetVersionNumber(std::wstring &valueOut)
	{
		bool ret = false;
		
		NSString *ver = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"];	
		if(ver)
		{
			valueOut = NinjaUtilities::NSStringToWString(ver);
			ret = valueOut.length() > 0;
		}

		return ret;
	}

	bool GetPreferenceBool(const wchar_t *key, bool defaultValue)
	{
		bool ret = defaultValue;
		if(key != nil && wcslen(key))
		{		
			NSString *nsKey = NinjaUtilities::StringToNSString(key);
			if([[NSUserDefaults standardUserDefaults] objectForKey:nsKey] != nil)
				ret = [[NSUserDefaults standardUserDefaults] boolForKey:nsKey];
			[nsKey release];
		}														   
		return ret;
	}
															   
	int GetPreferenceInt(const wchar_t *key, int defaultValue)
	{
		int ret = defaultValue;
		
		if(key != nil && wcslen(key))
		{		
			NSString *nsKey = NinjaUtilities::StringToNSString(key);
			if([[NSUserDefaults standardUserDefaults] objectForKey:nsKey] != nil)
				ret = [[NSUserDefaults standardUserDefaults] integerForKey:nsKey];
			[nsKey release];
		}	
		
		return ret;
	}
	
	double GetPreferenceDouble(const wchar_t *key, double defaultValue)
	{
		double ret = defaultValue;
		
		if(key != nil && wcslen(key))
		{		
			NSString *nsKey = NinjaUtilities::StringToNSString(key);
			if([[NSUserDefaults standardUserDefaults] objectForKey:nsKey] != nil)
				ret = [[NSUserDefaults standardUserDefaults] doubleForKey:nsKey];
			[nsKey release];
		}	
		
		return ret;
	}
		
	void GetPreferenceString(const wchar_t *key, std::wstring valueOut, const wchar_t *defaultValue)
	{
		valueOut = defaultValue; 
		
		if(key != nil && wcslen(key))
		{		
			NSString *nsKey = NinjaUtilities::StringToNSString(key);
			if([[NSUserDefaults standardUserDefaults] objectForKey:nsKey] != nil)
			{
				NSString *val = [[NSUserDefaults standardUserDefaults] stringForKey:nsKey];
				if(val)
				{
					valueOut = NinjaUtilities::NSStringToWString(val);
					[val release];
				}				
			}
			[nsKey release];
		}			
	}
	
private:
	AppController *appController;
	std::wstring cachedLocalOrigin;
};

@implementation AppController

-(id)init 
{
	[super init];
	
	fileMgr = new NinjaFileIO::MacFileIOManager();
	platformUtils = new MacPlatformUtils(self);
	svrWrapper = new CHttpServerWrapper();
	svrWrapper->SetFileIOManager(fileMgr);
	svrWrapper->SetPlatformUtilities(platformUtils);
	
	showingAdvancedOptions = NO;
    loggingEnabled = NO;
	
	// set our defaults
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSMutableDictionary *appDefaults = [NSMutableDictionary dictionary];
    [appDefaults setObject:[NSNumber numberWithInt:YES] forKey:limitIOKeyName];
    [appDefaults setObject:[NSNumber numberWithBool:NO] forKey:loggingEnabledKeyName];

    [defaults registerDefaults:appDefaults];
	
	return self;
}

-(void)dealloc
{	
	delete svrWrapper;
	delete fileMgr;
	delete platformUtils;
	[title release];
	
	[super dealloc];
}

-(void)LogMessage:(NSString*)str
{
    if(loggingEnabled)
    {
		NSString *strOut = [NSString stringWithFormat:@"%@\n", str];
		NSLog(@"%@", strOut);
	
		NSTextStorage *txtStore = [logCtrl textStorage];
		NSRange myRange = NSMakeRange([[logCtrl textStorage] length], 0);
	
		[txtStore beginEditing];
		[txtStore replaceCharactersInRange:myRange withString:strOut];
		[txtStore endEditing];
	}
}

-(void)awakeFromNib
{
	[startBtnCtrl setEnabled:YES];
	[stopBtnCtrl setEnabled:NO];
	[portNumberCtrl setEnabled:YES];
	[rootCtrl setEnabled:YES];
	
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSInteger portKeyValue = [defaults integerForKey:portKeyName];
	NSString *docRootValue = [defaults stringForKey:docRootKeyName];
	loggingEnabled = [defaults boolForKey:loggingEnabledKeyName];
    
	// find an available port to use as a default value
	if(portKeyValue == 0)
		portKeyValue = NinjaUtilities::FindAvailablePort();
	
	// default to "Ninja Projects" under the users home directory
	if(docRootValue == nil)
		docRootValue = [NSHomeDirectory() stringByAppendingString:@"/Documents/Ninja Projects"];
			
	// set default user values
    if(portKeyValue != 0)
	{
		[portNumberCtrl setIntValue:portKeyValue];
	}
	if(docRootValue != nil)
	{
		[rootCtrl setStringValue:docRootValue];
	}
    [enableLoggingCtrl setState:(loggingEnabled == YES ? NSOnState : NSOffState)];
}

-(void)InitAfterWindowLoad
{	
	title = [[[NSApp delegate] window] title];
	[self UpdateForAdvancedOptions];
	[self start:nil];
}

-(void)PrepareForExit
{
    [self start:nil];
    [self SaveSettings];
}

-(void)SaveSettings
{
	int portNum = [portNumberCtrl intValue];
	NSString *rootPath = [[rootCtrl stringValue] stringByExpandingTildeInPath];
    
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	[defaults setInteger:portNum forKey:portKeyName];
	[defaults setObject:rootPath forKey:docRootKeyName];
    [defaults setBool:loggingEnabled forKey:loggingEnabledKeyName];
}

-(void)UpdateURL
{
	int portNum = [portNumberCtrl intValue];

	std::wstring urlw;
	NinjaUtilities::GetLocalURLForPort(portNum, urlw);
	NSString *str = NinjaUtilities::WStringToNSString(urlw);
	
	[urlCtrl setEditable:YES];
	[urlCtrl setStringValue:str];
	[urlCtrl setEditable:NO];
	
	[str release];
}

-(void)UpdateStatus
{
	NSWindow *window = [[NSApp delegate] window];
	NSString *newTitle = title;
	if(svrWrapper->IsRunning())
		newTitle = [title stringByAppendingString:@" - Running"];

	[window setTitle:newTitle];
}


-(void)UpdateForAdvancedOptions
{
	NSWindow *window = [[NSApp delegate] window];
	if(window != nil)
	{
		NSRect curSize = [window frame];
		if(showingAdvancedOptions)
		{
			curSize.size.height = 495;
			[advancedBtnCtrl setTitle:@"Basic"];
		}
		else 
		{
			curSize.size.height = 168;
			[advancedBtnCtrl setTitle:@"Advanced"];
			
		}
		[window setFrame:curSize display:YES animate:YES];
		
		if([window isVisible] == NO)
			[window setIsVisible:YES];
	}
}

-(IBAction)start:(id)sender
{
	if(svrWrapper->IsRunning() == false)
	{
		int portNum = [portNumberCtrl intValue];
		NSString *rootPath = [[rootCtrl stringValue] stringByExpandingTildeInPath];
		std::wstring svrRoot = NinjaUtilities::NSStringToWString(rootPath);
		
		[self SaveSettings];

		bool serverStarted = svrWrapper->Start(portNum, svrRoot.c_str());
		if(serverStarted)
		{
			[startBtnCtrl setEnabled:NO];
			[stopBtnCtrl setEnabled:YES];
			[portNumberCtrl setEnabled:NO];
			[rootCtrl setEnabled:NO];
			
			[self LogMessage:@"Cloud server started"];
			[self UpdateURL];
		}
		else 
		{
			[self LogMessage:@"Cloud Server failed start!!!"];
		}
	}
	[self UpdateStatus];
}

-(IBAction)stop:(id)sender
{
	if(svrWrapper->IsRunning())
	{
		svrWrapper->Stop();
		if(svrWrapper->IsRunning() == false)
		{
			[startBtnCtrl setEnabled:YES];
			[stopBtnCtrl setEnabled:NO];
			[portNumberCtrl setEnabled:YES];
			[rootCtrl setEnabled:YES];

			[self LogMessage:@"Stopped cloud server."];
		}
		else 
		{
			[self LogMessage:@"Failed to stop cloud server!!"];
		}
	}
	[self UpdateStatus];
}

-(IBAction)clearLog:(id)sender
{
	NSRange range = NSMakeRange (0, [[logCtrl string] length]);
	[[logCtrl textStorage] deleteCharactersInRange:range];
}

-(IBAction)advancedOptions:(id)sender
{
	showingAdvancedOptions = !showingAdvancedOptions;
	[self UpdateForAdvancedOptions];
}


-(IBAction)copyURL:(id)sender
{
	NSString *url = [urlCtrl stringValue];
	NSPasteboard *pb = [NSPasteboard generalPasteboard];
	[pb declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:self];
    [pb setString:url forType:NSStringPboardType];
}

-(IBAction)enableLogging:(id)sender
{
    loggingEnabled = ([enableLoggingCtrl state] == NSOnState);
}

- (void)controlTextDidChange:(NSNotification *)aNotification
{
	[self UpdateURL];
}
@end
