/** 
 * @file llfasttimer_class.cpp
 * @brief Implementation of the fast timer.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

//
// LLFastTimer documentation, written by Aleric (Feb 2012).
//
// Disclaimer: this is horrible code and I distantiate myself from its design.
// It's neither robust nor object oriented.  I just document what I find, in
// order to be able to fix the bugs (that logically result from such a design).
//
// Note that the choosen names of the variables are non-intuitive and make
// understanding the code harder. However, I didn't change them in order to
// make merging less of a nightmare in the future -- Aleric.
//
//
// First of all, absolutely nothing in this code is even remotely thread-safe:
// FastTimers should only be used from the main thread and never from another
// thread.
//
// NamedTimerFactory is a singleton, accessed through NamedTimerFactory::instance().
//
// It has four pointer members which are initialized once to point to
// four objects with a life-time equal to the application/singleton:
//
//   mTimerRoot       --> NamedTimer("root")
//   mActiveTimerRoot --> NamedTimer("Frame")
//   mRootFrameState  --> FrameState(mActiveTimerRoot)
//   mAppTimer        --> LLFastTimer(mRootFrameState)
//
// A NamedTimer has a name and a life-time of approximately that of the application.
// There is exactly one instance per unique name.
// NamedTimer's are ordered in a hierarchy with each one parent and zero or more
// children (the "root" has parent NULL).
// The parent of mActiveTimerRoot is mTimerRoot, which has one child: mActiveTimerRoot.
// NamedTimer::getDepth() returns the number of parents; mTimerRoot has a depth of 0,
// mActiveTimerRoot has a depth of 1 and so on. NamedTimer::getRootNamedTimer() just
// returns mActiveTimerRoot.
//
// Each NamedTimer is linked to exactly one FrameState object, namely
// LLFastTimer::getFrameStateList()[named_timer.getFrameStateIndex()], where
// getFrameStateList() is a static function returning a global std::vector<FrameState>.
// This vector is ordered "Depth First" (the FrameState objects (belonging to
// NamedTimer objects) with smallest depth first). The vector is re-sorted a few
// times in the beginning (and indexes in FrameState updated) since timers are added
// whenever they are first used, not in "Depth First" order, but stabilizes after a
// while. This implies that FrameState pointers can't really be used: FrameState
// objects move around in memory whenever something is inserted or removed from the
// std::vector and/or when the vector is re-sorted. However, FrameState pointers ARE
// being used and code exists that tries to update those pointers in the above
// mentioned cases (this part had bugs, which I now fixed).
// 
// FrameState objects point back to their corresponding NamedTimer through mTimer.
// They have also parents: the FrameState object corresponding to the parent of mTimer.
//
// Thus, so far we have (assuming "namedtimerX" was created first):
//
//          NamedTimer's:                             FrameState's:
//
//             NULL
//              ^
//              |
// depth=0:  "root" (mTimerRoot)          <------->   getFrameStateList()[0]
//              ^                                           ^
//              | (parent)                                  | (parent)
//              |                                           |
// depth=1:  "Frame" (mActiveTimerRoot)   <------->   mRootFrameState
//              ^               ^                           ^                          ^
//              |               |                           |                          |
//              | (parent)      | (parent)                  | (parent)                 | (parent)
//              |               |                           |                          |
// depth=2:  "namedtimerX"      |         <------->   getFrameStateList()[2]           |
//                           "namedtimerY"  <------->                            getFrameStateList()[3]
//
// where the NamedTimer's point to the corresponding FrameState's by means of
// NamedTimer::mFrameStateIndex, and the FrameState's point back through FrameState::mTimer.
//
// Note the missing getFrameStateList()[1], which is ignored and replaced by
// a specific call to 'new FrameState' in initSingleton(). The reason for that is
// probably because otherwise mRootFrameState has to be updated every time the
// frame state list vector is moved in memory. This special case adds some complexity to,
// for instance, getFrameState() which now needs to test if the caller is mActiveTimerRoot.
//
// DeclareTimer objects are NameTimer/FrameState pointer pairs with again a lifetime
// of approximately that of the application. The are usually static, even global,
// and are passed an name as string; the name is looked up and added if not already
// existing, or else the previously created pair is returned. Obviously, "root" and
// "Frame" are the only ones that don't have a corresponding DeclareTimer object.
//
// LLFastTimer objects are short lived objects, created in a scope and destroyed
// at the end in order to measure the time that the application spent in that
// scope. They are passed DeclareTimer objects to know which timer to append to.
// LLFastTimer::mFrameState is a pointer to the corresponding timer.
// The static LLFastTimer::sCurTimerData is a CurTimerData struct that has
// a duplicate of that pointer as well as a pointer to the corresponding NamedTimer,
// of the last LLFastTimer object that was created (and not destroyed again);
// in other words: the running timer with the largest depth.
// When a new LLFastTimer object is created while one is already running,
// then this sCurTimerData is saved in the already running one (as
// LLFastTimer::mLastTimerData) and restored upon destruction of that child timer.
//
// The following FrameState pointers are being used:
//
// FrameState::mParent
// DeclareTimer::mFrameState
// CurTimerData::mFrameState
// LLFastTimer::mFrameState
//
// All of those can be invalidated whenever something is added to the std::vector<FrameState>,
// and when that vector is sorted.
//
// Adding new FrameState objects is done in NamedTimer(std::string const& name), called from
// createNamedTimer(), called whenever a DeclareTimer is constructed. At the end of the
// DeclareTimer constructor update_cached_pointers_if_changed() is called, which calls
// updateCachedPointers() if the std::vector moved in memory since last time it was called.
//
// Sorting is done in NamedTimer::resetFrame(), which theoretically can be called from
// anywhere. Also here updateCachedPointers() is called, directly after sorting the vector.
//
// I fixed updateCachedPointers() to correct all of the above pointers and removed
// another FrameState pointer that was unnecessary.

#include "linden_common.h"

#include "llfasttimer.h"

#include "llmemory.h"
#include "llprocessor.h"
#include "llsingleton.h"
#include "lltreeiterators.h"
#include "llsdserialize.h"

#include <boost/bind.hpp>


#if LL_WINDOWS
#include "lltimer.h"
#elif LL_LINUX || LL_SOLARIS
#include <sys/time.h>
#include <sched.h>
#include "lltimer.h"
#elif LL_DARWIN
#include <sys/time.h>
#include "lltimer.h"	// get_clock_count()
#else 
#error "architecture not supported"
#endif

//////////////////////////////////////////////////////////////////////////////
// statics

S32 LLFastTimer::sCurFrameIndex = -1;
S32 LLFastTimer::sLastFrameIndex = -1;
U64 LLFastTimer::sLastFrameTime = LLFastTimer::getCPUClockCount64();
bool LLFastTimer::sPauseHistory = 0;
bool LLFastTimer::sResetHistory = 0;
LLFastTimer::CurTimerData LLFastTimer::sCurTimerData;
BOOL LLFastTimer::sLog = FALSE;
std::string LLFastTimer::sLogName = "";
BOOL LLFastTimer::sMetricLog = FALSE;
LLMutex* LLFastTimer::sLogLock = NULL;
std::queue<LLSD> LLFastTimer::sLogQueue;
const int LLFastTimer::NamedTimer::HISTORY_NUM = 300;

#if defined(LL_WINDOWS) && !defined(_WIN64)
#define USE_RDTSC 1
#endif

std::vector<LLFastTimer::FrameState>* LLFastTimer::sTimerInfos = NULL;
U64				LLFastTimer::sTimerCycles = 0;
U32				LLFastTimer::sTimerCalls = 0;


// FIXME: move these declarations to the relevant modules

// helper functions
typedef LLTreeDFSPostIter<LLFastTimer::NamedTimer, LLFastTimer::NamedTimer::child_const_iter> timer_tree_bottom_up_iterator_t;

static timer_tree_bottom_up_iterator_t begin_timer_tree_bottom_up(LLFastTimer::NamedTimer& id) 
{ 
	return timer_tree_bottom_up_iterator_t(&id, 
							boost::bind(boost::mem_fn(&LLFastTimer::NamedTimer::beginChildren), _1), 
							boost::bind(boost::mem_fn(&LLFastTimer::NamedTimer::endChildren), _1));
}

static timer_tree_bottom_up_iterator_t end_timer_tree_bottom_up() 
{ 
	return timer_tree_bottom_up_iterator_t(); 
}

typedef LLTreeDFSIter<LLFastTimer::NamedTimer, LLFastTimer::NamedTimer::child_const_iter> timer_tree_dfs_iterator_t;


static timer_tree_dfs_iterator_t begin_timer_tree(LLFastTimer::NamedTimer& id) 
{ 
	return timer_tree_dfs_iterator_t(&id, 
		boost::bind(boost::mem_fn(&LLFastTimer::NamedTimer::beginChildren), _1), 
							boost::bind(boost::mem_fn(&LLFastTimer::NamedTimer::endChildren), _1));
}

static timer_tree_dfs_iterator_t end_timer_tree() 
{ 
	return timer_tree_dfs_iterator_t(); 
}



// factory class that creates NamedTimers via static DeclareTimer objects
class NamedTimerFactory : public LLSingleton<NamedTimerFactory>
{
public:
	NamedTimerFactory()
		: mActiveTimerRoot(NULL),
		  mTimerRoot(NULL),
		  mAppTimer(NULL),
		  mRootFrameState(NULL)
	{}

	/*virtual */ void initSingleton()
	{
		mTimerRoot = new LLFastTimer::NamedTimer("root");

		mActiveTimerRoot = new LLFastTimer::NamedTimer("Frame");
		mActiveTimerRoot->setCollapsed(false);

		mRootFrameState = new LLFastTimer::FrameState(mActiveTimerRoot);
		// getFrameState and setParent recursively call this function,
		// so we have to work around that by using a specialized implementation
		// for the special case were mTimerRoot != mActiveTimerRoot -- Aleric
		mRootFrameState->mParent = &LLFastTimer::getFrameStateList()[0];	// &mTimerRoot->getFrameState()
		mRootFrameState->mParent->mActiveCount = 1;
		// And the following four lines are mActiveTimerRoot->setParent(mTimerRoot);
		llassert(!mActiveTimerRoot->mParent);
		mActiveTimerRoot->mParent = mTimerRoot;								// mParent = parent;
		//mRootFrameState->mParent = mRootFrameState->mParent;				// getFrameState().mParent = &parent->getFrameState();
		mTimerRoot->getChildren().push_back(mActiveTimerRoot);				// parent->getChildren().push_back(this);
		mTimerRoot->mNeedsSorting = true;									// parent->mNeedsSorting = true;

		mAppTimer = new LLFastTimer(mRootFrameState);
	}

	~NamedTimerFactory()
	{
		std::for_each(mTimers.begin(), mTimers.end(), DeletePairedPointer());

		delete mAppTimer;
		delete mActiveTimerRoot; 
		delete mTimerRoot;
		delete mRootFrameState;
	}

	LLFastTimer::NamedTimer& createNamedTimer(const std::string& name)
	{
		timer_map_t::iterator found_it = mTimers.find(name);
		if (found_it != mTimers.end())
		{
			return *found_it->second;
		}

		LLFastTimer::NamedTimer* timer = new LLFastTimer::NamedTimer(name);
		timer->setParent(mTimerRoot);
		mTimers.insert(std::make_pair(name, timer));

		return *timer;
	}

	LLFastTimer::NamedTimer* getTimerByName(const std::string& name)
	{
		timer_map_t::iterator found_it = mTimers.find(name);
		if (found_it != mTimers.end())
		{
			return found_it->second;
		}
		return NULL;
	}

	LLFastTimer::NamedTimer* getActiveRootTimer() { return mActiveTimerRoot; }
	LLFastTimer::NamedTimer* getRootTimer() { return mTimerRoot; }
	const LLFastTimer* getAppTimer() { return mAppTimer; }
	LLFastTimer::FrameState& getRootFrameState() { return *mRootFrameState; }

	typedef std::map<std::string, LLFastTimer::NamedTimer*> timer_map_t;
	timer_map_t::iterator beginTimers() { return mTimers.begin(); }
	timer_map_t::iterator endTimers() { return mTimers.end(); }
	S32 timerCount() { return mTimers.size(); }

