/** 
 * @file lllslconstants.h
 * @author James Cook
 * @brief Constants used in lsl.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLLSLCONSTANTS_H
#define LL_LLLSLCONSTANTS_H

// LSL: Return flags for llGetAgentInfo
const U32 AGENT_FLYING		= 0x0001;
const U32 AGENT_ATTACHMENTS	= 0x0002;
const U32 AGENT_SCRIPTED	= 0x0004;
const U32 AGENT_MOUSELOOK	= 0x0008;
const U32 AGENT_SITTING		= 0x0010;
const U32 AGENT_ON_OBJECT	= 0x0020;
const U32 AGENT_AWAY		= 0x0040;
const U32 AGENT_WALKING		= 0x0080;
const U32 AGENT_IN_AIR		= 0x0100;
const U32 AGENT_TYPING		= 0x0200;
const U32 AGENT_CROUCHING	= 0x0400;
const U32 AGENT_BUSY		= 0x0800;
const U32 AGENT_ALWAYS_RUN	= 0x1000;
const U32 AGENT_AUTOPILOT	= 0x2000;

const S32 LSL_REMOTE_DATA_CHANNEL		= 1;
const S32 LSL_REMOTE_DATA_REQUEST		= 2;
const S32 LSL_REMOTE_DATA_REPLY			= 3;

// Constants used in extended LSL primitive setter and getters
const S32 LSL_PRIM_TYPE_LEGACY	= 1; // No longer supported.
const S32 LSL_PRIM_MATERIAL		= 2;
const S32 LSL_PRIM_PHYSICS		= 3;
const S32 LSL_PRIM_TEMP_ON_REZ	= 4;
const S32 LSL_PRIM_PHANTOM		= 5;
const S32 LSL_PRIM_POSITION		= 6;
const S32 LSL_PRIM_SIZE			= 7;
const S32 LSL_PRIM_ROTATION		= 8;
const S32 LSL_PRIM_TYPE			= 9; // Replacement for LSL_PRIM_TYPE_LEGACY
const S32 LSL_PRIM_TEXTURE		= 17;
const S32 LSL_PRIM_COLOR		= 18;
const S32 LSL_PRIM_BUMP_SHINY	= 19;
const S32 LSL_PRIM_FULLBRIGHT	= 20;
const S32 LSL_PRIM_FLEXIBLE		= 21;
const S32 LSL_PRIM_TEXGEN		= 22;
const S32 LSL_PRIM_POINT_LIGHT	= 23;
const S32 LSL_PRIM_CAST_SHADOWS	= 24;
const S32 LSL_PRIM_GLOW     	= 25;
const S32 LSL_PRIM_TEXT			= 26;
const S32 LSL_PRIM_NAME			= 27;
const S32 LSL_PRIM_DESC			= 28;
const S32 LSL_PRIM_ROT_LOCAL	= 29;
const S32 LSL_PRIM_PHYSICS_SHAPE_TYPE = 30;
const S32 LSL_PRIM_OMEGA		= 32;
const S32 LSL_PRIM_POS_LOCAL	= 33;
const S32 LSL_PRIM_LINK_TARGET	= 34;
const S32 LSL_PRIM_SLICE	    = 35;

const S32 LSL_PRIM_TYPE_BOX		= 0;
const S32 LSL_PRIM_TYPE_CYLINDER= 1;
const S32 LSL_PRIM_TYPE_PRISM	= 2;
const S32 LSL_PRIM_TYPE_SPHERE	= 3;
const S32 LSL_PRIM_TYPE_TORUS	= 4;
const S32 LSL_PRIM_TYPE_TUBE	= 5;
const S32 LSL_PRIM_TYPE_RING	= 6;
const S32 LSL_PRIM_TYPE_SCULPT  = 7;

const S32 LSL_PRIM_HOLE_DEFAULT = 0x00;
const S32 LSL_PRIM_HOLE_CIRCLE	= 0x10;
const S32 LSL_PRIM_HOLE_SQUARE  = 0x20;
const S32 LSL_PRIM_HOLE_TRIANGLE= 0x30;

const S32 LSL_PRIM_MATERIAL_STONE	= 0;
const S32 LSL_PRIM_MATERIAL_METAL	= 1;
const S32 LSL_PRIM_MATERIAL_GLASS	= 2;
const S32 LSL_PRIM_MATERIAL_WOOD	= 3;
const S32 LSL_PRIM_MATERIAL_FLESH	= 4;
const S32 LSL_PRIM_MATERIAL_PLASTIC	= 5;
const S32 LSL_PRIM_MATERIAL_RUBBER	= 6;
const S32 LSL_PRIM_MATERIAL_LIGHT	= 7;

const S32 LSL_PRIM_SHINY_NONE		= 0;
const S32 LSL_PRIM_SHINY_LOW		= 1;
const S32 LSL_PRIM_SHINY_MEDIUM		= 2;
const S32 LSL_PRIM_SHINY_HIGH		= 3;

const S32 LSL_PRIM_TEXGEN_DEFAULT	= 0;
const S32 LSL_PRIM_TEXGEN_PLANAR	= 1;

const S32 LSL_PRIM_BUMP_NONE		= 0;
const S32 LSL_PRIM_BUMP_BRIGHT		= 1;
const S32 LSL_PRIM_BUMP_DARK		= 2;
const S32 LSL_PRIM_BUMP_WOOD		= 3;
const S32 LSL_PRIM_BUMP_BARK		= 4;
const S32 LSL_PRIM_BUMP_BRICKS		= 5;
const S32 LSL_PRIM_BUMP_CHECKER		= 6;
const S32 LSL_PRIM_BUMP_CONCRETE	= 7;
const S32 LSL_PRIM_BUMP_TILE		= 8;
const S32 LSL_PRIM_BUMP_STONE		= 9;
const S32 LSL_PRIM_BUMP_DISKS		= 10;
const S32 LSL_PRIM_BUMP_GRAVEL		= 11;
const S32 LSL_PRIM_BUMP_BLOBS		= 12;
const S32 LSL_PRIM_BUMP_SIDING		= 13;
const S32 LSL_PRIM_BUMP_LARGETILE	= 14;
const S32 LSL_PRIM_BUMP_STUCCO		= 15;
const S32 LSL_PRIM_BUMP_SUCTION		= 16;
const S32 LSL_PRIM_BUMP_WEAVE		= 17;

const S32 LSL_PRIM_SCULPT_TYPE_SPHERE   = 1;
const S32 LSL_PRIM_SCULPT_TYPE_TORUS    = 2;
const S32 LSL_PRIM_SCULPT_TYPE_PLANE    = 3;
const S32 LSL_PRIM_SCULPT_TYPE_CYLINDER = 4;
const S32 LSL_PRIM_SCULPT_TYPE_MASK     = 7;
const S32 LSL_PRIM_SCULPT_FLAG_INVERT   = 64;
const S32 LSL_PRIM_SCULPT_FLAG_MIRROR   = 128;

const S32 LSL_PRIM_PHYSICS_SHAPE_PRIM	= 0;
const S32 LSL_PRIM_PHYSICS_SHAPE_NONE	= 1;
const S32 LSL_PRIM_PHYSICS_SHAPE_CONVEX = 2;

const S32 LSL_DENSITY				= 1;
const S32 LSL_FRICTION				= 2;
const S32 LSL_RESTITUTION			= 4;
const S32 LSL_GRAVITY_MULTIPLIER	= 8;

const S32 LSL_ALL_SIDES				= -1;
const S32 LSL_LINK_ROOT				= 1;
const S32 LSL_LINK_FIRST_CHILD		= 2;
const S32 LSL_LINK_SET				= -1;
const S32 LSL_LINK_ALL_OTHERS		= -2;
const S32 LSL_LINK_ALL_CHILDREN		= -3;
const S32 LSL_LINK_THIS				= -4;

// LSL constants for llSetForSell
const S32 SELL_NOT			= 0;
const S32 SELL_ORIGINAL		= 1;
const S32 SELL_COPY			= 2;
const S32 SELL_CONTENTS		= 3;

// LSL constants for llSetPayPrice
const S32 PAY_PRICE_HIDE = -1;
const S32 PAY_PRICE_DEFAULT = -2;
const S32 MAX_PAY_BUTTONS = 4;
const S32 PAY_BUTTON_DEFAULT_0 = 1;
const S32 PAY_BUTTON_DEFAULT_1 = 5;
const S32 PAY_BUTTON_DEFAULT_2 = 10;
const S32 PAY_BUTTON_DEFAULT_3 = 20;

// lsl email registration.
const S32 EMAIL_REG_SUBSCRIBE_OBJECT	= 0x01;
const S32 EMAIL_REG_UNSUBSCRIBE_OBJECT	= 0x02;
const S32 EMAIL_REG_UNSUBSCRIBE_SIM		= 0x04;

const S32 LIST_STAT_RANGE = 0;
const S32 LIST_STAT_MIN		= 1;
const S32 LIST_STAT_MAX		= 2;
const S32 LIST_STAT_MEAN	= 3;
const S32 LIST_STAT_MEDIAN	= 4;
const S32 LIST_STAT_STD_DEV	= 5;
const S32 LIST_STAT_SUM = 6;
const S32 LIST_STAT_SUM_SQUARES = 7;
const S32 LIST_STAT_NUM_COUNT = 8;
const S32 LIST_STAT_GEO_MEAN = 9;

const S32 STRING_TRIM_HEAD = 0x01;
const S32 STRING_TRIM_TAIL = 0x02;
const S32 STRING_TRIM = STRING_TRIM_HEAD | STRING_TRIM_TAIL;

// llGetObjectDetails
const S32 OBJECT_UNKNOWN_DETAIL = -1;
const S32 OBJECT_NAME = 1;
const S32 OBJECT_DESC = 2;
const S32 OBJECT_POS = 3;
const S32 OBJECT_ROT = 4;
const S32 OBJECT_VELOCITY = 5;
const S32 OBJECT_OWNER = 6;
const S32 OBJECT_GROUP = 7;
const S32 OBJECT_CREATOR = 8;
const S32 OBJECT_RUNNING_SCRIPT_COUNT = 9;
const S32 OBJECT_TOTAL_SCRIPT_COUNT = 10;
const S32 OBJECT_SCRIPT_MEMORY = 11;
const S32 OBJECT_SCRIPT_TIME = 12;
const S32 OBJECT_PRIM_EQUIVALENCE = 13;
const S32 OBJECT_SERVER_COST = 14;
const S32 OBJECT_STREAMING_COST = 15;
const S32 OBJECT_PHYSICS_COST = 16;

// llTextBox() magic token string - yes it's a hack. 
char const* const TEXTBOX_MAGIC_TOKEN = "!!llTextBox!!"; 

// changed() event flags
const U32	CHANGED_NONE = 0x0;
const U32	CHANGED_INVENTORY = 0x1;
const U32	CHANGED_COLOR = 0x2;
const U32	CHANGED_SHAPE = 0x4;
const U32	CHANGED_SCALE = 0x8;
const U32	CHANGED_TEXTURE = 0x10;
const U32	CHANGED_LINK = 0x20;
const U32	CHANGED_ALLOWED_DROP = 0x40;
const U32	CHANGED_OWNER = 0x80;
const U32	CHANGED_REGION = 0x100;
const U32	CHANGED_TELEPORT = 0x200;
const U32	CHANGED_REGION_START = 0x400;
const U32   CHANGED_MEDIA = 0x800;

// Possible error results
const U32 LSL_STATUS_OK                 = 0;
const U32 LSL_STATUS_MALFORMED_PARAMS   = 1000;
const U32 LSL_STATUS_TYPE_MISMATCH      = 1001;
const U32 LSL_STATUS_BOUNDS_ERROR       = 1002;
const U32 LSL_STATUS_NOT_FOUND          = 1003;
const U32 LSL_STATUS_NOT_SUPPORTED      = 1004;
const U32 LSL_STATUS_INTERNAL_ERROR     = 1999;

// Start per-function errors below, starting at 2000:
const U32 LSL_STATUS_WHITELIST_FAILED   = 2001;

// Memory profiling support
const S32 LSL_PROFILE_SCRIPT_NONE		= 0;
const S32 LSL_PROFILE_SCRIPT_MEMORY		= 1;

// HTTP responses contents type
const S32 LSL_CONTENT_TYPE_TEXT = 0;
const S32 LSL_CONTENT_TYPE_HTML = 1;

// Ray casting
const S32 LSL_RCERR_UNKNOWN				= -1;
const S32 LSL_RCERR_SIM_PERF_LOW		= -2;
const S32 LSL_RCERR_CAST_TIME_EXCEEDED	= -3;

const S32 LSL_RC_REJECT_TYPES			= 0;
const S32 LSL_RC_DETECT_PHANTOM			= 1;
const S32 LSL_RC_DATA_FLAGS				= 2;
const S32 LSL_RC_MAX_HITS				= 3;

const S32 LSL_RC_REJECT_AGENTS			= 1;
const S32 LSL_RC_REJECT_PHYSICAL		= 2;
const S32 LSL_RC_REJECT_NONPHYSICAL		= 4;
const S32 LSL_RC_REJECT_LAND			= 8;

const S32 LSL_RC_GET_NORMAL				= 1;
const S32 LSL_RC_GET_ROOT_KEY			= 2;
const S32 LSL_RC_GET_LINK_NUM			= 4;

// Estate management
const S32 LSL_ESTATE_ACCESS_ALLOWED_AGENT_ADD		= 4;
const S32 LSL_ESTATE_ACCESS_ALLOWED_AGENT_REMOVE	= 8;
const S32 LSL_ESTATE_ACCESS_ALLOWED_GROUP_ADD		= 16;
const S32 LSL_ESTATE_ACCESS_ALLOWED_GROUP_REMOVE	= 32;
const S32 LSL_ESTATE_ACCESS_BANNED_AGENT_ADD		= 64;
const S32 LSL_ESTATE_ACCESS_BANNED_AGENT_REMOVE		= 128;

// Key Frame Motion:
const S32 LSL_KFM_COMMAND		= 0;
const S32 LSL_KFM_MODE			= 1;
const S32 LSL_KFM_DATA			= 2;
const S32 LSL_KFM_FORWARD		= 0;
const S32 LSL_KFM_LOOP			= 1;
const S32 LSL_KFM_PING_PONG		= 2;
const S32 LSL_KFM_REVERSE		= 3;
const S32 LSL_KFM_ROTATION		= 1;
const S32 LSL_KFM_TRANSLATION	= 2;
const S32 LSL_KFM_CMD_PLAY		= 0;
const S32 LSL_KFM_CMD_STOP		= 1;
const S32 LSL_KFM_CMD_PAUSE		= 2;

// Second Life Server/12 12.04.30.255166  constants for llGetAgentList
const S32 AGENT_LIST_PARCEL               = 1;
const S32 AGENT_LIST_PARCEL_OWNER         = 2; 
const S32 AGENT_LIST_REGION               = 4;


// --- SL Constants ABOVE this line ---
// --- OpenSim / Aurora-Sim constants Below ---
// OpenSim Constants (\OpenSim\Region\ScriptEngine\Shared\Api\Runtime\LSL_Constants.cs) 
// Constants for cmWindlight (\OpenSim\Region\ScriptEngine\Shared\Api\Runtime\CM_Constants.cs) 
const S32 CHANGED_ANIMATION	        = 16384;
const S32 PARCEL_DETAILS_CLAIMDATE = 10; // used by OpenSim osSetParcelDetails
const S32 STATS_TIME_DILATION	    = 0;
const S32 STATS_SIM_FPS 	        = 1;
const S32 STATS_PHYSICS_FPS 	    = 2;
const S32 STATS_AGENT_UPDATES 	    = 3;
const S32 STATS_ROOT_AGENTS 	    = 4;
const S32 STATS_CHILD_AGENTS        = 5;
const S32 STATS_TOTAL_PRIMS         = 6;
const S32 STATS_ACTIVE_PRIMS	    = 7;
const S32 STATS_FRAME_MS 	        = 8;
const S32 STATS_NET_MS 	            = 9;
const S32 STATS_PHYSICS_MS 	        = 10;
const S32 STATS_IMAGE_MS 	        = 11;
const S32 STATS_OTHER_MS            = 12;
const S32 STATS_IN_PACKETS_PER_SECOND   = 13;
const S32 STATS_OUT_PACKETS_PER_SECOND 	= 14;
const S32 STATS_UNACKED_BYTES 	    = 15;
const S32 STATS_AGENT_MS 	        = 16;
const S32 STATS_PENDING_DOWNLOADS   = 17;
const S32 STATS_PENDING_UPLOADS 	= 18;
const S32 STATS_ACTIVE_SCRIPTS 	    = 19;
const S32 STATS_SCRIPT_LPS          = 20;
// osNPC
const S32 OS_NPC_FLY 	            = 0;
const S32 OS_NPC_NO_FLY 	        = 1;
const S32 OS_NPC_LAND_AT_TARGET 	= 2;
const S32 OS_NPC_SIT_NOW 	        = 0;
const U32 OS_NPC_CREATOR_OWNED 	    = 0x1;
const U32 OS_NPC_NOT_OWNED          = 0x2;
const U32 OS_NPC_SENSE_AS_AGENT     = 0x4;
const U32 OS_NPC_RUNNING            = 4;
// Lightshare / Windlight
const S32 WL_WATER_COLOR                = 0;
const S32 WL_WATER_FOG_DENSITY_EXPONENT	= 1;
const S32 WL_UNDERWATER_FOG_MODIFIER	= 2;
const S32 WL_REFLECTION_WAVELET_SCALE	= 3;
const S32 WL_FRESNEL_SCALE	            = 4;
const S32 WL_FRESNEL_OFFSET	            = 5;
const S32 WL_REFRACT_SCALE_ABOVE	    = 6;
const S32 WL_REFRACT_SCALE_BELOW	    = 7;
const S32 WL_BLUR_MULTIPLIER	        = 8;
const S32 WL_BIG_WAVE_DIRECTION	        = 9;
const S32 WL_LITTLE_WAVE_DIRECTION	    = 10;
const S32 WL_NORMAL_MAP_TEXTURE	        = 11;
const S32 WL_HORIZON	                = 12;
const S32 WL_HAZE_HORIZON	            = 13;
const S32 WL_BLUE_DENSITY	            = 14;
const S32 WL_HAZE_DENSITY	            = 15;
const S32 WL_DENSITY_MULTIPLIER	        = 16;
const S32 WL_DISTANCE_MULTIPLIER	    = 17;
const S32 WL_MAX_ALTITUDE	            = 18;
const S32 WL_SUN_MOON_COLOR	            = 19;
const S32 WL_AMBIENT	                = 20;
const S32 WL_EAST_ANGLE	                = 21;
const S32 WL_SUN_GLOW_FOCUS	            = 22;
const S32 WL_SUN_GLOW_SIZE	            = 23;
const S32 WL_SCENE_GAMMA	            = 24;
const S32 WL_STAR_BRIGHTNESS	        = 25;
const S32 WL_CLOUD_COLOR	            = 26;
const S32 WL_CLOUD_XY_DENSITY         	= 27;
const S32 WL_CLOUD_COVERAGE           	= 28;
const S32 WL_CLOUD_SCALE	            = 29;
const S32 WL_CLOUD_DETAIL_XY_DENSITY	= 30;
const S32 WL_CLOUD_SCROLL_X	            = 31;
const S32 WL_CLOUD_SCROLL_Y	            = 32;
const S32 WL_CLOUD_SCROLL_Y_LOCK	    = 33;
const S32 WL_CLOUD_SCROLL_X_LOCK	    = 34;
const S32 WL_DRAW_CLASSIC_CLOUDS	    = 35;
const S32 WL_SUN_MOON_POSITION	        = 36;
// Aurora-Sim Constants  (\Aurora\AuroraDotNetEngine\APIs\AA_Constants.cs) -->
const S32 BOT_FOLLOW_FLAG_NONE	        = 0;  
const S32 BOT_FOLLOW_FLAG_INDEFINITELY  = 1;  
const S32 BOT_FOLLOW_WALK	            = 0;  
const S32 BOT_FOLLOW_RUN	            = 1;  
const S32 BOT_FOLLOW_FLY	            = 2;  
const S32 BOT_FOLLOW_TELEPORT	        = 3;  
const S32 BOT_FOLLOW_WAIT	            = 4;  
const S32 BOT_FOLLOW_TRIGGER_HERE_EVENT	= 1;
const S32 BOT_FOLLOW_FLAG_FORCEDIRECTPATH = 4;
// string constants from Aurora-Sim
char const* const ENABLE_GRAVITY = "enable_gravity";
char const* const GRAVITY_FORCE_X = "gravity_force_x";
char const* const GRAVITY_FORCE_Y = "gravity_force_y";
char const* const GRAVITY_FORCE_Z = "gravity_force_z";
char const* const ADD_GRAVITY_POINT = "add_gravity_point";
char const* const ADD_GRAVITY_FORCE = "add_gravity_force";
char const* const START_TIME_REVERSAL_SAVING = "start_time_reversal_saving";
char const* const STOP_TIME_REVERSAL_SAVING = "stop_time_reversal_saving";
char const* const START_TIME_REVERSAL = "start_time_reversal";
char const* const STOP_TIME_REVERSAL = "stop_time_reversal";    

#endif
