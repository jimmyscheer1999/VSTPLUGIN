//-----------------------------------------------------------------------------
// VST Plug-Ins SDK
// VSTGUI: Graphical User Interface Framework not only for VST plugins : 
//
// Version 4.0
//
//-----------------------------------------------------------------------------
// VSTGUI LICENSE
// (c) 2009, Steinberg Media Technologies, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A  PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#if VSTGUI_LIVE_EDITING

#include "cviewinspector.h"
#include "cselection.h"
#include "viewfactory.h"
#include "../lib/cscrollview.h"
#include "../lib/ctabview.h"
#include "../lib/cdatabrowser.h"
#include "../lib/vstkeycode.h"
#include "../lib/cgraphicspath.h"
#include "../lib/cfont.h"
#include <set>
#include <vector>
#include <algorithm>
#include <sstream>

BEGIN_NAMESPACE_VSTGUI

//-----------------------------------------------------------------------------
class InspectorTabButton : public COnOffButton
//-----------------------------------------------------------------------------
{
public:
	InspectorTabButton (const CRect &size, const char* inName, long tabPosition = 0)
	: COnOffButton (size, 0, -1, 0)
	, name (0)
	, tabPosition (tabPosition)
	{
		if (inName)
		{
			name = (char*)malloc (strlen (inName) + 1);
			strcpy (name, inName);
		}
		backgroundColor = kDefaultUIDescriptionBackgroundColor;
		activeTextColor = kWhiteCColor;
		inactiveTextColor = kGreyCColor;
		textFont = kSystemFont; textFont->remember ();
	}

	virtual ~InspectorTabButton ()
	{
		if (textFont)
			textFont->forget ();
		if (name)
			free (name);
	}	

	virtual void draw (CDrawContext *pContext)
	{
		pContext->setDrawMode (kCopyMode);
		pContext->setFillColor (backgroundColor);
		CRect r (size);
		if (tabPosition < 0)
		{
			CRect r2 (r);
			r.left += r.getHeight () / 2;

			r2.right = r2.left + r2.getHeight ();
			pContext->drawArc (r2, 0, 90, kDrawFilled);
			r2.top += r.getHeight () / 2;
			r2.right = r.left+1;
			pContext->drawRect (r2, kDrawFilled);
		}
		else if (tabPosition > 0)
		{
			CRect r2 (r);
			r.right -= r.getHeight () / 2;

			r2.left = r2.right - r2.getHeight ();
			pContext->drawArc (r2, 270, 360, kDrawFilled);
			r2.top += r.getHeight () / 2;
			r2.left = r.right-1;
			pContext->drawRect (r2, kDrawFilled);
		}
		pContext->drawRect (r, kDrawFilled);
		if (name)
		{
			if (getFrame ()->getFocusView () == this)
			{
				CFontRef focusFont = (CFontRef)textFont->newCopy ();
				focusFont->setStyle (textFont->getStyle () | kUnderlineFace);
				pContext->setFont (focusFont);
				focusFont->forget ();
			}
			else
				pContext->setFont (textFont);
			pContext->setFontColor (value ? activeTextColor : inactiveTextColor);
			pContext->drawString (name, size, false);
		}
		CColor lineColor (backgroundColor);
		lineColor.alpha += 50;
		pContext->setFrameColor (lineColor);
		pContext->setLineWidth (1);
		pContext->setLineStyle (kLineSolid);
		pContext->moveTo (CPoint (size.left, size.bottom));
		pContext->lineTo (CPoint (size.right, size.bottom));
	}

	CMouseEventResult onMouseDown (CPoint &where, const long& button)
	{
		beginEdit ();
		value = ((long)value) ? 0.f : 1.f;
		
		if (listener)
			listener->valueChanged (this);
		endEdit ();
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}

	virtual void onDragEnter (CDragContainer* drag, const CPoint& where)
	{
		if (value == 0.f)
		{
			value = 1.f;
			if (listener)
				listener->valueChanged (this);
		}
	}

	CLASS_METHODS (InspectorTabButton, COnOffButton)
protected:
	char* name;
	CFontRef textFont;
	CColor activeTextColor;
	CColor inactiveTextColor;
	CColor backgroundColor;
	long tabPosition;
};


class AttributeChangeAction : public IActionOperation, protected std::map<CView*, std::string>
{
public:
	AttributeChangeAction (UIDescription* desc, CSelection* selection, const std::string& attrName, const std::string& attrValue)
	: desc (desc)
	, attrName (attrName)
	, attrValue (attrValue)
	{
		ViewFactory* viewFactory = dynamic_cast<ViewFactory*> (desc->getViewFactory ());
		std::string attrOldValue;
		FOREACH_IN_SELECTION(selection, view)
			viewFactory->getAttributeValue (view, attrName, attrOldValue, desc);
			insert (std::make_pair (view, attrOldValue));
			view->remember ();
		FOREACH_IN_SELECTION_END
		name = "'" + attrName + "' change";
	}

	~AttributeChangeAction ()
	{
		const_iterator it = begin ();
		while (it != end ())
		{
			(*it).first->forget ();
			it++;
		}
	}

	const char* getName ()
	{
		return name.c_str ();
	}
	
	void perform ()
	{
		ViewFactory* viewFactory = dynamic_cast<ViewFactory*> (desc->getViewFactory ());
		UIAttributes attr;
		attr.setAttribute (attrName.c_str (), attrValue.c_str ());
		const_iterator it = begin ();
		while (it != end ())
		{
			(*it).first->invalid ();	// we need to invalid before changing anything as the size may change
			viewFactory->applyAttributeValues ((*it).first, attr, desc);
			(*it).first->invalid ();	// and afterwards also
			it++;
		}
	}
	
	void undo ()
	{
		ViewFactory* viewFactory = dynamic_cast<ViewFactory*> (desc->getViewFactory ());
		const_iterator it = begin ();
		while (it != end ())
		{
			UIAttributes attr;
			attr.setAttribute (attrName.c_str (), (*it).second.c_str ());
			(*it).first->invalid ();	// we need to invalid before changing anything as the size may change
			viewFactory->applyAttributeValues ((*it).first, attr, desc);
			(*it).first->invalid ();	// and afterwards also
			it++;
		}
	}
protected:
	UIDescription* desc;
	std::string attrName;
	std::string attrValue;
	std::string name;
};

