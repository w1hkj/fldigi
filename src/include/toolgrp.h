#ifndef _HAVE_TOOL_GROUP_HDR_
#define _HAVE_TOOL_GROUP_HDR_

#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>

#include "dock_gp.h"
#include "drag_btn.h"

class toolgrp : public Fl_Group
{
private:
	// control variables
	short _docked;
	dockgroup *dock;

	// constructor helper function
	void create_dockable_group(void);
	void create_docked(dockgroup *d);
	void create_floating(dockgroup *d, int state, int x, int y, int w, int h, const char *l);

protected:
	// Widgets used by the toolbar
//	Fl_Button *dismiss;
	drag_btn *dragger;
	Fl_Group *inner_group;

	// Sets whether window is docked or not.
	void docked(short r);
	
	// Defines which dock the group can dock into
	void set_dock(dockgroup *w) {dock = w;}
	// get the dock group ID
	dockgroup *get_dock(void) {return dock;}

	// generic callback function for the dismiss button
//	static void cb_dismiss(Fl_Button*, void* v);

public:
	// Constructors for docked/floating window
	toolgrp(dockgroup *d, int f, int w, int h, const char *l = 0);
	toolgrp(dockgroup *d, int f, int x, int y, int w, int h, const char *l = 0);

	// methods for hiding/showing *all* the floating windows
	static void show_all(void);
	static void hide_all(void);

	// Tests whether window is docked, undocked or hidden
	short docked() { return _docked; }

	// generic callback function for the dock/undock checkbox
	void dock_grp(void* v);
	void undock_grp(void* v);
	void hide_show();

	// wrap some basic Fl_Group functions to access the enclosed inner_group
	inline void begin() {inner_group->begin(); }
	inline void end() {inner_group->end(); Fl_Group::end(); }
	inline void resizable(Fl_Widget *box) {inner_group->resizable(box); }
	inline void resizable(Fl_Widget &box) {inner_group->resizable(box); }
	inline Fl_Widget *resizable() const { return inner_group->resizable(); }
	inline void add( Fl_Widget &w ) { inner_group->add( w ); }
	inline void add( Fl_Widget *w ) { inner_group->add( w ); }
	inline void insert( Fl_Widget &w, int n ) { inner_group->insert( w, n ); }
	inline void insert( Fl_Widget &w, Fl_Widget* beforethis ) { inner_group->insert( w, beforethis ); }
	inline void remove( Fl_Widget &w ) { inner_group->remove( w ); }
	inline void remove( Fl_Widget *w ) { inner_group->remove( w ); }
//	inline void add_resizable( Fl_Widget &box ) { inner_group->add_resizable( box ); }
};

#endif // _HAVE_TOOL_GROUP_HDR_

