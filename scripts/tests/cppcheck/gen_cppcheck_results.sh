#! /bin/bash
#
# KL4YFD 2013
# Released under GNU GPL
#

# This script runs automatic source-code checks using the tool: "cppcheck"
# Checks are done to ensure needed binary exists first

cd $( dirname ${BASH_SOURCE[0]} )


INCLUDEDIR="../../../src/include"
SRCDIR="../../../src"
RESULTSDIR="results"
THREADS=8


function usage
{
	printf "\n\nThis script executes the tool \"cppcheck\" and "
	printf "\nsorts the results into separate files by severity / type\n Note: This analysis takes about the same time as compilation."
	printf "\n\nUsage:"
	printf "\n\tRun cppcheck tests:\t ./gen_cppcheck_results.sh run"
	printf "\n\tClean up all files:\t ./gen_cppcheck_results.sh clean"
	printf "\n\tPrint this usage:\t ./gen_cppcheck_results.sh help \n\n"
}

function cppcheck_clean {
	rm -Rf $RESULTSDIR
	printf "\ncppcheck results deleted!\n"
}


case "$1" in
"run")
  cppcheck_clean
  # Continue with rest of script...
  break
    ;;
"clean")
   cppcheck_clean
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

# Ensure the binary "cppcheck" is on the system
if ! which cppcheck ; then
	printf "\n\nERROR: Running the Fldigi cppcheck tests requires the program: cppcheck"
	printf "\n\t Please install this program to continue."
	printf "\n\n === ABORTING === \n\n"
	exit 1
fi

mkdir $RESULTSDIR

cppcheck --inline-suppr --inconclusive --enable=all -I $INCLUDEDIR -j $THREADS --force --verbose $SRCDIR  2> $RESULTSDIR/ALL.txt

cd $RESULTSDIR
  # Separate out the results into files based on their "cppcheck types"
  cat ALL.txt | grep "(error)" > error.txt
  cat ALL.txt | grep "(warning)" > warning.txt
  cat ALL.txt | grep "(style)" > style.txt
  cat ALL.txt | grep "(performance)" > performance.txt
  cat ALL.txt | grep "(portability)" > portability.txt
  cat ALL.txt | grep "(information)" > information.txt
  cat ALL.txt | grep "(debug)" > debug.txt
  
  # Separate out the tests with inconclusive results
  cat ALL.txt | grep "(error, inconclusive)" > error_inconclusive.txt
  cat ALL.txt | grep "(warning, inconclusive)" > warning_inconclusive.txt
  cat ALL.txt | grep "(style, inconclusive)" > style_inconclusive.txt
  cat ALL.txt | grep "(performance, inconclusive)" > performance_inconclusive.txt
  cat ALL.txt | grep "(portability, inconclusive)" > portability_inconclusive.txt
  cat ALL.txt | grep "(information, inconclusive)" > information_inconclusive.txt
  #cat ALL.txt | grep "(debug, inconclusive)" > debug.txt # debug is for Messages from cppcheck itself, not a test-result. Therefore no such combination.
  
  # Just in case... Catch everything _not_ in the above blocks. 
  cat ALL.txt | grep --invert-match "(error)" \
	      | grep --invert-match "(warning)" \
	      | grep --invert-match "(style)" \
	      | grep --invert-match "(performance)" \
	      | grep --invert-match "(portability)" \
	      | grep --invert-match "(information)" \
	      | grep --invert-match "(debug)" \
	      | grep --invert-match "(error, inconclusive)" \
	      | grep --invert-match "(warning, inconclusive)" \
	      | grep --invert-match "(style, inconclusive)" \
	      | grep --invert-match "(performance, inconclusive)" \
	      | grep --invert-match "(portability, inconclusive)" \
	      | grep --invert-match "(information, inconclusive)" > leftover.txt
cd ..

printf "\n\n === cppcheck source-code analysis complete. ==="
printf "\n\nResults saved in: $(pwd)/results \n\n"