//-----------------------------------------------------------------------------
class FocusOptionMenu : public COptionMenu
{
public:
	FocusOptionMenu (const CRect& size, CControlListener* listener, long tag, CBitmap* background = 0, CBitmap* bgWhenClick = 0, const long style = 0)
	: COptionMenu (size, listener, tag, background, bgWhenClick, style) {}
	
	void takeFocus ()
	{
		origBackgroundColor = backColor;
		backColor.alpha = 255;
	}
	
	void looseFocus ()
	{
		backColor = origBackgroundColor;
	}

protected:
	CColor origBackgroundColor;
};

//-----------------------------------------------------------------------------
class SimpleBooleanButton : public CParamDisplay
{
public:
	SimpleBooleanButton (const CRect& size, CControlListener* listener)
	: CParamDisplay (size)
	{
		setListener (listener);
		setStringConvert (booleanStringConvert);
		setWantsFocus (true);
	}

	static void booleanStringConvert (float value, char* string)
	{
		if (value == 0)
			strcpy (string, "false");
		else
			strcpy (string, "true");
	}
	
	CMouseEventResult onMouseDown (CPoint &where, const long& buttons)
	{
		value = value == 0 ? 1 : 0;
		beginEdit ();
		if (listener)
			listener->valueChanged (this);
		endEdit ();
		invalid ();
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}

	void takeFocus ()
	{
		origBackgroundColor = backColor;
		backColor.alpha = 255;
	}
	
	void looseFocus ()
	{
		backColor = origBackgroundColor;
	}
	
	long onKeyDown (VstKeyCode& keyCode)
	{
		if (keyCode.virt == VKEY_RETURN && keyCode.modifier == 0)
		{
			value = ((long)value) ? 0.f : 1.f;
			invalid ();
			beginEdit ();
			if (listener)
				listener->valueChanged (this);
			endEdit ();
			return 1;
		}
		return -1;
	}
	CColor origBackgroundColor;
};

//-----------------------------------------------------------------------------
class BrowserDelegateBase : public CBaseObject, public IDataBrowser
{
public:
	BrowserDelegateBase (UIDescription* desc, IActionOperator* actionOperator) : desc (desc), actionOperator (actionOperator), mouseRow (-1) {}
	
	virtual void getNames (std::list<const std::string*>& _names) = 0;

	void updateNames ()
	{
		std::list<const std::string*> _names;
		getNames (_names);
		names.clear ();
		names.insert (names.begin (), _names.begin (), _names.end ());
	}

	long dbGetNumRows (CDataBrowser* browser)
	{
		return names.size () + 1;
	}
	
	long dbGetNumColumns (CDataBrowser* browser)
	{
		return 3;
	}
	
	bool dbGetColumnDescription (long index, CCoord& minWidth, CCoord& maxWidth, CDataBrowser* browser)
	{
		return false;
	}
	
	CCoord dbGetCurrentColumnWidth (long index, CDataBrowser* browser)
	{
		if (index == 2)
			return 20;
		return (browser->getWidth () - 40) / 2;
	}
	
	void dbSetCurrentColumnWidth (long index, const CCoord& width, CDataBrowser* browser)
	{
	}
	
	CCoord dbGetRowHeight (CDataBrowser* browser)
	{
		return 20;
	}
	
	bool dbGetLineWidthAndColor (CCoord& width, CColor& color, CDataBrowser* browser)
	{
		width = 1;
		color = MakeCColor (255, 255, 255, 20);
		return true;
	}

	void dbDrawHeader (CDrawContext* context, const CRect& size, long column, long flags, CDataBrowser* browser)
	{
	}

	void drawBackgroundSelected (CDrawContext* context, const CRect& size)
	{
		context->setFillColor (MakeCColor (255, 255, 255, 10));
		context->drawRect (size, kDrawFilled);
	}

	void dbDrawCell (CDrawContext* context, const CRect& size, long row, long column, long flags, CDataBrowser* browser)
	{
		if (flags & kRowSelected)
			drawBackgroundSelected (context, size);
		if (row >= dbGetNumRows (browser)-1)
		{
			if (column == 0)
			{
				context->setFontColor (kRedCColor);
				context->setFont (kNormalFont);
				context->drawStringUTF8 ("<< Insert New >>", size);
			}
		}
		else if (column == 0)
		{
			context->setFontColor (kWhiteCColor);
			context->setFont (kNormalFont);
			context->drawStringUTF8 (names[row]->c_str (), size);
		}
		else if (column == dbGetNumColumns (browser)-1)
		{
			static CGraphicsPath path;
			static bool once = true;
			if (once)
			{
				path.addEllipse (CRect (0, 0, 1, 1));
				path.addLine (CPoint (0.3, 0.3), CPoint (0.7, 0.7));
				path.addLine (CPoint (0.3, 0.7), CPoint (0.7, 0.3));
				once = false;
			}
			CRect r (size);
			r.inset (4, 4);
			CGraphicsTransformation trans;
			trans.offset = r.getTopLeft ();
			trans.scaleX = r.getWidth ();
			trans.scaleY = r.getHeight ();
			context->setFrameColor (mouseRow == row ? kRedCColor : kGreyCColor);
			context->setLineWidth (1.5);
			context->setDrawMode (kAntialias);
			path.draw (context, CGraphicsPath::kStroked, &trans);
		}
		else
		{
			std::string text;
			getCellText (row, column, text, browser);
			context->setFontColor (kWhiteCColor);
			context->setFont (kNormalFont);
			context->drawStringUTF8 (text.c_str (), size);
		}
	}

	virtual bool getCellText (long row, long column, std::string& result, CDataBrowser* browser)	// return true if cell is editable
	{
		if (row >= dbGetNumRows (browser)-1)
		{
			if (column == 0)
			{
				result = "<< Insert New >>";
				return false;
			}
		}
		else if (column == 0)
		{
			result = *names[row];
			return true;
		}
		else if (column == dbGetNumColumns (browser)-1)
		{
			result = "x";
			return false;
		}
		return false;
	}

	void dbCellSetupTextEdit (long row, long column, CTextEdit* textEditControl, CDataBrowser* browser)
	{
		textEditControl->setBackColor (kWhiteCColor);
		textEditControl->setFrameColor (kBlackCColor);
		textEditControl->setFontColor (kBlackCColor);
		textEditControl->setFont (kNormalFont);
		CRect size = textEditControl->getViewSize (size);
		size.inset (1, 1);
		textEditControl->setViewSize (size, true);
		textEditControl->setMouseableArea (size);
	}

