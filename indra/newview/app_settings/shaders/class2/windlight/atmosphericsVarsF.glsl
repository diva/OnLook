/** 
 * @file atmosphericVars.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
 


VARYING vec3 vary_PositionEye;

VARYING vec3 vary_SunlitColor;
VARYING vec3 vary_AmblitColor;
VARYING vec3 vary_AdditiveColor;
VARYING vec3 vary_AtmosAttenuation;

vec3 getPositionEye()
{
	return vary_PositionEye;
}
vec3 getSunlitColor()
{
	return vary_SunlitColor;
}
vec3 getAmblitColor()
{
	return vary_AmblitColor;
}
vec3 getAdditiveColor()
{
	return vary_AdditiveColor;
}
vec3 getAtmosAttenuation()
{
	return vary_AtmosAttenuation;
}
