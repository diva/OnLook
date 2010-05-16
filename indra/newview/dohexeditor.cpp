/**
 * @file dohexeditor.cpp
 * @brief DOHexEditor Widget
 * @author Day Oh
 * 
 * $LicenseInfo:firstyear=2009&license=WTFPLV2$
 *  
 */

// <edit>
#include "llviewerprecompiledheaders.h"

#include "linden_common.h"
#include "dohexeditor.h"
#include "llfocusmgr.h"
#include "llkeyboard.h"
#include "llclipboard.h"
#include "llwindow.h" // setCursor
#include "lllocalinventory.h"

static LLRegisterWidget<DOHexEditor> r("hex_editor");

DOHexEditor::DOHexEditor(const std::string& name, const LLRect& rect)
:	LLUICtrl(name, rect, TRUE, NULL, NULL),
	LLEditMenuHandler(),
	mColumns(16),
	mCursorPos(0),
	mSecondNibble(FALSE),
	mSelecting(FALSE),
	mHasSelection(FALSE),
	mInData(FALSE),
	mSelectionStart(0),
	mSelectionEnd(0)
{
	mGLFont = LLFontGL::getFontMonospace();

	mTextRect.setOriginAndSize( 
		5, // border + padding
		1, // border
		getRect().getWidth() - SCROLLBAR_SIZE - 6,
		getRect().getHeight() - 5);

	S32 line_height = llround( mGLFont->getLineHeight() );
	S32 page_size = mTextRect.getHeight() / line_height;

	// Init the scrollbar
	LLRect scroll_rect;
	scroll_rect.setOriginAndSize( 
		getRect().getWidth() - SCROLLBAR_SIZE,
		1,
		SCROLLBAR_SIZE,
		getRect().getHeight() - 1);
	S32 lines_in_doc = getLineCount();
	mScrollbar = new LLScrollbar( std::string("Scrollbar"), scroll_rect,
		LLScrollbar::VERTICAL,
		lines_in_doc,						
		0,						
		page_size,
		NULL, this );
	mScrollbar->setFollowsRight();
	mScrollbar->setFollowsTop();
	mScrollbar->setFollowsBottom();
	mScrollbar->setEnabled( TRUE );
	mScrollbar->setVisible( TRUE );
	mScrollbar->setOnScrollEndCallback(NULL, NULL);
	addChild(mScrollbar);

	mBorder = new LLViewBorder( std::string("text ed border"),
							LLRect(0, getRect().getHeight(), getRect().getWidth(), 0),
							LLViewBorder::BEVEL_IN,
							LLViewBorder::STYLE_LINE,
							1 ); // border width
	addChild( mBorder );

	changedLength();

	mUndoBuffer = new LLUndoBuffer(DOUndoHex::create, 128);
}

DOHexEditor::~DOHexEditor()
{
	gFocusMgr.releaseFocusIfNeeded(this);
	if(gEditMenuHandler == this)
	{
		gEditMenuHandler = NULL;
	}
}

LLView* DOHexEditor::fromXML(LLXMLNodePtr node, LLView *parent, class LLUICtrlFactory *factory)
{
	std::string name("text_editor");
	node->getAttributeString("name", name);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	DOHexEditor* editor = new DOHexEditor(name, rect);

	editor->initFromXML(node, parent);

	return editor;
}

void DOHexEditor::setValue(const LLSD& value)
{
	mValue = value.asBinary();
	changedLength();
}

LLSD DOHexEditor::getValue() const
{
	return LLSD(mValue);
}

void DOHexEditor::setColumns(U8 columns)
{
	//mColumns = columns;
	mColumns = llclamp<U8>(llfloor(columns), 8, 64); //clamp this ffs
	changedLength();
}

U32 DOHexEditor::getLineCount()
{
	U32 lines = mValue.size();
	lines /= mColumns;
	lines++; // incomplete or extra line at bottom
	return lines;
}

