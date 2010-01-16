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

#include "vst3editor.h"
#include "../lib/vstkeycode.h"
#include "ceditframe.h"
#include "editingcolordefs.h"
#include "base/source/updatehandler.h"
#include "base/source/fstring.h"
#include "base/source/timer.h"
#include "pluginterfaces/base/keycodes.h"
#include "dialog.h"
#include "cviewinspector.h"
#include "vst3editortemplates.h"
#include "viewfactory.h"
#include <list>
#include <sstream>

namespace Steinberg {

//-----------------------------------------------------------------------------
class IdleUpdateHandler : public FObject, public ITimerCallback
{
public:
	OBJ_METHODS (IdleUpdateHandler, FObject)
	SINGLETON (IdleUpdateHandler)
protected:
	IdleUpdateHandler () { UpdateHandler::instance (); timer = Timer::create (this, 1000/30); } // 30 Hz timer
	~IdleUpdateHandler () { timer->release (); }
	void onTimer (Timer* timer)
	{
		UpdateHandler::instance ()->triggerDeferedUpdates ();
	}

	Steinberg::Timer* timer;
};

} // namespace Steinberg

namespace VSTGUI {

//-----------------------------------------------------------------------------
class ParameterChangeListener : public Steinberg::FObject
{
public:
	ParameterChangeListener (Steinberg::Vst::EditController* editController, Steinberg::Vst::Parameter* parameter, CControl* control)
	: editController (editController)
	, parameter (parameter)
	{
		if (parameter)
		{
			parameter->addRef ();
			parameter->addDependent (this);
		}
		addControl (control);
	}

	~ParameterChangeListener ()
	{
		if (parameter)
		{
			parameter->removeDependent (this);
			parameter->release ();
		}
		std::list<CControl*>::iterator it = controls.begin ();
		while (it != controls.end ())
		{
			(*it)->forget ();
			it++;
		}
	}

	void addControl (CControl* control)
	{
		control->remember ();
		controls.push_back (control);
		Steinberg::Vst::ParamValue value = 0.;
		if (parameter)
		{
			value = editController->getParamNormalized (getParameterID ());
		}
		else
		{
			CControl* control = controls.front ();
			if (control)
				value = (control->getValue () - control->getMin ()) / (control->getMax () - control->getMin ());
		}
		CParamDisplay* display = dynamic_cast<CParamDisplay*> (control);
		if (display)
			display->setStringConvert (stringConvert, this);

		COptionMenu* optMenu = dynamic_cast<COptionMenu*> (control);
		if (optMenu && parameter && parameter->getInfo ().stepCount > 0)
		{
			for (Steinberg::int32 i = 0; i <= parameter->getInfo ().stepCount; i++)
			{
				Steinberg::Vst::String128 utf16Str;
				editController->getParamStringByValue (getParameterID (), (Steinberg::Vst::ParamValue)i / (Steinberg::Vst::ParamValue)parameter->getInfo ().stepCount, utf16Str);
				Steinberg::String utf8Str (utf16Str);
				utf8Str.toMultiByte (Steinberg::kCP_Utf8);
				optMenu->addEntry (utf8Str);
			}
		}

		updateControlValue (value);
	}
	
	void removeControl (CControl* control)
	{
		std::list<CControl*>::iterator it = controls.begin ();
		while (it != controls.end ())
		{
			if ((*it) == control)
			{
				controls.remove (control);
				control->forget ();
				return;
			}
			it++;
		}
	}
	
	void PLUGIN_API update (FUnknown* changedUnknown, Steinberg::int32 message)
	{
		if (message == IDependent::kChanged && parameter)
		{
			updateControlValue (editController->getParamNormalized (getParameterID ()));
		}
	}

	Steinberg::Vst::ParamID getParameterID () 
	{
		if (parameter)
			return parameter->getInfo ().id;
		CControl* control = controls.front ();
		if (control)
			return control->getTag ();
		return 0xFFFFFFFF;
	}
	
	void beginEdit ()
	{
		if (parameter)
			editController->beginEdit (getParameterID ());
	}
	
