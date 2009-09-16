//-----------------------------------------------------------------------------
// VST Plug-Ins SDK
// VSTGUI: Graphical User Interface Framework for VST plugins : 
//
// Version 4.0
//
//-----------------------------------------------------------------------------
// VSTGUI LICENSE
// (c) 2008, Steinberg Media Technologies, All Rights Reserved
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

#ifndef __coffscreencontext__
#define __coffscreencontext__

#include "cdrawcontext.h"

BEGIN_NAMESPACE_VSTGUI
class CBitmap;

//-----------------------------------------------------------------------------
// COffscreenContext Declaration
//! \brief A drawing device which uses a pixmap as its drawing surface
/// \nosubgrouping
//-----------------------------------------------------------------------------
class COffscreenContext : public CDrawContext
{
public:
	//-----------------------------------------------------------------------------
	/// \name Constructors
	//-----------------------------------------------------------------------------
	//@{
	COffscreenContext (CDrawContext *pContext, CBitmap *pBitmap, bool drawInBitmap = false);
	COffscreenContext (CFrame *pFrame, long width, long height, const CColor backgroundColor = kBlackCColor);
	//@}
	virtual ~COffscreenContext ();
	
	//-----------------------------------------------------------------------------
	/// \name COffscreenContext Methods
	//-----------------------------------------------------------------------------
	//@{
	void copyFrom (CDrawContext *pContext, CRect destRect, CPoint srcOffset = CPoint (0, 0));	///< copy from offscreen to pContext
	void copyTo (CDrawContext* pContext, CRect& srcRect, CPoint destOffset = CPoint (0, 0));	///< copy to offscreen from pContext

	inline CCoord getWidth () const { return width; }
	inline CCoord getHeight () const { return height; }
	//@}

	//-------------------------------------------
protected:
	CBitmap	*pBitmap;
	CBitmap	*pBitmapBg;
	CCoord	height;
	CCoord	width;
	bool    bDestroyPixmap;
	bool	bDrawInBitmap;

	CColor  backgroundColor;

#if WINDOWS
	void* oldBitmap;
#endif // WINDOWS

#if VSTGUI_USES_COREGRAPHICS
	void* offscreenBitmap;
	virtual CGImageRef getCGImage () const;
	void releaseCGContext (CGContextRef context);
#endif // VSTGUI_USES_COREGRAPHICS

};

END_NAMESPACE_VSTGUI

#endif