void DOHexEditor::getPosAndContext(S32 x, S32 y, BOOL force_context, U32& pos, BOOL& in_data, BOOL& second_nibble)
{
	pos = 0;

	F32 line_height = mGLFont->getLineHeight();
	F32 char_width = mGLFont->getWidthF32(".");
	F32 data_column_width = char_width * 3; // " 00";
	F32 text_x = mTextRect.mLeft;
	F32 text_x_data = text_x + (char_width * 10.1f); // "00000000  ", dunno why it's a fraction off
	F32 text_x_ascii = text_x_data + (data_column_width * mColumns) + (char_width * 2);
	F32 text_y = (F32)(mTextRect.mTop - line_height);
	U32 first_line = mScrollbar->getDocPos();
	//U32 last_line = first_line + mScrollbar->getPageSize(); // don't -2 from scrollbar sizes
	S32 first_line_y = text_y - line_height;

	S32 ly = -(y - first_line_y); // negative vector from first line to y
	ly -= 5; // slight skew
	S32 line = ly / line_height;
	if(line < 0) line = 0;
	line += first_line;

	if(!force_context)
	{
		in_data = (x < (text_x_ascii - char_width)); // char width for less annoying
	}
	S32 lx = x;
	S32 offset;
	if(in_data)
	{
		lx -= char_width; // subtracting char width because idk
		lx -= text_x_data;
		offset = lx / data_column_width;

		// Now, which character
		S32 rem = S32( (F32)lx - (data_column_width * offset) - (char_width * 0.25) );
		if(rem > 0)
		{
			if(rem > char_width)
			{
				offset++; // next byte
				second_nibble = FALSE;
			}
			else second_nibble = TRUE;
		}
		else second_nibble = FALSE;
	}
	else
	{
		second_nibble = FALSE;
		lx += char_width; // adding char width because idk
		lx -= text_x_ascii;
		offset = lx / char_width;
	}
	if(offset < 0) offset = 0;
	if(offset >= mColumns)//offset = mColumns - 1;
	{
		offset = 0;
		line++;
		second_nibble = FALSE;
	}

	pos = (line * mColumns) + offset;
	if(pos < 0) pos = 0;
	if(pos > mValue.size()) pos = mValue.size();
	if(pos == mValue.size())
	{
		second_nibble = FALSE;
	}
}

void DOHexEditor::changedLength()
{
	S32 line_height = llround( mGLFont->getLineHeight() );
	S32 page_size = mTextRect.getHeight() / line_height;
	page_size -= 2; // don't count the spacer and header

	mScrollbar->setDocSize(getLineCount());
	mScrollbar->setPageSize(page_size);

	moveCursor(mCursorPos, mSecondNibble); // cursor was off end after undo of paste
}

void DOHexEditor::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLView::reshape( width, height, called_from_parent );
	mTextRect.setOriginAndSize( 
		5, // border + padding
		1, // border
		getRect().getWidth() - SCROLLBAR_SIZE - 6,
		getRect().getHeight() - 5);
	LLRect scrollrect;
	scrollrect.setOriginAndSize( 
		getRect().getWidth() - SCROLLBAR_SIZE,
		1,
		SCROLLBAR_SIZE,
		getRect().getHeight() - 1);
	mScrollbar->setRect(scrollrect);
	mBorder->setRect(LLRect(0, getRect().getHeight(), getRect().getWidth(), 0));
	changedLength();
}

void DOHexEditor::setFocus(BOOL b)
{
	if(b)
	{
		LLEditMenuHandler::gEditMenuHandler = this;
	}
	else
	{
		mSelecting = FALSE;
		gFocusMgr.releaseFocusIfNeeded(this);
		if(gEditMenuHandler == this)
		{
			gEditMenuHandler = NULL;
		}
	}
	LLUICtrl::setFocus(b);
}

F32 DOHexEditor::getSuggestedWidth(U8 cols)
{
	cols = cols>1?cols:mColumns;
	F32 char_width = mGLFont->getWidthF32(".");
	F32 data_column_width = char_width * 3; // " 00";
	F32 text_x = mTextRect.mLeft;
	F32 text_x_data = text_x + (char_width * 10.1f); // "00000000  ", dunno why it's a fraction off
	F32 text_x_ascii = text_x_data + (data_column_width * cols) + (char_width * 2);
	F32 suggested_width = text_x_ascii + (char_width * cols);
	suggested_width += mScrollbar->getRect().getWidth();
	suggested_width += 10.0f;
	return suggested_width;
}

U32 DOHexEditor::getProperSelectionStart()
{
	return (mSelectionStart < mSelectionEnd) ? mSelectionStart : mSelectionEnd;
}

U32 DOHexEditor::getProperSelectionEnd()
{
	return (mSelectionStart < mSelectionEnd) ? mSelectionEnd : mSelectionStart;
}

BOOL DOHexEditor::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	return mScrollbar->handleScrollWheel( 0, 0, clicks );
}

BOOL DOHexEditor::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;
	handled = LLView::childrenHandleMouseDown(x, y, mask) != NULL;
	if(!handled)
	{
		setFocus(TRUE);
		gFocusMgr.setMouseCapture(this);
		handled = TRUE;
		if(!mSelecting)
		{
			if(mask & MASK_SHIFT)
			{
				// extend a selection
				getPosAndContext(x, y, FALSE, mCursorPos, mInData, mSecondNibble);
				mSelectionEnd = mCursorPos;
				mHasSelection = (mSelectionStart != mSelectionEnd);
				mSelecting = TRUE;
			}
			else
			{
				// start selecting
				getPosAndContext(x, y, FALSE, mCursorPos, mInData, mSecondNibble);
				mSelectionStart = mCursorPos;
				mSelectionEnd = mCursorPos;
				mHasSelection = FALSE;
				mSelecting = TRUE;
			}
		}
	}
	return handled;
}

BOOL DOHexEditor::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;
	if(!hasMouseCapture())
	{
		handled = childrenHandleHover(x, y, mask) != NULL;
	}
	if(!handled && mSelecting && hasMouseCapture())
	{
		// continuation of selecting
		getPosAndContext(x, y, TRUE, mCursorPos, mInData, mSecondNibble);
		mSelectionEnd = mCursorPos;
		mHasSelection = (mSelectionStart != mSelectionEnd);
		handled = TRUE;
	}
	return handled;
}