	CMouseEventResult dbOnMouseDown (const CPoint& where, const long& buttons, long row, long column, CDataBrowser* browser)
	{
		if (row == dbGetNumRows (browser)-1)
		{
			if (column == 0)
				browser->beginTextEdit (row, column, "");
		}
		else
		{
			std::string str;
			if (getCellText (row, column, str, browser))
				browser->beginTextEdit (row, column, str.c_str ());
		}
		return kMouseEventHandled;
	}

	CMouseEventResult dbOnMouseMoved (const CPoint& where, const long& buttons, long row, long column, CDataBrowser* browser)
	{
		if (column == dbGetNumColumns (browser)-1)
		{
			if (mouseRow != row)
			{
				browser->invalidateRow (mouseRow);
				mouseRow = row;
				browser->invalidateRow (mouseRow);
			}
		}
		else if (mouseRow != -1)
		{
			browser->invalidateRow (mouseRow);
			mouseRow = -1;
		}
		return kMouseEventHandled;
	}

	virtual bool startEditing (long row, CDataBrowser* browser)
	{
		if (row == dbGetNumRows (browser)-1)
		{
			browser->beginTextEdit (row, 0, "");
			return true;
		}
		else if (row != -1)
		{
			std::string str;
			if (getCellText (row, 1, str, browser))
			{
				browser->beginTextEdit (row, 1, str.c_str ());
				return true;
			}
		}
		return false;
	}
	
	long dbOnKeyDown (const VstKeyCode& key, CDataBrowser* browser)
	{
		if (key.virt == VKEY_RETURN)
		{
			long row = browser->getSelectedRow ();
			if (startEditing (row, browser))
				return 1;
		}
		
		return -1;
	}


protected:
	UIDescription* desc;
	IActionOperator* actionOperator;
	std::vector<const std::string*> names;
	long mouseRow;
};

//-----------------------------------------------------------------------------
class BitmapBrowserDelegate : public BrowserDelegateBase
{
public:
	BitmapBrowserDelegate (UIDescription* desc, IActionOperator* actionOperator) : BrowserDelegateBase (desc, actionOperator) { updateNames (); }

	void getNames (std::list<const std::string*>& _names)
	{
		desc->collectBitmapNames (_names);
	}
	
	bool getCellText (long row, long column, std::string& result, CDataBrowser* browser)
	{
		if (column == 1)
		{
			CBitmap* bitmap = desc->getBitmap (names[row]->c_str ());
			if (bitmap)
			{
				result = bitmap->getResourceDescription ().u.name;
				return true;
			}
		}
		return BrowserDelegateBase::getCellText (row, column, result, browser);
	}

	CMouseEventResult dbOnMouseDown (const CPoint& where, const long& buttons, long row, long column, CDataBrowser* browser)
	{
		if (row < (dbGetNumRows (browser) - 1))
		{
			if (column == 2)
			{
				actionOperator->performBitmapChange (names[row]->c_str (), 0, true);
				updateNames ();
				browser->recalculateLayout (true);
				return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
			}
		}
		return BrowserDelegateBase::dbOnMouseDown (where, buttons, row, column, browser);
	}

	void dbCellTextChanged (long row, long column, const char* newText, CDataBrowser* browser)
	{
		if (newText == 0 || strlen (newText) == 0)
			return;
		std::string str;
		if (getCellText (row, column, str, browser) && str == newText)
			return;
		if (column == 0)
		{
			if (row == dbGetNumRows (browser)-1)
			{
				if (!desc->getBitmap (newText))
				{
					actionOperator->performBitmapChange (newText, "not yet defined");
					browser->beginTextEdit (row, 1, "not yet defined");
				}
			}
			else
			{
				actionOperator->performBitmapNameChange (names[row]->c_str (), newText);
			}
			updateNames ();
			browser->recalculateLayout (true);
		}
		else if (column == 1)
		{
			actionOperator->performBitmapChange (names[row]->c_str (), newText);
		}
	}
};

//-----------------------------------------------------------------------------
class ColorBrowserDelegate : public BrowserDelegateBase, public IPlatformColorChangeCallback
{
public:
	ColorBrowserDelegate (UIDescription* desc, IActionOperator* actionOperator) : BrowserDelegateBase (desc, actionOperator), browser (0) { updateNames (); }
	~ColorBrowserDelegate ()
	{
		PlatformUtilities::colorChooser (0, this);
	}
	
	void getNames (std::list<const std::string*>& _names)
	{
		lastChoosenRow = -1;
		desc->collectColorNames (_names);
	}
	
	bool getCellText (long row, long column, std::string& result, CDataBrowser* browser)
	{
		if (column == 1)
		{
			CColor color;
			if (desc->getColor (names[row]->c_str (), color))
			{
				unsigned char red = color.red;
				unsigned char green = color.green;
				unsigned char blue = color.blue;
				unsigned char alpha = color.alpha;
				char strBuffer[10];
				sprintf (strBuffer, "#%02x%02x%02x%02x", red, green, blue, alpha);
				result = strBuffer;
				return true;
			}
		}
		return BrowserDelegateBase::getCellText (row, column, result, browser);
	}

	bool startEditing (long row, CDataBrowser* browser)
	{
		if (row < (dbGetNumRows (browser) - 1))
		{
			CColor color;
			if (desc->getColor (names[row]->c_str (), color))
			{
				this->browser = browser;
				lastChoosenRow = row;
				PlatformUtilities::colorChooser (&color, this);
			}
			return true;
		}
		return BrowserDelegateBase::startEditing (row, browser);
	}
	
	CMouseEventResult dbOnMouseDown (const CPoint& where, const long& buttons, long row, long column, CDataBrowser* browser)
	{
		if (row < (dbGetNumRows (browser) - 1))
		{
			if (column == 1)
			{
				CColor color;
				if (desc->getColor (names[row]->c_str (), color))
				{
					this->browser = browser;
					lastChoosenRow = row;
					PlatformUtilities::colorChooser (&color, this);
				}
				return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
			}
			else if (column == 2)
			{
				actionOperator->performColorChange (names[row]->c_str (), MakeCColor (0, 0, 0, 0), true);
				updateNames ();
				browser->recalculateLayout (true);
				return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
			}
		}
		return BrowserDelegateBase::dbOnMouseDown (where, buttons, row, column, browser);
	}

	void colorChanged (const CColor& color)
	{
		if (lastChoosenRow != -1 && names.size () > lastChoosenRow)
		{
			long temp = lastChoosenRow;
			actionOperator->performColorChange (names[lastChoosenRow]->c_str (), color);
			updateNames ();
			lastChoosenRow = temp;
			browser->invalidateRow (lastChoosenRow);
		}
	}
	
