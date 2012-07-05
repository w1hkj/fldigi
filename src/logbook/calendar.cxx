#include <config.h>

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <FL/Fl.H>
#include <FL/Fl_Pixmap.H>

#include "pixmaps.h"
#include "calendar.h"

void popcal_cb (Fl_Widget *v, long d);

static void fl_calendar_button_cb (Fl_Button *a, void *b)
{
  long j=0;
  Fl_Calendar *c = (Fl_Calendar *)b;
  Fl_Button   *sb;
  int numdays = c->daysinmonth () + 1;
  for (int i=1; i < numdays; i++) {
    sb = c->day_button(i);
    sb->color (52);
    if (a == sb) {
	    c->Day (i);
	    j = i;
      sb->color (sb->selection_color());
      if (c->target) {
        ((Fl_Input2 *)(c->target))->value(c->szDate(c->calfmt));
        (c->target)->redraw();
      }
    }
  }
  c->redraw();
  c->do_callback(c, j);
}

void
Fl_Calendar_Base::setTarget (Fl_Widget *tgt)
{
  target = tgt;
}

Fl_Calendar_Base::Fl_Calendar_Base (int x, int y, int w, int h, 
				    const char *l) : Fl_Group (x, y, w, h, l), Date () 
{
  int i;  
  
  for (i = 0; i<(7*6); i++)
  {
    days[i] = new Fl_Button ((w/7)*(i%7) + x,
			     (h/6)*(i/7) + y,
			     (w/7),
			     (h/6));
    days[i]->down_box (FL_THIN_DOWN_BOX);
    days[i]->labelsize (10);
    days[i]->box (FL_THIN_UP_BOX);
    days[i]->color (52);
    days[i]->callback ((Fl_Callback*)&fl_calendar_button_cb, (void *)this);
  }
  calfmt = 0;
}

void Fl_Calendar_Base::csize (int cx, int cy, int cw, int ch)
{
  int i;
  for (i = 0; i<(7*6); i++)
  {
    days[i]->resize ((cw/7)*(i%7) + cx,
		   (ch/6)*(i/7) + cy,
		   (cw/7),
	    (ch/6));
  }
}


void
Fl_Calendar_Base::update ()
{
  int dow = dayofweek (Year(), Month(), 1);
  int dim = daysinmonth (Month(), isleapyear (Year()));
  int i;

  for (i=0; i<dow; i++)
    {
      days[i]->hide ();
    }

  for (i=(dim+dow); i<(6*7); i++)
    {
      days[i]->hide ();
    }
  
  for (i=dow; i<(dim+dow); i++)
    {
      char t[8];
      snprintf (t, sizeof(t), "%d", (i-dow+1));
      days[i]->label (strdup(t));
      days[i]->color (52);
      if ((i-dow+1) == Day())
        days[i]->color (selection_color());
      days[i]->show ();
    }
}

Fl_Button *
Fl_Calendar_Base::day_button (int i)
{
  if ((i > 0) && (i <= daysinmonth ()))
    return days[i + dayofweek (Year(), Month(), 1) - 1];
  return 0;
}



static void 
fl_calendar_prv_month_cb (Fl_Button *, void *b)
{
  Fl_Calendar *c = (Fl_Calendar *)b;
  c->previous_month ();
  c->do_callback(c, (long)0);
}

static void 
fl_calendar_nxt_month_cb (Fl_Button *, void *b)
{
  Fl_Calendar *c = (Fl_Calendar *)b;
  c->next_month ();
  c->do_callback(c, (long)0);
}

static void 
fl_calendar_prv_year_cb (Fl_Button *, void *b)
{
  Fl_Calendar *c = (Fl_Calendar *)b;
  c->previous_year ();
  c->do_callback(c, (long)0);
}

static void 
fl_calendar_nxt_year_cb (Fl_Button *, void *b)
{
  Fl_Calendar *c = (Fl_Calendar *)b;
  c->next_year ();
  c->do_callback(c, (long)0);
}