BOOL DOHexEditor::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;
	handled = LLView::childrenHandleMouseUp(x, y, mask) != NULL;
	if(!handled && mSelecting && hasMouseCapture())
	{
		gFocusMgr.setMouseCapture(NULL);
		mSelecting = FALSE;
	}
	return handled;
}

BOOL DOHexEditor::handleKeyHere(KEY key, MASK mask)
{
	return FALSE;
}

BOOL DOHexEditor::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	BOOL handled = FALSE;

	BOOL moved_cursor = FALSE;
	U32 old_cursor = mCursorPos;
	U32 cursor_line = mCursorPos / mColumns;
	U32 doc_first_line = 0;
	U32 doc_last_line = mValue.size() / mColumns;
	//U32 first_line = mScrollbar->getDocPos();
	//U32 last_line = first_line + mScrollbar->getPageSize(); // don't -2 from scrollbar sizes
	U32 beginning_of_line = mCursorPos - (mCursorPos % mColumns);
	U32 end_of_line = beginning_of_line + mColumns - 1;

	handled = TRUE;
	switch( key )
	{

	// Movement keys

	case KEY_UP:
		if(cursor_line > doc_first_line)
		{
			moveCursor(mCursorPos - mColumns, mSecondNibble);
			moved_cursor = TRUE;
		}
		break;
	case KEY_DOWN:
		if(cursor_line < doc_last_line)
		{
			moveCursor(mCursorPos + mColumns, mSecondNibble);
			moved_cursor = TRUE;
		}
		break;
	case KEY_LEFT:
		if(mCursorPos)
		{
			if(!mSecondNibble) moveCursor(mCursorPos - 1, FALSE);
			else moveCursor(mCursorPos, FALSE);
			moved_cursor = TRUE;
		}
		break;
	case KEY_RIGHT:
		moveCursor(mCursorPos + 1, FALSE);
		moved_cursor = TRUE;
		break;
	case KEY_PAGE_UP:
		mScrollbar->pageUp(1);
		break;
	case KEY_PAGE_DOWN:
		mScrollbar->pageDown(1);
		break;
	case KEY_HOME:
		if(mask & MASK_CONTROL)
			moveCursor(0, FALSE);
		else
			moveCursor(beginning_of_line, FALSE);
		moved_cursor = TRUE;
		break;
	case KEY_END:
		if(mask & MASK_CONTROL)
			moveCursor(mValue.size(), FALSE);
		else
			moveCursor(end_of_line, FALSE);
		moved_cursor = TRUE;
		break;

	// Special

	case KEY_INSERT:
		gKeyboard->toggleInsertMode();
		break;

	case KEY_ESCAPE:
		gFocusMgr.releaseFocusIfNeeded(this);
		break;

	// Editing
	case KEY_BACKSPACE:
		if(mHasSelection)
		{
			U32 start = getProperSelectionStart();
			U32 end = getProperSelectionEnd();
			del(start, end - 1, TRUE);
			moveCursor(start, FALSE);
		}
		else if(mCursorPos && (!mSecondNibble))
		{
			del(mCursorPos - 1, mCursorPos - 1, TRUE);
			moveCursor(mCursorPos - 1, FALSE);
		}
		break;
	
	case KEY_DELETE:
		if(mHasSelection)
		{
			U32 start = getProperSelectionStart();
			U32 end = getProperSelectionEnd();
			del(start, end - 1, TRUE);
			moveCursor(start, FALSE);
		}
		else if((mCursorPos != mValue.size()) && (!mSecondNibble))
		{
			del(mCursorPos, mCursorPos, TRUE);
		}
		break;

	default:
		handled = FALSE;
		break;
	}

	if(moved_cursor)
	{
		// Selecting and deselecting
		if(mask & MASK_SHIFT)
		{
			if(!mHasSelection) mSelectionStart = old_cursor;
			mSelectionEnd = mCursorPos;
		}
		else
		{
			mSelectionStart = mCursorPos;
			mSelectionEnd = mCursorPos;
		}
		mHasSelection = mSelectionStart != mSelectionEnd;
	}
	
	return handled;
}

