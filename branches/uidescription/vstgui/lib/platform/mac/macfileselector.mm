#import "../../cfileselector.h"

#if VSTGUI_NEW_CFILESELECTOR
// the cocoa fileselector is also used for carbon
#import <Cocoa/Cocoa.h>
#import "cocoa/cocoahelpers.h"

#if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_4
#define NSUInteger unsigned
#endif

#if MAC_COCOA
#import "cocoa/nsviewframe.h"

static Class fileSelectorDelegateClass = 0;
static id VSTGUI_FileSelector_Delegate_Init (id self, SEL _cmd, void* fileSelector);
static void VSTGUI_FileSelector_Delegate_Dealloc (id self, SEL _cmd);
static void VSTGUI_FileSelector_Delegate_OpenPanelDidEnd (id self, SEL _cmd, NSOpenPanel* openPanel, int returnCode, void* contextInfo);
#endif

namespace VSTGUI {

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class CocoaFileSelector : public CNewFileSelector
{
public:
	CocoaFileSelector (CFrame* frame, Style style);
	~CocoaFileSelector ();

	void openPanelDidEnd (NSOpenPanel* panel, int resultCode);
protected:
	static void initClass ();
	
	bool runInternal (CBaseObject* delegate);
	bool runModalInternal ();
	void cancelInternal ();

	Style style;
	CBaseObject* delegate;
	NSSavePanel* savePanel;
};

//-----------------------------------------------------------------------------
CNewFileSelector* CNewFileSelector::create (CFrame* frame, Style style)
{
	return new CocoaFileSelector (frame, style);
}

//-----------------------------------------------------------------------------
void CocoaFileSelector::initClass ()
{
	#if MAC_COCOA
	if (fileSelectorDelegateClass == 0)
	{
		NSMutableString* fileSelectorDelegateClassName = [[[NSMutableString alloc] initWithString:@"VSTGUI_FileSelector_Delegate"] autorelease];
		fileSelectorDelegateClass = generateUniqueClass (fileSelectorDelegateClassName, [NSObject class]);
		BOOL res = class_addMethod (fileSelectorDelegateClass, @selector(initWithFileSelector:), IMP (VSTGUI_FileSelector_Delegate_Init), "@@:@:^:");
		res = class_addMethod (fileSelectorDelegateClass, @selector(dealloc), IMP (VSTGUI_FileSelector_Delegate_Dealloc), "v@:@:");
		res = class_addMethod (fileSelectorDelegateClass, @selector(openPanelDidEnd:returnCode:contextInfo:), IMP (VSTGUI_FileSelector_Delegate_OpenPanelDidEnd), "v@:@:@:I:@:");
		res = class_addIvar (fileSelectorDelegateClass, "_fileSelector", sizeof (void*), (uint8_t)log2(sizeof(void*)), @encode(void*));
		objc_registerClassPair (fileSelectorDelegateClass);
	}
	#endif
}

//-----------------------------------------------------------------------------
CocoaFileSelector::CocoaFileSelector (CFrame* frame, Style style)
: CNewFileSelector (frame)
, style (style)
, delegate (0)
{
	savePanel = nil;
}

//-----------------------------------------------------------------------------
CocoaFileSelector::~CocoaFileSelector ()
{
	if (delegate)
		delegate->forget ();
}

//-----------------------------------------------------------------------------
void CocoaFileSelector::openPanelDidEnd (NSOpenPanel* openPanel, int res)
{
	if (res == NSFileHandlingPanelOKButton)
	{
		if (style == kSelectSaveFile)
		{
			NSURL* url = [openPanel URL];
			const char* utf8Path = url ? [[url path] UTF8String] : 0;
			if (utf8Path)
			{
				char* path = (char*)malloc (strlen (utf8Path) + 1);
				strcpy (path, utf8Path);
				result.push_back (path);
			}
		}
		else
		{
			NSArray* urls = [openPanel URLs];
			for (NSUInteger i = 0; i < [urls count]; i++)
			{
				NSURL* url = [urls objectAtIndex:i];
				if (url == 0 || [url path] == 0)
					continue;
				const char* utf8Path = [[url path] UTF8String];
				if (utf8Path)
				{
					char* path = (char*)malloc (strlen (utf8Path) + 1);
					strcpy (path, utf8Path);
					result.push_back (path);
				}
			}
		}
	}
	if (delegate)
		delegate->notify (this, CNewFileSelector::kSelectEndMessage);
}

//-----------------------------------------------------------------------------
void CocoaFileSelector::cancelInternal ()
{
	if (savePanel)
		[savePanel cancel:nil];
}

//-----------------------------------------------------------------------------
bool CocoaFileSelector::runInternal (CBaseObject* _delegate)
{
	remember ();
	NSWindow* parentWindow = nil;
	if (_delegate)
	{
		#if MAC_COCOA
		if (frame && frame->getPlatformFrame ())
		{
			NSViewFrame* nsViewFrame = dynamic_cast<NSViewFrame*> (frame->getPlatformFrame ());
			parentWindow = nsViewFrame ? [(nsViewFrame->getPlatformControl ()) window] : 0;
		}
		#endif
		delegate = _delegate;
		delegate->remember ();
	}
	NSOpenPanel* openPanel = nil;
	NSMutableArray* typesArray = nil;
	if (extensions.size () > 0)
	{
		typesArray = [[[NSMutableArray alloc] init] autorelease];
		std::list<CFileExtension>::const_iterator it = extensions.begin ();
		while (it != extensions.end ())
		{
			NSString* uti = 0;
			if ((*it).getMimeType ())
				uti = (NSString*)UTTypeCreatePreferredIdentifierForTag (kUTTagClassMIMEType, (CFStringRef)[NSString stringWithCString: (*it).getMimeType () encoding:NSUTF8StringEncoding], NULL);
			if (uti == 0 && (*it).getMacType ())
			{
				NSString* osType = (NSString*)UTCreateStringForOSType ((*it).getMacType ());
				if (osType)
				{
					uti = (NSString*)UTTypeCreatePreferredIdentifierForTag (kUTTagClassOSType, (CFStringRef)osType, NULL);
					[osType release];
				}
			}
			if (uti == 0 && (*it).getExtension ())
				uti = (NSString*)UTTypeCreatePreferredIdentifierForTag (kUTTagClassFilenameExtension, (CFStringRef)[NSString stringWithCString: (*it).getExtension () encoding:NSUTF8StringEncoding], NULL);
			if (uti)
			{
				[typesArray addObject:uti];
				[uti release];
			}
			it++;
		}
	}
	if (style == kSelectSaveFile)
	{
		savePanel = [NSSavePanel savePanel];
		if (typesArray)
			[savePanel setAllowedFileTypes:typesArray];
	}
	else
	{
		savePanel = openPanel = [NSOpenPanel openPanel];
		if (style == kSelectFile)
		{
			[openPanel setAllowsMultipleSelection:allowMultiFileSelection ? YES : NO];
		}
		else
		{
			[openPanel setCanChooseDirectories:YES];
		}
	}
	if (title && savePanel)
		[savePanel setTitle:[NSString stringWithCString: title encoding:NSUTF8StringEncoding]];
	if (openPanel)
	{
		#if MAC_COCOA
		if (parentWindow)
		{
			id fsdelegate = [[fileSelectorDelegateClass alloc] performSelector:@selector(initWithFileSelector:) withObject: (id)this];
			[openPanel beginSheetForDirectory:initialPath ? [NSString stringWithCString:initialPath encoding:NSUTF8StringEncoding] : nil file:nil types:typesArray modalForWindow:parentWindow modalDelegate:fsdelegate didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:) contextInfo:nil];
		}
		else
		#endif
		{
			int res = [openPanel runModalForDirectory:initialPath ? [NSString stringWithCString:initialPath encoding:NSUTF8StringEncoding] : nil file:nil types:typesArray];
			openPanelDidEnd (openPanel, res);
			return res == NSFileHandlingPanelOKButton;
		}
	}
	else if (savePanel)
	{
		#if MAC_COCOA
		if (parentWindow)
		{
			id fsdelegate = [[fileSelectorDelegateClass alloc] performSelector:@selector(initWithFileSelector:) withObject: (id)this];
			[savePanel beginSheetForDirectory:initialPath ? [NSString stringWithCString:initialPath encoding:NSUTF8StringEncoding] : nil file:defaultSaveName ? [NSString stringWithCString:defaultSaveName encoding:NSUTF8StringEncoding] : nil modalForWindow:parentWindow modalDelegate:fsdelegate didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:) contextInfo:nil];
		}
		else
		#endif
		{
			int res = [savePanel runModalForDirectory:initialPath ? [NSString stringWithCString:initialPath encoding:NSUTF8StringEncoding]:nil file:defaultSaveName ? [NSString stringWithCString:defaultSaveName encoding:NSUTF8StringEncoding] : nil];
			openPanelDidEnd (savePanel, res);
			return res == NSFileHandlingPanelOKButton;
		}
	}
	
	forget ();
	return true;
}

