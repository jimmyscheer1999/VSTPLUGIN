/*
 *  viewhierarchybrowser.cpp
 *  VST3PlugIns
 *
 *  Created by Arne Scheffler on 6/11/09.
 *  Copyright 2009 Arne Scheffler. All rights reserved.
 *
 */

#include "viewhierarchybrowser.h"
#include "viewfactory.h"
#include "ceditframe.h"
#include "../cdatabrowser.h"
#include "../vstkeycode.h"
#include <typeinfo>

BEGIN_NAMESPACE_VSTGUI

static CColor kDefaultScrollerColor = MakeCColor (255, 255, 255, 140);

//-----------------------------------------------------------------------------
class ViewHierarchyData : public IDataBrowser
{
public:
	ViewHierarchyData (ViewHierarchyBrowser* parent, UIDescription* description, IActionOperator* actionOperator);

	long dbGetNumRows (CDataBrowser* browser);
	long dbGetNumColumns (CDataBrowser* browser);
	bool dbGetColumnDescription (long index, CCoord& minWidth, CCoord& maxWidth, CDataBrowser* browser);
	CCoord dbGetCurrentColumnWidth (long index, CDataBrowser* browser);
	void dbSetCurrentColumnWidth (long index, const CCoord& width, CDataBrowser* browser);
	CCoord dbGetRowHeight (CDataBrowser* browser);
	bool dbGetLineWidthAndColor (CCoord& width, CColor& color, CDataBrowser* browser);
	void dbDrawHeader (CDrawContext* context, const CRect& size, long column, long flags, CDataBrowser* browser);
	void dbDrawCell (CDrawContext* context, const CRect& size, long row, long column, long flags, CDataBrowser* browser);
	CMouseEventResult dbOnMouseDown (const CPoint& where, const long& buttons, long row, long column, CDataBrowser* browser);
	CMouseEventResult dbOnMouseMoved (const CPoint& where, const long& buttons, long row, long column, CDataBrowser* browser);
	CMouseEventResult dbOnMouseUp (const CPoint& where, const long& buttons, long row, long column, CDataBrowser* browser);
	void dbSelectionChanged (CDataBrowser* browser);
	void dbCellTextChanged (long row, long column, const char* newText, CDataBrowser* browser) {}
	void dbCellSetupTextEdit (long row, long column, CTextEdit* textEditControl, CDataBrowser* browser) {}
	long dbOnKeyDown (const VstKeyCode& key, CDataBrowser* browser);
protected:
	void doMoveOperation (long row, bool up, CDataBrowser* browser);

	ViewHierarchyBrowser* parent;
	UIDescription* description;
	IActionOperator* actionOperator;
};

//-----------------------------------------------------------------------------
class HierarchyMoveViewOperation : public IActionOperation
{
public:
	HierarchyMoveViewOperation (CView* view, bool up)
	: view (view)
	, parent (0)
	, up (up)
	{
		view->remember ();
		parent = dynamic_cast<CViewContainer*> (view->getParentView ());
		if (parent)
			parent->remember ();
	}

	~HierarchyMoveViewOperation ()
	{
		view->forget ();
		if (parent)
			parent->forget ();
	}

	const char* getName () { return "change view hierarchy"; }

	void perform ()
	{
		if (!parent)
			return;
		CView* nextView = 0;
		long numSubViews = parent->getNbViews ();
		for (long i = 0; i < numSubViews; i++)
		{
			CView* v = parent->getView (i);
			if (up)
			{
				if (v == view)
					break;
				nextView = v;
			}
			else
			{
				if (v == view)
				{
					nextView = parent->getView (i+2);
					break;
				}
			}
		}
		view->remember ();
		parent->removeView (view);
		if (nextView)
			parent->addView (view, nextView);
		else
			parent->addView (view);
		view->invalid ();
	}
	
	void undo ()
	{
		up = !up;
		perform ();
		up = !up;
	}
protected:
	CView* view;
	CViewContainer* parent;
	bool up;
};