private:
	timer_map_t mTimers;

	LLFastTimer::NamedTimer*		mActiveTimerRoot;
	LLFastTimer::NamedTimer*		mTimerRoot;
	LLFastTimer*						mAppTimer;
	LLFastTimer::FrameState*		mRootFrameState;		// Points to memory allocated with new, so this pointer is not invalidated.
};

void update_cached_pointers_if_changed()
{
	// detect when elements have moved and update cached pointers
	static LLFastTimer::FrameState* sFirstTimerAddress = NULL;
	if (&*(LLFastTimer::getFrameStateList().begin()) != sFirstTimerAddress)
	{
		LLFastTimer::updateCachedPointers();
		sFirstTimerAddress = &*(LLFastTimer::getFrameStateList().begin());
	}
}

LLFastTimer::DeclareTimer::DeclareTimer(const std::string& name, bool open )
:	mTimer(NamedTimerFactory::instance().createNamedTimer(name))
{
	mTimer.setCollapsed(!open);
	mFrameState = &mTimer.getFrameState();
	update_cached_pointers_if_changed();
}

LLFastTimer::DeclareTimer::DeclareTimer(const std::string& name)
:	mTimer(NamedTimerFactory::instance().createNamedTimer(name))
{
	mFrameState = &mTimer.getFrameState();
	update_cached_pointers_if_changed();
}

