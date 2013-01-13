#ifndef ND_CONVEXDECOMPOSITION_H
#define ND_CONVEXDECOMPOSITION_H

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

#ifndef ND_HASCONVEXDECOMP_TRACER
 #define ND_HASCONVEXDECOMP_TRACER
#endif

class ndConvexDecompositionTracer
{
public:
	enum ETraceLevel
	{
		eNone = 0,
		eTraceFunctions = 0x1,
		eTraceData = 0x2,
	};
	
	virtual ~ndConvexDecompositionTracer()
	{ }

	virtual void trace( char const *a_strMsg ) = 0;

	virtual void startTraceData( char const *a_strWhat) = 0;
	virtual void traceData( char const *a_strData ) = 0;
	virtual void endTraceData() = 0;

	virtual int getLevel() = 0;

	virtual void addref() = 0;
	virtual void release() = 0;
};

class ndConvexDecompositionTracable
{
public:
	virtual void setTracer( ndConvexDecompositionTracer *) = 0;
};


#endif
