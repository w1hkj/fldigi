// ---------------------------------------------------------------------
// plot_xy.h
//
// Copyright (C) 2019
//		David Freese, W1HKJ
//
// This is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ---------------------------------------------------------------------

#ifndef plot_xy_H
#define plot_xy_H

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Widget.H>

#include "threads.h"

struct PLOT_XY {double x; double y;};

class plot_xy : public Fl_Widget {

public:

#define PLOT_XY_DEFAULT_WIDTH	100
#define PLOT_XY_DEFAULT_HEIGHT	100
#define	PLOT_XY_MAX_LEN			8192
#define PLOT_XY_NUM_GRIDS		100

private:
	PLOT_XY *buf_1;
	PLOT_XY *buf_2;

	int _len_1;
	int _len_2;
	int linecnt;
	bool xreverse;
	bool x_legend;
	bool y_legend;

	std::string sx_legend;
	std::string sy_legend;

	bool _show_1;
	bool _show_2;

	bool  _thick_lines;
	bool  _plot_over_axis;

protected:
	double xmin, xmax;
	double ymin, ymax;
	double x_graticule;
	double y_graticule;

	Fl_Color _bk_color;
	Fl_Color _axis_color;
	Fl_Color _color_1;
	Fl_Color _color_2;
	Fl_Color _legend_color;

public:
	plot_xy(int, int, int, int, const char *);
	~plot_xy();
	int handle(int);
	void resize(int x, int y, int w, int h);

	void draw();

	void data_1 (PLOT_XY *data_1, int len_1);
	void data_2 (PLOT_XY *data_2, int len_2);

	void x_scale( double _xmin, double _xmax, double _x_graticule) {
		xmin = _xmin; xmax = _xmax; x_graticule = _x_graticule;
	}
	void get_x_scale( double &_xmin, double &_xmax, double &_x_graticule) {
		_xmin = xmin; _xmax = xmax; _x_graticule = x_graticule; }

	void y_scale( double _ymin, double _ymax, double _y_graticule) {
		ymin = _ymin; ymax = _ymax; y_graticule = _y_graticule;
	}

	void get_y_scale( double _ymin, double _ymax, double _y_graticule) {
		_ymin = ymin; _ymax = ymax; _y_graticule = y_graticule; }

	void draw_x_legend(bool on = true) {
		x_legend = on;
	}
	void draw_y_legend(bool on = true) {
		y_legend = on;
	}
	void legends(bool on = true) {
		x_legend = on;
		y_legend = on;
	}

	void set_x_legend(std::string legend) {
		sx_legend = legend;
	}

	void set_y_legend(std::string legend) {
		sy_legend = legend;
	}

	void bk_color(Fl_Color c) { _bk_color = c; }
	Fl_Color bk_color() { return _bk_color; }

	void axis_color(Fl_Color c) { _axis_color = c; }
	Fl_Color axis_color() { return _axis_color; }

	void line_color_1(Fl_Color c) { _color_1 = c; }
	Fl_Color line_color_1() { return _color_1; }

	void line_color_2(Fl_Color c) { _color_2 = c; }
	Fl_Color line_color() { return _color_2; }

	void legend_color(Fl_Color c) { _legend_color = c; }
	Fl_Color legend_color() { return _legend_color; }

	void reverse_x(bool val) {
		xreverse = val;
	}

	void show_1(bool on) { _show_1 = on; }
	bool show_1() { return _show_1; }

	void show_2(bool on) { _show_2 = on; }
	bool show_2() { return _show_2; }

	void thick_lines(bool yes) { _thick_lines = yes; };
	void plot_over_axis(bool yes) { _plot_over_axis = yes; };

};

#endif
