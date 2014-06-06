/** 
 * @file llmatrix4a.h
 * @brief LLMatrix4a class header file - memory aligned and vectorized 4x4 matrix
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef	LL_LLMATRIX4A_H
#define	LL_LLMATRIX4A_H

#include "llvector4a.h"
#include "m4math.h"
#include "m3math.h"

class LLMatrix4a
{
public:
	LL_ALIGN_16(LLVector4a mMatrix[4]);

	inline F32* getF32ptr()
	{
		return mMatrix[0].getF32ptr();
	}

	inline void clear()
	{
		mMatrix[0].clear();
		mMatrix[1].clear();
		mMatrix[2].clear();
		mMatrix[3].clear();
	}

	inline void setIdentity()
	{
		static __m128 ones = _mm_set_ps(1.f,0.f,0.f,1.f);
		mMatrix[0] = _mm_movelh_ps(ones,_mm_setzero_ps());
		mMatrix[1] = _mm_movehl_ps(_mm_setzero_ps(),ones);
		mMatrix[2] = _mm_movelh_ps(_mm_setzero_ps(),ones);
		mMatrix[3] = _mm_movehl_ps(ones,_mm_setzero_ps());
	}

	inline void loadu(const LLMatrix4& src)
	{
		mMatrix[0] = _mm_loadu_ps(src.mMatrix[0]);
		mMatrix[1] = _mm_loadu_ps(src.mMatrix[1]);
		mMatrix[2] = _mm_loadu_ps(src.mMatrix[2]);
		mMatrix[3] = _mm_loadu_ps(src.mMatrix[3]);
	}

	inline void loadu(const LLMatrix3& src)
	{
		mMatrix[0].load3(src.mMatrix[0]);
		mMatrix[1].load3(src.mMatrix[1]);
		mMatrix[2].load3(src.mMatrix[2]);
		mMatrix[3].set(0,0,0,1.f);
	}

	inline void loadu(const F32* src)
	{
		mMatrix[0] = _mm_loadu_ps(src+0);
		mMatrix[1] = _mm_loadu_ps(src+4);
		mMatrix[2] = _mm_loadu_ps(src+8);
		mMatrix[3] = _mm_loadu_ps(src+12);
	}

	inline void add(const LLMatrix4a& rhs)
	{
		mMatrix[0].add(rhs.mMatrix[0]);
		mMatrix[1].add(rhs.mMatrix[1]);
		mMatrix[2].add(rhs.mMatrix[2]);
		mMatrix[3].add(rhs.mMatrix[3]);
	}

	inline void setRows(const LLVector4a& r0, const LLVector4a& r1, const LLVector4a& r2)
	{
		mMatrix[0] = r0;
		mMatrix[1] = r1;
		mMatrix[2] = r2;
	}

	inline void setMul(const LLMatrix4a& m, const F32 s)
	{
		mMatrix[0].setMul(m.mMatrix[0], s);
		mMatrix[1].setMul(m.mMatrix[1], s);
		mMatrix[2].setMul(m.mMatrix[2], s);
		mMatrix[3].setMul(m.mMatrix[3], s);
	}

	inline void setMul(const LLMatrix4a& m0, const LLMatrix4a& m1)
	{
		m0.rotate4(m1.mMatrix[0],mMatrix[0]);
		m0.rotate4(m1.mMatrix[1],mMatrix[1]);
		m0.rotate4(m1.mMatrix[2],mMatrix[2]);
		m0.rotate4(m1.mMatrix[3],mMatrix[3]);
	}

	inline void setLerp(const LLMatrix4a& a, const LLMatrix4a& b, F32 w)
	{
		LLVector4a d0,d1,d2,d3;
		d0.setSub(b.mMatrix[0], a.mMatrix[0]);
		d1.setSub(b.mMatrix[1], a.mMatrix[1]);
		d2.setSub(b.mMatrix[2], a.mMatrix[2]);
		d3.setSub(b.mMatrix[3], a.mMatrix[3]);

		// this = a + d*w
		
		d0.mul(w);
		d1.mul(w);
		d2.mul(w);
		d3.mul(w);

		mMatrix[0].setAdd(a.mMatrix[0],d0);
		mMatrix[1].setAdd(a.mMatrix[1],d1);
		mMatrix[2].setAdd(a.mMatrix[2],d2);
		mMatrix[3].setAdd(a.mMatrix[3],d3);
	}
	
	//Singu Note: Don't mess with this. It's intentionally different from LL's. 
	// Note how res isn't manipulated until the very end.
	inline void rotate(const LLVector4a& v, LLVector4a& res) const
	{
		LLVector4a x,y,z;

		x = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
		y = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
		z = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));

		x.mul(mMatrix[0]);
		y.mul(mMatrix[1]);
		z.mul(mMatrix[2]);

		x.add(y);
		res.setAdd(x,z);
	}
	
	inline void rotate4(const LLVector4a& v, LLVector4a& res) const
	{
		LLVector4a x,y,z,w;

		x = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
		y = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
		z = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
		w = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3));

		x.mul(mMatrix[0]);
		y.mul(mMatrix[1]);
		z.mul(mMatrix[2]);
		w.mul(mMatrix[3]);

		x.add(y);
		z.add(w);
		res.setAdd(x,z);
	}

	inline void affineTransform(const LLVector4a& v, LLVector4a& res) const
	{
		LLVector4a x,y,z;

		x = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
		y = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
		z = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
		
		x.mul(mMatrix[0]);
		y.mul(mMatrix[1]);
		z.mul(mMatrix[2]);

		x.add(y);
		z.add(mMatrix[3]);
		res.setAdd(x,z);
	}

	inline void transpose()
	{
		__m128 q1 = _mm_unpackhi_ps(mMatrix[0],mMatrix[1]);
		__m128 q2 = _mm_unpacklo_ps(mMatrix[0],mMatrix[1]);
		__m128 q3 = _mm_unpacklo_ps(mMatrix[2],mMatrix[3]);
		__m128 q4 = _mm_unpackhi_ps(mMatrix[2],mMatrix[3]);

		mMatrix[0] = _mm_movelh_ps(q2,q3);
		mMatrix[1] = _mm_movehl_ps(q3,q2);
		mMatrix[2] = _mm_movelh_ps(q1,q4);
		mMatrix[3] = _mm_movehl_ps(q4,q1);
	}

//  Following procedure adapted from:
//		http://software.intel.com/en-us/articles/optimized-matrix-library-for-use-with-the-intel-pentiumr-4-processors-sse2-instructions/
//
//  License/Copyright Statement:
//		
//			Copyright (c) 2001 Intel Corporation.
//
//		Permition is granted to use, copy, distribute and prepare derivative works 
//		of this library for any purpose and without fee, provided, that the above 
//		copyright notice and this statement appear in all copies.  
//		Intel makes no representations about the suitability of this library for 
//		any purpose, and specifically disclaims all warranties. 
//		See LEGAL-intel_matrixlib.TXT for all the legal information.
	inline float invert()
	{
		LL_ALIGN_16(const unsigned int Sign_PNNP[4]) = { 0x00000000, 0x80000000, 0x80000000, 0x00000000 };

		// The inverse is calculated using "Divide and Conquer" technique. The 
		// original matrix is divide into four 2x2 sub-matrices. Since each 
		// register holds four matrix element, the smaller matrices are 
		// represented as a registers. Hence we get a better locality of the 
		// calculations.

		LLVector4a A = _mm_movelh_ps(mMatrix[0], mMatrix[1]),    // the four sub-matrices 
				B = _mm_movehl_ps(mMatrix[1], mMatrix[0]),
				C = _mm_movelh_ps(mMatrix[2], mMatrix[3]),
				D = _mm_movehl_ps(mMatrix[3], mMatrix[2]);
		LLVector4a iA, iB, iC, iD,					// partial inverse of the sub-matrices
				DC, AB;
		LLSimdScalar dA, dB, dC, dD;                 // determinant of the sub-matrices
		LLSimdScalar det, d, d1, d2;
		LLVector4a rd;

		//  AB = A# * B
		AB.setMul(_mm_shuffle_ps(A,A,0x0F), B);
		AB.sub(_mm_mul_ps(_mm_shuffle_ps(A,A,0xA5), _mm_shuffle_ps(B,B,0x4E)));
		//  DC = D# * C
		DC.setMul(_mm_shuffle_ps(D,D,0x0F), C);
		DC.sub(_mm_mul_ps(_mm_shuffle_ps(D,D,0xA5), _mm_shuffle_ps(C,C,0x4E)));

		//  dA = |A|
		dA = _mm_mul_ps(_mm_shuffle_ps(A, A, 0x5F),A);
		dA -= _mm_movehl_ps(dA,dA);
		//  dB = |B|
		dB = _mm_mul_ps(_mm_shuffle_ps(B, B, 0x5F),B);
		dB -= _mm_movehl_ps(dB,dB);

		//  dC = |C|
		dC = _mm_mul_ps(_mm_shuffle_ps(C, C, 0x5F),C);
		dC -= _mm_movehl_ps(dC,dC);
		//  dD = |D|
		dD = _mm_mul_ps(_mm_shuffle_ps(D, D, 0x5F),D);
		dD -= _mm_movehl_ps(dD,dD);

		//  d = trace(AB*DC) = trace(A#*B*D#*C)
		d = _mm_mul_ps(_mm_shuffle_ps(DC,DC,0xD8),AB);

		//  iD = C*A#*B
		iD.setMul(_mm_shuffle_ps(C,C,0xA0), _mm_movelh_ps(AB,AB));
		iD.add(_mm_mul_ps(_mm_shuffle_ps(C,C,0xF5), _mm_movehl_ps(AB,AB)));
		//  iA = B*D#*C
		iA.setMul(_mm_shuffle_ps(B,B,0xA0), _mm_movelh_ps(DC,DC));
		iA.add(_mm_mul_ps(_mm_shuffle_ps(B,B,0xF5), _mm_movehl_ps(DC,DC)));

		//  d = trace(AB*DC) = trace(A#*B*D#*C) [continue]
		d = _mm_add_ps(d, _mm_movehl_ps(d, d));
		d += _mm_shuffle_ps(d, d, 1);
		d1 = dA*dD;
		d2 = dB*dC;

		//  iD = D*|A| - C*A#*B
		iD.setSub(_mm_mul_ps(D,_mm_shuffle_ps(dA,dA,0)), iD);

		//  iA = A*|D| - B*D#*C;
		iA.setSub(_mm_mul_ps(A,_mm_shuffle_ps(dD,dD,0)), iA);

		//  det = |A|*|D| + |B|*|C| - trace(A#*B*D#*C)
		det = d1+d2-d;

		__m128 is_zero_mask = _mm_cmpeq_ps(det,_mm_setzero_ps());
		rd = _mm_div_ss(_mm_set_ss(1.f),_mm_or_ps(_mm_andnot_ps(is_zero_mask, det), _mm_and_ps(is_zero_mask, _mm_set_ss(1.f))));
#ifdef ZERO_SINGULAR
		rd = _mm_and_ps(_mm_cmpneq_ss(det,_mm_setzero_ps()), rd);
#endif

		//  iB = D * (A#B)# = D*B#*A
		iB.setMul(D, _mm_shuffle_ps(AB,AB,0x33));
		iB.sub(_mm_mul_ps(_mm_shuffle_ps(D,D,0xB1), _mm_shuffle_ps(AB,AB,0x66)));
		//  iC = A * (D#C)# = A*C#*D
		iC.setMul(A, _mm_shuffle_ps(DC,DC,0x33));
		iC.sub(_mm_mul_ps(_mm_shuffle_ps(A,A,0xB1), _mm_shuffle_ps(DC,DC,0x66)));

		rd = _mm_shuffle_ps(rd,rd,0);
		rd = _mm_xor_ps(rd, _mm_load_ps((const float*)Sign_PNNP));

		//  iB = C*|B| - D*B#*A
		iB.setSub(_mm_mul_ps(C,_mm_shuffle_ps(dB,dB,0)), iB);

		//  iC = B*|C| - A*C#*D;
		iC.setSub(_mm_mul_ps(B,_mm_shuffle_ps(dC,dC,0)), iC);


		//  iX = iX / det
		iA.mul(rd);
		iB.mul(rd);
		iC.mul(rd);
		iD.mul(rd);

		mMatrix[0] = _mm_shuffle_ps(iA,iB,0x77);
		mMatrix[1] = _mm_shuffle_ps(iA,iB,0x22);
		mMatrix[2] = _mm_shuffle_ps(iC,iD,0x77);
		mMatrix[3] = _mm_shuffle_ps(iC,iD,0x22);

		return *(float*)&det;
	}
};

#endif