Fl_Calendar::Fl_Calendar (int x, int y, int w, int h, 
			  const char *l) : Fl_Calendar_Base (x, y, w, h, l) 
{
  int i, bw;
  for (i = 0; i<7; i++) {
//    weekdays[i] = new Fl_Button ((w/7)*(i%7) + x,
    weekdays[i] = new Fl_Box ((w/7)*(i%7) + x,
                              (h/8)*((i/7)+1) + y,
                              (w/7),
                              (h/8));
    weekdays[i]->box (FL_THIN_UP_BOX);  
    weekdays[i]->labelsize (10);
    weekdays[i]->color (52);
  }
  
  weekdays[SUNDAY]->label ("S");
  weekdays[MONDAY]->label ("M");
  weekdays[TUESDAY]->label ("T");
  weekdays[WEDNESDAY]->label ("W");
  weekdays[THURSDAY]->label ("T");
  weekdays[FRIDAY]->label ("F");
  weekdays[SATURDAY]->label ("S");

  bw = w/10 < 16 ? 16 : w/10;
  prv_year = new Fl_Button (x, y, bw, (h/8), "@<<");
  prv_year->box (FL_THIN_UP_BOX);
  prv_year->labeltype (FL_SYMBOL_LABEL);
  prv_year->labelsize (10);
  prv_year->down_box (FL_THIN_DOWN_BOX);
  prv_year->callback ((Fl_Callback*)&fl_calendar_prv_year_cb, (void *)this);  

  prv_month = new Fl_Button (x + bw, y, bw, (h/8), "@<");
  prv_month->box (FL_THIN_UP_BOX);
  prv_month->labeltype (FL_SYMBOL_LABEL);
  prv_month->labelsize (10);
  prv_month->down_box (FL_THIN_DOWN_BOX);
  prv_month->callback ((Fl_Callback*)&fl_calendar_prv_month_cb, (void *)this);  

  nxt_month = new Fl_Button (x + w - 2*bw, y, bw, (h/8), "@>");
  nxt_month->box (FL_THIN_UP_BOX);
  nxt_month->labeltype (FL_SYMBOL_LABEL);
  nxt_month->labelsize (10);
  nxt_month->down_box (FL_THIN_DOWN_BOX);
  nxt_month->callback ((Fl_Callback*)&fl_calendar_nxt_month_cb, (void *)this);
  
  nxt_year = new Fl_Button (x + w - bw, y, bw, (h/8), "@>>");
  nxt_year->box (FL_THIN_UP_BOX);
  nxt_year->labeltype (FL_SYMBOL_LABEL);
  nxt_year->labelsize (10);
  nxt_year->down_box (FL_THIN_DOWN_BOX);
  nxt_year->callback ((Fl_Callback*)&fl_calendar_nxt_year_cb, (void *)this);

//  caption = new Fl_Button (x + (w/10)*2, y, (6*w/10), (h/8));
  caption = new Fl_Box (x + 2*bw, y, w - 4*bw, (h/8));
  caption->box (FL_THIN_UP_BOX);
  caption->labeltype (FL_SYMBOL_LABEL);
  caption->labelfont (1);
  if (bw < 20)
    caption->labelsize (9);
  else
    caption->labelsize (11);
//  caption->down_box (FL_THIN_DOWN_BOX);
      
  Fl_Calendar_Base::csize (x, y + (2*h/8), w, (6*h/8));

  target = 0;

  update ();
}

void
Fl_Calendar::csize (int cx, int cy, int cw, int ch)
{
  int i;
  for (i = 0; i<7; i++)
    {
//      weekdays[i] = new Fl_Button ((cw/7)*(i%7) + cx,
      weekdays[i] = new Fl_Box ((cw/7)*(i%7) + cx,
				   (ch/8)*((i/7)+1) + cy,
				   (cw/7),
				   (ch/8));
    }

  prv_month->resize (cx + (cw/10), cy, (cw/10), (ch/8));
  nxt_month->resize (cx + (cw/10)*8, cy, (cw/10), (ch/8));
  prv_year->resize (cx, cy, (cw/10), (ch/8));
  nxt_year->resize (cx + (cw/10)*9, cy, (cw/10), (ch/8));
  caption->resize (cx + (cw/10)*2, cy, (cw/10)*6, (ch/8));
  
  Fl_Calendar_Base::csize (cx, cy + (2*ch/8), cw, (6*ch/8));  
}