BOOL DOHexEditor::handleUnicodeChar(llwchar uni_char, BOOL called_from_parent)
{
	U8 c = uni_char & 0xff;
	if(mInData)
	{
		if(c > 0x39)
		{
			if(c > 0x46) c -= 0x20;
			if(c >= 0x41 && c <= 0x46) c = (c & 0x0f) + 0x09;
			else return TRUE;
		}
		else if(c < 0x30) return TRUE;
		else c &= 0x0f;
	}

	if(uni_char < 0x20) return FALSE;

	if( (LL_KIM_INSERT == gKeyboard->getInsertMode() && (!mHasSelection))
		|| (!mHasSelection && (mCursorPos == mValue.size())) )// last byte? always insert
	{
		// Todo: this should overwrite if there's a selection
		if(!mInData)
		{
			std::vector<U8> new_data;
			new_data.push_back(c);
			insert(mCursorPos, new_data, TRUE);
			moveCursor(mCursorPos + 1, FALSE);
		}
		else if(!mSecondNibble)
		{
			c <<= 4;
			std::vector<U8> new_data;
			new_data.push_back(c);
			insert(mCursorPos, new_data, TRUE);
			moveCursor(mCursorPos, TRUE);
		}
		else
		{
			c |= (mValue[mCursorPos] & 0xF0);
			std::vector<U8> new_data;
			new_data.push_back(c);
			overwrite(mCursorPos, mCursorPos, new_data, TRUE);
			moveCursor(mCursorPos + 1, FALSE);
		}
	}
	else // overwrite mode
	{
		if(mHasSelection)
		{
			if(mInData) c <<= 4;
			std::vector<U8> new_data;
			new_data.push_back(c);
			U8 start = getProperSelectionStart();
			overwrite(start, getProperSelectionEnd() - 1, new_data, TRUE);
			if(mInData) moveCursor(start, TRUE); // we only entered a nibble
			else moveCursor(start + 1, FALSE); // we only entered a byte
		}
		else if(!mInData)
		{
			std::vector<U8> new_data;
			new_data.push_back(c);
			overwrite(mCursorPos, mCursorPos, new_data, TRUE);
			moveCursor(mCursorPos + 1, FALSE);
		}
		else if(!mSecondNibble)
		{
			c <<= 4;
			c |= (mValue[mCursorPos] & 0x0F);
			std::vector<U8> new_data;
			new_data.push_back(c);
			overwrite(mCursorPos, mCursorPos, new_data, TRUE);
			moveCursor(mCursorPos, TRUE);
		}
		else
		{
			c |= (mValue[mCursorPos] & 0xF0);
			std::vector<U8> new_data;
			new_data.push_back(c);
			overwrite(mCursorPos, mCursorPos, new_data, TRUE);
			moveCursor(mCursorPos + 1, FALSE);
		}
	}

	return TRUE;
}

BOOL DOHexEditor::handleUnicodeCharHere(llwchar uni_char)
{
	return FALSE;
}

