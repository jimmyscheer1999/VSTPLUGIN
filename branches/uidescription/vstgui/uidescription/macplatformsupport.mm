/*
 *  macplatformwindow.mm
 *
 *  Created by Arne Scheffler on 5/10/09.
 *  Copyright 2009 Arne Scheffler. All rights reserved.
 *
 */

#include "platformsupport.h"
#include "../cocoasupport.h"
#include <Cocoa/Cocoa.h>

//-----------------------------------------------------------------------------
@interface VSTGUI_CocoaWindowDelegate : NSObject {
	IPlatformWindowDelegate* delegate;
	PlatformWindow* platformWindow;
}
- (id) initWithDelegate:(IPlatformWindowDelegate*) delegate platformWindow: (PlatformWindow*) platformWindow;
@end

//-----------------------------------------------------------------------------
@interface VSTGUI_CocoaDraggingSource : NSObject {
	BOOL localOnly;
}
- (void) setLocalOnly:(BOOL)flag;
@end

//-----------------------------------------------------------------------------
@interface VSTGUI_ColorChangeCallback : NSObject {
	IPlatformColorChangeCallback* callback;
}
- (void) setCallback: (IPlatformColorChangeCallback*) _callback;
@end
static VSTGUI_ColorChangeCallback* gColorChangeCallback = 0;

//-----------------------------------------------------------------------------
static __attribute__((__destructor__)) void colorChangeCallbackDestructor () 
{
	if (gColorChangeCallback)
		[gColorChangeCallback release];
}

BEGIN_NAMESPACE_VSTGUI

//-----------------------------------------------------------------------------
class CocoaWindow : public PlatformWindow
{
public:
	CocoaWindow (const CRect& size, const char* title, WindowType type, long styleFlags, IPlatformWindowDelegate* delegate);
	~CocoaWindow ();

	void* getPlatformHandle () const { return [window contentView]; }
	void show ();
	void center ();
	CRect getSize ();
	void setSize (const CRect& size);
	void runModal ();
	void stopModal ();
protected:
	NSWindow* window;
	IPlatformWindowDelegate* delegate;
	VSTGUI_CocoaWindowDelegate* cocoaDelegate;
	WindowType type;
	long styleFlags;
};

//-----------------------------------------------------------------------------
PlatformWindow* PlatformWindow::create (const CRect& size, const char* title, WindowType type, long styleFlags, IPlatformWindowDelegate* delegate)
{
	return new CocoaWindow (size, title, type, styleFlags, delegate);
}

//-----------------------------------------------------------------------------
CocoaWindow::CocoaWindow (const CRect& size, const char* title, WindowType type, long styleFlags, IPlatformWindowDelegate* delegate)
: window (0)
, delegate (delegate)
, cocoaDelegate (0)
, type (type)
, styleFlags (styleFlags)
{
	NSRect contentRect = NSMakeRect (size.left, size.top, size.getWidth (), size.getHeight ());
	NSUInteger style = NSTitledWindowMask;
	if (styleFlags & kClosable)
		style |= NSClosableWindowMask;
	if (styleFlags & kResizable)
		style |= NSResizableWindowMask;
	if (type == kPanelType)
	{
		window = [[NSPanel alloc] initWithContentRect:contentRect styleMask:style|NSUtilityWindowMask|NSHUDWindowMask backing:NSBackingStoreBuffered defer:YES];
		[(NSPanel*)window setBecomesKeyOnlyIfNeeded:NO];
		[window setMovableByWindowBackground:NO];
	}
	else if (type == kWindowType)
	{
		window = [[NSWindow alloc] initWithContentRect:contentRect styleMask:style backing:NSBackingStoreBuffered defer:YES];
	}
	[window setReleasedWhenClosed:NO];
	if (title)
		[window setTitle:[NSString stringWithCString:title encoding:NSUTF8StringEncoding]];
	if (delegate)
	{
		cocoaDelegate = [[VSTGUI_CocoaWindowDelegate alloc] initWithDelegate:delegate platformWindow:this];
		[window setDelegate:cocoaDelegate];
	}
}

//-----------------------------------------------------------------------------
CocoaWindow::~CocoaWindow ()
{
	if (cocoaDelegate)
	{
		[window setDelegate:nil];
		[cocoaDelegate release];
	}
	[window release];
}

//-----------------------------------------------------------------------------
void CocoaWindow::show ()
{
	if (type == kPanelType)
		[window orderFront:nil];
	else
		[window makeKeyAndOrderFront:nil];
}

