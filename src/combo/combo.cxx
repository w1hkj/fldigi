#include <config.h>

#include "combo.h"
#include "debug.h"

void popbrwsr_cb (Fl_Widget *v, long d);

Fl_PopBrowser::Fl_PopBrowser (int X, int Y, int W, int H, retvals R)
 : Fl_Window (X, Y, W, H, "")
{
  Rvals = R;
  hRow  = H;
  wRow  = W;
  clear_border();
  box(FL_BORDER_BOX);
  popbrwsr = new Fl_Select_Browser(0,0,wRow,hRow,0);
  popbrwsr->callback ( (Fl_Callback*)popbrwsr_cb);
  parent = 0;
  end();
}

Fl_PopBrowser::~Fl_PopBrowser ()
{
}

int Fl_PopBrowser::handle(int event)
{
	if (!Fl::event_inside( child(0) ) && event == FL_PUSH) {
		pophide();
		return 1;
 	}
	return Fl_Group::handle(event);
}

void Fl_PopBrowser::add(char *s, void *d)
{
  popbrwsr->add(s,d);
}

void Fl_PopBrowser::clear()
{
  popbrwsr->clear();
}

void Fl_PopBrowser::sort()
{
  return;
}

void Fl_PopBrowser::popshow (int x, int y)
{
  int nRows = popbrwsr->size();
  int height = (nRows > 10 ? 10 : nRows)*hRow;// + 4;

  if (nRows == 0) return;
  popbrwsr->resize (0, 0, wRow, height);
  resize (x, y, wRow, height);
// locate first occurance of Output string value in the list
// and display that if found
  int i = 0;
  while (	i < parent->listsize && 
			strcasecmp (
				parent->Output->value(),
				parent->datalist[i]->s ) != 0 
		) i++;
  if (i > parent->listsize) i = 0;
  show ();
  popbrwsr->topline (i+1);

  Fl::grab(this);
}

void Fl_PopBrowser::pophide ()
{
  hide ();
  Fl::release();
}   

void Fl_PopBrowser::popbrwsr_cb_i (Fl_Widget *v, long d)
{
  Fl_PopBrowser *me = (Fl_PopBrowser *)(v->parent());
//  Fl_Output *tgt = me->Rvals.Inp;
  Fl_Input *tgt = me->Rvals.Inp;
// update the return values
  if (tgt) {
    int row = (me->popbrwsr)->value();
    if (row > 0) {
      tgt->value ((me->popbrwsr)->text (row));
      me->Rvals.retval = (me->popbrwsr)->data (row);
      *(me->Rvals.idx) = row;
    }
  }
  me->pophide();
// user selected an item from the browser list, so execute the
// callback if one is registered.
  if (me->parent)
    (me->parent)->do_callback();
  return;
}

void popbrwsr_cb (Fl_Widget *v, long d)
{
  ((Fl_PopBrowser *)(v))->popbrwsr_cb_i (v, d);
  return;
}

/* XPM Select_Browser Label */
static const char *brws_12[] = {
/* width height num_colors chars_per_pixel */
"    12    12        3            1",
/* colors */
". c #000000",
"d c none",
"e c #ffffff",
/* pixels */
"dddddddddddd",
"dddddddddddd",
"dddddddddddd",
"............",
"d..........d",
"dd........dd",
"ddd......ddd",
"dddd....dddd",
"ddddd..ddddd",
"dddddddddddd",
"dddddddddddd",
"dddddddddddd",
};


void Fl_ComboBox::fl_popbrwsr(Fl_Widget *p)
{
  Fl_Widget *who = this, *parent;
// compute the x,y position for the pop-up browser window
// x() and y() are locations relative to the current window
// also need to know where the root window for the application
// to compute the screen x,y position of the popup
  int xpos = who->x(), ypos = who->h() + who->y();
  parent = who;
  while (parent) {
    who = parent;
    parent = parent->parent();
    if (parent == 0) {
      xpos += who->x();
      ypos += who->y();
    }
  }
  if (Brwsr == 0) {
    Brwsr = new Fl_PopBrowser(xpos, ypos, width, height, R);
  }
// pass the calling widget to the popup browser so that the
// correct callback function can be called when the user selects an item
// from the browser list
  Brwsr->parent = (Fl_ComboBox *) p;
  Brwsr->popshow(xpos, ypos);
  return;
}

void btnComboBox_cb (Fl_Widget *v, void *d)
{
  Fl_Widget *p = v->parent();
  ((Fl_ComboBox *)p)->fl_popbrwsr (p);
  return;
}


