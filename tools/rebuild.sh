#!/bin/sh
# The Ghidra project and every function decompiled, from the game's Windows program.
#   tools/rebuild.sh              everything (logs in build/logs/)
#   tools/rebuild.sh --decomp     only decompile again (after new names in tools/names.txt)
# Everything it writes goes to build/ (git-ignored, never published).
# A PE program: Ghidra's own PE loader imports it (sections, imports by name, MSVC runtime
# recognised by Function ID); tools/pe2flat.py lays it out flat for the Python tools.
set -e
cd "$(dirname "$0")/.."
EXE=${EXE:-"Moto Racer/game.EXE"}   # the program, as the player's copy has it
NAME=${NAME:-mr}                    # the Ghidra project's name, build/<NAME>_flat.bin
PROG=$(basename "$EXE")             # the program's name inside the Ghidra project
G=/opt/ghidra/support/analyzeHeadless
P="build/ghidra $NAME"
L=build/${NAME}_layout.json

# from the program's layout: the code's range, where the BSS begins
lay() { python3 -I -c "
import json; d = json.load(open('$L'))
code = [o for o in d['objects'] if o['flags'] & 4]
data = [o for o in d['objects'] if o['name'] == '.data'][0]
print({'lo': hex(min(o['base'] for o in code)),
       'hi': hex(max(o['base'] + o['vsize'] for o in code)),
       'bss': hex(data['base'] + data['rawsize'])}['$1'])"; }

decompile() {
    rm -rf build/decomp && mkdir -p build/decomp
    DECOMP_BSS=$(lay bss) $G $P -process "$PROG" -noanalysis -scriptPath tools \
        -postScript DecompRange.java build/decomp $(lay lo) $(lay hi) > build/logs/decomp.log 2>&1
}

if [ "$1" = "--decomp" ]; then
    echo "rename";    $G $P -process "$PROG" -noanalysis -scriptPath tools \
        -postScript RenameFuncs.java tools/names.txt > build/logs/rename.log 2>&1
    echo "decompile"; decompile
    echo "done: $(wc -l < build/decomp/INDEX.txt) functions"; exit 0
fi

rm -rf build/ghidra build/decomp
mkdir -p build/ghidra build/logs build/decomp
python3 -I tools/pe2flat.py "$EXE" -o build/${NAME}_flat.bin --map $L
echo "import+analyse"; $G $P -import "$EXE" -analysisTimeoutPerFile 3600 \
    > build/logs/analyze.log 2>&1
echo "index";     DECOMP_BSS=$(lay bss) $G $P -process "$PROG" -noanalysis -scriptPath tools \
    -postScript DecompRange.java build/decomp $(lay lo) $(lay hi) > build/logs/decomp1.log 2>&1
python3 -I tools/seeds.py $L build/${NAME}_flat.bin build/decomp/INDEX.txt > build/seeds.txt
echo "seed";      $G $P -process "$PROG" -scriptPath tools -postScript SeedFuncs.java build/seeds.txt \
    > build/logs/seed.log 2>&1
[ -f tools/names.txt ] || touch tools/names.txt
echo "rename";    $G $P -process "$PROG" -noanalysis -scriptPath tools \
    -postScript RenameFuncs.java tools/names.txt > build/logs/rename.log 2>&1
echo "decompile"; decompile
echo "done: $(wc -l < build/decomp/INDEX.txt) functions"
