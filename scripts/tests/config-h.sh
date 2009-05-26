#!/bin/sh

# Check for translation units that don't include config.h

r=0
for f in $fldigi_SOURCES $flarq_SOURCES; do
    base=$(echo $f | sed -n '/\.[cC][cCpPxX]\{1,\}/ { s!.*/\(.*\)\.[^.]*$!\1!; p }')
    test "x$base" = "x" && continue
    test -f "fldigi-${base}.${OBJEXT}" || test -f "flarq-${base}.${OBJEXT}" || continue
    if ! grep "include.*config\.h" "${srcdir}/${f}" >/dev/null; then
        echo "E: $f does not include config.h" >&2
        r=1
    fi
done

exit $r
