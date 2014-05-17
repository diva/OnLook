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

#include "nd_hacdStructs.h"

void DecompHull::clear()
{
	mVertices.clear();
	mTriangles.clear();
	mLLVertices.clear();
	mLLTriangles.clear();
}

void DecompHull::computeLLVertices()
{
	if ( mLLVertices.size()*3 != mVertices.size() )
	{
		for ( size_t i = 0; i < mVertices.size(); ++i )
		{
			mLLVertices.push_back( static_cast<float>( mVertices[i].X() ) );
			mLLVertices.push_back( static_cast<float>( mVertices[i].Y() ) );
			mLLVertices.push_back( static_cast<float>( mVertices[i].Z() ) );
		}
	}
}

void DecompHull::computeLLTriangles()
{
	if ( mLLTriangles.size()*3 != mTriangles.size() )
	{
		for ( size_t i = 0; i < mTriangles.size(); ++i )
		{
			mLLTriangles.push_back( mTriangles[i].X() );
			mLLTriangles.push_back( mTriangles[i].Y() );
			mLLTriangles.push_back( mTriangles[i].Z() );
		}
	}

}

void DecompHull::toLLHull( LLCDHull *aHull )
{
	computeLLVertices();

	aHull->mVertexBase = &mLLVertices[0];
	aHull->mVertexStrideBytes = sizeof( float ) * 3;
	aHull->mNumVertices = mVertices.size();
}

void DecompHull::toLLMesh( LLCDMeshData *aMesh )
{
	computeLLVertices();
	computeLLTriangles();

	aMesh->mIndexType = LLCDMeshData::INT_32;
	aMesh->mVertexBase = &mLLVertices[0];
	aMesh->mNumVertices = mVertices.size();
	aMesh->mVertexStrideBytes = sizeof( float ) * 3;

	aMesh->mIndexBase = &mLLTriangles[0];
	aMesh->mIndexStrideBytes = sizeof( hacdUINT32 ) * 3;
	aMesh->mNumTriangles = mTriangles.size();
}

void DecompData::clear()
{
	mHulls.clear();
}

HACDDecoder::HACDDecoder()
{
	mStages.resize( NUM_STAGES );
	mCallback = 0;
}

void HACDDecoder::clear()
{
	mVertices.clear();
	mTriangles.clear();

	for ( size_t i = 0;  i < mStages.size(); ++i )
		mStages[i].clear();

	mSingleHull.clear();

	mCallback = 0;
}