//-----------------------------------------------------------------------------
ViewHierarchyData::ViewHierarchyData (ViewHierarchyBrowser* parent, UIDescription* description, IActionOperator* actionOperator)
: parent (parent)
, description (description)
, actionOperator (actionOperator)
{
}

//-----------------------------------------------------------------------------
long ViewHierarchyData::dbGetNumRows (CDataBrowser* browser)
{
	if (parent->getCurrentView () == 0)
		return 0;
	return parent->getCurrentView ()->getNbViews ();
}

//-----------------------------------------------------------------------------
long ViewHierarchyData::dbGetNumColumns (CDataBrowser* browser)
{
	return 3;
}

//-----------------------------------------------------------------------------
bool ViewHierarchyData::dbGetColumnDescription (long index, CCoord& minWidth, CCoord& maxWidth, CDataBrowser* browser)
{
	return false;
}

//-----------------------------------------------------------------------------
CCoord ViewHierarchyData::dbGetCurrentColumnWidth (long index, CDataBrowser* browser)
{
	CCoord scrollbarWidth = 0;
	if (browser->getVerticalScrollbar ())
		scrollbarWidth = browser->getVerticalScrollbar ()->getWidth ();
	if (index == 0)
		return browser->getWidth () - scrollbarWidth - 50;
	return 25;
}

//-----------------------------------------------------------------------------
void ViewHierarchyData::dbSetCurrentColumnWidth (long index, const CCoord& width, CDataBrowser* browser)
{
}

//-----------------------------------------------------------------------------
CCoord ViewHierarchyData::dbGetRowHeight (CDataBrowser* browser)
{
	return 20;
}

//-----------------------------------------------------------------------------
bool ViewHierarchyData::dbGetLineWidthAndColor (CCoord& width, CColor& color, CDataBrowser* browser)
{
	width = 1;
	color = MakeCColor (255, 255, 255, 20);
	return true;
}

//-----------------------------------------------------------------------------
void ViewHierarchyData::dbDrawHeader (CDrawContext* context, const CRect& size, long column, long flags, CDataBrowser* browser)
{
}

//-----------------------------------------------------------------------------
void ViewHierarchyData::dbDrawCell (CDrawContext* context, const CRect& size, long row, long column, long flags, CDataBrowser* browser)
{
	if (parent->getCurrentView () == 0)
		return;
	if (browser->getSelectedRow () == row)
	{
		context->setFillColor (MakeCColor (255, 255, 255, 10));
		context->drawRect (size, kDrawFilled);
	}
	if (column == 0)
	{
		CView* view = parent->getCurrentView ()->getView (row);
		if (view)
		{
			const char* viewname = 0;
			ViewFactory* factory = dynamic_cast<ViewFactory*> (description->getViewFactory ());
			if (factory)
				viewname = factory->getViewName (view);
			if (viewname == 0)
				viewname = typeid(*view).name ();
			if (dynamic_cast<CViewContainer*> (view))
			{
				context->setFontColor (kWhiteCColor);
			}
			else
				context->setFontColor (kGreyCColor);
			context->setFont (kNormalFont);
			context->drawStringUTF8 (viewname, size);
		}
	}
	else
	{
		CRect r (size);
		if (r.getWidth () > r.getHeight ())
		{
			CCoord diff = r.getWidth () - r.getHeight ();
			r.setWidth (r.getWidth () - diff);
			r.offset (diff/2, 0);
		}
		else if (r.getHeight () > r.getWidth ())
		{
			CCoord diff = r.getHeight () - r.getWidth ();
			r.setHeight (r.getHeight () - diff);
			r.offset (0, diff/2);
		}
		r.inset (6, 6);
		CPoint polygon[4];
		if (column == 1)
		{
			polygon[0] = CPoint (r.left, r.bottom);
			polygon[1] = CPoint (r.left + r.getWidth () / 2, r.top);
			polygon[2] = CPoint (r.right, r.bottom);
		}
		else
		{
			polygon[0] = CPoint (r.left, r.top);
			polygon[1] = CPoint (r.left + r.getWidth () / 2, r.bottom);
			polygon[2] = CPoint (r.right, r.top);
		}
		polygon[3] = polygon[0];
		context->setDrawMode (kAntialias);
		if ((row == 0 && column == 1) || (row == dbGetNumRows (browser) -1 && column == 2))
			context->setFillColor (MakeCColor (255, 255, 255, 50));
		else
			context->setFillColor (kWhiteCColor);
		context->drawPolygon (polygon, 4, kDrawFilled);
	}
}

