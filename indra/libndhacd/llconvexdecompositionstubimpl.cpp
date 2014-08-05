/** 
 * @file   llconvexdecompositionstubimpl.cpp
 * @author falcon@lindenlab.com
 * @brief  A stub implementation of LLConvexDecomposition
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */


#include <string.h>
#include <memory>
#include "llconvexdecompositionstubimpl.h"

LLConvexDecomposition* LLConvexDecompositionImpl::getInstance()
{
	return NULL;
}

LLCDResult LLConvexDecompositionImpl::initSystem()
{
	return LLCD_NOT_IMPLEMENTED;
}

LLCDResult LLConvexDecompositionImpl::initThread()
{
	return LLCD_NOT_IMPLEMENTED;
}

LLCDResult LLConvexDecompositionImpl::quitThread()
{
	return LLCD_NOT_IMPLEMENTED;
}

LLCDResult LLConvexDecompositionImpl::quitSystem()
{
	return LLCD_NOT_IMPLEMENTED;
}

void LLConvexDecompositionImpl::genDecomposition(int& decomp)
{

}

void LLConvexDecompositionImpl::deleteDecomposition(int decomp)
{

}

void LLConvexDecompositionImpl::bindDecomposition(int decomp)
{

}

LLCDResult LLConvexDecompositionImpl::setParam(const char* name, float val)
{
	return LLCD_NOT_IMPLEMENTED;
}

LLCDResult LLConvexDecompositionImpl::setParam(const char* name, bool val)
{
	return LLCD_NOT_IMPLEMENTED;
}

LLCDResult LLConvexDecompositionImpl::setParam(const char* name, int val)
{
	return LLCD_NOT_IMPLEMENTED;
}

LLCDResult LLConvexDecompositionImpl::setMeshData( const LLCDMeshData* data, bool vertex_based )
{
	return LLCD_NOT_IMPLEMENTED;
}

LLCDResult LLConvexDecompositionImpl::registerCallback(int stage, llcdCallbackFunc callback )
{
	return LLCD_NOT_IMPLEMENTED;
}

LLCDResult LLConvexDecompositionImpl::buildSingleHull()
{
	return LLCD_NOT_IMPLEMENTED;
}

LLCDResult LLConvexDecompositionImpl::executeStage(int stage)
{
	return LLCD_NOT_IMPLEMENTED;
}

int LLConvexDecompositionImpl::getNumHullsFromStage(int stage)
{
	return 0;
}

LLCDResult LLConvexDecompositionImpl::getSingleHull( LLCDHull* hullOut ) 
{
	memset( hullOut, 0, sizeof(LLCDHull) );
	return LLCD_NOT_IMPLEMENTED;
}

LLCDResult LLConvexDecompositionImpl::getHullFromStage( int stage, int hull, LLCDHull* hullOut )
{
	memset( hullOut, 0, sizeof(LLCDHull) );
	return LLCD_NOT_IMPLEMENTED;
}

LLCDResult LLConvexDecompositionImpl::getMeshFromStage( int stage, int hull, LLCDMeshData* meshDataOut )
{
	memset( meshDataOut, 0, sizeof(LLCDMeshData) );
	return LLCD_NOT_IMPLEMENTED;
}

LLCDResult	LLConvexDecompositionImpl::getMeshFromHull( LLCDHull* hullIn, LLCDMeshData* meshOut )
{
	memset( meshOut, 0, sizeof(LLCDMeshData) );
	return LLCD_NOT_IMPLEMENTED;
}

LLCDResult LLConvexDecompositionImpl::generateSingleHullMeshFromMesh(LLCDMeshData* meshIn, LLCDMeshData* meshOut)
{
	memset( meshOut, 0, sizeof(LLCDMeshData) );
	return LLCD_NOT_IMPLEMENTED;
}

void LLConvexDecompositionImpl::loadMeshData(const char* fileIn, LLCDMeshData** meshDataOut)
{
	static LLCDMeshData meshData;
	memset( &meshData, 0, sizeof(LLCDMeshData) );
	*meshDataOut = &meshData;
}
