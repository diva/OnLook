/* Copyright (c) 2006-2010, Linden Research, Inc.
 *
 * LLQtWebKit Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in GPL-license.txt in this distribution, or online at
 * http://secondlifegrid.net/technology-programs/license-virtual-world/viewerlicensing/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/technology-programs/license-virtual-world/viewerlicensing/flossexception
 *
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 *
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 */

#include <QDebug>
#include "lljsobject.h"

LLJsObject::LLJsObject( QObject* parent ) :
	QObject( parent )
{
	mEnabled = false;
	
	mAgentLanguage = QString();
	mAgentMaturity = QString();
	mAgentRegion = QString();

	mAgentLocation[ "x" ] = 0.0;
	mAgentLocation[ "y" ] = 0.0;
	mAgentLocation[ "z" ] = 0.0;

	mAgentGlobalLocation[ "x" ] = 0.0;
	mAgentGlobalLocation[ "y" ] = 0.0;
	mAgentGlobalLocation[ "z" ] = 0.0;
}

void LLJsObject::setSLObjectEnabled( bool enabled )
{
	mEnabled = enabled;
}

bool LLJsObject::getSLObjectEnabled()
{
	return mEnabled;
}

void LLJsObject::setAgentLanguage( const QString& agent_language )
{
	if ( mEnabled )
	{
		mAgentLanguage = agent_language;
	}
	else
	{
		mAgentLanguage = QString();
	}
}

void LLJsObject::setAgentRegion( const QString& agent_region )
{
	if ( mEnabled )
	{
		mAgentRegion = agent_region;
	}
	else
	{
		mAgentRegion = QString();
	}
}

void LLJsObject::setAgentMaturity( const QString& agent_maturity )
{
	if ( mEnabled )
	{
		mAgentMaturity = agent_maturity;
	}
	else
	{
		mAgentMaturity = QString();
	}
}

void LLJsObject::setAgentLocation( const QVariantMap agent_location )
{
	if ( mEnabled )
	{
		mAgentLocation = agent_location;
	}
	else
	{
		mAgentLocation[ "x" ] = 0.0;
		mAgentLocation[ "y" ] = 0.0;
		mAgentLocation[ "z" ] = 0.0;
	}
}

void LLJsObject::setAgentGlobalLocation( const QVariantMap agent_global_location )
{
	if ( mEnabled )
	{
		mAgentGlobalLocation = agent_global_location;
	}
	else
	{
		mAgentGlobalLocation[ "x" ] = 0.0;
		mAgentGlobalLocation[ "y" ] = 0.0;
		mAgentGlobalLocation[ "z" ] = 0.0;
	}
}

void LLJsObject::setAgentOrientation( const double angle  )
{
	if ( mEnabled )
	{
		mAgentOrientation = angle;
	}
	else
	{
		mAgentOrientation = 0.0;
	}
}

void LLJsObject::emitLocation()
{
	QVariantMap agent_location;
	
	agent_location[ "region" ] = mAgentRegion;
	agent_location[ "location" ] = mAgentLocation;
	agent_location[ "orientation" ] = mAgentOrientation;
	agent_location[ "globalLocation" ] = mAgentGlobalLocation;
	
	emit getLocation( agent_location );
}

void LLJsObject::emitMaturity()
{
	emit getMaturity( mAgentMaturity );
}

void LLJsObject::emitLanguage()
{
	emit getLanguage( mAgentLanguage );
}
