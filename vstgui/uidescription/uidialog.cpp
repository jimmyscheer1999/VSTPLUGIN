//-----------------------------------------------------------------------------
// VST Plug-Ins SDK
// VSTGUI: Graphical User Interface Framework not only for VST plugins : 
//
// Version 4.0
//
//-----------------------------------------------------------------------------
// VSTGUI LICENSE
// (c) 2010, Steinberg Media Technologies, All Rights Reserved
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

#include "uidialog.h"
#include "../lib/vstkeycode.h"
#include "../lib/cfont.h"
#include "../lib/cdrawcontext.h"
#include "../lib/controls/cbuttons.h"

namespace VSTGUI {

//-----------------------------------------------------------------------------
bool UIDialog::runViewModal (CPoint& position, CView* view, int32_t style, UTF8StringPtr title)
{
	UIDialog dialog (position, view, style, title);
	return dialog.run ();
}

namespace DialogInternal {

static const CCoord roundRectRadius = 6.;
//-----------------------------------------------------------------------------
class SimpleButton : public CKickButton
{
public:
	SimpleButton (const CRect& size, CControlListener* listener, int32_t tag, UTF8StringPtr _title)
	: CKickButton (size, listener, tag, 0)
	{
		if (_title)
			title = _title;
		setWantsFocus (true);
	}

	bool getFocusPath (CGraphicsPath& outPath)
	{
		CRect r (getViewSize ());
		CCoord focusWidth = getFrame ()->getFocusWidth ();
		r.inset (-focusWidth, -focusWidth);
		outPath.addRoundRect (r, roundRectRadius);
		return true;
	}

	void draw (CDrawContext* context)
	{
		bool highlight = value > 0.5 ? true : false;
		context->setDrawMode (kAntiAliasing);
		context->setLineWidth (1);
		context->setLineStyle (kLineSolid);
		context->setFrameColor (kBlackCColor);
		CRect r (getViewSize ());
		r.inset (0.5, 0.5);
		CGraphicsPath* path = context->createRoundRectGraphicsPath (r, roundRectRadius);
		if (path)
		{
			CColor color1 = highlight ? MakeCColor (180,180,180,255) : MakeCColor (220,220,220,255);
			CColor color2 = highlight ? MakeCColor (100,100,100,255) : MakeCColor (180,180,180,255);
			CGradient* gradient = path->createGradient (0.2, 1, color1, color2);
			if (gradient)
			{
				context->fillLinearGradient (path, *gradient, r.getTopLeft (), r.getBottomLeft (), false);
				gradient->forget ();
			}
			else
			{
				context->setFillColor (value > 0.5 ? kGreyCColor : kWhiteCColor);
				context->drawGraphicsPath (path, CDrawContext::kPathFilled);
			}
			context->drawGraphicsPath (path, CDrawContext::kPathStroked);
			path->forget ();
		}
		else
		{
			context->setFillColor (value > 0.5 ? kGreyCColor : kWhiteCColor);
			context->drawRect (size, kDrawFilledAndStroked);
		}
		context->setFont (kSystemFont);
		context->setFontColor (value > 0.5 ? kWhiteCColor : kBlackCColor);
		context->drawString (title.c_str (), size);
	}

