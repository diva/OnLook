/**
 * @file llassetuploadresponders.h
 * @brief Processes responses received for asset upload requests.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#ifndef LL_LLASSETUPLOADRESPONDER_H
#define LL_LLASSETUPLOADRESPONDER_H

#include "llhttpclient.h"
#include "llinventory.h"

void on_new_single_inventory_upload_complete(LLAssetType::EType asset_type,
											 LLInventoryType::EType inventory_type,
											 const std::string inventory_type_string,
											 const LLUUID& item_folder_id,
											 const std::string& item_name,
											 const std::string& item_description,
											 const LLSD& server_response,
											 S32 upload_price);

// Abstract class for supporting asset upload
// via capabilities
class LLAssetUploadResponder : public LLHTTPClient::ResponderWithResult
{
protected:
	LOG_CLASS(LLAssetUploadResponder);
public:
	LLAssetUploadResponder(const LLSD& post_data,
							const LLUUID& vfile_id,
							LLAssetType::EType asset_type);
	LLAssetUploadResponder(const LLSD& post_data, 
							const std::string& file_name,
							LLAssetType::EType asset_type);
	~LLAssetUploadResponder();

protected:
	virtual void httpFailure();
	virtual void httpSuccess();

public:
	virtual void uploadUpload(const LLSD& content);
	virtual void uploadComplete(const LLSD& content);
	virtual void uploadFailure(const LLSD& content);

protected:
	LLSD mPostData;
	LLAssetType::EType mAssetType;
	LLUUID mVFileID;
	std::string mFileName;
};

// TODO*: Remove this once deprecated
class LLNewAgentInventoryResponder : public LLAssetUploadResponder
{
	LOG_CLASS(LLNewAgentInventoryResponder);
public:
	LLNewAgentInventoryResponder(
		const LLSD& post_data,
		const LLUUID& vfile_id,
		LLAssetType::EType asset_type);
	LLNewAgentInventoryResponder(
		const LLSD& post_data,
		const std::string& file_name,
		LLAssetType::EType asset_type);
	virtual void uploadComplete(const LLSD& content);
	virtual void uploadFailure(const LLSD& content);
	/*virtual*/ char const* getName() const { return "LLNewAgentInventoryResponder"; }
protected:
	virtual void httpFailure();
};

// A base class which goes through and performs some default
// actions for variable price uploads.  If more specific actions
// are needed (such as different confirmation messages, etc.)
// the functions onApplicationLevelError and showConfirmationDialog.
class LLNewAgentInventoryVariablePriceResponder :
	public LLHTTPClient::ResponderWithResult
{
	LOG_CLASS(LLNewAgentInventoryVariablePriceResponder);
public:
	LLNewAgentInventoryVariablePriceResponder(
		const LLUUID& vfile_id,
		LLAssetType::EType asset_type,
		const LLSD& inventory_info);

	LLNewAgentInventoryVariablePriceResponder(
		const std::string& file_name,
		LLAssetType::EType asset_type,
		const LLSD& inventory_info);
	virtual ~LLNewAgentInventoryVariablePriceResponder();

private:
	/* virtual */ void httpFailure();
	/* virtual */ void httpSuccess();

public:
	virtual void onApplicationLevelError(
		const LLSD& error);
	virtual void showConfirmationDialog(
		S32 upload_price,
		S32 resource_cost,
										const std::string& confirmation_url);

private:
	class Impl;
	Impl* mImpl;
};

class LLUpdateAgentInventoryResponder : public LLAssetUploadResponder
{
public:
	LLUpdateAgentInventoryResponder(const LLSD& post_data,
								const LLUUID& vfile_id,
								LLAssetType::EType asset_type);
	LLUpdateAgentInventoryResponder(const LLSD& post_data,
								const std::string& file_name,
											   LLAssetType::EType asset_type);
	virtual void uploadComplete(const LLSD& content);
	/*virtual*/ char const* getName() const { return "LLUpdateAgentInventoryResponder"; }
};

class LLUpdateTaskInventoryResponder : public LLAssetUploadResponder
{
public:
	LLUpdateTaskInventoryResponder(const LLSD& post_data,
								const LLUUID& vfile_id,
								LLAssetType::EType asset_type);
	LLUpdateTaskInventoryResponder(const LLSD& post_data,
								const std::string& file_name,
								LLAssetType::EType asset_type);
	LLUpdateTaskInventoryResponder(const LLSD& post_data,
								const std::string& file_name,
								const LLUUID& queue_id,
								LLAssetType::EType asset_type);

	virtual void uploadComplete(const LLSD& content);
	/*virtual*/ char const* getName() const { return "LLUpdateTaskInventoryResponder"; }

private:
	LLUUID mQueueId;
};

#endif // LL_LLASSETUPLOADRESPONDER_H
