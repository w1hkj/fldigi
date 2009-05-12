#!/bin/sh

### Script to create a win32 installer file
### 20090510  Stelios Bounanos M0GLD


if [ $# -ne 3 ]; then
    echo "Syntax: $0 data-dir build-dir installer-file" >&2
    exit 1
fi

if [ -z "$PACKAGE_TARNAME" ]; then
    echo "E: \$PACKAGE_TARNAME undefined"
    exit 1
fi

PWD=`pwd`
data="${PWD}/$1"
build="${PWD}/$2"
installer_file="$3"

# more sanity checks
for d in "$data" "$build"; do
    test -d "$d" && continue
    echo "E: ${d}: not a directory" >&2
    exit 1
done
if ! test -w "$build"; then
    echo "E: ${build} is not writeable" >&2
    exit 1
fi

# aaaaaaaaaargh => Aaaaaaaaaargh
upcase1()
{
    sed 'h; s/\(^.\).*/\1/; y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/; G; s/\n.//'
}

binary="$PACKAGE_TARNAME".exe
if test "x$STRIP" != "x0"; then
    $STRIP -S "$binary"
fi

name="$(echo $PACKAGE_TARNAME | upcase1)"

$MAKENSIS -V2 -NOCD -D"INSTALLER_FILE=$installer_file" -D"LICENSE_FILE=$data/../COPYING" \
    -D"PROGRAM_NAME=$name" -D"PROGRAM_VERSION=$PACKAGE_VERSION" \
    -D"BINARY=$binary" -D"SUPPORT_URL=$PACKAGE_HOME" -D"UPDATES_URL=$PACKAGE_DL" \
    -D"DOCS_URL=$PACKAGE_DOCS" -D"GUIDE_URL=$PACKAGE_GUIDE" "$data/win32/fldigi.nsi"
