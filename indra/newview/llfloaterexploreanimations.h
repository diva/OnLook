// <edit>
#ifndef LL_LLFLOATEREXPLOREANIMATIONS_H
#define LL_LLFLOATEREXPLOREANIMATIONS_H

#include "llfloaterbvhpreview.h"
#include "llinstancetracker.h"

class LLAnimHistoryItem
{
public:
	LLAnimHistoryItem(LLUUID assetid = LLUUID(), bool playing = true);
	bool setPlaying(bool playing);

	LLUUID mAssetID;
	bool mPlaying;
	F64 mTimeStarted;
	F64 mTimeStopped;
};

class LLFloaterExploreAnimations
: public LLFloater, public LLInstanceTracker<LLFloaterExploreAnimations, LLUUID>
{
public:
	static void show();
	LLFloaterExploreAnimations(LLUUID avatarid);
	BOOL postBuild();

	void update();

private:
	virtual ~LLFloaterExploreAnimations();

public:
	void onSelectAnimation();

	BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	BOOL handleHover(S32 x, S32 y, MASK mask);
	BOOL handleScrollWheel(S32 x, S32 y, S32 clicks); 
	void onMouseCaptureLost();

// static stuff!
	static void processAnim(LLUUID avatarid, LLUUID assetid, bool playing);

	static std::map< LLUUID, std::list< LLAnimHistoryItem > > animHistory;
private:
	static void handleHistoryChange(LLUUID avatarid);

protected:
	void			draw();

	LL_ALIGN_16(LLPreviewAnimation mAnimPreview);
	LLRect				mPreviewRect;
	S32					mLastMouseX;
	S32					mLastMouseY;
};

#endif
// </edit>