//-----------------------------------------------------------------------------
void ViewHierarchyData::doMoveOperation (long row, bool up, CDataBrowser* browser)
{
	if (parent->getCurrentView ())
	{
		if (!(row == 0 && up) && !(row == dbGetNumRows (browser)-1 && !up))
		{
			actionOperator->performAction (new HierarchyMoveViewOperation (parent->getCurrentView ()->getView (row), up));
			browser->setSelectedRow (row + (up ? -1 : 1), true);
		}
	}
}

//-----------------------------------------------------------------------------
CMouseEventResult ViewHierarchyData::dbOnMouseDown (const CPoint& where, const long& buttons, long row, long column, CDataBrowser* browser)
{
	if (parent->getCurrentView ())
	{
		if (column == 0)
		{
			if (buttons & kDoubleClick)
			{
				CViewContainer* view = dynamic_cast<CViewContainer*> (parent->getCurrentView ()->getView (row));
				if (view)
					parent->setCurrentView (view);
			}
		}
		else if (actionOperator)
		{
			doMoveOperation (row, column == 1, browser);
		}
	}
	return kMouseEventHandled;
}

//-----------------------------------------------------------------------------
CMouseEventResult ViewHierarchyData::dbOnMouseMoved (const CPoint& where, const long& buttons, long row, long column, CDataBrowser* browser)
{
	return kMouseEventHandled;
}

//-----------------------------------------------------------------------------
CMouseEventResult ViewHierarchyData::dbOnMouseUp (const CPoint& where, const long& buttons, long row, long column, CDataBrowser* browser)
{
	return kMouseEventHandled;
}

