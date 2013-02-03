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

#include <string.h>
#include <memory>
#include "nd_hacdConvexDecomposition.h"

#include "nd_hacdDefines.h"
#include "nd_hacdStructs.h"
#include "nd_hacdUtils.h"
#include "ndConvexDecomposition.h"
#include "nd_EnterExitTracer.h"
#include "nd_StructTracer.h"

LLCDStageData nd_hacdConvexDecomposition::mStages[1];

LLCDParam nd_hacdConvexDecomposition::mParams[4];
LLCDParam::LLCDEnumItem nd_hacdConvexDecomposition::mMethods[1];
LLCDParam::LLCDEnumItem nd_hacdConvexDecomposition::mQuality[1];
LLCDParam::LLCDEnumItem nd_hacdConvexDecomposition::mSimplify[1];

LLConvexDecomposition* nd_hacdConvexDecomposition::getInstance()
{
	static nd_hacdConvexDecomposition sImpl;
	return &sImpl;
}

nd_hacdConvexDecomposition::nd_hacdConvexDecomposition()
{
	mNextId = 0;
	mCurrentDecoder = 0;
	mSingleHullMeshFromMesh = new HACDDecoder();
	mTracer = 0;
}

nd_hacdConvexDecomposition::~nd_hacdConvexDecomposition()
{
	if( mTracer )
		mTracer->release();
}

LLCDResult nd_hacdConvexDecomposition::initSystem()
{
	memset( &mStages[0], 0, sizeof( mStages ) );
	memset( &mParams[0], 0, sizeof( mParams ) );
	memset( &mMethods[0], 0, sizeof( mMethods ) );
	memset( &mQuality[0], 0, sizeof( mQuality ) );
	memset( &mSimplify[0], 0, sizeof( mSimplify ) );

	mStages[0].mName = "Decompose";
	mStages[0].mDescription = NULL;

	mMethods[0].mName = "Default";
	mMethods[0].mValue = 0;

	mQuality[0].mName = "Default";
	mQuality[0].mValue = 0;

	mSimplify[0].mName = "None";
	mSimplify[0].mValue = 0;

	mParams[0].mName = "nd_AlwaysNeedTriangles";
	mParams[0].mDescription = 0;
	mParams[0].mType = LLCDParam::LLCD_BOOLEAN;
	mParams[0].mDefault.mBool = true;

	mParams[1].mName = "Method";
	mParams[1].mType = LLCDParam::LLCD_ENUM;
	mParams[1].mDetails.mEnumValues.mNumEnums = sizeof(mMethods)/sizeof(LLCDParam::LLCDEnumItem);
	mParams[1].mDetails.mEnumValues.mEnumsArray = mMethods;
	mParams[1].mDefault.mIntOrEnumValue = 0;

	mParams[2].mName = "Decompose Quality";
	mParams[2].mType = LLCDParam::LLCD_ENUM;
	mParams[2].mDetails.mEnumValues.mNumEnums = sizeof(mQuality)/sizeof(LLCDParam::LLCDEnumItem);
	mParams[2].mDetails.mEnumValues.mEnumsArray = mQuality;
	mParams[2].mDefault.mIntOrEnumValue = 0;

	mParams[3].mName = "Simplify Method";
	mParams[3].mType = LLCDParam::LLCD_ENUM;
	mParams[3].mDetails.mEnumValues.mNumEnums = sizeof(mSimplify)/sizeof(LLCDParam::LLCDEnumItem);
	mParams[3].mDetails.mEnumValues.mEnumsArray = mSimplify;
	mParams[3].mDefault.mIntOrEnumValue = 0;

	return LLCD_OK;
}

LLCDResult nd_hacdConvexDecomposition::initThread()
{
	return LLCD_OK;
}

LLCDResult nd_hacdConvexDecomposition::quitThread()
{
	return LLCD_OK;
}

LLCDResult nd_hacdConvexDecomposition::quitSystem()
{
	return LLCD_OK;
}

void nd_hacdConvexDecomposition::genDecomposition( int& decomp )
{
	HACDDecoder *pGen = new HACDDecoder();
	decomp = mNextId;
	++mNextId;

	mDecoders[ decomp ] = pGen;
}

void nd_hacdConvexDecomposition::deleteDecomposition( int decomp )
{
	delete mDecoders[ decomp ];
	mDecoders.erase( decomp );
}

void nd_hacdConvexDecomposition::bindDecomposition( int decomp )
{
	TRACE_FUNC( mTracer );
	mCurrentDecoder = decomp;
}