// static
void LLFastTimer::updateCachedPointers()
{
	// Update DeclareTimer::mFrameState pointers.
	for (DeclareTimer::instance_iter it = DeclareTimer::beginInstances(); it != DeclareTimer::endInstances(); ++it)
	{
		// update cached pointer
		it->mFrameState = &it->mTimer.getFrameState();
	}

	// Update CurTimerData::mFrameState and LLFastTimer::mFrameState of timers on the stack.
	FrameState& root_frame_state(NamedTimerFactory::instance().getRootFrameState());	// This one is not invalidated.
	CurTimerData* cur_timer_data = &LLFastTimer::sCurTimerData;
	// If the the following condition holds then cur_timer_data->mCurTimer == mAppTimer and
	// we can stop since mAppTimer->mFrameState is allocated with new and does not invalidate.
	while(cur_timer_data->mFrameState != &root_frame_state)
	{
		cur_timer_data->mFrameState = cur_timer_data->mCurTimer->mFrameState = &cur_timer_data->mNamedTimer->getFrameState();
		cur_timer_data = &cur_timer_data->mCurTimer->mLastTimerData;
	}

	// Update FrameState::mParent
	info_list_t& frame_state_list(getFrameStateList());
	FrameState* const vector_start = &*frame_state_list.begin();
	int const vector_size = frame_state_list.size();
	FrameState const* const old_vector_start = root_frame_state.mParent;
	if (vector_start != old_vector_start)
	{
		// Vector was moved; if it was sorted then FrameState::mParent will get fixed after returning from this function (see LLFastTimer::NamedTimer::resetFrame).
		root_frame_state.mParent = vector_start;
		ptrdiff_t offset = vector_start - old_vector_start;
		llassert(frame_state_list[vector_size - 1].mParent == vector_start);		// The one that was added at the end is already OK.
		for (int i = 2; i < vector_size - 1; ++i)
		{
			FrameState*& parent = frame_state_list[i].mParent;
			if (parent != &root_frame_state)
			{
				parent += offset;
			}
		}
	}
}

