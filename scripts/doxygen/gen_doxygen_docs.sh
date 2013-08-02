#! /bin/bash -e
#
# KL4YFD 2013
# Released under GNU GPL
#

# This script generates DOXYGEN documentation from the fldigi source tree
# Checks are done to ensure needed binaries and files exists first

cd $( dirname ${BASH_SOURCE[0]} )

function usage
{
	printf "\n\nThis script generates Doxygen documentation from the "
	printf "\nfldigi sourcecode. By default, the tool \"cppcheck\" is also called. \nNote: This analysis takes longer than compilation and produces about 1.8GiB of data."
	printf "\n\nUsage:"
	printf "\n\tGenerate Doxygen documentation:\t ./gen_doxygen_docs.sh run"
	printf "\n\tClean up after Doxygen run:\t ./gen_doxygen_docs.sh clean"
	printf "\n\tClean up after Doxygen run:\t ./gen_doxygen_docs.sh nocppcheck"
	printf "\n\tPrint this usage summary:\t ./gen_doxygen_docs.sh help \n\n"
}

function doxygen_clean {
	rm -Rf HTML
	printf "\ndoxygen documentation deleted!\n"
}


case "$1" in
"run")
  doxygen_clean
  ../tests/cppcheck/gen_cppcheck_results.sh clean
  # Continue with rest of script...
  break
    ;;
"nocppcheck")
   cppcheck_clean
   exit
    ;;
"clean")
  doxygen_clean
  ../tests/cppcheck/gen_cppcheck_results.sh clean
   exit
    ;;
"--help" | "help")
    usage
    exit
    ;;
*)
    usage
    exit
    ;;
esac

printf "\nUsing support binaries:\n"

# Ensure the binary "doxygen" is on the system
if ! which doxygen ; then
	printf "\n\nERROR: Generating the Fldigi Doxygen documents requires the program: doxygen"
	printf "\n\tPlease install this program to continue."
	printf "\n\n === ABORTING === \n\n"
	exit 1
fi

# Ensure the binary "dot" is on the system
if ! which dot ; then
	printf "\n\nERROR: Generating the Fldigi Doxygen documents requires the program: dot"
	printf "\n\tThis program is part of the package: graphviz \n\n"
	printf "\n\tPlease install this program to continue."
	printf "\n\n === ABORTING === \n\n"
	exit 1
fi

# Ensure the binary "mscgen" is on the system
if ! which mscgen ; then
	printf "\n\nERROR: Generating the Fldigi Doxygen documents requires the program: mscgen"
	printf "\n\tPlease install this program to continue."
	printf "\n\n === ABORTING === \n\n"
	exit 1
fi

# Ensure the Doxygen config file exists
if [ ! -e ./fldigi_doxyfile.txt ]; then
	printf "\n\nERROR: Doxygen configuration file: \"fldigi_doxyfile.txt\" not found."
	printf "\n\n === ABORTING === \n\n"
	exit 1
fi

doxygen fldigi_doxyfile.txt 

# Go create some really useful information using git
cd HTML
	mkdir __git; cd __git
		git format-patch --summary -n HEAD~125 # Create patches for the last 125 commits
		git log --stat -n 125 > gitlog.txt # Dump the history of the last 125 commits
	cd ..
cd ..

if ! which cppcheck ; then
	printf "\n\nWARNING: Binary \"cppcheck\" not found."
	printf "\n\n\t Skipping sourcecode analysis. Install cppcheck and re-run.\n\n"
else
	../tests/cppcheck/gen_cppcheck_results.sh run
fi

printf "\n\n === DOXYGEN documentation generation complete. ==="
printf "\n\nDocumentation Directory: $(pwd)/HTML"
printf "\nMain file: $(pwd)/HTML/index.html\n\n"

# Open the main-Doxygen page in a web-browser.
if which xdg-open ; then
	xdg-open ./HTML/index.html # Ubuntu, Linux, etc...
else
	open ./HTML/index.html # OSX, Unix, etc..
fi