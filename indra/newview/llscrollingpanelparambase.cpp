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
	mSlider = getChild<LLSliderCtrl>("param slider");

	//Kill everything that isn't the slider...
	if(!bVisualHint)
	{
		child_list_t to_remove;
		child_list_t::const_iterator it;
		for (it = getChildList()->begin(); it != getChildList()->end(); it++)
		{
			if ((*it) != mSlider && (*it)->getName() != "panel border")
			{
				to_remove.push_back(*it);
			}
		}
		for (it = to_remove.begin(); it != to_remove.end(); it++)
		{
			removeChild(*it);
			delete (*it);
		}
		mSlider->translate(0,/*PARAM_HINT_HEIGHT*/128);
		reshape(getRect().getWidth(),getRect().getHeight()-128);
	}
	
	mSlider->setValue(weightToPercent(param->getWeight()));
	mSlider->setLabelArg("[DESC]", param->getDisplayName());
	mSlider->setEnabled(mAllowModify);
	mSlider->setCommitCallback(boost::bind(&LLScrollingPanelParamBase::onSliderMoved, this, _1));

	setVisible(FALSE);
	setBorderVisible( FALSE );
}

LLScrollingPanelParamBase::~LLScrollingPanelParamBase()
{
}

void LLScrollingPanelParamBase::updatePanel(BOOL allow_modify)
{
	if (!mWearable)
	{
		// not editing a wearable just now, no update necessary
		return;
	}

	F32 current_weight = mWearable->getVisualParamWeight(mParam->getID());
	mSlider->setValue(weightToPercent(current_weight));
	mAllowModify = allow_modify;
	mSlider->setEnabled(mAllowModify);
}

void LLScrollingPanelParamBase::onSliderMoved(LLUICtrl* ctrl)
{
	if (!mParam || !mWearable) return;

	F32 current_weight = mWearable->getVisualParamWeight(mParam->getID());
	F32 new_weight = percentToWeight(ctrl->getValue().asFloat());
	if (current_weight != new_weight)
	{
		mWearable->setVisualParamWeight( mParam->getID(), new_weight, FALSE);
		mWearable->writeToAvatar(gAgentAvatarp);
		gAgentAvatarp->updateVisualParams();
	}
}

F32 LLScrollingPanelParamBase::weightToPercent( F32 weight )
{
	return (weight - mParam->getMinWeight()) /  (mParam->getMaxWeight() - mParam->getMinWeight()) * 100.f;
}

F32 LLScrollingPanelParamBase::percentToWeight( F32 percent )
{
	return percent / 100.f * (mParam->getMaxWeight() - mParam->getMinWeight()) + mParam->getMinWeight();
}
