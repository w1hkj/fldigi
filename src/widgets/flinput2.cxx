#include <config.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Tooltip.H>

#include "icons.h"
#include "flinput2.h"
#include "gettext.h"


enum { OP_UNDO, OP_CUT, OP_COPY, OP_PASTE, OP_DELETE, OP_CLEAR, OP_SELECT_ALL };

static Fl_Menu_Item cmenu[] = {
	{ make_icon_label(_("Undo"), edit_undo_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ make_icon_label(_("Cut"), edit_cut_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Copy"), edit_copy_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Paste"), edit_paste_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Delete"), trash_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Clear"), edit_clear_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ make_icon_label(_("Select All"), edit_select_all_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ 0 }
};
static bool cmenu_init = false;


Fl_Input2::Fl_Input2(int x, int y, int w, int h, const char* l)
	: Fl_Input(x, y, w, h, l)
{
	if (!cmenu_init) {
		for (size_t i = 0; i < sizeof(cmenu)/sizeof(*cmenu) - 1; i++)
			if (cmenu[i].labeltype() == _FL_MULTI_LABEL)
				set_icon_label(&cmenu[i]);
		cmenu_init = true;
	}
}

int Fl_Input2::handle(int event)
{
	switch (event) {
	case FL_KEYBOARD: { // stop the move-to-next-field madness, we have Tab for that!
		int b = Fl::event_key();
		int p = position();
		if (unlikely((b == FL_Left && p == 0) || (b == FL_Right && p == size()) ||
			     (b == FL_Up && line_start(p) == 0) ||
			     (b == FL_Down && line_end(p) == size())))
			return 1;
	}
		return Fl_Input::handle(event);
	case FL_PUSH:
		if (Fl::event_button() == FL_RIGHT_MOUSE)
			break;
		// fall through
	default:
		return Fl_Input::handle(event);
	}

	bool sel = position() != mark(), ro = readonly();
	set_active(&cmenu[OP_UNDO], !ro);
	set_active(&cmenu[OP_CUT], !ro && sel);
	set_active(&cmenu[OP_COPY], sel);
	set_active(&cmenu[OP_PASTE], !ro);
	set_active(&cmenu[OP_DELETE], !ro && sel);
	set_active(&cmenu[OP_CLEAR], !ro && size());
	set_active(&cmenu[OP_SELECT_ALL], size());

	take_focus();
	window()->cursor(FL_CURSOR_DEFAULT);
	int t = Fl_Tooltip::enabled();
	Fl_Tooltip::disable();
	const Fl_Menu_Item* m = cmenu->popup(Fl::event_x(), Fl::event_y());
	Fl_Tooltip::enable(t);

	if (!m)
		return 1;
	switch (m - cmenu) {
	case OP_UNDO:
		undo();
		break;
	case OP_CUT:
		cut();
		copy_cuts();
		break;
	case OP_COPY:
		copy(1);
		break;
	case OP_PASTE:
		Fl::paste(*this, 1);
		break;
	case OP_DELETE:
		cut();
		break;
	case OP_CLEAR:
		cut(0, size());
		break;
	case OP_SELECT_ALL:
		position(0, size());
		break;
	}

	return 1;
}