void DOHexEditor::draw()
{
	S32 left = 0;
	S32 top = getRect().getHeight();
	S32 right = getRect().getWidth();
	S32 bottom = 0;

	BOOL has_focus = gFocusMgr.getKeyboardFocus() == this;

	F32 line_height = mGLFont->getLineHeight();
	F32 char_width = mGLFont->getWidthF32(".");
	F32 data_column_width = char_width * 3; // " 00";
	F32 text_x = mTextRect.mLeft;
	F32 text_x_data = text_x + (char_width * 10.1f); // "00000000  ", dunno why it's a fraction off
#ifdef COLUMN_SPAN
	mColumns = (right - char_width * 2 - text_x_data - mScrollbar->getRect().getWidth()) / (char_width * 4); // touch this if you dare...
#endif
	F32 text_x_ascii = text_x_data + (data_column_width * mColumns) + (char_width * 2);
	F32 text_y = (F32)(mTextRect.mTop - line_height);

	U32 data_length = mValue.size();
	U32 line_count = getLineCount();
	U32 first_line = mScrollbar->getDocPos();
	U32 last_line = first_line + mScrollbar->getPageSize(); // don't -2 from scrollbar sizes

	LLRect clip(getRect());
	clip.mRight = mScrollbar->getRect().mRight;
	clip.mLeft -= 10;
	clip.mBottom -= 10;
	LLLocalClipRect bgclip(clip);

	// Background
	gl_rect_2d(left, top, right, bottom, LLColor4::white);

	// Let's try drawing some helpful guides
	LLColor4 guide_color_light = LLColor4(1.0f, 1.0f, 0.9f);
	LLColor4 guide_color_dark = LLColor4(1.0f, 1.0f, 0.8f);
	for(U32 col = 0; col < mColumns; col += 2)
	{
		// Behind hex
		F32 box_left = text_x_data + (col * data_column_width) + 2; // skew 2
		F32 box_right = box_left + data_column_width;
		gl_rect_2d(box_left, top, box_right, bottom, (col & 3) ? guide_color_light : guide_color_dark);
		// Behind ASCII
		//box_left = text_x_ascii + (col * char_width) - 1; // skew 1
		//box_right = box_left + char_width;
		//gl_rect_2d(box_left, top, box_right, bottom, guide_color);
	}

	
	// Scrollbar & border (drawn twice?)
	mBorder->setKeyboardFocusHighlight(has_focus);
	LLView::draw();


	LLLocalClipRect textrect_clip(mTextRect);


	// Selection stuff is reused
	U32 selection_start = getProperSelectionStart();
	U32 selection_end = getProperSelectionEnd();
	U32 selection_first_line = selection_start / mColumns;
	U32 selection_last_line = selection_end / mColumns;
	U32 selection_start_column = selection_start % mColumns;
	U32 selection_end_column = selection_end % mColumns;

	// Don't pretend a selection there is visible
	if(!selection_end_column)
	{
		selection_last_line--;
		selection_end_column = mColumns;
	}

	if(mHasSelection)
	{
		LLColor4 selection_color_context(LLColor4::black);
		LLColor4 selection_color_not_context(LLColor4::grey3);
		LLColor4 selection_color_data(selection_color_not_context);
		LLColor4 selection_color_ascii(selection_color_not_context);
		if(mInData) selection_color_data = selection_color_context;
		else selection_color_ascii = selection_color_context;


		// Setup for selection in data
		F32 selection_pixel_x_base = text_x_data + char_width - 3; // skew 3
		F32 selection_pixel_x_right_base = selection_pixel_x_base + (data_column_width * mColumns) - char_width + 4;
		F32 selection_pixel_x;
		F32 selection_pixel_x_right;
		F32 selection_pixel_y = (F32)(mTextRect.mTop - line_height) - 3; // skew 3;
		selection_pixel_y -= line_height * 2;
		selection_pixel_y -= line_height * (S32(selection_first_line) - S32(first_line));

		// Selection in data, First line
		if(selection_first_line >= first_line && selection_first_line <= last_line)
		{
			selection_pixel_x = selection_pixel_x_base;
			selection_pixel_x += (data_column_width * selection_start_column);
			if(selection_first_line == selection_last_line)
			{
				// Select to last character
				selection_pixel_x_right = selection_pixel_x_base + (data_column_width * selection_end_column);
				selection_pixel_x_right -= (char_width - 4);
			}
			else
			{
				// Select to end of line
				selection_pixel_x_right = selection_pixel_x_right_base;
			}
			gl_rect_2d(selection_pixel_x, selection_pixel_y + line_height, selection_pixel_x_right, selection_pixel_y, selection_color_data);
		}

		// Selection in data, Middle lines
		for(U32 line = selection_first_line + 1; line < selection_last_line; line++)
		{
			selection_pixel_y -= line_height;
			if(line >= first_line && line <= last_line)
			{
				gl_rect_2d(selection_pixel_x_base, selection_pixel_y + line_height, selection_pixel_x_right_base, selection_pixel_y, selection_color_data);
			}
		}
		
		// Selection in data, Last line
		if(selection_first_line != selection_last_line
			&& selection_last_line >= first_line && selection_last_line <= last_line)
		{
			selection_pixel_x_right = selection_pixel_x_base + (data_column_width * selection_end_column);
			selection_pixel_x_right -= (char_width - 4);
			selection_pixel_y -= line_height;
			gl_rect_2d(selection_pixel_x_base, selection_pixel_y + line_height, selection_pixel_x_right, selection_pixel_y, selection_color_data);
		}

		selection_pixel_y = (F32)(mTextRect.mTop - line_height) - 3; // skew 3;
		selection_pixel_y -= line_height * 2;
		selection_pixel_y -= line_height * (S32(selection_first_line) - S32(first_line));

		// Setup for selection in ASCII
		selection_pixel_x_base = text_x_ascii - 1;
		selection_pixel_x_right_base = selection_pixel_x_base + (char_width * mColumns);

		// Selection in ASCII, First line
		if(selection_first_line >= first_line && selection_first_line <= last_line)
		{
			selection_pixel_x = selection_pixel_x_base;
			selection_pixel_x += (char_width * selection_start_column);
			if(selection_first_line == selection_last_line)
			{
				// Select to last character
				selection_pixel_x_right = selection_pixel_x_base + (char_width * selection_end_column);
			}
			else
			{
				// Select to end of line
				selection_pixel_x_right = selection_pixel_x_right_base;
			}
			gl_rect_2d(selection_pixel_x, selection_pixel_y + line_height, selection_pixel_x_right, selection_pixel_y, selection_color_ascii);
		}

		// Selection in ASCII, Middle lines
		for(U32 line = selection_first_line + 1; line < selection_last_line; line++)
		{
			selection_pixel_y -= line_height;
			if(line >= first_line && line <= last_line)
			{
				gl_rect_2d(selection_pixel_x_base, selection_pixel_y + line_height, selection_pixel_x_right_base, selection_pixel_y, selection_color_ascii);
			}
		}
		
		// Selection in ASCII, Last line
		if(selection_first_line != selection_last_line
			&& selection_last_line >= first_line && selection_last_line <= last_line)
		{
			selection_pixel_x_right = selection_pixel_x_base + (char_width * selection_end_column);
			selection_pixel_y -= line_height;
			gl_rect_2d(selection_pixel_x_base, selection_pixel_y + line_height, selection_pixel_x_right, selection_pixel_y, selection_color_ascii);
		}
	}


	// Insert/Overwrite
	std::string text = (LL_KIM_OVERWRITE == gKeyboard->getInsertMode()) ? "OVERWRITE" : "INSERT";
	mGLFont->renderUTF8(text, 0, text_x, text_y, LLColor4::purple);
	// Offset on top
	text = "";
	for(U32 i = 0; i < mColumns; i++)
	{
		text.append(llformat(" %02X", i));
	}
	mGLFont->renderUTF8(text, 0, text_x_data, text_y, LLColor4::blue);
	// Size
	{
		S32 size = mValue.size();
		std::string size_desc;
		if(size < 1000) size_desc = llformat("%d bytes", size);
		else
		{
			if(size < 1000000)
			{
				size_desc = llformat("%f", F32(size) / 1000.0f);
				int i = size_desc.length() - 1;
				for(; i && size_desc.substr(i, 1) == "0"; i--);
				if(size_desc.substr(i, 1) == ".") i--;
				size_desc = size_desc.substr(0, i + 1);
				size_desc.append(" KB");
			}
			else
			{
				size_desc = llformat("%f", F32(size) / 1000000.0f);
				int i = size_desc.length() - 1;
				for(; i && size_desc.substr(i, 1) == "0"; i--);
				if(size_desc.substr(i, 1) == ".") i--;
				size_desc = size_desc.substr(0, i + 1);
				size_desc.append(" MB");
			}
		}
		F32 x = text_x_ascii;
		x += (char_width * (mColumns - size_desc.length()));
		mGLFont->renderUTF8(size_desc, 0, x, text_y, LLColor4::purple);
	}
	// Leave a blank line
	text_y -= (line_height * 2);

	// Everything below "header"
	for(U32 line = first_line; line <= last_line; line++)
	{
		if(line >= line_count) break;
		
		// Offset on left
		text = llformat("%08X", line * mColumns); // offset on left
		mGLFont->renderUTF8(text, 0, text_x, text_y, LLColor4::blue);

		// Setup for rendering hex and ascii
		U32 line_char_offset = mColumns * line;
		U32 colstart0 = 0;
		U32 colend0 = mColumns;
		U32 colstart1 = mColumns;
		U32 colend1 = mColumns;
		U32 colstart2 = mColumns;
		U32 colend2 = mColumns;
		if(mHasSelection)
		{
			if(line == selection_first_line)
			{
				colend0 = selection_start_column;
				colstart1 = selection_start_column;
				if(selection_first_line == selection_last_line)
				{
					colend1 = selection_end_column;
					colstart2 = selection_end_column;
					colend2 = mColumns;
				}
			}
			else if(line > selection_first_line && line < selection_last_line)
			{
				colend0 = 0;
				colstart1 = 0;
				colend1 = mColumns;
			}
			else if(line == selection_last_line)
			{
				colend0 = 0;
				colstart1 = 0;
				colend1 = selection_end_column;
				colstart2 = selection_end_column;
				colend2 = mColumns;
			}
		}

		// Data in hex
		text = "";
		for(U32 c = colstart0; c < colend0; c++)
		{
			U32 o = line_char_offset + c;
			if(o >= data_length) text.append("   ");
			else text.append(llformat(" %02X", mValue[o]));
		}
		mGLFont->renderUTF8(text, 0, text_x_data + (colstart0 * data_column_width), text_y, LLColor4::black);
		text = "";
		for(U32 c = colstart1; c < colend1; c++)
		{
			U32 o = line_char_offset + c;
			if(o >= data_length) text.append("   ");
			else text.append(llformat(" %02X", mValue[o]));
		}
		mGLFont->renderUTF8(text, 0, text_x_data + (colstart1 * data_column_width), text_y, LLColor4::white);
		text = "";
		for(U32 c = colstart2; c < colend2; c++)
		{
			U32 o = line_char_offset + c;
			if(o >= data_length) text.append("   ");
			else text.append(llformat(" %02X", mValue[o]));
		}
		mGLFont->renderUTF8(text, 0, text_x_data + (colstart2 * data_column_width), text_y, LLColor4::black);

		// ASCII
		text = "";
		for(U32 c = colstart0; c < colend0; c++)
		{
			U32 o = line_char_offset + c;
			if(o >= data_length) break;
			if((mValue[o] < 0x20) || (mValue[o] >= 0x7F)) text.append(".");
			else text.append(llformat("%c", mValue[o]));
		}
		mGLFont->renderUTF8(text, 0, text_x_ascii + (colstart0 * char_width), text_y, LLColor4::black);
		text = "";
		for(U32 c = colstart1; c < colend1; c++)
		{
			U32 o = line_char_offset + c;
			if(o >= data_length) break;
			if((mValue[o] < 0x20) || (mValue[o] >= 0x7F)) text.append(".");
			else text.append(llformat("%c", mValue[o]));
		}
		mGLFont->renderUTF8(text, 0, text_x_ascii + (colstart1 * char_width), text_y, LLColor4::white);
		text = "";
		for(U32 c = colstart2; c < colend2; c++)
		{
			U32 o = line_char_offset + c;
			if(o >= data_length) break;
			if((mValue[o] < 0x20) || (mValue[o] >= 0x7F)) text.append(".");
			else text.append(llformat("%c", mValue[o]));
		}
		mGLFont->renderUTF8(text, 0, text_x_ascii + (colstart2 * char_width), text_y, LLColor4::black);

		text_y -= line_height;
	}



	// Cursor
	if(has_focus && !mHasSelection && (U32(LLTimer::getElapsedSeconds() * 2.0f) & 0x1))
	{
		U32 cursor_line = mCursorPos / mColumns;
		if((cursor_line >= first_line) && (cursor_line <= last_line))
		{
			F32 pixel_y = (F32)(mTextRect.mTop - line_height);
			pixel_y -= line_height * (2 + (cursor_line - first_line));

			U32 cursor_offset = mCursorPos % mColumns; // bytes
			F32 pixel_x = mInData ? text_x_data : text_x_ascii;
			if(mInData)
			{
				pixel_x += data_column_width * cursor_offset;
				pixel_x += char_width;
				if(mSecondNibble) pixel_x += char_width;
			}
			else
			{
				pixel_x += char_width * cursor_offset;
			}
			pixel_x -= 2.0f;
			pixel_y -= 2.0f;
			gl_rect_2d(pixel_x, pixel_y + line_height, pixel_x + 2, pixel_y, LLColor4::black);
		}
	}
}