void 
Fl_Calendar::update ()
{
  int dow = dayofweek (Year(), Month(), 1);
  int dim = daysinmonth (Month(), isleapyear (Year()));
  int i;
  
  for (i=dow; i<(dim+dow); i++)
    {
      char t[8];
      snprintf (t, sizeof(t), "%d", (i-dow+1));
      days[i]->label (strdup(t));
    }

  char tmp[32];
  snprintf (tmp, sizeof(tmp), "%s %d", month_name[Month()-1], Year());
  Fl_Calendar_Base::update ();
  if (caption->label ())
    free ((void *) caption->label ());
  caption->label (strdup(tmp));
  redraw ();
}

void Fl_Calendar::today ()
{
  Date::today();
  update ();
}

void Fl_Calendar::previous_month ()
{
  Date::previous_month();
  update ();
}

void
Fl_Calendar::next_month ()
{
  Date::next_month();
  update ();
}

void
Fl_Calendar::previous_year ()
{
  Date::previous_year();
  update ();
}

void Fl_Calendar::next_year ()
{
  Date::next_year();
  update ();
}

void Fl_Calendar::setDate(int m, int d, int y)
{
  Date::setDate(m,d,y);
}

int 
Fl_Calendar::handle (int event) 
{ 
  int m, d, y, o, md; 

  switch (event) 
    { 
    case FL_FOCUS: 
    case FL_UNFOCUS: 
      return 1; 
      
    case FL_KEYBOARD: 
      m = Month (); 
      d = Day (); 
      y = Year (); 
      switch(Fl::event_key ()) 
	{ 
        case FL_Enter:
          do_callback(this, d);
          return 1;
          break;
	case FL_Up: 
	  o = -7; 
	  break; 
	case FL_Down: 
	  o = 7; 
	  break; 
	case FL_Right: 
	  o = 1; 
	  break; 
	case FL_Left: 
	  o = -1; 
	  break; 
	case FL_Page_Up:
	  previous_month (); 
	  return 1; 
	case FL_Page_Down: 
	  next_month (); 
	  return 1; 
	default: 
	  return Fl_Group::handle (event); 
	} 
    if (datevalid (y, m, d + o))
      setDate (m, d + o, y); 
      else 
	{ 
	  if (o < 0) 
	    { 
	      previous_month (); 
	      m = Month (); 
	      y = Year (); 
	      md = daysinmonth (m, isleapyear (y)); 
	      d = d + o + md; 
        setDate (m, d, y);
	    } 
	  else 
	    { 
	      md = daysinmonth (m, isleapyear (y)); 
	      next_month (); 
	      m = Month (); 
	      y = Year (); 
	      d = d + o - md;
        setDate (m, d, y); 
	    } 
	} 
      return 1;
    }
  return Fl_Group::handle (event);
}


// Popup Calendar class

Fl_PopCal::Fl_PopCal (int X, int Y, int W, int H, Fl_Input2 * tgt)
 : Fl_Window (X, Y, W, H, "")
{
  target = tgt;
  clear_border();
  box(FL_UP_BOX);
//  popcal = new Fl_Calendar(2, 2);
  popcal = new Fl_Calendar(2, 2, W-4, H-4);
  popcal->callback ( (Fl_Callback*)popcal_cb);
  end();
}


Fl_PopCal::~Fl_PopCal ()
{
}

void Fl_PopCal::popcalfmt (int i) 
{
  popcalfmt_ = i;
}

int Fl_PopCal::popcalfmt () 
{
  return popcalfmt_;
}

void Fl_PopCal::setDate (int m, int d, int y)
{
  popcal->setDate (m,d,y);
  popcal->update();
}

int Fl_PopCal::handle(int event)
{
  int ex = Fl::event_x_root(),
      ey = Fl::event_y_root();
  if (event == FL_PUSH) {
    if ( ex < x() || ex > (x() + w()) ||
         ey < y() || ey > (y() + h()) ) {
      pophide();
      return 1;
    }
  }
  if (Fl_Group::handle(event)) return 1;
  return 0;
}

