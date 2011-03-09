//-----------------------------------------------------------------------------
// VST Plug-Ins SDK
// VSTGUI: Graphical User Interface Framework for VST plugins : 
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

#include "gdiplusgraphicspath.h"

#if WINDOWS

#include "gdiplusdrawcontext.h"
#include "cfontwin32.h"
#include "win32support.h"
#include <cmath>

namespace VSTGUI {

//-----------------------------------------------------------------------------
class GdiplusGradient : public CGradient
{
public:
	GdiplusGradient (double color1Start, double color2Start, const CColor& color1, const CColor& color2)
	: CGradient (color1Start, color2Start, color1, color2) {}
};

//-----------------------------------------------------------------------------
GdiplusGraphicsPath::GdiplusGraphicsPath ()
: platformPath (new Gdiplus::GraphicsPath ())
{
}

//-----------------------------------------------------------------------------
GdiplusGraphicsPath::~GdiplusGraphicsPath ()
{
	delete platformPath;
}

//-----------------------------------------------------------------------------
CGradient* GdiplusGraphicsPath::createGradient (double color1Start, double color2Start, const CColor& color1, const CColor& color2)
{
	return new GdiplusGradient (color1Start, color2Start, color1, color2);
}

//-----------------------------------------------------------------------------
void GdiplusGraphicsPath::closeSubpath ()
{
	platformPath->CloseFigure ();
}

//-----------------------------------------------------------------------------
void GdiplusGraphicsPath::addArc (const CRect& rect, double startAngle, double endAngle, bool clockwise)
{
	// TODO: clockwise
	if (endAngle < startAngle)
		endAngle += 360.f;
	endAngle = fabs (endAngle - (Gdiplus::REAL)startAngle);
	platformPath->AddArc ((Gdiplus::REAL)rect.left, (Gdiplus::REAL)rect.top, (Gdiplus::REAL)rect.getWidth (), (Gdiplus::REAL)rect.getHeight (), (Gdiplus::REAL)startAngle, (Gdiplus::REAL)endAngle);
}

//-----------------------------------------------------------------------------
void GdiplusGraphicsPath::addCurve (const CPoint& start, const CPoint& control1, const CPoint& control2, const CPoint& end)
{
	platformPath->AddBezier ((Gdiplus::REAL)start.x, (Gdiplus::REAL)start.y, (Gdiplus::REAL)control1.x, (Gdiplus::REAL)control1.y, (Gdiplus::REAL)control2.x, (Gdiplus::REAL)control2.y, (Gdiplus::REAL)end.x, (Gdiplus::REAL)end.y);
}

//-----------------------------------------------------------------------------
void GdiplusGraphicsPath::addEllipse (const CRect& rect)
{
	platformPath->AddEllipse ((Gdiplus::REAL)rect.left, (Gdiplus::REAL)rect.top, (Gdiplus::REAL)rect.getWidth (), (Gdiplus::REAL)rect.getHeight ());
}

//-----------------------------------------------------------------------------
void GdiplusGraphicsPath::addLine (const CPoint& start, const CPoint& end)
{
	if (start != getCurrentPosition ())
		platformPath->StartFigure ();
	platformPath->AddLine ((Gdiplus::REAL)start.x, (Gdiplus::REAL)start.y, (Gdiplus::REAL)end.x, (Gdiplus::REAL)end.y);
}

//-----------------------------------------------------------------------------
void GdiplusGraphicsPath::addRect (const CRect& rect)
{
	platformPath->AddRectangle (Gdiplus::RectF ((Gdiplus::REAL)rect.left, (Gdiplus::REAL)rect.top, (Gdiplus::REAL)rect.getWidth (), (Gdiplus::REAL)rect.getHeight ()));
}

//-----------------------------------------------------------------------------
void GdiplusGraphicsPath::addPath (const CGraphicsPath& inPath, CGraphicsTransform* t)
{
	const GdiplusGraphicsPath* path = dynamic_cast<const GdiplusGraphicsPath*> (&inPath);
	if (path)
	{
		Gdiplus::GraphicsPath* gdiPath = path->getGraphicsPath ()->Clone ();
		if (t)
		{
			Gdiplus::Matrix matrix ((Gdiplus::REAL)t->m11, (Gdiplus::REAL)t->m12, (Gdiplus::REAL)t->m21, (Gdiplus::REAL)t->m22, (Gdiplus::REAL)t->dx, (Gdiplus::REAL)t->dy);
			gdiPath->Transform (&matrix);
		}
		platformPath->AddPath (gdiPath, true); // TODO: maybe the second parameter must be false
		delete gdiPath;
	}
}

//-----------------------------------------------------------------------------
CPoint GdiplusGraphicsPath::getCurrentPosition () const
{
	CPoint p (0, 0);

	Gdiplus::PointF gdiPoint;
	platformPath->GetLastPoint (&gdiPoint);
	p.x = gdiPoint.X;
	p.y = gdiPoint.Y;

	return p;
}

//-----------------------------------------------------------------------------
CRect GdiplusGraphicsPath::getBoundingBox () const
{
	CRect r;

	Gdiplus::RectF gdiRect;
	platformPath->GetBounds (&gdiRect);
	r.left = gdiRect.X;
	r.top = gdiRect.Y;
	r.setWidth (gdiRect.Width);
	r.setHeight (gdiRect.Height);
	
	return r;
}

} // namespace

#endif