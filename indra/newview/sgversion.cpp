/* Copyright (C) 2012 Siana Gearz
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

#include "llviewerprecompiledheaders.h"

#include "llversionviewer.h"

#include "sgversion.h"

const S32 gVersionMajor = LL_VERSION_MAJOR;
const S32 gVersionMinor = LL_VERSION_MINOR;
const S32 gVersionPatch = LL_VERSION_PATCH;
const S32 gVersionBuild = LL_VERSION_BUILD;

const char* gVersionChannel = LL_CHANNEL;

#if LL_DARWIN
const char* gVersionBundleID = LL_VERSION_BUNDLE_ID;
#endif