//-----------------------------------------------------------------------------
void CocoaWindow::center ()
{
	[window center];
}

//-----------------------------------------------------------------------------
CRect CocoaWindow::getSize ()
{
	NSRect size = [window contentRectForFrameRect:[window frame]];
	CRect r (size.origin.x, size.origin.y, 0, 0);
	r.setWidth (size.size.width);
	r.setHeight (size.size.height);
	return r;
}

//-----------------------------------------------------------------------------
void CocoaWindow::setSize (const CRect& size)
{
	NSRect r = NSMakeRect (size.left, size.top, size.getWidth (), size.getHeight ());
	r = [window frameRectForContentRect:r];
	[window setFrame:r display:YES];
}

//-----------------------------------------------------------------------------
void CocoaWindow::runModal ()
{
	[NSApp runModalForWindow:window];
}

//-----------------------------------------------------------------------------
void CocoaWindow::stopModal ()
{
	[NSApp abortModal];
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool PlatformUtilities::collectPlatformFontNames (std::list<std::string*>& fontNames)
{
	NSArray* fonts = [[NSFontManager sharedFontManager] availableFontFamilies];
	for (NSString* font in fonts)
	{
		fontNames.push_back (new std::string ([font UTF8String]));
	}
	return true;
}

//-----------------------------------------------------------------------------
bool PlatformUtilities::startDrag (CFrame* frame, const CPoint& location, const char* string, CBitmap* dragBitmap, bool localOnly)
{
	NSView* nsView = (NSView*)frame->getNSView ();
	if (nsView)
	{
		NSPoint bitmapOffset = { location.x, location.y };
		NSPasteboard* nsPasteboard = [NSPasteboard pasteboardWithName:NSDragPboard];
		NSImage* nsImage = nil;
		if (dragBitmap)
		{
			CGImageRef cgImage = dragBitmap->createCGImage (false);
			if (cgImage)
			{
				nsImage = [imageFromCGImageRef (cgImage) autorelease];
				bitmapOffset.x -= [nsImage size].width/2;
				bitmapOffset.y += [nsImage size].height/2;
				CFRelease (cgImage);
			}
		}
		else
		{
			nsImage = [[[NSImage alloc] initWithSize:NSMakeSize (2, 2)] autorelease];
		}
		VSTGUI_CocoaDraggingSource* sourceObj = [[VSTGUI_CocoaDraggingSource alloc] init];
		[sourceObj setLocalOnly:localOnly];
		
		[nsPasteboard declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:sourceObj];
		[nsPasteboard setString:[NSString stringWithCString:string encoding:NSUTF8StringEncoding] forType:NSStringPboardType];
		
		[nsView dragImage:nsImage at:bitmapOffset offset:NSMakeSize (0, 0) event:[NSApp currentEvent] pasteboard:nsPasteboard source:sourceObj slideBack:YES];
		[sourceObj release];
	}
	return false;
}

//-----------------------------------------------------------------------------
void PlatformUtilities::colorChooser (const CColor* oldColor, IPlatformColorChangeCallback* callback)
{
	if (gColorChangeCallback == 0)
		gColorChangeCallback = [[VSTGUI_ColorChangeCallback alloc] init];
	[gColorChangeCallback setCallback:callback];
	NSColorPanel* colorPanel = [NSColorPanel sharedColorPanel];
	[colorPanel setTarget:nil];
	if (oldColor)
	{
		[colorPanel setShowsAlpha:YES];
		NSColor* nsColor = [NSColor colorWithDeviceRed:(float)oldColor->red/255.f green:(float)oldColor->green/255.f blue:(float)oldColor->blue/255.f alpha:(float)oldColor->alpha/255.f];
		[colorPanel setColor:nsColor];
		[colorPanel setTarget:gColorChangeCallback];
		[colorPanel setAction:@selector(colorChanged:)];
		[colorPanel makeKeyAndOrderFront:nil];
	}
}

#define VSTGUI_CFSTR(x) (CFStringRef)[NSString stringWithCString:x encoding:NSUTF8StringEncoding]
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void PlatformDefaults::setRect (const char* appID, const char* name, const CRect& value)
{
	NSMutableDictionary* dict = [NSMutableDictionary dictionaryWithCapacity:4];
	[dict setValue:[NSNumber numberWithDouble:value.left] forKey:@"left"];
	[dict setValue:[NSNumber numberWithDouble:value.right] forKey:@"right"];
	[dict setValue:[NSNumber numberWithDouble:value.top] forKey:@"top"];
	[dict setValue:[NSNumber numberWithDouble:value.bottom] forKey:@"bottom"];
	CFPreferencesSetAppValue (VSTGUI_CFSTR(name), dict, VSTGUI_CFSTR(appID));
	CFPreferencesAppSynchronize (VSTGUI_CFSTR(appID));
}

//-----------------------------------------------------------------------------
bool PlatformDefaults::getRect (const char* appID, const char* name, CRect& value)
{
	NSDictionary* dict = (NSDictionary*)CFPreferencesCopyAppValue (VSTGUI_CFSTR(name), VSTGUI_CFSTR(appID));
	if (dict)
	{
		NSNumber* n = [dict objectForKey:@"left"];
		if (n)
			value.left = [n doubleValue];
		n = [dict objectForKey:@"right"];
		if (n)
			value.right = [n doubleValue];
		n = [dict objectForKey:@"top"];
		if (n)
			value.top = [n doubleValue];
		n = [dict objectForKey:@"bottom"];
		if (n)
			value.bottom = [n doubleValue];
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
void PlatformDefaults::setString (const char* appID, const char* name, const std::string& value)
{
}

//-----------------------------------------------------------------------------
bool PlatformDefaults::getString (const char* appID, const char* name, std::string& value)
{
	return false;
}

//-----------------------------------------------------------------------------
void PlatformDefaults::setNumber (const char* appID, const char* name, long value)
{
}

//-----------------------------------------------------------------------------
bool PlatformDefaults::getNumber (const char* appID, const char* name, long& value)
{
	return false;
}

END_NAMESPACE_VSTGUI

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
@implementation VSTGUI_CocoaWindowDelegate
//-----------------------------------------------------------------------------
- (id) initWithDelegate:(IPlatformWindowDelegate*) _delegate platformWindow:(PlatformWindow*) _platformWindow
{
	self = [super init];
	if (self)
	{
		delegate = _delegate;
		platformWindow = _platformWindow;
	}
	return self;
}

//-----------------------------------------------------------------------------
- (void)windowDidResize:(NSNotification *)notification
{
	NSWindow* window = [notification object];
	NSRect size = [window contentRectForFrameRect:[window frame]];
	CRect r (size.origin.x, size.origin.y, 0, 0);
	r.setWidth (size.size.width);
	r.setHeight (size.size.height);
	delegate->windowSizeChanged (r, platformWindow);
}

//-----------------------------------------------------------------------------
- (void)windowWillClose:(NSNotification *)notification
{
	delegate->windowClosed (platformWindow);
}

//-----------------------------------------------------------------------------
- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize
{
	NSRect r = {{0, 0}, frameSize };
	r = [sender contentRectForFrameRect:r];
	CPoint p (r.size.width, r.size.height);
	delegate->checkWindowSizeConstraints (p, platformWindow);
	r.size.width = p.x;
	r.size.height = p.y;
	r = [sender frameRectForContentRect:r];
	return r.size;
}

@end

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
@implementation VSTGUI_CocoaDraggingSource
//-----------------------------------------------------------------------------
- (id) init
{
	self = [super init];
	if (self)
	{
		localOnly = NO;
	}
	return self;
}

//-----------------------------------------------------------------------------
- (void) setLocalOnly:(BOOL)flag
{
	localOnly = flag;
}

//-----------------------------------------------------------------------------
- (NSDragOperation)draggingSourceOperationMaskForLocal:(BOOL)flag
{
	if (localOnly && !flag)
		return NSDragOperationNone;
	return NSDragOperationGeneric;
}

//-----------------------------------------------------------------------------
- (BOOL)ignoreModifierKeysWhileDragging
{
	return YES;
}

@end

//-----------------------------------------------------------------------------
@implementation VSTGUI_ColorChangeCallback
- (void) setCallback: (IPlatformColorChangeCallback*) _callback
{
	callback = _callback;
}
- (void) colorChanged: (id) sender
{
	if (callback)
	{
		NSColor* nsColor = [sender color];
		nsColor = [nsColor colorUsingColorSpaceName:NSDeviceRGBColorSpace];
		if (nsColor)
		{
			CColor newColor = MakeCColor ([nsColor redComponent] * 255., [nsColor greenComponent] * 255., [nsColor blueComponent] * 255., [nsColor alphaComponent] * 255.);
			callback->colorChanged (newColor);
		}
	}
}
@end