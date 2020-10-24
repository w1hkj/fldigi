// ----------------------------------------------------------------------------
//      doxygen.h
//
// Copyright (C) 2013-2020
//              John Phelps, KL4YFD
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

/*
	This include file is not indended to be included by any source file in Fldigi !
	
	It is used only for DOXYGEN automatic-documentation generation.
	Used for Fldigi file: /scripts/srcdoc/gen_doxygen_srcdoc.sh 
	Please put only comments & Doxygen info in this file.
*/


/** \mainpage Fldigi Sourcecode Interactive Doxygen Documentation : 000GIT_REVISION000
 <div align="center"><img src="fldigi-psk.png" ></div>

 \section intro Introduction

  Welcome to the Fldigi Interactive Doxygen Sourcecode Documentation!
  Here you'll find information useful for developing and debugging fldigi and flarq. 
  
  fldigi - Digital modem program for Linux, Free-BSD, OS X / MacOS, Windows 2000 and newer
  
  flarq - Automatic Repeat reQuest program.

  Fldigi and Flarq are separate programs that are packaged together as source, binary, and installation files.



  <BR>
  \section download To Download Fldigi :
  <UL> 
  <LI> Latest release version at: <A HREF="https://sourceforge.net/projects/fldigi/files/fldigi/" target="_blank">https://sourceforge.net/projects/fldigi/files/fldigi/</A></LI>
  <LI> Alpha versions at: <A HREF="https://sourceforge.net/projects/fldigi/files/alpha_tests/" target="_blank">https://sourceforge.net/projects/fldigi/files/alpha_tests/</A></LI>
  <LI> Test suite available at: <A HREF="https://sourceforge.net/projects/fldigi/files/test_suite/" target="_blank">https://sourceforge.net/projects/fldigi/files/test_suite/</A></LI>
  
  <LI> To pull latest source with developer access: <B> git clone ssh://<YOUR-LOGIN>\@git.code.sf.net/p/fldigi/fldigi fldigi-fldigi</B></LI>
  <LI> To pull latest source with developer access behind proxy/firewall: <B> git clone https://<YOUR-LOGIN>\@git.code.sf.net/p/fldigi/fldigi fldigi-fldigi</B></LI>
  <LI> To pull latest source with no account, read-only: <B> git clone git://git.code.sf.net/p/fldigi/fldigi fldigi-fldigi</B></LI>
  </UL>
  
  
  <BR>
  \section generate Generating This Documentation :
  <OL>
  <LI>Download the Fldigi sourcecode as described above</LI>
  <LI>Unpack the sourcecode to a Linux / Unix / MacOS environment (Debian or Ubuntu preferred)</LI>
  <LI>Checkout the desired branch or release using the command - <B>git checkout</B></LI>
  <LI>Open a terminal and go to sourcecode sub-directory - <B>scripts/srcdoc</B></LI>
  <LI>In this directory, run command - <B>./gen_doxygen_srcdoc.sh run</B></LI>
    <OL>
    <LI type="a">If any needed binaires are missing, you will be notified.</LI>
    <LI type="a">On Debian / Ubuntu binaries can be installed by command <B>./gen_doxygen_srcdoc.sh install</B></LI>
    <LI type="a">On other systems, the following binaries must be installed manually:</LI>
        <UL>
        <LI>cppcheck dot doxygen mscgen gitstats</LI>
        </UL>
    </OL>
  </OL>
  
  
  <BR>
  \section entry Documentation Entry-Points :
  <UL>
  <LI><A HREF="main_8cxx_a3c04138a5bfe5d72780bb7e82a18e627_cgraph_org.svg" target="_blank"> Call Graph for %main() </A></LI>
  <LI><A HREF="annotated.html">List of all Classes with descriptions</A></LI>
  <LI><A HREF="classes.html">Index of all the classes in Fldigi</A></LI>
  <LI><A HREF="classmodem__inherit__graph.svg">The modem:: class inheritance graph</A></LI>
  <LI><A HREF="globals.html">list of all functions, variables, defines, enums, and typedefs with links to the files they belong to</A></LI>
  </UL>

  
  <BR>  
  \section core Core Files Documentation :
  <TABLE>
	<TR>
		<TH style="text-align:center"> </TH>
		<TH style="text-align:center">  Core  </TH>
		<TH style="text-align:center">  File Reference  </TH>
		<TH style="text-align:center">  C++ Source Code  </TH>
		<TH style="text-align:center">  Header Reference  </TH>
		<TH style="text-align:center">  Header Source Code  </TH>
	</TR>
	<TR>
		<TH style="text-align:center"> %main() </TH>
		<TD style="text-align:center"><A HREF="main_8cxx.html#a3c04138a5bfe5d72780bb7e82a18e627"> %main() </A></TD>
		<TD style="text-align:center"><A HREF="main_8cxx.html"> ( main.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="main_8cxx_source.html"> main.cxx </A></TD>
		<TD style="text-align:center"><A HREF="main_8h.html"> ( main.h ) </A></TD>
		<TD style="text-align:center"><A HREF="main_8h_source.html"> main.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> FLTK Window </TH>
		<TD style="text-align:center">  create_fl_digi_main()  </TD>
		<TD style="text-align:center"><A HREF="fl__digi_8cxx.html"> ( fl_digi.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="fl__digi_8cxx_source.html"> fl_digi.cxx </A></TD>
		<TD style="text-align:center"><A HREF="fl__digi_8h.html"> ( fl_digi.h ) </A></TD>
		<TD style="text-align:center"><A HREF="fl__digi_8h_source.html"> fl_digi.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> FLTK Window Backend </TH>
		<TD style="text-align:center">  create_fl_digi_main_primary()  </TD>
		<TD style="text-align:center"><A HREF="fl__digi__main_8cxx.html"> ( fl_digi_main.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="fl__digi__main_8cxx_source.html"> fl_digi_main.cxx </A></TD>
		<TD style="text-align:center"><A HREF="fl__digi__main_8h.html"> ( fl_digi_main.h ) </A></TD>
		<TD style="text-align:center"><A HREF="fl__digi__main_8h_source.html"> fl_digi_main.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> Transmit / Receive Loop </TH>
		<TD style="text-align:center"> trx_loop() </TD>
		<TD style="text-align:center"><A HREF="trx_8cxx.html"> ( trx.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="trx_8cxx_source.html"> trx.cxx </A></TD>
		<TD style="text-align:center"><A HREF="trx_8h.html"> ( trx.h ) </A></TD>
		<TD style="text-align:center"><A HREF="trx_8h_source.html"> trx.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> Modem Base Class </TH>
		<TD style="text-align:center"><A HREF="classmodem.html"> modem:: </A></TD>
		<TD style="text-align:center"><A HREF="modem_8cxx.html"> ( modem.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="modem_8cxx_source.html"> modem.cxx </A></TD>
		<TD style="text-align:center"><A HREF="modem_8h.html"> ( modem.h ) </A></TD>
		<TD style="text-align:center"><A HREF="modem_8h_source.html"> modem.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> Waterfall </TH>
		<TD style="text-align:center"><A HREF="classwaterfall.html"> waterfall:: </A></TD>
		<TD style="text-align:center"><A HREF="waterfall_8cxx.html"> ( waterfall.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="waterfall_8cxx_source.html"> waterfall.cxx </A></TD>
		<TD style="text-align:center"><A HREF="waterfall_8h.html"> ( waterfall.h ) </A></TD>
		<TD style="text-align:center"><A HREF="waterfall_8h_source.html"> waterfall.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> Globals </TH>
		<TD style="text-align:center"> N/A </TD>
		<TD style="text-align:center"><A HREF="globals_8cxx.html"> ( globals.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="globals_8cxx_source.html"> globals.cxx </A></TD>
		<TD style="text-align:center"><A HREF="globals_8h.html"> ( globals.h ) </A></TD>
		<TD style="text-align:center"><A HREF="globals_8h_source.html"> globals.h </A></TD>
	</TR>
  </TABLE>
  
  
  <BR>
  \section modems Modems Documentation :
  <TABLE>
	<TR>
		<TH style="text-align:center"> </TH>
		<TH style="text-align:center">  Class Reference  </TH>
		<TH style="text-align:center">  File Reference  </TH>
		<TH style="text-align:center">  C++ Source Code  </TH>
		<TH style="text-align:center">  Header Reference  </TH>
		<TH style="text-align:center">  Header Source Code  </TH>
	</TR>
		<TR>
		<TH style="text-align:center"> %BLANK </TH>
		<TD style="text-align:center"><A HREF="classBLANK.html"> BLANK:: </A></TD>
		<TD style="text-align:center"><A HREF="blank_8cxx.html">  ( blank.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="blank_8cxx_source.html"> blank.cxx </A></TD>
		<TD style="text-align:center"><A HREF="blank_8h.html"> ( blank.h ) </A></TD>
		<TD style="text-align:center"><A HREF="blank_8h_source.html"> blank.h </A></TD>
	</TR>	<TR>
		<TH style="text-align:center"> CW </TH>
		<TD style="text-align:center"><A HREF="classcw.html"> cw:: </A></TD>
		<TD style="text-align:center"><A HREF="cw_8cxx.html">  ( cw.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="cw_8cxx_source.html"> cw.cxx </A></TD>
		<TD style="text-align:center"><A HREF="cw_8h.html"> ( cw.h ) </A></TD>
		<TD style="text-align:center"><A HREF="cw_8h_source.html"> cw.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> Contestia </TH>
		<TD style="text-align:center"><A HREF="classcontestia.html"> contestia:: </A></TD>
		<TD style="text-align:center"><A HREF="contest_8cxx.html"> ( contestia.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="contest_8cxx_source.html"> contestia.cxx </A></TD>
		<TD style="text-align:center"><A HREF="contest_8h.html"> ( contestia.h ) </A></TD>
		<TD style="text-align:center"><A HREF="contest_8h_source.html"> contestia.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> DominoEX </TH>
		<TD style="text-align:center"><A HREF="classdominoex.html"> dominoex:: </A></TD>
		<TD style="text-align:center"><A HREF="dominoex_8cxx.html"> ( dominoex.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="dominoex_8cxx_source.html">  dominoex.cxx </A></TD>
		<TD style="text-align:center"><A HREF="dominoex_8h.html"> ( dominoex.h ) </A></TD>
		<TD style="text-align:center"><A HREF="dominoex_8h_source.html">  dominoex.h </A></TD>
	</TR>
		<TR>
		<TH style="text-align:center"> DTMF </TH>
		<TD style="text-align:center"><A HREF="classcDTMF.html"> cDTMF:: </A></TD>
		<TD style="text-align:center"><A HREF="dtmf_8cxx.html"> ( dtmf.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="dtmf_8cxx_source.html">  dtmf.cxx </A></TD>
		<TD style="text-align:center"><A HREF="dtmf_8h.html"> ( dtmf.h ) </A></TD>
		<TD style="text-align:center"><A HREF="dtmf_8h_source.html">  dtmf.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> FSQ </TH>
		<TD style="text-align:center"><A HREF="classfsq.html"> fsq:: </A></TD>
		<TD style="text-align:center"><A HREF="fsq_8cxx.html"> ( fsq.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="fsq_8cxx_source.html"> fsq.cxx </A></TD>
		<TD style="text-align:center"><A HREF="fsq_8h.html"> ( fsq.h ) </A></TD>
		<TD style="text-align:center"><A HREF="fsq_8h_source.html"> fsq.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> Feld Hell </TH>
		<TD style="text-align:center"><A HREF="classfeld.html"> feld:: </A></TD>
		<TD style="text-align:center"><A HREF="feld_8cxx.html"> ( feld.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="feld_8cxx_source.html">  feld.cxx </A></TD>
		<TD style="text-align:center"><A HREF="feld_8h.html"> ( feld.h ) </A></TD>
		<TD style="text-align:center"><A HREF="feld_8h_source.html"> feld.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> IFKP </TH>
		<TD style="text-align:center"><A HREF="classifkp.html"> ifkp:: </A></TD>
		<TD style="text-align:center"><A HREF="ifkp_8cxx.html"> ( ifkp.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="ifkp_8cxx_source.html"> ifkp.cxx </A></TD>
		<TD style="text-align:center"><A HREF="ifkp_8h.html"> ( ifkp.h ) </A></TD>
		<TD style="text-align:center"><A HREF="ifkp_8h_source.html"> ifkp.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> MFSK </TH>
		<TD style="text-align:center"><A HREF="classmfsk.html"> mfsk:: </A></TD>
		<TD style="text-align:center"><A HREF="mfsk_8cxx.html"> ( mfsk.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="mfsk_8cxx_source.html"> mfsk.cxx </A></TD>
		<TD style="text-align:center"><A HREF="mfsk_8h.html"> ( mfsk.h ) </A></TD>
		<TD style="text-align:center"><A HREF="mfsk_8h_source.html"> mfsk.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> MT63 </TH>
		<TD style="text-align:center"><A HREF="classmt63.html"> mt63:: </A></TD>
		<TD style="text-align:center"><A HREF="mt63_8cxx.html"> ( mt63.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="mt63_8cxx_source.html"> mt63.cxx  </A></TD>
		<TD style="text-align:center"><A HREF="mt63_8h.html"> ( mt63.h ) </A></TD>
		<TD style="text-align:center"><A HREF="mt63_8h_source.html"> mt63.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> Olivia </TH>
		<TD style="text-align:center"><A HREF="classolivia.html"> olivia:: </A></TD>
		<TD style="text-align:center"><A HREF="olivia_8cxx.html"> ( olivia.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="olivia_8cxx_source.html"> olivia.cxx </A></TD>
		<TD style="text-align:center"><A HREF="olivia_8h.html"> ( olivia.h ) </A></TD>
		<TD style="text-align:center"><A HREF="olivia_8h_source.html"> olivia.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> PSK </TH>
		<TD style="text-align:center"><A HREF="classpsk.html"> psk:: </A></TD>
		<TD style="text-align:center"><A HREF="psk_8cxx.html"> ( psk.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="psk_8cxx_source.html"> psk.cxx </A></TD>
		<TD style="text-align:center"><A HREF="psk_8h.html"> ( psk.h ) </A></TD>
		<TD style="text-align:center"><A HREF="psk_8h_source.html"> psk.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> QPSK </TH>
		<TD style="text-align:center"><A HREF="classpsk.html"> psk:: </A></TD>
		<TD style="text-align:center"><A HREF="psk_8cxx.html"> ( psk.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="psk_8cxx_source.html"> psk.cxx </A></TD>
		<TD style="text-align:center"><A HREF="psk_8h.html"> ( psk.h ) </A></TD>
		<TD style="text-align:center"><A HREF="psk_8h_source.html"> psk.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> 8PSK </TH>
		<TD style="text-align:center"><A HREF="classpsk.html"> psk:: </A></TD>
		<TD style="text-align:center"><A HREF="psk_8cxx.html"> ( psk.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="psk_8cxx_source.html"> psk.cxx </A></TD>
		<TD style="text-align:center"><A HREF="psk_8h.html"> ( psk.h ) </A></TD>
		<TD style="text-align:center"><A HREF="psk_8h_source.html"> psk.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> PSKR </TH>
		<TD style="text-align:center"><A HREF="classpsk.html"> psk:: </A></TD>
		<TD style="text-align:center"><A HREF="psk_8cxx.html"> ( psk.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="psk_8cxx_source.html"> psk.cxx </A></TD>
		<TD style="text-align:center"><A HREF="psk_8h.html"> ( psk.h ) </A></TD>
		<TD style="text-align:center"><A HREF="psk_8h_source.html"> psk.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> RTTY </TH>
		<TD style="text-align:center"><A HREF="classrtty.html"> rtty:: </A></TD>
		<TD style="text-align:center"><A HREF="rtty_8cxx.html"> ( rtty.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="rtty_8cxx_source.html">  rtty.cxx </A></TD>
		<TD style="text-align:center"><A HREF="rtty_8h.html"> ( rtty.h ) </A></TD>
		<TD style="text-align:center"><A HREF="rtty_8h_source.html"> rtty.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> THOR </TH>	
		<TD style="text-align:center"><A HREF="classthor.html"> thor:: </A></TD>
		<TD style="text-align:center"><A HREF="thor_8cxx.html"> ( thor.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="thor_8cxx_source.html"> thor.cxx </A></TD>
		<TD style="text-align:center"><A HREF="thor_8h.html"> ( thor.h ) </A></TD>
		<TD style="text-align:center"><A HREF="thor_8h_source.html"> thor.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> THORB </TH>
		<TD style="text-align:center"><A HREF="classthrob.html"> throb:: </A></TD>
		<TD style="text-align:center"><A HREF="throb_8cxx.html"> ( throb.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="throb_8cxx_source.html"> throb.cxx </A></TD>
		<TD style="text-align:center"><A HREF="throb_8h.html"> ( throb.h ) </A></TD>
		<TD style="text-align:center"><A HREF="throb_8h_source.html"> throb.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> WEFAX </TH>
		<TD style="text-align:center"><A HREF="classwefax.html"> wefax:: </A></TD>
		<TD style="text-align:center"><A HREF="wefax_8cxx.html"> ( wefax.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="wefax_8cxx_source.html"> wefax.cxx  </A></TD>
		<TD style="text-align:center"><A HREF="wefax_8h.html"> ( wefax.h ) </A></TD>
		<TD style="text-align:center"><A HREF="wefax_8h_source.html">  wefax.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> NAVTEX </TH>
		<TD style="text-align:center"><A HREF="classnavtex.html"> navtex:: </A></TD>
		<TD style="text-align:center"><A HREF="navtex_8cxx.html"> ( navtex.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="navtex_8cxx_source.html"> navtex.cxx </A></TD>
		<TD style="text-align:center"><A HREF="navtex_8h.html"> ( navtex.h ) </A></TD>
		<TD style="text-align:center"><A HREF="navtex_8h_source.html"> navtex.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> SITORB </TH>
		<TD style="text-align:center"><A HREF="classnavtex.html"> navtex::  </A></TD>
		<TD style="text-align:center"><A HREF="navtex_8cxx.html"> ( navtex.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="navtex_8cxx_source.html">  navtex.cxx </A></TD>
		<TD style="text-align:center"><A HREF="navtex_8h.html"> ( navtex.h ) </A></TD>
		<TD style="text-align:center"><A HREF="navtex_8h_source.html">  navtex.h </A></TD>
	</TR>
	<TR>
		<TH style="text-align:center"> WWV </TH>
		<TD style="text-align:center"><A HREF="classwwv.html"> wwv:: </A></TD>
		<TD style="text-align:center"><A HREF="wwv_8cxx.html"> ( wwv.cxx ) </A></TD>
		<TD style="text-align:center"><A HREF="wwv_8cxx_source.html"> wwv.cxx </A></TD>
		<TD style="text-align:center"><A HREF="wwv_8h.html"> ( wwv.h ) </A></TD>
		<TD style="text-align:center"><A HREF="wwv_8h_source.html"> wwv.h </A></TD>
		</TR>
  </TABLE>
  

  <BR>
  \section cppcheck_results CppCheck Static Program Analysis Results :
  
  <UL>
  <LI><B><A HREF="results/TOTALS.txt">TOTALS</A></B></LI>
  <LI><A HREF="results/error.txt">Errors</A></LI>
  <LI><A HREF="results/warning.txt">Warnings</A></LI>
  <LI><A HREF="results/style.txt">Code-Style Issues</A></LI>
  <LI><A HREF="results/performance.txt">Performance Issues</A></LI>
  <LI><A HREF="results/portability.txt">Portability Issues</A></LI>
  <LI><A HREF="results/information.txt">Information</A></LI>
  <LI><B>Inconclusive tests did not FAIL, but also did not PASS</B></LI>
  <LI><A HREF="results/error_inconclusive.txt"><I>Inconclusive Errors</I></A></LI>
  <LI><A HREF="results/warning_inconclusive.txt"><I>Inconclusive Warnings</I></A></LI>
  <LI><A HREF="results/style_inconclusive.txt"><I>Inconclusive Code-Style Issues</I></A></LI>
  <LI><A HREF="results/performance_inconclusive.txt"><I>Inconclusive Performance Issues</I></A></LI>
  <LI><A HREF="results/portability_inconclusive.txt"><I>Inconclusive Portability Issues</I></A></LI>
  <LI><A HREF="results/information_inconclusive.txt"><I>Inconclusive Information</I></A></LI>
  </UL>

  
  <BR>
  \section git Patches for the Previous 100 Git Commits :
  <UL>
  <LI><A HREF="__git/">Patches for the last 100 commits</A></LI>
  <LI><A HREF="__git/gitlog.txt">git-log for the last 100 commits</A></LI>
  </UL>


  <BR>
  \section license License

 	fldigi is free software; you can redistribute it and/or modify
 	it under the terms of the GNU General Public License as published by
 	the Free Software Foundation; either version 3 of the License, or
 	(at your option) any later version.

 	fldigi is distributed in the hope that it will be useful,
 	but WITHOUT ANY WARRANTY; without even the implied warranty of
 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 	GNU General Public License for more details.

 	You should have received a copy of the GNU General Public License
 	along with this program.  If not, see http://www.gnu.org/licenses

 
*/