// See lltimer.cpp.
#if USE_RDTSC
std::string LLFastTimer::sClockType = "rdtsc";
#elif LL_LINUX || LL_DARWIN || LL_SOLARIS
std::string LLFastTimer::sClockType = "gettimeofday";
#elif LL_WINDOWS
std::string LLFastTimer::sClockType = "QueryPerformanceCounter";
#else
#error "Platform not supported"
#endif

//static
U64 LLFastTimer::countsPerSecond() // counts per second for the *32-bit* timer
{
#if USE_RDTSC
	static U64 sCPUClockFrequency = U64(LLProcessorInfo().getCPUFrequency()*1000000.0);
#else
	static bool firstcall = true;
	static U64 sCPUClockFrequency;
	if (firstcall)
	{
		sCPUClockFrequency = calc_clock_frequency();
		firstcall = false;
	}
#endif
	return sCPUClockFrequency >> 8;
}

LLFastTimer::FrameState::FrameState(LLFastTimer::NamedTimer* timerp)
:	mActiveCount(0),
	mCalls(0),
	mSelfTimeCounter(0),
	mParent(NULL),
	mLastCaller(NULL),
	mMoveUpTree(false),
	mTimer(timerp)
{}


LLFastTimer::NamedTimer::NamedTimer(const std::string& name)
:	mName(name),
	mCollapsed(true),
	mParent(NULL),
	mTotalTimeCounter(0),
	mCountAverage(0),
	mCallAverage(0),
	mNeedsSorting(false)
{
	info_list_t& frame_state_list = getFrameStateList();
	mFrameStateIndex = frame_state_list.size();
	getFrameStateList().push_back(FrameState(this));

	mCountHistory.resize(HISTORY_NUM);
	mCallHistory.resize(HISTORY_NUM);
}

LLFastTimer::NamedTimer::~NamedTimer()
{
}

std::string LLFastTimer::NamedTimer::getToolTip(S32 history_idx)
{
	F64 ms_multiplier = 1000.0 / (F64)LLFastTimer::countsPerSecond();
	if (history_idx < 0)
	{
		// by default, show average number of call
		return llformat("%s (%.2f ms, %d calls)", getName().c_str(), (F32)((F32)getCountAverage() * ms_multiplier), (S32)getCallAverage());
	}
	else
	{
		return llformat("%s (%.2f ms, %d calls)", getName().c_str(), (F32)((F32)getHistoricalCount(history_idx) * ms_multiplier), (S32)getHistoricalCalls(history_idx));
	}
}

