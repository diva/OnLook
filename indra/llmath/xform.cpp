/** 
 * @file xform.cpp
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "linden_common.h"

#include "xform.h"

LLXform::LLXform()
{
	init();
}

LLXform::~LLXform()
{
}

// Link optimization - don't inline these llwarns
void LLXform::warn(const char* const msg)
{
	llwarns << msg << llendl;
}

LLXform* LLXform::getRoot() const
{
	const LLXform* root = this;
	while(root->mParent)
	{
		root = root->mParent;
	}
	return (LLXform*)root;
}

BOOL LLXform::isRoot() const
{
	return (!mParent);
}

BOOL LLXform::isRootEdit() const
{
	return (!mParent);
}

LLXformMatrix::~LLXformMatrix()
{
}

void LLXformMatrix::update()
{
	if (mParent) 
	{
		mWorldPosition = mPosition;
		if (mParent->getScaleChildOffset())
		{
			mWorldPosition.scaleVec(mParent->getScale());
		}
		mWorldPosition *= mParent->getWorldRotation();
		mWorldPosition += mParent->getWorldPosition();
		mWorldRotation = mRotation * mParent->getWorldRotation();
	}
	else
	{
		mWorldPosition = mPosition;
		mWorldRotation = mRotation;
	}
}

void LLXformMatrix::updateMatrix(BOOL update_bounds)
{
	update();

	LLMatrix4 world_matrix;
	world_matrix.initAll(mScale, mWorldRotation, mWorldPosition);
	mWorldMatrix.loadu(world_matrix);

	if (update_bounds && (mChanged & MOVED))
	{
		mMax = mMin = mWorldMatrix.getRow<3>();

		LLVector4a total_sum,sum1,sum2;
		total_sum.setAbs(mWorldMatrix.getRow<0>());
		sum1.setAbs(mWorldMatrix.getRow<1>());
		sum2.setAbs(mWorldMatrix.getRow<2>());
		sum1.add(sum2);
		total_sum.add(sum1);
		total_sum.mul(.5f);

		mMax.add(total_sum);
		mMin.sub(total_sum);
	}
}

void LLXformMatrix::getMinMax(LLVector3& min, LLVector3& max) const
{
	min.set(mMin.getF32ptr());
	max.set(mMax.getF32ptr());
}
