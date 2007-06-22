// This code is part of Fl_Text_Display::resize from fltk 1.1.7, modified
// to not hide the horizontal scrollbar when word wrapping is enabled.

#define TOP_MARGIN 1
#define BOTTOM_MARGIN 1
#define LEFT_MARGIN 3
#define RIGHT_MARGIN 3

const int oldWidth = w();

mHScrollBar->clear_visible();
mVScrollBar->resize(mVScrollBar->x(), mVScrollBar->y(),
                    mVScrollBar->w(), mVScrollBar->h() + scrollbar_width());

text_area.h += scrollbar_width();
if (scrollbar_align() & FL_ALIGN_TOP)
        text_area.y -= scrollbar_width();
mHScrollBar->resize(text_area.x-LEFT_MARGIN, Y,
                    text_area.w+LEFT_MARGIN+RIGHT_MARGIN, scrollbar_width());


if (mContinuousWrap && !mWrapMargin && W!=oldWidth) {
        int oldFirstChar = mFirstChar;
        mNBufferLines = count_lines(0, buffer()->length(), true);
        mFirstChar = line_start(mFirstChar);
        mTopLineNum = count_lines(0, mFirstChar, true)+1;
        absolute_top_line_number(oldFirstChar);
}

int nvlines = (text_area.h + mMaxsize - 1) / mMaxsize;
if (nvlines < 1) nvlines = 1;
if (mNVisibleLines != nvlines) {
        mNVisibleLines = nvlines;
        if (mLineStarts) delete[] mLineStarts;
        mLineStarts = new int [mNVisibleLines];
}

calc_line_starts(0, mNVisibleLines);
calc_last_char();

// user request to change viewport
if (mTopLineNumHint != mTopLineNum || mHorizOffsetHint != mHorizOffset)
        scroll_(mTopLineNumHint, mHorizOffsetHint);

// everything will fit in the viewport
if (mNBufferLines < mNVisibleLines || mBuffer == NULL || mBuffer->length() == 0)
        scroll_(1, mHorizOffset);
/* if empty lines become visible, there may be an opportunity to
   display more text by scrolling down */
else while (mLineStarts[mNVisibleLines-2] == -1)
             scroll_(mTopLineNum-1, mHorizOffset);

// in case horizontal offset is now greater than longest line
int maxhoffset = max(0, longest_vline()-text_area.w);
if (mHorizOffset > maxhoffset)
        scroll_(mTopLineNumHint, maxhoffset);

mTopLineNumHint = mTopLineNum;
mHorizOffsetHint = mHorizOffset;
display_insert_position_hint = 0;

if (mContinuousWrap)
        redraw();

update_v_scrollbar();
update_h_scrollbar();
