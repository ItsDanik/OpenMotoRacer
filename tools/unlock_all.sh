#!/bin/bash
# Every track of every list available in the player's folder, kept there for the next runs:
#   tools/unlock_all.sh [BINARY]     (OMR_HOME: another folder)
#   tools/unlock_all.sh --undo       the game's own lists again
cd "$(dirname "$0")/.."
if [ "$1" = --undo ]; then
    home=${OMR_HOME:-${XDG_CONFIG_HOME:-$HOME/.config}/openmotoracer}
    rm -f "$home/LIST.BPT" "$home"/LIST[0-9][0-9].BPT
    exit 0
fi
bin=${1:-port/build/openmotoracer}
tmp=$(mktemp)
for n in 0 1; do
    OMR_UNLOCK_ALL=1 OMR_LIST=$n OMR_PAGE=3 "$bin" --shot "$tmp" 1 > /dev/null 2>&1 || echo "list $n failed"
done
rm -f "$tmp"
