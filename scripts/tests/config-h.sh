#!/bin/sh

# Check for translation units that don't include config.h

r=0
for f in $fldigi_SOURCES; do
    base=$(echo $f | sed -n '/\.[cC][cCpPxX]\{1,\}/ { s!.*/\(.*\)\.[^.]*$!\1!; p }')
    if test "x$base" != "x" && test -f "${base}.${OBJEXT}" && \
       ! grep "include.*config\.h" "${srcdir}/${f}" >/dev/null; then
        echo "E: $f does not include config.h" >&2
        r=1
    fi
done

exit $r