void DOHexEditor::deselect()
{
	mSelectionStart = mCursorPos;
	mSelectionEnd = mCursorPos;
	mHasSelection = FALSE;
	mSelecting = FALSE;
}

BOOL DOHexEditor::canUndo() const
{
	return mUndoBuffer->canUndo();
}

void DOHexEditor::undo()
{
	mUndoBuffer->undoAction();
}

BOOL DOHexEditor::canRedo() const
{
	return mUndoBuffer->canRedo();
}

void DOHexEditor::redo()
{
	mUndoBuffer->redoAction();
}




void DOHexEditor::moveCursor(U32 pos, BOOL second_nibble)
{
	mCursorPos = pos;

	// Clamp and handle second nibble
	if(mCursorPos >= mValue.size())
	{
		mCursorPos = mValue.size();
		mSecondNibble = FALSE;
	}
	else
	{
		mSecondNibble = mInData ? second_nibble : FALSE;
	}

	// Change selection
	mSelectionEnd = mCursorPos;
	if(!mHasSelection) mSelectionStart = mCursorPos;

	// Scroll
	U32 line = mCursorPos / mColumns;
	U32 first_line = mScrollbar->getDocPos();
	U32 last_line = first_line + mScrollbar->getPageSize(); // don't -2 from scrollbar sizes
	if(line < first_line) mScrollbar->setDocPos(line);
	if(line > (last_line - 2)) mScrollbar->setDocPos(line - mScrollbar->getPageSize() + 1);
}

