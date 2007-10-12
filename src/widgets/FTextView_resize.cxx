// This code is part of Fl_Text_Display::resize from fltk 1.1.7, modified
// to not hide the horizontal scrollbar when word wrapping is enabled.

#define TOP_MARGIN 1
#define BOTTOM_MARGIN 1
#define LEFT_MARGIN 3
#define RIGHT_MARGIN 3

#ifdef DEBUG
  printf("Fl_Text_Display::resize(X=%d, Y=%d, W=%d, H=%d)\n", X, Y, W, H);
#endif // DEBUG
  const int oldWidth = w();
#ifdef DEBUG
  printf("    oldWidth=%d, mContinuousWrap=%d, mWrapMargin=%d\n", oldWidth,
         mContinuousWrap, mWrapMargin);
#endif // DEBUG
  Fl_Widget::resize(X,Y,W,H);
  if (!buffer()) return;
  X += Fl::box_dx(box());
  Y += Fl::box_dy(box());
  W -= Fl::box_dw(box());
  H -= Fl::box_dh(box());

  text_area.x = X+LEFT_MARGIN;
  text_area.y = Y+BOTTOM_MARGIN;
  text_area.w = W-LEFT_MARGIN-RIGHT_MARGIN;
  text_area.h = H-TOP_MARGIN-BOTTOM_MARGIN;
  int i;

  /* Find the new maximum font height for this text display */
  for (i = 0, mMaxsize = fl_height(textfont(), textsize()); i < mNStyles; i++)
    mMaxsize = max(mMaxsize, fl_height(mStyleTable[i].font, mStyleTable[i].size));

  // did we have scrollbars initially?
  int hscrollbarvisible = mHScrollBar->visible();
  int vscrollbarvisible = mVScrollBar->visible();

  // try without scrollbars first
  mVScrollBar->clear_visible();
  mHScrollBar->clear_visible();

  int again_ = 1;
  for (int again = 1; again;) {
     again = 0;
    /* In continuous wrap mode, a change in width affects the total number of
       lines in the buffer, and can leave the top line number incorrect, and
       the top character no longer pointing at a valid line start */
    if (mContinuousWrap && !mWrapMargin && W!=oldWidth) {
      int oldFirstChar = mFirstChar;
      mNBufferLines = count_lines(0, buffer()->length(), true);
      mFirstChar = line_start(mFirstChar);
      mTopLineNum = count_lines(0, mFirstChar, true)+1;
      absolute_top_line_number(oldFirstChar);

#ifdef DEBUG
      printf("    mNBufferLines=%d\n", mNBufferLines);
#endif // DEBUG
    }
 
    /* reallocate and update the line starts array, which may have changed
       size and / or contents.  */
    int nvlines = (text_area.h + mMaxsize - 1) / mMaxsize;
    if (nvlines < 1) nvlines = 1;
    if (mNVisibleLines != nvlines) {
      mNVisibleLines = nvlines;
      if (mLineStarts) delete[] mLineStarts;
      mLineStarts = new int [mNVisibleLines];
    }

    calc_line_starts(0, mNVisibleLines);
    calc_last_char();

    // figure the scrollbars
    if (scrollbar_width()) {
      /* Decide if the vertical scroll bar needs to be visible */
      if (scrollbar_align() & (FL_ALIGN_LEFT|FL_ALIGN_RIGHT) &&
          mNBufferLines >= mNVisibleLines - 1)
      {
        mVScrollBar->set_visible();
        if (scrollbar_align() & FL_ALIGN_LEFT) {
          text_area.x = X+scrollbar_width()+LEFT_MARGIN;
          text_area.w = W-scrollbar_width()-LEFT_MARGIN-RIGHT_MARGIN;
          mVScrollBar->resize(X, text_area.y-TOP_MARGIN, scrollbar_width(),
                              text_area.h+TOP_MARGIN+BOTTOM_MARGIN);
        } else {
          text_area.x = X+LEFT_MARGIN;
          text_area.w = W-scrollbar_width()-LEFT_MARGIN-RIGHT_MARGIN;
          mVScrollBar->resize(X+W-scrollbar_width(), text_area.y-TOP_MARGIN,
                              scrollbar_width(), text_area.h+TOP_MARGIN+BOTTOM_MARGIN);
        }
      }

      /*
         Decide if the horizontal scroll bar needs to be visible.  If there
         is a vertical scrollbar, a horizontal is always created too.  This
         is because the alternatives are unatractive:
          * Dynamically creating a horizontal scrollbar based on the currently
            visible lines is what the original nedit does, but it always wastes
            space for the scrollbar even when it's not used.  Since the FLTK
            widget dynamically allocates the space for the scrollbar and
            rearranges the widget to make room for it, this would create a very
            visually displeasing "bounce" effect when the vertical scrollbar is
            dragged.  Trust me, I tried it and it looks really bad.
          * The other alternative would be to keep track of what the longest
            line in the entire buffer is and base the scrollbar on that.  I
            didn't do this because I didn't see any easy way to do that using
            the nedit code and this could involve a lengthy calculation for
            large buffers.  If an efficient and non-costly way of doing this
            can be found, this might be a way to go.
      */
      /* WAS: Suggestion: Try turning the horizontal scrollbar on when
	 you first see a line that is too wide in the window, but then
	 don't turn it off (ie mix both of your solutions). */
      if (!mContinuousWrap && scrollbar_align() & (FL_ALIGN_TOP|FL_ALIGN_BOTTOM) &&
          (mVScrollBar->visible() || longest_vline() > text_area.w))
      {
        if (!mHScrollBar->visible()) {
          mHScrollBar->set_visible();
          again = 1; // loop again to see if we now need vert. & recalc sizes
        }
        if (scrollbar_align() & FL_ALIGN_TOP) {
          text_area.y = Y + scrollbar_width()+TOP_MARGIN;
          text_area.h = H - scrollbar_width()-TOP_MARGIN-BOTTOM_MARGIN;
          mHScrollBar->resize(text_area.x-LEFT_MARGIN, Y,
                              text_area.w+LEFT_MARGIN+RIGHT_MARGIN, scrollbar_width());
        } else {
          text_area.y = Y+TOP_MARGIN;
          text_area.h = H - scrollbar_width()-TOP_MARGIN-BOTTOM_MARGIN;
          mHScrollBar->resize(text_area.x-LEFT_MARGIN, Y+H-scrollbar_width(),
                              text_area.w+LEFT_MARGIN+RIGHT_MARGIN, scrollbar_width());
        }
      } else if (again_ == 1) { // loop once more
        again_ = 0;
        again = 1;
      }
    }
  }

  // user request to change viewport
  if (mTopLineNumHint != mTopLineNum || mHorizOffsetHint != mHorizOffset)
    FTextBase::scroll_(mTopLineNumHint, mHorizOffsetHint);

  // // everything will fit in the viewport
  // if (mNBufferLines < mNVisibleLines || mBuffer == NULL || mBuffer->length() == 0)
  //   scroll_(1, mHorizOffset);
  // /* if empty lines become visible, there may be an opportunity to
  //    display more text by scrolling down */
  // else while (mLineStarts[mNVisibleLines-2] == -1)
  //   scroll_(mTopLineNum-1, mHorizOffset);

  // user request to display insert position
  if (display_insert_position_hint)
    display_insert();

  if (!mContinuousWrap) {
    // in case horizontal offset is now greater than longest line
    int maxhoffset = max(0, longest_vline()-text_area.w);
    if (mHorizOffset > maxhoffset)
      FTextBase::scroll_(mTopLineNumHint, maxhoffset);
  }

  mTopLineNumHint = mTopLineNum;
  mHorizOffsetHint = mHorizOffset;
  display_insert_position_hint = 0;

  if (mContinuousWrap ||
      hscrollbarvisible != mHScrollBar->visible() ||
      vscrollbarvisible != mVScrollBar->visible())
    redraw();

//  update_v_scrollbar();
  mVScrollBar->value(mTopLineNum, mNVisibleLines, 1, mNBufferLines + scrollbar_tweak);
  mVScrollBar->linesize(3);

  if (!mContinuousWrap)
    update_h_scrollbar();

#undef TOP_MARGIN
#undef BOTTOM_MARGIN
#undef LEFT_MARGIN
#undef RIGHT_MARGIN