void LLFastTimer::NamedTimer::setParent(NamedTimer* parent)
{
	llassert_always(parent != this);
	llassert_always(parent != NULL);

	if (mParent)
	{
		// subtract our accumulated from previous parent
		for (S32 i = 0; i < HISTORY_NUM; i++)
		{
			mParent->mCountHistory[i] -= mCountHistory[i];
		}

		// subtract average timing from previous parent
		mParent->mCountAverage -= mCountAverage;

		std::vector<NamedTimer*>& children = mParent->getChildren();
		std::vector<NamedTimer*>::iterator found_it = std::find(children.begin(), children.end(), this);
		if (found_it != children.end())
		{
			children.erase(found_it);
		}
	}

	mParent = parent;
	if (parent)
	{
		getFrameState().mParent = &parent->getFrameState();
		parent->getChildren().push_back(this);
		parent->mNeedsSorting = true;
	}
}

S32 LLFastTimer::NamedTimer::getDepth()
{
	S32 depth = 0;
	NamedTimer* timerp = mParent;
	while(timerp)
	{
		depth++;
		timerp = timerp->mParent;
	}
	return depth;
}

// static
void LLFastTimer::NamedTimer::processTimes()
{
	if (sCurFrameIndex < 0) return;

	buildHierarchy();
	accumulateTimings();
}

// sort timer info structs by depth first traversal order
struct SortTimersDFS
{
	bool operator()(const LLFastTimer::FrameState& i1, const LLFastTimer::FrameState& i2)
	{
		return i1.mTimer->getFrameStateIndex() < i2.mTimer->getFrameStateIndex();
	}
};

// sort child timers by name
struct SortTimerByName
{
	bool operator()(const LLFastTimer::NamedTimer* i1, const LLFastTimer::NamedTimer* i2)
	{
		return i1->getName() < i2->getName();
	}
};

//static
void LLFastTimer::NamedTimer::buildHierarchy()
{
	if (sCurFrameIndex < 0 ) return;

	// set up initial tree
	{
		for (instance_iter it = beginInstances(); it != endInstances(); ++it)
		{
			NamedTimer& timer = *it;
			if (&timer == NamedTimerFactory::instance().getRootTimer()) continue;
			
			// bootstrap tree construction by attaching to last timer to be on stack
			// when this timer was called
			FrameState& frame_state(timer.getFrameState());
			if (frame_state.mLastCaller && timer.mParent == NamedTimerFactory::instance().getRootTimer())
			{
				timer.setParent(frame_state.mLastCaller);
				// no need to push up tree on first use, flag can be set spuriously
				frame_state.mMoveUpTree = false;
			}
		}
	}

	// bump timers up tree if they've been flagged as being in the wrong place
	// do this in a bottom up order to promote descendants first before promoting ancestors
	// this preserves partial order derived from current frame's observations
	for(timer_tree_bottom_up_iterator_t it = begin_timer_tree_bottom_up(*NamedTimerFactory::instance().getRootTimer());
		it != end_timer_tree_bottom_up();
		++it)
	{
		NamedTimer* timerp = *it;
		// skip root timer
		if (timerp == NamedTimerFactory::instance().getRootTimer()) continue;

		if (timerp->getFrameState().mMoveUpTree)
		{
			// since ancestors have already been visited, reparenting won't affect tree traversal
			//step up tree, bringing our descendants with us
			//llinfos << "Moving " << timerp->getName() << " from child of " << timerp->getParent()->getName() <<
			//	" to child of " << timerp->getParent()->getParent()->getName() << llendl;
			timerp->setParent(timerp->getParent()->getParent());
			timerp->getFrameState().mMoveUpTree = false;

			// don't bubble up any ancestors until descendants are done bubbling up
			it.skipAncestors();
		}
	}

	// sort timers by time last called, so call graph makes sense
	for(timer_tree_dfs_iterator_t it = begin_timer_tree(*NamedTimerFactory::instance().getRootTimer());
		it != end_timer_tree();
		++it)
	{
		NamedTimer* timerp = (*it);
		if (timerp->mNeedsSorting)
		{
			std::sort(timerp->getChildren().begin(), timerp->getChildren().end(), SortTimerByName());
		}
		timerp->mNeedsSorting = false;
	}
}

