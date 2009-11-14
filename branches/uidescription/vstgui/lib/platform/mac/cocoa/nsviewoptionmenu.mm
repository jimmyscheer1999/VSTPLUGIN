
#import "nsviewoptionmenu.h"

#if MAC_COCOA && VSTGUI_PLATFORM_ABSTRACTION

#import "cocoahelpers.h"
#import "nsviewframe.h"
#import "../../../controls/coptionmenu.h"
#import "../cgbitmap.h"

using namespace VSTGUI;

static Class menuClass = 0;

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
struct VSTGUI_NSMenu_Var
{
	COptionMenu* _optionMenu;
	COptionMenu* _selectedMenu;
	long _selectedItem;
};

//------------------------------------------------------------------------------------
static id VSTGUI_NSMenu_Init (id self, SEL _cmd, void* _menu)
{
	__OBJC_SUPER(self)
	self = objc_msgSendSuper (SUPER, @selector(init));
	if (self)
	{
		NSMenu* nsMenu = (NSMenu*)self;
		COptionMenu* menu = (COptionMenu*)_menu;
		VSTGUI_NSMenu_Var* var = new VSTGUI_NSMenu_Var;
		var->_optionMenu = menu;
		var->_selectedItem = 0;
		var->_selectedMenu = 0;
		OBJC_SET_VALUE(self, _private, var);

		bool multipleCheck = menu->getStyle () & (kMultipleCheckStyle & ~kCheckStyle);
		for (long i = 0; i < menu->getNbEntries (); i++)
		{
			NSMenuItem* nsItem = 0;
			CMenuItem* item = menu->getEntry (i);
			NSMutableString* itemTitle = [[[NSMutableString alloc] initWithCString:item->getTitle () encoding:NSUTF8StringEncoding] autorelease];
			if (menu->getPrefixNumbers ())
			{
				NSString* prefixString = 0;
				switch (menu->getPrefixNumbers ())
				{
					case 2:	prefixString = [NSString stringWithFormat:@"%1d ", i+1]; break;
					case 3: prefixString = [NSString stringWithFormat:@"%02d ", i+1]; break;
					case 4: prefixString = [NSString stringWithFormat:@"%03d ", i+1]; break;
				}
				[itemTitle insertString:prefixString atIndex:0];
			}
			if (item->getSubmenu ())
			{
				nsItem = [nsMenu addItemWithTitle:itemTitle action:nil keyEquivalent:@""];
				NSMenu* subMenu = [[menuClass alloc] performSelector:@selector(initWithOptionMenu:) withObject:(id)item->getSubmenu ()];
				[nsMenu setSubmenu: subMenu forItem:nsItem];
			}
			else if (item->isSeparator ())
			{
				[nsMenu addItem:[NSMenuItem separatorItem]];
			}
			else
			{
				nsItem = [nsMenu addItemWithTitle:itemTitle action:@selector(menuItemSelected:) keyEquivalent:@""];
				if (item->isTitle ())
					[nsItem setIndentationLevel:1];
				[nsItem setTarget:nsMenu];
				[nsItem setTag: i];
				if (multipleCheck && item->isChecked ())
					[nsItem setState:NSOnState];
				else
					[nsItem setState:NSOffState];
				if (item->getKeycode ())
				{
					[nsItem setKeyEquivalent:[NSString stringWithCString:item->getKeycode () encoding:NSUTF8StringEncoding]];
					unsigned int keyModifiers = 0;
					if (item->getKeyModifiers () & kControl)
						keyModifiers |= NSCommandKeyMask;
					if (item->getKeyModifiers () & kShift)
						keyModifiers |= NSShiftKeyMask;
					if (item->getKeyModifiers () & kAlt)
						keyModifiers |= NSAlternateKeyMask;
					if (item->getKeyModifiers () & kApple)
						keyModifiers |= NSControlKeyMask;
					[nsItem setKeyEquivalentModifierMask:keyModifiers];
				}
			}
			if (nsItem && item->getIcon ())
			{
				IPlatformBitmap* platformBitmap = item->getIcon ()->getPlatformBitmap ();
				CGBitmap* cgBitmap = platformBitmap ? dynamic_cast<CGBitmap*> (platformBitmap) : 0;
				CGImageRef image = cgBitmap ? cgBitmap->getCGImage () : 0;
				if (image)
				{
					NSImage* nsImage = imageFromCGImageRef (image);
					if (nsImage)
					{
						[nsItem setImage:nsImage];
						[nsImage release];
					}
				}
			}
		}
	}
	return self;
}

