void create_fl_digi_main_primary() {
// bx used as a temporary spacer
	Fl_Box *bx;
	int Wmacrobtn;
	int xpos;
	int ypos;
	int wBLANK;

	int fnt = progdefaults.FreqControlFontnbr;
	int freqheight = Hentry;
	fl_font(fnt, freqheight);
	int freqwidth = (int)fl_width("9") * 10;
	fl_font(progdefaults.LOGGINGtextfont, progdefaults.LOGGINGtextsize);

	int Y = 0;

#ifdef __APPLE__
	fl_mac_set_about(cb_mnuAboutURL, 0);
#endif

	IMAGE_WIDTH = 4000;

	Hwfall = progdefaults.wfheight;

	Wwfall = progStatus.mainW - 2 * DEFAULT_SW;

	int fixed_height =
		Hmenu +
		Hqsoframe +
		Hwfall +
		Hstatus;
	int hmacros = TB_HEIGHT * 4;

	fixed_height += hmacros;

//----------------------------------------------------------------------
// needed to prevent user from manually modifying fldigi_def.xml
// with values to would cause the UI to seg fault
	if (progdefaults.HellRcvHeight < 14) progdefaults.HellRcvHeight = 14;
	if (progdefaults.HellRcvHeight > 42) progdefaults.HellRcvHeight = 42;
	if (progdefaults.HellRcvWidth < 1) progdefaults.HellRcvWidth = 1;
	if (progdefaults.HellRcvWidth > 4) progdefaults.HellRcvWidth = 4;
//----------------------------------------------------------------------

	minhtext = 2 * progdefaults.HellRcvHeight + 4;//6;

	int Htext = 3 * minhtext;
	if (Htext < 120) Htext = 120;

	main_hmin = Htext + fixed_height;

// developer usage
//cout << "=============================================================" << endl;
//cout << "min main_height ..... " << main_hmin << endl;
//cout << " = Hmenu ............ " << Hmenu << endl;
//cout << " + Hqsoframe ........ " << Hqsoframe << endl;
//cout << " + Hwfall ........... " << Hwfall << endl;
//cout << " + Hstatus  ......... " << Hstatus << endl;
//cout << " + Hmacros .......... " << hmacros << endl;
//cout << " + text height ...... " << Htext << endl;
//cout << "=============================================================" << endl;

	if (progStatus.mainH < main_hmin) {
		progStatus.mainH = main_hmin;
	}

	if (progStatus.tile_y > Htext) progStatus.tile_y = Htext / 2;

	int W = progStatus.mainW;
	int H = main_hmin;
	int xtmp = 0;

	fl_digi_main = new Fl_Double_Window(
			progStatus.mainX, progStatus.mainY, W, H);

		int lfont = fl_digi_main->labelfont();
		int lsize = FL_NORMAL_SIZE;
		fl_font(lfont, lsize);

{ // mnuFrame
		mnuFrame = new Fl_Group(0,0, W, Hmenu);
			mnu = new Fl_Menu_Bar(0, 0, W - 325, Hmenu);
			// do some more work on the menu
			for (size_t i = 0; i < sizeof(menu_)/sizeof(menu_[0]); i++) {
				// FL_NORMAL_SIZE may have changed; update the menu items
				if (menu_[i].text) {
					menu_[i].labelsize_ = lsize;
				}
				// set the icon label for items with the multi label type
				if (menu_[i].labeltype() == _FL_MULTI_LABEL)
					icons::set_icon_label(&menu_[i]);
			}
			mnu->menu(menu_);
			toggle_visible_modes(NULL, NULL);

			tx_timer = new Fl_Box(W - 325, 0, 75, Hmenu, "");
			tx_timer->box(FL_UP_BOX);
			tx_timer->color(FL_BACKGROUND_COLOR);
			tx_timer->labelcolor(FL_BACKGROUND_COLOR);
			tx_timer->labelsize(FL_NORMAL_SIZE - 1);
			tx_timer->labelfont(lfont);

			btnAutoSpot = new Fl_Light_Button(W - 250, 0, 50, Hmenu, "Spot");
			btnAutoSpot->selection_color(progdefaults.SpotColor);
			btnAutoSpot->callback(cbAutoSpot, 0);
			btnAutoSpot->deactivate();
			btnAutoSpot->labelsize(FL_NORMAL_SIZE - 1);
			btnAutoSpot->labelfont(lfont);

			btnRSID = new Fl_Light_Button(W - 200, 0, 50, Hmenu, "RxID");
			btnRSID->tooltip("Receive RSID");
			btnRSID->selection_color(
				progdefaults.rsidWideSearch ? progdefaults.RxIDwideColor : progdefaults.RxIDColor);
			btnRSID->value(progdefaults.rsid);
			btnRSID->callback(cbRSID, 0);
			btnRSID->labelsize(FL_NORMAL_SIZE - 1);
			btnRSID->labelfont(lfont);

			btnTxRSID = new Fl_Light_Button(W - 150, 0, 50, Hmenu, "TxID");
			btnTxRSID->selection_color(progdefaults.TxIDColor);
			btnTxRSID->tooltip("Transmit RSID");
			btnTxRSID->callback(cbTxRSID, 0);
			btnTxRSID->labelsize(FL_NORMAL_SIZE - 1);
			btnTxRSID->labelfont(lfont);

			btnTune = new Fl_Light_Button(W - 100, 0, 50, Hmenu, "TUNE");
			btnTune->selection_color(progdefaults.TuneColor);
			btnTune->callback(cbTune, 0);
			btnTune->labelsize(FL_NORMAL_SIZE - 1);
			btnTune->labelfont(lfont);

			btnMacroTimer = new Fl_Button(W - 50, 0, 50, Hmenu);
			btnMacroTimer->labelcolor(FL_DARK_RED);
			btnMacroTimer->callback(cbMacroTimerButton);
			btnMacroTimer->set_output();
			btnMacroTimer->labelsize(FL_NORMAL_SIZE - 1);
			btnMacroTimer->labelfont(lfont);

			mnuFrame->resizable(mnu);
		mnuFrame->end();
}

	int alt_btn_width = 2 * DEFAULT_SW;

{ // Constants
		// reset the message dialog font
		fl_message_font(FL_HELVETICA, FL_NORMAL_SIZE);
		// reset the tooltip font
		Fl_Tooltip::font(FL_HELVETICA);
		Fl_Tooltip::size(FL_NORMAL_SIZE);
		Fl_Tooltip::hoverdelay(0.5);
		Fl_Tooltip::delay(2.0);
		Fl_Tooltip::enable(progdefaults.tooltips);

		Y += mnuFrame->h();
}
		TopFrame1 = new Fl_Group(
			0, Y,
			fl_digi_main->w(), Hqsoframe);
{ // TopFrame1

		int fnt1 = progdefaults.FreqControlFontnbr;
		int freqheight1 = 2 * Hentry + pad - 2;
		fl_font(fnt1, freqheight1);
		int freqwidth1 = (int)fl_width("9") * 10;
		int mode_cbo_w = (freqwidth1 - 2 * Wbtn - 3 * pad) / 2;
		int bw_cbo_w = freqwidth1 - 2 * Wbtn - 3 * pad - mode_cbo_w;
		int smeter_w = mode_cbo_w + bw_cbo_w + pad;
		int rig_control_frame_width = freqwidth1 + 3 * pad;

		fl_font(progdefaults.LOGGINGtextfont, progdefaults.LOGGINGtextsize);

		RigControlFrame = new Fl_Group(
			0, TopFrame1->y(),
			rig_control_frame_width, Hqsoframe);
{ // RigControlFrame 1

			RigControlFrame->box(FL_FLAT_BOX);

			qsoFreqDisp1 = new cFreqControl(
				pad, RigControlFrame->y() + pad,
				freqwidth1, freqheight1, "10");
			qsoFreqDisp1->box(FL_DOWN_BOX);
			qsoFreqDisp1->color(FL_BACKGROUND_COLOR);
			qsoFreqDisp1->selection_color(FL_BACKGROUND_COLOR);
			qsoFreqDisp1->labeltype(FL_NORMAL_LABEL);
			qsoFreqDisp1->font(progdefaults.FreqControlFontnbr);
			qsoFreqDisp1->labelsize(12);
			qsoFreqDisp1->labelcolor(FL_FOREGROUND_COLOR);
			qsoFreqDisp1->align(FL_ALIGN_CENTER);
			qsoFreqDisp1->when(FL_WHEN_RELEASE);
			qsoFreqDisp1->callback(qso_movFreq);
			qsoFreqDisp1->SetONOFFCOLOR(
				fl_rgb_color(	progdefaults.FDforeground.R,
								progdefaults.FDforeground.G,
								progdefaults.FDforeground.B),
				fl_rgb_color(	progdefaults.FDbackground.R,
								progdefaults.FDbackground.G,
								progdefaults.FDbackground.B));
			qsoFreqDisp1->value(0);
			qsoFreqDisp1->end();

			pwrmeter = new PWRmeter(
				qsoFreqDisp1->x(), qsoFreqDisp1->y() + qsoFreqDisp1->h() + pad,
				smeter_w, Hentry);
			pwrmeter->select(progdefaults.PWRselect);
			pwrmeter->tooltip(_("Click to set power level"));
			pwrmeter->callback( (Fl_Callback *) cb_meters);
			pwrmeter->hide();

			smeter = new Smeter(
				qsoFreqDisp1->x(), qsoFreqDisp1->y() + qsoFreqDisp1->h() + pad,
				smeter_w, Hentry);
			set_smeter_colors();
			smeter->tooltip(_("Click to set power level"));
			smeter->callback( (Fl_Callback *) cb_meters);
			smeter->hide();

			pwrlevel_grp = new Fl_Group(
					smeter->x(), smeter->y(),
					smeter->w(), smeter->h());

				pwr_level = new Fl_Value_Slider2(
						pwrlevel_grp->x(), pwrlevel_grp->y(),
						pwrlevel_grp->w() - 50, pwrlevel_grp->h());
				pwr_level->type(FL_HOR_NICE_SLIDER);
				pwr_level->range(0, 100.0);
				pwr_level->step(1);
				pwr_level->callback( (Fl_Callback *) cb_set_pwr_level );
				pwr_level->color( fl_rgb_color(
						progdefaults.bwsrSliderColor.R,
						progdefaults.bwsrSliderColor.G,
						progdefaults.bwsrSliderColor.B));
				pwr_level->selection_color( fl_rgb_color(
						progdefaults.bwsrSldrSelColor.R,
						progdefaults.bwsrSldrSelColor.G,
						progdefaults.bwsrSldrSelColor.B));
				pwr_level->tooltip(_("Adjust Power Level"));

				set_pwr_level = new Fl_Button(
						pwr_level->x() + pwr_level->w(), pwr_level->y(),
						50, pwr_level->h(),
						_("Done"));
				set_pwr_level->tooltip(_("Return to Smeter / Pmeter"));
				set_pwr_level->callback( (Fl_Callback *) cb_exit_pwr_level );

			pwrlevel_grp->end();
			pwrlevel_grp->hide();

			qso_combos = new Fl_Group(
				qsoFreqDisp1->x(), qsoFreqDisp1->y() + qsoFreqDisp1->h() + pad,
				smeter_w, Hentry);
			qso_combos->box(FL_FLAT_BOX);

				qso_opMODE = new Fl_ListBox(
					smeter->x(), smeter->y(), mode_cbo_w, Hentry);
				qso_opMODE->box(FL_DOWN_BOX);
				qso_opMODE->color(FL_BACKGROUND2_COLOR);
				qso_opMODE->selection_color(FL_BACKGROUND_COLOR);
				qso_opMODE->labeltype(FL_NORMAL_LABEL);
				qso_opMODE->labelfont(0);
				qso_opMODE->labelsize(FL_NORMAL_SIZE);
				qso_opMODE->labelcolor(FL_FOREGROUND_COLOR);
				qso_opMODE->callback((Fl_Callback*)cb_qso_opMODE);
				qso_opMODE->align(FL_ALIGN_TOP);
				qso_opMODE->when(FL_WHEN_RELEASE);
				qso_opMODE->readonly(true);
				qso_opMODE->end();

				qso_opBW = new Fl_ListBox(
							qso_opMODE->x() + mode_cbo_w + pad,
							smeter->y(),
							bw_cbo_w, Hentry);
				qso_opBW->box(FL_DOWN_BOX);
				qso_opBW->color(FL_BACKGROUND2_COLOR);
				qso_opBW->selection_color(FL_BACKGROUND_COLOR);
				qso_opBW->labeltype(FL_NORMAL_LABEL);
				qso_opBW->labelfont(0);
				qso_opBW->labelsize(FL_NORMAL_SIZE);
				qso_opBW->labelcolor(FL_FOREGROUND_COLOR);
				qso_opBW->callback((Fl_Callback*)cb_qso_opBW);
				qso_opBW->align(FL_ALIGN_TOP);
				qso_opBW->when(FL_WHEN_RELEASE);
				qso_opBW->readonly(true);
				qso_opBW->end();

				qso_opGROUP = new Fl_Group(
								qso_opMODE->x() + mode_cbo_w + pad,
								smeter->y(),
								bw_cbo_w, Hentry);
					qso_opGROUP->box(FL_FLAT_BOX);

					qso_btnBW1 = new Fl_Button(
								qso_opGROUP->x(), qso_opGROUP->y(),
								qso_opGROUP->h() * 3 / 4, qso_opGROUP->h());
					qso_btnBW1->callback((Fl_Callback*)cb_qso_btnBW1);

					qso_opBW1 = new Fl_ListBox(
								qso_btnBW1->x()+qso_btnBW1->w(), qso_btnBW1->y(),
								qso_opGROUP->w() - qso_btnBW1->w(), qso_btnBW1->h());
						qso_opBW1->box(FL_DOWN_BOX);
						qso_opBW1->color(FL_BACKGROUND2_COLOR);
						qso_opBW1->selection_color(FL_BACKGROUND_COLOR);
						qso_opBW1->labeltype(FL_NORMAL_LABEL);
						qso_opBW1->labelfont(0);
						qso_opBW1->labelsize(FL_NORMAL_SIZE);
						qso_opBW1->labelcolor(FL_FOREGROUND_COLOR);
						qso_opBW1->callback((Fl_Callback*)cb_qso_opBW1);
						qso_opBW1->align(FL_ALIGN_TOP);
						qso_opBW1->when(FL_WHEN_RELEASE);
					qso_opBW1->end();

					qso_btnBW1->hide();
					qso_opBW1->hide();

					qso_btnBW2 = new Fl_Button(
								qso_opGROUP->x(), qso_opGROUP->y(),
								qso_opGROUP->h() * 3 / 4, qso_opGROUP->h());
					qso_btnBW2->callback((Fl_Callback*)cb_qso_btnBW2);

					qso_opBW2 = new Fl_ListBox(
								qso_btnBW2->x()+qso_btnBW2->w(), qso_btnBW2->y(),
								qso_opGROUP->w() - qso_btnBW2->w(), qso_btnBW2->h());
						qso_opBW2->box(FL_DOWN_BOX);
						qso_opBW2->color(FL_BACKGROUND2_COLOR);
						qso_opBW2->selection_color(FL_BACKGROUND_COLOR);
						qso_opBW2->labeltype(FL_NORMAL_LABEL);
						qso_opBW2->labelfont(0);
						qso_opBW2->labelsize(FL_NORMAL_SIZE);
						qso_opBW2->labelcolor(FL_FOREGROUND_COLOR);
						qso_opBW2->callback((Fl_Callback*)cb_qso_opBW2);
						qso_opBW2->align(FL_ALIGN_TOP);
						qso_opBW2->when(FL_WHEN_RELEASE);
					qso_opBW2->end();

				qso_opGROUP->end();
				qso_opGROUP->hide();

			qso_combos->end();

			Fl_Button *smeter_toggle = new Fl_Button(
					qso_opBW->x() + qso_opBW->w() + pad, smeter->y(), Wbtn, Hentry);
			smeter_toggle->callback(cb_toggle_smeter, 0);
			smeter_toggle->tooltip(_("Toggle smeter / combo controls"));
			smeter_toggle->image(new Fl_Pixmap(tango_view_refresh));

			qso_opPICK = new Fl_Button(
					smeter_toggle->x() + Wbtn + pad, smeter->y(), Wbtn, Hentry);
			addrbookpixmap = new Fl_Pixmap(address_book_icon);
			qso_opPICK->image(addrbookpixmap);
			qso_opPICK->callback(showOpBrowserView, 0);
			qso_opPICK->tooltip(_("Open List"));

		RigControlFrame->resizable(NULL);
		RigControlFrame->end();
}
		Fl_Group *rightframes = new Fl_Group(
					rightof(RigControlFrame) + pad, RigControlFrame->y(),
					W - rightof(RigControlFrame) - pad, Hqsoframe);
			rightframes->box(FL_FLAT_BOX);
{ // rightframes
			RigViewerFrame = new Fl_Group(
					rightframes->x(), rightframes->y(),
					rightframes->w(), rightframes->h());
{ // RigViewerFrame

				qso_btnSelFreq = new Fl_Button(
					RigViewerFrame->x(), RigViewerFrame->y() + pad,
					Wbtn, Hentry);
				qso_btnSelFreq->image(new Fl_Pixmap(left_arrow_icon));
				qso_btnSelFreq->tooltip(_("Select"));
				qso_btnSelFreq->callback((Fl_Callback*)cb_qso_btnSelFreq);

				qso_btnAddFreq = new Fl_Button(
					rightof(qso_btnSelFreq) + pad, RigViewerFrame->y() + pad,
					Wbtn, Hentry);
				qso_btnAddFreq->image(new Fl_Pixmap(plus_icon));
				qso_btnAddFreq->tooltip(_("Add current frequency"));
				qso_btnAddFreq->callback((Fl_Callback*)cb_qso_btnAddFreq);

				qso_btnClearList = new Fl_Button(
					RigViewerFrame->x(), RigViewerFrame->y() + Hentry + 2 * pad,
					Wbtn, Hentry);
				qso_btnClearList->image(new Fl_Pixmap(trash_icon));
				qso_btnClearList->tooltip(_("Clear list"));
				qso_btnClearList->callback((Fl_Callback*)cb_qso_btnClearList);

				qso_btnDelFreq = new Fl_Button(
					rightof(qso_btnClearList) + pad, RigViewerFrame->y() + Hentry + 2 * pad,
					Wbtn, Hentry);
				qso_btnDelFreq->image(new Fl_Pixmap(minus_icon));
				qso_btnDelFreq->tooltip(_("Delete from list"));
				qso_btnDelFreq->callback((Fl_Callback*)cb_qso_btnDelFreq);

				qso_btnAct = new Fl_Button(
					RigViewerFrame->x(), RigViewerFrame->y() + 2*(Hentry + pad) + pad,
					Wbtn, Hentry);
				qso_btnAct->image(new Fl_Pixmap(chat_icon));
				qso_btnAct->callback(cb_qso_inpAct);
				qso_btnAct->tooltip("Show active frequencies");

				qso_inpAct = new Fl_Input2(
					rightof(qso_btnAct) + pad, RigViewerFrame->y() + 2*(Hentry + pad) + pad,
					Wbtn, Hentry);
				qso_inpAct->when(FL_WHEN_ENTER_KEY);
				qso_inpAct->callback(cb_qso_inpAct);
				qso_inpAct->tooltip("Grid prefix for activity list");

// fwidths set in rigsupport.cxx
				qso_opBrowser = new Fl_Browser(
					rightof(qso_btnDelFreq) + pad,  RigViewerFrame->y() + pad,
					rightframes->w() - 2*Wbtn - pad, Hqsoframe - 2 * pad );
				qso_opBrowser->column_widths(fwidths);
				qso_opBrowser->column_char('|');
				qso_opBrowser->tooltip(_("Select operating parameters"));
				qso_opBrowser->callback((Fl_Callback*)cb_qso_opBrowser);
				qso_opBrowser->type(FL_MULTI_BROWSER);
				qso_opBrowser->box(FL_DOWN_BOX);
				qso_opBrowser->labelfont(4);
				qso_opBrowser->labelsize(12);
#ifdef __APPLE__
				qso_opBrowser->textfont(FL_SCREEN_BOLD);
				qso_opBrowser->textsize(13);
#else
				qso_opBrowser->textfont(FL_HELVETICA);
				qso_opBrowser->textsize(13);
#endif
				opUsageFrame = new Fl_Group(
					qso_opBrowser->x(),
					qso_opBrowser->y(),
					qso_opBrowser->w(), Hentry);
					opUsageFrame->box(FL_DOWN_BOX);

					opOutUsage = new Fl_Output(
						opUsageFrame->x() + pad, opUsageFrame->y() + opUsageFrame->h() / 2 - Hentry / 2,
						opUsageFrame->w() * 4 / 10, Hentry);
						opOutUsage->color(FL_BACKGROUND_COLOR);

					opUsage = new Fl_Input2(
						opOutUsage->x() + opOutUsage->w() + pad,
						opOutUsage->y(),
						opUsageFrame->w() - opOutUsage->w() - 50 - 3 * pad,
						Hentry);

					opUsageEnter = new Fl_Button(
						opUsage->x() + opUsage->w() , opUsage->y(),
						50, Hentry, "Enter");
						opUsageEnter->callback((Fl_Callback*)cb_opUsageEnter);

				opUsageFrame->end();
				opUsageFrame->hide();

				RigViewerFrame->resizable(qso_opBrowser);

			RigViewerFrame->end();
			RigViewerFrame->hide();
}
			int y2 = TopFrame1->y() + Hentry + 2 * pad;
			int y3 = TopFrame1->y() + 2 * (Hentry + pad) + pad;

			x_qsoframe = RigViewerFrame->x();
			Logging_frame = new Fl_Group(
					rightframes->x(), rightframes->y(),
					rightframes->w(), rightframes->h());
{ // Logging frame
{ // buttons
				btnQRZ = new Fl_Button(
					x_qsoframe, qsoFreqDisp1->y(), Wbtn, Hentry);
				btnQRZ->image(new Fl_Pixmap(net_icon));
				btnQRZ->callback(cb_QRZ, 0);
				btnQRZ->tooltip(_("QRZ"));

				qsoClear = new Fl_Button(
					x_qsoframe, btnQRZ->y() + pad + Wbtn, Wbtn, Hentry);
				qsoClear->image(new Fl_Pixmap(edit_clear_icon));
				qsoClear->callback(qsoClear_cb, 0);
				qsoClear->tooltip(_("Clear"));

				qsoSave = new Fl_Button(
					x_qsoframe, qsoClear->y() + pad + Wbtn, Wbtn, Hentry);
				qsoSave->image(new Fl_Pixmap(save_icon));
				qsoSave->callback(qsoSave_cb, 0);
				qsoSave->tooltip(_("Save"));
}
				fl_font(progdefaults.LOGGINGtextfont, progdefaults.LOGGINGtextsize);
				wf1 = fl_width("xFreq") + 90 +
					  Hentry +
					  40 +
					  fl_width("xOff") + 40 +
					  fl_width("xIn") + 35 +
					  fl_width("xOut") + 35;

				Logging_frame_1 = new Fl_Group(
					rightof(btnQRZ) + pad,
					TopFrame1->y(), wf1, Hqsoframe);
{ // Logging frame 1
{ // Line 1
					inpFreq1 = new Fl_Input2(
						Logging_frame_1->x() + fl_width("xFreq"),
						TopFrame1->y() + pad, 90, Hentry, _("Freq"));
					inpFreq1->type(FL_NORMAL_OUTPUT);
					inpFreq1->tooltip(_("frequency kHz"));
					inpFreq1->align(FL_ALIGN_LEFT);

					btnTimeOn = new Fl_Button(
						rightof(inpFreq1), TopFrame1->y() + pad,
						Hentry, Hentry, _("On"));
					btnTimeOn->tooltip(_("Press to update QSO start time"));
					btnTimeOn->callback(cb_btnTimeOn);

					inpTimeOn1 = new Fl_Input2(
						rightof(btnTimeOn), TopFrame1->y() + pad,
						40, Hentry, "");
					inpTimeOn1->tooltip(_("QSO start time"));
					inpTimeOn1->align(FL_ALIGN_LEFT);
					inpTimeOn1->type(FL_INT_INPUT);

					inpTimeOff1 = new Fl_Input2(
						rightof(inpTimeOn1) + fl_width("xOff"), TopFrame1->y() + pad,
						40, Hentry, _("Off"));
					inpTimeOff1->tooltip(_("QSO end time"));

					inpRstIn1 = new Fl_Input2(
						rightof(inpTimeOff1) + fl_width("xIn"), TopFrame1->y() + pad,
						35, Hentry, _("In"));
					inpRstIn1->tooltip("RST in");
					inpRstIn1->align(FL_ALIGN_LEFT);

					inpRstOut1 = new Fl_Input2(
						rightof(inpRstIn1) + fl_width("xOut"), TopFrame1->y() + pad,
						35, Hentry, _("Out"));
					inpRstOut1->tooltip("RST out");
					inpRstOut1->align(FL_ALIGN_LEFT);

					inpCall1 = new Fl_Input2(
						inpFreq1->x(), y2,
						rightof(inpTimeOn1) - inpFreq1->x(),
						Hentry, _("Call"));
					inpCall1->tooltip(_("call sign"));
					inpCall1->align(FL_ALIGN_LEFT);

					inpName1 = new Fl_Input2(
						inpTimeOff1->x(), y2,
						rightof(inpRstIn1) - inpTimeOff1->x(),Hentry, _("Op"));
					inpName1->tooltip(_("Operator name"));
					inpName1->align(FL_ALIGN_LEFT);

					inpAZ = new Fl_Input2(
						inpRstOut1->x(), y2, 35, Hentry, "Az");
					inpAZ->tooltip(_("Azimuth"));
					inpAZ->align(FL_ALIGN_LEFT);

}
					gGEN_QSO_1 = new Fl_Group (x_qsoframe, y3, wf1, Hentry + pad);
{ // QSO frame 1
						inpQth = new Fl_Input2(
							inpCall1->x(), y3, inpCall1->w(), Hentry, "Qth");
						inpQth->tooltip(_("QTH City"));
						inpQth->align(FL_ALIGN_LEFT);
						inpQTH = inpQth;

						inpState1 = new Fl_Input2(
							rightof(inpQth) + 20, y3, 30, Hentry, "St");
						inpState1->tooltip(_("US State"));
						inpState1->align(FL_ALIGN_LEFT);
						inpState = inpState1;

						inpVEprov = new Fl_Input2(
							rightof(inpState1) + 20, y3, 30, Hentry, "Pr");
						inpVEprov->tooltip(_("Can. Province"));
						inpVEprov->align(FL_ALIGN_LEFT);

						inpLoc1 = new Fl_Input2(
							rightof(inpVEprov) + 15, y3,
							rightof(inpAZ) - (rightof(inpVEprov) + 15), Hentry, "L");
						inpLoc1->tooltip(_("Maidenhead Locator"));
						inpLoc1->align(FL_ALIGN_LEFT);

					gGEN_QSO_1->end();
}
					gGEN_CONTEST = new Fl_Group (
						Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // Contest - LOG_GENERIC
						outSerNo1 = new Fl_Input2(
							inpFreq1->x(), y3,
							40, Hentry,
							"S#");
						outSerNo1->align(FL_ALIGN_LEFT);
						outSerNo1->tooltip(_("Sent serial number (read only)"));
						outSerNo1->type(FL_NORMAL_OUTPUT);

						inpSerNo1 = new Fl_Input2(
							rightof(outSerNo1) + fl_width("xR#"), y3,
							40, Hentry,
							"R#");
						inpSerNo1->align(FL_ALIGN_LEFT);
						inpSerNo1->tooltip(_("Received serial number"));

						xtmp = rightof(inpSerNo1) + fl_width("xXch");
						inpXchgIn1 = new Fl_Input2(
							xtmp, y3,
							Logging_frame_1->x() + Logging_frame_1->w() - xtmp, Hentry,
							"Xch");
						inpXchgIn1->align(FL_ALIGN_LEFT);
						inpXchgIn1->tooltip(_("Contest exchange in"));

					gGEN_CONTEST->end();
					gGEN_CONTEST->hide();
}
					gFD = new Fl_Group (
						Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // Field Day - LOG_FD
						inp_FD_class1 = new Fl_Input2(
							Logging_frame_1->x() + fl_width("xClass"), y3, 40, Hentry,
							"Class");
						inp_FD_class1->align(FL_ALIGN_LEFT);
						inp_FD_class1->tooltip(_("Received FD class"));
						inp_FD_class1->type(FL_NORMAL_INPUT);

						inp_FD_section1 = new Fl_Input2(
							rightof(inp_FD_class1) + fl_width("xSection"), y3, 40, Hentry,
							"Section");
						inp_FD_section1->align(FL_ALIGN_LEFT);
						inp_FD_section1->tooltip(_("Received FD section"));
						inp_FD_section1->type(FL_NORMAL_INPUT);
					gFD->end();
					gFD->hide();
}
					gKD_1 = new Fl_Group (
						Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // ARRL Kids Day - LOG_KD
						inp_KD_age1 = new Fl_Input2(
							inpCall1->x(), y3, 40, Hentry,
							"Age");
						inp_KD_age1->align(FL_ALIGN_LEFT);
						inp_KD_age1->tooltip(_("Guest operators age"));
						inp_KD_age1->type(FL_NORMAL_INPUT);

						inp_KD_state1 = new Fl_Input2(
							rightof(inp_KD_age1) + fl_width("xSt"), y3, 40, Hentry,
							"St");
						inp_KD_state1->align(FL_ALIGN_LEFT);
						inp_KD_state1->tooltip(_("Station state"));
						inp_KD_state1->type(FL_NORMAL_INPUT);

						inp_KD_VEprov1 = new Fl_Input2(
							rightof(inp_KD_state1) + fl_width("xPr"), y3, 40, Hentry,
							"Pr");
						inp_KD_VEprov1->align(FL_ALIGN_LEFT);
						inp_KD_VEprov1->tooltip(_("Station province"));
						inp_KD_VEprov1->type(FL_NORMAL_INPUT);

						inp_KD_XchgIn1 = new Fl_Input2(
							rightof(inp_KD_VEprov1) + fl_width("xXchg"), y3,
							gKD_1->x() + gKD_1->w() - (rightof(inp_KD_VEprov1) + fl_width("xXchg")), Hentry,
							"Xch");
						inp_KD_XchgIn1->align(FL_ALIGN_LEFT);
						inp_KD_XchgIn1->tooltip(_("Additional Exchange"));
						inp_KD_XchgIn1->type(FL_NORMAL_INPUT);
					gKD_1->end();
					gKD_1->hide();
}
					gARR = new Fl_Group (
						Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // LOG_ARR rookie roundup
						inp_ARR_check1 = new Fl_Input2(
							inpCall1->x(), y3, 40, Hentry,
							"Chk");
						inp_ARR_check1->align(FL_ALIGN_LEFT);
						inp_ARR_check1->tooltip(_("Check / birth-year"));
						inp_ARR_check1->type(FL_NORMAL_INPUT);

						inp_ARR_XchgIn1 = new Fl_Input2(
							rightof(inp_ARR_check1) + fl_width("xXchg"), y3,
							gARR->x() + gARR->w() - (rightof(inp_ARR_check1) + fl_width("xXchg")), Hentry,
							"Xchg");
						inp_ARR_XchgIn1->align(FL_ALIGN_LEFT);
						inp_ARR_XchgIn1->tooltip(_("Round Up Exchange - State, Province, Country"));
						inp_ARR_XchgIn1->type(FL_NORMAL_INPUT);
					gARR->end();
					gARR->hide();
}
					g1010 = new Fl_Group (
						Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // LOG_1010
						inp_1010_nr1 = new Fl_Input2(
						g1010->x() + fl_width("x1010#"), y3, 60, Hentry,
							"1010#");
						inp_1010_nr1->align(FL_ALIGN_LEFT);
						inp_1010_nr1->tooltip(_("1010 number"));
						inp_1010_nr1->type(FL_NORMAL_INPUT);

						inp_1010_XchgIn1 = new Fl_Input2(
							rightof(inp_1010_nr1) + fl_width("xXchg"), y3,
							g1010->x() + g1010->w() - (rightof(inp_1010_nr1) + fl_width("xXchg")), Hentry,
							"Xchg");
						inp_1010_XchgIn1->align(FL_ALIGN_LEFT);
						inp_1010_XchgIn1->tooltip(_("1010 exchange"));
						inp_1010_XchgIn1->type(FL_NORMAL_INPUT);

					g1010->end();
					g1010->hide();
}
					gVHF = new Fl_Group (
						Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // LOG_VHF
						inp_vhf_RSTin1 = new Fl_Input2(
							gVHF->x() + fl_width("xRSTin"), y3, 60, Hentry,
							"RSTin");
						inp_vhf_RSTin1->align(FL_ALIGN_LEFT);
						inp_vhf_RSTin1->tooltip(_("Received RST"));
						inp_vhf_RSTin1->type(FL_NORMAL_INPUT);

						inp_vhf_RSTout1 = new Fl_Input2(
							rightof(inp_vhf_RSTin1) + fl_width("xout"), y3, 60, Hentry,
							"out");
						inp_vhf_RSTout1->align(FL_ALIGN_LEFT);
						inp_vhf_RSTout1->tooltip(_("Sent RST"));
						inp_vhf_RSTout1->type(FL_NORMAL_INPUT);

						inp_vhf_Loc1 = new Fl_Input2(
							rightof(inp_vhf_RSTout1) + fl_width("xGrid"), y3, 80, Hentry,
							"Grid");
						inp_vhf_Loc1->align(FL_ALIGN_LEFT);
						inp_vhf_Loc1->tooltip(_("Grid Locator"));
						inp_vhf_Loc1->type(FL_NORMAL_INPUT);

					gVHF->end();
					gVHF->hide();
}
					gCQWW_RTTY = new Fl_Group (
						Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // CQWW RTTY - LOG_CQWW_RTTY

						inp_CQzone1 = new Fl_Input2(
							gCQWW_RTTY->x() + fl_width("xCQz"), y3, 40, Hentry,
							"CQz");
						inp_CQzone1->align(FL_ALIGN_LEFT);
						inp_CQzone1->tooltip(_("Received CQ zone"));
						inp_CQzone1->type(FL_NORMAL_INPUT);

						inp_CQstate1 = new Fl_Input2(
							rightof(inp_CQzone1) + fl_width("xCQs"), y3, 40, Hentry,
							"CQs");
						inp_CQstate1->align(FL_ALIGN_LEFT);
						inp_CQstate1->tooltip(_("Received State/Prov"));
						inp_CQstate1->type(FL_NORMAL_INPUT);

					gCQWW_RTTY->end();
					gCQWW_RTTY->hide();
}
					gCQWW_DX = new Fl_Group (
						Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // CQWW DX -- LOG_CQWWDX0
						inp_CQDXzone1 = new Fl_Input2(
							gCQWW_DX->x() + fl_width("xCQz"), y3, 40, Hentry,
							"CQz");
						inp_CQDXzone1->align(FL_ALIGN_LEFT);
						inp_CQDXzone1->tooltip(_("Received CQ zone"));
						inp_CQDXzone1->type(FL_NORMAL_INPUT);

					gCQWW_DX->end();
					gCQWW_DX->hide();
}
					gCQWPX = new Fl_Group (
						Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // LOG_CQWPX
						outSerNo_WPX1 = new Fl_Input2(
							inpCall1->x(), y3, 40, Hentry,
							"S #");
						outSerNo_WPX1->align(FL_ALIGN_LEFT);
						outSerNo_WPX1->tooltip(_("Sent serno"));
						outSerNo_WPX1->type(FL_NORMAL_OUTPUT);

						inpSerNo_WPX1 = new Fl_Input2(
							rightof(outSerNo_WPX1) + fl_width("xR#"), y3, 40, Hentry,
							"R#");
						inpSerNo_WPX1->align(FL_ALIGN_LEFT);
						inpSerNo_WPX1->tooltip(_("Received serno"));
						inpSerNo_WPX1->type(FL_NORMAL_INPUT);

					gCQWPX->end();
					gCQWPX->hide();
}
					gCWSS = new Fl_Group (
						Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // CW Sweepstakes - LOG_CWSS
						outSerNo3 = new Fl_Input2(
							inpCall1->x(), y3, 40, Hentry,
							"S#");
						outSerNo3->align(FL_ALIGN_LEFT);
						outSerNo3->tooltip(_("Sent serno"));
						outSerNo3->type(FL_NORMAL_OUTPUT);

						inp_SS_SerialNoR1 = new Fl_Input2(
							rightof(outSerNo3) + fl_width("xR#"), y3, 40, Hentry,
							"R#");
						inp_SS_SerialNoR1->align(FL_ALIGN_LEFT);
						inp_SS_SerialNoR1->tooltip(_("Received serno"));
						inp_SS_SerialNoR1->type(FL_NORMAL_INPUT);

						inp_SS_Precedence1 = new Fl_Input2(
							rightof(inp_SS_SerialNoR1) + fl_width("xPre"), y3, 40, Hentry,
							"Pre");
						inp_SS_Precedence1->align(FL_ALIGN_LEFT);
						inp_SS_Precedence1->tooltip(_("SS Precedence"));
						inp_SS_Precedence1->type(FL_NORMAL_INPUT);

						inp_SS_Check1 = new Fl_Input2(
							rightof(inp_SS_Precedence1) + fl_width("xChk"), y3, 40, Hentry,
							"Chk");
						inp_SS_Check1->align(FL_ALIGN_LEFT);
						inp_SS_Check1->tooltip(_("SS Check"));
						inp_SS_Check1->type(FL_NORMAL_INPUT);

						inp_SS_Section1 = new Fl_Input2(
							rightof(inp_SS_Check1) + fl_width("xSec"), y3, 40, Hentry,
							"Sec");
						inp_SS_Section1->align(FL_ALIGN_LEFT);
						inp_SS_Section1->tooltip(_("SS section"));
						inp_SS_Section1->type(FL_NORMAL_INPUT);

					gCWSS->end();
					gCWSS->hide();
}
					gASCR = new Fl_Group (
						Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // School Roundup - LOG_ASCR
						inp_ASCR_class1 = new Fl_Input2(
							Logging_frame_1->x() + fl_width("xClass"), y3, 30, Hentry,
							"Class");
						inp_ASCR_class1->align(FL_ALIGN_LEFT);
						inp_ASCR_class1->tooltip(_("ASCR class, I/C/S"));
						inp_ASCR_class1->type(FL_NORMAL_INPUT);
						inp_ASCR_class1->hide();

						xtmp = rightof(inp_ASCR_class1) + fl_width("xSPC");
						inp_ASCR_XchgIn1 = new Fl_Input2(
							xtmp, y3,
							Logging_frame_1->x() + Logging_frame_1->w() - xtmp - pad, Hentry,
							"SPC");
						inp_ASCR_XchgIn1->align(FL_ALIGN_LEFT);
						inp_ASCR_XchgIn1->tooltip(_("State/Province/Country received"));
						inp_ASCR_XchgIn1->type(FL_NORMAL_INPUT);
						inp_ASCR_XchgIn1->hide();
					gASCR->end();
					gASCR->hide();
}
					gIARI = new Fl_Group (
						Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // IARI  - Italian International DX LOG_IARI
						inp_IARI_PR1 = new Fl_Input2(
							inpCall1->x(), y3, 40, Hentry,
							"Pr");
						inp_IARI_PR1->align(FL_ALIGN_LEFT);
						inp_IARI_PR1->tooltip(_("Received Province / Ser #"));
						inp_IARI_PR1->type(FL_NORMAL_INPUT);

						out_IARI_SerNo1 = new Fl_Input2(
							rightof(inp_IARI_PR1) + fl_width("xS#"), y3, 40, Hentry,
							"S#");
						out_IARI_SerNo1->align(FL_ALIGN_LEFT);
						out_IARI_SerNo1->tooltip(_("Sent serno"));
						out_IARI_SerNo1->type(FL_NORMAL_OUTPUT);

						inp_IARI_SerNo1 = new Fl_Input2(
							rightof(out_IARI_SerNo1) + fl_width("xR#"), y3, 40, Hentry,
							"R#");
						inp_IARI_SerNo1->align(FL_ALIGN_LEFT);
						inp_IARI_SerNo1->tooltip(_("Received serno"));
						inp_IARI_SerNo1->type(FL_NORMAL_INPUT);

					gIARI->end();
					gIARI->hide();
}
				gNAQP = new Fl_Group(
					Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // North American Qso Party - LOG_NAQP
					inpSPCnum_NAQP1 = new Fl_Input2(
						Logging_frame_1->x() + fl_width("xNAQP xchg"), y3, 100, Hentry,
						"NAQP xchg");
					inpSPCnum_NAQP1->align(FL_ALIGN_LEFT);
					inpSPCnum_NAQP1->tooltip(_("Received State/Province/Country"));
					inpSPCnum_NAQP1->type(FL_NORMAL_INPUT);
					inpSPCnum_NAQP1->hide();
				gNAQP->end();
				gNAQP->hide();
}
				gARRL_RTTY = new Fl_Group(
					Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // LOG_RTTY ARRL RTTY Roundup
						inpRTU_stpr1 = new Fl_Input2(
							inpCall1->x(), y3, fl_width("xWWW"), Hentry,
							"S/P");
						inpRTU_stpr1->align(FL_ALIGN_LEFT);
						inpRTU_stpr1->tooltip(_("State/Province/#"));
						inpRTU_stpr1->type(FL_NORMAL_INPUT);

						xtmp = rightof(inpRTU_stpr1) + fl_width("xSer");
						inpRTU_serno1 = new Fl_Input2(
							xtmp, y3, fl_width("x9999"), Hentry, "Ser");
						inpRTU_serno1->align(FL_ALIGN_LEFT);
						inpRTU_serno1->tooltip(_("Serial number received"));
						inpRTU_serno1->type(FL_NORMAL_INPUT);

				gARRL_RTTY->end();
				gARRL_RTTY->hide();
}
				gNAS = new Fl_Group (
					Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // NA Sprint - LOG_NAS
					outSerNo5 = new Fl_Input2(
						Logging_frame_1->x() + fl_width("xS#"), y3, 40, Hentry,
						"S#");
					outSerNo5->align(FL_ALIGN_LEFT);
					outSerNo5->tooltip(_("Sent serial number"));
					outSerNo5->type(FL_NORMAL_OUTPUT);
					outSerNo5->hide();

					xtmp = rightof(outSerNo5) + fl_width("xR#");
					inp_ser_NAS1 = new Fl_Input2(
						xtmp, y3, 40, Hentry,
						"R #");
					inp_ser_NAS1->align(FL_ALIGN_LEFT);
					inp_ser_NAS1->tooltip(_("Received serial number"));
					inp_ser_NAS1->type(FL_NORMAL_INPUT);
					inp_ser_NAS1->hide();

					xtmp = rightof(inp_ser_NAS1) + fl_width("xS/P/C");
					inpSPCnum_NAS1 = new Fl_Input2(
						xtmp, y3,
						Logging_frame_1->x() + Logging_frame_1->w() - xtmp - pad, Hentry,
						"S/P/C");
					inpSPCnum_NAS1->align(FL_ALIGN_LEFT);
					inpSPCnum_NAS1->tooltip(_("State/Province/Country received"));
					inpSPCnum_NAS1->type(FL_NORMAL_INPUT);
					inpSPCnum_NAS1->hide();

				gASCR->end();
				gASCR->hide();
}
				gAIDX = new Fl_Group (
					Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // LOG_AAM
					outSerNo7 = new Fl_Input2(
						Logging_frame_1->x() + fl_width("xS#"), y3, 40, Hentry,
						"S#");
					outSerNo7->align(FL_ALIGN_LEFT);
					outSerNo7->tooltip(_("Sent serial number"));
					outSerNo7->type(FL_NORMAL_OUTPUT);
					outSerNo7->hide();

					xtmp = rightof(outSerNo7) + fl_width("xR#");
					inpSerNo3 = new Fl_Input2(
						xtmp, y3, 40, Hentry,
						"R#");
					inpSerNo3->align(FL_ALIGN_LEFT);
					inpSerNo3->tooltip(_("Received serial number"));
					inpSerNo3->type(FL_NORMAL_INPUT);
					inpSerNo3->hide();

				gAIDX->end();
				gAIDX->hide();
}
				gJOTA = new Fl_Group (
					Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // LOG_JOTA - Jamboree On The Air
					xtmp = Logging_frame_1->x() + fl_width("xTroop");
					inp_JOTA_troop1 = new Fl_Input2(
						xtmp, y3, 60, Hentry,
						"Troop");
					inp_JOTA_troop1->align(FL_ALIGN_LEFT);
					inp_JOTA_troop1->tooltip(_("Troop received"));
					inp_JOTA_troop1->type(FL_NORMAL_INPUT);
					inp_JOTA_troop1->hide();

					xtmp = rightof(inp_JOTA_troop1) + fl_width("xScout");
					inp_JOTA_scout1 = new Fl_Input2(
						xtmp, y3, 80, Hentry,
						"Scout");
					inp_JOTA_scout1->align(FL_ALIGN_LEFT);
					inp_JOTA_scout1->tooltip(_("Scout name received"));
					inp_JOTA_scout1->type(FL_NORMAL_INPUT);
					inp_JOTA_scout1->hide();

					xtmp = rightof(inp_JOTA_scout1) + fl_width("xS/P/C");
					inp_JOTA_spc1 = new Fl_Input2(
						xtmp, y3,
						Logging_frame_1->x() + Logging_frame_1->w() - xtmp - pad, Hentry,
						"S/P/C");
					inp_JOTA_spc1->align(FL_ALIGN_LEFT);
					inp_JOTA_spc1->tooltip(_("State/Province/Country received"));
					inp_JOTA_spc1->type(FL_NORMAL_INPUT);
					inp_JOTA_spc1->hide();

				gJOTA->end();
				gJOTA->hide();
}
				gAICW = new Fl_Group (
					Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // LOG_AICW - ARRL International DX - CW
					xtmp = Logging_frame_1->x() + fl_width("xPwr-R");
					inpSPCnum_AICW1 = new Fl_Input2(
						xtmp, y3, 60, Hentry,
						"Pwr-R");
					inpSPCnum_AICW1->align(FL_ALIGN_LEFT);
					inpSPCnum_AICW1->tooltip(_("Power received"));
					inpSPCnum_AICW1->type(FL_NORMAL_INPUT);
					inpSPCnum_AICW1->hide();

				gAICW->end();
				gAICW->hide();
}
				gSQSO = new Fl_Group (
					Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // LOG_SQSO - all state QSO party controls

					xtmp = inpCall1->x();
					inpSQSO_state1 = new Fl_Input2(
						xtmp, y3, fl_width("xWW"), Hentry,
						"St");
					inpSQSO_state1->align(FL_ALIGN_LEFT);
					inpSQSO_state1->tooltip(_("State received"));
					inpSQSO_state1->type(FL_NORMAL_INPUT);
					inpSQSO_state1->hide();

					xtmp = rightof(inpSQSO_state1) + fl_width("xCnty");
					inpSQSO_county1 = new Fl_Input2(
						xtmp, y3, fl_width("WWWWW"), Hentry,
						"Cnty");
					inpSQSO_county1->align(FL_ALIGN_LEFT);
					inpSQSO_county1->tooltip(_("County received"));
					inpSQSO_county1->type(FL_NORMAL_INPUT);
					inpSQSO_county1->hide();

					inpCounty = inpSQSO_county1;

					xtmp = rightof(inpSQSO_county1) + fl_width("xS#");
					outSQSO_serno1 = new Fl_Input2(
						xtmp, y3, fl_width("9999"), Hentry,
						"S#");
					outSQSO_serno1->align(FL_ALIGN_LEFT);
					outSQSO_serno1->tooltip(_("Sent serial number"));
					outSQSO_serno1->type(FL_NORMAL_INPUT);
					outSQSO_serno1->hide();

					xtmp = rightof(outSQSO_serno1) + fl_width("xR#");
					inpSQSO_serno1 = new Fl_Input2(
						xtmp, y3, fl_width("9999"), Hentry,
						"R#");
					inpSQSO_serno1->align(FL_ALIGN_LEFT);
					inpSQSO_serno1->tooltip(_("Received serial number"));
					inpSQSO_serno1->type(FL_NORMAL_INPUT);
					inpSQSO_serno1->hide();

					xtmp = rightof(inpSQSO_serno1) + fl_width("x Cat");
					inpSQSO_category1 = new Fl_Input2(
						xtmp, y3, gSQSO->x() + gSQSO->w() - xtmp - pad, Hentry,
						"Cat");
					inpSQSO_category1->tooltip(_("Category: CLB, MOB, QRP, STD"));
					inpSQSO_category1->type(FL_NORMAL_INPUT);
					inpSQSO_category1->hide();

					inpSQSO_category = inpSQSO_category1;

				gSQSO->end();
				gSQSO->hide();
}
					gWAE = new Fl_Group (
						Logging_frame_1->x(), y3, wf1, Hentry + pad);
{ // LOG_WAE
						outSerNo_WAE1 = new Fl_Input2(
							inpCall1->x(), y3, 40, Hentry,
							"S #");
						outSerNo_WAE1->align(FL_ALIGN_LEFT);
						outSerNo_WAE1->tooltip(_("Sent serno"));
						outSerNo_WAE1->type(FL_NORMAL_OUTPUT);

						inpSerNo_WAE1 = new Fl_Input2(
							rightof(outSerNo_WAE1) + fl_width("xR#"), y3, 40, Hentry,
							"R#");
						inpSerNo_WAE1->align(FL_ALIGN_LEFT);
						inpSerNo_WAE1->tooltip(_("Received serno"));
						inpSerNo_WAE1->type(FL_NORMAL_INPUT);

//						xtmp = rightof(inpSerNo_WAE1) + fl_width("xCntry");
//						cboCountryWAE1 = new Fl_ComboBox(
//							xtmp, y3,
//							Logging_frame_1->x() + Logging_frame_1->w() - xtmp - pad, Hentry,
//							"Cntry");
//						cboCountryWAE1->align(FL_ALIGN_LEFT);
//						cboCountryWAE1->tooltip(_("Country"));
//						cboCountryWAE1->end();

					gWAE->end();
					gWAE->hide();
}

					Logging_frame_1->resizable(NULL);
				Logging_frame_1->end();
}
{  // NFtabs groups // Logging frame 2
				int nfx = rightof(Logging_frame_1) + pad;
				int nfy = Logging_frame_1->y();
				int nfw = W - nfx - pad;
				int nfh = Logging_frame_1->h();

				NFtabs = new Fl_Tabs(nfx, nfy, nfw, nfh, "");

				int cax = nfx + pad;
				int caw = nfw - 2*pad;
				int cay = nfy + Hentry;
				int cah = nfh - Hentry;

					Ccframe = new Fl_Group(cax, cay, caw, cah, "Cnty/Cntry");

						cboCountyQSO = new Fl_ComboBox(
							cax + pad, inpCall1->y(), caw - 2*pad, Hentry, "");
						cboCountyQSO->tooltip(_("County"));
						cboCountyQSO->callback(cb_CountyQSO);
						cboCountyQSO->readonly();
						cboCountyQSO->end();

						cboCountryQSO = new Fl_ComboBox(
							cax + pad, inpQth->y(), caw - 2*pad, Hentry, "");
						cboCountryQSO->tooltip(_("Country"));
						cboCountryQSO->readonly();
						cboCountryQSO->end();

					Ccframe->end();

					NotesFrame = new Fl_Group(cax, cay, caw, cah,"Notes");

						inpNotes = new Fl_Input2(
							cax + pad, cay + pad, caw-2*pad, cah-2*pad, "");
						inpNotes->type(FL_MULTILINE_INPUT);
						inpNotes->tooltip(_("Notes"));

					NotesFrame->end();

				NFtabs->end();
} 
// NFtabs end

					ifkp_avatar = new picture(
						W - 59 - pad, NFtabs->y(), 59, 74);
					ifkp_avatar->box(FL_FLAT_BOX);
					ifkp_avatar->noslant();
					ifkp_avatar->callback(cb_ifkp_send_avatar);
					ifkp_avatar->tooltip(_("Left click - save avatar\nRight click - send my avatar"));
					ifkp_load_avatar();
					ifkp_avatar->hide();

					thor_avatar = new picture(
						W - 59 - pad, NFtabs->y(), 59, 74);
					thor_avatar->box(FL_FLAT_BOX);
					thor_avatar->noslant();
					thor_avatar->callback(cb_thor_send_avatar);
					thor_avatar->tooltip(_("Left click - save avatar\nRight click - send my avatar"));
					thor_load_avatar();
					thor_avatar->hide();

				Logging_frame->end();
Logging_frame->resizable(NFtabs);
//				Logging_frame->resizable(Logging_frame_2);
}
			rightframes->end();
}
			TopFrame1->resizable(rightframes);
		TopFrame1->end();
}
		TopFrame2 = new Fl_Group(0, TopFrame1->y(), W, Hentry + 2 * pad);
{ // TopFrame2
			int y = TopFrame1->y() + pad;
			int h = Hentry;
			qsoFreqDisp2 = new cFreqControl(
				pad, y,
				freqwidth, freqheight, "10");
			qsoFreqDisp2->box(FL_DOWN_BOX);
			qsoFreqDisp2->color(FL_BACKGROUND_COLOR);
			qsoFreqDisp2->selection_color(FL_BACKGROUND_COLOR);
			qsoFreqDisp2->labeltype(FL_NORMAL_LABEL);
			qsoFreqDisp2->align(FL_ALIGN_CENTER);
			qsoFreqDisp2->when(FL_WHEN_RELEASE);
			qsoFreqDisp2->callback(qso_movFreq);
			qsoFreqDisp2->font(progdefaults.FreqControlFontnbr);
			qsoFreqDisp2->SetONOFFCOLOR(
				fl_rgb_color(	progdefaults.FDforeground.R,
								progdefaults.FDforeground.G,
								progdefaults.FDforeground.B),
				fl_rgb_color(	progdefaults.FDbackground.R,
								progdefaults.FDbackground.G,
								progdefaults.FDbackground.B));
			qsoFreqDisp2->value(0);

			qso_opPICK2 = new Fl_Button(
				rightof(qsoFreqDisp2), y,
				Wbtn, Hentry);
			qso_opPICK2->align(FL_ALIGN_INSIDE);
			qso_opPICK2->image(addrbookpixmap);
			qso_opPICK2->callback(showOpBrowserView2, 0);
			qso_opPICK2->tooltip(_("Open List"));

			btnQRZ2 = new Fl_Button(
					pad + rightof(qso_opPICK2), y,
					Wbtn, Hentry);
			btnQRZ2->align(FL_ALIGN_INSIDE);
			btnQRZ2->image(new Fl_Pixmap(net_icon));
			btnQRZ2->callback(cb_QRZ, 0);
			btnQRZ2->tooltip(_("QRZ"));

			qsoClear2 = new Fl_Button(
					pad + rightof(btnQRZ2), y,
					Wbtn, Hentry);
			qsoClear2->align(FL_ALIGN_INSIDE);
			qsoClear2->image(new Fl_Pixmap(edit_clear_icon));
			qsoClear2->callback(qsoClear_cb, 0);
			qsoClear2->tooltip(_("Clear"));

			qsoSave2 = new Fl_Button(
					pad + rightof(qsoClear2), y,
					Wbtn, Hentry);
			qsoSave2->align(FL_ALIGN_INSIDE);
			qsoSave2->image(new Fl_Pixmap(save_icon));
			qsoSave2->callback(qsoSave_cb, 0);
			qsoSave2->tooltip(_("Save"));

			const char *label2 = _("On");
			btnTimeOn2 = new Fl_Button(
				pad + rightof(qsoSave2), y,
				static_cast<int>(fl_width(label2)), h, label2);
			btnTimeOn2->tooltip(_("Press to update"));
			btnTimeOn2->callback(cb_btnTimeOn);
			inpTimeOn2 = new Fl_Input2(
				pad + btnTimeOn2->x() + btnTimeOn2->w(), y,
				w_inpTime2, h, "");
			inpTimeOn2->tooltip(_("Time On"));
			inpTimeOn2->type(FL_INT_INPUT);

			const char *label3 = _("Off");
			Fl_Box *bx3 = new Fl_Box(pad + rightof(inpTimeOn2), y,
				static_cast<int>(fl_width(label3)), h, label3);
			inpTimeOff2 = new Fl_Input2(
				pad + bx3->x() + bx3->w(), y,
				w_inpTime2, h, "");
			inpTimeOff2->tooltip(_("Time Off"));
			inpTimeOff2->type(FL_NORMAL_OUTPUT);

			const char *label4 = _("Call");
			Fl_Box *bx4 = new Fl_Box(pad + rightof(inpTimeOff2), y,
				static_cast<int>(fl_width(label4)), h, label4);
			inpCall2 = new Fl_Input2(
				pad + bx4->x() + bx4->w(), y,
				w_inpCall2, h, "");
			inpCall2->tooltip(_("Other call"));

			const char *label6 = _("In");
			Fl_Box *bx6 = new Fl_Box(pad + rightof(inpCall2), y,
				static_cast<int>(fl_width(label6)), h, label6);
			inpRstIn2 = new Fl_Input2(
				pad + bx6->x() + bx6->w(), y,
				w_inpRstIn2, h, "");
			inpRstIn2->tooltip(_("Received RST"));

			const char *label7 = _("Out");
			Fl_Box *bx7 = new Fl_Box(pad + rightof(inpRstIn2), y,
				static_cast<int>(fl_width(label7)), h, label7);
			inpRstOut2 = new Fl_Input2(
				pad + bx7->x() + bx7->w(), y,
				w_inpRstOut2, h, "");
			inpRstOut2->tooltip(_("Sent RST"));

			const char *label5 = _("Nm");
			Fl_Box *bx5 = new Fl_Box(pad + rightof(inpRstOut2), y,
				static_cast<int>(fl_width(label5)), h, label5);
			int xn = pad + bx5->x() + bx5->w();
			inpName2 = new Fl_Input2(
				xn, y,
				W - xn - pad, h, "");
			inpName2->tooltip(_("Other name"));

		TopFrame2->resizable(inpName2);
		TopFrame2->end();
		TopFrame2->hide();
}
		TopFrame3 = new Fl_Group(0, TopFrame1->y(), W, Hentry + 2 * pad);
{ // TopFrame3

			int y = TopFrame3->y() + pad;
			int h = Hentry;

			fl_font(progdefaults.LOGGINGtextfont, progdefaults.LOGGINGtextsize);
			const char *xData = "x8888";
			const char *xCall = "xWW8WWW";
			const char *xRST = "x599";
			int   wData = static_cast<int>(fl_width(xData));
			int   wCall = static_cast<int>(fl_width(xCall));
			int   wRST  = static_cast<int>(fl_width(xRST));

			int w3a = pad + freqwidth +
					  3*(pad + Wbtn) +
					  fl_width("xCall") + wCall;

// Top Frame 3a
// freqdisp, oppick, qsoclear, qsosave, call

			TopFrame3a = new Fl_Group(
				0, TopFrame1->y(),
				w3a, Hentry,"");

			qsoFreqDisp3 = new cFreqControl(
				pad, y,
				freqwidth, freqheight, "10");
			qsoFreqDisp3->box(FL_DOWN_BOX);
			qsoFreqDisp3->color(FL_BACKGROUND_COLOR);
			qsoFreqDisp3->selection_color(FL_BACKGROUND_COLOR);
			qsoFreqDisp3->labeltype(FL_NORMAL_LABEL);
			qsoFreqDisp3->align(FL_ALIGN_CENTER);
			qsoFreqDisp3->when(FL_WHEN_RELEASE);
			qsoFreqDisp3->callback(qso_movFreq);
			qsoFreqDisp3->font(progdefaults.FreqControlFontnbr);
			qsoFreqDisp3->SetONOFFCOLOR(
				fl_rgb_color(	progdefaults.FDforeground.R,
								progdefaults.FDforeground.G,
								progdefaults.FDforeground.B),
				fl_rgb_color(	progdefaults.FDbackground.R,
								progdefaults.FDbackground.G,
								progdefaults.FDbackground.B));
			qsoFreqDisp3->value(0);

			qso_opPICK3 = new Fl_Button(
				pad + rightof(qsoFreqDisp3), y,
				Wbtn, Hentry);
			qso_opPICK3->align(FL_ALIGN_INSIDE);
			qso_opPICK3->image(addrbookpixmap);
			qso_opPICK3->callback(showOpBrowserView2, 0);
			qso_opPICK3->tooltip(_("Open List"));

			qsoClear3 = new Fl_Button(
					pad + rightof(qso_opPICK3), y,
					Wbtn, Hentry);
			qsoClear3->align(FL_ALIGN_INSIDE);
			qsoClear3->image(new Fl_Pixmap(edit_clear_icon));
			qsoClear3->callback(qsoClear_cb, 0);
			qsoClear3->tooltip(_("Clear"));

			qsoSave3 = new Fl_Button(
					pad + rightof(qsoClear3), y,
					Wbtn, Hentry);
			qsoSave3->align(FL_ALIGN_INSIDE);
			qsoSave3->image(new Fl_Pixmap(save_icon));
			qsoSave3->callback(qsoSave_cb, 0);
			qsoSave3->tooltip(_("Save"));

			inpCall3 = new Fl_Input2(
				rightof(qsoSave3) + fl_width("Call"), y,
				wCall, h, "Call");
			inpCall3->align(FL_ALIGN_LEFT);
			inpCall3->tooltip(_("Other call"));

			TopFrame3a->end();

			TopFrame3b = new Fl_Group(
				rightof(TopFrame3a), TopFrame1->y(),
				W - rightof(TopFrame3a), Hentry,"");

// LOG_GENERIC - partial
			log_generic_frame = new Fl_Group(
				TopFrame3b->x(), TopFrame3b->y(),
				TopFrame3b->w(), Hentry,"");

			btnTimeOn3 = new Fl_Button(
				rightof(inpCall3) + pad, y,
				h, h, "On");
			btnTimeOn3->tooltip(_("Press to update"));
			btnTimeOn3->callback(cb_btnTimeOn);

			inpTimeOn3 = new Fl_Input2(
				rightof(btnTimeOn3) + pad, y,
				wData, h, "");
			inpTimeOn3->tooltip(_("Time On"));
			inpTimeOn3->type(FL_INT_INPUT);

			inpTimeOff3 = new Fl_Input2(
				rightof(inpTimeOn3) + fl_width("xOff"), y,
				wData, h, "Off");
			inpTimeOff3->tooltip(_("Time Off"));
			inpTimeOff3->type(FL_NORMAL_OUTPUT);

			inpSerNo2 = new Fl_Input2(
				rightof(inpTimeOff3) + fl_width("xR#"), y,
				wData, h, "R#");
			inpSerNo2->align(FL_ALIGN_LEFT);
			inpSerNo2->tooltip(_("Received serial number"));

			outSerNo2 = new Fl_Input2(
				rightof(inpSerNo2) + fl_width("xS#"), y,
				wData, h, "S#");
			outSerNo2->align(FL_ALIGN_LEFT);
			outSerNo2->tooltip(_("Sent serial number (read only)"));

			inpXchgIn2 = new Fl_Input2(
				rightof(outSerNo2) + fl_width("xXch"), y,
				fl_digi_main->w() - (rightof(outSerNo2) + fl_width("xXchg"))- pad, h, "Xch");
			inpXchgIn2->align(FL_ALIGN_LEFT);
			inpXchgIn2->tooltip(_("Contest exchange in"));

			Fl_Box lgf_box(rightof(inpXchgIn2), y, pad, h,"");
			lgf_box.box(FL_FLAT_BOX);

			log_generic_frame->end();
			log_generic_frame->hide();
			log_generic_frame->resizable(lgf_box);
// end LOG_GENERIC - partial

// LOG_FD - partial
			log_fd_frame = new Fl_Group(
				TopFrame3b->x(), TopFrame3b->y(),
				TopFrame3b->w(), Hentry,"");

			btnTimeOn4 = new Fl_Button(
				rightof(inpCall3) + pad, y,
				h, h, "On");
			btnTimeOn4->tooltip(_("Press to update"));
			btnTimeOn4->callback(cb_btnTimeOn);

			inpTimeOn4 = new Fl_Input2(
				rightof(btnTimeOn4) + pad, y,
				wData, h, "");
			inpTimeOn4->tooltip(_("Time On"));
			inpTimeOn4->type(FL_INT_INPUT);

			inpTimeOff4 = new Fl_Input2(
				rightof(inpTimeOn4) + fl_width("xOff"), y,
				wData, h, "Off");
			inpTimeOff4->tooltip(_("Time Off"));
			inpTimeOff4->type(FL_NORMAL_OUTPUT);

			inp_FD_class2 = new Fl_Input2(
				rightof(inpTimeOff4) + fl_width("xClass"), y, wData, h, " Class");
			inp_FD_class2->align(FL_ALIGN_LEFT);
			inp_FD_class2->tooltip(_("Received FD class"));
			inp_FD_class2->type(FL_NORMAL_INPUT);

			inp_FD_section2 = new Fl_Input2(
				rightof(inp_FD_class2) + fl_width("xSect") - pad, y, wData, h, "Sect");
			inp_FD_section2->align(FL_ALIGN_LEFT);
			inp_FD_section2->tooltip(_("Received FD section"));
			inp_FD_section2->type(FL_NORMAL_INPUT);

			Fl_Box lfd_box(rightof(inp_FD_section2), y, pad, h,"");
			lfd_box.box(FL_FLAT_BOX);

			log_fd_frame->end();
			log_fd_frame->hide();
			log_fd_frame->resizable(lfd_box);
// end LOG_FD - partial

// LOG_KD - partial
			log_kd_frame = new Fl_Group(
				TopFrame3b->x(), TopFrame3b->y(),
				TopFrame3b->w(), Hentry,"");

			inp_KD_name2 = new Fl_Input2(
				rightof(inpCall3) + fl_width("xNam"), y, 70, h, "Nam");
			inp_KD_name2->align(FL_ALIGN_LEFT);
			inp_KD_name2->tooltip("Guest operator");
			inp_KD_name2->type(FL_NORMAL_INPUT);

			inp_KD_age2 = new Fl_Input2(
				rightof(inp_KD_name2) + fl_width("xAge"), y, wData, h,
				"Age");
			inp_KD_age2->align(FL_ALIGN_LEFT);
			inp_KD_age2->tooltip(_("Guest operators age"));
			inp_KD_age2->type(FL_NORMAL_INPUT);

			inp_KD_state2 = new Fl_Input2(
				rightof(inp_KD_age2) + fl_width("xSt"), y, 40, h,
				"St");
			inp_KD_state2->align(FL_ALIGN_LEFT);
			inp_KD_state2->tooltip(_("Station state"));
			inp_KD_state2->type(FL_NORMAL_INPUT);

			inp_KD_VEprov2 = new Fl_Input2(
				rightof(inp_KD_state2) + fl_width("xPr"), y, 40, h,
				"Pr");
			inp_KD_VEprov2->align(FL_ALIGN_LEFT);
			inp_KD_VEprov2->tooltip(_("Station province"));
			inp_KD_VEprov2->type(FL_NORMAL_INPUT);

			inp_KD_XchgIn2 = new Fl_Input2(
				rightof(inp_KD_VEprov2) + fl_width("xXch"), y,
				fl_digi_main->w() - (rightof(inp_KD_state2) + fl_width("xXch")) - pad, h,
				"Xch");
			inp_KD_XchgIn2->align(FL_ALIGN_LEFT);
			inp_KD_XchgIn2->tooltip(_("Special Kids Day Special Exchange"));
			inp_KD_XchgIn2->type(FL_NORMAL_INPUT);

			Fl_Box lkd_box(rightof(inp_KD_XchgIn2), y, pad, h,"");
			lkd_box.box(FL_FLAT_BOX);

			log_kd_frame->end();
			log_kd_frame->hide();
			log_kd_frame->resizable(lkd_box);
// end LOG_KD -partial

// LOG_1010 - partial
			log_1010_frame = new Fl_Group(
				TopFrame3b->x(), TopFrame3b->y(),
				TopFrame3b->w(), Hentry,"");

			inp_1010_name2 = new Fl_Input2(
				rightof(inpCall3) + fl_width("xOp"), y, 80, h, "Op");
			inp_1010_name2->align(FL_ALIGN_LEFT);
			inp_1010_name2->tooltip("Operator's name");
			inp_1010_name2->type(FL_NORMAL_INPUT);

			inp_1010_nr2 = new Fl_Input2(
				rightof(inp_1010_name2) + fl_width("x1010"), y, wData, h,
				"1010");
			inp_1010_nr2->align(FL_ALIGN_LEFT);
			inp_1010_nr2->tooltip(_("1010 number"));
			inp_1010_nr2->type(FL_NORMAL_INPUT);

			inp_1010_XchgIn2 = new Fl_Input2(
				rightof(inp_1010_nr2) + fl_width("xXch"), y,
				fl_digi_main->w() - (rightof(inp_1010_nr2) + fl_width("xXch")) - pad, h,
				"Xch");
			inp_1010_XchgIn2->align(FL_ALIGN_LEFT);
			inp_1010_XchgIn2->tooltip(_("1010 Exchange"));
			inp_1010_XchgIn2->type(FL_NORMAL_INPUT);

			Fl_Box l1010_box(rightof(inp_1010_XchgIn2), y, pad, h,"");
			l1010_box.box(FL_FLAT_BOX);

			log_1010_frame->end();
			log_1010_frame->hide();
			log_1010_frame->resizable(l1010_box);
// end LOG_1010 -partial

// LOG_ARR - partial
			log_arr_frame = new Fl_Group(
				TopFrame3b->x(), TopFrame3b->y(),
				TopFrame3b->w(), Hentry,"");

			inp_ARR_Name2 = new Fl_Input2(
				rightof(inpCall3) + fl_width("xNam"), y, 80, h,
				"Nam");
			inp_ARR_Name2->align(FL_ALIGN_LEFT);
			inp_ARR_Name2->tooltip("Operator's name");
			inp_ARR_Name2->type(FL_NORMAL_INPUT);

			inp_ARR_check2 = new Fl_Input2(
				rightof(inp_ARR_Name2) + fl_width("xChk"), y, 40, h,
				"Chk");
			inp_ARR_check2->align(FL_ALIGN_LEFT);
			inp_ARR_check2->tooltip(_("Check / birth-year"));
			inp_ARR_check2->type(FL_NORMAL_INPUT);

			inp_ARR_XchgIn2 = new Fl_Input2(
				rightof(inp_ARR_check2) + fl_width("xXch"), y,
				fl_digi_main->w() - (rightof(inp_ARR_check2) + fl_width("xXch")) - pad, Hentry,
				"Xch");
			inp_ARR_XchgIn2->align(FL_ALIGN_LEFT);
			inp_ARR_XchgIn2->tooltip(_("Round Up Exchange"));
			inp_ARR_XchgIn2->type(FL_NORMAL_INPUT);

			Fl_Box larr_box(rightof(inp_ARR_XchgIn2), y, pad, h,"");
			larr_box.box(FL_FLAT_BOX);

			log_arr_frame->end();
			log_arr_frame->hide();
			log_arr_frame->resizable(larr_box);
// end LOG_ARR - partial

// LOG_VHF - partial
			log_vhf_frame = new Fl_Group(
				TopFrame3b->x(), TopFrame3b->y(),
				TopFrame3b->w(), Hentry,"");

			inp_vhf_RSTin2 = new Fl_Input2(
				rightof(inpCall3) + fl_width("xIn"), y, wRST, h,
				"In");
			inp_vhf_RSTin2->align(FL_ALIGN_LEFT);
			inp_vhf_RSTin2->tooltip(_("Received RST"));
			inp_vhf_RSTin2->type(FL_NORMAL_INPUT);

			inp_vhf_RSTout2 = new Fl_Input2(
				rightof(inp_vhf_RSTin2) + fl_width("xOut"), y, wRST, h,
				"Out");
			inp_vhf_RSTout2->align(FL_ALIGN_LEFT);
			inp_vhf_RSTout2->tooltip(_("Sent RST"));
			inp_vhf_RSTout2->type(FL_NORMAL_INPUT);

			inp_vhf_Loc2 = new Fl_Input2(
				rightof(inp_vhf_RSTout2) + fl_width("xGr")- pad, y, 80, h,
				"Gr");
			inp_vhf_Loc2->align(FL_ALIGN_LEFT);
			inp_vhf_Loc2->tooltip(_("Grid Locator"));
			inp_vhf_Loc2->type(FL_NORMAL_INPUT);

			Fl_Box lvhf_box(rightof(inp_vhf_Loc2), y, pad, h,"");
			lvhf_box.box(FL_FLAT_BOX);

			log_vhf_frame->end();
			log_vhf_frame->hide();
			log_vhf_frame->resizable(lvhf_box);
// end LOG_VHF - partial

// LOG_CQWW_DX - partial
			log_cqww_frame = new Fl_Group(
				TopFrame3b->x(), TopFrame3b->y(),
				TopFrame3b->w(), Hentry,"");

			inp_CQDX_RSTin2 = new Fl_Input2(
				rightof(inpCall3) + fl_width("xIn"), y, wRST, h, "In");
			inp_CQDX_RSTin2->align(FL_ALIGN_LEFT);
			inp_CQDX_RSTin2->tooltip(_("Received RST"));
			inp_CQDX_RSTin2->type(FL_NORMAL_INPUT);

			inp_CQDX_RSTout2 = new Fl_Input2(
				rightof(inp_CQDX_RSTin2) + fl_width("xOut"), y, wRST, h, "Out");
			inp_CQDX_RSTout2->align(FL_ALIGN_LEFT);
			inp_CQDX_RSTout2->tooltip(_("Sent RST"));
			inp_CQDX_RSTout2->type(FL_NORMAL_INPUT);

			inp_CQDXzone2 = new Fl_Input2(
				rightof(inp_CQDX_RSTout2) + fl_width("xCQz"), y, 40, h, "CQz");
			inp_CQDXzone2->align(FL_ALIGN_LEFT);
			inp_CQDXzone2->tooltip(_("Received CQ zone"));
			inp_CQDXzone2->type(FL_NORMAL_INPUT);

			xtmp = rightof(inp_CQDXzone2) + fl_width("xCQc");
			cboCountryCQDX2 = new Fl_ComboBox(
				xtmp, y,
				fl_digi_main->w() - xtmp - pad, Hentry, "CQc");
			cboCountryCQDX2->align(FL_ALIGN_LEFT);
			cboCountryCQDX2->tooltip(_("Received CQ country"));
			cboCountryCQDX2->end();

			Fl_Box lcqdx_box(rightof(cboCountryCQDX2), y, pad, h,"");
			lcqdx_box.box(FL_FLAT_BOX);

			log_cqww_frame->end();
			log_cqww_frame->hide();
			log_cqww_frame->resizable(lcqdx_box);
// end LOG_CQWW_DX - partial

// LOG_CQWW_RTTY - partial
			log_cqww_rtty_frame = new Fl_Group(
				TopFrame3b->x(), TopFrame3b->y(),
				TopFrame3b->w(), Hentry,"");

			inp_CQ_RSTin2 = new Fl_Input2(
				rightof(inpCall3) + fl_width("xIn"), y, wRST, h, "In");
			inp_CQ_RSTin2->align(FL_ALIGN_LEFT);
			inp_CQ_RSTin2->tooltip(_("Received RST"));
			inp_CQ_RSTin2->type(FL_NORMAL_INPUT);

			inp_CQ_RSTout2 = new Fl_Input2(
				rightof(inp_CQ_RSTin2) + fl_width("xOut"), y, wRST, h, "Out");
			inp_CQ_RSTout2->align(FL_ALIGN_LEFT);
			inp_CQ_RSTout2->tooltip(_("Sent RST"));
			inp_CQ_RSTout2->type(FL_NORMAL_INPUT);

			inp_CQzone2 = new Fl_Input2(
				rightof(inp_CQ_RSTout2) + fl_width("xCQz"), y, 40, h, "CQz");
			inp_CQzone2->align(FL_ALIGN_LEFT);
			inp_CQzone2->tooltip(_("Received CQ zone"));
			inp_CQzone2->type(FL_NORMAL_INPUT);

			inp_CQstate2 = new Fl_Input2(
				rightof(inp_CQzone2) + fl_width("xCQst"), y, 40, h, "CQst");
			inp_CQstate2->align(FL_ALIGN_LEFT);
			inp_CQstate2->tooltip(_("Received CQ State/Prov"));
			inp_CQstate2->type(FL_NORMAL_INPUT);

			cboCountryCQ2 = new Fl_ComboBox(
				rightof(inp_CQstate2) + fl_width("xCQc"), y,
				fl_digi_main->w() - (rightof(inp_CQstate2) + fl_width("xCQc")) - pad, Hentry, "CQc");
			cboCountryCQ2->align(FL_ALIGN_LEFT);
			cboCountryCQ2->tooltip(_("Received CQ country"));
			cboCountryCQ2->end();

			Fl_Box lcq_box(rightof(cboCountryCQ2), y, pad, h,"");
			lcq_box.box(FL_FLAT_BOX);

			log_cqww_rtty_frame->end();
			log_cqww_rtty_frame->hide();
			log_cqww_rtty_frame->resizable(lcq_box);
// end LOG_CQWW_RTTY - partial

// LOG CWSS - partial
			log_cqss_frame = new Fl_Group(
				TopFrame3b->x(), TopFrame3b->y(),
				TopFrame3b->w(), Hentry,"");

			outSerNo4 = new Fl_Input2(
				rightof(inpCall3) + fl_width("xS#"), y, 40, h,
				"S#");
			outSerNo4->align(FL_ALIGN_LEFT);
			outSerNo4->tooltip(_("Sent serno"));
			outSerNo4->type(FL_NORMAL_OUTPUT);

			inp_SS_SerialNoR2 = new Fl_Input2(
				rightof(outSerNo4) + fl_width("xR#"), y, 40, Hentry,
				"R#");
			inp_SS_SerialNoR2->align(FL_ALIGN_LEFT);
			inp_SS_SerialNoR2->tooltip(_("Received serno"));
			inp_SS_SerialNoR2->type(FL_NORMAL_INPUT);

			inp_SS_Precedence2 = new Fl_Input2(
				rightof(inp_SS_SerialNoR2) + fl_width("xPre"), y, 40, Hentry,
				"Pre");
			inp_SS_Precedence2->align(FL_ALIGN_LEFT);
			inp_SS_Precedence2->tooltip(_("SS Precedence"));
			inp_SS_Precedence2->type(FL_NORMAL_INPUT);

			inp_SS_Check2 = new Fl_Input2(
				rightof(inp_SS_Precedence2) + fl_width("xChk"), y, 40, Hentry,
				"Chk");
			inp_SS_Check2->align(FL_ALIGN_LEFT);
			inp_SS_Check2->tooltip(_("SS Check"));
			inp_SS_Check2->type(FL_NORMAL_INPUT);

			inp_SS_Section2 = new Fl_Input2(
				rightof(inp_SS_Check2) + fl_width("xSec"), y, 40, Hentry,
				"Sec");
			inp_SS_Section2->align(FL_ALIGN_LEFT);
			inp_SS_Section2->tooltip(_("SS section"));
			inp_SS_Section2->type(FL_NORMAL_INPUT);

			Fl_Box lss_box(rightof(inp_SS_Section2), y, pad, h,"");
			lss_box.box(FL_FLAT_BOX);

			log_cqss_frame->end();
			log_cqss_frame->hide();
			log_cqss_frame->resizable(lss_box);
// end LOG CWSS - partial

// LOG_CQWPX - partial
			log_cqwpx_frame = new Fl_Group(
				TopFrame3b->x(), TopFrame3b->y(),
				TopFrame3b->w(), Hentry,"");

			inpRstIn_WPX2 = new Fl_Input2(
				rightof(inpCall3) + fl_width("xIn"), y, wRST, h, "In");
			inpRstIn_WPX2->align(FL_ALIGN_LEFT);
			inpRstIn_WPX2->tooltip(_("Received RST"));
			inpRstIn_WPX2->type(FL_NORMAL_INPUT);

			inpRstOut_WPX2 = new Fl_Input2(
				rightof(inpRstIn_WPX2) + fl_width("xOut"), y, wRST, h, "Out");
			inpRstOut_WPX2->align(FL_ALIGN_LEFT);
			inpRstOut_WPX2->tooltip(_("Sent RST"));
			inpRstOut_WPX2->type(FL_NORMAL_INPUT);

			outSerNo_WPX2 = new Fl_Input2(
				rightof(inpRstOut_WPX2) + fl_width("xS#"), y, 40, h, "S#");
			outSerNo_WPX2->align(FL_ALIGN_LEFT);
			outSerNo_WPX2->tooltip(_("Sent serial number"));
			outSerNo_WPX2->type(FL_NORMAL_INPUT);

			inpSerNo_WPX2 = new Fl_Input2(
				rightof(outSerNo_WPX2) + fl_width("xR#") - pad, y, 40, h, "R#");
			inpSerNo_WPX2->align(FL_ALIGN_LEFT);
			inpSerNo_WPX2->tooltip(_("Received serial number"));
			inpSerNo_WPX2->type(FL_NORMAL_INPUT);

			Fl_Box lwpx_box(rightof(inpSerNo_WPX2), y, pad, h,"");
			lwpx_box.box(FL_FLAT_BOX);

			log_cqwpx_frame->end();
			log_cqwpx_frame->hide();
			log_cqwpx_frame->resizable(lwpx_box);
// end LOG_CQWPX - partial

// LOG_ASCR - partial
			log_ascr_frame = new Fl_Group(
				TopFrame3b->x(), TopFrame3b->y(),
				TopFrame3b->w(), Hentry,"");

			inp_ASCR_name2 = new Fl_Input2(
				rightof(inpCall3) + fl_width("xNam"), y, 80, h,
				"Nam");
			inp_ASCR_name2->align(FL_ALIGN_LEFT);
			inp_ASCR_name2->tooltip(_("Rcvd name"));
			inp_ASCR_name2->type(FL_NORMAL_INPUT);

			inp_ASCR_RSTin2 = new Fl_Input2(
				rightof(inp_ASCR_name2) + fl_width("xIn"), y, wRST, Hentry,
				"In");
			inp_ASCR_RSTin2->align(FL_ALIGN_LEFT);
			inp_ASCR_RSTin2->tooltip(_("Received RST"));
			inp_ASCR_RSTin2->type(FL_NORMAL_INPUT);

			inp_ASCR_RSTout2 = new Fl_Input2(
				rightof(inp_ASCR_RSTin2) + fl_width("xOut"), y, wRST, Hentry,
				"Out");
			inp_ASCR_RSTout2->align(FL_ALIGN_LEFT);
			inp_ASCR_RSTout2->tooltip(_("Sent RST"));
			inp_ASCR_RSTout2->type(FL_NORMAL_INPUT);

			inp_ASCR_class2 = new Fl_Input2(
				rightof(inp_ASCR_RSTout2) + fl_width("xClass"), y, 30, Hentry,
				"Class");
			inp_ASCR_class2->align(FL_ALIGN_LEFT);
			inp_ASCR_class2->tooltip(_("ASCR class"));
			inp_ASCR_class2->type(FL_NORMAL_INPUT);

			xtmp = rightof(inp_ASCR_class2) + fl_width("xSPC");

			inp_ASCR_XchgIn2 = new Fl_Input2(
				xtmp, y, TopFrame3->x() + TopFrame3->w() - xtmp - pad, Hentry,
				"SPC");
			inp_ASCR_XchgIn2->align(FL_ALIGN_LEFT);
			inp_ASCR_XchgIn2->tooltip(_("State/Province/Country received"));
			inp_ASCR_XchgIn2->type(FL_NORMAL_INPUT);

			Fl_Box lascr_box(rightof(inp_ASCR_XchgIn2), y, pad, h,"");
			lascr_box.box(FL_FLAT_BOX);

			log_ascr_frame->end();
			log_ascr_frame->hide();
			log_ascr_frame->resizable(lascr_box);

// end LOG_ASCR - partial

// LOG_NAQP - partial
			log_naqp_frame = new Fl_Group(
				TopFrame3b->x(), TopFrame3b->y(),
				TopFrame3b->w(), Hentry,"");

			btnTimeOn5 = new Fl_Button(
				rightof(inpCall3) + pad, y,
				h, h, "On");
			btnTimeOn5->tooltip(_("Press to update"));
			btnTimeOn5->callback(cb_btnTimeOn);

			inpTimeOn5 = new Fl_Input2(
				rightof(btnTimeOn3) + pad, y,
				wData, h, "");
			inpTimeOn5->tooltip(_("Time On"));
			inpTimeOn5->type(FL_INT_INPUT);

			inpTimeOff5 = new Fl_Input2(
				rightof(inpTimeOn5) + fl_width("xOff"), y,
				wData, h, "Off");
			inpTimeOff5->tooltip(_("Time Off"));
			inpTimeOff5->type(FL_NORMAL_OUTPUT);

			inpNAQPname2 = new Fl_Input2(
				rightof(inpTimeOff5) + fl_width("xNam"), y, 100, h, "Nam");
			inpNAQPname2->align(FL_ALIGN_LEFT);
			inpNAQPname2->tooltip(_("Received operator name"));
			inpNAQPname2->type(FL_NORMAL_INPUT);

			inpSPCnum_NAQP2 = new Fl_Input2(
				rightof(inpNAQPname2) + fl_width("xXch"), y,
				80, h, "Xch");
			inpSPCnum_NAQP2->align(FL_ALIGN_LEFT);
			inpSPCnum_NAQP2->tooltip(_("Received State/Province/Country"));
			inpSPCnum_NAQP2->type(FL_NORMAL_INPUT);

			Fl_Box lnaqp_box(rightof(inpSPCnum_NAQP2), y, pad, h,"");
			lnaqp_box.box(FL_FLAT_BOX);

			log_naqp_frame->end();
			log_naqp_frame->hide();
			log_naqp_frame->resizable(lnaqp_box);
// LOG_NAQP - partial

// LOG_RTTY - partial
			log_rtty_frame = new Fl_Group(
				TopFrame3b->x(), TopFrame3b->y(),
				TopFrame3b->w(), Hentry,"");

			inpRTU_stpr2 = new Fl_Input2(
				rightof(inpCall3) + fl_width("xS/P"), y, fl_width("xWW"), Hentry,
				"S/P");
			inpRTU_stpr2->align(FL_ALIGN_LEFT);
			inpRTU_stpr2->tooltip(_("State/Province"));
			inpRTU_stpr2->type(FL_NORMAL_INPUT);

			inpRTU_serno2 = new Fl_Input2(
				rightof(inpRTU_stpr2) + fl_width("xSer"), y, fl_width("x9999"), Hentry,
				"Ser");
			inpRTU_serno2->align(FL_ALIGN_LEFT);
			inpRTU_serno2->tooltip(_("Serial number received"));
			inpRTU_serno2->type(FL_NORMAL_INPUT);

			inpRTU_RSTin2 = new Fl_Input2(
				rightof(inpRTU_serno2) + fl_width("xR"), y, wRST, Hentry,
				"R");
			inpRTU_RSTin2->align(FL_ALIGN_LEFT);
			inpRTU_RSTin2->tooltip("Received RST");
			inpRTU_RSTin2->type(FL_NORMAL_INPUT);

			inpRTU_RSTout2 = new Fl_Input2(
				rightof(inpRTU_RSTin2) + fl_width("xS"), y, wRST, Hentry,
				"S");
			inpRTU_RSTout2->align(FL_ALIGN_LEFT);
			inpRTU_RSTout2->tooltip("Sent RST");
			inpRTU_RSTout2->type(FL_NORMAL_INPUT);

			xtmp = rightof(inpRTU_RSTout2) + fl_width("xCntry");
			cboCountryRTU2 = new Fl_ComboBox(
				xtmp, y,
				TopFrame3->x() + TopFrame3->w() - xtmp - pad, Hentry,
				"Cntry");
			cboCountryRTU2->align(FL_ALIGN_LEFT);
			cboCountryRTU2->tooltip(_("Country"));
			cboCountryRTU2->end();

			log_rtty_frame->end();
			log_rtty_frame->hide();
			log_rtty_frame->resizable(cboCountryRTU2);
// end LOG_RTTY - partial

// LOG_IARI - partial
			log_iari_frame = new Fl_Group(
				rightof(TopFrame3a), TopFrame1->y(),
				W - rightof(TopFrame3a), Hentry,"");

			inp_IARI_RSTin2 = new Fl_Input2(
				rightof(inpCall3) + fl_width("In"), y, wRST, h, "In");
			inp_IARI_RSTin2->align(FL_ALIGN_LEFT);
			inp_IARI_RSTin2->tooltip(_("Received RST"));
			inp_IARI_RSTin2->type(FL_NORMAL_INPUT);

			inp_IARI_RSTout2 = new Fl_Input2(
				rightof(inp_IARI_RSTin2) + fl_width("Out"), y, wRST, h, "Out");
			inp_IARI_RSTout2->align(FL_ALIGN_LEFT);
			inp_IARI_RSTout2->tooltip(_("Sent RST"));
			inp_IARI_RSTout2->type(FL_NORMAL_INPUT);

			inp_IARI_PR2 = new Fl_Input2(
				rightof(inp_IARI_RSTout2) + fl_width("xPr"), y, fl_width("WW"), Hentry,
				"Pr");
			inp_IARI_PR2->align(FL_ALIGN_LEFT);
			inp_IARI_PR2->tooltip(_("Received IARI Province"));
			inp_IARI_PR2->type(FL_NORMAL_INPUT);

			out_IARI_SerNo2 = new Fl_Input2(
				rightof(inp_IARI_PR2) + fl_width("S#"), y, wRST, Hentry,
				"S#");
			out_IARI_SerNo2->align(FL_ALIGN_LEFT);
			out_IARI_SerNo2->tooltip(_("Sent serno"));
			out_IARI_SerNo2->type(FL_NORMAL_OUTPUT);

			inp_IARI_SerNo2 = new Fl_Input2(
				rightof(out_IARI_SerNo2) + fl_width("R#"), y, wRST, Hentry,
				"R#");
			inp_IARI_SerNo2->align(FL_ALIGN_LEFT);
			inp_IARI_SerNo2->tooltip(_("Received serno"));
			inp_IARI_SerNo2->type(FL_NORMAL_INPUT);

			xtmp = rightof(inp_IARI_SerNo2) + fl_width("Cntry");
			cboCountryIARI2 = new Fl_ComboBox(
				xtmp, y,
				fl_digi_main->w() - xtmp - pad, Hentry, "Cntry");
			cboCountryIARI2->align(FL_ALIGN_LEFT);
			cboCountryIARI2->tooltip(_("Received IARI country"));
			cboCountryIARI2->end();

			Fl_Box liari_box(rightof(cboCountryIARI2), y, pad, h,"");
			liari_box.box(FL_FLAT_BOX);

			log_iari_frame->end();
			log_iari_frame->hide();
			log_iari_frame->resizable(liari_box);
// end LOG_IARI - partial

// LOG_NAS - partial
			log_nas_frame = new Fl_Group(
				TopFrame3b->x(), TopFrame3b->y(),
				TopFrame3b->w(), Hentry,"");

			outSerNo6 = new Fl_Input2(
				rightof(inpCall3) + fl_width("xS#"), y, 40, h, "S#");
			outSerNo6->align(FL_ALIGN_LEFT);
			outSerNo6->tooltip(_("Sent serial number"));
			outSerNo6->type(FL_NORMAL_OUTPUT);

			inp_ser_NAS2 = new Fl_Input2(
				rightof(outSerNo6) + fl_width("xR#"), y, 40, Hentry, "R#");
			inp_ser_NAS2->align(FL_ALIGN_LEFT);
			inp_ser_NAS2->tooltip(_("Received serno"));
			inp_ser_NAS2->type(FL_NORMAL_INPUT);

			xtmp = rightof(inp_ser_NAS2) + fl_width("xS/P/C");
			inpSPCnum_NAS2 = new Fl_Input2(
				xtmp, y, 80, Hentry,
				"S/P/C");
			inpSPCnum_NAS2->align(FL_ALIGN_LEFT);
			inpSPCnum_NAS2->tooltip(_("Received State/Province/Country"));
			inpSPCnum_NAS2->type(FL_NORMAL_INPUT);

			xtmp = rightof(inpSPCnum_NAS2) + fl_width("xNm");
			inp_name_NAS2 = new Fl_Input2(
				xtmp, y,
				TopFrame3->x() + TopFrame3->w() - xtmp - pad, Hentry,
				"Nm");
			inp_name_NAS2->align(FL_ALIGN_LEFT);
			inp_name_NAS2->tooltip(_("Name"));
			inp_name_NAS2->type(FL_NORMAL_INPUT);

			Fl_Box lnas_box(rightof(inp_name_NAS2), y, pad, h,"");
			lnas_box.box(FL_FLAT_BOX);

			log_nas_frame->end();
			log_nas_frame->hide();
			log_nas_frame->resizable(lnas_box);
// end LOG_NAS - partial

// LOG_AIDX - partial
			log_aidx_frame = new Fl_Group(
				TopFrame3b->x(), TopFrame3b->y(),
				TopFrame3b->w(), Hentry,"");

			outSerNo8 = new Fl_Input2(
				rightof(inpCall3) + fl_width("xS#"), y, 40, Hentry, "S#");
			outSerNo8->align(FL_ALIGN_LEFT);
			outSerNo8->tooltip(_("Sent serial number"));
			outSerNo8->type(FL_NORMAL_OUTPUT);

			inpSerNo4 = new Fl_Input2(
				rightof(outSerNo8) + fl_width("xR#"), y, 40, Hentry, "R#");
			inpSerNo4->align(FL_ALIGN_LEFT);
			inpSerNo4->tooltip(_("Received serial number"));
			inpSerNo4->type(FL_NORMAL_INPUT);

			xtmp = rightof(inpSerNo4) + fl_width("xIn");
			inpRstIn3 = new Fl_Input2(
				xtmp, y, wRST, Hentry,
				"In");
			inpRstIn3->align(FL_ALIGN_LEFT);
			inpRstIn3->tooltip(_("Received RST"));
			inpRstIn3->type(FL_NORMAL_INPUT);

			xtmp = rightof(inpRstIn3) + fl_width("xOut");
			inpRstOut3 = new Fl_Input2(
				xtmp, y, wRST, Hentry,
				"Out");
			inpRstOut3->align(FL_ALIGN_LEFT);
			inpRstOut3->tooltip(_("Sent RST"));
			inpRstOut3->type(FL_NORMAL_INPUT);

			xtmp = rightof(inpRstOut3) + fl_width("xCntry");
			cboCountryAIDX2 = new Fl_ComboBox(
				xtmp, y,
				TopFrame3->x() + TopFrame3->w() - xtmp - pad, Hentry,
				"Cntry");
			cboCountryAIDX2->align(FL_ALIGN_LEFT);
			cboCountryAIDX2->tooltip(_("Received Country"));
			cboCountryAIDX2->end();

			Fl_Box laam_box(rightof(cboCountryAIDX2), y, pad, h,"");
			laam_box.box(FL_FLAT_BOX);

			log_aidx_frame->end();
			log_aidx_frame->hide();
			log_aidx_frame->resizable(laam_box);
// end LOG_AIDX - partial

// LOG_JOTA - partial
			log_jota_frame = new Fl_Group(
				TopFrame3b->x(), TopFrame3b->y(),
				TopFrame3b->w(), Hentry,"");

			xtmp = rightof(inpCall3) + fl_width("xIn");
			inpRstIn4 = new Fl_Input2(
				xtmp, y, wRST, Hentry,
				"In");
			inpRstIn4->align(FL_ALIGN_LEFT);
			inpRstIn4->tooltip(_("Received RST"));
			inpRstIn4->type(FL_NORMAL_INPUT);

			xtmp = rightof(inpRstIn4) + fl_width("xOut");
			inpRstOut4 = new Fl_Input2(
				xtmp, y, wRST, Hentry,
				"Out");
			inpRstOut4->align(FL_ALIGN_LEFT);
			inpRstOut4->tooltip(_("Sent RST"));
			inpRstOut4->type(FL_NORMAL_INPUT);

			xtmp = rightof(inpRstOut4) + fl_width("xSc");
			inp_JOTA_troop2 = new Fl_Input2(
				xtmp, y, 50, Hentry,
				"Tp");
			inp_JOTA_troop2->align(FL_ALIGN_LEFT);
			inp_JOTA_troop2->tooltip(_("Received troop number"));
			inp_JOTA_troop2->type(FL_NORMAL_INPUT);

			xtmp = rightof(inp_JOTA_troop2) + fl_width("xTNm");
			inp_JOTA_scout2 = new Fl_Input2(
				xtmp, y, 80, Hentry,
				"Nm");
			inp_JOTA_scout2->align(FL_ALIGN_LEFT);
			inp_JOTA_scout2->tooltip(_("Received scout name"));
			inp_JOTA_scout2->type(FL_NORMAL_INPUT);

			xtmp = rightof(inp_JOTA_scout2) + fl_width("xSPC");
			inp_JOTA_spc2 = new Fl_Input2(
				xtmp, y,
				TopFrame3->x() + TopFrame3->w() - xtmp - pad, Hentry,
				"SPC");
			inp_JOTA_spc2->align(FL_ALIGN_LEFT);
			inp_JOTA_spc2->tooltip(_("State/Province,Country received"));
			inp_JOTA_spc2->type(FL_NORMAL_INPUT);

			Fl_Box ljota_box(rightof(inp_JOTA_spc2), y, pad, h,"");
			ljota_box.box(FL_FLAT_BOX);

			log_jota_frame->end();
			log_jota_frame->hide();
			log_jota_frame->resizable(ljota_box);
// LOG_JOTA - partial

// LOG_AICW - partial
			log_aicw_frame = new Fl_Group(
				TopFrame3b->x(), TopFrame3b->y(),
				TopFrame3b->w(), Hentry,"");

			xtmp = rightof(inpCall3) + fl_width("xIn");
			inpRstIn_AICW2 = new Fl_Input2(
				xtmp, y, wRST, Hentry,
				"In");
			inpRstIn_AICW2->align(FL_ALIGN_LEFT);
			inpRstIn_AICW2->tooltip(_("Received RST"));
			inpRstIn_AICW2->type(FL_NORMAL_INPUT);

			xtmp = rightof(inpRstIn_AICW2) + fl_width("xOut");
			inpRstOut_AICW2 = new Fl_Input2(
				xtmp, y, wRST, Hentry,
				"Out");
			inpRstOut_AICW2->align(FL_ALIGN_LEFT);
			inpRstOut_AICW2->tooltip(_("Sent RST"));
			inpRstOut_AICW2->type(FL_NORMAL_INPUT);

			xtmp = rightof(inpRstOut_AICW2) + fl_width("xPwr-R");
			inpSPCnum_AICW2 = new Fl_Input2(
				xtmp, y, 50, Hentry,
				"Pwr-R");
			inpSPCnum_AICW2->align(FL_ALIGN_LEFT);
			inpSPCnum_AICW2->tooltip(_("Received power"));
			inpSPCnum_AICW2->type(FL_NORMAL_INPUT);

			xtmp = rightof(inpSPCnum_AICW2) + fl_width("xCntry");
			cboCountryAICW2 = new Fl_ComboBox(
				xtmp, y,
				TopFrame3->x() + TopFrame3->w() - xtmp - pad, Hentry,
				"Cntry");
			cboCountryAICW2->align(FL_ALIGN_LEFT);
			cboCountryAICW2->tooltip(_("Country received"));
			cboCountryAICW2->end();

			Fl_Box laicw_box(rightof(cboCountryAICW2), y, pad, h,"");
			laicw_box.box(FL_FLAT_BOX);

			log_aicw_frame->end();
			log_aicw_frame->hide();
			log_aicw_frame->resizable(laicw_box);
// LOG_AICW - partial

// LOG_SQSO - partial
			log_sqso_frame = new Fl_Group(
				TopFrame3b->x(), TopFrame3b->y(),
				TopFrame3b->w(), Hentry,"");

			xtmp = rightof(inpCall3) + fl_width("St");
			inpSQSO_state2 = new Fl_Input2(
				xtmp, y, 35, Hentry,
				"St");
			inpSQSO_state2->align(FL_ALIGN_LEFT);
			inpSQSO_state2->tooltip(_("State"));
			inpSQSO_state2->type(FL_NORMAL_INPUT);

			xtmp = rightof(inpSQSO_state2) + fl_width("Co");
			inpSQSO_county2 = new Fl_Input2(
				xtmp, y, 50, Hentry,
				"Cy");
			inpSQSO_county2->align(FL_ALIGN_LEFT);
			inpSQSO_county2->tooltip(_("County"));
			inpSQSO_county2->type(FL_NORMAL_INPUT);

			xtmp = rightof(inpSQSO_county2) + fl_width("Ca");
			inpSQSO_category2 = new Fl_Input2(
				xtmp, y , 40, Hentry, "Cat");
			inpSQSO_category2->align(FL_ALIGN_LEFT);
			inpSQSO_category2->tooltip(_("Category: CLB, MOB, QRP, STD"));
			inpSQSO_category2->type(FL_NORMAL_INPUT);
			inpSQSO_category2->hide();

			xtmp = rightof(inpSQSO_county2) + fl_width("In");
			inpRstIn_SQSO2 = new Fl_Input2(
				xtmp, y, wRST, Hentry,
				"In");
			inpRstIn_SQSO2->align(FL_ALIGN_LEFT);
			inpRstIn_SQSO2->tooltip(_("Received RST"));
			inpRstIn_SQSO2->type(FL_NORMAL_INPUT);
			inpRstIn_SQSO2->hide();

			xtmp = rightof(inpRstIn_SQSO2) + fl_width("Out");
			inpRstOut_SQSO2 = new Fl_Input2(
				xtmp, y, wRST, Hentry,
				"Out");
			inpRstOut_SQSO2->align(FL_ALIGN_LEFT);
			inpRstOut_SQSO2->tooltip(_("Sent RST"));
			inpRstOut_SQSO2->type(FL_NORMAL_INPUT);
			inpRstOut_SQSO2->hide();

			xtmp = rightof(inpRstOut_SQSO2) + fl_width("S#");
			outSQSO_serno2 = new Fl_Input2(
				xtmp, y, 30, Hentry,
				"S#");
			outSQSO_serno2->align(FL_ALIGN_LEFT);
			outSQSO_serno2->tooltip(_("Sent serial number"));
			outSQSO_serno2->type(FL_NORMAL_INPUT);
			outSQSO_serno2->hide();

			xtmp = rightof(outSQSO_serno2) + fl_width("R#");
			inpSQSO_serno2 = new Fl_Input2(
				xtmp, y, 30, Hentry,
				"R#");
			inpSQSO_serno2->align(FL_ALIGN_LEFT);
			inpSQSO_serno2->tooltip(_("Received serial number"));
			inpSQSO_serno2->type(FL_NORMAL_INPUT);
			inpSQSO_serno2->hide();

			xtmp = rightof(inpSQSO_serno2) + fl_width("Nm");
			inpSQSO_name2 = new Fl_Input2(
				xtmp, y,
				TopFrame3b->x() + TopFrame3b->w() - xtmp - pad, Hentry,
				"Nm");
			inpSQSO_name2->align(FL_ALIGN_LEFT);
			inpSQSO_name2->tooltip(_("Rx name"));
			inpSQSO_name2->type(FL_NORMAL_INPUT);
			inpSQSO_name2->hide();

			Fl_Box lsqso_box(rightof(inpSQSO_name2), y, pad, h,"");
			lsqso_box.box(FL_FLAT_BOX);

			log_sqso_frame->end();
			log_sqso_frame->hide();
			log_sqso_frame->resizable(lsqso_box);
// end LOG_SQSO - partial

// LOG_WAE - partial
			log_wae_frame = new Fl_Group(
				TopFrame3b->x(), TopFrame3b->y(),
				TopFrame3b->w(), Hentry,"");

			inpRstIn_WAE2 = new Fl_Input2(
				rightof(inpCall3) + fl_width("xIn"), y, wRST, h, "In");
			inpRstIn_WAE2->align(FL_ALIGN_LEFT);
			inpRstIn_WAE2->tooltip(_("Received RST"));
			inpRstIn_WAE2->type(FL_NORMAL_INPUT);

			inpRstOut_WAE2 = new Fl_Input2(
				rightof(inpRstIn_WAE2) + fl_width("xOut"), y, wRST, h, "Out");
			inpRstOut_WAE2->align(FL_ALIGN_LEFT);
			inpRstOut_WAE2->tooltip(_("Sent RST"));
			inpRstOut_WAE2->type(FL_NORMAL_INPUT);

			outSerNo_WAE2 = new Fl_Input2(
				rightof(inpRstOut_WAE2) + fl_width("xS#"), y, 40, h, "S#");
			outSerNo_WAE2->align(FL_ALIGN_LEFT);
			outSerNo_WAE2->tooltip(_("Sent serial number"));
			outSerNo_WAE2->type(FL_NORMAL_INPUT);

			inpSerNo_WAE2 = new Fl_Input2(
				rightof(outSerNo_WAE2) + fl_width("xR#"), y, 40, h, "R#");
			inpSerNo_WAE2->align(FL_ALIGN_LEFT);
			inpSerNo_WAE2->tooltip(_("Received serial number"));
			inpSerNo_WAE2->type(FL_NORMAL_INPUT);

			xtmp = rightof(inpSerNo_WAE2) + fl_width("xCntry");
			cboCountryWAE2 = new Fl_ComboBox(
				xtmp, y,
				TopFrame3->x() + TopFrame3->w() - xtmp - pad, Hentry,
				"Cntry");
			cboCountryWAE2->align(FL_ALIGN_LEFT);
			cboCountryWAE2->tooltip(_("Country worked"));
			cboCountryWAE2->end();

			Fl_Box lwae_box(rightof(cboCountryWAE2), y, pad, h,"");
			lwae_box.box(FL_FLAT_BOX);

			log_wae_frame->end();
			log_wae_frame->hide();
			log_wae_frame->resizable(lwae_box);
// LOG_WAE - partial

			TopFrame3b->end();

		TopFrame3->end();
		TopFrame3->resizable(TopFrame3b);
		TopFrame3->hide();
}
{ // default controls
		inpFreq = inpFreq1;
		inpCall = inpCall1;
		inpTimeOn = inpTimeOn1;
		inpTimeOff = inpTimeOff1;
		inpName = inpName1;
		inpRstIn = inpRstIn1;
		inpRstOut = inpRstOut1;
		qsoFreqDisp = qsoFreqDisp1;
		inpSerNo = inpSerNo1;
		outSerNo = outSerNo1;
		inpXchgIn = inpXchgIn1;
		inpClass = inp_FD_class1;
		inpSection = inp_FD_section1;
		inp_CQzone = inp_CQzone1;
		inp_CQstate = inp_CQstate1;
		cboCountry = cboCountryQSO;//cboCountryCQ1;
		inpLoc = inpLoc1;
		inp_SS_Check = inp_SS_Check1;
		inp_SS_Precedence = inp_SS_Precedence1;
		inp_SS_Section = inp_SS_Section1;
		inp_SS_SerialNoR = inp_SS_SerialNoR1;
		inp_KD_age = inp_KD_age1;
		inp_1010_nr = inp_1010_nr1;
		inp_ARR_check = inp_ARR_check1;
		inpSPCnum = inpSPCnum_NAQP1;
		inp_JOTA_troop = inp_JOTA_troop1;
		inp_JOTA_scout = inp_JOTA_scout1;
		inp_JOTA_spc = inp_JOTA_spc1;

		qsoFreqDisp1->set_lsd(progdefaults.sel_lsd);
		qsoFreqDisp2->set_lsd(progdefaults.sel_lsd);
		qsoFreqDisp3->set_lsd(progdefaults.sel_lsd);
}
{ // Top Macro group
		Y = TopFrame1->y() + Hqsoframe + pad;

//------------------- 4 bar macros
		tbar = new Fl_Group(0, Y, fl_digi_main->w(), TB_HEIGHT * 4);
		{
			int xpos = tbar->x();
			int ypos = Y;
			int Wbtn = tbar->w() / 12;
			int remainder = tbar->w() - Wbtn * 12;
			tbar->box(FL_FLAT_BOX);
			for (int i = 0; i < 48; i++) {
				btnDockMacro[i] = new Fl_Button(
					xpos, ypos,
					(remainder > 0) ? Wbtn + 1 : Wbtn, TB_HEIGHT, "");
				remainder--;
				btnDockMacro[i]->box(FL_THIN_UP_BOX);
				btnDockMacro[i]->tooltip(_("Left Click - execute\nRight Click - edit"));
				btnDockMacro[i]->callback(macro_cb, reinterpret_cast<void *>(i | 0x80));
				xpos += btnDockMacro[i]->w();
				if (i == 11 || i == 23 || i == 35) {
					xpos = tbar->x();
					remainder = tbar->w() - Wbtn * 12;
					ypos += TB_HEIGHT;
				}
			}
			tbar->end();
			tbar->hide();
		}
//--------------------------------

		macroFrame2 = new Fl_Group(0, Y, W, MACROBAR_MAX);
			macroFrame2->box(FL_FLAT_BOX);
			mf_group2 = new Fl_Group(0, Y, W - alt_btn_width, macroFrame2->h());
			Wmacrobtn = (mf_group2->w()) / NUMMACKEYS;
			wBLANK = (mf_group2->w() - NUMMACKEYS * Wmacrobtn) / 2;
			xpos = 0;
			ypos = mf_group2->y();
			for (int i = 0; i < NUMMACKEYS; i++) {
				if (i == 4 || i == 8) {
					bx = new Fl_Box(xpos, ypos, wBLANK, macroFrame2->h());
					bx->box(FL_FLAT_BOX);
					xpos += wBLANK;
				}
				btnMacro[NUMMACKEYS + i] = new Fl_Button(xpos, ypos, Wmacrobtn, macroFrame2->h(),
					macros.name[NUMMACKEYS + i].c_str());
				btnMacro[NUMMACKEYS + i]->callback(macro_cb, reinterpret_cast<void *>(NUMMACKEYS + i));
				btnMacro[NUMMACKEYS + i]->tooltip(
					_("Left Click - execute\nShift-Fkey - execute\nRight Click - edit"));
				xpos += Wmacrobtn;
			}
			mf_group2->end();
			btnAltMacros2 = new Fl_Button(
					W - alt_btn_width, ypos,
					alt_btn_width, MACROBAR_MAX, "2");
			btnAltMacros2->callback(altmacro_cb, 0);
			btnAltMacros2->tooltip(_("Shift-key macro set"));
			macroFrame2->resizable(mf_group2);
		macroFrame2->end();

		Y += Hmacros;
}
{ // Center group Rx/Tx/Raster displays
		center_group = new Fl_Group(0, Y, W, Htext);
			center_group->box(FL_FLAT_BOX);

			text_group = new Fl_Group(0, Y, center_group->w(), center_group->h());
			text_group->box(FL_FLAT_BOX);

			text_panel = new Panel(0, Y, center_group->w(), center_group->h());
				text_panel->box(FL_FLAT_BOX);

				mvgroup = new Fl_Group(
					text_panel->x(), text_panel->y(),
					text_panel->w()/2, Htext, "");

					mainViewer = new pskBrowser(mvgroup->x(), mvgroup->y(), mvgroup->w(), mvgroup->h()-42, "");
					mainViewer->box(FL_DOWN_BOX);
					mainViewer->has_scrollbar(Fl_Browser_::VERTICAL);
					mainViewer->callback((Fl_Callback*)cb_mainViewer);
					mainViewer->setfont(progdefaults.ViewerFontnbr, progdefaults.ViewerFontsize);
					mainViewer->tooltip(_("Left click - select\nRight click - clear line"));

// mainViewer uses same regular expression evaluator as Viewer
					mainViewer->seek_re = &seek_re;

					Fl_Group* gseek = new Fl_Group(mvgroup->x(), mvgroup->y() + mvgroup->h() - 42, mvgroup->w(), 20);
// search field
						gseek->box(FL_FLAT_BOX);

						int seek_x = mvgroup->x();
						int seek_y = mvgroup->y() + Htext - 42;
						int seek_w = mvgroup->w();
						txtInpSeek = new Fl_Input2( seek_x, seek_y, seek_w, gseek->h(), "");
						txtInpSeek->callback((Fl_Callback*)cb_mainViewer_Seek);
						txtInpSeek->when(FL_WHEN_CHANGED);
						txtInpSeek->textfont(FL_HELVETICA);
						txtInpSeek->value(progStatus.browser_search.c_str());
						txtInpSeek->do_callback();
						txtInpSeek->tooltip(_("seek - regular expression"));
						gseek->resizable(txtInpSeek);
					gseek->end();

					Fl_Group *g = new Fl_Group(
								mvgroup->x(), mvgroup->y() + mvgroup->h() - 22,
								mvgroup->w(), 22);
						g->box(FL_DOWN_BOX);
				// squelch
						mvsquelch = new Fl_Value_Slider2(g->x(), g->y(), g->w() - 75, g->h());
						mvsquelch->type(FL_HOR_NICE_SLIDER);
						mvsquelch->range(-3.0, 6.0);
						mvsquelch->value(progStatus.VIEWER_psksquelch);
						mvsquelch->step(0.1);
						mvsquelch->color( fl_rgb_color(
							progdefaults.bwsrSliderColor.R,
							progdefaults.bwsrSliderColor.G,
							progdefaults.bwsrSliderColor.B));
						mvsquelch->selection_color( fl_rgb_color(
							progdefaults.bwsrSldrSelColor.R,
							progdefaults.bwsrSldrSelColor.G,
							progdefaults.bwsrSldrSelColor.B));
						mvsquelch->callback( (Fl_Callback *)cb_mvsquelch);
						mvsquelch->tooltip(_("Set Viewer Squelch"));

					// clear button
						btnClearMViewer = new Fl_Button(
										mvsquelch->x() + mvsquelch->w(), g->y(),
										75, g->h(),
										icons::make_icon_label(_("Clear"), edit_clear_icon));
						btnClearMViewer->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
						icons::set_icon_label(btnClearMViewer);
						btnClearMViewer->callback((Fl_Callback*)cb_btnClearMViewer);

						g->resizable(mvsquelch);
					g->end();

					mvgroup->resizable(mainViewer);
				mvgroup->end();
				save_mvx = mvgroup->w();

				int rh = progStatus.tile_y_ratio * text_panel->h();

				if (progdefaults.rxtx_swap) rh = text_panel->h() - rh;

				ReceiveText = new FTextRX(
								text_panel->x() + mvgroup->w(), text_panel->y(),
								text_panel->w() - mvgroup->w(), rh, "" );
					ReceiveText->color(
						fl_rgb_color(
							progdefaults.RxColor.R,
							progdefaults.RxColor.G,
							progdefaults.RxColor.B),
							progdefaults.RxTxSelectcolor);
					ReceiveText->setFont(progdefaults.RxFontnbr);
					ReceiveText->setFontSize(progdefaults.RxFontsize);
					ReceiveText->setFontColor(progdefaults.RxFontcolor, FTextBase::RECV);
					ReceiveText->setFontColor(progdefaults.XMITcolor, FTextBase::XMIT);
					ReceiveText->setFontColor(progdefaults.CTRLcolor, FTextBase::CTRL);
					ReceiveText->setFontColor(progdefaults.SKIPcolor, FTextBase::SKIP);
					ReceiveText->setFontColor(progdefaults.ALTRcolor, FTextBase::ALTR);

				FHdisp = new Raster(
						text_panel->x() + mvgroup->w(), text_panel->y(),
						text_panel->w() - mvgroup->w(), rh,
						progdefaults.HellRcvHeight);
					FHdisp->align(FL_ALIGN_CLIP);
					FHdisp->reverse(progdefaults.HellBlackboard);
					FHdisp->clear();
					FHdisp->hide();

				TransmitText = new FTextTX(
						text_panel->x() + mvgroup->w(), text_panel->y() + ReceiveText->h(),
						text_panel->w() - mvgroup->w(), text_panel->h() - ReceiveText->h() );
					TransmitText->color(
						fl_rgb_color(
							progdefaults.TxColor.R,
							progdefaults.TxColor.G,
							progdefaults.TxColor.B),
							progdefaults.RxTxSelectcolor);
					TransmitText->setFont(progdefaults.TxFontnbr);
					TransmitText->setFontSize(progdefaults.TxFontsize);
					TransmitText->setFontColor(progdefaults.TxFontcolor, FTextBase::RECV);
					TransmitText->setFontColor(progdefaults.XMITcolor, FTextBase::XMIT);
					TransmitText->setFontColor(progdefaults.CTRLcolor, FTextBase::CTRL);
					TransmitText->setFontColor(progdefaults.SKIPcolor, FTextBase::SKIP);
					TransmitText->setFontColor(progdefaults.ALTRcolor, FTextBase::ALTR);
					TransmitText->align(FL_ALIGN_CLIP);

					minbox = new Fl_Box(
						text_panel->x(),
						text_panel->y() + minhtext,
						text_panel->w() - 100,
						text_panel->h() - 2*minhtext);
					minbox->hide();

				text_panel->resizable(minbox);
			text_panel->end();

			text_group->end();
			text_group->resizable(text_panel);

//		wefax_group = new Fl_Group(0, Y, W, Htext);
//			wefax_group->box(FL_FLAT_BOX);
//		wefax_group = wefax_pic::create_wefax_rx_viewer(wefax_group->x(), wefax_group->y(), wefax_group->w(), wefax_group->h());
		wefax_group = create_wefax_rx_viewer(0, Y, W, Htext);
		wefax_group->end();
//			wefax_pic::create_both( true );
//		wefax_group->end();
//		wefax_pic::	wefax_pic::create_wefax_tx_viewer(0, 0, 800, 400 );

		fsq_group = new Fl_Group(0, Y, W, Htext);
			fsq_group->box(FL_FLAT_BOX);
// left, resizable rx/tx widgets
				fsq_left = new Panel(
							0, Y,
							W - 180, Htext);

				fsq_left->box(FL_FLAT_BOX);
					// add rx & monitor
					fsq_rx_text = new FTextRX(
								0, Y,
								fsq_left->w(), fsq_left->h() / 2);
					fsq_rx_text->color(
						fl_rgb_color(
							progdefaults.RxColor.R,
							progdefaults.RxColor.G,
							progdefaults.RxColor.B),
							progdefaults.RxTxSelectcolor);
					fsq_rx_text->setFont(progdefaults.RxFontnbr);
					fsq_rx_text->setFontSize(progdefaults.RxFontsize);
					fsq_rx_text->setFontColor(progdefaults.RxFontcolor, FTextBase::RECV);
					fsq_rx_text->setFontColor(progdefaults.XMITcolor, FTextBase::XMIT);
					fsq_rx_text->setFontColor(progdefaults.CTRLcolor, FTextBase::CTRL);
					fsq_rx_text->setFontColor(progdefaults.SKIPcolor, FTextBase::SKIP);
					fsq_rx_text->setFontColor(progdefaults.ALTRcolor, FTextBase::ALTR);
					fsq_rx_text->setFontColor(progdefaults.fsq_xmt_color, FTextBase::FSQ_TX);
					fsq_rx_text->setFontColor(progdefaults.fsq_directed_color, FTextBase::FSQ_DIR);
					fsq_rx_text->setFontColor(progdefaults.fsq_undirected_color, FTextBase::FSQ_UND);

					fsq_tx_text = new FTextTX(
									0, Y + fsq_rx_text->h(),
									fsq_left->w(), fsq_left->h() - fsq_rx_text->h());
					fsq_tx_text->color(
						fl_rgb_color(
							progdefaults.TxColor.R,
							progdefaults.TxColor.G,
							progdefaults.TxColor.B),
							progdefaults.RxTxSelectcolor);
					fsq_tx_text->setFont(progdefaults.TxFontnbr);
					fsq_tx_text->setFontSize(progdefaults.TxFontsize);
					fsq_tx_text->setFontColor(progdefaults.TxFontcolor, FTextBase::RECV);
					fsq_tx_text->setFontColor(progdefaults.XMITcolor, FTextBase::XMIT);
					fsq_tx_text->setFontColor(progdefaults.CTRLcolor, FTextBase::CTRL);
					fsq_tx_text->setFontColor(progdefaults.SKIPcolor, FTextBase::SKIP);
					fsq_tx_text->setFontColor(progdefaults.ALTRcolor, FTextBase::ALTR);
					fsq_tx_text->align(FL_ALIGN_CLIP);

					fsq_minbox = new Fl_Box(
							0, Y + minhtext,
							fsq_tx_text->w(),
							fsq_left->h() - 2 * minhtext);
					fsq_minbox->hide();

					fsq_left->resizable(fsq_minbox);
				fsq_left->end();

// right, heard list, special fsq controls, s/n indicator
				Fl_Group *fsq_right = new Fl_Group(
							fsq_left->w(), Y, 180, fsq_left->h());
					fsq_right->box(FL_FLAT_BOX);

				int bh = 20;
				int qh = bh + bh + 1 + 8 + image_s2n.h();

				static int heard_widths[] =
						{ 40*fsq_right->w()/100,
						  30*fsq_right->w()/100,
						  0 };
					fsq_heard = new Fl_Browser(
							fsq_right->x(), fsq_right->y(),
							fsq_right->w(), fsq_right->h() - qh);//minhtext);
					fsq_heard->column_widths(heard_widths);
					fsq_heard->column_char(',');
					fsq_heard->tooltip(_("Select FSQ station"));
					fsq_heard->callback((Fl_Callback*)cb_fsq_heard);
					fsq_heard->type(FL_MULTI_BROWSER);
					fsq_heard->box(FL_DOWN_BOX);
					fsq_heard->add("allcall");
					fsq_heard->labelfont(progdefaults.RxFontnbr);
					fsq_heard->labelsize(11);
#ifdef __APPLE__
					fsq_heard->textfont(FL_SCREEN_BOLD);
					fsq_heard->textsize(13);
#else
					fsq_heard->textfont(FL_HELVETICA);
					fsq_heard->textsize(13);
#endif

					int qw = fsq_right->w();

					int bw2 = qw / 2;
					int bw4 = qw / 4;

					fsq_lower_right = new Fl_Group(
							fsq_right->x(), fsq_heard->y() + fsq_heard->h(),
							qw, qh);
					fsq_lower_right->box(FL_FLAT_BOX);
					fsq_lower_right->color(FL_WHITE);

						int _yp = fsq_lower_right->y();
						int _xp = fsq_lower_right->x();

						btn_FSQCALL = new Fl_Light_Button(
							_xp, _yp, bw2, bh, "FSQ-ON");
						btn_FSQCALL->value(progdefaults.fsq_directed);
						btn_FSQCALL->selection_color(FL_DARK_GREEN);
						btn_FSQCALL->callback(cbFSQCALL, 0);
						btn_FSQCALL->tooltip("Left click - on/off");

						_xp += bw2;

						btn_SELCAL = new Fl_Light_Button(
							_xp, _yp, bw2, bh, "ACTIVE");
						btn_SELCAL->selection_color(FL_DARK_RED);
						btn_SELCAL->value(1);
						btn_SELCAL->callback(cbSELCAL, 0);
						btn_SELCAL->tooltip("Sleep / Active");

						_xp = fsq_lower_right->x();
						_yp += bh;

						btn_MONITOR = new Fl_Light_Button(
							_xp, _yp, bw4, bh, "MON");
						btn_MONITOR->selection_color(FL_DARK_GREEN);
						btn_MONITOR->value(progdefaults.fsq_show_monitor = false);
						btn_MONITOR->callback(cbMONITOR, 0);
						btn_MONITOR->tooltip("Monitor Open/Close");

						_xp += bw4;

						btn_FSQQTH = new Fl_Button(
							_xp, _yp, bw4, bh, "QTH");
						btn_FSQQTH->callback(cbFSQQTH, 0);
						btn_FSQQTH->tooltip("QTH->tx panel");

						_xp += bw4;

						btn_FSQQTC  = new Fl_Button(
							_xp, _yp, bw4, bh, "QTC");
						btn_FSQQTC->callback(cbFSQQTC, 0);
						btn_FSQQTC->tooltip("QTC->tx panel");

						_xp += bw4;

						btn_FSQCQ  = new Fl_Button(
							_xp, _yp, bw4, bh, "CQ");
						btn_FSQCQ->callback(cbFSQCQ, 0);
						btn_FSQCQ->tooltip("Xmt cqcqcq");

						_xp = fsq_lower_right->x();
						_yp += (bh + 1);

						ind_fsq_s2n = new Progress(
							_xp + 10, _yp, qw - 20, 8, "");
						ind_fsq_s2n->color(FL_WHITE, FL_DARK_GREEN);
						ind_fsq_s2n->type(Progress::HORIZONTAL);
						ind_fsq_s2n->value(40);

						_yp += 8;
						int th = fsq_tx_text->y() + fsq_tx_text->h();
						th = (th - _yp);

// Clear remainder of area if needed.
						if(th > image_s2n.h()) {
							Fl_Box *_nA = new Fl_Box(_xp, _yp, qw, th, "");
							_nA->box(FL_FLAT_BOX);
							_nA->color(FL_WHITE);
						}

// Add S/N rule
						Fl_Box *s2n = new Fl_Box(
							_xp + 10, _yp, qw - 20, image_s2n.h(), "");
						s2n->box(FL_FLAT_BOX);
						s2n->color(FL_WHITE);
						s2n->align(FL_ALIGN_INSIDE | FL_ALIGN_TOP | FL_ALIGN_CENTER | FL_ALIGN_CLIP);
						s2n->image(image_s2n);

					fsq_lower_right->end();

					fsq_right->resizable(fsq_heard);

				fsq_right->end();

			fsq_group->resizable(fsq_left);

		fsq_group->end();

		ifkp_group = new Fl_Group(0, Y, W, Htext);
			ifkp_group->box(FL_FLAT_BOX);
// upper, receive ifkp widgets
				ifkp_left = new Panel(
							0, Y,
							W - (image_s2n.w()+4), Htext);
// add rx & tx
				ifkp_rx_text = new FTextRX(
							0, Y,
							ifkp_left->w(), ifkp_group->h() / 2);
				ifkp_rx_text->color(
					fl_rgb_color(
						progdefaults.RxColor.R,
						progdefaults.RxColor.G,
						progdefaults.RxColor.B),
						progdefaults.RxTxSelectcolor);
				ifkp_rx_text->setFont(progdefaults.RxFontnbr);
				ifkp_rx_text->setFontSize(progdefaults.RxFontsize);
				ifkp_rx_text->setFontColor(progdefaults.RxFontcolor, FTextBase::RECV);
				ifkp_rx_text->setFontColor(progdefaults.XMITcolor, FTextBase::XMIT);
				ifkp_rx_text->setFontColor(progdefaults.CTRLcolor, FTextBase::CTRL);
				ifkp_rx_text->setFontColor(progdefaults.SKIPcolor, FTextBase::SKIP);
				ifkp_rx_text->setFontColor(progdefaults.ALTRcolor, FTextBase::ALTR);

				ifkp_tx_text = new FTextTX(
						0, Y + ifkp_rx_text->h(),
						ifkp_rx_text->w(), ifkp_group->h() - ifkp_rx_text->h());
				ifkp_tx_text->color(
					fl_rgb_color(
						progdefaults.TxColor.R,
						progdefaults.TxColor.G,
						progdefaults.TxColor.B),
						progdefaults.RxTxSelectcolor);
				ifkp_tx_text->setFont(progdefaults.TxFontnbr);
				ifkp_tx_text->setFontSize(progdefaults.TxFontsize);
				ifkp_tx_text->setFontColor(progdefaults.TxFontcolor, FTextBase::RECV);
				ifkp_tx_text->setFontColor(progdefaults.XMITcolor, FTextBase::XMIT);
				ifkp_tx_text->setFontColor(progdefaults.CTRLcolor, FTextBase::CTRL);
				ifkp_tx_text->setFontColor(progdefaults.SKIPcolor, FTextBase::SKIP);
				ifkp_tx_text->setFontColor(progdefaults.ALTRcolor, FTextBase::ALTR);
				ifkp_tx_text->align(FL_ALIGN_CLIP);

				ifkp_minbox = new Fl_Box(
						0, Y + minhtext,
						ifkp_tx_text->w(),
						ifkp_left->h() - 2 * minhtext);
				ifkp_minbox->hide();

				ifkp_left->resizable(ifkp_minbox);
			ifkp_left->end();

			ifkp_right = new Fl_Group(
							ifkp_left->w(), Y,
							image_s2n.w()+4, ifkp_group->h());
			ifkp_right->box(FL_FLAT_BOX);

			static int ifkp_heard_widths[] =
				{ 40*ifkp_right->w()/100,
				  30*ifkp_right->w()/100,
				  0 };
				ifkp_heard = new Fl_Browser(
								ifkp_right->x(), ifkp_right->y(),
								image_s2n.w()+4, ifkp_right->h() - (14 + image_s2n.h()));
				ifkp_heard->column_widths(ifkp_heard_widths);
				ifkp_heard->type(FL_MULTI_BROWSER);
				ifkp_heard->callback((Fl_Callback*)cb_ifkp_heard);
				ifkp_heard->column_char(',');
				ifkp_heard->tooltip(_("Stations Heard"));
				ifkp_heard->box(FL_DOWN_BOX);
				ifkp_heard->labelfont(progdefaults.RxFontnbr);
				ifkp_heard->labelsize(11);
#ifdef __APPLE__
				ifkp_heard->textfont(FL_SCREEN_BOLD);
				ifkp_heard->textsize(13);
#else
				ifkp_heard->textfont(FL_HELVETICA);
				ifkp_heard->textsize(13);
#endif
				Fl_Group *ifkp_sn_box = new Fl_Group(
					ifkp_heard->x(), ifkp_heard->y() + ifkp_heard->h(),
					ifkp_heard->w(), 14 + image_s2n.h(), "");
				ifkp_sn_box->box(FL_DOWN_BOX);

					ifkp_sn_box->color(FL_WHITE);
					ifkp_s2n_progress = new Progress(
						ifkp_sn_box->x() + 2, ifkp_sn_box->y() + 2,
						image_s2n.w(), 10, "");
					ifkp_s2n_progress->color(FL_WHITE, FL_DARK_GREEN);
					ifkp_s2n_progress->type(Progress::HORIZONTAL);
					ifkp_s2n_progress->value(40);

					Fl_Box *ifkp_s2n = new Fl_Box(
						ifkp_s2n_progress->x(), ifkp_s2n_progress->y() + ifkp_s2n_progress->h(),
						image_s2n.w(), image_s2n.h(), "");
					ifkp_s2n->box(FL_FLAT_BOX);
					ifkp_s2n->color(FL_WHITE);
					ifkp_s2n->align(FL_ALIGN_INSIDE | FL_ALIGN_TOP | FL_ALIGN_CENTER | FL_ALIGN_CLIP);
					ifkp_s2n->image(image_s2n);

					ifkp_sn_box->end();

				ifkp_right->end();
				ifkp_right->resizable(ifkp_heard);

			ifkp_right->end();

// lower, transmit ifkp widgets
			ifkp_group->resizable(ifkp_left);

		ifkp_group->end();

		fmt_group = fmt_panel(0, Y, W, Htext);
		fmt_group->end();

		center_group->end();

		text_group->show();
		wefax_group->hide();
		fsq_group->hide();
		ifkp_group->hide();
		fmt_group->hide();

}
{ // Bottom Macro group
		Y += center_group->h();

		Fl::add_handler(default_handler);

		macroFrame1 = new Fl_Group(0, Y, W, MACROBAR_MAX);
			macroFrame1->box(FL_FLAT_BOX);
			mf_group1 = new Fl_Group(0, Y, W - alt_btn_width, macroFrame1->h());
			Wmacrobtn = (mf_group1->w()) / NUMMACKEYS;
			wBLANK = (mf_group1->w() - NUMMACKEYS * Wmacrobtn) / 2;
			xpos = 0;
			ypos = mf_group1->y();
			for (int i = 0; i < NUMMACKEYS; i++) {
				if (i == 4 || i == 8) {
					bx = new Fl_Box(xpos, ypos, wBLANK, macroFrame1->h());
					bx->box(FL_FLAT_BOX);
					xpos += wBLANK;
				}
				btnMacro[i] = new Fl_Button(xpos, ypos, Wmacrobtn, macroFrame1->h(),
					macros.name[i].c_str());
				btnMacro[i]->callback(macro_cb, reinterpret_cast<void *>(i));
				btnMacro[i]->tooltip(_("Left Click - execute\nFkey - execute\nRight Click - edit"));
				xpos += Wmacrobtn;
			}
			mf_group1->end();
			btnAltMacros1 = new Fl_Button(
					W - alt_btn_width, ypos,
					alt_btn_width, macroFrame1->h(), "1");
			btnAltMacros1->callback(altmacro_cb, 0);
			btnAltMacros1->labelsize(progdefaults.MacroBtnFontsize);
			btnAltMacros1->tooltip(_("Primary macro set"));
			macroFrame1->resizable(mf_group1);
		macroFrame1->end();
		Y += Hmacros;
}
{ // Waterfall group
		wf_group = new Fl_Pack(0, Y, W, Hwfall);
			wf_group->type(1);

			wf = new waterfall(0, Y, Wwfall, Hwfall);
			wf->end();

			pgrsSquelch = new Progress(
				rightof(wf), Y,
				DEFAULT_SW, Hwfall,
				"");
			pgrsSquelch->color(FL_BACKGROUND2_COLOR, FL_DARK_GREEN);
			pgrsSquelch->type(Progress::VERTICAL);
			pgrsSquelch->tooltip(_("Detected signal level"));
				sldrSquelch = new Fl_Slider2(
				rightof(pgrsSquelch), Y,
				DEFAULT_SW, Hwfall,
				"");
			sldrSquelch->minimum(100);
			sldrSquelch->maximum(0);
			sldrSquelch->step(1);
			sldrSquelch->value(progStatus.sldrSquelchValue);
			sldrSquelch->callback((Fl_Callback*)cb_sldrSquelch);
			sldrSquelch->color(FL_INACTIVE_COLOR);
			sldrSquelch->tooltip(_("Squelch level"));
//				Fl_Group::current()->resizable(wf);
		wf_group->end();
		wf_group->resizable(wf);
}
{ // Status bar group
		Y += Hwfall;

		status_group = new Fl_Group(0, Y, W, Hstatus);

			MODEstatus = new Fl_Button(0, Y, Wmode, Hstatus, "");
			MODEstatus->box(FL_DOWN_BOX);
			MODEstatus->color(FL_BACKGROUND2_COLOR);
			MODEstatus->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			MODEstatus->callback(status_cb, (void *)0);
			MODEstatus->when(FL_WHEN_CHANGED);
			MODEstatus->tooltip(_("Left click: change mode\nRight click: configure"));

			cntCW_WPM = new Fl_Counter2(
				rightof(MODEstatus), Y,
				Ws2n - Hstatus, Hstatus, "");
			cntCW_WPM->callback(cb_cntCW_WPM);
			cntCW_WPM->minimum(progdefaults.CWlowerlimit);
			cntCW_WPM->maximum(progdefaults.CWupperlimit);
			cntCW_WPM->value(progdefaults.CWspeed);
			cntCW_WPM->type(1);
			cntCW_WPM->step(1);
			cntCW_WPM->tooltip(_("CW transmit WPM"));
			cntCW_WPM->hide();

			btnCW_Default = new Fl_Button(
				rightof(cntCW_WPM), Y,
				Hstatus, Hstatus, "*");
			btnCW_Default->callback(cb_btnCW_Default);
			btnCW_Default->tooltip(_("Default WPM"));
			btnCW_Default->hide();

			Status1 = new Fl_Box(
				rightof(MODEstatus), Y,
				Ws2n, Hstatus, "STATUS1");
			Status1->box(FL_DOWN_BOX);
			Status1->color(FL_BACKGROUND2_COLOR);
			Status1->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

			Status2 = new Fl_Box(
				rightof(Status1), Y,
				Wimd, Hstatus, "STATUS2");
			Status2->box(FL_DOWN_BOX);
			Status2->color(FL_BACKGROUND2_COLOR);
			Status2->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

			inpCall4 = new Fl_Input2(
				rightof(Status1), Y,
				Wimd, Hstatus, "");
			inpCall4->align(FL_ALIGN_LEFT);
			inpCall4->tooltip(_("Other call"));
			inpCall4->hide();

// see corner_box below
// corner_box used to leave room for OS X corner drag handle
#ifdef __APPLE__
	#define cbwidth DEFAULT_SW
#else
	#define cbwidth 0
#endif

			StatusBar = new status_box(
				rightof(Status2), Y,
				W - rightof(Status2)
				- bwAfcOnOff - bwSqlOnOff
				- Wwarn - bwTxLevel
				- bwSqlOnOff
				- cbwidth,
				Hstatus, "");
			StatusBar->box(FL_DOWN_BOX);
			StatusBar->color(FL_BACKGROUND2_COLOR);
			StatusBar->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			StatusBar->callback((Fl_Callback *)StatusBar_cb);
			StatusBar->when(FL_WHEN_RELEASE_ALWAYS);
			StatusBar->tooltip(_("Left click to toggle VuMeter"));

			VuMeter = new vumeter(StatusBar->x(), StatusBar->y(), StatusBar->w() - 2, StatusBar->h(), "");
			VuMeter->align(Fl_Align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE));
			VuMeter->when(FL_WHEN_RELEASE);
			VuMeter->callback((Fl_Callback *)VuMeter_cb);
			VuMeter->when(FL_WHEN_RELEASE_ALWAYS);
			VuMeter->tooltip(_("Left click to toggle Status Bar"));

			if (progStatus.vumeter_shown) {
				VuMeter->show();
				StatusBar->hide();
			} else {
				VuMeter->hide();
				StatusBar->show();
			}

			Fl_Box vuspacer(rightof(VuMeter),Y,2,Hstatus,"");
			vuspacer.box(FL_FLAT_BOX);

			cntTxLevel = new Fl_Counter2(
				rightof(&vuspacer), Y,
				bwTxLevel, Hstatus, "");
			cntTxLevel->minimum(-30);
			cntTxLevel->maximum(0);
			cntTxLevel->value(-6);
			cntTxLevel->callback((Fl_Callback*)cb_cntTxLevel);
			cntTxLevel->value(progdefaults.txlevel);
			cntTxLevel->lstep(1.0);
			cntTxLevel->tooltip(_("Tx level attenuator (dB)"));

			WARNstatus = new Fl_Box(
				rightof(cntTxLevel) + pad, Y,
				Wwarn, Hstatus, "");
			WARNstatus->box(FL_DIAMOND_DOWN_BOX);
			WARNstatus->color(FL_BACKGROUND_COLOR);
			WARNstatus->labelcolor(FL_RED);
			WARNstatus->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);

			btnAFC = new Fl_Light_Button(
				rightof(WARNstatus) + pad, Y,
				bwAfcOnOff, Hstatus, "AFC");

			btnSQL = new Fl_Light_Button(
				rightof(btnAFC), Y,
				bwSqlOnOff, Hstatus, "SQL");