//static
void LLFastTimer::NamedTimer::accumulateTimings()
{
	U32 cur_time = getCPUClockCount32();

	// walk up stack of active timers and accumulate current time while leaving timing structures active
	LLFastTimer* cur_timer = sCurTimerData.mCurTimer;
	// root defined by parent pointing to self
	CurTimerData* cur_data = &sCurTimerData;
	while(cur_timer->mLastTimerData.mCurTimer != cur_timer)
	{
		U32 cumulative_time_delta = cur_time - cur_timer->mStartTime;
		U32 self_time_delta = cumulative_time_delta - cur_data->mChildTime;
		cur_data->mChildTime = 0;
		cur_timer->mFrameState->mSelfTimeCounter += self_time_delta;
		cur_timer->mStartTime = cur_time;

		cur_data = &cur_timer->mLastTimerData;
		cur_data->mChildTime += cumulative_time_delta;

		cur_timer = cur_timer->mLastTimerData.mCurTimer;
	}

	// traverse tree in DFS post order, or bottom up
	for(timer_tree_bottom_up_iterator_t it = begin_timer_tree_bottom_up(*NamedTimerFactory::instance().getActiveRootTimer());
		it != end_timer_tree_bottom_up();
		++it)
	{
		NamedTimer* timerp = (*it);
		timerp->mTotalTimeCounter = timerp->getFrameState().mSelfTimeCounter;
		for (child_const_iter child_it = timerp->beginChildren(); child_it != timerp->endChildren(); ++child_it)
		{
			timerp->mTotalTimeCounter += (*child_it)->mTotalTimeCounter;
		}

		S32 cur_frame = sCurFrameIndex;
		if (cur_frame >= 0)
		{
			// update timer history
			int hidx = cur_frame % HISTORY_NUM;

			int weight = llmin(100, cur_frame);

			timerp->mCountHistory[hidx] = timerp->mTotalTimeCounter;
			timerp->mCountAverage = ((F64)timerp->mCountAverage * weight + (F64)timerp->mTotalTimeCounter) / (weight+1);
			timerp->mCallHistory[hidx] = timerp->getFrameState().mCalls;
			timerp->mCallAverage = ((F64)timerp->mCallAverage * weight + (F64)timerp->getFrameState().mCalls) / (weight+1);
		}
	}
}

U32 LLFastTimer::NamedTimer::getCountAverage() const 
{
	return mCountAverage;// (sCurFrameIndex <= 0 || mCountAverage <= 0)  ? 0 : mCountAverage / llmin(sCurFrameIndex,(S32)HISTORY_NUM);
}
U32 LLFastTimer::NamedTimer::getCallAverage() const 
{ 
	return mCallAverage;// (sCurFrameIndex <= 0 || mCallAverage <= 0)  ? 0 : mCallAverage / llmin(sCurFrameIndex,(S32)HISTORY_NUM);
}

// static
void LLFastTimer::NamedTimer::resetFrame()
{
	if (sLog)
	{ //output current frame counts to performance log

		static S32 call_count = 0;
		if (call_count % 100 == 0)
		{
			llinfos << "countsPerSecond (32 bit): " << countsPerSecond() << llendl;
			llinfos << "get_clock_count (64 bit): " << get_clock_count() << llendl;
			llinfos << "LLProcessorInfo().getCPUFrequency() " << LLProcessorInfo().getCPUFrequency() << llendl;
			llinfos << "getCPUClockCount32() " << getCPUClockCount32() << llendl;
			llinfos << "getCPUClockCount64() " << getCPUClockCount64() << llendl;
			llinfos << "elapsed sec " << ((F64)getCPUClockCount64())/((F64)LLProcessorInfo().getCPUFrequency()*1000000.0) << llendl;
		}
		call_count++;

		F64 iclock_freq = 1000.0 / countsPerSecond(); // good place to calculate clock frequency

		F64 total_time = 0;
		LLSD sd;

		{
			for (instance_iter it = beginInstances(); it != endInstances(); ++it)
			{
				NamedTimer& timer = *it;
				FrameState& info = timer.getFrameState();
				sd[timer.getName()]["Time"] = (LLSD::Real) (info.mSelfTimeCounter*iclock_freq);	
				sd[timer.getName()]["Calls"] = (LLSD::Integer) info.mCalls;
				
				// computing total time here because getting the root timer's getCountHistory
				// doesn't work correctly on the first frame
				total_time = total_time + info.mSelfTimeCounter * iclock_freq;
			}
		}

		sd["Total"]["Time"] = (LLSD::Real) total_time;
		sd["Total"]["Calls"] = (LLSD::Integer) 1;

		{		
			LLMutexLock lock(sLogLock);
			sLogQueue.push(sd);
		}
	}


	// tag timers by position in depth first traversal of tree
	S32 index = 0;
	for(timer_tree_dfs_iterator_t it = begin_timer_tree(*NamedTimerFactory::instance().getRootTimer());
		it != end_timer_tree();
		++it)
	{
		NamedTimer* timerp = (*it);
		
		timerp->mFrameStateIndex = index;
		index++;
	}
	llassert(index == (S32)getFrameStateList().size());

	// sort timers by DFS traversal order to improve cache coherency
	std::sort(getFrameStateList().begin(), getFrameStateList().end(), SortTimersDFS());

	// update pointers into framestatelist now that we've sorted it
	updateCachedPointers();

	// reset for next frame
	{
		for (instance_iter it = beginInstances(); it != endInstances(); ++it)
		{
			NamedTimer& timer = *it;
			
			FrameState& info = timer.getFrameState();
			info.mSelfTimeCounter = 0;
			info.mCalls = 0;
			info.mLastCaller = NULL;
			info.mMoveUpTree = false;
			// update parent pointer in timer state struct
			if (timer.mParent)
			{
				info.mParent = &timer.mParent->getFrameState();
			}
		}
	}

	//sTimerCycles = 0;
	//sTimerCalls = 0;
}

