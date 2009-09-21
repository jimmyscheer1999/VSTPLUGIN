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

#ifndef __cautoanimation__
#define __cautoanimation__

#include "ccontrol.h"

BEGIN_NAMESPACE_VSTGUI

//-----------------------------------------------------------------------------
// CAutoAnimation Declaration
//!
/// \nosubgrouping
/// \ingroup controls
//-----------------------------------------------------------------------------
class CAutoAnimation : public CControl, public IMultiBitmapControl
{
public:
	//-----------------------------------------------------------------------------
	/// \name Constructor
	//-----------------------------------------------------------------------------
	//@{
	CAutoAnimation (const CRect& size, CControlListener* listener, long tag, CBitmap* background, const CPoint& offset = CPoint (0, 0));
	CAutoAnimation (const CRect& size, CControlListener* listener, long tag, long subPixmaps, CCoord heightOfOneImage, CBitmap* background, const CPoint& offset = CPoint (0, 0));
	CAutoAnimation (const CAutoAnimation& autoAnimation);
	//@}

	virtual ~CAutoAnimation ();

	virtual void draw (CDrawContext*);
	virtual CMouseEventResult onMouseDown (CPoint& where, const long& buttons);

	//-----------------------------------------------------------------------------
	/// \name CAutoAnimation Methods
	//-----------------------------------------------------------------------------
	//@{
	virtual void openWindow (void);			///< enabled drawing
	virtual void closeWindow (void);		///< disable drawing

	virtual void nextPixmap (void);			///< the next sub bitmap should be displayed
	virtual void previousPixmap (void);		///< the previous sub bitmap should be displayed

	bool    isWindowOpened () const { return bWindowOpened; }
	//@}

	void setNumSubPixmaps (long numSubPixmaps) { IMultiBitmapControl::setNumSubPixmaps (numSubPixmaps); invalid (); }

	CLASS_METHODS(CAutoAnimation, CControl)
protected:
	CPoint	offset;

	CCoord	totalHeightOfBitmap;

	bool	bWindowOpened;
};

END_NAMESPACE_VSTGUI

#endif
