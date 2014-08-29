#!/bin/bash
#
# A simple script for creating/archiving doxygen documentation for FLDigi

PRG_NAME="FLDigi"

LATEX="0"
DOXY="0"
BUILD_PROG_DOCS="0"
BUILD_USER_DOCS="0"

if [ -z $1 ]; then
	BUILD_USER_DOCS="1"
else
	BUILD_USER_DOCS="0"
fi

macintosh_file_clean()
{
	BASE_PATH=$1
	if [ -z $BASE_PATH ]; then
		BASE_PATH="${PWD}"
	fi

	find $BASE_PATH -name ".DS_Store" -exec rm -rf {} \;
}

doc_version()
{
	if [ ! -f $1 ]; then
		echo "Doxyfile not found ($1)"
		echo "PWD=$PWD"
		VER_NUM="UNKNOWN_VERSION"
		return
	fi

	VER_STR=`grep -e "PROJECT_NUMBER*=*" $1`

	if [ -z $VER_STR ]; then
		VER_NUM="UNKNOWN_VERSION"
		return
	fi

	VER_NUM="${VER_STR#*=}"
	VER_NUM="${VER_NUM#"${VER_NUM%%[![:space:]]*}"}"
	VER_NUM="${VER_NUM%"${VER_NUM##*[![:space:]]}"}"
}

help()
{
	echo ""
	echo "Use:"
	echo "   make_docs.sh <user or help>"
	echo ""
	echo "default: user"
	echo "user: Generate user documentation"
	echo "help: This message"
	echo ""
}

rename_file()
{
	if [ -f "$1" ]; then
		mv "$1" "$2"
	fi
}

make_dir()
{
	echo "Make Dir: $1"

	if [ -d "$1" ]; then
		return
	fi

	mkdir "$1"
}

function check_dir()
{
	if [ -d "$1" ]; then
		echo "1"
		return
	fi

	echo "0"
}

function check_file()
{
	if [ -f "$1" ]; then
		echo "1"
		return
	fi

	echo "0"
}

check_doxy()
{
	RESULTS=$(check_file "$PWD/Doxyfile")

	if [ "$RESULTS" = "1" ]; then
		doxygen
	else
		echo "Doxyfile not found in directory $PWD"
	fi
}

check_doxy_exec()
{
	PROG_NAME=`which doxygen`
	EXEC_FILE=`basename $PROG_NAME`

	if [ "$EXEC_FILE" != "doxygen" ]; then
		echo "Install doxygen to build documentation"
		DOXY="0"
	else
		echo "Found $PROG_NAME"
		DOXY="1"
	fi
}

check_latex_exec()
{
	PROG_NAME=`which latex`
	EXEC_FILE=`basename $PROG_NAME`

	if [ "$EXEC_FILE" != "latex" ]; then
		echo "Install TeX/LaTeX to build pdf documentation"
		LATEX="0"
	else
		echo "Found $PROG_NAME"
		LATEX="1"
	fi
}

pdf_docs()
{
	if [ "$LATEX" = "1" ]; then

		OP_DIR="$1"

		RESULTS=$(check_dir "$OP_DIR")

		if [ "$RESULTS" = "1" ]; then
			cd $OP_DIR
			make
			rename_file "refman.pdf" "$2"
			TAR=`which tar`
			if [ -z $TAR ]; then
	    		echo "***************************************"
				echo "* Compression program 'tar' not found *"
	    		echo "***************************************"
				return
			fi
			DIR=`dirname "$2"`
			FILE=`basename "$2" .pdf`
			( cd $DIR
              $TAR -cvzf "${FILE}_pdf.tgz" "${FILE}.pdf" )
		fi
	fi
}

compress_html()
{
	if [ -z $1 ]; then
		return
	fi

	if [ -z $2 ]; then
		return
	fi

	TAR=`which tar`
	if [ -z $TAR ]; then
	    echo "***************************************"
		echo "* Compression program 'tar' not found *"
	    echo "***************************************"
		return
	fi

	echo "PWD=$PWD"
	cd "../$2"

	RESULTS=$(check_dir "$COMP_DIR")

	SAVE_NAME="../compressed_html/$1_html.tgz"

	if [ "$RESULTS" = "1" ]; then
		$TAR -cvzf "$SAVE_NAME" "html/"
	fi
}

copy_files()
{
	SRC="images"
	DST="user_docs/latex"

	echo "********************************************************"
	echo "* Copying required images to LaTeX folder"
	echo "* Source $PWD/$SRC"
	echo "* Destination $PWD/$DST"
	echo "********************************************************"

	cp $SRC/event-log-debug.png $DST/event-log-debug.png
	cp $SRC/event-log-error.png $DST/event-log-error.png
	cp $SRC/event-log-info.png $DST/event-log-info.png
	cp $SRC/event-log-quiet.png $DST/event-log-quiet.png
	cp $SRC/event-log-verbose.png $DST/event-log-verbose.png
	cp $SRC/event-log-warning.png $DST/event-log-warning.png
	cp $SRC/digiscope-psk-nosig.png $DST/digiscope-psk-nosig.png
	cp $SRC/digiscope-psk.png $DST/digiscope-psk.png
	cp $SRC/digiscope-psk-low.png $DST/digiscope-psk-low.png
	cp $SRC/digiscope-psk-high.png $DST/digiscope-psk-high.png
	cp $SRC/digiscope-psk-history.png $DST/digiscope-psk-history.png
	cp $SRC/digiscope-psk-history-amplitude.png $DST/digiscope-psk-history-amplitude.png
	cp $SRC/wwv3.png $DST/wwv3.png
	cp $SRC/wwv1-lgneg.png $DST/wwv1-lgneg.png
	cp $SRC/wwv2-000ppm.png $DST/wwv2-000ppm.png
	cp $SRC/wwv2-lgpos.png $DST/wwv2-lgpos.png
	cp $SRC/wwv2-125ppm.png $DST/wwv2-125ppm.png
	cp $SRC/dcf77-0.png $DST/dcf77-0.png
	cp $SRC/dcf77-0zoom.png $DST/dcf77-0zoom.png
	cp $SRC/dcf77-1000.png $DST/dcf77-1000.png
	cp $SRC/dcf77-65zoom.png $DST/dcf77-65zoom.png
	cp $SRC/RWMpre-cal.png $DST/RWMpre-cal.png
	cp $SRC/RWM+25361ppm.png $DST/RWM+25361ppm.png
	cp $SRC/RWMpost-cal-x5.png $DST/RWMpost-cal-x5.png


	DST="user_docs/html"

	echo "********************************************************"
	echo "* Copying required images to HTML folder"
	echo "* Source $PWD/$SRC"
	echo "* Destination $PWD/$DST"
	echo "********************************************************"

	cp $SRC/event-log-debug.png $DST/event-log-debug.png
	cp $SRC/event-log-error.png $DST/event-log-error.png
	cp $SRC/event-log-info.png $DST/event-log-info.png
	cp $SRC/event-log-quiet.png $DST/event-log-quiet.png
	cp $SRC/event-log-verbose.png $DST/event-log-verbose.png
	cp $SRC/event-log-warning.png $DST/event-log-warning.png
	cp $SRC/digiscope-psk-nosig.png $DST/digiscope-psk-nosig.png
	cp $SRC/digiscope-psk.png $DST/digiscope-psk.png
	cp $SRC/digiscope-psk-low.png $DST/digiscope-psk-low.png
	cp $SRC/digiscope-psk-high.png $DST/digiscope-psk-high.png
	cp $SRC/digiscope-psk-history.png $DST/digiscope-psk-history.png
	cp $SRC/digiscope-psk-history-amplitude.png $DST/digiscope-psk-history-amplitude.png
	cp $SRC/wwv3.png $DST/wwv3.png
	cp $SRC/wwv1-lgneg.png $DST/wwv1-lgneg.png
	cp $SRC/wwv2-000ppm.png $DST/wwv2-000ppm.png
	cp $SRC/wwv2-lgpos.png $DST/wwv2-lgpos.png
	cp $SRC/wwv2-125ppm.png $DST/wwv2-125ppm.png
	cp $SRC/dcf77-0.png $DST/dcf77-0.png
	cp $SRC/dcf77-0zoom.png $DST/dcf77-0zoom.png
	cp $SRC/dcf77-1000.png $DST/dcf77-1000.png
	cp $SRC/dcf77-65zoom.png $DST/dcf77-65zoom.png
	cp $SRC/RWMpre-cal.png $DST/RWMpre-cal.png
	cp $SRC/RWM+25361ppm.png $DST/RWM+25361ppm.png
	cp $SRC/RWMpost-cal-x5.png $DST/RWMpost-cal-x5.png

}

for var in "$@"
do
	case "$var" in
		prog)
			BUILD_PROG_DOCS="1"
			;;
		PROG)
			BUILD_PROG_DOCS="1"
			;;
		USER)
			BUILD_USER_DOCS="1"
			;;
		user)
			BUILD_USER_DOCS="1"
			;;
		*)
			help
			exit
			;;
	esac
