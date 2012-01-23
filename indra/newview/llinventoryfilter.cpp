/** 
* @file llinventoryfilter.cpp
* @brief Support for filtering your inventory to only display a subset of the
* available items.
*
* $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include "llinventoryfilter.h"

// viewer includes
#include "llfoldervieweventlistener.h"
#include "llfolderview.h"
#include "llinventorymodel.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llviewercontrol.h"
#include "llfolderview.h"
#include "llinventorybridge.h"
#include "llviewerfoldertype.h"
#include "llagentwearables.h"
#include "llvoavatarself.h"

// linden library includes
#include "lltrans.h"

LLInventoryFilter::FilterOps::FilterOps() :
	mFilterTypes(0xffffffff),
	mMinDate(time_min()),
	mMaxDate(time_max()),
	mHoursAgo(0),
	mShowFolderState(SHOW_NON_EMPTY_FOLDERS),
	mPermissions(PERM_NONE) {}
///----------------------------------------------------------------------------
/// Class LLInventoryFilter
///----------------------------------------------------------------------------
LLInventoryFilter::LLInventoryFilter(const std::string& name)
:	mName(name),
	mModified(FALSE),
	mNeedTextRebuild(TRUE)
{
	mOrder = SO_FOLDERS_BY_NAME; // This gets overridden by a pref immediately

	mSubStringMatchOffset = 0;
	mFilterSubString.clear();
	mFilterWorn = false;
	mFilterGeneration = 0;
	mMustPassGeneration = S32_MAX;
	mMinRequiredGeneration = 0;
	mFilterCount = 0;
	mNextFilterGeneration = mFilterGeneration + 1;

	mLastLogoff = gSavedPerAccountSettings.getU32("LastLogoff");
	mFilterBehavior = FILTER_NONE;

	// copy mFilterOps into mDefaultFilterOps
	markDefault();
}

LLInventoryFilter::~LLInventoryFilter()
{
}

BOOL LLInventoryFilter::check(LLFolderViewItem* item) 
{
	LLFolderViewEventListener* listener = item->getListener();
	const LLUUID& item_id = listener->getUUID();
	const LLInventoryObject *obj = gInventory.getObject(item_id);
	if (isActive() && obj && obj->getIsLinkType())
	{
		// When filtering is active, omit links.
		return FALSE;
	}

	time_t earliest;

	earliest = time_corrected() - mFilterOps.mHoursAgo * 3600;
	if (mFilterOps.mMinDate > time_min() && mFilterOps.mMinDate < earliest)
	{
		earliest = mFilterOps.mMinDate;
	}
	else if (!mFilterOps.mHoursAgo)
	{
		earliest = 0;
	}
	mSubStringMatchOffset = mFilterSubString.size() ? item->getSearchableLabel().find(mFilterSubString) : std::string::npos;
	BOOL passed = (0x1 << listener->getInventoryType() & mFilterOps.mFilterTypes || listener->getInventoryType() == LLInventoryType::IT_NONE)
					&& (mFilterSubString.size() == 0 || mSubStringMatchOffset != std::string::npos)
					&& (mFilterWorn == false || gAgentWearables.isWearingItem(item_id) ||
						(gAgentAvatarp && gAgentAvatarp->isWearingAttachment(item_id)))
					&& ((listener->getPermissionMask() & mFilterOps.mPermissions) == mFilterOps.mPermissions)
					&& (listener->getCreationDate() >= earliest && listener->getCreationDate() <= mFilterOps.mMaxDate);
	return passed;
}

const std::string& LLInventoryFilter::getFilterSubString(BOOL trim) const
{
	return mFilterSubString;
}

std::string::size_type LLInventoryFilter::getStringMatchOffset() const
{
	return mSubStringMatchOffset;
}

// has user modified default filter params?
BOOL LLInventoryFilter::isNotDefault() const
{
	return mFilterOps.mFilterTypes != mDefaultFilterOps.mFilterTypes 
		|| mFilterSubString.size() 
		|| mFilterWorn
		|| mFilterOps.mPermissions != mDefaultFilterOps.mPermissions
		|| mFilterOps.mMinDate != mDefaultFilterOps.mMinDate 
		|| mFilterOps.mMaxDate != mDefaultFilterOps.mMaxDate
		|| mFilterOps.mHoursAgo != mDefaultFilterOps.mHoursAgo;
}

BOOL LLInventoryFilter::isActive() const
{
	return mFilterOps.mFilterTypes != 0xffffffff 
		|| mFilterSubString.size() 
		|| mFilterWorn
		|| mFilterOps.mPermissions != PERM_NONE 
		|| mFilterOps.mMinDate != time_min()
		|| mFilterOps.mMaxDate != time_max()
		|| mFilterOps.mHoursAgo != 0;
}

BOOL LLInventoryFilter::isModified() const
{
	return mModified;
}

BOOL LLInventoryFilter::isModifiedAndClear()
{
	BOOL ret = mModified;
	mModified = FALSE;
	return ret;
}

void LLInventoryFilter::setFilterTypes(U32 types)
{
	if (mFilterOps.mFilterTypes != types)
	{
		// keep current items only if no type bits getting turned off
		BOOL fewer_bits_set = (mFilterOps.mFilterTypes & ~types);
		BOOL more_bits_set = (~mFilterOps.mFilterTypes & types);
	
		mFilterOps.mFilterTypes = types;
		if (more_bits_set && fewer_bits_set)
		{
			// neither less or more restrive, both simultaneously
			// so we need to filter from scratch
			setModified(FILTER_RESTART);
		}
		else if (more_bits_set)
		{
			// target is only one of all requested types so more type bits == less restrictive
			setModified(FILTER_LESS_RESTRICTIVE);
		}
		else if (fewer_bits_set)
		{
			setModified(FILTER_MORE_RESTRICTIVE);
		}

	}
}

void LLInventoryFilter::setFilterSubString(const std::string& string)
{
	std::string filter_sub_string_new = string;
	mFilterSubStringOrig = string;
	LLStringUtil::trimHead(filter_sub_string_new);
	LLStringUtil::toUpper(filter_sub_string_new);

	if (mFilterSubString != filter_sub_string_new)
	{
		// hitting BACKSPACE, for example
		const BOOL less_restrictive = mFilterSubString.size() >= filter_sub_string_new.size()
			&& !mFilterSubString.substr(0, filter_sub_string_new.size()).compare(filter_sub_string_new);

		// appending new characters
		const BOOL more_restrictive = mFilterSubString.size() < filter_sub_string_new.size()
			&& !filter_sub_string_new.substr(0, mFilterSubString.size()).compare(mFilterSubString);

		mFilterSubString = filter_sub_string_new;
		if (less_restrictive)
		{
			setModified(FILTER_LESS_RESTRICTIVE);
		}
		else if (more_restrictive)
		{
			setModified(FILTER_MORE_RESTRICTIVE);
		}
		else
		{
			setModified(FILTER_RESTART);
		}
	}
}

void LLInventoryFilter::setFilterPermissions(PermissionMask perms)
{
	if (mFilterOps.mPermissions != perms)
	{
		// keep current items only if no perm bits getting turned off
		BOOL fewer_bits_set = (mFilterOps.mPermissions & ~perms);
		BOOL more_bits_set = (~mFilterOps.mPermissions & perms);
		mFilterOps.mPermissions = perms;

		if (more_bits_set && fewer_bits_set)
		{
			setModified(FILTER_RESTART);
		}
		else if (more_bits_set)
		{
			// target must have all requested permission bits, so more bits == more restrictive
			setModified(FILTER_MORE_RESTRICTIVE);
		}
		else if (fewer_bits_set)
		{
			setModified(FILTER_LESS_RESTRICTIVE);
		}
	}
}

void LLInventoryFilter::setDateRange(time_t min_date, time_t max_date)
{
	mFilterOps.mHoursAgo = 0;
	if (mFilterOps.mMinDate != min_date)
	{
		mFilterOps.mMinDate = min_date;
		setModified();
	}
	if (mFilterOps.mMaxDate != llmax(mFilterOps.mMinDate, max_date))
	{
		mFilterOps.mMaxDate = llmax(mFilterOps.mMinDate, max_date);
		setModified();
	}
}

void LLInventoryFilter::setDateRangeLastLogoff(BOOL sl)
{
	if (sl && !isSinceLogoff())
	{
		setDateRange(mLastLogoff, time_max());
		setModified();
	}
	if (!sl && isSinceLogoff())
	{
		setDateRange(0, time_max());
		setModified();
	}
}

BOOL LLInventoryFilter::isSinceLogoff() const
{
	return (mFilterOps.mMinDate == (time_t)mLastLogoff) &&
		(mFilterOps.mMaxDate == time_max());
}

void LLInventoryFilter::clearModified()
{
	mModified = FALSE; 
	mFilterBehavior = FILTER_NONE;
}

void LLInventoryFilter::setHoursAgo(U32 hours)
{
	if (mFilterOps.mHoursAgo != hours)
	{
		bool are_date_limits_valid = mFilterOps.mMinDate == time_min() && mFilterOps.mMaxDate == time_max();

		bool is_increasing = hours > mFilterOps.mHoursAgo;
		bool is_increasing_from_zero = is_increasing && !mFilterOps.mHoursAgo;

		// *NOTE: need to cache last filter time, in case filter goes stale
		BOOL less_restrictive = (are_date_limits_valid && ((is_increasing && mFilterOps.mHoursAgo)) || !hours);
		BOOL more_restrictive = (are_date_limits_valid && (!is_increasing && hours) || is_increasing_from_zero);

		mFilterOps.mHoursAgo = hours;
		mFilterOps.mMinDate = time_min();
		mFilterOps.mMaxDate = time_max();
		if (less_restrictive)
		{
			setModified(FILTER_LESS_RESTRICTIVE);
		}
		else if (more_restrictive)
		{
			setModified(FILTER_MORE_RESTRICTIVE);
		}
		else
		{
			setModified(FILTER_RESTART);
		}
	}
}
void LLInventoryFilter::setShowFolderState(EFolderShow state)
{
	if (mFilterOps.mShowFolderState != state)
	{
		mFilterOps.mShowFolderState = state;
		if (state == SHOW_NON_EMPTY_FOLDERS)
		{
			// showing fewer folders than before
			setModified(FILTER_MORE_RESTRICTIVE);
		}
		else if (state == SHOW_ALL_FOLDERS)
		{
			// showing same folders as before and then some
			setModified(FILTER_LESS_RESTRICTIVE);
		}
		else
		{
			setModified();
		}
	}
}

void LLInventoryFilter::setSortOrder(U32 order)
{
	if (mOrder != order)
	{
		mOrder = order;
		setModified();
	}
}

void LLInventoryFilter::markDefault()
{
	mDefaultFilterOps = mFilterOps;
}

void LLInventoryFilter::resetDefault()
{
	mFilterOps = mDefaultFilterOps;
	setModified();
}

void LLInventoryFilter::setModified(EFilterBehavior behavior)
{
	mModified = TRUE;
	mNeedTextRebuild = TRUE;
	mFilterGeneration = mNextFilterGeneration++;
	
	if (mFilterBehavior == FILTER_NONE)
	{
		mFilterBehavior = behavior;
	}
	else if (mFilterBehavior != behavior)
	{
		// trying to do both less restrictive and more restrictive filter
		// basically means restart from scratch
		mFilterBehavior = FILTER_RESTART;
	}

	if (isNotDefault())
	{
		// if not keeping current filter results, update last valid as well
		switch(mFilterBehavior)
		{
		case FILTER_RESTART:
			mMustPassGeneration = mFilterGeneration;
			mMinRequiredGeneration = mFilterGeneration;
			break;
		case FILTER_LESS_RESTRICTIVE:
			mMustPassGeneration = mFilterGeneration;
			break;
		case FILTER_MORE_RESTRICTIVE:
			mMinRequiredGeneration = mFilterGeneration;
			// must have passed either current filter generation (meaningless, as it hasn't been run yet)
			// or some older generation, so keep the value
			mMustPassGeneration = llmin(mMustPassGeneration, mFilterGeneration);
			break;
		default:
			llerrs << "Bad filter behavior specified" << llendl;
		}
	}
	else
	{
		// shortcut disabled filters to show everything immediately
		mMinRequiredGeneration = 0;
		mMustPassGeneration = S32_MAX;
	}
}

BOOL LLInventoryFilter::isFilterWith(LLInventoryType::EType t) const
{
	return mFilterOps.mFilterTypes & (0x01 << t);
}

const std::string& LLInventoryFilter::getFilterText()
{
	if (!mNeedTextRebuild)
	{
		return mFilterText;
	}

	mNeedTextRebuild = FALSE;
	std::string filtered_types;
	std::string not_filtered_types;
	BOOL filtered_by_type = FALSE;
	BOOL filtered_by_all_types = TRUE;
	S32 num_filter_types = 0;
	mFilterText.clear();

	if (isFilterWith(LLInventoryType::IT_ANIMATION))
	{
		filtered_types += " Animations,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Animations,";
		filtered_by_all_types = FALSE;
	}

	if (isFilterWith(LLInventoryType::IT_CALLINGCARD))
	{
		filtered_types += " Calling Cards,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Calling Cards,";
		filtered_by_all_types = FALSE;
	}

	if (isFilterWith(LLInventoryType::IT_WEARABLE))
	{
		filtered_types += " Clothing,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Clothing,";
		filtered_by_all_types = FALSE;
	}

	if (isFilterWith(LLInventoryType::IT_GESTURE))
	{
		filtered_types += " Gestures,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Gestures,";
		filtered_by_all_types = FALSE;
	}

	if (isFilterWith(LLInventoryType::IT_LANDMARK))
	{
		filtered_types += " Landmarks,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Landmarks,";
		filtered_by_all_types = FALSE;
	}

	if (isFilterWith(LLInventoryType::IT_NOTECARD))
	{
		filtered_types += " Notecards,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Notecards,";
		filtered_by_all_types = FALSE;
	}
	
	if (isFilterWith(LLInventoryType::IT_OBJECT) && isFilterWith(LLInventoryType::IT_ATTACHMENT))
	{
		filtered_types += " Objects,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Objects,";
		filtered_by_all_types = FALSE;
	}
	
	if (isFilterWith(LLInventoryType::IT_LSL))
	{
		filtered_types += " Scripts,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Scripts,";
		filtered_by_all_types = FALSE;
	}
	
	if (isFilterWith(LLInventoryType::IT_SOUND))
	{
		filtered_types += " Sounds,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Sounds,";
		filtered_by_all_types = FALSE;
	}

	if (isFilterWith(LLInventoryType::IT_TEXTURE))
	{
		filtered_types += " Textures,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Textures,";
		filtered_by_all_types = FALSE;
	}

	if (isFilterWith(LLInventoryType::IT_SNAPSHOT))
	{
		filtered_types += " Snapshots,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Snapshots,";
		filtered_by_all_types = FALSE;
	}

	if (!LLInventoryModelBackgroundFetch::instance().backgroundFetchActive() 
		&& filtered_by_type 
		&& !filtered_by_all_types)
	{
		mFilterText += " - ";
		if (num_filter_types < 5)
		{
			mFilterText += filtered_types;
		}
		else
		{
			mFilterText += "No ";
			mFilterText += not_filtered_types;
		}
		// remove the ',' at the end
		mFilterText.erase(mFilterText.size() - 1, 1);
	}

	if (isSinceLogoff())
	{
		mFilterText += " - Since Logoff";
	}
	
	if (getFilterWorn())
	{
		mFilterText += " - Worn";
	}
	
	return mFilterText;
}

void LLInventoryFilter::toLLSD(LLSD& data) const
{
	data["filter_types"] = (LLSD::Integer)getFilterTypes();
	data["min_date"] = (LLSD::Integer)getMinDate();
	data["max_date"] = (LLSD::Integer)getMaxDate();
	data["hours_ago"] = (LLSD::Integer)getHoursAgo();
	data["show_folder_state"] = (LLSD::Integer)getShowFolderState();
	data["permissions"] = (LLSD::Integer)getFilterPermissions();
	data["substring"] = (LLSD::String)getFilterSubString();
	data["sort_order"] = (LLSD::Integer)getSortOrder();
	data["since_logoff"] = (LLSD::Boolean)isSinceLogoff();
}

void LLInventoryFilter::fromLLSD(LLSD& data)
{
	if(data.has("filter_types"))
	{
		setFilterTypes((U32)data["filter_types"].asInteger());
	}

	if(data.has("min_date") && data.has("max_date"))
	{
		setDateRange(data["min_date"].asInteger(), data["max_date"].asInteger());
	}

	if(data.has("hours_ago"))
	{
		setHoursAgo((U32)data["hours_ago"].asInteger());
	}

	if(data.has("show_folder_state"))
	{
		setShowFolderState((EFolderShow)data["show_folder_state"].asInteger());
	}

	if(data.has("permissions"))
	{
		setFilterPermissions((PermissionMask)data["permissions"].asInteger());
	}

	if(data.has("substring"))
	{
		setFilterSubString(std::string(data["substring"].asString()));
	}

	if(data.has("sort_order"))
	{
		setSortOrder((U32)data["sort_order"].asInteger());
	}

	if(data.has("since_logoff"))
	{
		setDateRangeLastLogoff((bool)data["since_logoff"].asBoolean());
	}
}

BOOL LLInventoryFilter::hasFilterString() const
{
	return mFilterSubString.size() > 0;
}

PermissionMask LLInventoryFilter::getFilterPermissions() const
{
	return mFilterOps.mPermissions;
}

time_t LLInventoryFilter::getMinDate() const
{
	return mFilterOps.mMinDate;
}

time_t LLInventoryFilter::getMaxDate() const 
{ 
	return mFilterOps.mMaxDate; 
}
U32 LLInventoryFilter::getHoursAgo() const 
{ 
	return mFilterOps.mHoursAgo; 
}
LLInventoryFilter::EFolderShow LLInventoryFilter::getShowFolderState() const
{ 
	return mFilterOps.mShowFolderState; 
}
U32 LLInventoryFilter::getSortOrder() const 
{ 
	return mOrder; 
}
const std::string& LLInventoryFilter::getName() const 
{ 
	return mName; 
}

void LLInventoryFilter::setFilterCount(S32 count) 
{ 
	mFilterCount = count; 
}
S32 LLInventoryFilter::getFilterCount() const
{
	return mFilterCount;
}

void LLInventoryFilter::decrementFilterCount() 
{ 
	mFilterCount--; 
}

S32 LLInventoryFilter::getCurrentGeneration() const 
{ 
	return mFilterGeneration; 
}
S32 LLInventoryFilter::getMinRequiredGeneration() const 
{ 
	return mMinRequiredGeneration; 
}
S32 LLInventoryFilter::getMustPassGeneration() const 
{ 
	return mMustPassGeneration; 
}
