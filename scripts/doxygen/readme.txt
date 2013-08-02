scripts/doxygen/readme.txt

This directory contains the run-script: gen_doxygen_docs.sh and 
configuration file: fldigi_doxyfile.txt used for auto-generating 
DOXYGEN source-code documentation.

This documentation can be extremely useful for both learning and debugging fldigi/flarq.


== Usage ==

1) Execute: ' ./gen_doxygen_docs.sh run ' on a Unix-Like system. (OSX, Linux, etc...)
	- the script will complain usefully if anything is missing
	- NOTE: this will generate 1.8GiB of data and take longer than compilation.

2) Once generation completes, a web-browser will automatically open the file:  HTML/index.html
	- URL Example: file:///tmp/fldigi/scripts/doxygen/HTML/index.html