//static
void LLFastTimer::NamedTimer::reset()
{
	resetFrame(); // reset frame data

	// walk up stack of active timers and reset start times to current time
	// effectively zeroing out any accumulated time
	U32 cur_time = getCPUClockCount32();

	// root defined by parent pointing to self
	CurTimerData* cur_data = &sCurTimerData;
	LLFastTimer* cur_timer = cur_data->mCurTimer;
	while(cur_timer->mLastTimerData.mCurTimer != cur_timer)
	{
		cur_timer->mStartTime = cur_time;
		cur_data->mChildTime = 0;

		cur_data = &cur_timer->mLastTimerData;
		cur_timer = cur_data->mCurTimer;
	}

	// reset all history
	{
		for (instance_iter it = beginInstances(); it != endInstances(); ++it)
		{
			NamedTimer& timer = *it;
			if (&timer != NamedTimerFactory::instance().getRootTimer()) 
			{
				timer.setParent(NamedTimerFactory::instance().getRootTimer());
			}
			
			timer.mCountAverage = 0;
			timer.mCallAverage = 0;
			timer.mCountHistory.clear();
			timer.mCountHistory.resize(HISTORY_NUM);
			timer.mCallHistory.clear();
			timer.mCallHistory.resize(HISTORY_NUM);
		}
	}

	sLastFrameIndex = 0;
	sCurFrameIndex = 0;
}

//static 
LLFastTimer::info_list_t& LLFastTimer::getFrameStateList() 
{ 
	if (!sTimerInfos) 
	{ 
		sTimerInfos = new info_list_t;
#if 0
		// Avoid the vector being moved in memory by reserving enough memory right away.
		sTimerInfos->reserve(1024);
#endif
	} 
	return *sTimerInfos; 
}


U32 LLFastTimer::NamedTimer::getHistoricalCount(S32 history_index) const
{
	S32 history_idx = (getLastFrameIndex() + history_index) % LLFastTimer::NamedTimer::HISTORY_NUM;
	return mCountHistory[history_idx];
}

U32 LLFastTimer::NamedTimer::getHistoricalCalls(S32 history_index ) const
{
	S32 history_idx = (getLastFrameIndex() + history_index) % LLFastTimer::NamedTimer::HISTORY_NUM;
	return mCallHistory[history_idx];
}

LLFastTimer::FrameState& LLFastTimer::NamedTimer::getFrameState() const
{
	llassert_always(mFrameStateIndex >= 0);
	if (this == NamedTimerFactory::instance().getActiveRootTimer()) 
	{
		return NamedTimerFactory::instance().getRootFrameState();
	}
	return getFrameStateList()[mFrameStateIndex];
}

// static
LLFastTimer::NamedTimer& LLFastTimer::NamedTimer::getRootNamedTimer()
{ 
	return *NamedTimerFactory::instance().getActiveRootTimer(); 
}

std::vector<LLFastTimer::NamedTimer*>::const_iterator LLFastTimer::NamedTimer::beginChildren()
{ 
	return mChildren.begin(); 
}

std::vector<LLFastTimer::NamedTimer*>::const_iterator LLFastTimer::NamedTimer::endChildren()
{
	return mChildren.end();
}