	void dbCellTextChanged (long row, long column, const char* newText, CDataBrowser* browser)
	{
		if (newText == 0 || strlen (newText) == 0)
			return;
		std::string str;
		if (getCellText (row, column, str, browser) && str == newText)
			return;
		if (column == 0)
		{
			if (row == dbGetNumRows (browser)-1)
			{
				CColor color;
				if (!desc->getColor (newText, color))
				{
					color = MakeCColor (1, 2, 3, 4);
					actionOperator->performColorChange (newText, color);
				}
			}
			else
			{
				actionOperator->performColorNameChange (names[row]->c_str (), newText);
			}
			updateNames ();
			browser->recalculateLayout (true);
		}
		else if (column == 1)
		{
			std::string colorString (newText);
			CColor newColor;
			if (UIDescription::parseColor (colorString, newColor))
			{
				actionOperator->performColorChange (names[row]->c_str (), newColor);
			}
		}
	}

	void dbDrawCell (CDrawContext* context, const CRect& size, long row, long column, long flags, CDataBrowser* browser)
	{
		if (column == 1 && row < (dbGetNumRows (browser) - 1))
		{
			if (flags & kRowSelected)
				drawBackgroundSelected (context, size);
			CColor color;
			if (desc->getColor (names[row]->c_str (), color))
			{
				CRect colorRect (size);
				colorRect.setWidth (colorRect.getHeight ());
				colorRect.inset (2, 2);
				context->setFillColor (color);
				context->drawRect (colorRect, kDrawFilled);

				unsigned char red = color.red;
				unsigned char green = color.green;
				unsigned char blue = color.blue;
				unsigned char alpha = color.alpha;
				char strBuffer[10];
				sprintf (strBuffer, "#%02x%02x%02x%02x", red, green, blue, alpha);
				context->setFontColor (kWhiteCColor);
				context->setFont (kNormalFont);
				context->drawStringUTF8 (strBuffer, size);
			}
			return;
		}
		BrowserDelegateBase::dbDrawCell (context, size, row, column, flags, browser);
	}
	long lastChoosenRow;
	CDataBrowser* browser;
};

//-----------------------------------------------------------------------------
class TagBrowserDelegate : public BrowserDelegateBase
{
public:
	TagBrowserDelegate (UIDescription* desc, IActionOperator* actionOperator) : BrowserDelegateBase (desc, actionOperator) { updateNames (); }

	void getNames (std::list<const std::string*>& _names)
	{
		desc->collectControlTagNames (_names);
	}
	
	bool getCellText (long row, long column, std::string& result, CDataBrowser* browser)
	{
		if (column == 1 && row < (dbGetNumRows (browser) - 1))
		{
			std::stringstream str;
			str << desc->getTagForName (names[row]->c_str ());
			result = str.str ();
			return true;
		}
		return BrowserDelegateBase::getCellText (row, column, result, browser);
	}

	CMouseEventResult dbOnMouseDown (const CPoint& where, const long& buttons, long row, long column, CDataBrowser* browser)
	{
		if (row < (dbGetNumRows (browser) - 1))
		{
			if (column == 2)
			{
				actionOperator->performTagChange (names[row]->c_str (), 0, true);
				updateNames ();
				browser->recalculateLayout (true);
				return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
			}
		}
		return BrowserDelegateBase::dbOnMouseDown (where, buttons, row, column, browser);
	}

	void dbCellTextChanged (long row, long column, const char* newText, CDataBrowser* browser)
	{
		if (newText == 0 || strlen (newText) == 0)
			return;
		std::string str;
		if (getCellText (row, column, str, browser) && str == newText)
			return;
		if (column == 0)
		{
			if (row == dbGetNumRows (browser)-1)
			{
				if (desc->getTagForName (newText) == -1)
				{
					actionOperator->performTagChange (newText, -2);
					browser->beginTextEdit (row, 1, "-2");
				}
			}
			else
			{
				actionOperator->performTagNameChange (names[row]->c_str (), newText);
			}
			updateNames ();
			browser->recalculateLayout (true);
		}
		else if (column == 1)
		{
			long tag = strtol (newText, 0, 10);
			actionOperator->performTagChange (names[row]->c_str (), tag);
		}
	}
};


//-----------------------------------------------------------------------------
class FontBrowserDelegate : public BrowserDelegateBase
{
public:
	FontBrowserDelegate (UIDescription* desc, IActionOperator* actionOperator) : BrowserDelegateBase (desc, actionOperator) { updateNames (); }

	void getNames (std::list<const std::string*>& _names)
	{
		desc->collectFontNames (_names);
	}
	
	long dbGetNumColumns (CDataBrowser* browser)
	{
		return 5;
	}
	
	CCoord dbGetCurrentColumnWidth (long index, CDataBrowser* browser)
	{
		if (index == 4)
			return 20;
		CCoord half = (browser->getWidth () - 40) / 2;
		switch (index)
		{
			case 0 : return half;
			case 1 : return half * (2./3.);
			default: return half * (1./6.);
		}
		return (browser->getWidth () - 40) / 4;
	}
	
	bool getCellText (long row, long column, std::string& result, CDataBrowser* browser)
	{
		if (row < (dbGetNumRows (browser) - 1))
		{
			CFontRef font = desc->getFont (names[row]->c_str ());
			if (!font)
				return false;
			if (column == 1)
			{
				result = font->getName ();
				return true;
			}
			else if (column == 2)
			{
				std::stringstream str;
				long fstyle = font->getStyle ();
				if (fstyle & kBoldFace)
					str << "b";
				if (fstyle & kItalicFace)
					str << "i";
				if (fstyle & kUnderlineFace)
					str << "u";
				result = str.str ();
				return true;
			}
			else if (column == 3)
			{
				std::stringstream str;
				str << font->getSize ();
				result = str.str ();
				return true;
			}
		}
		return BrowserDelegateBase::getCellText (row, column, result, browser);
	}

