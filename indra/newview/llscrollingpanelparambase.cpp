/** 
 * @file llscrollingpanelparam.cpp
 * @brief UI panel for a list of visual param panels
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llscrollingpanelparambase.h"
#include "llviewerjointmesh.h"
#include "llviewervisualparam.h"
#include "llwearable.h"
#include "llviewervisualparam.h"
#include "lltoolmorph.h"
#include "lltrans.h"
#include "llbutton.h"
#include "llsliderctrl.h"
#include "llagent.h"
#include "llviewborder.h"
#include "llvoavatarself.h"
#include "llagentwearables.h"

LLScrollingPanelParamBase::LLScrollingPanelParamBase( const std::string& name,
							LLViewerJointMesh* mesh, LLViewerVisualParam* param, BOOL allow_modify, LLWearable* wearable, bool bVisualHint, LLRect rect )
	: LLScrollingPanel( name, rect ),
	mParam(param),
	mAllowModify(allow_modify),
	mWearable(wearable)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_scrolling_param.xml");
	//Set up the slider
	LLSliderCtrl *slider = getChild<LLSliderCtrl>("param slider");

	//Kill everything that isn't the slider...
	if(!bVisualHint)
	{
		child_list_t to_remove;
		child_list_t::const_iterator it;
		for (it = getChildList()->begin(); it != getChildList()->end(); it++)
		{
			if ((*it) != slider && (*it)->getName() != "panel border")
			{
				to_remove.push_back(*it);
			}
		}
		for (it = to_remove.begin(); it != to_remove.end(); it++)
		{
			removeChild(*it);
			delete (*it);
		}
		slider->translate(0,/*PARAM_HINT_HEIGHT*/128);
		reshape(getRect().getWidth(),getRect().getHeight()-128);
	}
	
	slider->setValue(weightToPercent(param->getWeight()));
	slider->setLabelArg("[DESC]", param->getDisplayName());
	slider->setEnabled(mAllowModify);
	slider->setCommitCallback(boost::bind(&LLScrollingPanelParamBase::onSliderMoved, this, _1));

	setVisible(FALSE);
	setBorderVisible( FALSE );
}

LLScrollingPanelParamBase::~LLScrollingPanelParamBase()
{
}

void LLScrollingPanelParamBase::updatePanel(BOOL allow_modify)
{
	LLViewerVisualParam* param = mParam;
	
	if(!mWearable)
	{
		// not editing a wearable just now, no update necessary
		return;
	}

	F32 current_weight = mWearable->getVisualParamWeight( param->getID() );
	childSetValue("param slider", weightToPercent( current_weight ) );
	mAllowModify = allow_modify;
	childSetEnabled("param slider", mAllowModify);
}

void LLScrollingPanelParamBase::onSliderMoved(LLUICtrl* ctrl)
{
	if(!mParam)
	{
		return;
	}

	if(!mWearable)
	{
		return;
	}
	
	LLSliderCtrl* slider = (LLSliderCtrl*) ctrl;

	F32 current_weight = mWearable->getVisualParamWeight(mParam->getID());
	F32 new_weight = percentToWeight( (F32)slider->getValue().asReal() );
	if (current_weight != new_weight )
	{
		mWearable->setVisualParamWeight( mParam->getID(), new_weight, FALSE);
		mWearable->writeToAvatar(gAgentAvatarp);
		gAgentAvatarp->updateVisualParams();
	}
}

F32 LLScrollingPanelParamBase::weightToPercent( F32 weight )
{
	LLViewerVisualParam* param = mParam;
	return (weight - param->getMinWeight()) /  (param->getMaxWeight() - param->getMinWeight()) * 100.f;
}

F32 LLScrollingPanelParamBase::percentToWeight( F32 percent )
{
	LLViewerVisualParam* param = mParam;
	return percent / 100.f * (param->getMaxWeight() - param->getMinWeight()) + param->getMinWeight();
}