	int32_t onKeyDown (VstKeyCode& keyCode)
	{
		if ((keyCode.virt == VKEY_ENTER || keyCode.virt == VKEY_RETURN) && getFrame ()->getFocusView () == this)
		{
			beginEdit ();
			value = 1;
			valueChanged ();
			value = 0;
			endEdit ();
			return 1;
		}
		return -1;
	}
	
protected:
	std::string title;
};

//-----------------------------------------------------------------------------
enum {
	kOkTag,
	kCancelTag
};

#if MAC
static bool DialogOkIsRightMost = true;
#else
static bool DialogOkIsRightMost = false;
#endif

} // namespace DialogInternal

//-----------------------------------------------------------------------------
UIDialog::UIDialog (const CPoint& position, CView* rootView, int32_t style, UTF8StringPtr title)
: platformWindow (0)
, result (false)
{
	const CCoord kMargin = 10;
	const CCoord kControlHeight = 22;
	CRect size (rootView->getViewSize ());
	size.offset (-size.left, -size.top);
	rootView->setViewSize (size);
	rootView->setMouseableArea (size);
	size.bottom += kMargin*3+kControlHeight;
	if (position.x != -1 && position.y != -1)
		size.offset (position.x, position.y);
	platformWindow = PlatformWindow::create (size, title, PlatformWindow::kWindowType, 0, 0);
	if (platformWindow)
	{
		if (position.x == -1 && position.y == -1)
			platformWindow->center ();
		else
			size.offset (-position.x, -position.y);

		#if MAC_CARBON && MAC_COCOA
		CFrame::setCocoaMode (true);
		#endif

		frame = new CFrame (size, platformWindow->getPlatformHandle (), this);
		frame->setKeyboardHook (this);
		frame->setFocusDrawingEnabled (true);
		frame->setFocusColor (MakeCColor (100, 100, 255, 200));
		frame->setFocusWidth (1.2);
		CViewContainer* rootContainer = dynamic_cast<CViewContainer*> (rootView);
		if (rootContainer)
		{
			frame->setTransparency (rootContainer->getTransparency ());
			frame->setBackgroundColor (rootContainer->getBackgroundColor ());
		}
		else
		{
			frame->setTransparency (true);
		}
		frame->addView (rootView);
		rootView->remember (); // caller is responsible to forget it

		CRect r (rootView->getViewSize ());
		r.top = r.bottom+kMargin;
		r.bottom += kControlHeight+kMargin;
		r.setWidth (80);
		r.offset (size.right-(2*kMargin+r.getWidth ()), 0);

		bool okIsFirst = (DialogInternal::DialogOkIsRightMost || style == kOkButton);
		DialogInternal::SimpleButton* button;
		button = new DialogInternal::SimpleButton (r, this, okIsFirst ? DialogInternal::kOkTag : DialogInternal::kCancelTag, DialogInternal::DialogOkIsRightMost ? "OK" : "Cancel");
		frame->addView (button);
		if (style == kOkCancelButtons)
		{
			r.offset (-(r.getWidth () + kMargin), 0);
			button = new DialogInternal::SimpleButton (r, this, DialogInternal::DialogOkIsRightMost ? DialogInternal::kCancelTag : DialogInternal::kOkTag, DialogInternal::DialogOkIsRightMost ? "Cancel" : "OK");
			frame->addView (button);
		}
	}
}

//-----------------------------------------------------------------------------
UIDialog::~UIDialog ()
{
	if (frame)
		frame->close ();
	if (platformWindow)
		platformWindow->forget ();
}

//-----------------------------------------------------------------------------
bool UIDialog::run ()
{
	if (platformWindow)
	{
		platformWindow->show ();
		frame->setFocusView (0);
		frame->advanceNextFocusView (0, false);
		platformWindow->runModal ();
	}
	return result;
}

//-----------------------------------------------------------------------------
int32_t UIDialog::onKeyDown (const VstKeyCode& code, CFrame* frame)
{
	static bool recursion = false;
	if (recursion)
		return -1;

	recursion = true;

	VstKeyCode keyCode = code;
	int32_t keyResult = frame->onKeyDown (keyCode);

	recursion = false;

	if (keyResult == -1)
	{
		if (code.virt == VKEY_ESCAPE)
		{
			platformWindow->stopModal ();
			result = false;
			return 1;
		}
		else if (code.virt == VKEY_RETURN)
		{
			platformWindow->stopModal ();
			result = true;
			return 1;
		}
	}
	return keyResult;
}

//-----------------------------------------------------------------------------
int32_t UIDialog::onKeyUp (const VstKeyCode& code, CFrame* frame)
{
	return -1;
}

//-----------------------------------------------------------------------------
void UIDialog::valueChanged (CControl* pControl)
{
	switch (pControl->getTag ())
	{
		case DialogInternal::kOkTag:
		{
			if (pControl->getValue () > 0.5)
			{
				platformWindow->stopModal ();
				result = true;
			}
			break;
		}
		case DialogInternal::kCancelTag:
		{
			if (pControl->getValue () > 0.5)
			{
				platformWindow->stopModal ();
				result = false;
			}
			break;
		}
	}
}

} // namespace

#endif // VSTGUI_LIVE_EDITING