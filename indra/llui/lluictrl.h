/** 
 * @file lluictrl.h
 * @author James Cook, Richard Nelson, Tom Yedwab
 * @brief Abstract base class for UI controls
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

#ifndef LL_LLUICTRL_H
#define LL_LLUICTRL_H

#include "llview.h"
#include "llrect.h"
#include "llsd.h"
#include <boost/function.hpp>
#include <boost/signals2.hpp>

#include "llviewmodel.h"		// *TODO move dependency to .cpp file

class LLUICtrl
: public LLView, public boost::signals2::trackable
{
public:
	typedef boost::function<void (LLUICtrl* ctrl, const LLSD& param)> commit_callback_t;
	typedef boost::signals2::signal<void (LLUICtrl* ctrl, const LLSD& param)> commit_signal_t;
	typedef boost::signals2::signal<bool (LLUICtrl* ctrl, const LLSD& param), boost_boolean_combiner> enable_signal_t;

	typedef void (*LLUICtrlCallback)(LLUICtrl* ctrl, void* userdata);
	typedef BOOL (*LLUICtrlValidate)(LLUICtrl* ctrl, void* userdata);

	LLUICtrl();
	LLUICtrl( const std::string& name, const LLRect& rect, BOOL mouse_opaque,
		LLUICtrlCallback callback,
		void* callback_userdata,
		U32 reshape=FOLLOWS_NONE);
	/*virtual*/ ~LLUICtrl();

	// We need this virtual so we can override it with derived versions
	virtual LLViewModel* getViewModel() const;
    // We shouldn't ever need to set this directly
    //virtual void    setViewModel(const LLViewModelPtr&);
	// LLView interface
	/*virtual*/ void	initFromXML(LLXMLNodePtr node, LLView* parent);
	/*virtual*/ LLXMLNodePtr getXML(bool save_children = true) const;
	/*virtual*/ BOOL	setLabelArg( const std::string& key, const LLStringExplicit& text );
	/*virtual*/ BOOL	isCtrl() const;

	// From LLFocusableElement
	/*virtual*/ void	setFocus( BOOL b );
	/*virtual*/ BOOL	hasFocus() const;
	
	// New virtuals

	// Return NULL by default (overrride if the class has the appropriate interface)
	virtual class LLCtrlSelectionInterface* getSelectionInterface();
	virtual class LLCtrlListInterface* getListInterface();
	virtual class LLCtrlScrollInterface* getScrollInterface();

	virtual void	setTentative(BOOL b);
	virtual BOOL	getTentative() const;
	virtual void	setValue(const LLSD& value);
	virtual LLSD	getValue() const;
    /// When two widgets are displaying the same data (e.g. during a skin
    /// change), share their ViewModel.
    virtual void    shareViewModelFrom(const LLUICtrl& other);

	virtual BOOL	setTextArg(  const std::string& key, const LLStringExplicit& text );
	virtual void	setIsChrome(BOOL is_chrome);

	virtual BOOL	acceptsTextInput() const; // Defaults to false

	// A control is dirty if the user has modified its value.
	// Editable controls should override this.
	virtual BOOL	isDirty() const; // Defauls to false
	virtual void	resetDirty(); //Defaults to no-op
	
	// Call appropriate callbacks
	virtual void	onCommit();
	
	// Default to no-op:
	virtual void	onTabInto();
	virtual void	clear();
	virtual void	setColor(const LLColor4& color);
	virtual void	setAlpha(F32 alpha);
	virtual void	setMinValue(LLSD min_value);
	virtual void	setMaxValue(LLSD max_value);

	BOOL	focusNextItem(BOOL text_entry_only);
	BOOL	focusPrevItem(BOOL text_entry_only);
	BOOL 	focusFirstItem(BOOL prefer_text_fields = FALSE, BOOL focus_flash = TRUE );
	BOOL	focusLastItem(BOOL prefer_text_fields = FALSE);

	// Non Virtuals
	BOOL			getIsChrome() const;
	
	void			setTabStop( BOOL b );
	BOOL			hasTabStop() const;

	LLUICtrl*		getParentUICtrl() const;

	//Start using these!
	boost::signals2::connection setCommitCallback( const commit_signal_t::slot_type& cb );
	boost::signals2::connection setValidateCallback( const enable_signal_t::slot_type& cb );

	// *TODO: Deprecate; for backwards compatability only:
	//Keeping userdata around with legacy setCommitCallback because it's used ALL OVER THE PLACE.
	void*			getCallbackUserData() const								{ return mCallbackUserData; }
	void			setCallbackUserData( void* data )						{ mCallbackUserData = data; }
	
	void			setCommitCallback( void (*cb)(LLUICtrl*, void*) )		{ mCommitCallback = cb; }
	void			setValidateBeforeCommit( BOOL(*cb)(LLUICtrl*, void*) )	{ mValidateCallback = cb; }
	// *TODO: Deprecate; for backwards compatability only:
	boost::signals2::connection setCommitCallback( boost::function<void (LLUICtrl*,void*)> cb, void* data);	
	boost::signals2::connection setValidateBeforeCommit( boost::function<bool (const LLSD& data)> cb );
	
	static LLView* fromXML(LLXMLNodePtr node, LLView* parent, class LLUICtrlFactory* factory);

	LLUICtrl*		findRootMostFocusRoot();

	class LLTextInputFilter : public LLQueryFilter, public LLSingleton<LLTextInputFilter>
	{
		/*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const 
		{
			return filterResult_t(view->isCtrl() && static_cast<const LLUICtrl *>(view)->acceptsTextInput(), TRUE);
		}
	};

protected:

	commit_signal_t*		mCommitSignal;
	enable_signal_t*		mValidateSignal;
	
    LLViewModelPtr  mViewModel;
	void			(*mCommitCallback)( LLUICtrl* ctrl, void* userdata );
	BOOL			(*mValidateCallback)( LLUICtrl* ctrl, void* userdata );

	void*			mCallbackUserData;

private:

	BOOL			mTabStop;
	BOOL			mIsChrome;
	BOOL			mTentative;

	class DefaultTabGroupFirstSorter;
};

#endif  // LL_LLUICTRL_H
