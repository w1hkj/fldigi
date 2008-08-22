#!/bin/sh

### Script to create the .app structure for osx
### 20080227  Stelios Bounanos M0GLD
### Updated 20080727: enable the .icns support

if [ $# -ne 4 ]; then
    echo "Syntax: $0 data-dir build-dir bundle-dir static-bundle-dir" >&2
    exit 1
fi

if [ -z "$PACKAGE_TARNAME" ]; then
    echo "E: \$PACKAGE_TARNAME undefined"
    exit 1
fi

PWD=`pwd`
data="${PWD}/$1"
build="${PWD}/$2"
bundle_dir="$3"
static_bundle_dir="$4"
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
icon="${data}/mac/fldigi.icns"
for f in "$plist" "$icon"; do
    test -r "$f" && continue
    echo "E: ${f}: not readable" >&2
    exit 1
done

# aaaaaaaaaargh => Aaaaaaaaaargh
upcase1()
{
    sed 'h; s/\(^.\).*/\1/; y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/; G; s/\n.//'
}

identifier="com.w1hkj.$PACKAGE_TARNAME"
name=$(echo "$PACKAGE_TARNAME" | upcase1)
# we'll use the first four consonants as the signature
signature="$(echo $PACKAGE_TARNAME | sed 's/[aeiouAEIOU]//g; s/\(^....\).*/\1/')"
binary="$PACKAGE_TARNAME"
version="${FLDIGI_VERSION_MAJOR}.${FLDIGI_VERSION_MINOR}"

set -e

cd "$build"

if test "x$STRIP" = "x0"; then
    INSTALL_PROGRAM_CMD="$INSTALL_STRIP_PROGRAM"
else
    INSTALL_PROGRAM_CMD="$INSTALL_PROGRAM"
fi

# bundle the binary
appname="${PACKAGE_TARNAME}-${PACKAGE_VERSION}.app"
echo "Creating ${build}/$bundle_dir/$appname"
$mkinstalldirs "$bundle_dir/$appname/Contents/MacOS" "$bundle_dir/$appname/Contents/Resources"
cd "$bundle_dir"
$INSTALL_PROGRAM_CMD "${build}/$binary" "$appname/Contents/MacOS"
$INSTALL_DATA "$icon" "$appname/Contents/Resources"
echo "APPL${signature}" > "$appname/Contents/PkgInfo"
sed -e "s!%%IDENTIFIER%%!${identifier}!g; s!%%NAME%%!${name}!g;\
        s!%%SIGNATURE%%!${signature}!g; s!%%BINARY%%!${binary}!g;\
        s!%%VERSION%%!${version}!g; s!%%ICON%%!${icon##*/}!g;" < "$plist" > "$appname/Contents/Info.plist"
if grep '%%[A-Z]*%%' "$appname/Contents/Info.plist"; then
    echo "E: unsubstituted variables in $appname/Contents/Info.plist" >&2
    exit 1
fi


# bundle the binary and its non-standard dependencies
echo "Creating ${build}/$static_bundle_dir/$appname"
cd ..
$mkinstalldirs "$static_bundle_dir"
cp -pR "$bundle_dir/$appname" "$static_bundle_dir"
$mkinstalldirs "$static_bundle_dir/$appname/Contents/Frameworks"
cd "$static_bundle_dir/$appname/Contents"

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

cd "$build"
hdiutil create -srcfolder "$bundle_dir" -format UDZO -tgtimagekey zlib-level=9 "$PACKAGE_TARNAME-$PACKAGE_VERSION-nolibs.dmg"
hdiutil create -srcfolder "$static_bundle_dir" -format UDZO -tgtimagekey zlib-level=9 "$PACKAGE_TARNAME-$PACKAGE_VERSION.dmg"
