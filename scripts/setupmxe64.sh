#!/bin/bash

export MXE_DIR=$HOME/.fldigiMXE

# ======================================================================
# script support functions

function MXEBuildFail
{
	echo -e "\nERROR: MXE Package failed to build. Aborting installation.\n\n"
	HALT
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

# ======================================================================

echo -e "\nInstalling 64-bit MXE crossbuild sub-environment in $MXE_DIR \n"

#CheckDirectoryMXE 	# If an MXE directory already exists, display message and exit
#MXEDownload
#MXEInstall64

cd $MXE_DIR || exit 1

cd mxe || exit 1

MXE_64="MXE_TARGETS=x86_64-w64-mingw32.static" # 64-bit libraries

# Compatible with all X86_64 processors
CFLAGS=" -march=x86-64 -mtune=k8 -O2 -ffast-math -mno-3dnow -mmmx -msse -msse2 -mfpmath=sse "
CXXFLAGS=" -march=x86-64 -mtune=k8 -O2 -ffast-math -mno-3dnow -mmmx -msse -msse2 -mfpmath=sse "

export MXE_64
export CFLAGS
export CXXFLAGS

echo -e "\nStarting compile of MXE 64-bit cross-compile environment"
echo "Settings:"
echo -e "\t$MXE_DIR"
echo -e "\t$MXE_64"
echo -e "\tCFLAGS=$CFLAGS"
echo -e "\tCXXFLAGS=$CXXFLAGS\n\n"

# NOTE: The order these are compiled in matters
make -j 4 $MXE_64  cc | tee $MXE_DIR/cc.64.log || MXEBuildFail
make -j 4 $MXE_64  zlib | tee $MXE_DIR/zlib.64.log || MXEBuildFail
make -j 4 $MKE_64  libpng | tee $MXE_DIR/libpng.64.log || MXEBuildFail
make -j 4 $MKE_64  pthreads | tee $MXE_DIR/pthreads.64.log || MXEBuildFail
make -j 4 $MKE_64  pcre | tee $MXE_DIR/pcre.64.log || MXEBuildFail
make -j 4 $MKE_64  gnutls | tee $MXE_DIR/gnutls.64.log || MXEBuildFail
make -j 4 $MKE_64  portaudio | tee $MXE_DIR/portaudio.64.log || MXEBuildFail
make -j 4 $MKE_64  libsndfile | tee $MXE_DIR/libsndfile.64.log || MXEBuildFail
make -j 4 $MKE_64  libsamplerate | tee $MXE_DIR/libsamplerate.64.log || MXEBuildFail
make -j 4 $MKE_64  hamlib | tee $MXE_DIR/hamlib.64.log || MXEBuildFail
make -j 4 $MKE_64  fltk | tee $MXE_DIR/fltk.64.log || MXEBuildFail
make -j 4 $MKE_64  libgnurx | tee $MXE_DIR/libgnurx.64.log || MXEBuildFail

date > $MXE_DIR/DATE.64.log # log the date/time MXE finished setup

echo -e "\nMXE 64-bit cross-compile environment successfully setup"

echo -e "MXE windows 64-bit crossbuild environment installed in $MXE_DIR \n\n"
