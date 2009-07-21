/*
 *  cviewswitchcontainer.cpp
 *
 *  Created by Arne Scheffler on 7/19/09.
 *  Copyright 2009 Arne Scheffler. All rights reserved.
 *
 */

#include "cviewswitchcontainer.h"
#include "viewcreator.h"

BEGIN_NAMESPACE_VSTGUI

//-----------------------------------------------------------------------------
CViewSwitchContainer::CViewSwitchContainer (const CRect& size)
: CViewContainer (size, 0)
, controller (0)
, currentViewIndex (0)
{
}

//-----------------------------------------------------------------------------
CViewSwitchContainer::~CViewSwitchContainer ()
{
	setController (0);
}

//-----------------------------------------------------------------------------
void CViewSwitchContainer::setController (IViewSwitchController* _controller)
{
	if (controller)
	{
		CBaseObject* obj = dynamic_cast<CBaseObject*> (controller);
		if (obj)
			obj->forget ();
	}
	controller = _controller;
}

//-----------------------------------------------------------------------------
void CViewSwitchContainer::setCurrentViewIndex (long viewIndex)
{
	if (controller)
	{
		CView* view = controller->createViewForIndex (viewIndex);
		if (view)
		{
			CViewContainer::removeView (getView (0));
			CViewContainer::addView (view);
			currentViewIndex = viewIndex;
			invalid ();
		}
	}
}

//-----------------------------------------------------------------------------
bool CViewSwitchContainer::attached (CView* parent)
{
	setCurrentViewIndex (currentViewIndex);
	bool result = CViewContainer::attached (parent);
	if (result && controller)
		controller->switchContainerAttached ();
	return result;
}

//-----------------------------------------------------------------------------
UIDescriptionViewSwitchController::UIDescriptionViewSwitchController (CViewSwitchContainer* viewSwitch, UIDescription* uiDescription, IController* uiController)
: IViewSwitchController (viewSwitch)
, uiDescription (uiDescription)
, uiController (uiController)
, switchControlTag (-1)
{
	init ();
}

//-----------------------------------------------------------------------------
CView* UIDescriptionViewSwitchController::createViewForIndex (long index)
{
	if (index < templateNames.size ())
	{
		return uiDescription->createView (templateNames[index].c_str (), uiController);
	}
	return 0;
}

//-----------------------------------------------------------------------------
static CControl* findControlTag (CViewContainer* parent, long tag)
{
	CControl* result = 0;
	for (long i = 0; i < parent->getNbViews (); i++)
	{
		CView* view = parent->getView (i);
		CControl* control = dynamic_cast<CControl*> (view);
		if (control)
		{
			if (control->getTag () == tag)
				result = control;
		}
		else
		{
			CViewContainer* container = dynamic_cast<CViewContainer*> (view);
			if (container)
				result = findControlTag (container, tag);
		}
		if (result)
			break;
	}
	return result;
}

//-----------------------------------------------------------------------------
void UIDescriptionViewSwitchController::switchContainerAttached ()
{
	if (switchControlTag != -1)
	{
		// find the switch Control
		CControl* control = findControlTag (viewSwitch->getFrame (), switchControlTag);
		if (control)
			control->setListener (this);
	}
}

//-----------------------------------------------------------------------------
void UIDescriptionViewSwitchController::valueChanged (CControl* pControl)
{
	float value = pControl->getValue ();
	float min = pControl->getMin ();
	float max = pControl->getMax ();
	float norm = (value - min) / (max - min);
	long index = std::min<long> (norm * templateNames.size (), templateNames.size ()-1);
	viewSwitch->setCurrentViewIndex (index);
}

//-----------------------------------------------------------------------------
void UIDescriptionViewSwitchController::setTemplateNames (const char* _templateNames)
{
	templateNames.clear ();
	if (_templateNames)
	{
		std::string temp (_templateNames);
		size_t start = 0;
		size_t pos = temp.find (",", start, 1);
		if (pos != std::string::npos)
		{
			while (pos != std::string::npos)
			{
				std::string name (temp, start, pos - start);
				templateNames.push_back (name);
				start = pos+1;
				pos = temp.find (",", start, 1);
			}
			std::string name (temp, start, std::string::npos);
			templateNames.push_back (name);
		}
		else
		{
			// only one template name
			templateNames.push_back (temp);
		}
	}
}

