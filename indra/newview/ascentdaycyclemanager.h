/** 
 * @file ascentdaycyclemanager.h
 * @Author Duncan Garrett
 * Manager for Windlight Daycycles so we can actually save more than one
 *
 * Created October 04 2010
 * 
 * ALL SOURCE CODE IS PROVIDED "AS IS." THE CREATOR MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * k ilu bye
 */

#ifndef ASCENT_DAYCYCLEMANAGER_H
#define ASCENT_DAYCYCLEMANAGER_H

#include <vector>
#include <map>
#include "llwldaycycle.h"
#include "llwlanimator.h"
#include "llwldaycycle.h"
#include "llviewercamera.h"

/// WindLight parameter manager class - what controls all the wind light shaders
class AscentDayCycleManager
{
public:

	AscentDayCycleManager();
	~AscentDayCycleManager();

	/// load a preset file
	void loadPresets(const std::string & fileName);

	/// save the preset file
	void savePresets(const std::string & fileName);

	/// load an individual preset into the sky
	void loadPreset(const std::string & name,bool propogate=true);

	/// save the parameter presets to file
	void savePreset(const std::string & name);

	/// Set shader uniforms dirty, so they'll update automatically.
	void propagateParameters(void);
	
	/// Update shader uniforms that have changed.
	void updateShaderUniforms(LLGLSLShader * shader);

	/// setup the animator to run
	void resetAnimator(F32 curTime, bool run);

	/// update information camera dependent parameters
	void update(LLViewerCamera * cam);


	/// Perform global initialization for this class.
	static void initClass(void);

	// Cleanup of global data that's only inited once per class.
	static void cleanupClass();
	
	/// add a param to the list
	bool addParamSet(const std::string& name, LLWLDayCycle& param);

	/// add a param to the list
	BOOL addParamSet(const std::string& name, LLSD const & param);

	/// get a param from the list
	bool getParamSet(const std::string& name, LLWLDayCycle& param);

	/// set the param in the list with a new param
	bool setParamSet(const std::string& name, LLWLDayCycle& param);
	
	/// set the param in the list with a new param
	bool setParamSet(const std::string& name, LLSD const & param);	

	/// gets rid of a parameter and any references to it
	/// returns true if successful
	bool removeParamSet(const std::string& name, bool delete_from_disk);

	// singleton pattern implementation
	static AscentDayCycleManager * instance();

public:

	// helper variables
	LLWLAnimator mAnimator;


	// list of params and how they're cycled for days
	LLWLDayCycle mDay;

	LLWLDayCycle mCurParams;

	/// Sun Delta Terrain tweak variables.
	F32 mSunDeltaYaw;
	
	// list of all the day cycles, listed by name
	std::map<std::string, LLWLDayCycle> mParamList;
	
	
private:
	// our parameter manager singleton instance
	static AscentDayCycleManager * sInstance;

};

#endif