Fl_ComboBox::Fl_ComboBox (int X,int Y,int W,int H, const char *L)
 : Fl_Group (X, Y, W, H, L)
{
  width = W; height = H - 4;
  Btn = new Fl_Button (X + W - 18, Y + 1, 18, H - 2);
  btnmap = new Fl_Pixmap(brws_12);
  btnmap->label(Btn);
  Btn->callback ((Fl_Callback *)btnComboBox_cb, 0);
  Output = new Fl_Input (X, Y, W-18, H);

  Brwsr = 0;
  datalist = new datambr *[FL_COMBO_LIST_INCR];
  maxsize = FL_COMBO_LIST_INCR;
  for (int i = 0; i < FL_COMBO_LIST_INCR; i++) datalist[i] = 0;
  listsize = 0;
  listtype = 0;
  end();
  R.Inp = Output;
  R.retval = retdata;
  R.idx = &idx;
}

Fl_ComboBox::~Fl_ComboBox()
{
	if (Brwsr) delete Brwsr;
	for (int i = 0; i < listsize; i++) {
		if (datalist[i]) {
			if (datalist[i]->s) delete [] datalist[i]->s;
			delete datalist[i];
		}
	}
	delete [] datalist;
	delete btnmap;
}

void Fl_ComboBox::type (int t)
{
  listtype = t;
}

void Fl_ComboBox::readonly()
{
    Output->type(FL_NORMAL_OUTPUT);
}

// ComboBox value is contained in the Output widget

void Fl_ComboBox::value( const char *s )
{
	int i;
	if ((listtype & FL_COMBO_UNIQUE_NOCASE) == FL_COMBO_UNIQUE_NOCASE) {
		for (i = 0; i < listsize; i++) {
			if (strcasecmp (s, datalist[i]->s) == 0)
				break;
		}
    } else {
		for (i = 0; i < listsize; i++) {
			if (strcmp (s, datalist[i]->s) == 0)
				break;
		}
    }
	if ( i < listsize)
		Output->value(datalist[i]->s);
}

void Fl_ComboBox::put_value(const char *s)
{
	value(s);
}

void Fl_ComboBox::index(int i)
{
	if (i >= 0 && i < listsize)
		Output->value( datalist[i]->s);
}


const char *Fl_ComboBox::value()
{
  return (Output->value ());
}

int Fl_ComboBox::index() {
  return idx;
}

void * Fl_ComboBox::data() {
  return retdata; 
}

void Fl_ComboBox::add( const char *s, void * d)
{
  if (Brwsr == 0) {
    Brwsr = new Fl_PopBrowser(0, 0, width, height, R);
  }
// test for uniqueness of entry if required
  if ((listtype & FL_COMBO_UNIQUE) == FL_COMBO_UNIQUE) {
    if ((listtype & FL_COMBO_UNIQUE_NOCASE) == FL_COMBO_UNIQUE_NOCASE) {
      for (int i = 0; i < listsize; i++) {
        if (strcasecmp (s, datalist[i]->s) == 0)
	  return;
      }
    } else {
      for (int i = 0; i < listsize; i++) {
        if (strcmp (s, datalist[i]->s) == 0)
	  return;
      }
    }
  }
// not unique or not in list, so add this entry
  datalist[listsize] = new datambr;
  datalist[listsize]->s = new char [strlen(s) + 1];
  datalist[listsize]->s[0] = 0;
  strcpy (datalist[listsize]->s, s);
  datalist[listsize]->d = d;
  Brwsr->add(datalist[listsize]->s,d);
  listsize++;
  if (listsize == maxsize) {
    int nusize = maxsize + FL_COMBO_LIST_INCR;
    datambr **temparray = new datambr *[nusize];
    for (int i = 0; i < listsize; i++)	temparray[i] = datalist[i];
	delete [] datalist;
    datalist = temparray;
    maxsize = nusize;
  }
}

void Fl_ComboBox::clear()
{
	if (Brwsr == 0)
		Brwsr = new Fl_PopBrowser(0, 0, width, height, R);
	else
		Brwsr->clear();
	
	if (listsize == 0) return;
	for (int i = 0; i < listsize; i++) {
		delete [] datalist[i]->s;
		delete datalist[i];
	}
	listsize = 0;
}

int DataCompare( const void *x1, const void *x2 )
{
  int cmp;
  datambr *X1, *X2;
  X1 = *(datambr **)(x1);
  X2 = *(datambr **)(x2);
  cmp = strcasecmp (X1->s, X2->s);
  if (cmp < 0)
    return -1;
  if (cmp > 0)
    return 1;
  return 0;
}

void Fl_ComboBox::sort() {
  Brwsr->clear ();
  qsort (&datalist[0],
         listsize,
         sizeof (datambr *),
         DataCompare);
  for (int i = 0; i < listsize; i++)
    Brwsr->add (datalist[i]->s, datalist[i]->d);
}

void Fl_ComboBox::textfont (int fnt)
{
  Output->textfont (fnt);
}

void Fl_ComboBox::textsize (uchar n)
{
  Output->textsize (n);
}