	//-----------------------------------------------------------------------------
	static CFontRef showFontMenu (CFrame* frame, const CPoint& location, CFontRef oldFont)
	{
		CFontRef result = 0;
		CMenuItem* item = 0;
		COptionMenu* fontMenu = new COptionMenu ();
		fontMenu->setStyle (kPopupStyle|kCheckStyle);
		std::list<std::string*> fontNames;
		if (PlatformUtilities::collectPlatformFontNames (fontNames))
		{
			fontNames.sort (std__stringCompare);
			std::list<std::string*>::const_iterator it = fontNames.begin ();
			while (it != fontNames.end ())
			{
				item = fontMenu->addEntry (new CMenuItem ((*it)->c_str ()));
				if (*(*it) == oldFont->getName ())
				{
					item->setChecked (true);
					fontMenu->setValue (fontMenu->getNbEntries ()-1);
				}
				delete (*it);
				it++;
			}
		}
		if (fontMenu->popup (frame, location))
		{
			long index = 0;
			COptionMenu* menu = fontMenu->getLastItemMenu (index);
			if (menu)
			{
				item = menu->getEntry (index);
				result = new CFontDesc (*oldFont);
				result->setName (item->getTitle ()); 
			}
		}
		fontMenu->forget ();
		return result;
	}

	bool startEditing (long row, CDataBrowser* browser)
	{
		if (row < (dbGetNumRows (browser) - 1))
		{
			CRect r = browser->getCellBounds (row, 1);
			CPoint location (r.getTopLeft ());
			browser->localToFrame (location);
			CFontRef currentFont = desc->getFont (names[row]->c_str ());
			CFontRef newFont = showFontMenu (browser->getFrame (), location, currentFont);
			if (newFont)
			{
				actionOperator->performFontChange (names[row]->c_str (), newFont);
				newFont->forget ();
				updateNames ();
				browser->recalculateLayout (true);
			}
			return true;
		}
		return BrowserDelegateBase::startEditing (row, browser);
	}
	
	CMouseEventResult dbOnMouseDown (const CPoint& where, const long& buttons, long row, long column, CDataBrowser* browser)
	{
		if (row < (dbGetNumRows (browser) - 1))
		{
			if (column == dbGetNumColumns (browser) - 1)
			{
				actionOperator->performFontChange (names[row]->c_str (), 0, true);
				updateNames ();
				browser->recalculateLayout (true);
				return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
			}
			else if (column == 1)
			{
				CPoint location (where);
				browser->localToFrame (location);
				CFontRef currentFont = desc->getFont (names[row]->c_str ());
				CFontRef newFont = showFontMenu (browser->getFrame (), location, currentFont);
				if (newFont)
				{
					actionOperator->performFontChange (names[row]->c_str (), newFont);
					newFont->forget ();
					updateNames ();
					browser->recalculateLayout (true);
				}
				return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
			}
			else if (column == 2)
			{
				CPoint location (where);
				browser->localToFrame (location);
				CFontRef currentFont = desc->getFont (names[row]->c_str ());
				COptionMenu* styleMenu = new COptionMenu ();
				styleMenu->setStyle (kPopupStyle|kCheckStyle|kMultipleCheckStyle);
				CMenuItem* item = styleMenu->addEntry (new CMenuItem ("Bold", kBoldFace));
				if (currentFont->getStyle () & kBoldFace)
					item->setChecked (true);
				item = styleMenu->addEntry (new CMenuItem ("Italic", kItalicFace));
				if (currentFont->getStyle () & kItalicFace)
					item->setChecked (true);
				item = styleMenu->addEntry (new CMenuItem ("Underline", kUnderlineFace));
				if (currentFont->getStyle () & kUnderlineFace)
					item->setChecked (true);
				if (styleMenu->popup (browser->getFrame (), location))
				{
					CFontRef newFont = new CFontDesc (*currentFont);
					long style = newFont->getStyle ();
					long index;
					styleMenu->getLastItemMenu (index);
					item = styleMenu->getEntry (index);
					if (item->isChecked ())
						style |= item->getTag ();
					else
						style &= ~item->getTag ();
					newFont->setStyle (style);
					actionOperator->performFontChange (names[row]->c_str (), newFont);
					newFont->forget ();
					updateNames ();
					browser->recalculateLayout (true);
				}
				return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
			}
		}
		return BrowserDelegateBase::dbOnMouseDown (where, buttons, row, column, browser);
	}

	void dbCellTextChanged (long row, long column, const char* newText, CDataBrowser* browser)
	{
		if (newText == 0 || strlen (newText) == 0)
			return;
		std::string str;
		if (getCellText (row, column, str, browser) && str == newText)
			return;
		if (column == 0)
		{
			if (row == dbGetNumRows (browser)-1)
			{
				if (!desc->getFont (newText))
				{
					CFontRef font = new CFontDesc (*kSystemFont);
					actionOperator->performFontChange (newText, font);
					font->forget ();
				}
			}
			else
			{
				actionOperator->performFontNameChange (names[row]->c_str (), newText);
			}
			updateNames ();
			browser->recalculateLayout (true);
		}
		else if (column == 3)
		{
			CFontRef currentFont = desc->getFont (names[row]->c_str ());
			if (currentFont)
			{
				CCoord size = strtol (newText, 0, 10);
				CFontRef newFont = new CFontDesc (*currentFont);
				newFont->setSize (size);
				actionOperator->performFontChange (names[row]->c_str (), newFont);
				newFont->forget ();
				updateNames ();
				browser->recalculateLayout (true);
			}
		}
	}
};

static const CViewAttributeID attrNameID = 'atnm';

//-----------------------------------------------------------------------------
static void setViewAttributeName (CView* view, const char* name)
{
	view->setAttribute (attrNameID, strlen (name) + 1, name);
}

//-----------------------------------------------------------------------------
static bool getViewAttributeName (CView* view, std::string& name)
{
	bool result = false;
	long attrSize = 0;
	if (view->getAttributeSize (attrNameID, attrSize))
	{
		char* attrNameCStr = new char [attrSize];
		if (view->getAttribute (attrNameID, attrSize, attrNameCStr, attrSize))
		{
			name = attrNameCStr;
			result = true;
		}
		delete [] attrNameCStr;
	}
	return result;
}

//-----------------------------------------------------------------------------
static void updateMenuFromList (COptionMenu* menu, std::list<const std::string*>& names, const std::string& defaultValue, bool addNoneItem = false)
{
	menu->removeAllEntry ();
	names.sort (std__stringCompare);
	long current = -1;
	std::list<const std::string*>::const_iterator it = names.begin ();
	while (it != names.end ())
	{
		menu->addEntry (new CMenuItem ((*it)->c_str ()));
		if (*(*it) == defaultValue)
			current = menu->getNbEntries () - 1;
		it++;
	}
	menu->setValue (current);
	if (addNoneItem)
	{
		menu->addSeparator ();
		menu->addEntry (new CMenuItem ("None", -1000));
		if (current == -1)
			menu->setValue (menu->getNbEntries () - 1);
	}
	else if (current == -1)
	{
		menu->addEntry (new CMenuItem (defaultValue.c_str ()));
		menu->setValue (menu->getNbEntries () - 1);
	}
}

