/** 
 * @file llfilepicker.cpp
 * @brief OS-specific file picker
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
#include "llfilepicker.h"
#include "llpreprocessor.h"
#include "llerror.h"
#include "basic_plugin_base.h"		// For PLS_INFOS etc.

#if LL_SDL
#include "llwindowsdl.h" // for some X/GTK utils to help with filepickers
#endif // LL_SDL

// Translation map.
typedef std::map<std::string, std::string> translation_map_type;
translation_map_type translation_map;

// A temporary hack to minimize the number of changes from the original llfilepicker.cpp.
#define LLTrans translation
namespace translation
{
	std::string getString(char const* key)
	{
		translation_map_type::iterator iter = translation_map.find(key);
		return (iter != translation_map.end()) ? iter->second : key;
	}

	void add(std::string const& key, std::string const& translation)
	{
		PLS_INFOS << "Adding translation \"" << key << "\" --> \"" << translation << "\"" << PLS_ENDL;
		translation_map[key] = translation;
	}
}

#if LL_GTK
namespace LLWindowSDL {
	bool ll_try_gtk_init(void)
	{
		static BOOL done_gtk_diag = FALSE;
		static BOOL gtk_is_good = FALSE;
		static BOOL done_setlocale = FALSE;
		static BOOL tried_gtk_init = FALSE;

		if (!done_setlocale)
		{
			PLS_INFOS << "Starting GTK Initialization." << PLS_ENDL;
			//maybe_lock_display();
			gtk_disable_setlocale();
			//maybe_unlock_display();
			done_setlocale = TRUE;
		}

		if (!tried_gtk_init)
		{
			tried_gtk_init = TRUE;
			if (!g_thread_supported ()) g_thread_init (NULL);
			//maybe_lock_display();
			gtk_is_good = gtk_init_check(NULL, NULL);
			//maybe_unlock_display();
			if (!gtk_is_good)
				PLS_WARNS << "GTK Initialization failed." << PLS_ENDL;
		}
		if (gtk_is_good && !done_gtk_diag)
		{
			PLS_INFOS << "GTK Initialized." << PLS_ENDL;
			PLS_INFOS << "- Compiled against GTK version "
				<< GTK_MAJOR_VERSION << "."
				<< GTK_MINOR_VERSION << "."
				<< GTK_MICRO_VERSION << PLS_ENDL;
			PLS_INFOS << "- Running against GTK version "
				<< gtk_major_version << "."
				<< gtk_minor_version << "."
				<< gtk_micro_version << PLS_ENDL;
			//maybe_lock_display();
			const gchar* gtk_warning = gtk_check_version(
				GTK_MAJOR_VERSION,
				GTK_MINOR_VERSION,
				GTK_MICRO_VERSION);
			//maybe_unlock_display();
			if (gtk_warning)
			{
				PLS_WARNS << "- GTK COMPATIBILITY WARNING: " << gtk_warning << PLS_ENDL;
				gtk_is_good = FALSE;
			}
			else
			{
				PLS_INFOS << "- GTK version is good." << PLS_ENDL;
			}
			done_gtk_diag = TRUE;
		}
		return gtk_is_good;
	}
}
#endif

//
// Globals
//

LLFilePicker LLFilePicker::sInstance;

#if LL_WINDOWS
#define SOUND_FILTER L"Sounds (*.wav)\0*.wav\0"
#define IMAGE_FILTER L"Images (*.tga; *.bmp; *.jpg; *.jpeg; *.png)\0*.tga;*.bmp;*.jpg;*.jpeg;*.png\0"
#define ANIM_FILTER L"Animations (*.bvh)\0*.bvh\0"
#ifdef _CORY_TESTING
#define GEOMETRY_FILTER L"SL Geometry (*.slg)\0*.slg\0"
#endif
#define XML_FILTER L"XML files (*.xml)\0*.xml\0"
#define SLOBJECT_FILTER L"Objects (*.slobject)\0*.slobject\0"
#define RAW_FILTER L"RAW files (*.raw)\0*.raw\0"
#endif

//
// Implementation
//
LLFilePickerBase::LLFilePickerBase()
	: mCurrentFile(0),
	  mLocked(FALSE)

{
	reset();

#if LL_WINDOWS
	mOFN.lStructSize = sizeof(OPENFILENAMEW);
	mOFN.hwndOwner = NULL;							// Set later with setWindowID
	mOFN.hInstance = NULL;
	mOFN.lpstrCustomFilter = NULL;
	mOFN.nMaxCustFilter = 0;
	mOFN.lpstrFile = NULL;							// set in open and close
	mOFN.nMaxFile = LL_MAX_PATH;
	mOFN.lpstrFileTitle = NULL;
	mOFN.nMaxFileTitle = 0;
	mOFN.lpstrInitialDir = NULL;
	mOFN.lpstrTitle = NULL;
	mOFN.Flags = 0;									// set in open and close
	mOFN.nFileOffset = 0;
	mOFN.nFileExtension = 0;
	mOFN.lpstrDefExt = NULL;
	mOFN.lCustData = 0L;
	mOFN.lpfnHook = NULL;
	mOFN.lpTemplateName = NULL;
#endif

#if LL_DARWIN
	memset(&mNavOptions, 0, sizeof(mNavOptions));
	OSStatus	error = NavGetDefaultDialogCreationOptions(&mNavOptions);
	if (error == noErr)
	{
		mNavOptions.modality = kWindowModalityAppModal;
	}
#endif
}

const std::string LLFilePickerBase::getFirstFile()
{
	mCurrentFile = 0;
	return getNextFile();
}

const std::string LLFilePickerBase::getNextFile()
{
	if (mCurrentFile >= getFileCount())
	{
		mLocked = FALSE;
		return std::string();
	}
	else
	{
		return mFiles[mCurrentFile++];
	}
}

const std::string LLFilePickerBase::getCurFile()
{
	if (mCurrentFile >= getFileCount())
	{
		mLocked = FALSE;
		return std::string();
	}
	else
	{
		return mFiles[mCurrentFile];
	}
}

void LLFilePickerBase::reset()
{
	mLocked = FALSE;
	mFiles.clear();
	mCurrentFile = 0;
}

#if LL_WINDOWS

bool LLFilePickerBase::setupFilter(ELoadFilter filter)
{
	bool res = TRUE;
	switch (filter)
	{
	case FFLOAD_ALL:
		mOFN.lpstrFilter = L"All Files (*.*)\0*.*\0" \
		SOUND_FILTER \
		IMAGE_FILTER \
		ANIM_FILTER \
		L"\0";
		break;
	case FFLOAD_WAV:
		mOFN.lpstrFilter = SOUND_FILTER \
			L"\0";
		break;
	case FFLOAD_IMAGE:
		mOFN.lpstrFilter = IMAGE_FILTER \
			L"\0";
		break;
	case FFLOAD_ANIM:
		mOFN.lpstrFilter = ANIM_FILTER \
			L"\0";
		break;
#ifdef _CORY_TESTING
	case FFLOAD_GEOMETRY:
		mOFN.lpstrFilter = GEOMETRY_FILTER \
			L"\0";
		break;
#endif
	case FFLOAD_XML:
		mOFN.lpstrFilter = XML_FILTER \
			L"\0";
		break;
	case FFLOAD_SLOBJECT:
		mOFN.lpstrFilter = SLOBJECT_FILTER \
			L"\0";
		break;
	case FFLOAD_RAW:
		mOFN.lpstrFilter = RAW_FILTER \
			L"\0";
		break;
	default:
		res = FALSE;
		break;
	}
	return res;
}

// FIXME: Use folder
bool LLFilePickerBase::getLoadFile(ELoadFilter filter, std::string const& folder)
{
	if( mLocked )
	{
		return FALSE;
	}
	bool success = FALSE;

	// don't provide default file selection
	mFilesW[0] = '\0';

	mOFN.lpstrFile = mFilesW;
	mOFN.nMaxFile = SINGLE_FILENAME_BUFFER_SIZE;
	mOFN.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR ;
	mOFN.nFilterIndex = 1;

	setupFilter(filter);
	
	// Modal, so pause agent
	send_agent_pause();

	reset();
	
	// NOTA BENE: hitting the file dialog triggers a window focus event, destroying the selection manager!!
	success = GetOpenFileName(&mOFN);
	if (success)
	{
		std::string filename = utf16str_to_utf8str(llutf16string(mFilesW));
		mFiles.push_back(filename);
	}
	send_agent_resume();

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}

// FIXME: Use folder
bool LLFilePickerBase::getMultipleLoadFiles(ELoadFilter filter, std::string const& folder)
{
	if( mLocked )
	{
		return FALSE;
	}
	bool success = FALSE;

	// don't provide default file selection
	mFilesW[0] = '\0';

	mOFN.lpstrFile = mFilesW;
	mOFN.nFilterIndex = 1;
	mOFN.nMaxFile = FILENAME_BUFFER_SIZE;
	mOFN.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR |
		OFN_EXPLORER | OFN_ALLOWMULTISELECT;

	setupFilter(filter);

	reset();
	
	// Modal, so pause agent
	send_agent_pause();
	// NOTA BENE: hitting the file dialog triggers a window focus event, destroying the selection manager!!
	success = GetOpenFileName(&mOFN); // pauses until ok or cancel.
	if( success )
	{
		// The getopenfilename api doesn't tell us if we got more than
		// one file, so we have to test manually by checking string
		// lengths.
		if( wcslen(mOFN.lpstrFile) > mOFN.nFileOffset )	/*Flawfinder: ignore*/
		{
			std::string filename = utf16str_to_utf8str(llutf16string(mFilesW));
			mFiles.push_back(filename);
		}
		else
		{
			mLocked = TRUE;
			WCHAR* tptrw = mFilesW;
			std::string dirname;
			while(1)
			{
				if (*tptrw == 0 && *(tptrw+1) == 0) // double '\0'
					break;
				if (*tptrw == 0)
					tptrw++; // shouldn't happen?
				std::string filename = utf16str_to_utf8str(llutf16string(tptrw));
				if (dirname.empty())
					dirname = filename + "\\";
				else
					mFiles.push_back(dirname + filename);
				tptrw += filename.size();
			}
		}
	}
	send_agent_resume();

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}

