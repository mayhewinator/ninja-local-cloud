/*
 <copyright>
 This file contains proprietary software owned by Motorola Mobility, Inc.
 No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.
 (c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
 </copyright>
 */


#import <Cocoa/Cocoa.h>
@class AppController;

@interface NinjaCloudSimulatorAppDelegate : NSObject <NSApplicationDelegate> {
    NSWindow *window;
	AppController *appController;
}

@property (assign) IBOutlet NSWindow *window;
@property (assign) IBOutlet AppController *appController;

@end