//-----------------------------------------------------------------------------
bool CocoaFileSelector::runModalInternal ()
{
	return runInternal (0);
}

}

#if MAC_COCOA
using namespace VSTGUI;

//-----------------------------------------------------------------------------
__attribute__((__destructor__)) void cleanup_VSTGUI_FileSelector ()
{
	if (fileSelectorDelegateClass)
		objc_disposeClassPair (fileSelectorDelegateClass);
}

//-----------------------------------------------------------------------------
id VSTGUI_FileSelector_Delegate_Init (id self, SEL _cmd, void* fileSelector)
{
	__OBJC_SUPER(self)
	self = objc_msgSendSuper (SUPER, @selector(init)); // self = [super init];
	if (self)
	{
		((CocoaFileSelector*)fileSelector)->remember ();
		OBJC_SET_VALUE (self, _fileSelector, (id)fileSelector);
	}
	return self;
}

//-----------------------------------------------------------------------------
void VSTGUI_FileSelector_Delegate_Dealloc (id self, SEL _cmd)
{
	id fileSelector = OBJC_GET_VALUE(self, _fileSelector);
	if (fileSelector)
		((CocoaFileSelector*)fileSelector)->forget ();
	__OBJC_SUPER(self)
	objc_msgSendSuper (SUPER, @selector(dealloc)); // [super dealloc];
}

//-----------------------------------------------------------------------------
void VSTGUI_FileSelector_Delegate_OpenPanelDidEnd (id self, SEL _cmd, NSOpenPanel* openPanel, int returnCode, void* contextInfo)
{
	id fileSelector = OBJC_GET_VALUE(self, _fileSelector);
	if (fileSelector)
	{
		((CocoaFileSelector*)fileSelector)->openPanelDidEnd (openPanel, returnCode);
	}
	[self autorelease];
}
#endif // MAC_COCOA

#endif // VSTGUI_NEW_CFILESELECTOR