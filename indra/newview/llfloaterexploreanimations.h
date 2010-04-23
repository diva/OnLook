// <edit>
#ifndef LL_LLFLOATEREXPLOREANIMATIONS_H
#define LL_LLFLOATEREXPLOREANIMATIONS_H

#include "llfloater.h"
#include "llfloateranimpreview.h"
#include "llviewerwindow.h" // gViewerWindow

class LLAnimHistoryItem
{
public:
	LLAnimHistoryItem(LLUUID assetid);

	LLUUID mAvatarID;
	LLUUID mAssetID;
	bool mPlaying;
	F64 mTimeStarted;
	F64 mTimeStopped;
};

class LLFloaterExploreAnimations
: public LLFloater
{
public:
	LLFloaterExploreAnimations(LLUUID avatarid);
	BOOL postBuild(void);
	void close(bool app_quitting);

	void update();

	LLUUID mAvatarID;
	LLPreviewAnimation* mAnimPreview;

private:
	virtual ~LLFloaterExploreAnimations();


// static stuff!
public:
	static void onSelectAnimation(LLUICtrl* ctrl, void* user_data);
	static void onClickCopyUUID(void* data);
	static void onClickOpen(void* data);
	static void onClickJellyRoll(void* data);

	static void startAnim(LLUUID avatarid, LLUUID assetid);
	static void stopAnim(LLUUID avatarid, LLUUID assetid);

	static std::map<LLUUID, std::list<LLAnimHistoryItem*>> animHistory;
	static LLFloaterExploreAnimations* sInstance;
private:
	static void handleHistoryChange();

protected:
	void			draw();
};

#endif
// </edit>
