/* Copyright (C) 2013 Liru Færs
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA */

#ifndef LFSIMFEATUREHANDLER_H
#define LFSIMFEATUREHANDLER_H

#include "llsingleton.h"
#include "llpermissions.h"

template<typename Type>
class SignaledType
{
public:
	SignaledType(Type b) : mValue(b) {}

	template<typename Slot>
	boost::signals2::connection connect(Slot slot) { return mSignal.connect(slot); }

	SignaledType& operator =(Type val)
	{
		if (val != mValue)
		{
			mValue = val;
			mSignal();
		}
		return *this;
	}
	operator Type() const { return mValue; }

private:
	boost::signals2::signal<void()> mSignal;
	Type mValue;
};

class LFSimFeatureHandler : public LFSimFeatureHandlerInterface, public LLSingleton<LFSimFeatureHandler>
{
protected:
	friend class LLSingleton<LFSimFeatureHandler>;
	LFSimFeatureHandler();

public:
	void handleRegionChange();
	void setSupportedFeatures();

	// Connection setters
	boost::signals2::connection setSupportsExportCallback(const boost::signals2::signal<void()>::slot_type& slot);

	// Accessors
	/*virtual*/ bool simSupportsExport() const { return mSupportsExport; }

private:
	// SignaledTypes
	SignaledType<bool> mSupportsExport;
};

#endif //LFSIMFEATUREHANDLER_H

