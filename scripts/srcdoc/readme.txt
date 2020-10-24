/scripts/srcdoc/readme.txt

This directory contains the script: gen_doxygen_srcdoc.sh
and the configuration file: fldigi_doxyfile.txt
used for auto-generating interactive HTML DOXYGEN source-code documentation.

Unless disabled, the tool CPPcheck is also ran.

This documentation can be extremely useful for 
both learning and debugging fldigi/flarq.


== Preparation ==

 * Install the programs: 
	- cppcheck, graphviz, doxygen, mscgen
	
 * On a Debian based system (Ubuntu, Mint, etc) install required binaries by running:
	-  ./gen_doxygen_srcdoc.sh install

	
== Generating the Sourcecode Documentation ==

 * Execute:
	-  ./gen_doxygen_srcdoc.sh run 
	( the script will complain usefully if any needed binaries are missing )
	NOTE: this will generate over 700MB of data and take much longer than compilation!

 * Once generation completes, the source documentation can be accessed at:  
	- HTML/index.html

	
== Generate Source Documentation Without CppCheck ==

 * Execute:
	-  ./gen_doxygen_srcdoc.sh run nocppcheck

 
== Cleanup ==
 
 * Execute:
	-  ./gen_doxygen_srcdoc.sh clean

