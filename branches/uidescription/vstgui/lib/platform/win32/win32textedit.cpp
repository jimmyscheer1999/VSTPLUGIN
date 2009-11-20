/*
 *  win32textedit.cpp
 *  VST3PlugIns
 *
 *  Created by Arne Scheffler on 11/18/09.
 *  Copyright 2009 Arne Scheffler. All rights reserved.
 *
 */

#include "win32textedit.h"

#if WINDOWS

#include "win32support.h"
#include "../../vstkeycode.h"

namespace VSTGUI {

//-----------------------------------------------------------------------------
Win32TextEdit::Win32TextEdit (HWND parent, IPlatformTextEditCallback* textEdit)
: IPlatformTextEdit (textEdit)
, platformControl (0)
, platformFont (0)
, platformBackColor (0)
, oldWndProcEdit (0)
{
	CRect rect = textEdit->platformGetSize ();
	CFontRef fontID = textEdit->platformGetFont ();
	
	CHoriTxtAlign horiTxtAlign = textEdit->platformGetHoriTxtAlign ();
	int wstyle = 0;
	if (horiTxtAlign == kLeftText)
		wstyle |= ES_LEFT;
	else if (horiTxtAlign == kRightText)
		wstyle |= ES_RIGHT;
	else
		wstyle |= ES_CENTER;

	CPoint textInset = textEdit->platformGetTextInset ();
	rect.offset (textInset.x, textInset.y);
	rect.right -= textInset.x*2;
	rect.bottom -= textInset.y*2;

	// get/set the current font
	LOGFONT logfont = {0};

	CCoord fontH = fontID->getSize ();
	if (fontH > rect.height ())
		fontH = rect.height () - 3;
	if (fontH < rect.height ())
	{
		CCoord adjust = (rect.height () - (fontH + 3)) / (CCoord)2;
		rect.top += adjust;
		rect.bottom -= adjust;
	}
	UTF8StringHelper stringHelper (textEdit->platformGetText ());

	wstyle |= WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL;
	platformControl = CreateWindowEx (WS_EX_TRANSPARENT,
		TEXT("EDIT"), stringHelper, wstyle,
		(int)rect.left, (int)rect.top, (int)rect.width (), (int)rect.height (),
		parent, NULL, GetInstance (), 0);

	logfont.lfWeight = FW_NORMAL;
	logfont.lfHeight = (LONG)-fontH;
	logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
	UTF8StringHelper fontNameHelper (fontID->getName ());
	VSTGUI_STRCPY (logfont.lfFaceName, fontNameHelper);

	logfont.lfClipPrecision	 = CLIP_STROKE_PRECIS;
	logfont.lfOutPrecision	 = OUT_STRING_PRECIS;
	logfont.lfQuality 	     = DEFAULT_QUALITY;
	logfont.lfCharSet        = ANSI_CHARSET;
  
	platformFont = CreateFontIndirect (&logfont);

	SetWindowLongPtr (platformControl, GWLP_USERDATA, (__int3264)(LONG_PTR)this);
	SendMessage (platformControl, WM_SETFONT, (WPARAM)platformFont, true);
	SendMessage (platformControl, EM_SETMARGINS, EC_LEFTMARGIN|EC_RIGHTMARGIN, MAKELONG (0, 0));
	SendMessage (platformControl, EM_SETSEL, 0, -1);
	SendMessage (platformControl, EM_LIMITTEXT, 255, 0);
	SetFocus (platformControl);

	oldWndProcEdit = (WINDOWSPROC)(LONG_PTR)SetWindowLongPtr (platformControl, GWLP_WNDPROC, (__int3264)(LONG_PTR)procEdit);

	CColor backColor = textEdit->platformGetBackColor ();
	platformBackColor = CreateSolidBrush (RGB (backColor.red, backColor.green, backColor.blue));
}

//-----------------------------------------------------------------------------
Win32TextEdit::~Win32TextEdit ()
{
	if (platformControl)
	{
		SetWindowLongPtr (platformControl, GWLP_WNDPROC, (__int3264)(LONG_PTR)oldWndProcEdit);
		DestroyWindow (platformControl);
	}
	if (platformFont)
		DeleteObject (platformFont);
	if (platformBackColor)
		DeleteObject (platformBackColor);
}

//-----------------------------------------------------------------------------
bool Win32TextEdit::getText (char* text, long maxSize)
{
	if (platformControl)
	{
		TCHAR* newText = new TCHAR[maxSize+1];
		GetWindowText (platformControl, newText, maxSize);
		UTF8StringHelper windowText (newText);
		strncpy (text, windowText, maxSize-1);
		delete [] newText;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool Win32TextEdit::setText (const char* text)
{
	if (platformControl)
	{
		UTF8StringHelper windowText (text);
		return SetWindowText (platformControl, windowText) ? true : false;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool Win32TextEdit::updateSize ()
{
	return false;
}

//-----------------------------------------------------------------------------
LONG_PTR WINAPI Win32TextEdit::procEdit (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{	
	Win32TextEdit* win32TextEdit = (Win32TextEdit*)(LONG_PTR) GetWindowLongPtr (hwnd, GWLP_USERDATA);
	if (win32TextEdit)
	{
		WINDOWSPROC oldProc = win32TextEdit->oldWndProcEdit;
		switch (message)
		{
			case WM_GETDLGCODE :
			{
				long flags = DLGC_WANTALLKEYS;
				return flags;
			}

			case WM_KEYDOWN:
			{
				if (win32TextEdit->textEdit)
				{
					if (wParam == VK_RETURN)
					{
						win32TextEdit->textEdit->platformLooseFocus (true);
						return 0;
					}
					else if (wParam == VK_TAB)
					{
						VstKeyCode keyCode = {0};
						keyCode.virt = VKEY_TAB;
						keyCode.modifier = GetKeyState (VK_SHIFT) < 0 ? MODIFIER_SHIFT : 0;
						if (win32TextEdit->textEdit->platformOnKeyDown (keyCode))
							return 0;
					}
				}
			} break;

			case WM_KILLFOCUS:
			{
				if (win32TextEdit->textEdit)
				{
					win32TextEdit->textEdit->platformLooseFocus (false);
					return 0;
				}
			} break;
//			case WM_PAINT:
//			{
//				LONG_PTR result = CallWindowProc (oldProc, hwnd, message, wParam, lParam);
//				return result;
//				break;
//			}
		}
		return CallWindowProc (oldProc, hwnd, message, wParam, lParam);
	}
	return DefWindowProc (hwnd, message, wParam, lParam);
}

} // namespace

#endif // WINDOWS