//-----------------------------------------------------------------------------
long ViewHierarchyData::dbOnKeyDown (const VstKeyCode& key, CDataBrowser* browser)
{
	if (parent->getCurrentView ())
	{
		if (key.virt == VKEY_RETURN && key.modifier == 0)
		{
			CViewContainer* view = dynamic_cast<CViewContainer*> (parent->getCurrentView ()->getView (browser->getSelectedRow ()));
			if (view)
			{
				parent->setCurrentView (view);
				return 1;
			}
		}
		else if (key.virt == VKEY_BACK && key.modifier == 0)
		{
			CViewContainer* view = dynamic_cast<CViewContainer*> (parent->getCurrentView ()->getParentView ());
			if (view)
			{
				parent->setCurrentView (view);
				return 1;
			}
		}
		else if (key.virt == VKEY_UP && key.modifier == MODIFIER_CONTROL)
		{
			doMoveOperation (browser->getSelectedRow (), true, browser);
			return 1;
		}
		else if (key.virt == VKEY_DOWN && key.modifier == MODIFIER_CONTROL)
		{
			doMoveOperation (browser->getSelectedRow (), false, browser);
			return 1;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
void ViewHierarchyData::dbSelectionChanged (CDataBrowser* browser)
{
	if (actionOperator && parent->getCurrentView ())
	{
		actionOperator->makeSelection (parent->getCurrentView ()->getView (browser->getSelectedRow ()));
	}
}

//-----------------------------------------------------------------------------
class ViewHierarchyPathView : public CView
{
public:
	ViewHierarchyPathView (const CRect& size, ViewHierarchyBrowser* browser, ViewFactory* viewFactory);
	~ViewHierarchyPathView ();

	CMouseEventResult onMouseDown (CPoint &where, const long& buttons);
	void draw (CDrawContext* context);
	void setViewSize (CRect &rect, bool invalid) { CView::setViewSize (rect, invalid); needCompute = true; }

	void setHierarchyDirty () { needCompute = true; invalid (); }
protected:
	class PathElement
	{
	public:
		PathElement (const PathElement& pe)
		: view (pe.view), name (pe.name), nameWidth (pe.nameWidth), drawWidth (pe.drawWidth) {}
		
		PathElement (ViewFactory* factory, CViewContainer* view, CDrawContext* context)
		: view (view)
		{
			const char* viewname = 0;
			if (factory)
				viewname = factory->getViewName (view);
			if (viewname == 0)
				viewname = typeid(*view).name ();
			name = viewname;
			drawWidth = nameWidth = context->getStringWidthUTF8 (name.c_str ());
		}
		bool operator==(const PathElement& pe) const { return pe.view == view; }
		void setDrawWidth (CCoord w) { drawWidth = w; }

		const char* getName () const { return name.c_str (); }
		CViewContainer* getView () const { return view; }
		CCoord getNameWidth () const { return nameWidth; }
		CCoord getDrawWidth () const { return drawWidth; }
	protected:
		CViewContainer* view;
		std::string name;
		CCoord nameWidth;
		CCoord drawWidth;
	};

	void compute (CDrawContext* context);
	void drawPathElement (const CRect& size, const PathElement& element, CDrawContext* context, bool isLast);

	ViewHierarchyBrowser* browser;
	ViewFactory* viewFactory;
	std::list<PathElement> elements;
	CCoord margin;
	bool needCompute;

	typedef std::list<PathElement>::iterator elements_iterator;
	typedef std::list<PathElement>::const_iterator const_elements_iterator;
};

//-----------------------------------------------------------------------------
ViewHierarchyPathView::ViewHierarchyPathView (const CRect& size, ViewHierarchyBrowser* browser, ViewFactory* viewFactory)
: CView (size)
, browser (browser)
, viewFactory (viewFactory)
, margin (10)
, needCompute (true)
{
}

//-----------------------------------------------------------------------------
ViewHierarchyPathView::~ViewHierarchyPathView ()
{
}

//-----------------------------------------------------------------------------
void ViewHierarchyPathView::compute (CDrawContext* context)
{
	elements.clear ();
	elements.push_back (PathElement (viewFactory, browser->getCurrentView (), context));
	CViewContainer* container = browser->getCurrentView ();
	while (container != browser->getBaseView ())
	{
		container = dynamic_cast<CViewContainer*> (container->getParentView ());
		if (container)
			elements.push_back (PathElement (viewFactory, container, context));
		else
			return; // should never happen
	}
	CCoord totalWidth = 0;
	const_elements_iterator it = elements.begin ();
	while (it != elements.end ())
	{
		totalWidth += (*it).getNameWidth () + margin;
		it++;
	}
	if (totalWidth > getWidth ())
	{
		CCoord lessen = (totalWidth - getWidth ()) / (elements.size () - 1);
		elements_iterator it = elements.begin ();
		it++;
		while (it != elements.end ())
		{
			(*it).setDrawWidth ((*it).getDrawWidth () - lessen);
			it++;
		}
	}
	elements.reverse ();
}

//-----------------------------------------------------------------------------
void ViewHierarchyPathView::drawPathElement (const CRect& size, const PathElement& element, CDrawContext* context, bool isLast)
{
	CRect r (size);
	CPoint polygon[6];
	if (isLast)
	{
		CCoord right = r.right - margin;
		right += 4;
		polygon[0] = CPoint (r.left, r.top);
		polygon[1] = CPoint (right, r.top);
		polygon[2] = CPoint (right, r.bottom);
		polygon[3] = CPoint (r.left, r.bottom);
		polygon[4] = polygon[0];
	}
	else
	{
		polygon[0] = CPoint (r.left, r.top);
		polygon[1] = CPoint (r.right - margin, r.top);
		polygon[2] = CPoint (r.right, r.top + r.getHeight () / 2);
		polygon[3] = CPoint (r.right - margin, r.bottom);
		polygon[4] = CPoint (r.left, r.bottom);
		polygon[5] = polygon[0];
	}
	context->drawPolygon (polygon, isLast ? 5 : 6, kDrawFilledAndStroked);
	r.right -= margin;
	r.offset (2, 0);
	context->drawStringUTF8 (element.getName (), r);
}

//-----------------------------------------------------------------------------
void ViewHierarchyPathView::draw (CDrawContext* context)
{
	setDirty (false);

	if (!browser->getCurrentView ())
		return;

	context->setFrameColor (MakeCColor (255, 255, 255, 40));
	context->setFillColor (MakeCColor (255, 255, 255, 20));
	context->setFontColor (kWhiteCColor);
	context->setFont (kNormalFont);
	context->setDrawMode (kAntialias);
	context->setLineWidth (1);

	if (needCompute)
		compute (context);

	CRect r (size);
	const_elements_iterator it = elements.begin ();
	while (it != elements.end ())
	{
		r.setWidth ((*it).getDrawWidth () + margin);
		context->setClipRect (r);
		drawPathElement (r, (*it), context, (*it) == elements.back ());
		r.offset (r.getWidth (), 0);
		it++;
	}
}

//-----------------------------------------------------------------------------
CMouseEventResult ViewHierarchyPathView::onMouseDown (CPoint &where, const long& buttons)
{
	CRect r (size);
	const_elements_iterator it = elements.begin ();
	while (it != elements.end ())
	{
		r.setWidth ((*it).getDrawWidth () + margin);
		if (r.pointInside (where))
		{
			browser->setCurrentView ((*it).getView ());
			break;
		}
		r.offset (r.getWidth (), 0);
		it++;
	}
	
	return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
ViewHierarchyBrowser::ViewHierarchyBrowser (const CRect& rect, CViewContainer* baseView, UIDescription* description, IActionOperator* actionOperator)
: CViewContainer (rect, 0)
, baseView (baseView)
, currentView (baseView)
, browser (0)
, data (0)
, pathView (0)
{
	setTransparency (true);
	data = new ViewHierarchyData (this, description, actionOperator);
	CRect r2 (rect);
	r2.offset (-r2.left, -r2.top);
	r2.setHeight (25);
	r2.inset (1, 4);
	pathView = new ViewHierarchyPathView (r2, this, description ? dynamic_cast<ViewFactory*>(description->getViewFactory ()) : 0);
	pathView->setAutosizeFlags (kAutosizeLeft|kAutosizeRight|kAutosizeTop);
	addView (pathView);

	CRect r (rect);
	r.offset (-r.left, -r.top);
	r.top += 25;
	browser = new CDataBrowser (r, 0, data, CScrollView::kVerticalScrollbar|CScrollView::kDontDrawFrame|CDataBrowser::kDrawRowLines|CDataBrowser::kDrawColumnLines, 10);
	browser->setTransparency (true);
	browser->setAutosizeFlags (kAutosizeAll);
	CScrollbar* bar = browser->getVerticalScrollbar ();
	bar->setScrollerColor (kDefaultScrollerColor);
	bar->setBackgroundColor (kTransparentCColor);
	bar->setFrameColor (kTransparentCColor);
	addView (browser);
	setCurrentView (baseView);
}

//-----------------------------------------------------------------------------
ViewHierarchyBrowser::~ViewHierarchyBrowser ()
{
}

//-----------------------------------------------------------------------------
void ViewHierarchyBrowser::setCurrentView (CViewContainer* newView)
{
	if ((newView && newView->isChild (baseView, true)) || newView == currentView)
		return;
	currentView = newView;
	browser->recalculateLayout (false);
	browser->setSelectedRow (0, true);
	pathView->setHierarchyDirty ();
}

//-----------------------------------------------------------------------------
void ViewHierarchyBrowser::changeBaseView (CViewContainer* newBaseView)
{
	baseView = currentView = newBaseView;
	browser->recalculateLayout (false);
	browser->setSelectedRow (0, true);
	pathView->setHierarchyDirty ();
}

//-----------------------------------------------------------------------------
void ViewHierarchyBrowser::notifyHierarchyChange (CView* view, bool wasRemoved)
{
	CViewContainer* parent = dynamic_cast<CViewContainer*> (view->getParentView ());
	if (parent == currentView)
		browser->recalculateLayout (true);
	else if (view == currentView)
	{
		if (wasRemoved)
			setCurrentView (dynamic_cast<CViewContainer*> (view->getParentView ()));
		else
			browser->recalculateLayout (true);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const char* ViewHierarchyBrowserWindow::kMsgWindowClosed = "ViewHierarchyBrowserWindow closed";

//-----------------------------------------------------------------------------
ViewHierarchyBrowserWindow::ViewHierarchyBrowserWindow (CViewContainer* baseView, CBaseObject* owner, UIDescription* description)
: owner (owner)
, platformWindow (0)
, browser (0)
{
	CRect size (0, 0, 300, 500);
	platformWindow = PlatformWindow::create (size, "VSTGUI Hierarchy Browser", PlatformWindow::kPanelType, PlatformWindow::kClosable|PlatformWindow::kResizable, this);
	if (platformWindow)
	{
		frame = new CFrame (size, platformWindow->getPlatformHandle (), this);
		#if MAC
		frame->setBackgroundColor (kTransparentCColor);
		#elif WINDOWS
		frame->setBackgroundColor (kBlackCColor);
		#endif

		const CCoord kMargin = 12;
		size.left += kMargin;
		size.right -= kMargin;
		size.bottom -= kMargin;
		browser = new ViewHierarchyBrowser (size, baseView, description, dynamic_cast<IActionOperator*> (owner));
		browser->setAutosizeFlags (kAutosizeAll);
		frame->addView (browser);

		platformWindow->center ();
		if (PlatformDefaults::getRect ("net.sourceforge.vstgui.uidescription", "ViewHierarchyBrowserWindow size", size))
			platformWindow->setSize (size);
		platformWindow->show ();
	}
}

//-----------------------------------------------------------------------------
ViewHierarchyBrowserWindow::~ViewHierarchyBrowserWindow ()
{
	owner = 0;
	if (platformWindow)
		windowClosed (platformWindow);
}

//-----------------------------------------------------------------------------
void ViewHierarchyBrowserWindow::changeBaseView (CViewContainer* newBaseView)
{
	if (browser)
		browser->changeBaseView (newBaseView);
}

//-----------------------------------------------------------------------------
void ViewHierarchyBrowserWindow::notifyHierarchyChange (CView* view, bool wasRemoved)
{
	if (browser)
		browser->notifyHierarchyChange (view, wasRemoved);
}

//-----------------------------------------------------------------------------
void ViewHierarchyBrowserWindow::windowSizeChanged (const CRect& newSize, PlatformWindow* platformWindow)
{
	frame->setSize (newSize.getWidth (), newSize.getHeight ());
}

//-----------------------------------------------------------------------------
void ViewHierarchyBrowserWindow::windowClosed (PlatformWindow* _platformWindow)
{
	if (_platformWindow == platformWindow)
	{
		PlatformDefaults::setRect ("net.sourceforge.vstgui.uidescription", "ViewHierarchyBrowserWindow size", platformWindow->getSize ());
		platformWindow->forget ();
		platformWindow = 0;
	}
	if (owner)
		owner->notify (this, kMsgWindowClosed);
}

//-----------------------------------------------------------------------------
void ViewHierarchyBrowserWindow::checkWindowSizeConstraints (CPoint& size, PlatformWindow* platformWindow)
{
	if (size.x < 300)
		size.x = 300;
	if (size.y < 200)
		size.y = 200;
}

END_NAMESPACE_VSTGUI