LLCDResult nd_hacdConvexDecomposition::setParam( const char* name, float val )
{
	TRACE_FUNC( mTracer );
	return LLCD_OK;
}

LLCDResult nd_hacdConvexDecomposition::setParam( const char* name, bool val )
{
	TRACE_FUNC( mTracer );
	return LLCD_OK;
}

LLCDResult nd_hacdConvexDecomposition::setParam( const char* name, int val )
{
	TRACE_FUNC( mTracer );
	return LLCD_OK;
}

LLCDResult nd_hacdConvexDecomposition::setMeshData( const LLCDMeshData* data, bool vertex_based )
{
	TRACE_FUNC( mTracer );
	ndStructTracer::trace( data, vertex_based, mTracer );

	HACDDecoder *pC = mDecoders[ mCurrentDecoder ];
	return ::setMeshData( data, vertex_based, pC );
}

LLCDResult nd_hacdConvexDecomposition::registerCallback( int stage, llcdCallbackFunc callback )
{
	TRACE_FUNC( mTracer );
	if( mDecoders.end() == mDecoders.find( mCurrentDecoder ) )
		return LLCD_STAGE_NOT_READY;

	HACDDecoder *pC = mDecoders[ mCurrentDecoder ];
	pC->mCallback = callback;

	return LLCD_OK;
}

LLCDResult nd_hacdConvexDecomposition::buildSingleHull()
{
	TRACE_FUNC( mTracer );
	return LLCD_OK;
}

LLCDResult nd_hacdConvexDecomposition::executeStage( int stage )
{
	TRACE_FUNC( mTracer );
	if ( stage < 0 || stage >= NUM_STAGES )
		return LLCD_INVALID_STAGE;

	HACDDecoder *pC = mDecoders[ mCurrentDecoder ];
	tHACD *pHACD = init( 1, MIN_NUMBER_OF_CLUSTERS, MAX_VERTICES_PER_HULL, CONNECT_DISTS[0], pC );

	DecompData oRes = decompose( pHACD );
	ndStructTracer::trace( oRes, mTracer );

	delete pHACD;

	pC->mStages[ stage ] = oRes;

	return LLCD_OK;
}

int nd_hacdConvexDecomposition::getNumHullsFromStage( int stage )
{
	TRACE_FUNC( mTracer );
	HACDDecoder *pC = mDecoders[ mCurrentDecoder ];

	if ( !pC )
		return 0;

	if ( stage < 0 || static_cast<size_t>(stage) >= pC->mStages.size() )
		return 0;

	return pC->mStages[stage].mHulls.size();
}

DecompData toSingleHull( HACDDecoder *aDecoder, LLCDResult &aRes, ndConvexDecompositionTracer *aTracer )
{
	TRACE_FUNC( aTracer );
	aRes = LLCD_REQUEST_OUT_OF_RANGE;

	for ( int i = 0; i < TO_SINGLE_HULL_TRIES; ++i )
	{
		tHACD *pHACD = init( CONCAVITY_FOR_SINGLE_HULL[i], 1, MAX_VERTICES_PER_HULL, CONNECT_DISTS[i], aDecoder );

		DecompData oRes = decompose( pHACD );
		delete pHACD;

		ndStructTracer::trace( oRes, aTracer );

		if ( oRes.mHulls.size() == 1 )
		{
			aRes = LLCD_OK;
			return oRes;
		}
	}

	return DecompData();
}

LLCDResult nd_hacdConvexDecomposition::getSingleHull( LLCDHull* hullOut )
{
	TRACE_FUNC( mTracer );
	HACDDecoder *pC = mDecoders[ mCurrentDecoder ];

	memset( hullOut, 0, sizeof( LLCDHull ) );

	LLCDResult res;

	// Will already trace oRes
	DecompData oRes = ::toSingleHull( pC, res, mTracer );

	if ( LLCD_OK != res || oRes.mHulls.size() != 1 )
		return res;

	pC->mSingleHull = oRes.mHulls[0];
	pC->mSingleHull.toLLHull( hullOut );

	return LLCD_OK;
}

LLCDResult nd_hacdConvexDecomposition::getHullFromStage( int stage, int hull, LLCDHull* hullOut )
{
	TRACE_FUNC( mTracer );
	HACDDecoder *pC = mDecoders[ mCurrentDecoder ];

	memset( hullOut, 0, sizeof( LLCDHull ) );

	if ( stage < 0 || static_cast<size_t>(stage) >= pC->mStages.size() )
		return LLCD_INVALID_STAGE;

	DecompData &oData = pC->mStages[ stage ];

	if ( hull < 0 ||static_cast<size_t>(hull) >= oData.mHulls.size() )
		return LLCD_REQUEST_OUT_OF_RANGE;

	oData.mHulls[ hull ].toLLHull( hullOut );
	ndStructTracer::trace( hullOut, mTracer );

	return LLCD_OK;
}

