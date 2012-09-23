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

#ifndef LLJSOBJECT_H
#define LLJSOBJECT_H

#include <QString>
#include <QObject>
#include <QVariantMap>

class LLJsObject : 
	public QObject
{
    Q_OBJECT

    public:
        LLJsObject( QObject* parent = 0 );

		void setSLObjectEnabled( bool enabled );
		bool getSLObjectEnabled();
        
        void setAgentLanguage( const QString& agent_language );
        void setAgentRegion( const QString& agent_region );
        void setAgentMaturity( const QString& agent_maturity );
		void setAgentLocation( const QVariantMap agent_location );
		void setAgentGlobalLocation( const QVariantMap agent_global_location );			
		void setAgentOrientation( const double angle );
		
		void emitLocation();
		void emitMaturity();
		void emitLanguage();
		
	signals:
		void getLocation( const QVariantMap agent_location );
		void getMaturity( const QString agent_maturity );
		void getLanguage( const QString agent_language );

    private:
    	bool mEnabled;
    	
	    QString mAgentLanguage;
	    QString mAgentMaturity;
	    QString mAgentRegion;
		QVariantMap mAgentLocation;
		QVariantMap mAgentGlobalLocation;
		double mAgentOrientation;
};

#endif	// LLJSOBJECT_H
