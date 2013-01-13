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

#include "nd_EnterExitTracer.h"

#include <vector>
#include <sstream>

void EnterExitTracer::doTrace( char const *aPrefix )
{
	if( !mFunc || !mTracer )
		return;

	if( ndConvexDecompositionTracer::eTraceFunctions == (mTracer->getLevel() & ndConvexDecompositionTracer::eTraceFunctions ) )
		return;

	std::stringstream str;
	if( aPrefix )
		str << aPrefix;

	str << mFunc;	
	mTracer->trace( str.str().c_str() );
}

EnterExitTracer::EnterExitTracer( char const *aFunc, ndConvexDecompositionTracer *aTracer )
	: mFunc( aFunc )
	, mTracer( aTracer )
{
	doTrace( "enter: " );
}

EnterExitTracer::~EnterExitTracer( )
{
	doTrace( "exit: " );
}

