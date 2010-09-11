// <edit>
#ifndef LL_LLAO_H
#define LL_LLAO_H

//this is for debugging ;D
//#define AO_DEBUG

#include "llfloater.h"
#include "llcombobox.h"

class LLAOStandTimer : public LLEventTimer
{
public:
	LLAOStandTimer(F32 period);
	BOOL tick();
	void pause();
	void resume();
	void reset();

private:
	BOOL mPaused;
};

class LLAO
{
public:
	static void setup();
	static std::map<LLUUID,LLUUID> mOverrides;
	static std::list<std::string> mStandOverrides;
	static std::list<std::string> mWalkOverrides;
	static std::list<std::string> mRunOverrides;
	static std::list<std::string> mCrouchwalkOverrides;
	static std::list<std::string> mFlyOverrides;
	static std::list<std::string> mTurnLeftOverrides;
	static std::list<std::string> mTurnRightOverrides;
	static std::list<std::string> mJumpOverrides;
	static std::list<std::string> mFlyUpOverrides;
	static std::list<std::string> mFlyDownOverrides;
	static std::list<std::string> mCrouchOverrides;
	static std::list<std::string> mHoverOverrides;
	static std::list<std::string> mSitOverrides;
	static std::list<std::string> mPreJumpOverrides;
	static std::list<std::string> mFallOverrides;
	static std::list<std::string> mStrideOverrides;
	static std::list<std::string> mSoftLandOverrides;
	static std::list<std::string> mMediumLandOverrides;
	static std::list<std::string> mHardLandOverrides;
	static std::list<std::string> mSlowFlyOverrides;
	static std::list<std::string> mGroundSitOverrides;

	static BOOL isEnabled(){ return mEnabled; }
	static BOOL isStand(LLUUID _id);
	static void refresh();
	static void runAnims(BOOL enabled);
	static bool handleAOEnabledChanged(const LLSD& newvalue);
	static bool handleAOPeriodChanged(const LLSD& newvalue);
	static const LLUUID& getAssetIDByName(const std::string& name);
	static std::list<std::string>* getOverrideList(const std::string name);
	static void pushTo(std::string name, std::string value);
	static LLUUID getFrontUUID();
	static LLUUID getBackUUID();
	static LLAOStandTimer* mTimer;

	//Horribly hacked-up stuff from llpreviewgesture.h, try to fix in the near future. -HgB
	/*virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 std::string& tooltip_msg);*/

private:
	static BOOL mEnabled;
	static F32 mPeriod;
};

class LLFloaterAO : public LLFloater
{
public:
	static LLFloaterAO* sInstance;
	static void show();
	LLFloaterAO();
	void addAnimations();
	BOOL postBuild(void);
	void refresh();
	static void onCommitAnim(LLUICtrl* ctrl, void* user_data);
	static void onCommitType(LLUICtrl* ctrl,void* user_data);
	static void onClickAnimRemove(void* user_data);
	static void onClickAnimAdd(void* user_data);
	static void onClickSave(void* user_data);
	static void onClickLoad(void* user_data);
	
private:
	LLComboBox*			mAnimListCombo;
	LLComboBox*			mAnimTypeCombo;
	LLScrollListCtrl*	mAnimationList;
	std::string			mCurrentAnimType;
	virtual ~LLFloaterAO();
protected:
	static void onCommitAnimation(LLUICtrl* ctrl, void* data);
};

#endif
// </edit>
