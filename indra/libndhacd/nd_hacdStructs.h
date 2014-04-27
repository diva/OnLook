/**
 * copyright 2011 sl.nicky.ml@googlemail.com
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
 */

#ifndef ND_HACD_DECOMPSTRUCTS_H
#define ND_HACD_DECOMPSTRUCTS_H

#include "nd_hacdDefines.h"
#include "hacdHACD.h"
#include "llconvexdecomposition.h"
#include <vector>

struct LLCDHull;
struct LLCDMeshData;

struct DecompHull
{
	std::vector< tVecDbl > mVertices;
	std::vector< tVecLong > mTriangles;

	std::vector< float > mLLVertices;
	std::vector< hacdUINT32 > mLLTriangles;

	void clear();

	void computeLLVertices();

	void computeLLTriangles();

	void toLLHull( LLCDHull *aHull );
	void toLLMesh( LLCDMeshData *aMesh );
};

struct DecompData
{
	std::vector< DecompHull > mHulls;

	void clear();
};

struct HACDDecoder: public HACD::ICallback
{
	std::vector< tVecDbl > mVertices;
	std::vector< tVecLong > mTriangles;

	std::vector< DecompData > mStages;
	DecompHull mSingleHull;

	llcdCallbackFunc mCallback;

	HACDDecoder();
	void clear();

	virtual void operator()( char const *aMsg, double aProgress, double aConcavity, size_t aVertices)
	{
		if( mCallback )
			(*mCallback)(aMsg, static_cast<int>(aProgress), aVertices );
	}

};


#endif
