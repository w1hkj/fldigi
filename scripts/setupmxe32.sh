#!/bin/bash
# Copyright GNU GPL
# (c) John Phelps (KL4YFD) 2020 - 2021
#     Dave Freese (W1HKJ) 2021

# This script automatically downloads and compiles the
# M Cross Environment (MXE) on Linux, then uses MXE
# to compile Fldigi's supporting libraries for a Windows OS.

# After running this script successfully, compile Fldigi for Windows
# easily by running:   ./scripts/buildmxe.sh

# Currently Supported Distributions:
# Debian, Fedora, Mint, MX, OpenSuse, Ubuntu

### Extra-Security compile-flags copied from Hardened Gentoo
HARDEN=false		# set true to compile Fldigi with extra security
HARD_CFLAGS=" -fPIE -fstack-protector-all -D_FORTIFY_SOURCE=2 "
HARD_LDFLAGS=" -Wl,-z,now -Wl,-z,relro "
###

MXE_DIRECTORY=$HOME/fldigi.MXE

do_checkout=false
GITHASH=0

OS_detected="-1"

THREADS=8

MYPID=$$

function Usage {
	echo -e "\nThis script auto downloads, compiles, and installs an optimized MXE \ncross-compiling sub-environment for Fldigi in: $MXE_DIRECTORY"
	echo -e "Used by buildmxe.sh for cross-compiling Fldigi to Windows targets."
	echo -e "\nTo Use:"
	echo -e "  1) sudo ./setupmxe.sh packages   <-- Install needed OS packages"
	echo -e "  2) ./setupmxe.sh setup32   <-- Download, compile, install MXE 32-bit"
	echo -e "  3) ./buildmxe.sh   <-- Build the windows binaries and installer"

	echo -e "\n  install a 64-bit environment by instead running:\n   2) ./setupmxe.sh setup64"

	echo -e "\n  use a specific MXE revision by instead running:\n   2) ./setupmxe.sh setup32 GITHASH\n"

	exit  #### Just in case someone forgets to call exit after this function
}

# Completely stops the script and all subshells/processes
function HALT
{
	kill -10 $MYPID
}

# Detect the running OS, set a global variable with Linux distribution-name
function DetectOS
{
	OS_detected="-1"

	cat /etc/redhat-release | grep -i fedora > /dev/null
	if [[ $? -eq 0 ]]; then
		OS_detected="fedora"
		return
	fi

	cat /etc/os-release | grep -i mint > /dev/null # must come before debian and ubuntu
	if [[ $? -eq 0 ]]; then
		OS_detected="mint"
		return
	fi

	cat /etc/os-release | grep -i ubuntu > /dev/null # must come before debian
	if [[ $? -eq 0 ]]; then
		OS_detected="ubuntu"
		return
	fi

	cat /etc/os-release | grep -i debian > /dev/null # this also detects MX Linux
	if [[ $? -eq 0 ]]; then
		OS_detected="debian"
		return
	fi

	cat /etc/os-release | grep -i opensuse > /dev/null
	if [[ $? -eq 0 ]]; then
		OS_detected="opensuse"
		return
	fi

	if [ $OS_detected -eq "-1" ]; then
		echo -e "\nERROR: OS detection failed. Aborting\n"
		exit 1
	else
		echo -e "\nOS Detected: $OS_detected\n"
		return

	fi

	exit 100 # Should never get here
}

# Delete an installed MXE environment
function CleanMXE
{
	# If an MXE direcory already exists, remove it
	if [ -d $MXE_DIRECTORY ]; then
		cd $MXE_DIRECTORY || exit 1
			rm -Rfv mxe
			rm -fv *
		cd ..
		rmdir $MXE_DIRECTORY
		echo -e "\nMXE windows crossbuild environment deleted from $MXE_DIRECTORY \n"
	else
		echo -e "\nERROR: MXE directory not found \n"
	fi

}


function MXEBuildFail
{
	echo -e "\nERROR: MXE Package failed to build. Aborting installation.\n\n"
	HALT
}

function AmRoot
{
	if [ "$EUID" -ne 0 ]; then
		echo -e "\nERROR: Package installation must be done as the ROOT user. \n"
		exit 1
	fi
}


function PKGInstallDebian
{
	# This block also used for MX Linux
	apt -y build-dep fldigi || exit 1  # install Fldigi build dependencies
	apt -y install openssl make autoconf git gperf libgdk-pixbuf2.0-dev libtool libtool-bin libssl-dev p7zip nsis bison flex ruby libgettextpo-dev intltool patch lzip python || exit 1
}

function PKGInstallFedora
{
	dnf -y builddep fldigi || exit 1 # install Fldigi build dependencies
	dnf -y install openssl openssl-static yum-utils make autoconf git gperf gdk-pixbuf2-devel libtool p7zip nsis bison flex ruby gettext-devel intltool patch lzip python2 || exit 1
}

function PKGInstallMint
{
	apt  build-dep -y fldigi || exit 1  # install Fldigi build dependencies
	apt  install -y openssl make autoconf git gperf libgdk-pixbuf2.0-dev libtool libtool-bin p7zip nsis bison flex ruby libgettextpo-dev intltool patch lzip python || exit 1
}