//-----------------------------------------------------------------------------
void UIDescriptionViewSwitchController::getTemplateNames (std::string& str)
{
	str.clear ();
	for (size_t i = 0; i < templateNames.size (); i++)
	{
		str += templateNames[i];
		if (i != templateNames.size () - 1)
		{
			str += ",";
		}
	}
}

//-----------------------------------------------------------------------------
class CViewSwitchContainerCreator : public IViewCreator
{
public:
	CViewSwitchContainerCreator () { ViewFactory::registerViewCreator (*this); }
	const char* getViewName () const { return "CViewSwitchContainer"; }
	const char* getBaseViewName () const { return "CViewContainer"; }
	CView* create (const UIAttributes& attributes, IUIDescription* description) const 
	{
		CViewSwitchContainer* vsc = new CViewSwitchContainer (CRect (0, 0, 100, 100));
		new UIDescriptionViewSwitchController (vsc, dynamic_cast<UIDescription*> (description), description->getController ());
		return vsc;
	}

	bool apply (CView* view, const UIAttributes& attributes, IUIDescription* description) const
	{
		CViewSwitchContainer* viewSwitch = dynamic_cast<CViewSwitchContainer*> (view);
		if (!viewSwitch)
			return false;
		const std::string* attr = attributes.getAttributeValue ("template-names");
		if (attr)
		{
			UIDescriptionViewSwitchController* controller = dynamic_cast<UIDescriptionViewSwitchController*> (viewSwitch->getController ());
			if (controller)
			{
				controller->setTemplateNames (attr->c_str ());
			}
		}
		attr = attributes.getAttributeValue ("template-switch-control");
		if (attr)
		{
			rememberAttributeValueString (view, "template-switch-control", *attr);
			UIDescriptionViewSwitchController* controller = dynamic_cast<UIDescriptionViewSwitchController*> (viewSwitch->getController ());
			if (controller)
			{
				long tag = description->getTagForName (attr->c_str ());
				controller->setSwitchControlTag (tag);
			}
		}
		return true;
	}
	bool getAttributeNames (std::list<std::string>& attributeNames) const
	{
		attributeNames.push_back ("template-names");
		attributeNames.push_back ("template-switch-control");
		return true;
	}
	AttrType getAttributeType (const std::string& attributeName) const
	{
		if (attributeName == "template-names") return kStringType;
		if (attributeName == "template-switch-control") return kTagType;
		return kUnknownType;
	}
	bool getAttributeValue (CView* view, const std::string& attributeName, std::string& stringValue, IUIDescription* desc) const
	{
		CViewSwitchContainer* viewSwitch = dynamic_cast<CViewSwitchContainer*> (view);
		if (!viewSwitch)
			return false;
		if (attributeName == "template-names")
		{
			UIDescriptionViewSwitchController* controller = dynamic_cast<UIDescriptionViewSwitchController*> (viewSwitch->getController ());
			if (controller)
			{
				controller->getTemplateNames (stringValue);
				return true;
			}
		}
		if (attributeName == "template-switch-control")
		{
			UIDescriptionViewSwitchController* controller = dynamic_cast<UIDescriptionViewSwitchController*> (viewSwitch->getController ());
			if (controller)
			{
				if (getRememberedAttributeValueString (view, "template-switch-control", stringValue))
					return true;
				const char* controlTag = desc->lookupControlTagName (controller->getSwitchControlTag ());
				if (controlTag)
				{
					stringValue = controlTag;
					return true;
				}
				return true;
			}
		}
		return false;
	}
};
CViewSwitchContainerCreator __gCViewSwitchContainerCreator;


END_NAMESPACE_VSTGUI