	void endEdit ()
	{
		if (parameter)
			editController->endEdit (getParameterID ());
	}
	
	void performEdit (Steinberg::Vst::ParamValue value)
	{
		if (parameter)
		{
			editController->setParamNormalized (getParameterID (), value);
			editController->performEdit (getParameterID (), value);
		}
		else
		{
			updateControlValue (value);
		}
	}
	Steinberg::Vst::Parameter* getParameter () const { return parameter; }

protected:
	void convertValueToString (float value, char* string)
	{
		if (parameter)
		{
			Steinberg::Vst::String128 utf16Str;
			editController->getParamStringByValue (getParameterID (), value, utf16Str);
			Steinberg::String utf8Str (utf16Str);
			utf8Str.toMultiByte (Steinberg::kCP_Utf8);
			utf8Str.copyTo8 (string, 0, 256);
		}
	}

	static void stringConvert (float value, char* string, void* userDta)
	{
		ParameterChangeListener* This = (ParameterChangeListener*)userDta;
		This->convertValueToString (value, string);
	}
	
	void updateControlValue (Steinberg::Vst::ParamValue value)
	{
		std::list<CControl*>::iterator it = controls.begin ();
		while (it != controls.end ())
		{
			COptionMenu* optMenu = dynamic_cast<COptionMenu*> (*it);
			if (optMenu)
			{
				if (parameter)
					optMenu->setValue (editController->normalizedParamToPlain (getParameterID (), value), true);
			}
			else
			{
				double min = (*it)->getMin ();
				double max = (*it)->getMax ();
				double value2 = min + value * (max - min);
				(*it)->setValue (value2, true);
			}
			(*it)->invalid ();
			it++;
		}
	}
	Steinberg::Vst::EditController* editController;
	Steinberg::Vst::Parameter* parameter;
	std::list<CControl*> controls;
};

//-----------------------------------------------------------------------------
static bool parseSize (const std::string& str, CPoint& point)
{
	size_t sep = str.find (',', 0);
	if (sep != std::string::npos)
	{
		point.x = strtol (str.c_str (), 0, 10);
		point.y = strtol (str.c_str () + sep+1, 0, 10);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
VST3Editor::VST3Editor (void* controller, const char* _viewName, const char* _xmlFile)
: VSTGUIEditor (controller)
, doCreateView (false)
, tooltipSupport (0)
, tooltipsEnabled (true)
{
	description = new UIDescription (_xmlFile);
	viewName = _viewName;
	xmlFile = _xmlFile;
	init ();
}

//-----------------------------------------------------------------------------
VST3Editor::VST3Editor (UIDescription* desc, void* controller, const char* _viewName, const char* _xmlFile)
: VSTGUIEditor (controller)
, doCreateView (false)
, tooltipSupport (0)
, tooltipsEnabled (true)
{
	description = desc;
	description->remember ();
	viewName = _viewName;
	if (_xmlFile)
		xmlFile = _xmlFile;
	init ();
}

//-----------------------------------------------------------------------------
VST3Editor::~VST3Editor ()
{
	description->forget ();
}

//-----------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API VST3Editor::queryInterface (const Steinberg::TUID iid, void** obj)
{
	QUERY_INTERFACE(iid, obj, Steinberg::Vst::IParameterFinder::iid, Steinberg::Vst::IParameterFinder)
	return VSTGUIEditor::queryInterface (iid, obj);
}

//-----------------------------------------------------------------------------
void VST3Editor::init ()
{
	setIdleRate (300);
	Steinberg::IdleUpdateHandler::instance ();
	if (description->parse ())
	{
		// get sizes
		const UIAttributes* attr = description->getViewAttributes (viewName.c_str ());
		if (attr)
		{
			const std::string* sizeStr = attr->getAttributeValue ("size");
			const std::string* minSizeStr = attr->getAttributeValue ("minSize");
			const std::string* maxSizeStr = attr->getAttributeValue ("maxSize");
			if (sizeStr)
			{
				CPoint p;
				if (parseSize (*sizeStr, p))
				{
					rect.right = (Steinberg::int32)p.x;
					rect.bottom = (Steinberg::int32)p.y;
					minSize = p;
					maxSize = p;
				}
			}
			if (minSizeStr)
				parseSize (*minSizeStr, minSize);
			if (maxSizeStr)
				parseSize (*maxSizeStr, maxSize);
		}
		#if DEBUG
		else
		{
			UIAttributes* attr = new UIAttributes ();
			attr->setAttribute ("class", "CViewContainer");
			attr->setAttribute ("size", "300, 300");
			description->addNewTemplate (viewName.c_str (), attr);
			rect.right = 300;
			rect.bottom = 300;
			minSize (rect.right, rect.bottom);
			maxSize (rect.right, rect.bottom);
			description->changeColor ("black", kBlackCColor);
			description->changeColor ("white", kWhiteCColor);
		}
		#endif
	}
	#if DEBUG
	else
	{
		UIAttributes* attr = new UIAttributes ();
		attr->setAttribute ("class", "CViewContainer");
		attr->setAttribute ("size", "300, 300");
		description->addNewTemplate (viewName.c_str (), attr);
		rect.right = 300;
		rect.bottom = 300;
		minSize (rect.right, rect.bottom);
		maxSize (rect.right, rect.bottom);
		description->changeColor ("black", kBlackCColor);
		description->changeColor ("white", kWhiteCColor);
	}
	#endif
}

//-----------------------------------------------------------------------------
bool VST3Editor::exchangeView (const char* newViewName)
{
	const UIAttributes* attr = description->getViewAttributes (newViewName);
	if (attr)
	{
		viewName = newViewName;
		doCreateView = true;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
void VST3Editor::enableTooltips (bool state)
{
	tooltipsEnabled = state;
	if (state)
	{
		if (frame && !tooltipSupport)
			tooltipSupport = new CTooltipSupport (frame);
	}
	else
	{
		if (tooltipSupport)
		{
			tooltipSupport->forget ();
			tooltipSupport = 0;
		}
	}
}

//-----------------------------------------------------------------------------
ParameterChangeListener* VST3Editor::getParameterChangeListener (long tag)
{
	if (tag != -1)
	{
		std::map<long, ParameterChangeListener*>::iterator it = paramChangeListeners.find (tag);
		if (it != paramChangeListeners.end ())
		{
			return it->second;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
void VST3Editor::valueChanged (CControl* pControl)
{
	ParameterChangeListener* pcl = getParameterChangeListener (pControl->getTag ());
	if (pcl)
	{
		Steinberg::Vst::ParamValue value = (pControl->getValue () - pControl->getMin ()) / (pControl->getMax () - pControl->getMin ());
		CTextEdit* textEdit = dynamic_cast<CTextEdit*> (pControl);
		if (textEdit && pcl->getParameter ())
		{
			Steinberg::String str (textEdit->getText ());
			str.toWideString (Steinberg::kCP_Utf8);
			if (getController ()->getParamValueByString (pcl->getParameterID (), (Steinberg::Vst::TChar*)str.text16 (), value) != Steinberg::kResultTrue)
			{
				pcl->changed ();
				return;
			}
		}
		pcl->performEdit (value);
	}
}

//-----------------------------------------------------------------------------
void VST3Editor::beginEdit (long index)
{
	// we don't assume that every control tag is a parameter tag handled by this editor
	// as sub classes could build custom CControlListeners for controls
}

//-----------------------------------------------------------------------------
void VST3Editor::endEdit (long index)
{
	// see above
}

//-----------------------------------------------------------------------------
void VST3Editor::controlBeginEdit (CControl* pControl)
{
	ParameterChangeListener* pcl = getParameterChangeListener (pControl->getTag ());
	if (pcl)
	{
		pcl->beginEdit ();
	}
}

//-----------------------------------------------------------------------------
void VST3Editor::controlEndEdit (CControl* pControl)
{
	ParameterChangeListener* pcl = getParameterChangeListener (pControl->getTag ());
	if (pcl)
	{
		pcl->endEdit ();
	}
}

//-----------------------------------------------------------------------------
void VST3Editor::onViewAdded (CFrame* frame, CView* view)
{
}

//-----------------------------------------------------------------------------
void VST3Editor::onViewRemoved (CFrame* frame, CView* view)
{
	CControl* control = dynamic_cast<CControl*> (view);
	if (control && control->getTag () != -1)
	{
		ParameterChangeListener* pcl = getParameterChangeListener (control->getTag ());
		if (pcl)
		{
			pcl->removeControl (control);
		}
	}
}

//-----------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API VST3Editor::findParameter (Steinberg::int32 xPos, Steinberg::int32 yPos, Steinberg::Vst::ParamID& resultTag)
{
	CView* view = frame->getViewAt (CPoint (xPos, yPos), true);
	if (view)
	{
		CControl* control = dynamic_cast<CControl*> (view);
		if (control && control->getTag () != -1)
		{
			ParameterChangeListener* pcl = getParameterChangeListener (control->getTag ());
			if (pcl)
			{
				resultTag = pcl->getParameterID ();
				return Steinberg::kResultTrue;
			}
		}
		VST3EditorDelegate* delegate = dynamic_cast<VST3EditorDelegate*> (getController ());
		if (delegate)
		{
			return (delegate->findParameter (CPoint (xPos, yPos), resultTag, this) ? Steinberg::kResultTrue : Steinberg::kResultFalse);
		}
	}
	return Steinberg::kResultFalse;
}

//-----------------------------------------------------------------------------
CView* VST3Editor::createView (const UIAttributes& attributes, IUIDescription* description)
{
	const std::string* customViewName = attributes.getAttributeValue ("custom-view-name");
	if (customViewName)
	{
		VST3EditorDelegate* delegate = dynamic_cast<VST3EditorDelegate*> (getController ());
		if (delegate)
		{
			CView* view = delegate->createCustomView (customViewName->c_str (), attributes, description, this);
			if (view)
			{
				UIDescription* uiDesc = dynamic_cast<UIDescription*> (description);
				ViewFactory* viewFactory = uiDesc ? dynamic_cast<ViewFactory*> (uiDesc->getViewFactory ()) : 0;
				if (viewFactory)
				{
					const std::string* viewClass = attributes.getAttributeValue ("class");
					if (viewClass)
						viewFactory->applyCustomViewAttributeValues (view, viewClass->c_str (), attributes, description);
				}
			}
			return view;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
CView* VST3Editor::verifyView (CView* view, const UIAttributes& attributes, IUIDescription* description)
{
	CControl* control = dynamic_cast<CControl*> (view);
	if (control && control->getTag () != -1 && control->getListener () == this)
	{
		ParameterChangeListener* pcl = getParameterChangeListener (control->getTag ());
		if (pcl)
		{
			pcl->addControl (control);
		}
		else
		{
			Steinberg::Vst::EditController* editController = getController ();
			if (editController)
			{
				Steinberg::Vst::Parameter* parameter = editController->getParameterObject (control->getTag ());
				paramChangeListeners.insert (std::make_pair (control->getTag (), new ParameterChangeListener (editController, parameter, control)));
			}
		}
	}
	return view;
}

//-----------------------------------------------------------------------------
void VST3Editor::recreateView ()
{
	doCreateView = false;
	frame->remember ();
	close ();

	CView* view = description->createView (viewName.c_str (), this);
	if (view)
	{
		if (plugFrame)
		{
			rect.right = rect.left + (Steinberg::int32)view->getWidth ();
			rect.bottom = rect.top + (Steinberg::int32)view->getHeight ();
			plugFrame->resizeView (this, &rect);
		}
		else
		{
			frame->setSize (view->getWidth (), view->getHeight ());
		}
		frame->addView (view);
		if (tooltipsEnabled)
			tooltipSupport = new CTooltipSupport (frame);
	}
	init ();
	frame->invalid ();
}

#define kFrameEnableFocusDrawingAttr	"frame-enable-focus-drawing"
#define kFrameFocusColorAttr			"frame-focus-color"
#define kFrameFocusWidthAttr			"frame-focus-width"

//-----------------------------------------------------------------------------
bool PLUGIN_API VST3Editor::open (void* parent)
{
	CView* view = description->createView (viewName.c_str (), this);
	if (view)
	{
	#if VSTGUI_LIVE_EDITING
		frame = new CEditFrame (view->getViewSize (), parent, this, CEditFrame::kNoEditMode, 0, description, viewName.c_str ());
	#else
		frame = new CFrame (view->getViewSize (), parent, this);
	#endif
		frame->setViewAddedRemovedObserver (this);
		frame->setTransparency (true);
		frame->addView (view);
		CRect size (rect.left, rect.top, rect.right, rect.bottom);
		frame->setSize (size.getWidth (), size.getHeight ());
		if (tooltipsEnabled)
			tooltipSupport = new CTooltipSupport (frame);
			
		// focus drawing support
		const UIAttributes* attributes = description->getCustomAttributes ("VST3Editor");
		if (attributes)
		{
			const std::string* attr = attributes->getAttributeValue (kFrameEnableFocusDrawingAttr);
			if (attr && *attr == "true")
			{
				frame->setFocusDrawingEnabled (true);
				attr = attributes->getAttributeValue (kFrameFocusColorAttr);
				if (attr)
				{
					CColor focusColor;
					if (description->getColor (attr->c_str (), focusColor))
						frame->setFocusColor (focusColor);
				}
				attr = attributes->getAttributeValue (kFrameFocusWidthAttr);
				if (attr)
				{
					float focusWidth = strtod (attr->c_str (), 0);
					frame->setFocusWidth (focusWidth);
				}
			}
		}
		VST3EditorDelegate* delegate = dynamic_cast<VST3EditorDelegate*> (getController ());
		if (delegate)
			delegate->didOpen (this);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
void PLUGIN_API VST3Editor::close ()
{
	std::map<long, ParameterChangeListener*>::iterator it = paramChangeListeners.begin ();
	while (it != paramChangeListeners.end ())
	{
		it->second->release ();
		it++;
	}
	paramChangeListeners.clear ();
	if (tooltipSupport)
	{
		tooltipSupport->forget ();
		tooltipSupport = 0;
	}
	if (frame)
	{
		VST3EditorDelegate* delegate = dynamic_cast<VST3EditorDelegate*> (getController ());
		if (delegate)
			delegate->willClose (this);
		frame->removeAll (true);
		long refCount = frame->getNbReference ();
		frame->forget ();
		if (refCount == 1)
			frame = 0;
	}
}

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API VST3Editor::onSize (Steinberg::ViewRect* newSize)
{
	return VSTGUIEditor::onSize (newSize);
}

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API VST3Editor::canResize ()
{
	return (minSize == maxSize) ? Steinberg::kResultFalse : Steinberg::kResultTrue;
}

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API VST3Editor::checkSizeConstraint (Steinberg::ViewRect* rect)
{
	CCoord width = rect->right - rect->left;
	CCoord height = rect->bottom - rect->top;
	if (width < minSize.x)
		width = minSize.x;
	else if (width > maxSize.x)
		width = maxSize.x;
	if (height < minSize.y)
		height = minSize.y;
	else if (height > maxSize.y)
		height = maxSize.y;
	if (width != rect->right - rect->left || height != rect->bottom - rect->top)
	{
		rect->right = (Steinberg::int32)width + rect->left;
		rect->bottom = (Steinberg::int32)height + rect->top;
	}
	return Steinberg::kResultTrue;
}

//------------------------------------------------------------------------
CMessageResult VST3Editor::notify (CBaseObject* sender, const char* message)
{
	if (message == CVSTGUITimer::kMsgTimer)
	{
		if (doCreateView)
			recreateView ();
 	}
	#if VSTGUI_LIVE_EDITING
	else if (message == CEditFrame::kMsgShowOptionsMenu)
	{
		COptionMenu* menu = dynamic_cast<COptionMenu*> (sender);
		if (menu)
		{
			menu->addSeparator ();
			menu->addEntry (new CMenuItem ("Template Settings..."));
			std::list<const std::string*> templateNames;
			description->collectTemplateViewNames (templateNames);
			if (templateNames.size () > 0)
			{
				COptionMenu* submenu = new COptionMenu ();
				long menuTag = 1000;
				std::list<const std::string*>::const_iterator it = templateNames.begin ();
				while (it != templateNames.end ())
				{
					submenu->addEntry (new CMenuItem ((*it)->c_str (), menuTag++));
					it++;
				}
				menu->addEntry (submenu, "Change Template");
				submenu->forget ();

				ViewFactory* viewFactory = dynamic_cast<ViewFactory*> (description->getViewFactory ());
				if (viewFactory)
				{
					std::list<const std::string*> viewNames;
					viewFactory->collectRegisteredViewNames (viewNames, "CViewContainer");
					if (viewNames.size () > 0)
					{
						submenu = new COptionMenu ();
						CMenuItem* item = submenu->addEntry ("Root View Type");
						item->setIsTitle (true);
						menuTag = 10000;
						std::list<const std::string*>::const_iterator it = viewNames.begin ();
						while (it != viewNames.end ())
						{
							submenu->addEntry (new CMenuItem ((*it)->c_str (), menuTag++));
							it++;
						}
						menu->addEntry (submenu, "Add New Template");
						submenu->forget ();
					}
				}
			}
		}
		return kMessageNotified;
	}
	else if (message == CEditFrame::kMsgPerformOptionsMenuAction)
	{
		CMenuItem* item = dynamic_cast<CMenuItem*> (sender);
		if (item)
		{
			if (item->getTitle () == std::string ("Template Settings..."))
			{
				runTemplateSettingsDialog ();
			}
			else
			{
				long index = item->getTag ();
				if (index >= 10000)
				{
					runNewTemplateDialog (item->getTitle ());
				}
				else
				{
					exchangeView (item->getTitle ());
				}
			}
		}
		return kMessageNotified;
	}
	else if (message == CEditFrame::kMsgEditEnding)
	{
		exchangeView (viewName.c_str ());
		return kMessageNotified;
	}
	else if (message == kMsgViewSizeChanged)
	{
		if (plugFrame)
		{
			rect.right = rect.left + (Steinberg::int32)frame->getWidth ();
			rect.bottom = rect.top + (Steinberg::int32)frame->getHeight ();
			plugFrame->resizeView (this, &rect);
		}
	}
	#endif
 	return VSTGUIEditor::notify (sender, message); 
}

#if VSTGUI_LIVE_EDITING
//------------------------------------------------------------------------
class VST3EditorNewTemplateDialogController : public IController
{
public:
	enum {
		kName,
		kWidth,
		kHeight
	};
	
	VST3EditorNewTemplateDialogController ()
	{
		values[kName] = "TemplateName";
		values[kWidth] = "300";
		values[kHeight] = "300";
	}
	void valueChanged (VSTGUI::CControl* pControl)
	{
		CTextEdit* textEdit = dynamic_cast<CTextEdit*> (pControl);
		if (textEdit)
		{
			long tag = textEdit->getTag ();
			if (tag <= 2)
			{
				if (tag > 0)
				{
					// verify text
					long tmp = strtol (textEdit->getText (), 0, 10);
					if (tmp > 0)
					{
						std::stringstream str;
						str << tmp;
						values[tag] = str.str ();
					}
					textEdit->setText (values[tag].c_str ());
				}
				else
					values[tag] = textEdit->getText ();
			}
		}
	}
	
	CView* verifyView (CView* view, const UIAttributes& attributes, IUIDescription* description)
	{
		CTextEdit* textEdit = dynamic_cast<CTextEdit*> (view);
		if (textEdit)
		{
			long tag = textEdit->getTag ();
			if (tag <= 2)
				textEdit->setText (values[tag].c_str ());
		}
		return view;
	}

	std::string values[3];
};

//------------------------------------------------------------------------
void VST3Editor::runNewTemplateDialog (const char* baseViewName)
{
	Xml::MemoryContentProvider mcp (vst3EditorTemplatesString, strlen (vst3EditorTemplatesString));
	UIDescription uiDesc (&mcp);
	if (!uiDesc.parse ())
		return;
	VST3EditorNewTemplateDialogController controller;
	CView* view = uiDesc.createView ("CreateNewTemplate", &controller);
	if (view)
	{
		CPoint p (-1, -1);
		if (Dialog::runViewModal (p, view, Dialog::kOkCancelButtons, "Create New Template"))
		{
			std::string sizeAttr (controller.values[VST3EditorNewTemplateDialogController::kWidth]);
			sizeAttr += ", ";
			sizeAttr += controller.values[VST3EditorNewTemplateDialogController::kHeight];
			UIAttributes* attr = new UIAttributes ();
			attr->setAttribute ("class", baseViewName);
			attr->setAttribute ("size", sizeAttr.c_str ());
			if (description->addNewTemplate (controller.values[VST3EditorNewTemplateDialogController::kName].c_str (), attr))
				exchangeView (controller.values[VST3EditorNewTemplateDialogController::kName].c_str ());
		}
		view->forget ();
	}
}

//------------------------------------------------------------------------
class VST3EditorTemplateSettingsDialogController : public IController
{
public:
	enum {
		kMinWidth,
		kMinHeight,
		kMaxWidth,
		kMaxHeight,
		kFocusColor,
		kFocusWidth,
		kFocusDrawingEnabled,
	};

	VST3EditorTemplateSettingsDialogController (const CPoint& minSize, const CPoint& maxSize, bool focusDrawingEnabled, const char* focusColorName, CCoord focusWidth, std::list<const std::string*>& colorNames)
	: focusDrawingEnabled (focusDrawingEnabled)
	, colorNames (colorNames)
	{
		std::stringstream str;
		str << minSize.x;
		values[kMinWidth] = str.str ();
		str.str ("");
		str << minSize.y;
		values[kMinHeight] = str.str ();
		str.str ("");
		str << maxSize.x;
		values[kMaxWidth] = str.str ();
		str.str ("");
		str << maxSize.y;
		values[kMaxHeight] = str.str ();
		str.str ("");
		values[kFocusColor] = focusColorName;
		str << focusWidth;
		values[kFocusWidth] = str.str ();
	}
	
	void valueChanged (VSTGUI::CControl* pControl) 
	{
		long tag = pControl->getTag ();
		CTextEdit* textEdit = dynamic_cast<CTextEdit*> (pControl);
		if (textEdit)
		{
			if (tag == kFocusWidth)
			{
				CCoord tmp = strtod (textEdit->getText (), 0);
				if (tmp > 0)
				{
					std::stringstream str;
					str << tmp;
					values[kFocusWidth] = str.str ();
				}
				textEdit->setText (values[kFocusWidth].c_str ());
			}
			else
			{
				long tmp = strtol (textEdit->getText (), 0, 10);
				if (tmp > 0)
				{
					std::stringstream str;
					str << tmp;
					values[tag] = str.str ();
				}
				textEdit->setText (values[tag].c_str ());
			}
		}
		else
		{
			COptionMenu* menu = dynamic_cast<COptionMenu*> (pControl);
			if (menu)
			{
				values[kFocusColor] = menu->getEntry (menu->getValue ())->getTitle ();
			}
			else if (tag == kFocusDrawingEnabled)
			{
				focusDrawingEnabled = pControl->getValue () == 1 ? true : false;
			}
		}

	}
	
	CView* verifyView (CView* view, const UIAttributes& attributes, IUIDescription* description)
	{
		CTextEdit* textEdit = dynamic_cast<CTextEdit*> (view);
		if (textEdit)
		{
			long tag = textEdit->getTag ();
			if (tag < 6)
				textEdit->setText (values[tag].c_str ());
		}
		CCheckBox* box = dynamic_cast<CCheckBox*> (view);
		if (box && box->getTag () == kFocusDrawingEnabled)
			box->setValue (focusDrawingEnabled ? 1.f : 0.f);
		COptionMenu* menu = dynamic_cast<COptionMenu*> (view);
		if (menu && menu->getTag () == kFocusColor)
		{
			CRect size;
			COptionMenu* colorMenu = CViewInspector::createMenuFromList (size, 0, colorNames, values[kFocusColor].c_str ());
			if (colorMenu)
			{
				CMenuItemIterator it = colorMenu->getItems ()->begin ();
				while (it != colorMenu->getItems ()->end ())
				{
					menu->addEntry ((*it));
					(*it)->remember ();
					it++;
				}
				menu->setValue (colorMenu->getValue ());
				colorMenu->forget ();
			}
		}
		return view;
	}
	
	std::string values[6];
	bool focusDrawingEnabled;
	std::list<const std::string*>& colorNames;
};

//------------------------------------------------------------------------
void VST3Editor::runTemplateSettingsDialog ()
{
	Xml::MemoryContentProvider mcp (vst3EditorTemplatesString, strlen (vst3EditorTemplatesString));
	UIDescription uiDesc (&mcp);
	if (!uiDesc.parse ())
		return;

	bool focusDrawingEnabled = false;
	CColor focusColor = kBlueCColor;
	CCoord focusWidth = 2;
	std::string currentColorName;
	UIAttributes* attributes = description->getCustomAttributes ("VST3Editor");
	if (attributes)
	{
		const std::string* attr = attributes->getAttributeValue (kFrameEnableFocusDrawingAttr);
		if (attr && *attr == "true")
		{
			focusDrawingEnabled = true;
		}
		attr = attributes->getAttributeValue (kFrameFocusColorAttr);
		if (attr)
		{
			if (description->getColor (attr->c_str (), focusColor))
				currentColorName = *attr;
		}
		attr = attributes->getAttributeValue (kFrameFocusWidthAttr);
		if (attr)
		{
			focusWidth = strtod (attr->c_str (), 0);
		}
	}
	std::list<const std::string*> colorNames;
	description->collectColorNames (colorNames);

	VST3EditorTemplateSettingsDialogController controller (minSize, maxSize, focusDrawingEnabled, currentColorName.c_str (), focusWidth, colorNames);
	CView* view = uiDesc.createView ("TemplateSettings", &controller);
	if (view)
	{
		CPoint p (-1, -1);
		if (Dialog::runViewModal (p, view, Dialog::kOkCancelButtons, "Template Settings"))
		{
			currentColorName = controller.values[VST3EditorTemplateSettingsDialogController::kFocusColor];
			focusWidth = strtod (controller.values[VST3EditorTemplateSettingsDialogController::kFocusWidth].c_str (), 0);
			if (attributes == 0)
				attributes = new UIAttributes ();
			attributes->setAttribute (kFrameEnableFocusDrawingAttr, controller.focusDrawingEnabled ? "true" : "false");
			attributes->setAttribute (kFrameFocusColorAttr, currentColorName.c_str ());
			attributes->setAttribute (kFrameFocusWidthAttr, controller.values[VST3EditorTemplateSettingsDialogController::kFocusWidth].c_str ());
			
			description->setCustomAttributes ("VST3Editor", attributes);
			
			frame->setFocusDrawingEnabled (controller.focusDrawingEnabled);
			if (description->getColor (currentColorName.c_str (), focusColor))
				frame->setFocusColor (focusColor);
			frame->setFocusWidth (focusWidth);
			frame->invalid ();

			UIAttributes* attr = const_cast<UIAttributes*> (description->getViewAttributes (viewName.c_str ()));
			if (attr)
			{
				std::string temp (controller.values[VST3EditorTemplateSettingsDialogController::kMinWidth]);
				temp += ", ";
				temp += controller.values[VST3EditorTemplateSettingsDialogController::kMinHeight];
				attr->setAttribute ("minSize", temp.c_str ());
				temp = controller.values[VST3EditorTemplateSettingsDialogController::kMaxWidth];
				temp += ", ";
				temp += controller.values[VST3EditorTemplateSettingsDialogController::kMaxHeight];
				attr->setAttribute ("maxSize", temp.c_str ());
				recreateView ();
			}
		}
		view->forget ();
	}
	
}

#endif // VSTGUI_LIVE_EDITING

} // namespace
