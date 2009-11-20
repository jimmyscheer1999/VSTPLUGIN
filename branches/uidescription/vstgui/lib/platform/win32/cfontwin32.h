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

#ifndef __cfontwin32__
#define __cfontwin32__

#include "../../cfont.h"

#if WINDOWS

#include <windows.h>
#if GDIPLUS
#include <objidl.h>
#include <gdiplus.h>
#endif

BEGIN_NAMESPACE_VSTGUI

#if GDIPLUS
//-----------------------------------------------------------------------------
class GdiPlusFont : public CPlatformFont, public IFontPainter
{
public:
	GdiPlusFont (const char* name, const CCoord& size, const long& style);

	Gdiplus::Font* getFont () const { return font; }

protected:
	~GdiPlusFont ();
	
	double getAscent () const;
	double getDescent () const;
	double getLeading () const;
	double getCapHeight () const;

	IFontPainter* getPainter () { return this; }

	void drawString (CDrawContext* context, const char* utf8String, const CPoint& p, bool antialias = true);
	CCoord getStringWidth (CDrawContext* context, const char* utf8String, bool antialias = true);

	Gdiplus::Font* font;
	INT gdiStyle;
};
#else // GDIPLUS
//-----------------------------------------------------------------------------
class GdiFont : public CPlatformFont, public IFontPainter
{
	GdiFont (const char* name, const CCoord& size, const long& style);

	HANDLE getFont () const { return font; }
protected:
	~GdiFont ();
	
	double getAscent () const;
	double getDescent () const;
	double getLeading () const;
	double getCapHeight () const;

	IFontPainter* getPainter () { return this; }

	void drawString (CDrawContext* context, const char* utf8String, const CPoint& p, bool antialias = true);
	CCoord getStringWidth (CDrawContext* context, const char* utf8String, bool antialias = true);

	HANDLE font;
};

#endif

END_NAMESPACE_VSTGUI

#endif // WINDOWS

#endif