// FIXME: Use folder
bool LLFilePickerBase::getSaveFile(ESaveFilter filter, std::string const& filename, std::string const& folder)
{
	if( mLocked )
	{
		return FALSE;
	}
	bool success = FALSE;

	mOFN.lpstrFile = mFilesW;
	if (!filename.empty())
	{
		llutf16string tstring = utf8str_to_utf16str(filename);
		wcsncpy(mFilesW, tstring.c_str(), FILENAME_BUFFER_SIZE);	}	/*Flawfinder: ignore*/
	else
	{
		mFilesW[0] = '\0';
	}

	switch( filter )
	{
	case FFSAVE_ALL:
		mOFN.lpstrDefExt = NULL;
		mOFN.lpstrFilter =
			L"All Files (*.*)\0*.*\0" \
			L"WAV Sounds (*.wav)\0*.wav\0" \
			L"Targa, Bitmap Images (*.tga; *.bmp)\0*.tga;*.bmp\0" \
			L"\0";
		break;
	case FFSAVE_LSL:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.lsl", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"lsl";
		mOFN.lpstrFilter = 
			L"LSL Files (*.lsl)\0*.lsl\0"
			L"Text files (*.txt)\0*.txt\0"
			L"\0";
		break;
	case FFSAVE_TEXT:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.txt", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"txt";
		mOFN.lpstrFilter = 
			L"Text files (*.txt)\0*.txt\0"
			L"RTF Files (*.rtf)\0*.rtf\0"
			L"\0";
		break;
	case FFSAVE_WAV:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.wav", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"wav";
		mOFN.lpstrFilter =
			L"WAV Sounds (*.wav)\0*.wav\0" \
			L"\0";
		break;
	case FFSAVE_TGA:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.tga", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"tga";
		mOFN.lpstrFilter =
			L"Targa Images (*.tga)\0*.tga\0" \
			L"\0";
		break;
	case FFSAVE_BMP:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.bmp", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"bmp";
		mOFN.lpstrFilter =
			L"Bitmap Images (*.bmp)\0*.bmp\0" \
			L"\0";
		break;
	case FFSAVE_PNG:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.png", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"png";
		mOFN.lpstrFilter =
			L"PNG Images (*.png)\0*.png\0" \
			L"\0";
		break;
	case FFSAVE_JPEG:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.jpeg", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"jpeg";
		mOFN.lpstrFilter =
			L"JPEG Images (*.jpeg)\0*.jpeg\0" \
			L"\0";
		break;
	case FFSAVE_AVI:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.avi", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"avi";
		mOFN.lpstrFilter =
			L"AVI Movie File (*.avi)\0*.avi\0" \
			L"\0";
		break;
	case FFSAVE_ANIM:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.xaf", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"xaf";
		mOFN.lpstrFilter =
			L"XAF Anim File (*.xaf)\0*.xaf\0" \
			L"\0";
		break;
#ifdef _CORY_TESTING
	case FFSAVE_GEOMETRY:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.slg", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"slg";
		mOFN.lpstrFilter =
			L"SLG SL Geometry File (*.slg)\0*.slg\0" \
			L"\0";
		break;
#endif
	case FFSAVE_XML:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.xml", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}

		mOFN.lpstrDefExt = L"xml";
		mOFN.lpstrFilter =
			L"XML File (*.xml)\0*.xml\0" \
			L"\0";
		break;
	case FFSAVE_COLLADA:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.collada", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"collada";
		mOFN.lpstrFilter =
			L"COLLADA File (*.collada)\0*.collada\0" \
			L"\0";
		break;
	case FFSAVE_RAW:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.raw", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"raw";
		mOFN.lpstrFilter =	RAW_FILTER \
							L"\0";
		break;
	case FFSAVE_J2C:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.j2c", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"j2c";
		mOFN.lpstrFilter =
			L"Compressed Images (*.j2c)\0*.j2c\0" \
			L"\0";
		break;
	default:
		return FALSE;
	}

 
	mOFN.nMaxFile = SINGLE_FILENAME_BUFFER_SIZE;
	mOFN.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;

	reset();

	// Modal, so pause agent
	send_agent_pause();
	{
		// NOTA BENE: hitting the file dialog triggers a window focus event, destroying the selection manager!!
		success = GetSaveFileName(&mOFN);
		if (success)
		{
			std::string filename = utf16str_to_utf8str(llutf16string(mFilesW));
			mFiles.push_back(filename);
		}
		gKeyboard->resetKeys();
	}
	send_agent_resume();

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}