function PKGInstallOpenSuse
{
	zypper -n si -d fldigi || exit 1 	# install Fldigi build dependencies
	zypper -n install openssl libopenssl-devel make autoconf git gperf gdk-pixbuf-devel libtool p7zip bison flex ruby tinygettext-devel intltool patch lzip python || exit 1

	echo -e "\n\nWARNING: Package nsis missing on OpenSuse. Cannot create Windows Installer.\n\n"
}

function PKGInstallUbuntu
{
	apt -y build-dep fldigi || exit 1  # install Fldigi build dependencies
	apt -y install openssl make autoconf git gperf libgdk-pixbuf2.0-dev libtool libtool-bin libssl-dev p7zip nsis bison flex ruby libgettextpo-dev intltool patch lzip python || exit 1
}


function MXEDownload
{
	mkdir $MXE_DIRECTORY || exit 1
	cd $MXE_DIRECTORY || exit 1

	git clone https://github.com/mxe/mxe.git mxe || exit 1

	# Checkout and install a specific MXE revision
	if $do_checkout ; then
		cd mxe || exit 1
			git checkout $GITHASH || exit 1
		cd ..
	fi

	# Log the current Git-Revision hash for debugging
	cd mxe
	git rev-parse HEAD > ../HEAD.log
}


function MXEInstall32
{
	set -o pipefail # Make non-zero exit-codes pass-through piped commands

	echo -e "\nInstalling 32-bit MXE crossbuild sub-environment in $MXE_DIRECTORY \n"

	cd $MXE_DIRECTORY || exit 1

	cd mxe || exit 1

	MXE_32="MXE_TARGETS=i686-w64-mingw32.static" # 32-bit libraries
	#export CFLAGS=" -march=i686 -mtune=i686 -O3 -mno-3dnow -mmmx -msse -mfpmath=sse "	# Compatible with all 686 processors
	export CFLAGS=" -march=i686 -mtune=i686 -O2 -ffast-math -mno-3dnow -mmmx -msse -mfpmath=sse "	# Compatible with all 686 processors
	if $HARDEN; then
		CFLAGS+=$HARD_CFLAGS
		LDFLAGS+=$HARD_LDFLAGS
	fi
	export CXXFLAGS=$CFLAGS

	echo -e "\nStarting compile of MXE 32-bit cross-compile environment"
	echo -e "\t$MXE_32"
	echo -e "\tCFLAGS=$CFLAGS"
	echo -e "\tCXXFLAGS=$CXXFLAGS\n\n"

	# NOTE: The order these are compiled in matters
	make -j$THREADS $MXE_32  cc 			| tee $MXE_DIRECTORY/cc.32.log				|| MXEBuildFail
	make -j$THREADS $MXE_32  zlib			| tee $MXE_DIRECTORY/zlib.32.log			|| MXEBuildFail
	make -j$THREADS $MXE_32  libpng			| tee $MXE_DIRECTORY/libpng.32.log			|| MXEBuildFail
	make -j$THREADS $MXE_32  pthreads		| tee $MXE_DIRECTORY/pthreads.32.log		|| MXEBuildFail
	make -j$THREADS $MXE_32  pcre			| tee $MXE_DIRECTORY/pcre.32.log			|| MXEBuildFail
	make -j$THREADS $MXE_32  gnutls			| tee $MXE_DIRECTORY/gnutls.32.log			|| MXEBuildFail
	make -j$THREADS $MXE_32  portaudio		| tee $MXE_DIRECTORY/portaudio.32.log		|| MXEBuildFail
	make -j$THREADS $MXE_32  libsndfile		| tee $MXE_DIRECTORY/libsndfile.32.log		|| MXEBuildFail
	make -j$THREADS $MXE_32  libsamplerate	| tee $MXE_DIRECTORY/libsamplerate.32.log	|| MXEBuildFail
	make -j$THREADS $MXE_32  hamlib			| tee $MXE_DIRECTORY/hamlib.32.log			|| MXEBuildFail
	make -j$THREADS $MXE_32  fltk			| tee $MXE_DIRECTORY/fltk.32.log			|| MXEBuildFail
	make -j$THREADS $MXE_32  libgnurx		| tee $MXE_DIRECTORY/libgnurx.32.log		|| MXEBuildFail

	# Make a list of the compiled-packages versions (for debugging)
	cd pkg
	ls -1 > $MXE_DIRECTORY/VERSIONS.log

	date > $MXE_DIRECTORY/DATE.32.log # log the date/time MXE finished setup

	echo -e "\nMXE 32-bit cross-compile environment successfully setup"
	echo "Settings:"
	echo -e "\t$MXE_32"
	echo -e "\tCFLAGS=$CFLAGS"
	echo -e "\tCXXFLAGS=$CXXFLAGS\n"

	echo -e "\n"
	if $HARDEN; then
		echo -n "HARDENED "
	fi
	echo -e "MXE windows 32-bit crossbuild environment installed in $MXE_DIRECTORY \n\n"

}