std::vector<LLFastTimer::NamedTimer*>& LLFastTimer::NamedTimer::getChildren()
{
	return mChildren;
}

//static
void LLFastTimer::nextFrame()
{
	countsPerSecond(); // good place to calculate clock frequency
	U64 frame_time = getCPUClockCount64();
	if ((frame_time - sLastFrameTime) >> 8 > 0xffffffff)
	{
		llinfos << "Slow frame, fast timers inaccurate" << llendl;
	}

	if (!sPauseHistory)
	{
		NamedTimer::processTimes();
		sLastFrameIndex = sCurFrameIndex;
		++sCurFrameIndex;
	}
	
	// get ready for next frame
	NamedTimer::resetFrame();
	sLastFrameTime = frame_time;
}

//static
void LLFastTimer::dumpCurTimes()
{
	// accumulate timings, etc.
	NamedTimer::processTimes();
	
	F64 clock_freq = (F64)countsPerSecond();
	F64 iclock_freq = 1000.0 / clock_freq; // clock_ticks -> milliseconds

	// walk over timers in depth order and output timings
	for(timer_tree_dfs_iterator_t it = begin_timer_tree(*NamedTimerFactory::instance().getRootTimer());
		it != end_timer_tree();
		++it)
	{
		NamedTimer* timerp = (*it);
		F64 total_time_ms = ((F64)timerp->getHistoricalCount(0) * iclock_freq);
		// Don't bother with really brief times, keep output concise
		if (total_time_ms < 0.1) continue;

		std::ostringstream out_str;
		for (S32 i = 0; i < timerp->getDepth(); i++)
		{
			out_str << "\t";
		}


		out_str << timerp->getName() << " " 
			<< std::setprecision(3) << total_time_ms << " ms, "
			<< timerp->getHistoricalCalls(0) << " calls";

		llinfos << out_str.str() << llendl;
	}
}

//static 
void LLFastTimer::reset()
{
	NamedTimer::reset();
}


//static
void LLFastTimer::writeLog(std::ostream& os)
{
	while (!sLogQueue.empty())
	{
		LLSD& sd = sLogQueue.front();
		LLSDSerialize::toXML(sd, os);
		LLMutexLock lock(sLogLock);
		sLogQueue.pop();
	}
}

//static
const LLFastTimer::NamedTimer* LLFastTimer::getTimerByName(const std::string& name)
{
	return NamedTimerFactory::instance().getTimerByName(name);
}

LLFastTimer::LLFastTimer(LLFastTimer::FrameState* state)
:	mFrameState(state)
{
	// Only called for mAppTimer with mRootFrameState, which never invalidates.
	llassert(state == &NamedTimerFactory::instance().getRootFrameState());

	U32 start_time = getCPUClockCount32();
	mStartTime = start_time;
	mFrameState->mActiveCount++;
	LLFastTimer::sCurTimerData.mCurTimer = this;
	LLFastTimer::sCurTimerData.mNamedTimer = mFrameState->mTimer;
	LLFastTimer::sCurTimerData.mFrameState = mFrameState;
	LLFastTimer::sCurTimerData.mChildTime = 0;
	// This is the root FastTimer (mAppTimer), mark it as such by having
	// mLastTimerData be equal to sCurTimerData (which is a rather arbitrary
	// and not very logical way to do that --Aleric).
	mLastTimerData = LLFastTimer::sCurTimerData;
}

//////////////////////////////////////////////////////////////////////////////
//
// Important note: These implementations must be FAST!
//

#if USE_RDTSC
U32 LLFastTimer::getCPUClockCount32()
{
	U32 ret_val;
	__asm
	{
        _emit   0x0f
        _emit   0x31
		shr eax,8
		shl edx,24
		or eax, edx
		mov dword ptr [ret_val], eax
	}
    return ret_val;
}

// return full timer value, *not* shifted by 8 bits
U64 LLFastTimer::getCPUClockCount64()
{
	U64 ret_val;
	__asm
	{
        _emit   0x0f
        _emit   0x31
		mov eax,eax
		mov edx,edx
		mov dword ptr [ret_val+4], edx
		mov dword ptr [ret_val], eax
	}
    return ret_val;
}
#else
//LL_COMMON_API U64 get_clock_count(); // in lltimer.cpp
// These use QueryPerformanceCounter, which is arguably fine and also works on AMD architectures.
U32 LLFastTimer::getCPUClockCount32()
{
	return (U32)(get_clock_count()>>8);
}

U64 LLFastTimer::getCPUClockCount64()
{
	return get_clock_count();
}
#endif
