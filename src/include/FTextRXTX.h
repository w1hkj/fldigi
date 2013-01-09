// ----------------------------------------------------------------------------
//      FTextView.h
//
// Copyright (C) 2007-2009
//              Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef FTextRXTX_H_
#define FTextRXTX_H_

#include <string>

#include "FTextView.h"

///
/// A TextBase subclass to display received & transmitted text
///
class FTextRX : public FTextView
{
public:
	FTextRX(int x, int y, int w, int h, const char *l = 0);
        ~FTextRX();

	virtual int	handle(int event);

#if FLDIGI_FLTK_API_MAJOR == 1 && FLDIGI_FLTK_API_MINOR == 3
	virtual void	add(unsigned int  c, int attr = RECV);
	virtual	void	add(const char *s, int attr = RECV)
        {
                while (*s)
                        add(*s++, attr);
        }
#else
	virtual void	add(unsigned char c, int attr = RECV);
	virtual	void	add(const char *s, int attr = RECV)
        {
                while (*s)
                        add(*s++, attr);
        }
#endif

	void		set_quick_entry(bool b);
	bool		get_quick_entry(void) { return menu[RX_MENU_QUICK_ENTRY].value(); }
	void		set_scroll_hints(bool b);
	bool		get_scroll_hints(void) { return menu[RX_MENU_SCROLL_HINTS].value(); }
	void		mark(FTextBase::TEXT_ATTR attr = CLICK_START);
	void		clear(void);

	void		setFont(Fl_Font f, int attr = NATTR);

protected:
	enum {
		RX_MENU_QRZ_THIS, RX_MENU_CALL, RX_MENU_NAME, RX_MENU_QTH,
		RX_MENU_STATE, RX_MENU_PROVINCE,RX_MENU_COUNTRY, RX_MENU_LOC,
		RX_MENU_RST_IN, RX_MENU_XCHG, RX_MENU_SERIAL,
		RX_MENU_DIV, RX_MENU_COPY, RX_MENU_CLEAR, RX_MENU_SELECT_ALL,
		RX_MENU_SAVE, RX_MENU_WRAP, RX_MENU_QUICK_ENTRY, RX_MENU_SCROLL_HINTS
	};

	void		handle_clickable(int x, int y);
	void		handle_qsy(int start, int end);
	void		handle_qso_data(int start, int end);
	void		handle_context_menu(void);
	void		menu_cb(size_t item);

	const char*	dxcc_lookup_call(int x, int y);
	static void	dxcc_tooltip(void* obj);

private:
	FTextRX();
	FTextRX(const FTextRX &t);

protected:
	static Fl_Menu_Item menu[];
	struct {
		bool enabled;
		float delay;
	} tooltips;
};


///
/// A FTextBase subclass to display and edit text to be transmitted
///
class FTextTX : public FTextEdit
{
public:
	FTextTX(int x, int y, int w, int h, const char *l = 0);

	virtual int	handle(int event);

	void		clear(void);
	void		clear_sent(void);
	int			nextChar(void);
	bool		eot(void);
	void		add_text(std::string s);
	void		pause() { PauseBreak = true; }

	void		setFont(Fl_Font f, int attr = NATTR);

protected:
	enum { TX_MENU_TX, TX_MENU_RX, TX_MENU_ABORT, TX_MENU_MFSK16_IMG,
	       TX_MENU_CUT, TX_MENU_COPY, TX_MENU_PASTE, TX_MENU_CLEAR, TX_MENU_READ,
	       TX_MENU_WRAP
	};
	int		handle_key_shortcuts(int key);
	int		handle_key(int key);
	int		handle_key_macro(int key);
	int		handle_dnd_drag(int pos);
	void		handle_context_menu(void);
	void		menu_cb(size_t item);
	void		change_keybindings(void);
	static int	kf_default(int c, Fl_Text_Editor_mod* e);
	static int	kf_enter(int c, Fl_Text_Editor_mod* e);
	static int	kf_delete(int c, Fl_Text_Editor_mod* e);
	static int	kf_cut(int c, Fl_Text_Editor_mod* e);
	static int	kf_paste(int c, Fl_Text_Editor_mod* e);

private:
	FTextTX();
	FTextTX(const FTextTX &t);

protected:
	static Fl_Menu_Item	menu[];
	bool		PauseBreak;
	int			txpos;
	int			utf8_txpos;
	static int	*ptxpos;
	int			bkspaces;
};

#endif // FTextRXTX_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