BOOL DOHexEditor::canCut() const
{
	return mHasSelection;
}

void DOHexEditor::cut()
{
	if(!canCut()) return;

	copy();

	U32 start = getProperSelectionStart();
	del(start, getProperSelectionEnd() - 1, TRUE);
    
	moveCursor(start, FALSE);
}

BOOL DOHexEditor::canCopy() const
{
	return mHasSelection;
}

void DOHexEditor::copy()
{
	if(!canCopy()) return;

	std::string text;
	if(mInData)
	{
		U32 start = getProperSelectionStart();
		U32 end = getProperSelectionEnd();
		for(U32 i = start; i < end; i++)
			text.append(llformat("%02X", mValue[i]));
	}
	else
	{
		U32 start = getProperSelectionStart();
		U32 end = getProperSelectionEnd();
		for(U32 i = start; i < end; i++)
			text.append(llformat("%c", mValue[i]));
	}
	LLWString wtext = utf8str_to_wstring(text);
	gClipboard.copyFromSubstring(wtext, 0, wtext.length());
}

BOOL DOHexEditor::canPaste() const
{
	return TRUE;
}

void DOHexEditor::paste()
{
	if(!canPaste()) return;

	std::string clipstr = wstring_to_utf8str(gClipboard.getPasteWString());
	const char* clip = clipstr.c_str();

	std::vector<U8> new_data;
	if(mInData)
	{
		int len = strlen(clip);
		for(int i = 0; (i + 1) < len; i += 2)
		{
			int c = 0;
			if(sscanf(&(clip[i]), "%02X", &c) != 1) break;
			new_data.push_back(U8(c));
		}
	}
	else
	{
		int len = strlen(clip);
		for(int i = 0; i < len; i++)
		{
			U8 c = 0;
			if(sscanf(&(clip[i]), "%c", &c) != 1) break;
			new_data.push_back(c);
		}
	}

	U32 start = mCursorPos;
	if(!mHasSelection)
		insert(start, new_data, TRUE);
	else
	{
		start = getProperSelectionStart();
		overwrite(start, getProperSelectionEnd() - 1, new_data, TRUE);
	}

	moveCursor(start + new_data.size(), FALSE);
}

BOOL DOHexEditor::canDoDelete() const
{
	return mValue.size() > 0;
}

void DOHexEditor::doDelete()
{
	if(!canDoDelete()) return;

	U32 start = getProperSelectionStart();
	del(start, getProperSelectionEnd(), TRUE);

	moveCursor(start, FALSE);
}

