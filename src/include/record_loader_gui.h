#ifndef RECORD_LOADER_GUI_H
#define RECORD_LOADER_GUI_H

#include <FL/Fl_Table.H>
#include <FL/Fl_Double_Window.H>

class RecordLoaderInterface ;

class DerivedRecordLst : public Fl_Table
{
	DerivedRecordLst();
	DerivedRecordLst( const DerivedRecordLst & );
	DerivedRecordLst & operator=( const DerivedRecordLst & );
public:
	DerivedRecordLst(int, int, int, int, const char * title = 0);
	virtual ~DerivedRecordLst();

	static void cbGuiUpdate();
	static void cbGuiReset();

	void AddRow(int R);
	void DrawRow(int R);
protected:
	void draw_cell(TableContext context,  		// table cell drawing
    		   int R=0, int C=0, int X=0, int Y=0, int W=0, int H=0);
};

#endif // RECORD_LOADER_GUI_H