function MXEInstall64
{
	set -o pipefail # Make non-zero exit-codes pass-through piped commands

	echo -e "\nInstalling 64-bit MXE crossbuild sub-environment in $MXE_DIRECTORY \n"

	cd $MXE_DIRECTORY || exit 1

	cd mxe || exit 1

	MXE_64="MXE_TARGETS=x86_64-w64-mingw32.static" # 64-bit libraries

# Compatible with all X86_64 processors
	export CFLAGS=" -march=x86-64 -mtune=k8 -O2 -ffast-math -mno-3dnow -mmmx -msse -msse2 -mfpmath=sse "
	if $HARDEN; then
		CFLAGS+=$HARD_CFLAGS
		LDFLAGS+=$HARD_LDFLAGS
	fi
	export CXXFLAGS=$CFLAGS

	echo -e "\nStarting compile of MXE 64-bit cross-compile environment"
	echo "Settings:"
	echo -e "\t$MXE_64"
	echo -e "\tCFLAGS=$CFLAGS"
	echo -e "\tCXXFLAGS=$CXXFLAGS\n\n"

	# NOTE: The order these are compiled in matters
	make -j$THREADS $MXE_64  cc 			| tee $MXE_DIRECTORY/cc.64.log				|| MXEBuildFail
	make -j$THREADS $MXE_64  zlib			| tee $MXE_DIRECTORY/zlib.64.log			|| MXEBuildFail
	make -j$THREADS $MKE_64  libpng			| tee $MXE_DIRECTORY/libpng.64.log			|| MXEBuildFail
	make -j$THREADS $MKE_64  pthreads		| tee $MXE_DIRECTORY/pthreads.64.log		|| MXEBuildFail
	make -j$THREADS $MKE_64  pcre			| tee $MXE_DIRECTORY/pcre.64.log			|| MXEBuildFail
	make -j$THREADS $MKE_64  gnutls			| tee $MXE_DIRECTORY/gnutls.64.log			|| MXEBuildFail
	make -j$THREADS $MKE_64  portaudio		| tee $MXE_DIRECTORY/portaudio.64.log		|| MXEBuildFail
	make -j$THREADS $MKE_64  libsndfile		| tee $MXE_DIRECTORY/libsndfile.64.log		|| MXEBuildFail
	make -j$THREADS $MKE_64  libsamplerate	| tee $MXE_DIRECTORY/libsamplerate.64.log	|| MXEBuildFail
	make -j$THREADS $MKE_64  hamlib			| tee $MXE_DIRECTORY/hamlib.64.log			|| MXEBuildFail
	make -j$THREADS $MKE_64  fltk			| tee $MXE_DIRECTORY/fltk.64.log			|| MXEBuildFail
	make -j$THREADS $MKE_64  libgnurx		| tee $MXE_DIRECTORY/libgnurx.64.log		|| MXEBuildFail

	date > $MXE_DIRECTORY/DATE.64.log # log the date/time MXE finished setup

	echo -e "\nMXE 64-bit cross-compile environment successfully setup"
	echo "Settings:"
	echo -e "\t$MXE_64"
	echo -e "\tCFLAGS=$CFLAGS"
	echo -e "\tCXXFLAGS=$CXXFLAGS\n"

	echo -e "\n"
	if $HARDEN; then
		echo -n "HARDENED "
	fi
	echo -e "MXE windows 64-bit crossbuild environment installed in $MXE_DIRECTORY \n\n"
}


function InstallOSPackages
{
	DetectOS

	echo -e "\nInstalling required OS packages for: $OS_detected \n"

	case "$OS_detected" in
		"debian")
			PKGInstallDebian
			;;
		"fedora")
			PKGInstallFedora
			;;
		"mint")
			PKGInstallMint
			;;
		"opensuse")
			PKGInstallOpenSuse
			;;
		"ubuntu")
			PKGInstallUbuntu
			;;
		*)
			exit 1
	esac

	echo -e "\nAll $OS_detected OS packages successfully installed\n"
}

# Check if the MXE directory already exists, and exit if it does
function CheckDirectoryMXE
{
	if [ -d $MXE_DIRECTORY ]; then
		echo -e "\nERROR: MXE directory already exists\nTo remove run: # setupmxe.sh clean\n"

		exit 1
	fi
}


################################

# Ensure a commandline option was passed
if [[ "$#" != 1 ]] && [[ "$#" != 2 ]]; then
	Usage
	exit 1
fi

# Checkout a specific version of MXE
if [[ "$#" -eq 2 ]]; then
	GITHASH=$2
	do_checkout=true
fi


# Parse the commandline
if [[ $1 == "clean" ]]; then
	CleanMXE
	exit 0

elif [[ $1 == "packages" ]]; then
	AmRoot
 	InstallOSPackages
	exit 0

elif [[ $1 == "setup32" ]]; then
	CheckDirectoryMXE 	# If an MXE directory already exists, display message and exit
	MXEDownload
	MXEInstall32
	exit 0

elif [[ $1 == "setup64" ]]; then
	CheckDirectoryMXE 	# If an MXE directory already exists, display message and exit
	MXEDownload
	MXEInstall64
	exit 0

else
	Usage
	exit 1
fi