//-----------------------------------------------------------------------------
static COptionMenu* createMenuFromList (const CRect& size, CControlListener* listener, std::list<const std::string*>& names, const std::string& defaultValue, bool addNoneItem = false)
{
	COptionMenu* menu = new FocusOptionMenu (size, listener, -1);
	menu->setStyle (kCheckStyle|kPopupStyle);
	updateMenuFromList (menu, names, defaultValue, addNoneItem);
	return menu;
}

//-----------------------------------------------------------------------------
CViewInspector::CViewInspector (CSelection* selection, IActionOperator* actionOperator)
: selection (selection)
, actionOperator (actionOperator)
, description (0)
, scrollView (0)
, platformWindow (0)
{
	selection->remember ();
	selection->addDependent (this);
}

//-----------------------------------------------------------------------------
CViewInspector::~CViewInspector ()
{
	hide ();
	setUIDescription (0);
	selection->removeDependent (this);
	selection->forget ();
}

//-----------------------------------------------------------------------------
void CViewInspector::setUIDescription (UIDescription* desc)
{
	if (description != desc)
	{
		if (description)
		{
			description->forget ();
		}
		description = desc;
		if (description)
		{
			UIAttributes* attr = description->getCustomAttributes ("CViewInspector");
			if (attr)
				attr->getRectAttribute ("windowSize", windowSize);
			description->remember ();
		}
	}
}

//-----------------------------------------------------------------------------
void CViewInspector::updateAttributeValueView (const std::string& attrName)
{
}

//-----------------------------------------------------------------------------
CView* CViewInspector::createViewForAttribute (const std::string& attrName, CCoord width)
{
	if (description == 0)
		return 0;
	ViewFactory* viewFactory = dynamic_cast<ViewFactory*> (description->getViewFactory ());
	if (viewFactory == 0)
		return 0;

	const CCoord height = 20;

	CViewContainer* container = new CViewContainer (CRect (0, 0, width, height), 0);
	container->setAutosizeFlags (kAutosizeLeft|kAutosizeRight|kAutosizeColumn);
	container->setTransparency (true);

	CCoord middle = width/2;

	CTextLabel* label = new CTextLabel (CRect (5, 0, middle - 10, height), attrName.c_str ());
	label->setTransparency (true);
	label->setHoriAlign (kRightText);
	label->setFontColor (kWhiteCColor);
	label->setFont (kNormalFont);
	container->addView (label);

	bool hasDifferentValues = false;
	CRect r (middle+10, 0, width-5, height);
	CView* valueView = 0;
	IViewCreator::AttrType attrType = viewFactory->getAttributeType (*selection->begin (), attrName);
	std::string attrValue;
	bool first = true;
	FOREACH_IN_SELECTION(selection, view)
		std::string temp;
		viewFactory->getAttributeValue (view, attrName, temp, description);
		if (temp != attrValue && !first)
			hasDifferentValues = true;
		attrValue = temp;
		first = false;
	FOREACH_IN_SELECTION_END
	switch (attrType)
	{
		case IViewCreator::kColorType:
		{
			std::list<const std::string*> names;
			description->collectColorNames (names);
			COptionMenu* menu = createMenuFromList (r, this, names, attrValue);
			if (menu->getValue () == -1)
			{
				menu->addEntry (new CMenuItem (attrValue.c_str ()));
				menu->setValue (menu->getNbEntries ());
			}
			valueView = menu;
			break;
		}
		case IViewCreator::kFontType:
		{
			std::list<const std::string*> names;
			description->collectFontNames (names);
			COptionMenu* menu = createMenuFromList (r, this, names, attrValue);
			valueView = menu;
			break;
		}
		case IViewCreator::kBitmapType:
		{
			std::list<const std::string*> names;
			description->collectBitmapNames (names);
			COptionMenu* menu = createMenuFromList (r, this, names, attrValue, true);
			valueView = menu;
			break;
		}
		case IViewCreator::kTagType:
		{
			std::list<const std::string*> names;
			description->collectControlTagNames (names);
			COptionMenu* menu = createMenuFromList (r, this, names, attrValue, true);
			valueView = menu;
			break;
		}
		case IViewCreator::kBooleanType:
		{
			SimpleBooleanButton* booleanButton = new SimpleBooleanButton (r, this);
			if (attrValue == "true")
				booleanButton->setValue (1);
			valueView = booleanButton;
			break;
		}
		default:
		{
			CTextEdit* textEdit = new CTextEdit (r, this, -1);
			textEdit->setText (attrValue.c_str ());
			valueView = textEdit;
		}
	}
	if (valueView)
	{
		CParamDisplay* paramDisplay = dynamic_cast<CParamDisplay*> (valueView);
		if (paramDisplay)
		{
			paramDisplay->setHoriAlign (kLeftText);
			paramDisplay->setBackColor (hasDifferentValues ? MakeCColor (100, 100, 255, 150) : MakeCColor (255, 255, 255, 150));
			paramDisplay->setFrameColor (MakeCColor (0, 0, 0, 180));
			paramDisplay->setFontColor (kBlackCColor);
			paramDisplay->setFont (kNormalFont);
			paramDisplay->setStyle (paramDisplay->getStyle ());
			paramDisplay->setTextInset (CPoint (4,0));
		}
		setViewAttributeName (valueView, attrName.c_str ());
		container->addView (valueView);
		attributeViews.push_back (valueView);
	}
	return container;
}

