// ----------------------------------------------------------------------------
//      doxygen.h
//
// Copyright (C) 2013-2015
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
	This include file is not indended to be included by any source file in Fldigi,
	but is used only for DOXYGEN automatic-documentation generation. 
	Please put only comments & Doxygen info in this file.
*/


/** \mainpage Fldigi Developer Doxygen Documentation
 <div align="center"><img src="../../../data/fldigi-psk.png" ></div>

 \section intro Introduction

  Welcome to the Fldigi doxygen documentation.
  Here you'll find information useful for developing and debugging fldigi and flarq. 
  
  fldigi - Digital modem program for Linux, Free-BSD, OS X, Windows XP, NT, W2K, Vista and Win7.
  
  flarq - Automatic Repeat reQuest program.

  Fldigi and Flarq are separate programs that are packaged together as source, binary, and installation files.




  Build updated to fltk-1.3.0 library standards.  Can also be built using fltk-1.1.10
  
  \section download To Download Fldigi :
  <UL> 
  <LI> Latest release version at: <A HREF="https://sourceforge.net/projects/fldigi/files/fldigi/">https://sourceforge.net/projects/fldigi/files/fldigi/</A></LI>
  <LI> Alpha versions at: <A HREF="https://sourceforge.net/projects/fldigi/files/alpha_tests/">https://sourceforge.net/projects/fldigi/files/alpha_tests/</A></LI>
  <LI> Test suite available at: <A HREF="https://sourceforge.net/projects/fldigi/files/test_suite/">https://sourceforge.net/projects/fldigi/files/test_suite/</A></LI>
  <LI> To pull latest source with developer access: <B> git clone ssh://<I>YOUR-LOGIN</I>@git.code.sf.net/p/fldigi/fldigi fldigi-fldigi</B></LI>
  <LI> To pull latest source with developer access behind proxy/firewall: <B> git clone https://<I>YOUR-LOGIN</I>@git.code.sf.net/p/fldigi/fldigi fldigi-fldigi</B></LI>
  <LI> To pull latest source with no account, read-only: <B> git clone git://git.code.sf.net/p/fldigi/fldigi fldigi-fldigi</B></LI>
  </UL>
 
  
  \section entry Good Entry-Points for Navigating the Doxygen Documentation :
  <UL>
  <LI><A HREF="annotated.html">List of all Classes and Data Structures</A></LI>
  <LI><A HREF="classmodem.html">The modem:: class documentation</A></LI>
  <LI><A HREF="classmodem__inherit__graph.png">The modem:: class inheritange graph</A></LI>
  <LI><A HREF="globals.html">list of all functions, variables, defines, enums, and typedefs with links to the files they belong to</A></LI>
  </UL>
  
  
  \section cppcheck_results Results from the cppcheck static program analysis tool:
  Analysis was ran during doxygen documentation generation.
  <UL>
  <LI><A HREF="../../tests/cppcheck/results/error.txt">Errors</A></LI>
  <LI><A HREF="../../tests/cppcheck/results/warning.txt">Warnings</A></LI>
  <LI><A HREF="../../tests/cppcheck/results/style.txt">Code-Style Issues</A></LI>
  <LI><A HREF="../../tests/cppcheck/results/performance.txt">Performance Issues</A></LI>
  <LI><A HREF="../../tests/cppcheck/results/portability.txt">Portability Issues</A></LI>
  <LI><A HREF="../../tests/cppcheck/results/information.txt">Information</A></LI>
  <LI><B>Inconclusive tests did not FAIL, but also did not PASS</B></LI>
  <LI><A HREF="../../tests/cppcheck/results/error_inconclusive.txt"><I>Inconclusive Errors</I></A></LI>
  <LI><A HREF="../../tests/cppcheck/results/warning_inconclusive.txt"><I>Inconclusive Warnings</I></A></LI>
  <LI><A HREF="../../tests/cppcheck/results/style_inconclusive.txt"><I>Inconclusive Code-Style Issues</I></A></LI>
  <LI><A HREF="../../tests/cppcheck/results/performance_inconclusive.txt"><I>Inconclusive Performance Issues</I></A></LI>
  <LI><A HREF="../../tests/cppcheck/results/portability_inconclusive.txt"><I>Inconclusive Portability Issues</I></A></LI>
  <LI><A HREF="../../tests/cppcheck/results/information_inconclusive.txt"><I>Inconclusive Information</I></A></LI>
  </UL>
  
  \section git Information from git :
  <UL>
  <LI><A HREF="__git/">Patches for the last 100 commits</A></LI>
  <LI><A HREF="__git/gitlog.txt">git-log for the last 100 commits</A></LI>
  <LI><A HREF="GITSTATS/index.html">General GIT Statistics for the project</A></LI>
  <LI><A HREF="GITSTATS/activity.html">Project Activity Log</A></LI>
  <LI><A HREF="GITSTATS/authors.html">Author Statistics</A></LI>
  <LI><A HREF="GITSTATS/files.html">File Statistics</A></LI>
  <LI><A HREF="GITSTATS/lines.html">Lines of Code Graph</A></LI>
  <LI><A HREF="GITSTATS/tags.html">Tags List</A></LI>
  </UL>

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