// btnPSQL will be resized later depending on the state of the
// configuration parameter to show that widget

			btnPSQL = new Fl_Light_Button(
				rightof(btnSQL), Y,
				bwSqlOnOff, Hstatus, "PSM");

			btnSQL->selection_color(progdefaults.Sql1Color);

			btnAFC->callback(cbAFC, 0);
			btnAFC->value(1);
			btnAFC->tooltip(_("Automatic Frequency Control"));

			btnSQL->callback(cbSQL, 0);
			btnSQL->value(1);
			btnSQL->tooltip(_("Squelch"));

			btnPSQL->selection_color(progdefaults.Sql1Color);
			btnPSQL->value(progdefaults.kpsql_enabled);
			btnPSQL->callback(cbPwrSQL, 0);
			btnPSQL->tooltip(_("Power Signal Monitor"));

			corner_box = new Fl_Box(
				fl_digi_main->w() - cbwidth, Y,
				cbwidth, Hstatus, "");

			corner_box->box(FL_FLAT_BOX);

		status_group->end();
		status_group->resizable(VuMeter);

		Y += status_group->h();
}
{ // adjust callbacks
		showMacroSet();

		Fl_Widget* logfields[] = {
			inpCall1, inpCall2, inpCall3, inpCall4,
			inpLoc1, inp_vhf_Loc1, inp_vhf_Loc2,
			inpName1, inpName1,
			inpTimeOn1, inpTimeOn2, inpTimeOn3, inpTimeOff1, inpTimeOff2, inpTimeOff3,
			inpRstIn1, inpRstIn2, inpRstIn3, inpRstIn4, inpRstIn_AICW2,
			inpRstOut1, inpRstOut2, inpRstOut3, inpRstOut4, inpRstOut_AICW2,
			inpQth, inpVEprov,
			inpAZ, inpNotes,
			inpState1,
			inpSerNo1, inpSerNo2, outSerNo1, outSerNo2, outSerNo3,
			inp_SS_Check1, inp_SS_Precedence1, inp_SS_Section1, inp_SS_SerialNoR1,
			outSerNo4, inp_SS_Check2, inp_SS_Precedence2, inp_SS_Section2, inp_SS_SerialNoR2,
			inpXchgIn1, inpXchgIn2,
			inp_FD_class1, inp_FD_class2, inp_FD_section1, inp_FD_section2,
			inp_KD_age1, inp_KD_age2,
			inp_KD_state1, inp_KD_state2,
			inp_KD_VEprov1, inp_KD_VEprov2,
			inp_KD_XchgIn1, inp_KD_XchgIn2,
			inp_vhf_RSTin1, inp_vhf_RSTin2, inp_vhf_RSTout1, inp_vhf_RSTout2,
			inp_1010_XchgIn1, inp_1010_XchgIn2, inp_1010_name2, inp_1010_nr1, inp_1010_nr2,
			inp_ARR_Name2, inp_ARR_XchgIn1, inp_ARR_XchgIn2, inp_ARR_check1, inp_ARR_check2,
			inp_ASCR_RSTin2, inp_ASCR_RSTout2, inp_ASCR_XchgIn1, inp_ASCR_XchgIn2,
			inp_ASCR_class1, inp_ASCR_class2, inp_ASCR_name2,
			inpSPCnum_NAQP1, inpSPCnum_NAQP2,
			inpRTU_stpr1, inpRTU_stpr2,
			inpRTU_serno1, inpRTU_serno2,
			cboCountryRTU2,
			inpRTU_RSTin2,
			inp_IARI_PR1, inp_IARI_PR2,
			inp_IARI_RSTin2, inp_IARI_RSTout2,
			out_IARI_SerNo1, inp_IARI_SerNo1,
			out_IARI_SerNo2, inp_IARI_SerNo2,
			inp_IARI_PR2, cboCountryIARI2,
			inp_JOTA_scout1, inp_JOTA_scout2,
			inp_JOTA_spc1, inp_JOTA_spc2,
			inp_JOTA_troop1, inp_JOTA_troop2,
			inp_CQzone1,
			inpSPCnum_AICW1, inpSPCnum_AICW2,
			inpSQSO_state1, inpSQSO_state2,
			inpSQSO_county1, inpSQSO_county2,
			inpSQSO_serno1, inpSQSO_serno2,
			outSQSO_serno1, outSQSO_serno2,
			inpSQSO_name2,
			inpSQSO_category1, inpSQSO_category2
};
		for (size_t i = 0; i < sizeof(logfields)/sizeof(*logfields); i++) {
			logfields[i]->callback(cb_log);
			logfields[i]->when(FL_WHEN_CHANGED);//RELEASE || FL_WHEN_ENTER_KEY );//CHANGED);
		}

		Fl_ComboBox *country_fields[] = {
			cboCountryQSO, 
			cboCountryAICW2,
			cboCountryAIDX2,
			cboCountryCQ2,
			cboCountryCQDX2,
			cboCountryIARI2,
			cboCountryRTU2// ,
//			cboCountryWAE2
		};
		for (size_t i = 0; i < sizeof(country_fields)/sizeof(*country_fields); i++) {
			country_fields[i]->callback(cb_country);
			country_fields[i]->when(FL_WHEN_CHANGED);
		}

		// exceptions
		inpCall1->callback(cb_call);
		inpCall1->when(FL_WHEN_CHANGED | FL_WHEN_ENTER_KEY_ALWAYS);
		inpCall2->callback(cb_call);
		inpCall2->when(FL_WHEN_CHANGED | FL_WHEN_ENTER_KEY_ALWAYS);
		inpCall3->callback(cb_call);
		inpCall3->when(FL_WHEN_CHANGED | FL_WHEN_ENTER_KEY_ALWAYS);
		inpCall4->callback(cb_call);
		inpCall4->when(FL_WHEN_CHANGED | FL_WHEN_ENTER_KEY_ALWAYS);

		inpNotes->when(FL_WHEN_RELEASE);

}

	fl_digi_main->end();

	fl_digi_main->callback(cb_wMain);
	fl_digi_main->resizable(center_group);