LLCDResult nd_hacdConvexDecomposition::getMeshFromStage( int stage, int hull, LLCDMeshData* meshDataOut )
{
	TRACE_FUNC( mTracer );
	HACDDecoder *pC = mDecoders[ mCurrentDecoder ];

	memset( meshDataOut, 0, sizeof( LLCDHull ) );

	if ( stage < 0 || static_cast<size_t>(stage) >= pC->mStages.size() )
		return LLCD_INVALID_STAGE;

	DecompData &oData = pC->mStages[ stage ];

	if ( hull < 0 || static_cast<size_t>(hull) >= oData.mHulls.size() )
		return LLCD_REQUEST_OUT_OF_RANGE;

	oData.mHulls[ hull ].toLLMesh( meshDataOut );
	ndStructTracer::trace( meshDataOut, true, mTracer );
	return LLCD_OK;
}

LLCDResult nd_hacdConvexDecomposition::getMeshFromHull( LLCDHull* hullIn, LLCDMeshData* meshOut )
{
	TRACE_FUNC( mTracer );
	memset( meshOut, 0, sizeof( LLCDMeshData ) );

	mMeshToHullVertices.clear();
	mMeshToHullTriangles.clear();

	if( !hullIn || !hullIn->mVertexBase || !meshOut )
		return LLCD_NULL_PTR;
	if( hullIn->mVertexStrideBytes < 3*sizeof(float) || hullIn->mNumVertices < 3 )
		return LLCD_INVALID_HULL_DATA;

	LLCDResult oRet = convertHullToMesh( hullIn, mMeshToHullVertices, mMeshToHullTriangles );

	if( LLCD_OK != oRet )
		return oRet;

	meshOut->mVertexStrideBytes = sizeof( float )*3;
	meshOut->mNumVertices = mMeshToHullVertices.size()/3;
	meshOut->mVertexBase = &mMeshToHullVertices[0];

	meshOut->mIndexType = LLCDMeshData::INT_32;
	meshOut->mIndexStrideBytes = sizeof( hacdUINT32 ) * 3;
	meshOut->mNumTriangles = mMeshToHullTriangles.size()/3;
	meshOut->mIndexBase = &mMeshToHullTriangles[0];

	return LLCD_OK;
}

LLCDResult nd_hacdConvexDecomposition::generateSingleHullMeshFromMesh( LLCDMeshData* meshIn, LLCDMeshData* meshOut )
{
	TRACE_FUNC( mTracer );
	memset( meshOut, 0, sizeof( LLCDMeshData ) );
	mSingleHullMeshFromMesh->clear();
	LLCDResult res = ::setMeshData( meshIn, meshIn->mNumVertices > 3, mSingleHullMeshFromMesh );

	if ( LLCD_OK != res )
		return res;

	// Will already trace oRes
	DecompData oRes = ::toSingleHull( mSingleHullMeshFromMesh, res, mTracer );

	if ( LLCD_OK != res || oRes.mHulls.size() != 1 )
		return res;

	mSingleHullMeshFromMesh->mSingleHull = oRes.mHulls[0];
	mSingleHullMeshFromMesh->mSingleHull.toLLMesh( meshOut );

	return LLCD_OK;
}

void nd_hacdConvexDecomposition::loadMeshData( const char* fileIn, LLCDMeshData** meshDataOut )
{
	TRACE_FUNC( mTracer );
	static LLCDMeshData meshData;
	memset( &meshData, 0, sizeof( LLCDMeshData ) );
	*meshDataOut = &meshData;
}

int nd_hacdConvexDecomposition::getParameters( const LLCDParam** paramsOut )
{
	TRACE_FUNC( mTracer );
	*paramsOut = mParams;
	return sizeof(mParams)/sizeof(LLCDParam);
}

int nd_hacdConvexDecomposition::getStages( const LLCDStageData** stagesOut )
{
	TRACE_FUNC( mTracer );

	*stagesOut = mStages;
	return sizeof(mStages)/sizeof(LLCDStageData);
}

void nd_hacdConvexDecomposition::setTracer( ndConvexDecompositionTracer * aTracer)
{
	if( mTracer )
		mTracer->release();

	mTracer = aTracer;

	if( mTracer )
		mTracer->addref();
}


bool nd_hacdConvexDecomposition::isFunctional()
{
	return true;
}