void Fl_PopCal::popposition (int x, int y)
{
  position (x, y);
}

void Fl_PopCal::popshow ()
{
  show ();
  Fl::grab(this);
}

void Fl_PopCal::pophide ()
{
  hide ();
  Fl::release();
}   

void Fl_PopCal::popcal_cb_i (Fl_Widget *v, long d)
{
  int ey = Fl::event_y_root();
  Fl_PopCal *me = (Fl_PopCal *)(v->parent());
  Fl_Input2 *tgt = me->target;
  if (ey > me->y() + 40) {
    if (d && tgt) 
      tgt->value (((Fl_Calendar *)v)->szDate (me->popcalfmt_));
    me->pophide();
  }
  return;
}

void popcal_cb (Fl_Widget *v, long d)
{
  ((Fl_PopCal *)(v))->popcal_cb_i (v, d);
  return;
}

void
Fl_DateInput::fl_popcal()
{
  Fl_Widget *who = this, *parent;
  int xpos = who->x(), ypos = who->h() + who->y();
  int w = who->w(), h;
  int m = 0, d = 0, y = 0;

  w = w < 140 ? 140 : w;
  w = w - (w % 7);
  h = 8*(w/7);
  w += 4; h += 4;
  parent = who;
  while (parent) {
    who = parent;
    parent = parent->parent();
    if (parent == 0) {
      xpos += who->x();
      ypos += who->y();
    }
  }
  if (!Cal)
//    Cal = new Fl_PopCal(xpos, ypos, 7*20+4, 8*20+4, Input);
    Cal = new Fl_PopCal(xpos, ypos, w, h, Input);
  else
    Cal->popposition(xpos, ypos);

  if (popcalfmt_ < 3) {
    switch (popcalfmt_) {
      case 0:
      case 1:
        sscanf(Input->value(), "%d/%d/%d", &m, &d, &y);
        break;
      case 2:
      default:
        sscanf(Input->value(),"%4d%2d%2d", &y, &m, &d);
        break;
    }
    if (y < 10) y+=2000;
    if (y < 100) y+=1900;
    Cal->setDate (m,d,y);
  }
  Cal->popcalfmt (popcalfmt_);

  Cal->popshow();
  return;
}

void btnDateInput_cb (Fl_Widget *v, void *d)
{
  ((Fl_DateInput *)(v->parent()))->fl_popcal ();
  return;
}


Fl_DateInput::Fl_DateInput (int X,int Y,int W,int H, const char *L)
 : Fl_Group (X, Y, W, H, 0)
{
  Btn = new Fl_Button (X + W - H, Y, H, H);
  (new Fl_Pixmap (time_icon))->label (Btn);
  Btn->callback ((Fl_Callback *)btnDateInput_cb, 0);
  Input = new Fl_Input2 (X, Y, W-H, H, L);

  popcalfmt_ = 0;
  Cal = 0;
  end();
}

void Fl_DateInput::align (Fl_Align how)
{
  Input->align(how);
}

// DateInput value is contained in the Input widget

void Fl_DateInput::value( const char *s )
{
  Input->value (s);
}

const char *Fl_DateInput::value()
{
  return (Input->value ());
}


void Fl_DateInput::textfont(int tf)
{
  Input->textfont (tf);
}
 
void Fl_DateInput::textsize(int sz)
{
  Input->textsize (sz);
}

void Fl_DateInput::textcolor(Fl_Color c)
{
  Input->textcolor(c);
}

void Fl_DateInput::color(Fl_Color c)
{
  Input->color(c);
}

void Fl_DateInput::labelfont(int fnt)
{
  Input->labelfont(fnt);
}

void Fl_DateInput::labelsize(int size)
{
  Input->labelsize(size);
}

void Fl_DateInput::labelcolor(int clr)
{
  Input->labelcolor(clr);
}

void Fl_DateInput::format (int fmt)
{
  switch (fmt) {
    case 0: 
    case 1: 
    case 2:
    case 3:
    case 4: 
      popcalfmt_ = fmt; 
      break;
    default : 
      popcalfmt_ = 0;
  }
}

void Fl_DateInput::take_focus() {
  Input->take_focus();
}
