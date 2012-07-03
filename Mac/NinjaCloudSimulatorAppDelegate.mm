/*
 <copyright>
 This file contains proprietary software owned by Motorola Mobility, Inc.
 No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.
 (c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
 </copyright>
 */


#import "NinjaCloudSimulatorAppDelegate.h"
#import "AppController.h"

@implementation NinjaCloudSimulatorAppDelegate

@synthesize window;
@synthesize appController;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification 
{
	[appController InitAfterWindowLoad]; // call the controller to do its final initialization
}

-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication
{
	return YES; // we don't want the app to remain open when our main window is closed
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    [appController PrepareForExit];
}

@end
