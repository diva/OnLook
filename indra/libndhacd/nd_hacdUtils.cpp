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

#include "nd_hacdUtils.h"

tHACD* init( int nConcavity, int nClusters, int nMaxVerticesPerHull, double dMaxConnectDist, HACDDecoder *aData )
{
	tHACD *pDec = HACD::CreateHACD(0);
	pDec->SetPoints( &aData->mVertices[0] );
	pDec->SetNPoints( aData->mVertices.size() );

	if ( aData->mTriangles.size() )
	{
		pDec->SetTriangles( &aData->mTriangles[0] );
		pDec->SetNTriangles( aData->mTriangles.size() );
	}

	pDec->SetCompacityWeight( 0.1f );
	pDec->SetVolumeWeight( 0 );
	pDec->SetNClusters( nClusters );
	pDec->SetAddExtraDistPoints( true );
	pDec->SetAddFacesPoints( true );
	pDec->SetNVerticesPerCH( nMaxVerticesPerHull );
	pDec->SetConcavity( nConcavity );
	pDec->SetConnectDist( dMaxConnectDist );

	pDec->SetCallBack( aData );

	return pDec;
}

DecompData decompose( tHACD *aHACD )
{
	aHACD->Compute();

	DecompData oRet;

	int nClusters = aHACD->GetNClusters();

	for ( int i = 0; i < nClusters; ++i )
	{
		int nVertices = aHACD->GetNPointsCH( i );
		int nTriangles = aHACD->GetNTrianglesCH( i );
		DecompHull oHull;

		oHull.mVertices.resize( nVertices );
		oHull.mTriangles.resize( nTriangles );
		aHACD->GetCH( i, &oHull.mVertices[0], &oHull.mTriangles[0] );

		oRet.mHulls.push_back( oHull );
	}

	return oRet;
}

tVecLong fromI16( void const *& pPtr, int aStride )
{
	hacdUINT16 const *pVal = reinterpret_cast< hacdUINT16 const * >( pPtr );
	tVecLong oRet( pVal[0], pVal[1], pVal[2] );
	pVal += aStride / 2;
	pPtr = pVal;
	return oRet;
}

tVecLong fromI32( void const *& pPtr, int aStride )
{
	hacdUINT32 const *pVal = reinterpret_cast< hacdUINT32 const * >( pPtr );
	tVecLong oRet( pVal[0], pVal[1], pVal[2] );
	pVal += aStride / 4;
	pPtr = pVal;
	return oRet;
}

LLCDResult setMeshData( const LLCDMeshData* data, bool vertex_based, HACDDecoder *aDec )
{
	if ( !data || !data->mVertexBase || ( data->mNumVertices < 3 ) || ( data->mVertexStrideBytes != 12 && data->mVertexStrideBytes != 16 ) )
		return LLCD_INVALID_MESH_DATA;

	if ( !vertex_based && ( ( data->mNumTriangles < 1 ) || ! data->mIndexBase ) )
		return LLCD_INVALID_MESH_DATA;

	aDec->clear();
	int nCount = data->mNumVertices;
	float const* pVertex = data->mVertexBase;
	int nStride = data->mVertexStrideBytes / sizeof( float );

	for ( int i = 0; i < nCount; ++i )
	{
		tVecDbl oPt( pVertex[0], pVertex[1], pVertex[2] );
		aDec->mVertices.push_back( oPt );
		pVertex += nStride;
	}

	if ( !vertex_based )
	{
		fFromIXX pFunc = 0;
		nCount = data->mNumTriangles;
		nStride = data->mIndexStrideBytes;
		void const *pData = data->mIndexBase;

		if ( data->mIndexType == LLCDMeshData::INT_16 )
			pFunc = fromI16;
		else
			pFunc = fromI32;

		for ( int i = 0; i < nCount; ++i )
		{
			tVecLong oVal( ( *pFunc )( pData, nStride ) );
			aDec->mTriangles.push_back( oVal );
		}
	}

	return LLCD_OK;
}

LLCDResult convertHullToMesh( const LLCDHull* aHull, std::vector< float > &aVerticesOut, std::vector< int > &aTrianglesOut )
{
	if( !aHull || !aHull->mVertexBase )
		return LLCD_NULL_PTR;
	if( aHull->mVertexStrideBytes < 3*sizeof(float) || aHull->mNumVertices < 3 )
		return LLCD_INVALID_HULL_DATA;

	HACD::ICHull oHull;

	int nCount = aHull->mNumVertices;
	float const *pVertex = aHull->mVertexBase;
	int nStride = aHull->mVertexStrideBytes / sizeof( float );

	std::vector< tVecDbl > vcPoints;

	for ( int i = 0; i < nCount; ++i )
	{
		vcPoints.push_back( tVecDbl( pVertex[0], pVertex[1], pVertex[2] ) ); 
		pVertex += nStride;
	}

	oHull.AddPoints( &vcPoints[0], vcPoints.size() );

	HACD::ICHullError eErr = oHull.Process( MAX_VERTICES_PER_HULL );
	if( HACD::ICHullErrorOK != eErr )
		return LLCD_INVALID_HULL_DATA;

	HACD::TMMesh &oMesh = oHull.GetMesh();

	std::vector< tVecDbl > vPoints;
	std::vector< tVecLong > vTriangles;

	vPoints.resize( oMesh.GetNVertices() );
	vTriangles.resize( oMesh.GetNTriangles() );

	oMesh.GetIFS( &vPoints[0], &vTriangles[0] );

	for( size_t i = 0; i < vPoints.size(); ++i )
	{
		aVerticesOut.push_back( static_cast<float>( vPoints[i].X() ) );
		aVerticesOut.push_back( static_cast<float>( vPoints[i].Y() ) );
		aVerticesOut.push_back( static_cast<float>( vPoints[i].Z() ) );
	}

	for( size_t i = 0; i < vTriangles.size(); ++i )
	{
		aTrianglesOut.push_back( vTriangles[i].X() );
		aTrianglesOut.push_back( vTriangles[i].Y() );
		aTrianglesOut.push_back( vTriangles[i].Z() );
	}

	return LLCD_OK;
}