//-----------------------------------------------------------------------------
static void VSTGUI_NSMenu_Dealloc (id self, SEL _cmd)
{
	VSTGUI_NSMenu_Var* var = (VSTGUI_NSMenu_Var*)OBJC_GET_VALUE(self, _private);
	if (var)
		delete var;
	__OBJC_SUPER(self)
	objc_msgSendSuper (SUPER, @selector(dealloc)); // [super dealloc];
}

//------------------------------------------------------------------------------------
static BOOL VSTGUI_NSMenu_ValidateMenuItem (id self, SEL _cmd, id item)
{
	VSTGUI_NSMenu_Var* var = (VSTGUI_NSMenu_Var*)OBJC_GET_VALUE(self, _private);
	if (var && var->_optionMenu)
	{
		CMenuItem* menuItem = var->_optionMenu->getEntry ([item tag]);
		if (!menuItem->isEnabled () || menuItem->isTitle ())
			return NO;
	}
	return YES;
}

//------------------------------------------------------------------------------------
static void VSTGUI_NSMenu_MenuItemSelected (id self, SEL _cmd, id item)
{
	VSTGUI_NSMenu_Var* var = (VSTGUI_NSMenu_Var*)OBJC_GET_VALUE(self, _private);
	if (var)
	{
		id menu = self;
		while ([menu supermenu]) menu = [menu supermenu];
		[menu performSelector:@selector (setSelectedMenu:) withObject: (id)var->_optionMenu];
		[menu performSelector:@selector (setSelectedItem:) withObject: (id)[item tag]];
	}
}

//------------------------------------------------------------------------------------
static void* VSTGUI_NSMenu_OptionMenu (id self, SEL _cmd)
{
	VSTGUI_NSMenu_Var* var = (VSTGUI_NSMenu_Var*)OBJC_GET_VALUE(self, _private);
	return var ? var->_optionMenu : 0;
}

//------------------------------------------------------------------------------------
static void* VSTGUI_NSMenu_SelectedMenu (id self, SEL _cmd)
{
	VSTGUI_NSMenu_Var* var = (VSTGUI_NSMenu_Var*)OBJC_GET_VALUE(self, _private);
	return var ? var->_selectedMenu : 0;
}

//------------------------------------------------------------------------------------
static long VSTGUI_NSMenu_SelectedItem (id self, SEL _cmd)
{
	VSTGUI_NSMenu_Var* var = (VSTGUI_NSMenu_Var*)OBJC_GET_VALUE(self, _private);
	return var ? var->_selectedItem : 0;
}

//------------------------------------------------------------------------------------
static void VSTGUI_NSMenu_SetSelectedMenu (id self, SEL _cmd, void* menu)
{
	VSTGUI_NSMenu_Var* var = (VSTGUI_NSMenu_Var*)OBJC_GET_VALUE(self, _private);
	if (var)
		var->_selectedMenu = (COptionMenu*)menu;
}

//------------------------------------------------------------------------------------
static void VSTGUI_NSMenu_SetSelectedItem (id self, SEL _cmd, long item)
{
	VSTGUI_NSMenu_Var* var = (VSTGUI_NSMenu_Var*)OBJC_GET_VALUE(self, _private);
	if (var)
		var->_selectedItem = item;
}

