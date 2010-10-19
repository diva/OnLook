#if SHY_MOD //Command handler
#include "llviewerobject.h"
#include "llagentdata.h"
#include "llcontrol.h"

class SHCommandHandler
{
public:
	typedef void(*command_fn_t)(const SHCommandHandler *pCmd, const LLSD &args, const std::string &full_str, const LLUUID &callerid, LLViewerObject *pCaller);
private:	
	//Dispatch handled through a function pointer because doing it via class deriviation requires constructors each time
	//	which makes macroing ugly.
	command_fn_t mDispatchFn;

	//Static initialization fiasco avoidance
	static std::list<SHCommandHandler*> &getScriptCommandList();
	static std::list<SHCommandHandler*> &getChatCommandList();
public:

	//ctor/dtors
	SHCommandHandler(bool from_chat, command_fn_t callback);

//Virtuals:
	//Virtual destructors in abstract classes are a good thing.
	virtual ~SHCommandHandler() {}	

	//Must override in derived classes.
	virtual const std::string &getName() const = 0; 

	//Call the function pointed to if set.
	virtual void Dispatch(const LLSD &args, const std::string &full_str, const LLUUID &callerid, LLViewerObject *pCaller) //Overrideable
	{
		if(mDispatchFn)
			(mDispatchFn)(this,args,full_str,callerid,pCaller);
	}

//Statics:
	//Iterate through commands and run Dispatch on any matches.
	static bool handleCommand(bool from_chat, const std::string &full_str, const LLUUID &callerid, LLViewerObject *pCaller);

	//Self-explanatory
	static void send_chat_to_object(const std::string &message, int channel, const LLUUID &target_id = gAgentID);
	static void send_chat_to_object(const std::string &message, int channel, LLViewerObject *pTargetObject);
};

class SHDynamicCommand : public SHCommandHandler
{
private:
	LLCachedControl<std::string> *pControl;
public:

	//ctor/dtors
	SHDynamicCommand(bool from_chat, const std::string &str, command_fn_t callback, LLControlGroup *pGroup)
		: SHCommandHandler(from_chat,callback), pControl(new LLCachedControl<std::string>(str,""))
		{}
	SHDynamicCommand(bool from_chat, const std::string &str, command_fn_t callback, LLControlGroup &pGroup = gSavedSettings)
		: SHCommandHandler(from_chat,callback), pControl(new LLCachedControl<std::string>(str,""))
		{}

//Virtuals:
	virtual ~SHDynamicCommand()
	{
		if(pControl)
			delete pControl;
	}
	virtual const std::string &getName() const
	{
		static const std::string empty_str;
		return pControl ? pControl->get() : empty_str;
	}
};

class SHCommand : public SHCommandHandler
{
private:
	const std::string mCommandName;
public:
	SHCommand(bool from_chat, const std::string &str, command_fn_t callback)
		: SHCommandHandler(from_chat,callback), mCommandName(str)
	{}
	virtual const std::string &getName() const
	{
		return mCommandName;
	}
};

#define SETUP_SETTING_CMD(type, varname, group, chat) \
	void cmd_dispatch_##type##_##varname(const SHCommandHandler *pCmd, const LLSD &args, const std::string &full_str, const LLUUID &callerid, LLViewerObject *pCaller); \
	SHDynamicCommand cmd_cls_##type##_##varname(chat,#varname,&cmd_dispatch_##type##_##varname,group); \
	void cmd_dispatch_##type##_##varname(const SHCommandHandler *pCmd, const LLSD &args, const std::string &full_str, const LLUUID &callerid, LLViewerObject *pCaller)

#define SETUP_STATIC_CMD(type, varname, chat) \
	void cmd_dispatch_##type##_##varname(const SHCommandHandler *pCmd, const LLSD &args, const std::string &full_str, const LLUUID &callerid, LLViewerObject *pCaller); \
	SHCommand cmd_cls_##type##_##varname(chat,#varname,&cmd_dispatch_##type##_##varname); \
	void cmd_dispatch_##type##_##varname(const SHCommandHandler *pCmd, const LLSD &args, const std::string &full_str, const LLUUID &callerid, LLViewerObject *pCaller )

//Specify control group
#define CMD_SCRIPT_SETTING(varname,group)	SETUP_SETTING_CMD(script,varname,group,false)
#define CMD_CHAT_SETTING(varname,group)		SETUP_SETTING_CMD(chat,varname,group,true)

//command name is static
#define CMD_SCRIPT(varname)					SETUP_STATIC_CMD(script,varname,false)
#define CMD_CHAT(varname)					SETUP_STATIC_CMD(chat,varname,true)

#endif //shy_mod
