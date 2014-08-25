/*
 * @file llmediafilter.h
 * @brief Definitions for paranoia controls
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Sione Lomu.
 * Copyright (C) 2014, Cinder Roxley.
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
 * $/LicenseInfo$
 */

#ifndef LL_MEDIAFILTER_H
#define LL_MEDIAFILTER_H

class LLParcel;

class LLMediaFilter : public LLSingleton<LLMediaFilter>
{
public:
	typedef enum e_media_list {
		WHITELIST,
		BLACKLIST
	} EMediaList;

	typedef std::list<std::string> string_list_t;
	typedef std::vector<std::string> string_vec_t;
	typedef boost::signals2::signal<void(EMediaList list)> media_list_signal_t;
	boost::signals2::connection setMediaListUpdateCallback(const media_list_signal_t::slot_type& cb)
	{
		return mMediaListUpdate.connect(cb);
	}

	LLMediaFilter();
	void filterMediaUrl(LLParcel* parcel);
	void filterAudioUrl(const std::string& url);
	//void filterSharedMediaUrl

	void addToMediaList(const std::string& in_url, EMediaList list, bool extract = true);
	void removeFromMediaList(string_vec_t, EMediaList list);
	string_list_t getWhiteList() const { return mWhiteList; }
	string_list_t getBlackList() const { return mBlackList; }
	U32 getQueuedMediaCommand() const { return mMediaCommandQueue; }
	void setQueuedMediaCommand(U32 command) { mMediaCommandQueue = command; }
	bool isAlertActive() const { return mAlertActive; }
	void setAlertStatus(bool active) { mAlertActive = active; }
	LLParcel* getCurrentParcel() const { return mCurrentParcel; }
	LLParcel* getQueuedMedia() const { return mMediaQueue; }
	void clearQueuedMedia() { mMediaQueue = NULL; }
	std::string getQueuedAudio() const { return mAudioQueue; }
	void clearQueuedAudio() { mAudioQueue.clear(); }
	void setCurrentAudioURL(const std::string url ) { mCurrentAudioURL = url; }
	void clearCurrentAudioURL() { mCurrentAudioURL.clear(); }
	bool filter(const std::string& url, EMediaList list);

private:
	void loadMediaFilterFromDisk();
	void saveMediaFilterToDisk() const;

	media_list_signal_t mMediaListUpdate;
	string_list_t mBlackList;
	string_list_t mWhiteList;
	U32 mMediaCommandQueue;
	LLParcel* mCurrentParcel;
	LLParcel* mMediaQueue;
	std::string mAudioQueue;
	std::string mCurrentAudioURL;
	std::string mCurrentMediaURL;
	bool mAlertActive;
	//typedef enum e_audio_state {
	//	PLAY,
	//	STOP
	//} EAudioState;
	//EAudioState mAudioState;

};

#endif // LL_MEDIAFILTER_H