//-----------------------------------------------------------------------------
void CViewInspector::updateAttributeViews ()
{
	if (description == 0)
		return;
	ViewFactory* viewFactory = dynamic_cast<ViewFactory*> (description->getViewFactory ());
	if (viewFactory == 0)
		return;

	std::string attrName;
	std::string attrValue;
	std::list<CView*>::const_iterator it = attributeViews.begin ();
	while (it != attributeViews.end ())
	{
		CView* view = (*it);
		if (view && getViewAttributeName (view, attrName))
		{
			viewFactory->getAttributeValue (*selection->begin (), attrName, attrValue, description);
			CTextEdit* textEdit = dynamic_cast<CTextEdit*> (view);
			COptionMenu* optMenu = dynamic_cast<COptionMenu*> (view);
			SimpleBooleanButton* booleanButton = dynamic_cast<SimpleBooleanButton*> (view);
			if (textEdit)
			{
				textEdit->setText (attrValue.c_str ());
				if (textEdit->isDirty ())
					textEdit->invalid ();
			}
			else if (optMenu)
			{
				IViewCreator::AttrType type = viewFactory->getAttributeType (*selection->begin (), attrName);
				bool addNoneItem = false;
				std::list<const std::string*> names;
				switch (type)
				{
					case IViewCreator::kColorType:
					{
						description->collectColorNames (names);
						break;
					}
					case IViewCreator::kFontType:
					{
						description->collectFontNames (names);
						break;
					}
					case IViewCreator::kBitmapType:
					{
						description->collectBitmapNames (names);
						addNoneItem = true;
						break;
					}
					case IViewCreator::kTagType:
					{
						description->collectControlTagNames (names);
						addNoneItem = true;
						break;
					}
				}
				updateMenuFromList (optMenu, names, attrValue, addNoneItem);
			}
			else if (booleanButton)
			{
				float newValue = attrValue == "true" ? 1.f : 0.f;
				if (newValue != booleanButton->getValue ())
				{
					booleanButton->setValue (newValue);
					booleanButton->invalid ();
				}
			}
		}
		it++;
	}
}

//-----------------------------------------------------------------------------
CView* CViewInspector::createAttributesView (CCoord width)
{
	if (description == 0)
		return 0;
	ViewFactory* viewFactory = dynamic_cast<ViewFactory*> (description->getViewFactory ());
	if (viewFactory == 0)
		return 0;
	CRect size (0, 0, width, 400);
	if (scrollView == 0)
	{
		scrollView = new CScrollView (size, size, 0, CScrollView::kVerticalScrollbar|CScrollView::kDontDrawFrame, 10);
		scrollView->setBackgroundColor (kTransparentCColor);
		scrollView->setAutosizeFlags (kAutosizeAll);
		CScrollbar* bar = scrollView->getVerticalScrollbar ();
		bar->setScrollerColor (kDefaultUIDescriptionScrollerColor);
		bar->setBackgroundColor (kTransparentCColor);
		bar->setFrameColor (kTransparentCColor);
	}
	else
	{
		width = scrollView->getWidth ();
		size.setWidth (width);
		attributeViews.clear ();
		scrollView->removeAll ();
	}
	CCoord viewLocation = 0;
	CCoord containerWidth = width - 10;
	if (selection->total () > 0)
	{
		std::list<std::string> attrNames;
		FOREACH_IN_SELECTION(selection, view)
			std::list<std::string> temp;
			if (viewFactory->getAttributeNamesForView (view, temp))
			{
				if (attrNames.size () == 0)
					attrNames = temp;
				else
				{
					std::list<std::string>::const_iterator it = attrNames.begin ();
					while (it != attrNames.end ())
					{
						bool found = false;
						std::list<std::string>::const_iterator it2 = temp.begin ();
						while (it2 != temp.end ())
						{
							if ((*it) == (*it2))
							{
								found = true;
								break;
							}
							it2++;
						}
						if (!found)
							attrNames.remove (*it);
						it++;
					}
				}
			}
		FOREACH_IN_SELECTION_END
		std::list<std::string>::const_iterator it = attrNames.begin ();
		while (it != attrNames.end ())
		{
			CView* view = createViewForAttribute ((*it), containerWidth);
			if (view)
			{
				CRect viewSize = view->getViewSize ();
				viewSize.offset (0, viewLocation);
				view->setViewSize (viewSize);
				view->setMouseableArea (viewSize);
				viewLocation += view->getHeight () + 2;
				scrollView->addView (view);
			}
			it++;
		}
	}
	size.setHeight (viewLocation);
	scrollView->setContainerSize (size);
	scrollView->invalid ();
	return scrollView;
}