done

check_doxy_exec

if [ "$DOXY" != "1" ]; then
	echo "**************************************"
	echo "* Install Doxygen to build documents *"
	echo "**************************************"
	exit
fi

check_latex_exec

if [ "$LATEX" != "1" ]; then
	echo "********************************************"
	echo "* Install TeX/LaTeX to build PDF documents *"
	echo "********************************************"
fi


SCRIPT_PATH="$PWD/make_docs.sh"

echo "Looking for Script: $SCRIPT_PATH"

if [ -f "$SCRIPT_PATH" ]; then

	HTML="fldigi_${VER_NUM}_user_manual_html"

	make_dir "pdf"
	make_dir "compressed_html"
	make_dir "user_docs"
	make_dir "user_docs/latex"
	make_dir "user_docs/html"
	make_dir "prog_docs"
	make_dir "prog_docs/latex"
	make_dir "prog_docs/html"

	( cd doxygen; copy_files )

else
	echo "***********************************************************************"
	echo "* Change Directory to the ./make_doc.sh location then execute script. *"
	echo "***********************************************************************"
	exit
fi

# User Manual
if [ $BUILD_USER_DOCS -eq "1" ]; then
(
	cd user_src_docs
	doc_version "${PWD}/Doxyfile"
	check_doxy
	( compress_html "${PRG_NAME}_${VER_NUM}_Users_Manual" "user_docs")
	pdf_docs "../user_docs/latex" "../../pdf/${PRG_NAME}_${VER_NUM}_Users_Manual.pdf"
)
fi

# Programmers Code Reference
if [ $BUILD_PROG_DOCS -eq "1" ]; then
(
	cd prog_src_doc
	doc_version "${PWD}/Doxyfile"
	check_doxy
	( compress_html "${PRG_NAME}_${VER_NUM}_Code_Reference" "prog_docs" )
	pdf_docs "../prog_docs/latex" "../../pdf/${PRG_NAME}_${VER_NUM}_Code_Reference.pdf"
)
fi
