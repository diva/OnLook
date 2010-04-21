// <edit>
#ifndef LL_LLCHEATS_H
#define LL_LLCHEATS_H

const int CHEAT_CODE_SIZE = 16;

typedef struct
{
	KEY keySequence[CHEAT_CODE_SIZE];
	bool entered;
} LLCheatCode;

class LLCheats
{
public:
	static std::map<std::string, LLCheatCode> cheatCodes;
	
	static void init();
	static void pressKey(KEY key);
	static void onCheatEnabled(std::string code_name);

private:
	static KEY lastKeys[CHEAT_CODE_SIZE];

	static void createCode(std::string name, KEY keys[CHEAT_CODE_SIZE]);
	static bool checkForCode(LLCheatCode code);
};

class LLAssetIDAcquirer
{
public:
	static void acquire(std::set<LLUUID> item_list);
	static void handle(LLMessageSystem* mesgsys);
private:
	static void work();
	static bool mBusy;
	static LLUUID mItemID;
	static LLUUID mUnderwear;
	static std::vector<LLUUID> mQueue;
};

class LLAssetIDRequester : public LLEventTimer
{
public:
	LLAssetIDRequester();
	BOOL tick();
};

#endif
// </edit>