//-----------------------------------------------------------------------------
void CViewInspector::show ()
{
	if (platformWindow == 0)
	{
		CView* attributesView = createAttributesView (400);
		if (attributesView == 0)
			return;
		CRect size = attributesView->getViewSize ();
		if (size.getHeight () < 400)
			size.setHeight (400);

		ColorBrowserDelegate* colorDelegate = new ColorBrowserDelegate (description, actionOperator);
		CDataBrowser* colorBrowser = new CDataBrowser (size, 0, colorDelegate, CScrollView::kVerticalScrollbar|CScrollView::kDontDrawFrame|CDataBrowser::kDrawRowLines|CDataBrowser::kDrawColumnLines, 10);
		colorBrowser->setBackgroundColor (kTransparentCColor);
		colorBrowser->setAutosizeFlags (kAutosizeAll);
		colorDelegate->forget ();
		CScrollbar* bar = colorBrowser->getVerticalScrollbar ();
		bar->setScrollerColor (kDefaultUIDescriptionScrollerColor);
		bar->setBackgroundColor (kTransparentCColor);
		bar->setFrameColor (kTransparentCColor);

		BitmapBrowserDelegate* bmpDelegate = new BitmapBrowserDelegate (description, actionOperator);
		CDataBrowser* bitmapBrowser = new CDataBrowser (size, 0, bmpDelegate, CScrollView::kVerticalScrollbar|CScrollView::kDontDrawFrame|CDataBrowser::kDrawRowLines|CDataBrowser::kDrawColumnLines, 10);
		bitmapBrowser->setBackgroundColor (kTransparentCColor);
		bitmapBrowser->setAutosizeFlags (kAutosizeAll);
		bmpDelegate->forget ();
		bar = bitmapBrowser->getVerticalScrollbar ();
		bar->setScrollerColor (kDefaultUIDescriptionScrollerColor);
		bar->setBackgroundColor (kTransparentCColor);
		bar->setFrameColor (kTransparentCColor);

		FontBrowserDelegate* fontDelegate = new FontBrowserDelegate (description, actionOperator);
		CDataBrowser* fontBrowser = new CDataBrowser (size, 0, fontDelegate, CScrollView::kVerticalScrollbar|CScrollView::kDontDrawFrame|CDataBrowser::kDrawRowLines|CDataBrowser::kDrawColumnLines, 10);
		fontBrowser->setBackgroundColor (kTransparentCColor);
		fontBrowser->setAutosizeFlags (kAutosizeAll);
		fontDelegate->forget ();
		bar = fontBrowser->getVerticalScrollbar ();
		bar->setScrollerColor (kDefaultUIDescriptionScrollerColor);
		bar->setBackgroundColor (kTransparentCColor);
		bar->setFrameColor (kTransparentCColor);

		TagBrowserDelegate* tagDelegate = new TagBrowserDelegate (description, actionOperator);
		CDataBrowser* controlTagBrowser = new CDataBrowser (size, 0, tagDelegate, CScrollView::kVerticalScrollbar|CScrollView::kDontDrawFrame|CDataBrowser::kDrawRowLines|CDataBrowser::kDrawColumnLines, 10);
		controlTagBrowser->setBackgroundColor (kTransparentCColor);
		controlTagBrowser->setAutosizeFlags (kAutosizeAll);
		tagDelegate->forget ();
		bar = controlTagBrowser->getVerticalScrollbar ();
		bar->setScrollerColor (kDefaultUIDescriptionScrollerColor);
		bar->setBackgroundColor (kTransparentCColor);
		bar->setFrameColor (kTransparentCColor);

		const CCoord kMargin = 12;
		size.bottom += 50;
		size.offset (kMargin, 0);
		CRect tabButtonSize (0, 0, 400/5, 50);
		CTabView* tabView = new CTabView (size, 0, tabButtonSize, 0, CTabView::kPositionTop);
		tabView->setTabViewInsets (CPoint (5, 1));
		tabView->addTab (attributesView, new InspectorTabButton (tabButtonSize, "Attributes", -1));
		tabView->addTab (bitmapBrowser, new InspectorTabButton (tabButtonSize, "Bitmaps"));
		tabView->addTab (colorBrowser, new InspectorTabButton (tabButtonSize, "Colors"));
		tabView->addTab (fontBrowser, new InspectorTabButton (tabButtonSize, "Fonts"));
		tabView->addTab (controlTagBrowser, new InspectorTabButton (tabButtonSize, "Tags", 1));
		tabView->alignTabs ();
		tabView->setAutosizeFlags (kAutosizeAll);
		tabView->setBackgroundColor (kDefaultUIDescriptionBackgroundColor);

		size.offset (-kMargin, 0);
		size.right += kMargin*2;
		size.bottom += kMargin;
		platformWindow = PlatformWindow::create (size, "VSTGUI Inspector", PlatformWindow::kPanelType, PlatformWindow::kResizable, this);
		if (platformWindow)
		{
			#if MAC_CARBON
			CFrame::setCocoaMode (true);
			#endif
			frame = new CFrame (size, platformWindow->getPlatformHandle (), this);
#if WINDOWS
			frame->setBackgroundColor (MakeCColor (0, 0, 0, 255));
#else
			frame->setBackgroundColor (kDefaultUIDescriptionBackgroundColor);
#endif
			frame->addView (tabView);
			platformWindow->center ();
			if (windowSize.getWidth () > 0)
				platformWindow->setSize (windowSize);
			platformWindow->show ();
		}
		else
		{
			attributeViews.clear ();
			tabView->forget ();
			scrollView = 0;
		}
	}
}

//-----------------------------------------------------------------------------
void CViewInspector::beforeSave ()
{
	if (platformWindow)
		windowSize = platformWindow->getSize ();
	if (description)
	{
		UIAttributes* attr = description->getCustomAttributes ("CViewInspector");
		if (!attr)
			attr = new UIAttributes;
		attr->setRectAttribute ("windowSize", windowSize);
		description->setCustomAttributes ("CViewInspector", attr);
	}
}

//-----------------------------------------------------------------------------
void CViewInspector::hide ()
{
	if (platformWindow)
	{
		beforeSave ();
		attributeViews.clear ();
		frame->forget ();
		frame = 0;
		scrollView = 0;
		platformWindow->forget ();
		platformWindow = 0;
	}
}

//-----------------------------------------------------------------------------
void CViewInspector::valueChanged (CControl* pControl)
{
	if (description == 0)
		return;
	ViewFactory* viewFactory = dynamic_cast<ViewFactory*> (description->getViewFactory ());
	if (viewFactory == 0)
		return;

	std::string attrName;
	if (!getViewAttributeName (pControl, attrName))
		return;
	if (attrName.size () > 0)
	{
		std::string attrValue;
		CTextEdit* textEdit = dynamic_cast<CTextEdit*> (pControl);
		COptionMenu* optMenu = dynamic_cast<COptionMenu*> (pControl);
		SimpleBooleanButton* booleanButton = dynamic_cast<SimpleBooleanButton*> (pControl);
		if (textEdit)
		{
			const char* textValueCStr = textEdit->getText ();
			if (textValueCStr)
			{
				attrValue = textValueCStr;
			}
		}
		else if (optMenu)
		{
			long index = optMenu->getLastResult ();
			CMenuItem* item = optMenu->getEntry (index);
			if (item)
			{
				if (item->getTag () == -1000)
					attrValue = "";
				else
					attrValue = item->getTitle ();
			}
			optMenu->setValue (index);
			optMenu->invalid ();
		}
		else if (booleanButton)
		{
			attrValue = booleanButton->getValue () == 0 ? "false" : "true";
		}
		actionOperator->performAction (new AttributeChangeAction (description, selection, attrName, attrValue));
	}
}

//-----------------------------------------------------------------------------
CMessageResult CViewInspector::notify (CBaseObject* sender, const char* message)
{
	if (message == CSelection::kMsgSelectionChanged)
	{
		if (frame)
		{
			createAttributesView (400);
		}
		return kMessageNotified;
	}
	else if (message == CSelection::kMsgSelectionViewChanged)
	{
		if (frame)
		{
			updateAttributeViews ();
		}
		return kMessageNotified;
	}
	return kMessageUnknown;
}

//-----------------------------------------------------------------------------
void CViewInspector::checkWindowSizeConstraints (CPoint& size, PlatformWindow* platformWindow)
{
	if (size.x < 400)
		size.x = 400;
	if (size.y < 200)
		size.y = 200;
}

//-----------------------------------------------------------------------------
void CViewInspector::windowSizeChanged (const CRect& newSize, PlatformWindow* platformWindow)
{
	frame->setSize (newSize.getWidth (), newSize.getHeight ());
}

//-----------------------------------------------------------------------------
void CViewInspector::windowClosed (PlatformWindow* platformWindow)
{
}

END_NAMESPACE_VSTGUI

#endif // VSTGUI_LIVE_EDITING