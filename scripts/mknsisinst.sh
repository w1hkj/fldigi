#!/bin/sh

### Script to create a win32 installer file
### 20110209  Stelios Bounanos M0GLD

if [ $# -ne 2 ]; then
    echo "Syntax: $0 data-dir build-dir" >&2
    exit 1
fi

if [ -z "$PACKAGE_TARNAME" ]; then
    echo "E: \$PACKAGE_TARNAME undefined"
    exit 1
fi

PWD=`pwd`
data="${PWD}/$1"
build="${PWD}/$2"

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

set -e

fldigi_name=Fldigi
fldigi_bin=fldigi.exe
flarq_name=Flarq
flarq_bin=flarq.exe

def=

if test "x$WANT_FLDIGI" != "xyes" && test "x$WANT_FLARQ" != "xyes"; then
    echo "E: refusing to create empty installer" >&2
    exit 1
fi
if test "x$WANT_FLDIGI" = "xyes"; then
#    test "x$NOSTRIP" = "x" && $STRIP -S "$fldigi_bin"
    def="$def -DHAVE_FLDIGI -DFLDIGI_NAME=$fldigi_name -DFLDIGI_BINARY=$fldigi_bin -DFLDIGI_VERSION=$PACKAGE_VERSION"
fi
if test "x$WANT_FLARQ" = "xyes"; then
#    test "x$NOSTRIP" = "x" && $STRIP -S "$flarq_bin"
    def="$def -DHAVE_FLARQ -DFLARQ_NAME=$flarq_name -DFLARQ_BINARY=$flarq_bin -DFLARQ_VERSION=$FLARQ_VERSION"
fi

if test "x$USE_NLS" = "xyes" && make -C "$srcdir/../po" install prefix="$build" >/dev/null; then
    def="$def -DFLDIGI_LOCALE_PATH=$build/share -DFLDIGI_LOCALE_DIR=locale"
fi

$MAKENSIS -V2 -NOCD -D"INSTALLER_FILE=$INSTALLER_FILE" -D"LICENSE_FILE=$data/../COPYING" \
    -D"SUPPORT_URL=$PACKAGE_HOME" -D"UPDATES_URL=$PACKAGE_DL" -D"FLDIGI_DOCS_URL=$PACKAGE_DOCS" \
    -D"FLARQ_DOCS_URL=$FLARQ_DOCS" -D"GUIDE_URL=$PACKAGE_GUIDE" $def "$data/win32/fldigi.nsi"