{ // scope view dialog
	scopeview = new Fl_Double_Window(0,0,140,140, _("Scope"));
	scopeview->xclass(PACKAGE_NAME);
	digiscope = new Digiscope (0, 0, 140, 140);
	scopeview->resizable(digiscope);
	scopeview->size_range(SCOPEWIN_MIN_WIDTH, SCOPEWIN_MIN_HEIGHT);
	scopeview->end();
	scopeview->hide();
}

{ // field day viewer dialog
	field_day_viewer = make_fd_view();
	field_day_viewer->hide();
}

{ // adjust menu toggle items
	if (!progdefaults.menuicons)
		icons::toggle_icon_labels();

	// Set the state of checked toggle menu items. Never changes.
	const struct {
		bool var; const char* label;
	} toggles[] = {
		{ progStatus.LOGenabled, LOG_TO_FILE_MLABEL },
		{ progStatus.WF_UI, WF_MLABEL },
		{ progStatus.Rig_Log_UI, RIGLOG_PARTIAL_MLABEL },
		{ !progStatus.Rig_Log_UI, RIGLOG_FULL_MLABEL },
		{ progStatus.NO_RIGLOG, RIGLOG_NONE_MLABEL },
		{ progStatus.DOCKEDSCOPE, DOCKEDSCOPE_MLABEL }
	};
	Fl_Menu_Item* toggle;
	for (size_t i = 0; i < sizeof(toggles)/sizeof(*toggles); i++) {
		if (toggles[i].var && (toggle = getMenuItem(toggles[i].label))) {
			toggle->set();
			if (toggle->callback()) {
				mnu->value(toggle);
				toggle->do_callback(reinterpret_cast<Fl_Widget*>(mnu));
			}
		}
	}
}

	if (!dxcc_is_open())
		getMenuItem(COUNTRIES_MLABEL)->hide();

	toggle_smeter();

	adjust_for_contest(0);

	UI_select();

	wf->UI_select(progStatus.WF_UI);

	LOGGING_colors_font();

	init_country_fields();

	clearQSO();

	fsqMonitor = create_fsqMonitor();

	createConfig();

	createRecordLoader();

	switch (progdefaults.mbar_scheme) {
		case 0: btn_scheme_0->setonly(); break;
		case 1: btn_scheme_1->setonly(); break;
		case 2: btn_scheme_2->setonly(); break;
		case 3: btn_scheme_3->setonly(); break;
		case 4: btn_scheme_4->setonly(); break;
		case 5: btn_scheme_5->setonly(); break;
		case 6: btn_scheme_6->setonly(); break;
		case 7: btn_scheme_7->setonly(); break;
		case 8: btn_scheme_8->setonly(); break;
		case 9: btn_scheme_9->setonly(); break;
		case 10: btn_scheme_10->setonly(); break;
		case 11: btn_scheme_11->setonly(); break;
		case 12: btn_scheme_12->setonly(); break;
	}

	colorize_macros();

	if (rx_only) {
		btnTune->deactivate();
		wf->xmtrcv->deactivate();
	}

	set_mode_controls(active_modem->get_mode());

	create_wefax_tx_viewer(0, 0, 800, 400 );

}
