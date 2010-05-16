/**
 * @file dohexeditor.h
 * @brief DOHexEditor Widget
 * @author Day Oh
 * 
 * $LicenseInfo:firstyear=2009&license=WTFPLV2$
 *  
 */

// <edit>
#ifndef DO_DOHEXEDITOR_H
#define DO_DOHEXEDITOR_H

#define MIN_COLS 8
#define MAX_COLS 48

#ifndef COLUMN_SPAN
#define COLUMN_SPAN
#endif

#include "lluictrl.h"
#include "llscrollbar.h"
#include "llviewborder.h"
#include "llundo.h"
#include "lleditmenuhandler.h"

class DOHexEditor : public LLUICtrl, public LLEditMenuHandler
{
public:
	DOHexEditor(const std::string& name, const LLRect& rect);
	~DOHexEditor();
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, class LLUICtrlFactory *factory);
	void setValue(const LLSD& value);
	LLSD getValue() const;
	void setColumns(U8 columns);
	U8 getColumns() { return mColumns; };
	U32 getLineCount();
	F32 getSuggestedWidth(U8 cols = -1);
	U32 getProperSelectionStart();
	U32 getProperSelectionEnd();
	void reshape(S32 width, S32 height, BOOL called_from_parent);
	void setFocus(BOOL b);
	
	BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);
	BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	BOOL handleHover(S32 x, S32 y, MASK mask);
	BOOL handleMouseUp(S32 x, S32 y, MASK mask);

	BOOL handleKeyHere(KEY key, MASK mask);
	BOOL handleKey(KEY key, MASK mask, BOOL called_from_parent);
	BOOL handleUnicodeChar(llwchar uni_char, BOOL called_from_parent);
	BOOL handleUnicodeCharHere(llwchar uni_char);

	void draw();

	void moveCursor(U32 pos, BOOL second_nibble);

	void insert(U32 pos, std::vector<U8> new_data, BOOL undoable);
	void overwrite(U32 first_pos, U32 last_pos, std::vector<U8> new_data, BOOL undoable);
	void del(U32 first_pos, U32 last_pos, BOOL undoable);

	virtual void	cut();
	virtual BOOL	canCut() const;

	virtual void	copy();
	virtual BOOL	canCopy() const;

	virtual void	paste();
	virtual BOOL	canPaste() const;
	
	virtual void	doDelete();
	virtual BOOL	canDoDelete() const;

	virtual void	selectAll();
	virtual BOOL	canSelectAll() const;

	virtual void	deselect();
	virtual BOOL	canDeselect() const;

	virtual void	undo();
	virtual BOOL	canUndo() const;

	virtual void	redo();
	virtual BOOL	canRedo() const;

private:
	std::vector<U8> mValue;
	U8 mColumns;

	U32 mCursorPos;
	BOOL mSecondNibble;
	BOOL mInData;
	BOOL mSelecting;
	BOOL mHasSelection;
	U32 mSelectionStart;
	U32 mSelectionEnd;

	LLFontGL* mGLFont;
	LLRect mTextRect;
	LLScrollbar* mScrollbar;
	LLViewBorder* mBorder;

	LLUndoBuffer* mUndoBuffer;

	void changedLength();
	void getPosAndContext(S32 x, S32 y, BOOL force_context, U32& pos, BOOL& in_data, BOOL& second_nibble);
};

class DOUndoHex : public LLUndoBuffer::LLUndoAction
{
protected:
	DOUndoHex() {  }
	DOHexEditor* mHexEditor;
	U32 mFirstPos;
	U32 mLastPos;
	std::vector<U8> mOldData;
	std::vector<U8> mNewData;
public:
	static LLUndoAction* create() { return new DOUndoHex(); }
	virtual void set(DOHexEditor* hex_editor,
					 void (*undo_action)(DOUndoHex*),
					 void (*redo_action)(DOUndoHex*),
					 U32 first_pos,
					 U32 last_pos,
					 std::vector<U8> old_data,
					 std::vector<U8> new_data);
	void (*mUndoAction)(DOUndoHex*);
	void (*mRedoAction)(DOUndoHex*);
	virtual void undo();
	virtual void redo();
	
	static void undoInsert(DOUndoHex* action);
	static void redoInsert(DOUndoHex* action);
	static void undoOverwrite(DOUndoHex* action);
	static void redoOverwrite(DOUndoHex* action);
	static void undoDel(DOUndoHex* action);
	static void redoDel(DOUndoHex* action);
};

class DOHexInsert : public DOUndoHex
{
	virtual void undo();
	virtual void redo();
};

class DOHexOverwrite : public DOUndoHex
{
	virtual void undo();
	virtual void redo();
};

class DOHexDel : public DOUndoHex
{
	virtual void undo();
	virtual void redo();
};

#endif
// </edit>
