#include "ScopeDialog.h"
#include "main.h"
#include "trx.h"
#include "modem.h"
#include "globals.h"

Fl_Button 		 *btnCloseScope = (Fl_Button *)0;
Fl_Button		 *btnDispType = (Fl_Button *)0;
Fl_Double_Window *ScopeDialog = 0;
Digiscope 		 *digiscope = 0;

static void cb_btnCloseScope(Fl_Button* o, void*) {
  ScopeDialog->hide();
}

static void cb_btnDispType(Fl_Button * o, void *) {
	if (active_modem->get_mode() == MODE_RTTY) {
		if (rtty_modem->scopeMode() == Digiscope::RTTY)
			rtty_modem->scopeMode(Digiscope::XHAIRS);
		else 
			rtty_modem->scopeMode(Digiscope::RTTY);
	}
}

void createScopeDialog() {
	if (!ScopeDialog) {
		ScopeDialog = new Fl_Double_Window(108,135, "Scope");
		digiscope = new Digiscope (4, 4, 100, 100);
		btnDispType = new Fl_Button ( 4, 112, 50, 20, "Alt");
		btnDispType->callback( (Fl_Callback*)cb_btnDispType);
		btnCloseScope = new Fl_Button (54, 112, 50, 20, "Close" );
		btnCloseScope->callback((Fl_Callback*)cb_btnCloseScope);
    }
}

void ShowScope() {
	if (!ScopeDialog) createScopeDialog();
	ScopeDialog->show();
}
