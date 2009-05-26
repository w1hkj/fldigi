#!/bin/sh

# Check for files with CRLF line terminators

r=0
for f in $fldigi_SOURCES $flarq_SOURCES $EXTRA_fldigi_SOURCES $EXTRA_DIST; do
    base=$(echo $f | sed -n '/\.[cC][cCpPxX]\{1,\}/ { s!.*/\(.*\)\.[^.]*$!\1!; p }')
    if test "x$base" != "x" && grep "" "${srcdir}/${f}" >/dev/null; then
        echo "E: $f has CRLF line terminators" >&2
        r=1
    fi
done

if [ $r -eq 1 ]; then
    echo "Please remove the CRs from the above file(s)." 2>&1
    echo "You can use something like:  sed -i 's/\r$//' FILE1 FILE2 ..." 2>&1
fi

exit $r