namespace VSTGUI {

//-----------------------------------------------------------------------------
__attribute__((__destructor__)) void cleanup_VSTGUI_NSMenu ()
{
	if (menuClass)
		objc_disposeClassPair (menuClass);
}

//-----------------------------------------------------------------------------
bool NSViewOptionMenu::initClass ()
{
	if (menuClass == 0)
	{
		NSMutableString* menuClassName = [[[NSMutableString alloc] initWithString:@"VSTGUI_NSMenu"] autorelease];
		menuClass = generateUniqueClass (menuClassName, [NSMenu class]);
		BOOL res = class_addMethod (menuClass, @selector(initWithOptionMenu:), IMP (VSTGUI_NSMenu_Init), "@@:@:^:");
		res = class_addMethod (menuClass, @selector(dealloc), IMP (VSTGUI_NSMenu_Dealloc), "v@:@:");
		res = class_addMethod (menuClass, @selector(validateMenuItem:), IMP (VSTGUI_NSMenu_ValidateMenuItem), "B@:@:@:");
		res = class_addMethod (menuClass, @selector(menuItemSelected:), IMP (VSTGUI_NSMenu_MenuItemSelected), "v@:@:@:");
		res = class_addMethod (menuClass, @selector(optionMenu), IMP (VSTGUI_NSMenu_OptionMenu), "^@:@:");
		res = class_addMethod (menuClass, @selector(selectedMenu), IMP (VSTGUI_NSMenu_SelectedMenu), "^@:@:");
		res = class_addMethod (menuClass, @selector(selectedItem), IMP (VSTGUI_NSMenu_SelectedItem), "l@:@:");
		res = class_addMethod (menuClass, @selector(setSelectedMenu:), IMP (VSTGUI_NSMenu_SetSelectedMenu), "^@:@:^:");
		res = class_addMethod (menuClass, @selector(setSelectedItem:), IMP (VSTGUI_NSMenu_SetSelectedItem), "^@:@:l:");
		res = class_addIvar (menuClass, "_private", sizeof (VSTGUI_NSMenu_Var*), (uint8_t)log2(sizeof(VSTGUI_NSMenu_Var*)), @encode(VSTGUI_NSMenu_Var*));
		objc_registerClassPair (menuClass);
	}
	return menuClass != 0;
}

//-----------------------------------------------------------------------------
PlatformOptionMenuResult NSViewOptionMenu::popup (COptionMenu* optionMenu)
{
	PlatformOptionMenuResult result = {0};

	if (!initClass ())
		return result;

	CFrame* frame = optionMenu->getFrame ();
	if (!frame || !frame->getPlatformFrame ())
		return result;
	NSViewFrame* nsViewFrame = dynamic_cast<NSViewFrame*> (frame->getPlatformFrame ());

	bool multipleCheck = optionMenu->getStyle () & (kMultipleCheckStyle & ~kCheckStyle);
	NSView* view = nsViewFrame->getPlatformControl ();
	NSMenu* nsMenu = [[menuClass alloc] performSelector:@selector(initWithOptionMenu:) withObject:(id)optionMenu];
	CPoint p (optionMenu->getViewSize ().left, optionMenu->getViewSize ().top);
	optionMenu->localToFrame (p);
	NSRect cellFrameRect = {0};
	cellFrameRect.origin = nsPointFromCPoint (p);
	cellFrameRect.size.width = optionMenu->getViewSize ().getWidth ();
	cellFrameRect.size.height = optionMenu->getViewSize ().getHeight ();
	if (!(optionMenu->getStyle () & kPopupStyle))
		[nsMenu insertItemWithTitle:@"" action:nil keyEquivalent:@"" atIndex:0];
	if (!multipleCheck && optionMenu->getStyle () & kCheckStyle)
		[[nsMenu itemWithTag:(NSInteger)optionMenu->getValue ()] setState:NSOnState];

	NSView* cellContainer = [[NSView alloc] initWithFrame:cellFrameRect];
	[view addSubview:cellContainer];
	cellFrameRect.origin.x = 0;
	cellFrameRect.origin.y = 0;

	NSPopUpButtonCell* cell = [[NSPopUpButtonCell alloc] initTextCell:@"" pullsDown:optionMenu->getStyle () & kPopupStyle ? NO : YES];
	[cell setAltersStateOfSelectedItem: NO];
	[cell setAutoenablesItems:NO];
	[cell setMenu:nsMenu];
	if (optionMenu->getStyle () & kPopupStyle)
		[cell selectItemWithTag:(NSInteger)optionMenu->getValue ()];
	[cell performClickWithFrame:cellFrameRect inView:cellContainer];
	[cellContainer removeFromSuperviewWithoutNeedingDisplay];
	[cellContainer release];
	result.menu = (COptionMenu*)[nsMenu performSelector:@selector(selectedMenu)];
	result.index = (long)[nsMenu performSelector:@selector(selectedItem)];

	return result;
}


} // namespace

#endif // MAC_COCOA && VSTGUI_PLATFORM_ABSTRACTION