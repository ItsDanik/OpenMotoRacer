#!/bin/bash
# A race and its replay must be the same to the bit: one line a step, the sum of every
# rider's place and speed, for the race and again for its replay (OMR_RACELOG, OMR_WATCH_AT).
#   tools/replaycheck.sh [BINARY]
cd "$(dirname "$0")/.."
bin=${1:-port/build/openmotoracer}
tmp=build/replaycheck
mkdir -p "$tmp"
export OMR_HOME=$PWD/$tmp/home
rm -rf "$OMR_HOME"
keys="100:hold:up 500:hold:left 530:release:left 800:hold:right 860:release:right 1200:hold:left 1260:release:left 1600:hold:down 1660:release:down 1700:hold:right 1800:release:right 2000:hold:left 2100:release:left"
fail=0
check() {   # name choices steps
    OMR_RACELOG=$tmp/$1.log OMR_WATCH_AT=$3 OMR_CHOICES=$2 OMR_INPUT="$keys" \
        "$bin" --shot "$tmp/$1.bmp" $(($3 * 2 + 120)) > /dev/null 2>&1
    head -n "$3" "$tmp/$1.log" | cut -d" " -f2,3 > "$tmp/$1.race"
    awk '$1==1{print $2,$3}' "$tmp/$1.log" > "$tmp/$1.replay"
    n=$(wc -l < "$tmp/$1.replay")
    if [ "$n" -gt 0 ] && cmp -s "$tmp/$1.race" "$tmp/$1.replay"; then
        echo "$1: same over $n steps of $(wc -l < "$tmp/$1.race")"
    else
        echo "$1: DIFFERENT"
        diff "$tmp/$1.race" "$tmp/$1.replay" | head -4
        fail=1
    fi
}
export OMR_PAGE=1
check snow "1,1,3,0,0" 1500
check speedbay "1,0,3,0,0" 2400
check westway "2,4,2,0,0" 1800
exit $fail
