#!/bin/sh

### Script to create the .app structure for osx
### 20080227  Stelios Bounanos M0GLD
### Updated 20080727: enable the .icns support

if [ $# -ne 2 ]; then
    echo "Syntax: $0 data-dir build-dir" >&2
    exit 1
fi

if [ -z "$PACKAGE" ]; then
    echo "E: \$PACKAGE undefined"
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

plist="${data}/mac/Info.plist.in"
icons="${data}/mac/fldigi.icns"
for f in "$plist" "$icons"; do
    test -r "$f" && continue
    echo "E: ${f}: not readable" >&2
    exit 1
done


identifier="com.w1hkj.fldigi"
name="Fldigi"
signature="fldg"
binary="fldigi"
version="${FLDIGI_VERSION_MAJOR}.${FLDIGI_VERSION_MINOR}"
icon="`basename $icons`"

set -e

cd "$build"

# bundle the binary
echo "Creating ${build}/mac-bundle/"$PACKAGE".app"
$mkinstalldirs mac-bundle/"$PACKAGE".app/Contents/MacOS mac-bundle/"$PACKAGE".app/Contents/Resources
cd mac-bundle
$INSTALL_STRIP_PROGRAM "${build}/$binary" "$PACKAGE".app/Contents/MacOS
$INSTALL_DATA "$icons" "$PACKAGE".app/Contents/Resources
echo "APPL${signature}" > "$PACKAGE".app/Contents/PkgInfo
sed -e "s!%%IDENTIFIER%%!${identifier}!g; s!%%NAME%%!${name}!g;\
        s!%%SIGNATURE%%!${signature}!g; s!%%BINARY%%!${binary}!g;\
        s!%%VERSION%%!${version}!g; s!%%ICON%%!${icon}!g;" < "$plist" > "$PACKAGE".app/Contents/Info.plist
if grep '%%[A-Z]*%%' "$PACKAGE".app/Contents/Info.plist; then
    echo "E: unsubstituted variables in Info.plist!" >&2
    exit 1
fi


# bundle the binary and its non-standard dependencies
echo "Creating ${build}/mac-libs-bundle/"$PACKAGE".app"
cd ..
$mkinstalldirs mac-libs-bundle
cp -pR mac-bundle/"$PACKAGE".app mac-libs-bundle
$mkinstalldirs mac-libs-bundle/"$PACKAGE".app/Contents/Frameworks
cd mac-libs-bundle/"$PACKAGE".app/Contents

list="MacOS/$binary"
while test "x$list" != "x"; do
    change="$list"
    list=""

    for obj in $change; do
	for lib in `otool -L $obj | \
	             sed -n 's!^.*[[:space:]]\([^[:space:]]*\.dylib\).*$!\1!p' | \
                     grep -Ev '^/(usr/lib|System)'`; do
	    libfn="`basename $lib`"
	    if ! test -f "Frameworks/$libfn"; then
		cp "$lib" "Frameworks/$libfn"
		install_name_tool -id "@executable_path/../Frameworks/$libfn" "Frameworks/$libfn"
		list="$list Frameworks/$libfn"
	    fi
	    install_name_tool -change "$lib" "@executable_path/../Frameworks/$libfn" "$obj"
	done
    done
done
