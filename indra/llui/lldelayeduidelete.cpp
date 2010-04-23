// <edit>
#include "linden_common.h"
#include "lldelayeduidelete.h"
#define DELETE_DELAY 0.1f
#define DELETES_PER_DELAY 512

LLDeleteScheduler* gDeleteScheduler;

std::list<LLDeleteJob*> LLDeleteScheduler::sJobs;
LLDeleteScheduler::LLDeleteScheduler() : LLEventTimer(DELETE_DELAY)
{
}
void LLDeleteScheduler::addViewDeleteJob(std::list<LLView*> views)
{
	if(!views.empty())
	{
		LLViewDeleteJob* job = new LLViewDeleteJob(views);
		sJobs.push_back(job);
	}
}
BOOL LLDeleteScheduler::tick() // IMPORTANT: never return TRUE
{
	if(!sJobs.empty())
	{
		U32 completed = 0;
		do
		{
			LLDeleteJob* job = sJobs.front();
			if(job->work(completed))
			{
				delete job;
				sJobs.pop_front();
			}
		} while((completed < DELETES_PER_DELAY) && !sJobs.empty());
	}
	return FALSE; // EVER
}
BOOL LLDeleteJob::work(U32& completed)
{
	llwarns << "THIS IS SPOSED TO BE OVERRIDDEN" << llendl;
	return TRUE;
}
LLViewDeleteJob::LLViewDeleteJob(std::list<LLView*> views)
: mList(views)
{
}
LLViewDeleteJob::~LLViewDeleteJob()
{
}
BOOL LLViewDeleteJob::work(U32& completed)
{
	do
	{
		if(!mList.empty())
		{
			LLView* view = mList.front();
			delete view;
			mList.pop_front();
		}
		else
		{
			return TRUE; // job done
		}
	} while(++completed < DELETES_PER_DELAY);
	return FALSE;
}
// </edit>