BOOL DOHexEditor::canSelectAll() const
{
	return mValue.size() > 0;
}

void DOHexEditor::selectAll()
{
	if(!canSelectAll()) return;

	mSelectionStart = 0;
	mSelectionEnd = mValue.size();
	mHasSelection = mSelectionStart != mSelectionEnd;
}

BOOL DOHexEditor::canDeselect() const
{
	return mHasSelection;
}

void DOHexEditor::insert(U32 pos, std::vector<U8> new_data, BOOL undoable)
{
	if(pos > mValue.size())
	{
		llwarns << "pos outside data!" << llendl;
		return;
	}

	deselect();

	if(undoable)
	{
		DOUndoHex* action = (DOUndoHex*)(mUndoBuffer->getNextAction());
		action->set(this, &(DOUndoHex::undoInsert), &(DOUndoHex::redoInsert), pos, pos, std::vector<U8>(), new_data);
	}

	std::vector<U8>::iterator wheres = mValue.begin() + pos;

	mValue.insert(wheres, new_data.begin(), new_data.end());

	changedLength();
}

void DOHexEditor::overwrite(U32 first_pos, U32 last_pos, std::vector<U8> new_data, BOOL undoable)
{
	if(first_pos > mValue.size() || last_pos > mValue.size())
	{
		llwarns << "pos outside data!" << llendl;
		return;
	}

	deselect();

	std::vector<U8>::iterator first = mValue.begin() + first_pos;
	std::vector<U8>::iterator last = mValue.begin() + last_pos;

	std::vector<U8> old_data;
	if(last_pos > 0) old_data = std::vector<U8>(first, last + 1);

	if(undoable)
	{
		DOUndoHex* action = (DOUndoHex*)(mUndoBuffer->getNextAction());
		action->set(this, &(DOUndoHex::undoOverwrite), &(DOUndoHex::redoOverwrite), first_pos, last_pos, old_data, new_data);
	}

	mValue.erase(first, last + 1);
	first = mValue.begin() + first_pos;
	mValue.insert(first, new_data.begin(), new_data.end());

	changedLength();
}

void DOHexEditor::del(U32 first_pos, U32 last_pos, BOOL undoable)
{
	if(first_pos > mValue.size() || last_pos > mValue.size())
	{
		llwarns << "pos outside data!" << llendl;
		return;
	}

	deselect();

	std::vector<U8>::iterator first = mValue.begin() + first_pos;
	std::vector<U8>::iterator last = mValue.begin() + last_pos;

	std::vector<U8> old_data;
	if(last_pos > 0) old_data = std::vector<U8>(first, last + 1);

	if(undoable)
	{
		DOUndoHex* action = (DOUndoHex*)(mUndoBuffer->getNextAction());
		action->set(this, &(DOUndoHex::undoDel), &(DOUndoHex::redoDel), first_pos, last_pos, old_data, std::vector<U8>());
	}
	
	mValue.erase(first, last + 1);

	changedLength();
}

void DOUndoHex::set(DOHexEditor* hex_editor,
					void (*undo_action)(DOUndoHex*),
					void (*redo_action)(DOUndoHex*),
					U32 first_pos,
					U32 last_pos,
					std::vector<U8> old_data,
					std::vector<U8> new_data)
{
	mHexEditor = hex_editor;
	mUndoAction = undo_action;
	mRedoAction = redo_action;
	mFirstPos = first_pos;
	mLastPos = last_pos;
	mOldData = old_data;
	mNewData = new_data;
}

void DOUndoHex::undo()
{
	mUndoAction(this);
}

void DOUndoHex::redo()
{
	mRedoAction(this);
}

void DOUndoHex::undoInsert(DOUndoHex* action)
{
	//action->mHexEditor->del(action->mFirstPos, action->mLastPos, FALSE);
	action->mHexEditor->del(action->mFirstPos, action->mFirstPos + action->mNewData.size() - 1, FALSE);
}

void DOUndoHex::redoInsert(DOUndoHex* action)
{
	action->mHexEditor->insert(action->mFirstPos, action->mNewData, FALSE);
}

void DOUndoHex::undoOverwrite(DOUndoHex* action)
{
	//action->mHexEditor->overwrite(action->mFirstPos, action->mLastPos, action->mOldData, FALSE);
	action->mHexEditor->overwrite(action->mFirstPos, action->mFirstPos + action->mNewData.size() - 1, action->mOldData, FALSE);
}

void DOUndoHex::redoOverwrite(DOUndoHex* action)
{
	action->mHexEditor->overwrite(action->mFirstPos, action->mLastPos, action->mNewData, FALSE);
}

void DOUndoHex::undoDel(DOUndoHex* action)
{
	action->mHexEditor->insert(action->mFirstPos, action->mOldData, FALSE);
}

void DOUndoHex::redoDel(DOUndoHex* action)
{
	action->mHexEditor->del(action->mFirstPos, action->mLastPos, FALSE);
}


// </edit>