#elif LL_DARWIN

Boolean LLFilePickerBase::navOpenFilterProc(AEDesc *theItem, void *info, void *callBackUD, NavFilterModes filterMode)
{
	Boolean		result = true;
	ELoadFilter filter = *((ELoadFilter*) callBackUD);
	OSStatus	error = noErr;
	
	if (filterMode == kNavFilteringBrowserList && filter != FFLOAD_ALL && (theItem->descriptorType == typeFSRef || theItem->descriptorType == typeFSS))
	{
		// navInfo is only valid for typeFSRef and typeFSS
		NavFileOrFolderInfo	*navInfo = (NavFileOrFolderInfo*) info;
		if (!navInfo->isFolder)
		{
			AEDesc	desc;
			error = AECoerceDesc(theItem, typeFSRef, &desc);
			if (error == noErr)
			{
				FSRef	fileRef;
				error = AEGetDescData(&desc, &fileRef, sizeof(fileRef));
				if (error == noErr)
				{
					LSItemInfoRecord	fileInfo;
					error = LSCopyItemInfoForRef(&fileRef, kLSRequestExtension | kLSRequestTypeCreator, &fileInfo);
					if (error == noErr)
					{
						if (filter == FFLOAD_IMAGE)
						{
							if (fileInfo.filetype != 'JPEG' && fileInfo.filetype != 'JPG ' && 
								fileInfo.filetype != 'BMP ' && fileInfo.filetype != 'TGA ' &&
								fileInfo.filetype != 'TIFF' && fileInfo.filetype != 'PSD ' &&
								fileInfo.filetype != 'BMPf' && fileInfo.filetype != 'TPIC' &&
								fileInfo.filetype != 'PNG ' &&
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("jpeg"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("jpg"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("bmp"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("tga"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("psd"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("tiff"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("tif"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("png"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
								)
							{
								result = false;
							}
						}
						else if (filter == FFLOAD_WAV)
						{
							if (fileInfo.filetype != 'WAVE' && fileInfo.filetype != 'WAV ' && 
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("wave"), kCFCompareCaseInsensitive) != kCFCompareEqualTo && 
								CFStringCompare(fileInfo.extension, CFSTR("wav"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
							)
							{
								result = false;
							}
						}
						else if (filter == FFLOAD_ANIM)
						{
							if (fileInfo.filetype != 'BVH ' && 
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("bvh"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
							)
							{
								result = false;
							}
						}
#ifdef _CORY_TESTING
						else if (filter == FFLOAD_GEOMETRY)
						{
							if (fileInfo.filetype != 'SLG ' && 
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("slg"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
							)
							{
								result = false;
							}
						}
#endif
						else if (filter == FFLOAD_SLOBJECT)
						{
							PLS_WARNS << "*** navOpenFilterProc: FFLOAD_SLOBJECT NOT IMPLEMENTED ***" << PLS_ENDL;
						}
						else if (filter == FFLOAD_RAW)
						{
							if (fileInfo.filetype != '\?\?\?\?' && 
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("raw"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
							)
							{
								result = false;
							}
						}
						
						if (fileInfo.extension)
						{
							CFRelease(fileInfo.extension);
						}
					}
				}
				AEDisposeDesc(&desc);
			}
		}
	}
	return result;
}

OSStatus	LLFilePickerBase::doNavChooseDialog(ELoadFilter filter)
{
	OSStatus		error = noErr;
	NavDialogRef	navRef = NULL;
	NavReplyRecord	navReply;

	memset(&navReply, 0, sizeof(navReply));
	
	// NOTE: we are passing the address of a local variable here.  
	//   This is fine, because the object this call creates will exist for less than the lifetime of this function.
	//   (It is destroyed by NavDialogDispose() below.)
	error = NavCreateChooseFileDialog(&mNavOptions, NULL, NULL, NULL, navOpenFilterProc, (void*)(&filter), &navRef);

	//gViewerWindow->mWindow->beforeDialog();

	if (error == noErr)
		error = NavDialogRun(navRef);

	//gViewerWindow->mWindow->afterDialog();

	if (error == noErr)
		error = NavDialogGetReply(navRef, &navReply);

	if (navRef)
		NavDialogDispose(navRef);

	if (error == noErr && navReply.validRecord)
	{
		SInt32	count = 0;
		SInt32	index;
		
		// AE indexes are 1 based...
		error = AECountItems(&navReply.selection, &count);
		for (index = 1; index <= count; index++)
		{
			FSRef		fsRef;
			AEKeyword	theAEKeyword;
			DescType	typeCode;
			Size		actualSize = 0;
			char		path[MAX_PATH];	/*Flawfinder: ignore*/
			
			memset(&fsRef, 0, sizeof(fsRef));
			error = AEGetNthPtr(&navReply.selection, index, typeFSRef, &theAEKeyword, &typeCode, &fsRef, sizeof(fsRef), &actualSize);
			
			if (error == noErr)
				error = FSRefMakePath(&fsRef, (UInt8*) path, sizeof(path));
			
			if (error == noErr)
				mFiles.push_back(std::string(path));
		}
	}
	
	return error;
}

OSStatus	LLFilePickerBase::doNavSaveDialog(ESaveFilter filter, const std::string& filename)
{
	OSStatus		error = noErr;
	NavDialogRef	navRef = NULL;
	NavReplyRecord	navReply;
	
	memset(&navReply, 0, sizeof(navReply));
	
	// Setup the type, creator, and extension
	OSType		type, creator;
	CFStringRef	extension = NULL;
	switch (filter)
	{
		case FFSAVE_WAV:
			type = 'WAVE';
			creator = 'TVOD';
			extension = CFSTR(".wav");
			break;
		
		case FFSAVE_TGA:
			type = 'TPIC';
			creator = 'prvw';
			extension = CFSTR(".tga");
			break;
		
		case FFSAVE_BMP:
			type = 'BMPf';
			creator = 'prvw';
			extension = CFSTR(".bmp");
			break;
		case FFSAVE_JPEG:
			type = 'JPEG';
			creator = 'prvw';
			extension = CFSTR(".jpeg");
			break;
		case FFSAVE_PNG:
			type = 'PNG ';
			creator = 'prvw';
			extension = CFSTR(".png");
			break;
		case FFSAVE_AVI:
			type = '\?\?\?\?';
			creator = '\?\?\?\?';
			extension = CFSTR(".mov");
			break;

		case FFSAVE_ANIM:
			type = '\?\?\?\?';
			creator = '\?\?\?\?';
			extension = CFSTR(".xaf");
			break;

#ifdef _CORY_TESTING
		case FFSAVE_GEOMETRY:
			type = '\?\?\?\?';
			creator = '\?\?\?\?';
			extension = CFSTR(".slg");
			break;
#endif		
		case FFSAVE_RAW:
			type = '\?\?\?\?';
			creator = '\?\?\?\?';
			extension = CFSTR(".raw");
			break;

		case FFSAVE_J2C:
			type = '\?\?\?\?';
			creator = 'prvw';
			extension = CFSTR(".j2c");
			break;
		
		case FFSAVE_ALL:
		default:
			type = '\?\?\?\?';
			creator = '\?\?\?\?';
			extension = CFSTR("");
			break;
	}
	
	// Create the dialog
	error = NavCreatePutFileDialog(&mNavOptions, type, creator, NULL, NULL, &navRef);
	if (error == noErr)
	{
		CFStringRef	nameString = NULL;
		bool		hasExtension = true;
		
		// Create a CFString of the initial file name
		if (!filename.empty())
			nameString = CFStringCreateWithCString(NULL, filename.c_str(), kCFStringEncodingUTF8);
		else
			nameString = CFSTR("Untitled");
			
		// Add the extension if one was not provided
		if (nameString && !CFStringHasSuffix(nameString, extension))
		{
			CFStringRef	tempString = nameString;
			hasExtension = false;
			nameString = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@%@"), tempString, extension);
			CFRelease(tempString);
		}
		
		// Set the name in the dialog
		if (nameString)
		{
			error = NavDialogSetSaveFileName(navRef, nameString);
			CFRelease(nameString);
		}
		else
		{
			error = paramErr;
		}
	}
	
	//gViewerWindow->mWindow->beforeDialog();

	// Run the dialog
	if (error == noErr)
		error = NavDialogRun(navRef);

	//gViewerWindow->mWindow->afterDialog();

	if (error == noErr)
		error = NavDialogGetReply(navRef, &navReply);
	
	if (navRef)
		NavDialogDispose(navRef);

	if (error == noErr && navReply.validRecord)
	{
		SInt32	count = 0;
		
		// AE indexes are 1 based...
		error = AECountItems(&navReply.selection, &count);
		if (count > 0)
		{
			// Get the FSRef to the containing folder
			FSRef		fsRef;
			AEKeyword	theAEKeyword;
			DescType	typeCode;
			Size		actualSize = 0;
			
			memset(&fsRef, 0, sizeof(fsRef));
			error = AEGetNthPtr(&navReply.selection, 1, typeFSRef, &theAEKeyword, &typeCode, &fsRef, sizeof(fsRef), &actualSize);
			
			if (error == noErr)
			{
				char	path[PATH_MAX];		/*Flawfinder: ignore*/
				char	newFileName[SINGLE_FILENAME_BUFFER_SIZE];	/*Flawfinder: ignore*/
				
				error = FSRefMakePath(&fsRef, (UInt8*)path, PATH_MAX);
				if (error == noErr)
				{
					if (CFStringGetCString(navReply.saveFileName, newFileName, sizeof(newFileName), kCFStringEncodingUTF8))
					{
						mFiles.push_back(std::string(path) + "/" +  std::string(newFileName));
					}
					else
					{
						error = paramErr;
					}
				}
				else
				{
					error = paramErr;
				}
			}
		}
	}
	
	return error;
}

// FIXME: Use folder
bool LLFilePickerBase::getLoadFile(ELoadFilter filter, std::string const& folder)
{
	if( mLocked )
		return FALSE;

	bool success = FALSE;

	OSStatus	error = noErr;
	
	reset();
	
	mNavOptions.optionFlags &= ~kNavAllowMultipleFiles;
	// Modal, so pause agent
	send_agent_pause();
	{
		error = doNavChooseDialog(filter);
	}
	send_agent_resume();
	if (error == noErr)
	{
		if (getFileCount())
			success = true;
	}

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}

// FIXME: Use folder
bool LLFilePickerBase::getMultipleLoadFiles(ELoadFilter filter, std::string const& folder)
{
	if( mLocked )
		return FALSE;

	bool success = FALSE;

	OSStatus	error = noErr;

	reset();
	
	mNavOptions.optionFlags |= kNavAllowMultipleFiles;
	// Modal, so pause agent
	send_agent_pause();
	{
		error = doNavChooseDialog(filter);
	}
	send_agent_resume();
	if (error == noErr)
	{
		if (getFileCount())
			success = true;
		if (getFileCount() > 1)
			mLocked = TRUE;
	}

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}

// FIXME: Use folder
bool LLFilePickerBase::getSaveFile(ESaveFilter filter, std::string const& filename, std::string const& folder)
{
	if( mLocked )
		return FALSE;
	bool success = FALSE;
	OSStatus	error = noErr;

	reset();
	
	mNavOptions.optionFlags &= ~kNavAllowMultipleFiles;

	// Modal, so pause agent
	send_agent_pause();
	{
		error = doNavSaveDialog(filter, filename);
	}
	send_agent_resume();
	if (error == noErr)
	{
		if (getFileCount())
			success = true;
	}

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}

#elif LL_LINUX || LL_SOLARIS

# if LL_GTK

// static
void LLFilePickerBase::add_to_selectedfiles(gpointer data, gpointer user_data)
{
	// We need to run g_filename_to_utf8 in the user's locale
	std::string saved_locale(setlocale(LC_ALL, NULL));
	setlocale(LC_ALL, "");

	LLFilePickerBase* picker = (LLFilePickerBase*) user_data;
	GError *error = NULL;
//	gchar* filename_utf8 = g_filename_to_utf8((gchar*)data,
//						  -1, NULL, NULL, &error);
	gchar* filename_utf8 = g_filename_display_name ((gchar*) data);
	if (error)
	{
		// *FIXME.
		// This condition should really be notified to the user, e.g.
		// through a message box.  Just logging it is inappropriate.
		
		// Ghhhh.  g_filename_display_name is new to glib 2.6, and it
		// is too new for SL! (Note that the latest glib as of this
		// writing is 2.22. *sigh*) LL supplied *makeASCII family are
		// also unsuitable since they allow control characters...

		// muhahaha ... Imprudence can !


		PLS_WARNS << "g_filename_display_name failed on"  << filename_utf8  << ": "<< error->message << PLS_ENDL;
	}

	if (filename_utf8)
	{
		picker->mFiles.push_back(std::string(filename_utf8));
		PLS_DEBUGS << "ADDED FILE " << filename_utf8 << PLS_ENDL;
		g_free(filename_utf8);
	}

	setlocale(LC_ALL, saved_locale.c_str());
}

// static
void LLFilePickerBase::chooser_responder(GtkWidget *widget, gint response, gpointer user_data)
{
	LLFilePicker* picker = (LLFilePicker*)user_data;

	PLS_DEBUGS << "GTK DIALOG RESPONSE " << response << PLS_ENDL;

	if (response == GTK_RESPONSE_ACCEPT)
	{
		GSList *file_list = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(widget));
		g_slist_foreach(file_list, (GFunc)add_to_selectedfiles, picker);
		g_slist_foreach(file_list, (GFunc)g_free, NULL);
		g_slist_free (file_list);
	}

	gtk_widget_destroy(widget);
	gtk_main_quit();
}


GtkWindow* LLFilePickerBase::buildFilePicker(bool is_save, bool is_folder, std::string const& folder)
{
	if (LLWindowSDL::ll_try_gtk_init())
	{
		GtkWidget *win = NULL;
		GtkFileChooserAction pickertype =
			is_save?
			(is_folder?
			 GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER :
			 GTK_FILE_CHOOSER_ACTION_SAVE) :
			(is_folder?
			 GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER :
			 GTK_FILE_CHOOSER_ACTION_OPEN);

		win = gtk_file_chooser_dialog_new(NULL, NULL,
						  pickertype,
						  GTK_STOCK_CANCEL,
						   GTK_RESPONSE_CANCEL,
						  is_folder ?
						  GTK_STOCK_APPLY :
						  (is_save ? 
						   GTK_STOCK_SAVE :
						   GTK_STOCK_OPEN),
						   GTK_RESPONSE_ACCEPT,
						  (gchar *)NULL);

		if (!folder.empty())
		{
			gtk_file_chooser_set_current_folder
				(GTK_FILE_CHOOSER(win),
				 folder.c_str());
		}

#  if LL_X11
		// Make GTK tell the window manager to associate this
		// dialog with our non-GTK raw X11 window, which should try
		// to keep it on top etc.
		if (mX11WindowID != None)
		{
			gtk_widget_realize(GTK_WIDGET(win)); // so we can get its gdkwin
			GdkWindow *gdkwin = gdk_window_foreign_new(mX11WindowID);
			gdk_window_set_transient_for(GTK_WIDGET(win)->window, gdkwin);
		}
#  endif //LL_X11

		g_signal_connect (GTK_FILE_CHOOSER(win),
				  "response",
				  G_CALLBACK(LLFilePickerBase::chooser_responder),
				  this);

		gtk_window_set_modal(GTK_WINDOW(win), TRUE);

		/* GTK 2.6: if (is_folder)
			gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(win),
			TRUE); */

		return GTK_WINDOW(win);
	}
	else
	{
		return NULL;
	}
}

static void add_common_filters_to_gtkchooser(GtkFileFilter *gfilter,
					     GtkWindow *picker,
					     std::string filtername)
{	
	gtk_file_filter_set_name(gfilter, filtername.c_str());
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(picker),
				    gfilter);
	GtkFileFilter *allfilter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(allfilter, "*");
	gtk_file_filter_set_name(allfilter, LLTrans::getString("all_files").c_str());
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(picker), allfilter);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(picker), gfilter);
}

static std::string add_simple_pattern_filter_to_gtkchooser(GtkWindow *picker,
							   std::string pattern,
							   std::string filtername)
{
	GtkFileFilter *gfilter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(gfilter, pattern.c_str());
	add_common_filters_to_gtkchooser(gfilter, picker, filtername);
	return filtername;
}

static std::string add_simple_mime_filter_to_gtkchooser(GtkWindow *picker,
							std::string mime,
							std::string filtername)
{
	GtkFileFilter *gfilter = gtk_file_filter_new();
	gtk_file_filter_add_mime_type(gfilter, mime.c_str());
	add_common_filters_to_gtkchooser(gfilter, picker, filtername);
	return filtername;
}

static std::string add_wav_filter_to_gtkchooser(GtkWindow *picker)
{
	GtkFileFilter *gfilter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(gfilter, "*.wav");
	gtk_file_filter_add_mime_type(gfilter,"audio/x-wav");//not working

	std::string filtername = LLTrans::getString("sound_files") + " (*.wav)";
	add_common_filters_to_gtkchooser(gfilter, picker, filtername);
	return filtername;
}

static std::string add_bvh_filter_to_gtkchooser(GtkWindow *picker)
{
	return add_simple_pattern_filter_to_gtkchooser(picker,  "*.bvh",
						       LLTrans::getString("animation_files") + " (*.bvh)");
}

static std::string add_imageload_filter_to_gtkchooser(GtkWindow *picker)
{
	GtkFileFilter *gfilter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(gfilter, "*.tga");
	gtk_file_filter_add_mime_type(gfilter, "image/jpeg");
	gtk_file_filter_add_mime_type(gfilter, "image/png");
	gtk_file_filter_add_mime_type(gfilter, "image/bmp");
	std::string filtername = LLTrans::getString("image_files") + " (*.tga; *.bmp; *.jpg; *.png)";
	add_common_filters_to_gtkchooser(gfilter, picker, filtername);
	return filtername;
}


bool LLFilePickerBase::getSaveFile(ESaveFilter filter, std::string const& filename, std::string const& folder)
{
	bool rtn = FALSE;

	//gViewerWindow->mWindow->beforeDialog();

	reset();
	
	GtkWindow* picker = buildFilePicker(true, false, folder);

	if (picker)
	{
		std::string suggest_name = "untitled";
		std::string suggest_ext = "";
		std::string caption = LLTrans::getString("save_file_verb") + " ";
		switch (filter)
		{
		case FFSAVE_WAV:
			caption += add_wav_filter_to_gtkchooser(picker);
			suggest_ext = ".wav";
			break;
		case FFSAVE_TGA:
			caption += add_simple_pattern_filter_to_gtkchooser
				(picker, "*.tga", LLTrans::getString("targa_image_files") + " (*.tga)");
			suggest_ext = ".tga";
			break;
		case FFSAVE_BMP:
			caption += add_simple_mime_filter_to_gtkchooser
				(picker, "image/bmp", LLTrans::getString("bitmap_image_files") + " (*.bmp)");
			suggest_ext = ".bmp";
			break;
		case FFSAVE_AVI:
			caption += add_simple_mime_filter_to_gtkchooser
				(picker, "video/x-msvideo",
				 LLTrans::getString("avi_movie_file") + " (*.avi)");
			suggest_ext = ".avi";
			break;
		case FFSAVE_ANIM:
			caption += add_simple_pattern_filter_to_gtkchooser
				(picker, "*.xaf", LLTrans::getString("xaf_animation_file") + " (*.xaf)");
			suggest_ext = ".xaf";
			break;
		case FFSAVE_XML:
			caption += add_simple_pattern_filter_to_gtkchooser
				(picker, "*.xml", LLTrans::getString("xml_file") + " (*.xml)");
			suggest_ext = ".xml";
			break;
		case FFSAVE_RAW:
			caption += add_simple_pattern_filter_to_gtkchooser
				(picker, "*.raw", LLTrans::getString("raw_file") + " (*.raw)");
			suggest_ext = ".raw";
			break;
		case FFSAVE_J2C:
			caption += add_simple_mime_filter_to_gtkchooser
				(picker, "images/jp2",
				 LLTrans::getString("compressed_image_files") + " (*.j2c)");
			suggest_ext = ".j2c";
			break;
		default:;
			break;
		}
		
		gtk_window_set_title(GTK_WINDOW(picker), caption.c_str());

		if (filename.empty())
		{
			suggest_name += suggest_ext;

			gtk_file_chooser_set_current_name
				(GTK_FILE_CHOOSER(picker),
				 suggest_name.c_str());
		}
		else
		{
			gtk_file_chooser_set_current_name
				(GTK_FILE_CHOOSER(picker), filename.c_str());
		}

		gtk_widget_show_all(GTK_WIDGET(picker));
		gtk_main();

		rtn = (getFileCount() == 1);
	}

	//gViewerWindow->mWindow->afterDialog();

	return rtn;
}

bool LLFilePickerBase::getLoadFile(ELoadFilter filter, std::string const& folder)
{
	bool rtn = FALSE;

	//gViewerWindow->mWindow->beforeDialog();

	reset();
	
	GtkWindow* picker = buildFilePicker(false, false, folder);

	if (picker)
	{
		std::string caption = LLTrans::getString("load_file_verb") + " ";
		std::string filtername = "";
		switch (filter)
		{
		case FFLOAD_WAV:
			filtername = add_wav_filter_to_gtkchooser(picker);
			break;
		case FFLOAD_ANIM:
			filtername = add_bvh_filter_to_gtkchooser(picker);
			break;
		case FFLOAD_IMAGE:
			filtername = add_imageload_filter_to_gtkchooser(picker);
			break;
		default:;
			break;
		}

		caption += filtername;
		
		gtk_window_set_title(GTK_WINDOW(picker), caption.c_str());

		gtk_widget_show_all(GTK_WIDGET(picker));
		gtk_main();

		rtn = (getFileCount() == 1);
	}

	//gViewerWindow->mWindow->afterDialog();

	return rtn;
}

bool LLFilePickerBase::getMultipleLoadFiles(ELoadFilter filter, std::string const& folder)
{
	bool rtn = FALSE;

	//gViewerWindow->mWindow->beforeDialog();

	reset();
	
	GtkWindow* picker = buildFilePicker(false, false, folder);

	if (picker)
	{
		gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(picker),
						      TRUE);

		gtk_window_set_title(GTK_WINDOW(picker), LLTrans::getString("load_files").c_str());

		gtk_widget_show_all(GTK_WIDGET(picker));
		gtk_main();
		rtn = !mFiles.empty();
	}

	//gViewerWindow->mWindow->afterDialog();

	return rtn;
}

# else // LL_GTK

// Hacky stubs designed to facilitate fake getSaveFile and getLoadFile with
// static results, when we don't have a real filepicker.

bool LLFilePickerBase::getSaveFile(ESaveFilter filter, std::string const& filename, std::string const& folder)
{
	reset();
	
	PLS_INFOS << "getSaveFile suggested filename is [" << filename << "]" << PLS_ENDL;
	if (!filename.empty())
	{
		mFiles.push_back(gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter() + filename);
		return TRUE;
	}
	return FALSE;
}

// FIXME: Use folder
bool LLFilePickerBase::getLoadFile(ELoadFilter filter, std::string const& folder)
{
	reset();
	
	// HACK: Static filenames for 'open' until we implement filepicker
	std::string filename = gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter() + "upload";
	switch (filter)
	{
	case FFLOAD_WAV: filename += ".wav"; break;
	case FFLOAD_IMAGE: filename += ".tga"; break;
	case FFLOAD_ANIM: filename += ".bvh"; break;
	default: break;
	}
	mFiles.push_back(filename);
	PLS_INFOS << "getLoadFile: Will try to open file: " << hackyfilename << PLS_ENDL;
	return TRUE;
}

bool LLFilePickerBase::getMultipleLoadFiles(ELoadFilter filter, std::string const& folder)
{
	reset();
	return FALSE;
}

#endif // LL_GTK

#else // not implemented

bool LLFilePickerBase::getSaveFile(ESaveFilter filter, std::string const& filename, std::string const& folder)
{
	reset();	
	return FALSE;
}

bool LLFilePickerBase::getLoadFile(ELoadFilter filter, std::string const& folder)
{
	reset();
	return FALSE;
}

bool LLFilePickerBase::getMultipleLoadFiles(ELoadFilter filter, std::string const& folder)
{
	reset();
	return FALSE;
}

#endif
