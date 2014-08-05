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

#ifndef ND_HACD_UTILS_H
#define ND_HACD_UTILS_H

#include "nd_hacdStructs.h"

tHACD* init( int nConcavity, int nClusters, int nMaxVerticesPerHull, double dMaxConnectDist, HACDDecoder *aData );
DecompData decompose( tHACD *aHACD );

tVecLong fromI16( void *& pPtr, int aStride );
tVecLong fromI32( void *& pPtr, int aStride );

LLCDResult setMeshData( const LLCDMeshData* data, bool vertex_based, HACDDecoder *aDec );
LLCDResult convertHullToMesh( const LLCDHull* aHull, std::vector< float > &aVerticesOut, std::vector< int > &aTrianglesOut );

#endif
