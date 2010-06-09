// <edit>
#ifndef LL_LLAO_H
#define LL_LLAO_H

#include "llfloater.h"

class LLAO
{
public:
	static std::map<LLUUID,LLUUID> mOverrides;

	static void refresh();
};

class LLFloaterAO : public LLFloater
{
public:
	static LLFloaterAO* sInstance;
	static void show();
	LLFloaterAO();
	BOOL postBuild(void);
	void refresh();
	static void onCommitAnim(LLUICtrl* ctrl, void* user_data);
	static void onClickSave(void* user_data);
	static void onClickLoad(void* user_data);
private:
	virtual ~LLFloaterAO();
	std::string idstr(LLUUID id); // silly utility
};

#endif
// </edit>
