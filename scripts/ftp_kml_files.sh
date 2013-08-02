#!/bin/sh

# This script copies KML files created by fldigi, to a remote machine,
# where the resulting URL can be given to Google Maps.
# The script path name ùust be entered in the parameter "Command"
# of the KML configuration tab. it will started each time new 
# KML files are created or updated.

HOST=$1
if [ "$HOST" == "" ]
then
	read -p "Host name:" HOST
fi

USER=$2
if [ "$USER" == "" ]
then
	read -p "FTP user:" USER
fi

PASSWD=$3
if [ "$PASSWD" == "" ]
then
	read -p "FTP password:" PASSWD
fi

DIRECTORY=$4
if [ "$DIRECTORY" == "" ]
then
	read -p "FTP target directory:" DIRECTORY
fi

cd ~/.fldigi/kml

#cat > toto <<END_SCRIPT
ftp -n $HOST <<END_SCRIPT
quote USER $USER
quote PASS $PASSWD
cd $DIRECTORY
prompt
mput *.kml
quit
END_SCRIPT

cd -

echo "FTP remote copy to $HOST done"
