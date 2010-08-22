/** 
 * @file importtracker.h
 * @brief A utility for importing linksets from XML.
 * Discrete wuz here
 */

#ifndef IMPORTTRACKER_H
#define IMPORTTRACKER_H

#include "llviewerobject.h"



class ImportTracker
{
	public:
		enum ImportState { IDLE, WAND, BUILDING, LINKING, POSITIONING };			
		
		ImportTracker()
		: numberExpected(0),
		state(IDLE),
		last(0),
		groupcounter(0),
		updated(0)
		{ }
		ImportTracker(LLSD &data) { state = IDLE; linkset = data; numberExpected=0;}
		~ImportTracker() { localids.clear(); linkset.clear(); }
	
		//Chalice - support import of linkset groups
		void importer(std::string file, void (*callback)(LLViewerObject*));
		void cleargroups();
		void import(LLSD &ls_data);
		void expectRez();
		void clear();
		void finish();
		void cleanUp();
		void get_update(S32 newid, BOOL justCreated = false, BOOL createSelected = false);
		
		const int getState() { return state; }

		U32 asset_insertions;
		
	protected:		
		void send_inventory(LLSD &prim);
		void send_properties(LLSD &prim, int counter);
		void send_vectors(LLSD &prim, int counter);
		void send_shape(LLSD &prim);
		void send_image(LLSD &prim);
		void send_extras(LLSD &prim);
		void send_namedesc(LLSD &prim);
		void link();
		void wear(LLSD &prim);
		void position(LLSD &prim);
		void plywood_above_head();
	
	private:
		int				numberExpected;
		int				state;
		S32				last;
		LLVector3			root;
		LLQuaternion		rootrot;
		std::list<S32>			localids;
		LLSD				linksetgroups;
		int					groupcounter;
		int					updated;
		LLVector3			linksetoffset;
		LLVector3			initialPos;
		LLSD				linkset;

		std::string filepath;
		std::string asset_dir;
		void	(*mDownCallback)(LLViewerObject*);

		U32 lastrootid;
};

extern ImportTracker gImportTracker;

//extern LLAgent gAgent;

#endif
